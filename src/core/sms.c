#include "sms.h"

#include <stdio.h>
#include <string.h>

static size_t sms_json_escape(const char *src, char *dst, size_t dst_len)
{
	size_t off = 0;

	if(!dst_len)
		return 0;
	for(size_t i = 0; src[i] != '\0' && off + 1 < dst_len; i++) {
		unsigned char c = (unsigned char)src[i];
		switch(c) {
		case '\\':
		case '"':
			if(off + 2 >= dst_len)
				goto done;
			dst[off++] = '\\';
			dst[off++] = (char)c;
			break;
		case '\b':
			if(off + 2 >= dst_len)
				goto done;
			dst[off++] = '\\';
			dst[off++] = 'b';
			break;
		case '\f':
			if(off + 2 >= dst_len)
				goto done;
			dst[off++] = '\\';
			dst[off++] = 'f';
			break;
		case '\n':
			if(off + 2 >= dst_len)
				goto done;
			dst[off++] = '\\';
			dst[off++] = 'n';
			break;
		case '\r':
			if(off + 2 >= dst_len)
				goto done;
			dst[off++] = '\\';
			dst[off++] = 'r';
			break;
		case '\t':
			if(off + 2 >= dst_len)
				goto done;
			dst[off++] = '\\';
			dst[off++] = 't';
			break;
		default:
			if(c < 0x20) {
				if(off + 6 >= dst_len)
					goto done;
				off += snprintf(dst + off, dst_len - off, "\\u%04x", c);
			} else {
				dst[off++] = (char)c;
			}
			break;
		}
	}
done:
	dst[off] = 0;
	return off;
}

static size_t sms_utf8_encode(uint16_t code, char *dst, size_t dst_len)
{
	if(code <= 0x7f) {
		if(dst_len < 1)
			return 0;
		dst[0] = (char)code;
		return 1;
	}
	if(code <= 0x7ff) {
		if(dst_len < 2)
			return 0;
		dst[0] = (char)(0xc0 | ((code >> 6) & 0x1f));
		dst[1] = (char)(0x80 | (code & 0x3f));
		return 2;
	}
	if(dst_len < 3)
		return 0;
	dst[0] = (char)(0xe0 | ((code >> 12) & 0x0f));
	dst[1] = (char)(0x80 | ((code >> 6) & 0x3f));
	dst[2] = (char)(0x80 | (code & 0x3f));
	return 3;
}

void sms_hex_encode(const uint8_t *src, size_t len, char *dst, size_t dst_len)
{
	static const char hex[] = "0123456789abcdef";
	size_t off = 0;
	for(size_t i = 0; i < len && off + 2 < dst_len; i++) {
		dst[off++] = hex[(src[i] >> 4) & 0x0f];
		dst[off++] = hex[src[i] & 0x0f];
	}
	dst[off] = 0;
}

static void sms_bcd_digits(const uint8_t *src, size_t src_len, char *dst, size_t dst_len, size_t digits)
{
	size_t off = 0;
	for(size_t i = 0; i < src_len && off + 1 < dst_len; i++) {
		uint8_t low = src[i] & 0x0f;
		uint8_t high = (src[i] >> 4) & 0x0f;
		if((i * 2) < digits && low <= 9)
			dst[off++] = '0' + low;
		if((i * 2 + 1) < digits && high != 0x0f && high <= 9 && off + 1 < dst_len)
			dst[off++] = '0' + high;
	}
	dst[off] = 0;
}

static size_t sms_decode_address_field(const uint8_t *buf, size_t len, uint8_t digits, sms_addr_t *addr)
{
	size_t addr_bytes;
	if(len < 1)
		return 0;
	memset(addr, 0, sizeof(*addr));
	addr->ton = (buf[0] >> 4) & 0x7;
	addr->npi = buf[0] & 0x0f;
	addr_bytes = (digits + 1) / 2;
	if(len < 1 + addr_bytes)
		return 0;
	sms_bcd_digits(buf + 1, addr_bytes, addr->digits, sizeof(addr->digits), digits);
	addr->present = 1;
	return 1 + addr_bytes;
}

static void sms_decode_timestamp(const uint8_t *buf, char *dst, size_t dst_len)
{
	char tmp[15] = {0};
	sms_bcd_digits(buf, 7, tmp, sizeof(tmp), 14);
	snprintf(dst, dst_len, "20%c%c-%c%c-%c%cT%c%c:%c%c:%c%c",
		tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5],
		tmp[6], tmp[7], tmp[8], tmp[9], tmp[10], tmp[11]);
}

static int sms_gsm7_to_char(uint8_t v)
{
	static const char table[128] = {
		'@','?','$','Y','e','e','u','i','o','C','\n','O','o','\r','A','a',
		'D','_','F','G','L','O','P','Y','S','T','X',27,'A','a','s','E',
		' ','!','"','#',-92,'%','&','\'', '(',')','*','+',',','-','.','/',
		'0','1','2','3','4','5','6','7','8','9',':',';','<','=','>','?',
		'I','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
		'P','Q','R','S','T','U','V','W','X','Y','Z',-60,-42,-47,-36,-89,
		-65,'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
		'p','q','r','s','t','u','v','w','x','y','z',-28,-10,-15,-4,-32
	};
	if(v == 27)
		return '^';
	if(v < sizeof(table))
		return table[v];
	return '.';
}

static void sms_decode_user_data_7bit(const uint8_t *src, size_t src_len, size_t septets,
		size_t skip_septets, char *dst, size_t dst_len)
{
	size_t out = 0;
	for(size_t i = skip_septets; i < septets && out + 1 < dst_len; i++) {
		size_t bit = i * 7;
		size_t byte = bit / 8;
		unsigned shift = bit % 8;
		uint8_t c = 0;
		if(byte < src_len)
			c = (src[byte] >> shift) & 0x7f;
		if(shift && byte + 1 < src_len)
			c |= (src[byte + 1] << (8 - shift)) & 0x7f;
		dst[out++] = (char)sms_gsm7_to_char(c);
	}
	dst[out] = 0;
}

static void sms_decode_user_data_8bit(const uint8_t *src, size_t src_len, char *dst, size_t dst_len)
{
	size_t count = (src_len < dst_len - 1) ? src_len : dst_len - 1;
	for(size_t i = 0; i < count; i++)
		dst[i] = (src[i] >= 32 && src[i] < 127) ? (char)src[i] : '.';
	dst[count] = 0;
}

static void sms_decode_user_data_ucs2(const uint8_t *src, size_t src_len, char *dst, size_t dst_len)
{
	size_t out = 0;
	for(size_t i = 0; i + 1 < src_len && out + 1 < dst_len; i += 2) {
		uint16_t code = ((uint16_t)src[i] << 8) | src[i + 1];
		size_t written = sms_utf8_encode(code, dst + out, dst_len - out - 1);
		if(!written)
			break;
		out += written;
	}
	dst[out] = 0;
}

static void sms_decode_dcs(uint8_t dcs, sms_dcs_info_t *info)
{
	memset(info, 0, sizeof(*info));
	info->raw = dcs;
	if((dcs & 0xf0) == 0xf0) {
		info->alphabet = (dcs & 0x04) ? SMS_ALPHABET_8BIT : SMS_ALPHABET_GSM7;
		info->has_message_class = 1;
		info->message_class = dcs & 0x03;
		info->class_meaning_valid = 1;
		info->marked_for_deletion = (dcs & 0x08) ? 1 : 0;
	} else if((dcs & 0xc0) == 0x00) {
		info->compressed = (dcs >> 5) & 0x01;
		info->has_message_class = (dcs >> 4) & 0x01;
		switch((dcs >> 2) & 0x03) {
		case 0: info->alphabet = SMS_ALPHABET_GSM7; break;
		case 1: info->alphabet = SMS_ALPHABET_8BIT; break;
		case 2: info->alphabet = SMS_ALPHABET_UCS2; break;
		default: info->alphabet = SMS_ALPHABET_RESERVED; break;
		}
		if(info->has_message_class) {
			info->message_class = dcs & 0x03;
			info->class_meaning_valid = 1;
		}
	} else {
		switch((dcs >> 2) & 0x03) {
		case 0: info->alphabet = SMS_ALPHABET_GSM7; break;
		case 1: info->alphabet = SMS_ALPHABET_8BIT; break;
		case 2: info->alphabet = SMS_ALPHABET_UCS2; break;
		default: info->alphabet = SMS_ALPHABET_RESERVED; break;
		}
	}
	switch(info->alphabet) {
	case SMS_ALPHABET_GSM7: strcpy(info->alphabet_name, "gsm7"); break;
	case SMS_ALPHABET_8BIT: strcpy(info->alphabet_name, "8bit"); break;
	case SMS_ALPHABET_UCS2: strcpy(info->alphabet_name, "ucs2"); break;
	default: strcpy(info->alphabet_name, "reserved"); break;
	}
	snprintf(info->description, sizeof(info->description),
		"alphabet=%s%s%s",
		info->alphabet_name,
		info->compressed ? ", compressed" : "",
		info->has_message_class ? ", class-indicated" : "");
}

static int sms_decode_user_data(const uint8_t *src, size_t src_len, uint8_t udl,
		const sms_dcs_info_t *dcs, uint8_t udhi, sms_user_data_t *ud)
{
	size_t payload_off = 0;
	size_t payload_len = src_len;
	size_t skip_septets = 0;
	memset(ud, 0, sizeof(*ud));
	ud->length = udl;
	ud->header_present = udhi;
	if(udhi) {
		if(src_len < 1)
			return -1;
		ud->header_length = src[0];
		if(src_len < (size_t)ud->header_length + 1)
			return -1;
		payload_off = ud->header_length + 1;
		payload_len = src_len - payload_off;
		sms_hex_encode(src, payload_off, ud->header_hex, sizeof(ud->header_hex));
		skip_septets = (payload_off * 8 + 6) / 7;
		ud->septet_padding = (uint8_t)((skip_septets * 7) - payload_off * 8);
	}
	sms_hex_encode(src + payload_off, payload_len, ud->data_hex, sizeof(ud->data_hex));
	switch(dcs->alphabet) {
	case SMS_ALPHABET_GSM7:
		sms_decode_user_data_7bit(src, src_len, udl, skip_septets, ud->text, sizeof(ud->text));
		ud->text_valid = 1;
		break;
	case SMS_ALPHABET_8BIT:
		sms_decode_user_data_8bit(src + payload_off, payload_len < udl ? payload_len : udl,
			ud->text, sizeof(ud->text));
		ud->text_valid = 1;
		break;
	case SMS_ALPHABET_UCS2:
		sms_decode_user_data_ucs2(src + payload_off, payload_len < udl ? payload_len : udl,
			ud->text, sizeof(ud->text));
		ud->text_valid = 1;
		break;
	default:
		break;
	}
	return 0;
}

int sms_decode_tpdu(const uint8_t *buf, size_t len, sms_message_t *out)
{
	size_t off = 0, consumed = 0;
	uint8_t first_octet;
	if(!buf || !out || !len)
		return -1;
	memset(out, 0, sizeof(*out));
	first_octet = buf[off++];
	out->first_octet = first_octet;

	switch(first_octet & 0x03) {
	case 0:
		out->type = SMS_TPDU_DELIVER;
		out->flags.deliver.mti = first_octet & 0x03;
		out->flags.deliver.mms = (first_octet >> 2) & 0x01;
		out->flags.deliver.lp = (first_octet >> 3) & 0x01;
		out->flags.deliver.sri = (first_octet >> 5) & 0x01;
		out->flags.deliver.udhi = (first_octet >> 6) & 0x01;
		out->flags.deliver.rp = (first_octet >> 7) & 0x01;
		if(len < off + 1)
			return -1;
		consumed = sms_decode_address_field(buf + off + 1, len - off - 1, buf[off], &out->tp_originating_address);
		if(!consumed)
			return -1;
		off += 1 + consumed;
		if(len < off + 9)
			return -1;
		out->tp_pid = buf[off++];
		sms_decode_dcs(buf[off++], &out->tp_dcs);
		sms_decode_timestamp(buf + off, out->tp_service_centre_time_stamp,
			sizeof(out->tp_service_centre_time_stamp));
		out->tp_service_centre_time_stamp_present = 1;
		off += 7;
		if(len < off + 1)
			return -1;
		if(sms_decode_user_data(buf + off + 1, len - off - 1, buf[off], &out->tp_dcs,
			out->flags.deliver.udhi, &out->tp_user_data))
			return -1;
		off += 1;
		break;
	case 1:
		out->type = SMS_TPDU_SUBMIT;
		out->flags.submit.mti = first_octet & 0x03;
		out->flags.submit.rd = (first_octet >> 2) & 0x01;
		out->flags.submit.vpf = (first_octet >> 3) & 0x03;
		out->flags.submit.srr = (first_octet >> 5) & 0x01;
		out->flags.submit.udhi = (first_octet >> 6) & 0x01;
		out->flags.submit.rp = (first_octet >> 7) & 0x01;
		if(len < off + 2)
			return -1;
		out->tp_message_reference = buf[off++];
		out->tp_message_reference_present = 1;
		consumed = sms_decode_address_field(buf + off + 1, len - off - 1, buf[off], &out->tp_destination_address);
		if(!consumed)
			return -1;
		off += 1 + consumed;
		if(len < off + 2)
			return -1;
		out->tp_pid = buf[off++];
		sms_decode_dcs(buf[off++], &out->tp_dcs);
		out->tp_validity_period_format = out->flags.submit.vpf;
		if(out->flags.submit.vpf == 2) {
			if(len < off + 7)
				return -1;
			sms_decode_timestamp(buf + off, out->tp_validity_period_absolute,
				sizeof(out->tp_validity_period_absolute));
			out->tp_validity_period_present = 1;
			off += 7;
		} else if(out->flags.submit.vpf == 1 || out->flags.submit.vpf == 3) {
			if(len < off + 1)
				return -1;
			out->tp_validity_period_relative = buf[off++];
			out->tp_validity_period_present = 1;
		}
		if(len < off + 1)
			return -1;
		if(sms_decode_user_data(buf + off + 1, len - off - 1, buf[off], &out->tp_dcs,
			out->flags.submit.udhi, &out->tp_user_data))
			return -1;
		off += 1;
		break;
	default:
		return -1;
	}
	out->valid = 1;
	return 0;
}

size_t sms_json_print(const sms_message_t *sms, char *buf, size_t size)
{
	int off = 0;
	char escaped_text[SMS_MAX_USER_DATA_TEXT * 6];
	if(!sms || !sms->valid)
		return snprintf(buf, size, "{}");
	off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
		"{\"type\":\"%s\",\"firstOctet\":%u",
		sms->type == SMS_TPDU_DELIVER ? "sms-deliver" : "sms-submit",
		sms->first_octet);
	if(sms->type == SMS_TPDU_DELIVER) {
		off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
			",\"flags\":{\"tpMti\":%u,\"tpMms\":%u,\"tpLp\":%u,\"tpSri\":%u,\"tpUdhi\":%u,\"tpRp\":%u}",
			sms->flags.deliver.mti, sms->flags.deliver.mms, sms->flags.deliver.lp,
			sms->flags.deliver.sri, sms->flags.deliver.udhi, sms->flags.deliver.rp);
	} else {
		off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
			",\"flags\":{\"tpMti\":%u,\"tpRd\":%u,\"tpVpf\":%u,\"tpSrr\":%u,\"tpUdhi\":%u,\"tpRp\":%u}",
			sms->flags.submit.mti, sms->flags.submit.rd, sms->flags.submit.vpf,
			sms->flags.submit.srr, sms->flags.submit.udhi, sms->flags.submit.rp);
	}
	if(sms->tp_originating_address.present)
		off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
			",\"tpOriginatingAddress\":{\"digits\":\"%s\",\"ton\":%u,\"npi\":%u}",
			sms->tp_originating_address.digits, sms->tp_originating_address.ton,
			sms->tp_originating_address.npi);
	if(sms->tp_destination_address.present)
		off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
			",\"tpDestinationAddress\":{\"digits\":\"%s\",\"ton\":%u,\"npi\":%u}",
			sms->tp_destination_address.digits, sms->tp_destination_address.ton,
			sms->tp_destination_address.npi);
	if(sms->tp_message_reference_present)
		off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
			",\"tpMessageReference\":%u", sms->tp_message_reference);
	off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
		",\"tpPid\":%u,\"tpDcs\":{\"raw\":%u,\"alphabet\":\"%s\",\"description\":\"%s\",\"compressed\":%u,\"hasMessageClass\":%u",
		sms->tp_pid, sms->tp_dcs.raw, sms->tp_dcs.alphabet_name,
		sms->tp_dcs.description, sms->tp_dcs.compressed,
		sms->tp_dcs.has_message_class);
	if(sms->tp_dcs.class_meaning_valid)
		off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
			",\"messageClass\":%u", sms->tp_dcs.message_class);
	if(sms->tp_dcs.marked_for_deletion)
		off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
			",\"markedForDeletion\":1");
	off += snprintf(buf + off, size > (size_t)off ? size - off : 0, "}");
	if(sms->tp_service_centre_time_stamp_present)
		off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
			",\"tpServiceCentreTimeStamp\":\"%s\"", sms->tp_service_centre_time_stamp);
	if(sms->tp_validity_period_present) {
		if(sms->tp_validity_period_format == 2)
			off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
				",\"tpValidityPeriod\":{\"format\":%u,\"absolute\":\"%s\"}",
				sms->tp_validity_period_format, sms->tp_validity_period_absolute);
		else
			off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
				",\"tpValidityPeriod\":{\"format\":%u,\"relative\":%u}",
				sms->tp_validity_period_format, sms->tp_validity_period_relative);
	}
	off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
		",\"tpUserData\":{\"tpUserDataLength\":%u,\"hasHeader\":%u",
		sms->tp_user_data.length, sms->tp_user_data.header_present);
	if(sms->tp_user_data.header_present)
		off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
			",\"tpUserDataHeaderLength\":%u,\"tpUserDataHeaderHex\":\"%s\",\"septetPaddingBits\":%u",
			sms->tp_user_data.header_length, sms->tp_user_data.header_hex,
			sms->tp_user_data.septet_padding);
	if(sms->tp_user_data.text_valid)
		sms_json_escape(sms->tp_user_data.text, escaped_text,
			sizeof(escaped_text));
	if(sms->tp_user_data.text_valid)
		off += snprintf(buf + off, size > (size_t)off ? size - off : 0,
			",\"decoded\":\"%s\"", escaped_text);
	off += snprintf(buf + off, size > (size_t)off ? size - off : 0, "}}");
	return off;
}
