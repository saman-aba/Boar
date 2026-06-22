#ifndef __MAP_UTILS_H__
#define __MAP_UTILS_H__
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "asn1.h"
#include "ss7.h"

#define printf_stdout(...) //fprintf(stdout, __VA_ARGS__);
#define printf_stderr(...) //fprintf(stderr, __VA_ARGS__);

#define error_printf(...)  printf_stdout("(asn1 err): "__VA_ARGS__)

#ifdef map_debug

#define ansi_color_red "\x1b[31m"
#define ansi_color_green "\x1b[32m"
#define ansi_color_yellow "\x1b[33m"
#define ansi_color_blue "\x1b[34m"
#define ansi_color_magenta "\x1b[35m"
#define ansi_color_cyan "\x1b[36m"
#define ansi_color_reset "\x1b[0m"

#define debug_printf(...)  printf_stdout("(asn1 debug): "__VA_ARGS__)	
#else
#define debug_printf(...)
#endif

// TODO: keep this untill implementation of arena
#include <stdlib.h>


int asn1_primitive_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx);
size_t asn1_primitive_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size);
void asn1_primitive_free(const struct asn1_param *p, void *field);
static void asn1_sequence_of_free(const struct asn1_param *p, void *field);
int asn1_primitive_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size);


/* The cursor is a bright and simple idea that fixes a lot of problem
 * about moving pointer in the buffer and consuming bytes */
static inline void asn1_cursor_init(asn1_cursor *c,
		const uint8_t *buf,
		size_t len)
{
	c->p = buf;
	c->len = len;
}

static inline int asn1_cursor_read(asn1_cursor *c, size_t n,
		uint8_t *out, bool copy)
{
	if (c->len < n)
		return -1;
	if(copy && out)
		memcpy(out, c->p, n);
	c->p += n;
	c->len -= n;
	return 0;
}

static inline int asn1_cursor_skip(asn1_cursor *c, size_t n)
{
	if (c->len < n)
		return -1;
	c->p += n;
	c->len -= n;
	return 0;
}

static  bool asn1_tag_matches(const struct asn1_param *p, asn1_tag_t tag)
{
	if (p->tag == ASN1_TYPE_ANY || p->tag == 0)
		return true;
	/* TODO: This must be fixed as soon as possible,
	 * for EXPLICIT, outer tag must match */
	if (p->type_desc->flags == ASN1_FLAG_EXPLICIT)
		return (tag == p->type_desc->tag);
	return (tag == p->tag);
}

static asn1_tag_t asn1_param_effective_tag(const struct asn1_param *p)
{
	if (p->tag)
		return p->tag;
	return p->type_desc->tag;
}

static size_t asn1_tag_write(asn1_tag_t tag, uint8_t *buf, size_t size)
{
	uint32_t cls = (tag >> 30) & 0x03;
	uint32_t cons = (tag >> 29) & 0x01;
	uint32_t number = tag & 0x1fffffff;
	size_t off = 0;

	if (!buf)
		return number < 31 ? 1 : 1 + ((number ? 32 - __builtin_clz(number) : 1) + 6) / 7;
	if (size < 1)
		return 0;
	if (number < 31) {
		buf[0] = (cls << 6) | (cons << 5) | number;
		return 1;
	}

	buf[off++] = (cls << 6) | (cons << 5) | 0x1f;
	uint8_t tmp[5];
	size_t count = 0;
	do {
		tmp[count++] = number & 0x7f;
		number >>= 7;
	} while (number);
	if (size < 1 + count)
		return 0;
	while (count) {
		uint8_t octet = tmp[--count];
		if (count)
			octet |= 0x80;
		buf[off++] = octet;
	}
	return off;
}

static size_t asn1_length_write(size_t len, uint8_t *buf, size_t size)
{
	if (!buf)
		return len < 128 ? 1 : 1 + ((sizeof(size_t) * 8 - __builtin_clzl(len)) + 7) / 8;
	if (len < 128) {
		if (size < 1)
			return 0;
		buf[0] = len;
		return 1;
	}

	uint8_t tmp[sizeof(size_t)];
	size_t count = 0, value = len;
	while (value) {
		tmp[count++] = value & 0xff;
		value >>= 8;
	}
	if (size < 1 + count)
		return 0;
	buf[0] = 0x80 | count;
	for (size_t i = 0; i < count; i++)
		buf[1 + i] = tmp[count - i - 1];
	return 1 + count;
}

static int asn1_write_tlv(asn1_tag_t tag, const uint8_t *value,
		size_t value_len, uint8_t *buf, size_t size)
{
	size_t tag_len = asn1_tag_write(tag, NULL, 0);
	size_t len_len = asn1_length_write(value_len, NULL, 0);
	size_t total = tag_len + len_len + value_len;

	if (!buf)
		return total;
	if (size < total)
		return -1;
	if (!asn1_tag_write(tag, buf, size))
		return -1;
	if (!asn1_length_write(value_len, buf + tag_len, size - tag_len))
		return -1;
	if (value_len && value)
		memcpy(buf + tag_len + len_len, value, value_len);
	return total;
}

static int asn1_read_tag(asn1_cursor *c, asn1_tag_t *out)
{
	if (c->len == 0)
		return -1;
	uint8_t b = c->p[0];
	uint8_t cls = (b >> 6) & 0x03;
	uint8_t pc  = (b >> 5) & 0x01;
	uint8_t tag = b & 0x1F;

	if (asn1_cursor_skip(c, 1) < 0)
		return -1;

	if (tag != 0x1F) {
		*out = ASN1_TAG_KEY(cls, pc, tag);
		return 0;
	}

	uint32_t val = 0;
	for (;;) {
		if (c->len == 0)
			return -1;
		uint8_t b2 = *c->p;
		if (asn1_cursor_skip(c, 1) < 0)
			return -1;
		val = (val << 7) | (b2 & 0x7F);
		if (!(b2 & 0x80))
			break;
	}
	*out = ASN1_TAG_KEY(cls, pc, val);
	return 0;
}
static int asn1_read_length(asn1_cursor *c, size_t *out, uint8_t *indefinite)
{
	if(indefinite)
		*indefinite = 0;
	if (c->len == 0)
		return -1;
	uint8_t b = c->p[0];

	if (asn1_cursor_skip(c, 1) < 0)
		return -1;
	
	if ((b & 0x80) == 0) {
		*out = b;
		return 0;
	}

	/* TODO: Bad design, obvious performance issue,
	 * but sadly i have no choice to leave it for now. */
	uint8_t n = b & 0x7F;
	if (n == 0) {
		*indefinite = 1;
		*out = c->len;
		return 0;
	}

	size_t len = 0;
	while (n--) {
		 if (c->len == 0)
		 	return -1;
		 len = (len << 8) | c->p[0];
		 if (asn1_cursor_skip(c, 1) < 0)
		 	return -1;
	}

	*out = len;
	return 0;
}

static int asn1_read_tlv(asn1_cursor *c,
		asn1_tag_t *tag,
		size_t *len,
		const uint8_t **val,
		uint8_t *indefinite)
{
	if (asn1_read_tag(c, tag)) {
		debug_printf("asn1_read_tlv: read tag error\n");
		return -1;
	}

	if (asn1_read_length(c, len, indefinite)) {
		debug_printf("asn1_read_tlv: read length error\n");
		return -1;
	}
	
	if (tag == 0 && len == 0)
		return ASN1_EOC;
	if (val)
		*val = c->p;

	if (indefinite && *indefinite)
		return 0;

	if (c->len < *len ) {
		debug_printf("asn1_read_tlv: remaning buffer is less than"
				"requested length\n");	
		return -1;
	}

	return 0;
}

static int asn1_param_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx)
{
	const struct asn1_type_desc *desc = p->type_desc;
	if (desc->type == ASN1_TYPE_NON_PRIMITIVE) {
		/* TODO: fix the parameter arrangement later */
		if(desc->ops->ber_decode)
			return desc->ops->ber_decode(buf, len, p, field, ctx);
		return -1;
	}
	return asn1_primitive_decode_ber(p, buf, len, field, ctx);
}

static int asn1_param_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	const struct asn1_type_desc *desc = p->type_desc;

	if (!field)
		return 0;
	if (desc->type == ASN1_TYPE_NON_PRIMITIVE) {
		if(desc->ops && desc->ops->ber_encode)
			return desc->ops->ber_encode(p, field, buf, size);
		return -1;
	}
	return asn1_primitive_encode_ber(p, field, buf, size);
}

static int asn1_param_lookup_bounded(const struct asn1_type_desc *desc,
		const struct asn1_param **pos, asn1_tag_t key)
{
	const struct asn1_param *start;

	if(!desc || !desc->params || !pos || !desc->nb_params)
		return -1;

	if(!*pos)
		start = desc->params;
	else
		start = *pos + 1;

	for(const struct asn1_param *p = start;
			p < desc->params + desc->nb_params; p++) {
		asn1_tag_t tag = p->tag ? p->tag : p->type_desc->tag;
		if(tag == key || tag == ASN1_TYPE_ANY) {
			*pos = p;
			return 0;
		}
	}

	for(const struct asn1_param *p = desc->params; p < start; p++) {
		asn1_tag_t tag = p->tag ? p->tag : p->type_desc->tag;
		if(tag == key || tag == ASN1_TYPE_ANY) {
			*pos = p;
			return 0;
		}
	}

	return -1;
}

static size_t asn1_choice_field_offset(const struct asn1_param *p)
{
	size_t ptr_align = _Alignof(void *);
	size_t default_offset = sizeof(uint32_t);

	default_offset = (default_offset + ptr_align - 1) & ~(ptr_align - 1);
	return p->offset ? p->offset : default_offset;
}

static size_t asn1_param_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	size_t ret;
	const struct asn1_type_desc *desc = p->type_desc;
	char valbuf[8192];

	if (desc->type == ASN1_TYPE_NON_PRIMITIVE) {
		assert(desc->ops->json_print);
		ret = desc->ops->json_print(p, data, valbuf, sizeof(valbuf));
	} else {
		ret = asn1_primitive_print_json(p, data, valbuf, sizeof(valbuf));
	}

	if(!ret)
		return 0;
	if((ret == 2 && !strncmp(valbuf, "{}", 2)) ||
			(ret == 2 && !strncmp(valbuf, "[]", 2))) {
		debug_printf("empty JSON field omitted: %s\n", p->name);
		return 0;
	}

	return snprintf(strbuf, size, "\"%s\":%.*s,",
			p->name, (int)ret, valbuf);
}

static void asn1_param_free_owned(const struct asn1_param *p, void *field);
static void asn1_param_free_content(const struct asn1_param *p, void *field);

int asn1_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx)
{
	asn1_cursor c;
	asn1_cursor_init(&c, buf, len);

	asn1_tag_t tag;
	size_t val_len;
	const uint8_t *val;
	uint8_t indefinite = 0;

	if (p->type_desc->flags & ASN1_FLAG_EXPLICIT) {
		if (asn1_read_tlv(&c, &tag, &val_len, &val, &indefinite)) {
			debug_printf("asn1_decode_ber: tlv failure\n");
			return -1;
		}
		
		if (!asn1_tag_matches(p, tag)) {
			debug_printf("asn1_decode_ber: tag did not match\n");
			return -1;
		}
		/* TODO: this must be checked later, i wrote it but not using it.
		 * it is here to check whether EXPLICIT is constructed or not
		 if(!ASN1_CONSTRUCTED(tag)
		 	debug_printf("
		 	return -1;
		 */
		 asn1_cursor inner = {0};
		 asn1_cursor_init(&inner, val, val_len);
		 return asn1_param_decode_ber(p, inner.p, inner.len, field, ctx); 
	}

	return asn1_param_decode_ber(p, c.p, c.len, field, ctx);
}

int asn1_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	assert(p);
	return asn1_param_encode_ber(p, field, buf, size);
}

size_t asn1_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	assert(p);
	if (data)
		return asn1_param_print_json(p, data, strbuf, size);
	return 0;
}

void asn1_free(const struct asn1_param *p, void *field)
{
	assert(p);
	if (field) {
		if (p->flags & ASN1_PARAM_OPTIONAL) {
			return asn1_param_free_owned(p, field);
		}
		return asn1_param_free_content(p, field);
	}
}

/*** BOOLEAN ***/
static int asn1_any_decode_ber(const struct asn1_param *p,
		const uint8_t *buf, size_t len, void *field, void *ctx)
{
	asn1_cursor c = { buf, len };
	asn1_tag_t tag;
	size_t val_len;
	const uint8_t *val;
	uint8_t indefinite = 0;
	asn1_ANY_t *any = field;

	if (asn1_read_tlv(&c, &tag, &val_len, &val, &indefinite))
		return -1;
	if (asn1_cursor_skip(&c, val_len))
		return -1;
	any->ptr = (void *)buf;
	any->size = c.p - buf;
	return 0;
}

static size_t asn1_any_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	static const char hextbl[] = "0123456789abcdef";
	const asn1_ANY_t *any = data;
	size_t off = 0;

	if(!any || !any->ptr)
		return 0;
	if(size < (any->size * 2) + 3)
		return 0;
	strbuf[off++] = '"';
	for(size_t i = 0; i < any->size; i++) {
		const uint8_t *ptr = any->ptr;
		strbuf[off++] = hextbl[(ptr[i] >> 4) & 0xf];
		strbuf[off++] = hextbl[ptr[i] & 0xf];
	}
	strbuf[off++] = '"';
	return off;
}

static int asn1_any_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	const asn1_ANY_t *any = field;

	if(!any || !any->ptr)
		return 0;
	if(!buf)
		return any->size;
	if(size < any->size)
		return -1;
	memcpy(buf, any->ptr, any->size);
	return any->size;
}

static int asn1_boolean_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx)
{
	asn1_cursor c = { buf, len };
	asn1_tag_t tag;
	size_t val_len;
	uint8_t indefinite = 0;
	const uint8_t *val;
	
	if (asn1_read_tlv(&c, &tag, &val_len, &val, &indefinite)) {
		debug_printf("asn1_decode_ber: tlv failure\n");
		return -1;
	}

	if (!asn1_tag_matches(p, tag)) {
		debug_printf("asn1_boolean_decode_ber: tag did not match\n");
		return -1;
	}
	if (val_len != 1) {
		debug_printf("asn1_boolean_decode_ber: invalid boolean value\n");
		return -1;
	}
	val = c.p;
	
	*(asn1_BOOLEAN_t *)field = (val[0] != 0);
	return 0;
}
static int asn1_boolean_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	uint8_t value = *(const asn1_BOOLEAN_t *)field ? 0xff : 0x00;

	return asn1_write_tlv(asn1_param_effective_tag(p), &value, 1,
			buf, size);
}
static size_t asn1_boolean_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	asn1_BOOLEAN_t boolean = *(asn1_BOOLEAN_t *)data;
	return snprintf(strbuf, size, "\"%s\"", (boolean ? "true" : "false"));
}
static void asn1_boolean_free(const struct asn1_param *p, void *field)
{
}

/*** INTEGER ***/
static int asn1_integer_decode_ber(const struct asn1_param *p,
		const uint8_t *buf, size_t len, void *field, void *ctx)
{
	asn1_cursor c = { buf, len };
	asn1_tag_t tag;
	size_t val_len;
	uint8_t indefinite = 0;
	const uint8_t *val;
	
	if (asn1_read_tlv(&c, &tag, &val_len, &val, &indefinite)) {
		debug_printf("asn1_decode_ber: tlv failure\n");
		return -1;
	}
	if (!asn1_tag_matches(p, tag)) {
		debug_printf("asn1_integer_decode_ber: tag did not match\n");
		return -1;
	}
	
	int64_t v = 0;
	if ( val_len > 8) {
		debug_printf("asn1_integer_decode_ber: more than 64bit integer\n");
		return -1;
	}
	for (size_t i = 0; i < val_len; i++) {
		v = (v << 8) | val[i];
	}

	if (val_len > 0 && (val[0] & 0x80)) {
		for (size_t i = val_len; i < 8; i++)
			v |= ((int64_t)0xff << (i *8));
	}
	*(int64_t *)field = v;
	return 0;
}
static int asn1_integer_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	uint64_t value = *(const asn1_INTEGER_t *)field;
	uint8_t tmp[9];
	size_t len = 0;

	do {
		tmp[sizeof(tmp) - ++len] = value & 0xff;
		value >>= 8;
	} while (value);
	if (tmp[sizeof(tmp) - len] & 0x80)
		tmp[sizeof(tmp) - ++len] = 0;

	return asn1_write_tlv(asn1_param_effective_tag(p),
			tmp + sizeof(tmp) - len, len, buf, size);
}
static size_t asn1_integer_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	asn1_INTEGER_t integer = *(asn1_INTEGER_t *)data;
	return snprintf(strbuf, size, "%ld", integer);
}
static void asn1_integer_free(const struct asn1_param *p, void *field)
{
}

/*** ENUMERATED ***/
static int asn1_enumerated_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx)
{
	return asn1_integer_decode_ber(p, buf, len, field, ctx);
}

static int asn1_enumerated_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	return asn1_integer_encode_ber(p, field, buf, size);
}

static size_t asn1_enumerated_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	const struct asn1_type_desc *desc = p->type_desc;
	asn1_ENUMERATED_t e = *(asn1_ENUMERATED_t *)data;
	
	return snprintf(strbuf, size, "\"%s(%ld)\"",
			(e < desc->enum_str_sz ? desc->enum_str[e]: "unkonwn"), e);
}
static void asn1_enumerated_free(const struct asn1_param *p, void *field)
{
	return asn1_integer_free(p, field);
}

/*** NULL ***/
static int asn1_null_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx)
{
	asn1_cursor c = { buf, len };
	asn1_tag_t tag;
	size_t val_len;
	uint8_t indefinite = 0;
	
	if (asn1_read_tlv(&c, &tag, &val_len, NULL, &indefinite)) {
		debug_printf("asn1_decode_ber: tlv failure\n");
		return -1;
	}
	if (!asn1_tag_matches(p, tag)) {
		debug_printf("asn1_null_decode_ber: tag did not match\n");
		return -1;
	}
	if ( val_len != 0) {
		debug_printf("asn1_null_decode_ber: invalid null\n");
		return -1;
	}
	return 0;
};

static int asn1_null_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	return asn1_write_tlv(asn1_param_effective_tag(p), NULL, 0,
			buf, size);
}

static size_t asn1_null_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	if (size < 3) 
		return 0;
	strbuf[0] = '\"';
	strbuf[1] = '+';
	strbuf[2] = '\"';
	return 3;
}
static void asn1_null_free(const struct asn1_param *p, void *field)
{
	assert(0);
}

/*** OCTET STRING ***/
static int asn1_octet_string_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx)
{
	const struct asn1_type_desc *desc = p->type_desc;
	asn1_tag_t tag;
	size_t val_len;
	uint8_t indefinite = 0;
	const uint8_t *val;
	
	asn1_cursor c = { buf, len };

	if (asn1_read_tlv(&c, &tag, &val_len, &val, &indefinite)) {
		debug_printf("asn1_decode_ber: tlv failure\n");
		return -1;
	}
	if (!asn1_tag_matches(p, tag)) {
		debug_printf("asn1_octet_string_decode_ber: tag did not match\n");
		return -1;
	}
	
	asn1_OCTET_STRING_t *o = field;
	o->slice.data = val;
	o->slice.size = val_len;

	if(desc->ops->data_fmt)
		desc->ops->data_fmt(val, val_len, p, field, (void **)&ctx);
	return 0;
}
static int asn1_octet_string_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	const asn1_OCTET_STRING_t *oc = field;

	return asn1_write_tlv(asn1_param_effective_tag(p),
			oc->slice.data, oc->slice.size, buf, size);
}
static size_t asn1_octet_string_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	static const char hextbl[] = {
		'0','1','2','3',
		'4','5','6','7',
		'8','9','a','b',
		'c','d','e','f'
	};

	size_t ret, off = 0;
	const struct asn1_type_desc *desc = p->type_desc;

	asn1_OCTET_STRING_t *oc = (asn1_OCTET_STRING_t *)data;

	if (desc->ops->json_print) {
		ret = desc->ops->json_print(p, data, strbuf, size);
		off += ret;
	} else {
		strbuf[off++] = '\"';
		for (size_t i = 0; i < oc->slice.size; i++) {
			strbuf[off + (2 * i)] = hextbl[(oc->slice.data[i] >> 4) & 0xf];
			strbuf[off + (2 * i) + 1] = hextbl[(oc->slice.data[i]) & 0xf];
		}
		off += (oc->slice.size * 2);
		strbuf[off++] = '\"';
	}
	
	return off;
}
static void asn1_octet_string_free(const struct asn1_param *p, void *field)
{
	asn1_OCTET_STRING_t *oc = field;

	if(!oc)
		return;
	if(p->type_desc->ops && p->type_desc->ops->free_fmt)
		p->type_desc->ops->free_fmt(p, field);
}

/*** BIT STRING ***/
static int asn1_bit_string_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx)
{
	asn1_tag_t tag;
	size_t val_len;
	uint8_t indefinite = 0;
	const uint8_t *val;

	asn1_cursor c = { buf, len };

	if (asn1_read_tlv(&c, &tag, &val_len, &val, &indefinite)) {
		debug_printf("asn1_decode_ber: tlv failure\n");
		return -1;
	}
	if (!asn1_tag_matches(p, tag)) {
		debug_printf("asn1_bit_string_decode_ber: tag did not match\n");
		return -1;
	}

	uint8_t unused = val[0];

	if (unused > 7) {
		debug_printf("asn1_bit_string_decode_ber: illegal unused bits\n");
		return -1;
	}

	asn1_BIT_STRING_t *bs = field;
	bs->nb_bits = (val_len - 1) * 8 - unused;
	bs->padding = unused;
	bs->slice.data = val + 1;
	bs->slice.size = val_len - 1;
	bs->bits = val + 1;
	return 0;
}
static int asn1_bit_string_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	const asn1_BIT_STRING_t *bs = field;
	uint8_t tmp[1024];
	const uint8_t *bits = bs->bits ? bs->bits : bs->slice.data;

	if (bs->slice.size + 1 > sizeof(tmp))
		return -1;
	tmp[0] = bs->padding;
	if (bs->slice.size && bits)
		memcpy(tmp + 1, bits, bs->slice.size);
	return asn1_write_tlv(asn1_param_effective_tag(p),
			tmp, bs->slice.size + 1, buf, size);
}
static size_t asn1_bit_string_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	size_t ret, off = 0, rem = size;
	const struct asn1_type_desc *desc = p->type_desc;

	asn1_BIT_STRING_t *bs = (asn1_BIT_STRING_t *)data;

	if (!desc->bit_str || !bs->bits)
		return 0;

	strbuf[off++] = '{';
	rem--;
	ret = snprintf(strbuf + off, rem, "\"padding\":%d,", bs->padding);
	off += ret; rem -= ret;

	for (size_t i = 0; i < desc->bit_str_sz; i++) {
		if (desc->bit_str[i]) {
			size_t byte_index = i / 8;
			if(byte_index >= bs->slice.size)
				break;
			ret = snprintf(strbuf + off, rem, "\"%s\":%d,", 
					desc->bit_str[i],
					(bs->bits[byte_index] & (0x80 >> (i % 8))) ? 1 : 0);
			off += ret; rem -= ret;
		}
	}
	if(off > 1 && strbuf[off - 1] == ',')
		off--;
	strbuf[off++] = '}';
	return off;
}
static void asn1_bit_string_free(const struct asn1_param *p, void *field)
{
}

/*** OBJECT IDENTIFIER ***/
static int asn1_object_identifier_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx)
{
	asn1_tag_t tag;
	size_t val_len;
	uint8_t indefinite = 0;
	const uint8_t *val;
	
	asn1_cursor c = { buf, len };

	if (asn1_read_tlv(&c, &tag, &val_len, &val, &indefinite)) {
		debug_printf("asn1_decode_ber: tlv failure\n");
		return -1;
	}

	if (!asn1_tag_matches(p, tag)) {
		debug_printf("asn1_object_identifier_decode_ber: tag did not match\n");
		return -1;
	}

	asn1_OBJECT_IDENTIFIER_t *oid = field;
	if(!val_len)
		return -1;

	uint32_t first = val[0] / 40;
	uint32_t second = val[0] % 40;

	size_t arc_count = 2;
	oid->arcs[0] = first;
	oid->arcs[1] = second;
	size_t pos = 1;
	while (pos < val_len) {
		uint32_t v = 0;
		if(arc_count >= sizeof(oid->arcs) / sizeof(oid->arcs[0]))
			return -1;
		for (;;) {
			if (pos >= val_len)
				return -1;
			uint8_t b2 = val[pos++];
			v = (v << 7) | (b2 & 0x7F);
			if (!(b2 & 0x80))
				break;
		}
		oid->arcs[arc_count++] = v;
	}
	oid->arc_count = arc_count;
	return 0;
}
static int asn1_object_identifier_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	const asn1_OBJECT_IDENTIFIER_t *oid = field;
	uint8_t tmp[128];
	size_t off = 0;

	if (oid->arc_count < 2)
		return -1;
	tmp[off++] = oid->arcs[0] * 40 + oid->arcs[1];
	for (size_t i = 2; i < oid->arc_count; i++) {
		uint16_t arc = oid->arcs[i];
		uint8_t stack[3];
		size_t count = 0;

		do {
			stack[count++] = arc & 0x7f;
			arc >>= 7;
		} while (arc);
		if (off + count > sizeof(tmp))
			return -1;
		while (count) {
			uint8_t octet = stack[--count];
			if (count)
				octet |= 0x80;
			tmp[off++] = octet;
		}
	}

	return asn1_write_tlv(asn1_param_effective_tag(p),
			tmp, off, buf, size);
}
static size_t asn1_object_identifier_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	size_t ret, off = 0;

	asn1_OBJECT_IDENTIFIER_t *oid = (asn1_OBJECT_IDENTIFIER_t *)data;
	
	strbuf[off++] = '\"';
	for(size_t i = 0; i < oid->arc_count ; i++) {
		ret = snprintf(strbuf + off, size, "%d.", oid->arcs[i]);
		off += ret;
	}
	strbuf[off-1] = '\"';

	return off;
}
static void asn1_object_identifier_free(const struct asn1_param *p, void *field)
{
}

/*** SEQUENCE ***/
int asn1_sequence_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx)
{
	const asn1_type_desc *desc = p->type_desc;
	asn1_cursor c = { buf, len };
	asn1_tag_t tag;
	size_t val_len;
	uint8_t indefinite = 0;
	const uint8_t *val;
	
	if (p->flags & ASN1_PARAM_EXPLICIT_TAG) {
		if (asn1_read_tlv(&c, &tag, &val_len, &val, &indefinite)) {
			debug_printf("asn1_decode_ber: tlv failure\n");
			return -1;
		}

		if (!asn1_tag_matches(p, tag)) {
			debug_printf("asn1_sequence_decode_ber:"
					" tag did not match\n");
			return -1;
		}
	} else {
		if (p->tag && p->tag != ASN1_TYPE_ANY &&
				!asn1_read_tlv(&c, &tag, &val_len, &val,
					&indefinite) &&
				asn1_tag_matches(p, tag)) {
			/* IMPLICIT constructed SEQUENCE: decode the wrapped value. */
		} else {
			val = buf;
			val_len = len;
		}
	}

	
	asn1_cursor inner = { val, val_len };

	uint64_t *seen_mask = (uint64_t *)field;
	
	const struct asn1_param *last_param = NULL;
	while (inner.len > 0) {
		asn1_tag_t ftag;
		size_t flen;
		const uint8_t *fval;
		uint8_t findefinite = 0;

		const uint8_t *start = inner.p;
	
		if (asn1_read_tlv(&inner, &ftag, &flen, &fval, &findefinite)) {
			debug_printf("asn1_decode_ber: tlv failure\n");
			return -1;
		}
		if(!findefinite && flen > inner.len) {
			debug_printf("asn1_sequence_decode_ber: field length exceeds remaining buffer\n");
			return -1;
		}

		/* Looking up for the parameter in param sequence */
		if (asn1_param_lookup_bounded(desc, &last_param, ftag)) {
			if(asn1_cursor_skip(&inner, flen))
				return -1;
			continue;
		}
		
		if(asn1_cursor_skip(&inner, flen))
			return -1;

		void *field_ptr = (uint8_t *)field + last_param->offset;
		int decode_ret;
		if( last_param->flags & ASN1_PARAM_OPTIONAL) {
			void **pp = field_ptr;
			*pp = SS7_CALLOC(ctx, 1,
					last_param->type_desc->sizeof_struct);
			if(!*pp)
				return -1;
			decode_ret = asn1_decode_ber(last_param, start,
					inner.p - start, *pp, ctx);
			if (decode_ret)
				debug_printf("asn1_sequence_decode_ber:"
						"decode failure: %s in %s\n",
						last_param->name, p->name);
			if (decode_ret) {
				asn1_param_free_owned(last_param, *pp);
				*pp = NULL;
			}
		} else {
			decode_ret = asn1_decode_ber(last_param, start,
					inner.p - start, field_ptr, ctx);
			if (decode_ret)
				debug_printf("asn1_sequence_decode_ber:"
						"decode failure: %s in %s\n",
						last_param->name, p->name);
		}

		if (!decode_ret && seen_mask)
			*seen_mask |= (1ULL << last_param->bit);
	}

	if (desc->mandatory_mask) {
		if((*seen_mask & desc->mandatory_mask) != desc->mandatory_mask)
			debug_printf("asn1_sequence_decode_ber:"
					" missing mandatory fields\n");
	}

	return 0;
}
static size_t asn1_sequence_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	size_t ret, off = 0, rem = size;
	uint64_t seen_mask;
	const struct asn1_type_desc *desc = p->type_desc;
	void *field;
	
	seen_mask = *((uint64_t *)data);

	strbuf[off++] = '{';
	rem--;
	for(size_t i = 0; i < desc->nb_params; i++)
	{
		const struct asn1_param *par = &desc->params[i];

		if (!(seen_mask & (1ULL << par->bit)))
			continue;

		if (par->flags & ASN1_PARAM_OPTIONAL) {
			void **field_ptr = (void **)(data + par->offset);
			field = *field_ptr;
		} else {
			field = (void *)(data + par->offset);
		}

		if (!field)
			continue;
		ret = asn1_print_json(par, field, strbuf + off, rem);
		if(!ret)
			continue;
		off += ret;
		rem -= ret;
	}
	if(off > 1 && strbuf[off - 1] == ',')
		off--;
	strbuf[off++] = '}';
	return off;
}
static int asn1_sequence_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	uint64_t seen_mask = *((const uint64_t *)field);
	const struct asn1_type_desc *desc = p->type_desc;
	uint8_t tmp[8192];
	size_t off = 0;

	for(size_t i = 0; i < desc->nb_params; i++)
	{
		const struct asn1_param *par = &desc->params[i];
		const void *par_field;
		int ret;

		if (!(seen_mask & (1ULL << par->bit)))
			continue;
		if (par->flags & ASN1_PARAM_OPTIONAL) {
			void * const *field_ptr =
				(void * const *)((const uint8_t *)field + par->offset);
			par_field = *field_ptr;
		} else {
			par_field = (const uint8_t *)field + par->offset;
		}
		if (!par_field)
			continue;
		ret = asn1_encode_ber(par, par_field, tmp + off,
				sizeof(tmp) - off);
		if (ret < 0)
			return ret;
		off += ret;
	}

	return asn1_write_tlv(asn1_param_effective_tag(p),
			tmp, off, buf, size);
}
void asn1_sequence_free(const struct asn1_param *p, void *field)
{
	uint64_t seen_mask;

	const struct asn1_type_desc *desc = p->type_desc;
	
	seen_mask = *((uint64_t *)field);

	for(size_t i = 0; i < desc->nb_params; i++)
	{
		const struct asn1_param *par = &desc->params[i];

		if (!(seen_mask & (1ULL << par->bit)))
			continue;

		if (par->flags & ASN1_PARAM_OPTIONAL) {
			void **par_field_ptr = (void **)((uint8_t *)field + par->offset);
			asn1_param_free_owned(par, *par_field_ptr);
			*par_field_ptr = NULL;
		} else {
			void *par_field = (uint8_t *)field + par->offset;
			asn1_param_free_content(par, par_field);
		}
	}
}

/*** CHOICE ***/
static int asn1_choice_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx)
{
	const asn1_type_desc *desc = p->type_desc;
	asn1_cursor c = { buf, len };
	asn1_tag_t tag;
	size_t val_len;
	uint8_t indefinite = 0;
	const uint8_t *val;
	const struct asn1_param *last_param = NULL;


	if (asn1_read_tlv(&c, &tag, &val_len, &val, &indefinite)) {
		debug_printf("asn1_decode_ber: tlv failure\n");
		return -1;
	}
	if (!asn1_tag_matches(p, tag)) {
		debug_printf("asn1_sequence_decode_ber: tag did not match\n");
		return -1;
	}
	
	if (p->flags & ASN1_PARAM_EXPLICIT_TAG) {
		if (asn1_read_tlv(&c, &tag, &val_len, &val, &indefinite)) {
			debug_printf("asn1_decode_ber: tlv failure\n");
			return -1;
		}
	}
	if (p->tag == ASN1_TYPE_ANY) {
		c.p = buf;
		c.len = len;
	}
		
	uint32_t *choice = (uint32_t *)field;

	if(asn1_param_lookup_bounded(desc, &last_param, tag)) {
		asn1_cursor_skip(&c, val_len);
		return -1;
	}

	*choice = last_param->bit;

	void *field_ptr = (uint8_t *)field + asn1_choice_field_offset(last_param);
	void **pp = field_ptr;

	*pp = SS7_CALLOC(ctx, 1, last_param->type_desc->sizeof_struct);
	if (!*pp)
		return -1;

	if (asn1_decode_ber(last_param, c.p, c.len, *pp, ctx)) {
		asn1_param_free_owned(last_param, *pp);
		*pp = NULL;
		*choice = 0;
		return -1;
	}
	
	return 0;
}
static size_t asn1_choice_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	size_t off = 0, rem = size;
	uint32_t choice;
	const struct asn1_type_desc *desc = p->type_desc;
	void *field;
	
	choice = *((uint32_t *)data);

	strbuf[off++] = '{';
	rem--;
	for(size_t i = 0; i < desc->nb_params; i++)
	{
		const struct asn1_param *par = &desc->params[i];

		if (choice != par->bit)
			continue;

		void **field_ptr = (void **)(data + asn1_choice_field_offset(par));
		field = *field_ptr;

		if (!field)
			return 0;
		size_t ret = asn1_print_json(par, field, strbuf + off, rem);
		if(!ret)
			return 0;
		off += ret;
		break;
	}
	if(off > 1 && strbuf[off - 1] == ',')
		off--;
	strbuf[off++] = '}';
	return off;
}
static int asn1_choice_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	uint32_t choice = *((const uint32_t *)field);
	const struct asn1_type_desc *desc = p->type_desc;

	for(size_t i = 0; i < desc->nb_params; i++)
	{
		const struct asn1_param *par = &desc->params[i];
		void * const *field_ptr;

		if (choice != par->bit)
			continue;
		field_ptr = (void * const *)((const uint8_t *)field +
				asn1_choice_field_offset(par));
		return asn1_encode_ber(par, *field_ptr, buf, size);
	}
	return -1;
}
static void asn1_choice_free(const struct asn1_param *p, void *field)
{
	uint32_t choice;
	const struct asn1_type_desc *desc = p->type_desc;
	
	choice = *((uint32_t *)field);

	for(size_t i = 0; i < desc->nb_params; i++)
	{
		const struct asn1_param *par = &desc->params[i];

		if (choice != par->bit)
			continue;

		void **par_field_ptr = (void **)((uint8_t *)field +
				asn1_choice_field_offset(par));
		asn1_param_free_owned(par, *par_field_ptr);
		*par_field_ptr = NULL;
	}
}
/*** SEQUENCE OF ***/
typedef struct {
    void **array;
    size_t count;
} asn1_seqof_ctx;

static int asn1_sequence_of_decode_ber(const struct asn1_param *p,
                        const uint8_t *buf,
                        size_t len,
                        void *field,
			void *ctx)
{
	asn1_cursor c = { buf, len };

	const asn1_type_desc *desc = p->type_desc;
	asn1_tag_t tag;
	size_t val_len;
	const uint8_t *val;
	uint8_t indefinite = 0;

	if (asn1_read_tlv(&c, &tag, &val_len, &val, &indefinite) < 0)
		return -1;

	if (!asn1_tag_matches(p, tag))
		return -1;

/* TODO: Check, tag must be Constructed
    if (ASN1_CONSTRUCTED(tag))
        return -1;
*/
	asn1_SEQUENCE_OF_t *seqof = field;
	asn1_cursor inner = { val, val_len };
	asn1_cursor count_cursor = { val, val_len };
	size_t count = 0;
	size_t index = 0;

	while (count_cursor.len > 0) {
		asn1_tag_t etag;
		size_t elen;
		const uint8_t *eval;
		uint8_t eindefinit = 0;

		if (asn1_read_tlv(&count_cursor, &etag, &elen, &eval, &eindefinit) < 0)
			return -1;
		if(!eindefinit && elen > count_cursor.len)
			return -1;
		if(asn1_cursor_skip(&count_cursor, elen))
			return -1;
		count++;
	}

	seqof->count = count;
	seqof->seq = NULL;
	if(count) {
		seqof->seq = SS7_CALLOC(ctx, count, sizeof(void *));
		if(!seqof->seq) {
			seqof->count = 0;
			return -1;
		}
	}

	while (inner.len > 0) {

		const uint8_t *start = inner.p;

		asn1_tag_t etag;
		size_t elen;
		const uint8_t *eval;
		uint8_t eindefinit;

		if (asn1_read_tlv(&inner, &etag, &elen, &eval, &eindefinit) < 0)
			return -1;
		if(!eindefinit && elen > inner.len) {
			asn1_sequence_of_free(p, field);
			seqof->seq = NULL;
			seqof->count = 0;
			return -1;
		}

		if(asn1_cursor_skip(&inner, elen)) {
			asn1_sequence_of_free(p, field);
			seqof->seq = NULL;
			seqof->count = 0;
			return -1;
		}

		void *elem = SS7_CALLOC(ctx, 1,
				desc->params[0].type_desc->sizeof_struct);

		if (asn1_decode_ber(desc->params, start,
				inner.p - start, elem, ctx) < 0) {
			SS7_FREE(ctx, elem);
			asn1_sequence_of_free(p, field);
			seqof->seq = NULL;
			seqof->count = 0;
			return -1;
		}
		if(index >= seqof->count) {
			asn1_param_free_owned(desc->params, elem);
			asn1_sequence_of_free(p, field);
			seqof->seq = NULL;
			seqof->count = 0;
			return -1;
		}
		seqof->seq[index++] = elem;
	}

	return 0;
}
static size_t asn1_sequence_of_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	const struct asn1_type_desc *desc = p->type_desc;
	void *field;
	size_t ret = 0,
		off = 0,
		rem = size;
	
	if (!data)
		return 0;
	
	asn1_SEQUENCE_OF_t *seqof = (asn1_SEQUENCE_OF_t *)data;

	strbuf[off++] = '[';
	rem--;
	for(size_t i = 0; i < seqof->count; i++){
		field = seqof->seq[i];
		strbuf[off++] = '{';
		rem--;
		ret = asn1_print_json(desc->params, seqof->seq[i],
				strbuf + off, rem);
		if(!ret) {
			off--;
			rem++;
			continue;
		}
		off += ret;
		rem -= ret;
		
		if(off > 0 && strbuf[off - 1] == ',')
			off--;
		strbuf[off++] = '}';
		strbuf[off++] = ',';
		rem -= 2;

	}
	if(off > 1 && strbuf[off - 1] == ',')
		off--;
	strbuf[off++] = ']';
	return off;	
}
static int asn1_sequence_of_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	const struct asn1_type_desc *desc = p->type_desc;
	const asn1_SEQUENCE_OF_t *seqof = field;
	uint8_t tmp[8192];
	size_t off = 0;

	for(size_t i = 0; i < seqof->count; i++){
		int ret = asn1_encode_ber(desc->params, seqof->seq[i],
				tmp + off, sizeof(tmp) - off);
		if (ret < 0)
			return ret;
		off += ret;
	}

	return asn1_write_tlv(asn1_param_effective_tag(p),
			tmp, off, buf, size);
}
static void asn1_sequence_of_free(const struct asn1_param *p, void *field)
{
	asn1_SEQUENCE_OF_t *seqof = field;

	if(!seqof)
		return;
	for(size_t i = 0; i < seqof->count; i++)
		asn1_param_free_owned(p->type_desc->params, seqof->seq[i]);
#ifdef SS7_USE_ARENA
	return;
#endif
	free(seqof->seq);
}

int asn1_primitive_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx)
{
	const struct asn1_type_desc *desc = p->type_desc;

	if(desc->type == ASN1_TYPE_ANY)
		return asn1_any_decode_ber(p, buf, len, field, ctx);
	if(desc->ops && desc->ops->ber_decode)
		return desc->ops->ber_decode(buf, len, p, field, ctx);

	switch(desc->type) {
	case ASN1_TYPE_BOOLEAN:
		return asn1_boolean_decode_ber(p, buf, len, field, ctx);
	case ASN1_TYPE_ENUMERATED:
		return asn1_enumerated_decode_ber(p, buf, len, field, ctx);
	case ASN1_TYPE_INTEGER:
		return asn1_integer_decode_ber(p, buf, len, field, ctx);
	case ASN1_TYPE_BIT_STRING:
		return asn1_bit_string_decode_ber(p, buf, len, field, ctx);
	case ASN1_TYPE_NULL:
		return asn1_null_decode_ber(p, buf, len, field, ctx);
	case ASN1_TYPE_OCTET_STRING:
		return asn1_octet_string_decode_ber(p, buf, len, field, ctx);
	case ASN1_TYPE_OID:
		break;
	case ASN1_TYPE_OBJECT_IDENTIFIER:
		return asn1_object_identifier_decode_ber(p, buf, len, field, ctx);
		break;
	case ASN1_TYPE_REAL:
		return -2;
	case ASN1_TYPE_EMBEDDED_PDV:
		return -2;
	/* TODO: Must be revised later */
	case ASN1_TYPE_EXTERNAL:
	case ASN1_TYPE_SEQUENCE:
		return asn1_sequence_decode_ber(p, buf, len, field, ctx);
	case ASN1_TYPE_CHOICE:
		return asn1_choice_decode_ber(p, buf, len, field, ctx);
	case ASN1_TYPE_SEQUENCE_OF:
		return asn1_sequence_of_decode_ber(p, buf, len, field, ctx);
	case ASN1_TYPE_SET:
		return -2;
	default:
		return -1;
	}
	return -1;
}

int asn1_primitive_encode_ber(const struct asn1_param *p,
		const void *field, uint8_t *buf, size_t size)
{
	const struct asn1_type_desc *desc = p->type_desc;

	if(desc->type == ASN1_TYPE_ANY)
		return asn1_any_encode_ber(p, field, buf, size);
	if(desc->ops && desc->ops->ber_encode)
		return desc->ops->ber_encode(p, field, buf, size);

	switch(desc->type) {
	case ASN1_TYPE_BOOLEAN:
		return asn1_boolean_encode_ber(p, field, buf, size);
	case ASN1_TYPE_ENUMERATED:
		return asn1_enumerated_encode_ber(p, field, buf, size);
	case ASN1_TYPE_INTEGER:
		return asn1_integer_encode_ber(p, field, buf, size);
	case ASN1_TYPE_BIT_STRING:
		return asn1_bit_string_encode_ber(p, field, buf, size);
	case ASN1_TYPE_NULL:
		return asn1_null_encode_ber(p, field, buf, size);
	case ASN1_TYPE_OCTET_STRING:
		return asn1_octet_string_encode_ber(p, field, buf, size);
	case ASN1_TYPE_OID:
		break;
	case ASN1_TYPE_OBJECT_IDENTIFIER:
		return asn1_object_identifier_encode_ber(p, field, buf, size);
	case ASN1_TYPE_REAL:
		return -2;
	case ASN1_TYPE_EMBEDDED_PDV:
		return -2;
	/* TODO: Must be revised later */
	case ASN1_TYPE_EXTERNAL:
	case ASN1_TYPE_SEQUENCE:
		return asn1_sequence_encode_ber(p, field, buf, size);
	case ASN1_TYPE_CHOICE:
		return asn1_choice_encode_ber(p, field, buf, size);
	case ASN1_TYPE_SEQUENCE_OF:
		return asn1_sequence_of_encode_ber(p, field, buf, size);
	case ASN1_TYPE_SET:
		return -2;
	default:
		return -1;
	}
	return -1;
}

size_t asn1_primitive_print_json(const struct asn1_param *p,
		const void *data, char *strbuf, size_t size)
{
	int not_implemented = 0;

	const struct asn1_type_desc *desc = p->type_desc;

	if(desc->type == ASN1_TYPE_ANY)
		return asn1_any_print_json(p, data, strbuf, size);
	switch(desc->type) {
	case ASN1_TYPE_BOOLEAN:
		return asn1_boolean_print_json(p, data, strbuf, size);
	case ASN1_TYPE_ENUMERATED:
		return asn1_enumerated_print_json(p, data, strbuf, size);
	case ASN1_TYPE_INTEGER:
		return asn1_integer_print_json(p, data, strbuf, size);
	case ASN1_TYPE_BIT_STRING:
		return asn1_bit_string_print_json(p, data, strbuf, size);
	case ASN1_TYPE_NULL:
		return asn1_null_print_json(p, data, strbuf, size);
	case ASN1_TYPE_OCTET_STRING:
		return asn1_octet_string_print_json(p, data, strbuf, size);
	case ASN1_TYPE_OID:
		break;
	case ASN1_TYPE_OBJECT_IDENTIFIER:
		return asn1_object_identifier_print_json(p, data, strbuf, size);
	/* TODO: Must be revised later */
	case ASN1_TYPE_EXTERNAL:
	case ASN1_TYPE_SEQUENCE:
		return asn1_sequence_print_json(p, data, strbuf, size);
	case ASN1_TYPE_CHOICE:
		return asn1_choice_print_json(p, data, strbuf, size);
	case ASN1_TYPE_SEQUENCE_OF:
		return asn1_sequence_of_print_json(p, data, strbuf, size);
	case ASN1_TYPE_SET:
	case ASN1_TYPE_REAL:
	case ASN1_TYPE_EMBEDDED_PDV:
	default:
		assert(not_implemented);
	}
}
void  asn1_primitive_free(const struct asn1_param *p, void *field)
{
	int not_implemented = 0;
	const struct asn1_type_desc *desc = p->type_desc;
	if(desc->type == ASN1_TYPE_ANY)
		return;
	switch(desc->type) {
	case ASN1_TYPE_BOOLEAN:
		return asn1_boolean_free(p, field);
	case ASN1_TYPE_ENUMERATED:
		return asn1_enumerated_free(p, field);
	case ASN1_TYPE_INTEGER:
		return asn1_integer_free(p, field);
	case ASN1_TYPE_BIT_STRING:
		return asn1_bit_string_free(p, field);
	case ASN1_TYPE_NULL:
		return asn1_null_free(p, field);
	case ASN1_TYPE_OCTET_STRING:
		return asn1_octet_string_free(p, field);
	case ASN1_TYPE_OID:
		break;
	case ASN1_TYPE_OBJECT_IDENTIFIER:
		return asn1_object_identifier_free(p, field);
	/* TODO: Must be revised later */
	case ASN1_TYPE_EXTERNAL:
	case ASN1_TYPE_SEQUENCE:
		return asn1_sequence_free(p, field);
	case ASN1_TYPE_CHOICE:
		return asn1_choice_free(p, field);
	case ASN1_TYPE_SEQUENCE_OF:
		return asn1_sequence_of_free(p, field);
	case ASN1_TYPE_SET:
	case ASN1_TYPE_REAL:
	case ASN1_TYPE_EMBEDDED_PDV:
	default:
		assert(not_implemented);
	}
}

static void asn1_param_free_owned(const struct asn1_param *p, void *field)
{
	if(!p || !field)
		return;

	asn1_param_free_content(p, field);
#ifdef SS7_USE_ARENA
	return;
#endif
	free(field);
}

static void asn1_param_free_content(const struct asn1_param *p, void *field)
{
	const struct asn1_type_desc *desc;

	if(!p || !field)
		return;
	desc = p->type_desc;

	switch(desc->type) {
	case ASN1_TYPE_NON_PRIMITIVE:
		assert(desc->ops->free);
		desc->ops->free(p, field);
		break;
	case ASN1_TYPE_SEQUENCE:
		asn1_sequence_free(p, field);
		break;
	case ASN1_TYPE_CHOICE:
		asn1_choice_free(p, field);
		break;
	case ASN1_TYPE_SEQUENCE_OF:
		asn1_sequence_of_free(p, field);
		break;
	case ASN1_TYPE_OCTET_STRING:
		asn1_octet_string_free(p, field);
		break;
	default:
		break;
	}
}

struct asn1_type_desc asn1_ANY_type_desc = {
	.name = "ANY",
	.type = ASN1_TYPE_ANY,
	.tag = ASN1_TYPE_ANY,
	.ops = NULL,
	.sizeof_struct = sizeof(asn1_ANY_t),
};

struct asn1_type_ops asn1_NULL_type_ops = {0};
struct asn1_type_desc asn1_NULL_type_desc = {
	.name = "NULL",
	.type = ASN1_TYPE_NULL,
	.ops = &asn1_NULL_type_ops,
	.sizeof_struct = sizeof(asn1_NULL_t),
};

struct asn1_type_ops asn1_INTEGER_type_ops = {0};
struct asn1_type_desc asn1_INTEGER_type_desc = {
	.name = "INTEGER",
	.type = ASN1_TYPE_INTEGER,
	.ops = &asn1_INTEGER_type_ops,
	.sizeof_struct = sizeof(asn1_INTEGER_t),
};

struct asn1_type_ops asn1_ENUMERATED_type_ops = {0};
struct asn1_type_desc asn1_ENUMERATED_type_desc = {
	.name = "ENUMERATED",
	.type = ASN1_TYPE_ENUMERATED,
	.ops = &asn1_ENUMERATED_type_ops,
	.sizeof_struct = sizeof(asn1_ENUMERATED_t),
};

struct asn1_type_ops asn1_BOOLEAN_type_ops = {0};
struct asn1_type_desc asn1_BOOLEAN_type_desc = {
	.name = "BOOLEAN",
	.type = ASN1_TYPE_BOOLEAN,
	.ops = &asn1_BOOLEAN_type_ops,
	.sizeof_struct = sizeof(asn1_BOOLEAN_t),
};

struct asn1_type_ops asn1_OCTET_STRING_type_ops = {0};
struct asn1_type_desc asn1_OCTET_STRING_type_desc = {
	.name = "OCTET STRING",
	.type = ASN1_TYPE_OCTET_STRING,
	.ops = &asn1_OCTET_STRING_type_ops,
	.sizeof_struct = sizeof(asn1_OCTET_STRING_t),
};

struct asn1_type_ops asn1_BIT_STRING_type_ops = {0};
struct asn1_type_desc asn1_BIT_STRING_type_desc = {
	.name = "BIT STRING",
	.type = ASN1_TYPE_BIT_STRING,
	.ops = &asn1_OCTET_STRING_type_ops,
	.sizeof_struct = sizeof(asn1_BIT_STRING_t),
};

struct asn1_type_ops asn1_OBJECT_IDENTIFIER_type_ops = {0};
struct asn1_type_desc asn1_OBJECT_IDENTIFIER_type_desc = {
	.name = "OBJECT IDENTIFIER",
	.type = ASN1_TYPE_OBJECT_IDENTIFIER,
	.ops = &asn1_OBJECT_IDENTIFIER_type_ops,
	.sizeof_struct = sizeof(asn1_OBJECT_IDENTIFIER_t),
};

#endif //__ASN1_PRIMITIVES_H__
