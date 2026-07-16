#include "diameter.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <endian.h>
#include "../common.h"
#include "../val-str.h"

static const struct val_str avp_type_table[] = { 
	{ OctetString, 		"OctetString" },
	{ Integer32, 		"Integer32" },
	{ Integer64, 		"Integer64" },
	{ Unsigned32, 		"Unsigned32" },
	{ Unsigned64, 		"Unsigned64" },
	{ Float32, 		"Float32" },
	{ Float64, 		"Float64" },
	{ Grouped, 		"Grouped" },
	{ 0, NULL } 
};

static const struct val_str os_derived_table[] = {
	{ OS_OctetString, 	"OctetString" },
	{ OS_Address, 		"Address" },
	{ OS_Time, 		"Time" },
	{ OS_UTF8String, 	"UTF8String" },
	{ OS_DiameterIdentity, 	"DiameterIdentity" },
	{ OS_DiameterURI, 	"DiameterURI" },
	{ OS_IPFilterRule, 	"IPFilterRule" },
	{ OS_QoSFilterRule, 	"QoSFilterRule" },
	{0, NULL},
};

int _serialize_diameter_avp(struct diameter_avp *avp, uint8_t *buf);

static int _create_os_avp(struct diameter_avp *avp, char *val, int val_sz)
{
	if (val_sz > 0) {
		avp->data.octetstring = malloc(val_sz + 1);
		memcpy(avp->data.octetstring, val, val_sz);
		avp->data.octetstring[val_sz] = '\0';

		//		avp->pad = PAD(avp->data.octetstring);

		free(val);
	}
	return 0;
}
static int _create_i32_avp(struct diameter_avp *avp, int32_t val, int val_sz)
{
	avp->data.int32 = htonl(val);
	return 0;
}
static int _create_i64_avp(struct diameter_avp *avp, int64_t val, int val_sz)
{
	avp->data.int64 = htobe64(val);
	return 0;
}
static int _create_u32_avp(struct diameter_avp *avp, uint32_t val, int val_sz)
{
	avp->data.unsigned32 = htonl(val);
	return 0;
}
static int _create_u64_avp(struct diameter_avp *avp, uint64_t val, int val_sz)
{
	avp->data.unsigned64 = htobe64(val);
	return 0;
}
static int _create_grouped_avp(struct diameter_avp *avp,
			       struct diameter_avp **arr, int val_sz)
{
	if (val_sz) {
		avp->data.group = arr;
	}
	return 0;
}
static int _create_f32_avp(struct diameter_avp *avp, float val, int val_sz)
{
	return 0;
}

static int _create_f64_avp(struct diameter_avp *avp, double val, int val_sz)
{
	return 0;
}

static void _create_avp_data(struct diameter_avp *avp, avp_type type,
			     uint64_t data, int data_sz)
{
	switch (type) {
	case OctetString: 
		_create_os_avp(avp, (char *)data, data_sz);
		break;
	case Integer32: 
		_create_i32_avp(avp, (int32_t)data, data_sz);
		break;
	case Integer64:
		_create_i64_avp(avp, (int64_t)data, data_sz);
		break;
	case Unsigned32:
		_create_u32_avp(avp, (uint32_t)data, data_sz);
		break;
	case Unsigned64:
		_create_u64_avp(avp, (uint64_t)data, data_sz);
		break;
	case Float32:
		_create_f32_avp(avp, (float)data, data_sz);
		break;
	case Float64:
		_create_f64_avp(avp, (double)data, data_sz);
		break;
	case Grouped:
		_create_grouped_avp(avp, (struct diameter_avp **)data, data_sz);
		break;
	case Invalid:
	case Unknown:
	default:
		break;
	}
}
struct diameter_avp *diameter_new_avp(avp_type type, unsigned int code,
				      unsigned char flags, uint64_t data,
				      int avp_length, unsigned int vendor_id)
{
	struct diameter_avp *avp;
	int header_size = 0;

	avp = malloc(sizeof(struct diameter_avp));
	memset(avp, 0, sizeof(struct diameter_avp));

	avp->type = type;
	//	AVP_HEADER(avp).length = sizeof(struct diameter_avp_hdr);
	AVP_HEADER(avp).length = avp_length;
	AVP_HEADER(avp).code = code;
	AVP_HEADER(avp).flags = flags;
	header_size = 8;

	if (avp_length < 8 || (flags & AVP_FLAG_VENDOR && avp_length < 12)) {
		return avp;
	}

	if (flags & AVP_FLAG_VENDOR) {
		avp->vendor_id = vendor_id;
		header_size += SIZEOF_VENDOR_ID;
	}

	_create_avp_data(avp, type, data, avp_length - header_size);

	return avp;
}

struct diameter_pkt *diameter_new_packet()
{
	struct diameter_pkt *pkt = calloc(1, sizeof(struct diameter_pkt));
	pkt->header.length = 20;
	return pkt;
}

static void _avp_free(struct diameter_avp **avp)
{
	struct diameter_avp *curr, *next;
	if ((*avp)->type == Grouped) {
		curr = (struct diameter_avp *)(*avp)->value;
		while (curr) {
			next = curr->next;
			_avp_free(&curr);
			curr = next;
		}
	}
	if ((*avp)->type == OctetString)
		free((char *)((*avp)->value));
	free(*avp);
	*avp = NULL;
}

void diameter_packet_free(struct diameter_pkt *pkt)
{
	struct diameter_avp *curr, *next;
	for (curr = pkt->avp_list; curr; curr = next) {
		next = curr->next;
		_avp_free(&curr);
	}
	free(pkt);
}

void diameter_insert_avp(struct diameter_pkt *pkt, struct diameter_avp *obj)
{
	struct diameter_avp *head = pkt->avp_list, *curr;

	curr = head;
	while (curr->next)
		curr = curr->next;
	curr->next = obj;

	pkt->header.length += AVP_HEADER(obj).length + obj->pad;
}

void diameter_insert_avp_after(struct diameter_pkt *pkt,
			       struct diameter_avp *node,
			       struct diameter_avp *newavp)
{
	struct diameter_avp *head = pkt->avp_list, *curr;
	for (curr = head; curr; curr = curr->next) {
		if (curr == node) {
			newavp->next = curr->next;
			curr->next = newavp;
			pkt->header.length +=
				AVP_HEADER(newavp).length + newavp->pad;
			return;
		}
	}
}

void diameter_insert_avp_before(struct diameter_pkt *pkt,
				struct diameter_avp *avp)
{
}

void diameter_swap_avp(const struct diameter_pkt *pkt,
		       struct diameter_avp **first,
		       struct diameter_avp **second)
{
	struct diameter_avp *tmp = NULL, *f_previous = NULL, *s_previous = NULL,
			    *curr_avp, *head = pkt->avp_list;

	for (curr_avp = head; curr_avp; curr_avp = curr_avp->next) {
		if (curr_avp->next == *first)
			f_previous = curr_avp;
		if (curr_avp->next == *second)
			s_previous = curr_avp;
	}

	assert(f_previous && s_previous);

	f_previous->next = *second;
	s_previous->next = *first;
	tmp = (*first)->next;
	(*first)->next = (*second)->next;
	(*second)->next = tmp;
}

void diameter_remove_avp(struct diameter_pkt *pkt, struct diameter_avp *avp)
{
	struct diameter_avp *head, *iterator = NULL, *previous;
	head = pkt->avp_list;
	if (avp == head) {
		iterator = head;
		pkt->avp_list = iterator->next;
	} else {
		for (iterator = head; iterator; iterator = iterator->next) {
			if (iterator->next == avp)
				previous = iterator;
			if (iterator == avp) {
				previous->next = iterator->next;
			}
		}
	}

	if (iterator) {
		if (iterator->type == OctetString)
			free(iterator->data.octetstring);
		if (iterator->type == Grouped)
			;

		pkt->header.length -= (iterator->header.length + iterator->pad);
		free(iterator);
	}
}

//typedef char *( os_formatter_fn)( char *);

static int hex_os_formatter(char **o_str, char *i_str)
{
	int index = 0;
	int len = strlen(i_str) / 2 + (strlen(i_str) % 2);
	*o_str = malloc(len + 1);
	memset(*o_str, 0, len + 1);
	for (size_t i = 0; i < strlen(i_str); i += 2) {
		(*o_str)[index] = 0;
		if ((i_str[i] & 0xf0) == 0x30)
			(*o_str)[index] = ((i_str[i] & 0x0f) << 4);
		else
			(*o_str)[index] =
				((((i_str[i] - 1) & 0x0f) + 0x0a) << 4);
		if ((i_str[i + 1] & 0xf0) == 0x30)
			(*o_str)[index] |= (i_str[i + 1] & 0x0f);
		else
			(*o_str)[index] |= (((i_str[i + 1] - 1) & 0x0f) + 0x0a);
		index++;
	}
	(*o_str)[len] = '\0';
	return len;
}
/*
int tbcd_os_formatter(char **o_str, char *i_str)
{
	int index = 0;
	int len = strlen(i_str) / 2 + (strlen(i_str) % 2);
	*o_str = malloc(len + 1);
	memset(*o_str, 0, len + 1);
	for (size_t i = 0; i < strlen(i_str); i += 2) {
		(*o_str)[index] |= (i_str[i] & 0x0f);
		(*o_str)[index] |= ((i_str[i + 1] & 0x0f) << 4);
		index++;
	}
	(*o_str)[len] = '\0';
	return len;
}

int vplmn_os_formatter(char **vplmn, char *i_str)
{
	int len = 3;
	*vplmn = malloc(4);

	(*vplmn)[0] = (i_str[0] - '0') << 4 | (i_str[1] - '0');
	(*vplmn)[1] = (i_str[2] - '0' < 9 ? (i_str[2] - '0') : 0x0f) << 4 |
		      (i_str[3] - '0');
	(*vplmn)[2] = (i_str[4] - '0') << 4 | (i_str[5] - '0');
	(*vplmn)[3] = '\0';

	return len;
}
*/

static int default_os_formatter(char **o_str, char *i_str)
{
	int len = strlen(i_str);

	if (len > 0) {
		*o_str = malloc(len + 1);
		memcpy((*o_str), i_str, len);
		(*o_str)[len] = '\0';
	}
	return len;
}

typedef int (*os_format_fn)(char **, char *);

static os_format_fn override_os_formatter(os_derived type)
{
	switch (type) {
	case OS_OctetString:
	case OS_Address:
		return hex_os_formatter;
	case OS_Invalid:
	case OS_Unknown:
	case OS_Time:
	case OS_UTF8String:
	case OS_DiameterIdentity:
	case OS_DiameterURI:
	case OS_IPFilterRule:
	case OS_QoSFilterRule:
	default:
		return default_os_formatter;
	}
}

static int read_os_json_value(char **o_str, char *i_str, 
	os_format_fn formatter)
{
	return formatter(o_str, i_str);
}

static int _parse_json_avp_array(struct diameter_avp **head, json_t *avp_arr)
{
	int ret = 0;

	uint64_t value;
	unsigned value_sz = 0;
	unsigned avp_count = 0;
	size_t index = 0;
	avp_type type;
	unsigned int vendorId = 0;

	json_t *avp_obj;
	json_t *avp_value_obj;

	const char *avp_name;
	const char *avp_type;
	const char *flag_str;
	unsigned int avp_code = 0;
	unsigned char avp_flags = 0;
	unsigned avp_length = 0;

	struct diameter_avp **curr = head;

	avp_count = json_array_size(avp_arr);

	for (; index < avp_count; index++) {
		avp_code = 0;
		avp_flags = 0;
		avp_obj = json_array_get(avp_arr, index);

/*		if (type < 0) {
			printf("Invalid avp type : %s\n",
			       json_string_value(
				       json_object_get(avp_obj, "type")));
			return -1;
		}
*/
		avp_code = json_integer_value(json_object_get(avp_obj, "code"));
		if (!avp_code) {
			fprintf(stderr, "AVP code must be specified\n");
			return -1;
		}

		avp_name = json_string_value(json_object_get(avp_obj, "name"));
		if (!avp_name) {
			fprintf(stderr, "Name must be specified for avp: %d.\n",
				avp_code);
			return -1;
		}
		
		avp_type = json_string_value(json_object_get(avp_obj, "type"));
		if(!avp_type) {
			fprintf(stderr, "Type for %s avp have to be specified.\n",
				avp_name);
			return -1;
		}
		type = value_from_string(
			json_string_value(json_object_get(avp_obj, "type")),
			avp_type_table);


		flag_str = json_string_value(json_object_get(avp_obj, "flags"));
		if (!flag_str) {
			fprintf(stderr,
				"Invalid flag field for avp: %s (%d).\n",
				avp_name, avp_code);
			return -1;
		}
		if (strlen(flag_str) > 8) {
			fprintf(stderr, "Invalid flag for avp %d: %s.\n",
				avp_code, flag_str);
			return -1;
		}
		if (strchr(flag_str, 'v')) {
			fprintf(stderr, "Use V for vendor bit.\n");
			return -1;
		}
		if (strchr(flag_str, 'm')) {
			fprintf(stderr, "Use M for mandatory bit.\n");
			return -1;
		}
		if (strchr(flag_str, 'p')) {
			fprintf(stderr, "Use P for protected bit.\n");
			return -1;
		}
		if (strchr(flag_str, 'V'))
			avp_flags += AVP_FLAG_VENDOR;
		if (strchr(flag_str, 'M'))
			avp_flags += AVP_FLAG_MANDATORY;
		if (strchr(flag_str, 'P'))
			avp_flags += AVP_FLAG_PROTECTED;
		if (strchr(flag_str, '1'))
			avp_flags += AVP_FLAG_RESERVED_4;
		if (strchr(flag_str, '2'))
			avp_flags += AVP_FLAG_RESERVED_3;
		if (strchr(flag_str, '3'))
			avp_flags += AVP_FLAG_RESERVED_2;
		if (strchr(flag_str, '4'))
			avp_flags += AVP_FLAG_RESERVED_1;
		if (strchr(flag_str, '5'))
			avp_flags += AVP_FLAG_RESERVED_0;
		if (avp_flags & AVP_FLAG_VENDOR) {
			json_t *vendorObj =
				json_object_get(avp_obj, "vendor-id");
			if (!vendorObj) {
				fprintf(stderr,
					ANSI_COLOR_YELLOW
					"Warining" ANSI_COLOR_RESET ": "
					"Vendor bit for avp %s (%d) is set but "
					"no vendor-id value has been specified.\n",
					avp_name, avp_code);
				;
			} else {
				vendorId = json_integer_value(vendorObj);
			}
		}

		avp_length =
			json_integer_value(json_object_get(avp_obj, "length"));

		avp_value_obj = json_object_get(avp_obj, "value");

		value_sz = 0;
		if (avp_value_obj) {
			switch (json_typeof(avp_value_obj)) {
			case JSON_STRING: {
				type = OctetString;
				os_derived os_type; 
				os_type = value_from_string(avp_type,
							os_derived_table);
					value_sz = read_os_json_value(
						(char **)&value,
						(char *)json_string_value(
						avp_value_obj),
						override_os_formatter(os_type));
		//			if(value_sz < 0)
		//				continue;
				break;
			}
			case JSON_INTEGER: {
				value = json_integer_value(avp_value_obj);
				if (type == Integer64 || type == Unsigned64 ||
				    type == Float64) {
					type = Integer64;
					value_sz = 8;

				} else {
					type = Integer32;
					value_sz = 4;
				}
				break;
			}
			case JSON_ARRAY: {
				json_array_size(avp_value_obj);
				struct diameter_avp *child_arr;
				value_sz = _parse_json_avp_array(&child_arr,
								 avp_value_obj);
				value = (uint64_t)child_arr;
				break;
			}
			case JSON_OBJECT:
			case JSON_REAL:
			case JSON_TRUE:
			case JSON_FALSE:
			case JSON_NULL:
			default:
				break;
			}
		}

		if (!avp_length) {
			avp_length = value_sz + AVP_HEADER_SIZE +
				     ((avp_flags & AVP_FLAG_VENDOR) ?
					      SIZEOF_VENDOR_ID :
					      0);
		}
		*curr = diameter_new_avp(type, avp_code, avp_flags, value,
					 avp_length, vendorId);
		curr = &(*curr)->next;

		value = 0;
		ret += PAD4(avp_length);
	}
	return ret;
}

int diameter_parse_json(struct diameter_pkt *pkt, json_t *diam_obj)
{
	const char *value;
	json_t *obj, *hdr_obj;
	int body_size;

	hdr_obj = json_object_get(diam_obj, "common-header");

	/* Version */
	obj = json_object_get(hdr_obj, "version");
	if (obj)
		pkt->header.version = json_integer_value(obj);
	else
		pkt->header.version = 1;

	/* Flags */
	obj = json_object_get(hdr_obj, "flags");
	if (!obj) {
		fprintf(stderr, "Error: flags field must be specified.\n");
		return -1;
	}
	value = json_string_value(obj);
	if (!value) {
		fprintf(stderr, "Unable to read flags.\n");
		return -1;
	}
	if (strlen(value) > 4) {
		fprintf(stderr, "Invalid value for flags field.\n");
		return -1;
	}
	if (strchr(value, 'r')) {
		fprintf(stderr, "Use R for Request bit.\n");
		return -1;
	}
	if (strchr(value, 'p')) {
		fprintf(stderr, "Use P for Proxyable bit.\n");
		return -1;
	}
	if (strchr(value, 'e')) {
		fprintf(stderr, "Use E for Error bit.\n");
		return -1;
	}
	if (strchr(value, 't')) {
		fprintf(stderr, "Use T for Potentially re-transmitted bit.\n");
		return -1;
	}
	if (strchr(value, 'R'))
		pkt->header.flags += FLAG_REQUEST;
	if (strchr(value, 'P'))
		pkt->header.flags += FLAG_PROXYABLE;
	if (strchr(value, 'E'))
		pkt->header.flags += FLAG_ERROR;
	if (strchr(value, 'T'))
		pkt->header.flags += FLAG_RE_TRANS;

	/* Command-Code */
	obj = json_object_get(hdr_obj, "command-code");
	if (!obj) {
		fprintf(stderr, "Error: command-code must be specified.\n");
		return -1;
	} else
		pkt->header.command_code = json_integer_value(obj);

	/* Application-Id */
	obj = json_object_get(hdr_obj, "application-id");
	if (!obj) {
		printf("Error: application-id must be specified.\n");
	} else
		pkt->header.application_id = json_integer_value(obj);

	/* Hop-by-Hop-Id */
	obj = json_object_get(hdr_obj, "hop-by-hop-id");
	if (obj)
		pkt->header.hop_by_hop_id = json_integer_value(obj);
	else
		pkt->header.hop_by_hop_id = 0;

	/* End-to-End-Id */
	obj = json_object_get(hdr_obj, "end-to-end-id");
	if (obj)
		pkt->header.end_to_end_id = json_integer_value(obj);
	else
		pkt->header.end_to_end_id = 0;

	/* AVP list */
	obj = json_object_get(diam_obj, "avps");
	if (!obj) {
		printf("Error: avps list must be specified\n");
		return -1;
	} else
		body_size = _parse_json_avp_array(&pkt->avp_list, obj);

	if (body_size < 0) {
		printf("Failed to parse json file. Cause : errors in avp array.\n");
		return -1;
	}

	pkt->header.length += body_size;

	/* Whole Packet lenght */
	/* Caution: this field is only for creating corrupted packets,
	 * if you don't know what you are doing, ignore it. */
	obj = json_object_get(hdr_obj, "length");
	if (obj) {
		printf(ANSI_COLOR_YELLOW
		       "Caution: length may not be correct!" ANSI_COLOR_RESET
		       "\n");
		pkt->header.length = json_integer_value(obj);
	}

	return 0;
}
/* Depricated */
struct diameter_pkt *diameter_read_json_packet(const char *buf)
{
	json_t *json, *dimObj;
	json_error_t error;
	struct diameter_pkt *pkt;

	json = json_loads(buf, 0, &error);
	if (!json) {
		printf("Unable to parse buffer. line : %d, error :%s\n",
		       error.line, error.text);
		return NULL;
	}

	pkt = diameter_new_packet();
	dimObj = json_object_get(json, "diameter");

	if (diameter_parse_json(pkt, dimObj))
		return NULL;

	json_decref(json);
	return pkt;
}

static void _serialize_octetstring(uint8_t *buf, char *ostr, int len)
{
	memcpy(buf, ostr, len);
	memset(buf + len, 0, PAD4(len));
}

static void _serialize_integer32(uint8_t *buf, int32_t val, int size)
{
	memcpy(buf, &val, size);
}

static void _serialize_integer64(uint8_t *buf, int64_t val, int size)
{
	memcpy(buf, &val, size);
}

static void _serialize_unsigned32(uint8_t *buf, uint32_t val, int size)
{
	memcpy(buf, &val, size);
}

static void _serialize_unsigned64(uint8_t *buf, uint64_t val, int size)
{
	memcpy(buf, &val, size);
}

static void _serialize_float32(uint8_t *buf, float val, int size)
{
	memcpy(buf, &val, size);
}

static void _serialize_float64(uint8_t *buf, double val, int size)
{
	memcpy(buf, &val, size);
}

static int _serialize_group(uint8_t *buf, struct diameter_avp *arr, int size)
{
	int ret = 0;
	while (arr) {
		ret += _serialize_diameter_avp(arr, buf + ret);
		arr = arr->next;
	}
	return ret;
}

static void _serialize_avp_data(struct diameter_avp *avp, uint8_t *buf, int size)
{
	switch (avp->type) {
	case OctetString:
		_serialize_octetstring(buf, avp->data.octetstring, size);
		break;
	case Integer32:
		_serialize_integer32(buf, avp->data.int32, size);
		break;
	case Integer64:
		_serialize_integer64(buf, avp->data.int64, size);
		break;
	case Unsigned32:
		_serialize_unsigned32(buf, avp->data.unsigned32, size);
		break;
	case Unsigned64:
		_serialize_unsigned64(buf, avp->data.unsigned64, size);
		break;
	case Float32:
		_serialize_float32(buf, avp->data.float32, size);
		break;
	case Float64:
		_serialize_float64(buf, avp->data.float64, size);
		break;
	case Grouped:
		_serialize_group(buf, avp->data.group, size);
		break;
	case Invalid:
	default:
		break;
	}
}

int _serialize_diameter_avp(struct diameter_avp *avp, uint8_t *buf)
{
	int ret = 0;
	int offt = 0;
	struct diameter_avp_hdr hdr = { 0 };

	hdr.code = htonl(AVP_HEADER(avp).code);
	hdr.flags = AVP_HEADER(avp).flags;
	hdr.length = ((AVP_HEADER(avp).length & 0x0000ff) << 16) |
		     ((AVP_HEADER(avp).length & 0x00ff00)) |
		     ((AVP_HEADER(avp).length & 0xff0000) >> 16);
	ret = PAD4(AVP_HEADER(avp).length);

	if (AVP_HEADER(avp).length < 8) {
		memcpy(buf, &hdr, AVP_HEADER(avp).length);
		return ret;
	} else {
		memcpy(buf, &hdr, sizeof(struct diameter_avp_hdr));
		offt += 8;
	}

	//	ret = sizeof(struct diameter_avp_hdr);
	if (AVP_HEADER(avp).flags & AVP_FLAG_VENDOR) {
		if (AVP_HEADER(avp).length < 12)
			return ret;
		else {
			uint32_t vendor_hton = htonl(avp->vendor_id);
			memcpy(buf + offt, &vendor_hton, SIZEOF_VENDOR_ID);
			offt += SIZEOF_VENDOR_ID;
		}
	}
	_serialize_avp_data(avp, buf + offt, AVP_HEADER(avp).length - offt);

	return ret;
}

int diameter_serialize_packet(const struct diameter_pkt *pkt, uint8_t *buf)
{
	int offt = 0;

	struct diameter_hdr tmphdr = { 0 };
	struct diameter_avp *head, *iterator;

	tmphdr.version = pkt->header.version;
	tmphdr.length = ((pkt->header.length & 0x0000ff) << 16) |
			((pkt->header.length & 0x00ff00)) |
			((pkt->header.length & 0xff0000) >> 16);

	tmphdr.flags = pkt->header.flags;
	tmphdr.command_code = ((pkt->header.command_code & 0x0000ff) << 16) |
			      ((pkt->header.command_code & 0x00ff00)) |
			      ((pkt->header.command_code & 0xff0000) >> 16);

	tmphdr.application_id = htonl(pkt->header.application_id);
	tmphdr.hop_by_hop_id = htonl(pkt->header.hop_by_hop_id);
	tmphdr.end_to_end_id = htonl(pkt->header.end_to_end_id);

	memcpy(buf, &tmphdr, sizeof(struct diameter_hdr));
	offt += sizeof(struct diameter_hdr);

	head = pkt->avp_list;
	for (iterator = head; iterator; iterator = iterator->next) {
		offt += _serialize_diameter_avp(iterator, buf + offt);
	}
	return offt;
}

void diameter_deserialize_packet(const char *buf, int buf_size,
				 struct diameter_pkt *pkt)
{
	int offt = 0;

	struct diameter_hdr tmp_dimhdr;

	memcpy(&tmp_dimhdr, buf, sizeof(struct diameter_hdr));

	pkt->header.version = tmp_dimhdr.version;
	pkt->header.length = ntohl(tmp_dimhdr.length);
	pkt->header.length = ((tmp_dimhdr.length & 0x0000ff) << 16) |
			     ((tmp_dimhdr.length & 0x00ff00) << 8) |
			     ((tmp_dimhdr.length & 0xff0000));

	pkt->header.flags = tmp_dimhdr.flags;
	pkt->header.command_code =
		((tmp_dimhdr.command_code & 0x0000ff) << 16) |
		((tmp_dimhdr.command_code & 0x00ff00) << 8) |
		((tmp_dimhdr.command_code & 0xff0000));

	pkt->header.application_id = ntohl(tmp_dimhdr.application_id);
	pkt->header.hop_by_hop_id = ntohl(tmp_dimhdr.hop_by_hop_id);
	pkt->header.end_to_end_id = ntohl(tmp_dimhdr.end_to_end_id);

	offt += sizeof(struct diameter_hdr);

	/*
	struct diameter_avp_hdr tmp_avphdr;
	int data_sz;

	while(offt < buf_size)
	{
		memset(&tmp_avphdr, 0, sizeof(struct diameter_avp_hdr));
		struct diameter_avp *avp = malloc(sizeof(struct diameter_avp));

		memcpy(&tmp_avphdr, buf + offt, sizeof(struct diameter_avp_hdr));
		offt += sizeof(struct diameter_avp_hdr);

		avp->header.code = ntohl(tmp_avphdr.code);
		avp->header.flags = tmp_avphdr.flags;
		avp->header.length = 	((tmp_avphdr.length & 0x0000ff) << 16)	|
					((tmp_avphdr.length & 0x00ff00) << 8)	|
					((tmp_avphdr.length & 0xff0000));

		data_sz = avp->header.length - sizeof(struct diameter_avp_hdr);
		avp->pad = (4 - data_sz%4)%4;

		avp->data = malloc(data_sz + avp->pad);
		memcpy(avp->data, buf + offt, data_sz);
		offt += data_sz;
		if(avp->pad)
		{
			memset(avp->data, 0, avp->pad);
			offt += avp->pad;
		};

		diameter_insert_avp(pkt,avp);
	}
	*/
}

struct diameter_pkt *diameter_read_json_file(const char *filename)
{
	FILE *fp;
	int file_sz = 0;
	struct stat sb;
	char *buf;
	struct diameter_pkt *diameter;

	fp = fopen(filename, "rb");

	if (!fp) {
		printf("No such file or directory.\n");
		return NULL;
	}

	fstat(fileno(fp), &sb);
	file_sz = sb.st_size;

	buf = malloc(file_sz + 1);
	fread(buf, file_sz, 1, fp);
	buf[file_sz] = '\0';
	fclose(fp);

	diameter = diameter_read_json_packet(buf);

	if (!diameter) {
		printf("Failed to read json file.\n");
		return NULL;
	}

	free(buf);
	return diameter;
}

/* Detect diameter proto */
int diameter_detect_proto(const char *data, uint32_t offt, uint32_t pl_len)
{
	const char *diam_payload = data + offt;

	uint32_t diam_len = (((uint32_t)diam_payload[1] & 0xff) << 16) |
			    (((uint32_t)diam_payload[2] & 0xff) << 8) |
			    ((uint32_t)diam_payload[3] & 0xff);
	return ((pl_len - offt) == diam_len ? 1 : 0);
}

int diameter_send(struct diameter_pkt *diam_pkt, const char *addr,
		  uint16_t port, char proto, uint64_t flags)
{
	/*	char 		*buf = NULL;
	uint16_t 	buf_len = 0;
	int 		result;
	struct pkt_buffer *pkt;

	pkt = malloc( sizeof( struct pkt_buffer));

	pkt->tnl_idx 	= 0;
	pkt->data_len 	= diam_pkt->header.length;
	pkt->data 	= malloc( pkt->data_len);

	pkt->d_addr	= inet_addr(addr);
	pkt->l4_sport	= htons(4001);
	pkt->l4_dport	= htons(port);

	if( flags & NET_CAP_FRM)
		pkt->enable_cap = 1;
	if( flags & NET_ENBL_TNL)
		pkt->enable_tnl = 1;

	result = diameter_serialize_packet( diam_pkt, pkt->data);

	if( result != pkt->data_len)
	{
		printf("Something went wrong.\n");
		return -1;
	}

	sctp_send( pkt, SCTP_DATA_CHUNK, 3);
	return result;
*/
	return 0;
}

static void _print_avp(const struct diameter_avp *avp, int indent)
{
	char *idnt = malloc(indent + 1);
	memset(idnt, '\t', indent);
	idnt[indent] = '\0';

	while (avp) {
		//		char *avp_name = string_from_value(avp->header.code,
		//					avp_display, "Unknown");
		printf("%sAVP code : %u (null)\n", idnt, avp->header.code);
		printf("%sLength : %u\n", idnt, avp->header.length);

		switch (avp->type) {
		case OctetString: {
			printf("%s%s\n", idnt, avp->data.octetstring);
			break;
		}
		case Integer32: {
			printf("%s%d\n", idnt, avp->data.int32);
			break;
		}
		case Integer64: {
			printf("%s%ld\n", idnt, avp->data.int64);
			break;
		}
		case Unsigned32: {
			printf("%s%u\n", idnt, avp->data.unsigned32);
			break;
		}
		case Unsigned64: {
			printf("%s%lu\n", idnt, avp->data.unsigned64);
			break;
		}
		case Grouped: {
			_print_avp((const struct diameter_avp *)avp->data.group,
				   indent + 1);
			break;
		}
		default:;
		}
		avp = avp->next;
	}
	if (idnt)
		free(idnt);
}

void diameter_init()
{
}

void diameter_clean_up()
{
}

struct diameter_pkt *diameter_parse_buffer(const char *buf, size_t buf_len)
{
	struct diameter_pkt *diam;

	diam = malloc(sizeof(struct diameter_pkt));

	diam->header = *(struct diameter_hdr *)buf;

	printf("\n\t--------------DIAMETER---------------\n");
	printf("\t\tVersion:\t%d\n", diam->header.version);
	printf("\t\tLength:\t%10u\n", htonl(diam->header.length << 8));
	printf("\t\tFlags :\t\t%c%c%c\n",
	       (diam->header.flags & FLAG_REQUEST ? 'R' : '-'),
	       (diam->header.flags & FLAG_PROXYABLE ? 'P' : '-'),
	       (diam->header.flags & FLAG_ERROR ? 'E' : '-'));
	printf("\t\tCommand-code : %d\n",
	       (htonl(diam->header.command_code << 8)));
	printf("\t----------------AVPS-----------------\n");

	return diam;
}

void diameter_print_packet(const struct diameter_pkt *pkt)
{
	assert(pkt);
	printf("--------------DIAMETER---------------\n");
	printf("Version:%10d\n", pkt->header.version);
	printf("Length:\t%10u\n", pkt->header.length);
	printf("Flags:\n");
	printf("----------------AVPS-----------------\n");

	_print_avp((const struct diameter_avp *)pkt->avp_list, 0);
	printf("\n");
}
