#ifndef __ASN1_H__
#define __ASN1_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define ASN1_EOC 0xffff
#define SIZEOF_DESC(arr) (sizeof(arr) / sizeof((arr)[0]))

//#define ASN1_TAG_ANY 0

#define OFF(type, field) offsetof(type, field)

#define ASN1_PARAM_OPTIONAL  (1u << 0)
#define ASN1_PARAM_IMPLICIT_TAG  (1u << 1)
#define ASN1_PARAM_EXPLICIT_TAG  (1u << 2)
#define ASN1_PARAM_UNIQUE   (1u << 3)
#define ASN1_PARAM_IGNORE_TAG (1u << 4)
#define ASN1_PARAM_ALLOCATE (1u << 5)

#define ASN1_UNIVERSAL(tag) (tag)
#define ASN1_APPLICATION(tag) (0x40 | tag)
#define ASN1_CONTEXT_SPECIFIC(tag) (0x80 | tag)
#define ASN1_PRIVATE(tag) (0xc0 | tag)




#define ASN1_FLAG_IMPLICIT 0x00
#define ASN1_FLAG_EXPLICIT 0x01
//#define ASN1_FLAG_OPTIONAL 0x02
//#define ASN1_FLAG_ALLOCATE 0x04


typedef enum {
	ASN1_TYPE_BOOLEAN = 1,
	ASN1_TYPE_INTEGER,
	ASN1_TYPE_BIT_STRING,
	ASN1_TYPE_OCTET_STRING = 4,
	ASN1_TYPE_NULL = 5,
	ASN1_TYPE_OID = 6,
	ASN1_TYPE_OBJECT_IDENTIFIER,	/* 0x */
	ASN1_TYPE_EXTERNAL,		/* 0x28 */
	ASN1_TYPE_REAL,
	ASN1_TYPE_ENUMERATED,
	ASN1_TYPE_EMBEDDED_PDV,
	ASN1_TYPE_UTF8String,
	ASN1_TYPE_RELATIVE_OID,
	/* 14 Reserved
	 * 15 Reserved */
	ASN1_TYPE_SEQUENCE = 16,
	ASN1_TYPE_SET = 17,
	ASN1_TYPE_NumericString,
	ASN1_TYPE_PrintableString,
	ASN1_TYPE_TeletexString,
	ASN1_TYPE_VideotexString,
	ASN1_TYPE_IA5String,
	ASN1_TYPE_UTCTime,
	ASN1_TYPE_Generalized,
	ASN1_TYPE_GraphicString,
	ASN1_TYPE_VisibleString,
	ASN1_TYPE_GeneralString,
	ASN1_TYPE_UniversalString,
	ASN1_TYPE_CHARACTERSTRING,
	ASN1_TYPE_BMPString,
	ASN1_TYPE_CHOICE,
	ASN1_TYPE_SEQUENCE_OF,
	ASN1_TYPE_NON_PRIMITIVE,
	ASN1_TYPE_ANY,
} asn1_type_t;

typedef struct {
	const uint8_t *p;
	size_t len;
} asn1_cursor;

typedef uint32_t asn1_tag_t;

#define ASN1_TAG_KEY(cls, cons, tag)	\
	(((uint32_t)(cls) << 30) |	\
	((uint32_t)(cons) << 29) |	\
	((uint32_t)(tag)))



typedef int(*register_fn)(void *, void *);

struct asn1_param {
	char *name;
	asn1_tag_t tag;
	uint16_t flags;
	uint16_t bit;
	size_t offset;

	const struct asn1_type_desc *type_desc;

	bool register_val; /* register pointer to its value to an external buffer */
	register_fn register_data;
};

typedef void(*asn1_free_fn)(const struct asn1_param *, void *);

typedef int(*asn1_ber_decode_fn)(const uint8_t *, size_t size, 
		const struct asn1_param *,
		void *ctx,
		void *);

typedef int(*asn1_ber_encode_fn)(const struct asn1_param *,
		const void *ctx,
		uint8_t *buf,
		size_t size);

typedef int(*format_fn)(const uint8_t *buf, size_t size,
		const struct asn1_param *,
		void *ctx, void **ext);

typedef void(*format_free_fn)(const struct asn1_param *, void *);

typedef size_t (*print_fn)(const struct asn1_param *,
		const void *data,
		char *buf,
		size_t size);

/* Operations dedicated to a type */
struct asn1_type_ops {
	asn1_free_fn free;
	asn1_ber_decode_fn ber_decode;
	asn1_ber_encode_fn ber_encode;
	format_fn data_fmt;
	format_free_fn free_fmt;
	print_fn json_print;
	register_fn register_hook;
};

typedef struct asn1_type_desc {
	char *name;
	const asn1_type_t type;
	asn1_tag_t tag;
	uint8_t flags;
	const struct asn1_type_ops *ops;
	uint16_t sizeof_struct;

	/* SEQUENCE, SEQUENCE OF, CHOICE */
	const struct asn1_param *params;
	uint16_t nb_params;
	uint64_t mandatory_mask;
	
	/* OCTET_STRING */
	void *oc_internal;

	const char **bit_str;
	uint16_t bit_str_sz;
	const char **enum_str;
	uint16_t enum_str_sz;
	uint64_t external_id;
} asn1_type_desc;

int asn1_decode_ber(const struct asn1_param *p,
		const uint8_t *buf,
		size_t len,
		void *field,
		void *ctx);

int asn1_encode_ber(const struct asn1_param *p,
		const void *field,
		uint8_t *buf,
		size_t size);

size_t asn1_print_json(const struct asn1_param *p, const void *data,
		char *strbuf,
		size_t size);

void asn1_free(const struct asn1_param *p, void *field);

typedef struct {
	const uint8_t *data;
	size_t size;
} slice_t;

/*** ASN1 Primitive types ***/

/* ANY */
typedef struct {
	void *ptr;
	size_t size;
} asn1_ANY_t;
extern struct asn1_type_desc asn1_ANY_type_desc;

/* NULL */
typedef uint8_t asn1_NULL_t;

int asn1_NULL_ber_decode(const uint8_t *buf, size_t buflen,
		const struct asn1_param *par,
		void *ctx,
		void *);

size_t asn1_NULL_json_print(char *buf, size_t size,
		const struct asn1_param *par,
		const void *ctx);

extern struct asn1_type_desc asn1_NULL_type_desc;
extern struct asn1_type_ops asn1_NULL_type_ops;


/* BOOLEAN */
typedef uint8_t asn1_BOOLEAN_t;

int asn1_BOOLEAN_ber_decode(const uint8_t *buf, size_t buflen,
		const struct asn1_param *par,
		void *ctx,
		void *);

size_t asn1_BOOLEAN_json_print(char *buf, size_t size,
		const struct asn1_param *par,
		const void *ctx);

void asn1_BOOLEAN_free(const struct asn1_param *par, void *ctx);

extern struct asn1_type_ops asn1_BOOLEAN_type_ops;
extern struct asn1_type_desc asn1_BOOLEAN_type_desc;

/* INTEGER */
typedef uint64_t asn1_INTEGER_t;

int asn1_INTEGER_ber_decode(const uint8_t *buf, size_t buflen,
		const struct asn1_param *par,
		void *ctx,
		void *);

size_t asn1_INTEGER_json_print(char *buf, size_t size,
		const struct asn1_param *par,
		const void *ctx);

void asn1_INTEGER_free(const struct asn1_param *par, void *ctx);

extern struct asn1_type_ops asn1_INTEGER_type_ops;
extern struct asn1_type_desc asn1_INTEGER_type_desc;

/* ENUMERATED */
typedef uint64_t asn1_ENUMERATED_t;

size_t asn1_ENUMERATED_json_print(char *buf, size_t size,
		const struct asn1_param *par,
		const void *ctx);
extern struct asn1_type_ops asn1_ENUMERATED_type_ops;
extern struct asn1_type_desc asn1_ENUMERATED_type_desc;

/* OCTET STRING */
typedef struct {
	slice_t slice;
	void *internal;
} asn1_OCTET_STRING_t;

extern struct asn1_type_ops asn1_OCTET_STRING_type_ops;
extern struct asn1_type_desc asn1_OCTET_STRING_type_desc;

/* BIT STRING */
typedef struct {
	slice_t slice;
	uint16_t padding;
	uint16_t nb_bits;
	const uint8_t *bits;
} asn1_BIT_STRING_t;

int asn1_BIT_STRING_ber_decode(const uint8_t *buf, size_t buflen,
		const struct asn1_param *par,
		void *ctx,
		void *);

size_t asn1_BIT_STRING_json_print(char *buf, size_t size,
		const struct asn1_param *par,
		const void *ctx);

void asn1_BIT_STRING_free( const struct asn1_param *par,
		void *ctx);

extern struct asn1_type_ops asn1_BIT_STRING_type_ops;
extern struct asn1_type_desc asn1_BIT_STRING_type_desc;

/* OBJECT IDENTIFIER */
typedef struct {
	uint16_t arc_count;
	uint16_t arcs[16];
} asn1_OBJECT_IDENTIFIER_t;
extern struct asn1_type_ops asn1_OBJECT_IDENTIFIER_type_ops;
extern struct asn1_type_desc asn1_OBJECT_IDENTIFIER_type_desc;

/* EXTERNAL */
enum {
	external_direct_reference,
	external_indirect_reference,
	external_object_descriptor,
	external_encoding,
};
typedef struct EXTERNAL {
	uint64_t seen_mask;	
	asn1_OBJECT_IDENTIFIER_t *direct_reference;
	asn1_INTEGER_t indirect_reference;
	void *object_descriptor;
	union {
		void *single_asn1_type;
		asn1_OCTET_STRING_t *octet_aligned;
		asn1_BIT_STRING_t *arbitrary;
	} encoding;
} asn1_EXTERNAL_t;

int asn1_EXTERNAL_ber_decode(const uint8_t *buf, size_t buflen,
		const struct asn1_param *par,
		void *ctx,
		void *);

size_t asn1_EXTERNAL_json_print(char *strbuf, size_t size,
		const struct asn1_param *par,
		const void *ctx);

void asn1_EXTERNAL_free(const struct asn1_param *par, void *ctx);

/* CHOICE */
int asn1_CHOICE_ber_decode(const uint8_t *buf, size_t buflen,
		const struct asn1_param *par, void *ctx, void *);

size_t asn1_CHOICE_json_print(char *buf, size_t size,
		const struct asn1_param *par,
		const void *ctx);

void asn1_CHOICE_free( const struct asn1_param *par, void *ctx);


/* SEQUENCE OF */
typedef struct {
	uint64_t count;
	void **seq;
} asn1_SEQUENCE_OF_t;

int asn1_SEQUENCE_OF_ber_decode(const uint8_t *buf, size_t buflen, 
		const struct asn1_param *par,
		void *ctx,
		void *);

size_t asn1_SEQUENCE_OF_json_print(char *buf, size_t size,
		const struct asn1_param *par,
		void *ctx);

void asn1_SEQUENCE_OF_free(const struct asn1_param *par, void *ctx);

extern struct asn1_type_ops asn1_SEQUENCE_OF_type_ops;
extern struct asn1_type_desc asn1_SEQUENCE_OF_type_desc;

/* Data types
#define SEQ_MAX_ELEMENTS 64

struct ber_sequence_of {
	void *elements;
	size_t element_size;
	size_t count;
};

struct ber_oid {
    uint32_t arcs[16];
    size_t arc_count;
};


#define BER_PARAM_OPTIONAL  (1u << 0)
#define BER_PARAM_REPEAT   (1u << 1)


struct ber_octet_string {
	const uint8_t *buf;
	size_t len;
};

struct ber_bit_string {
	const uint8_t *buf;
	size_t len;
};
*/
const uint8_t *ber_decode_octet_string(const uint8_t *buf, size_t len,
		void* ctx);

typedef int64_t ber_integer_t;
typedef ber_integer_t ber_enum_t;

typedef enum {
	ASN1_OK = 0,
	ASN1_NEED_MORE = 1,
	ASN1_ERR = -1,
	ASN1_ERR_INVALID_TAG = -2,
	ASN1_ERR_LENGTH = -3,
	ASN1_ERR_OVERFLOW = -3,
	ASN1_ERR_INDEFINITE_UNSUPPORTED = -5,
	ASN1_ERR_RECURSION = -6,
	ASN1_ERR_UNEXPECTED_EOC = -7
} asn1_rc_e;


enum {
	ASN1_CLASS_ANY = -1,
	ASN1_CLASS_UNIVERSAL = 0,
	ASN1_CLASS_APPLICATION = 1,
	ASN1_CLASS_CONTEXT = 2,
	ASN1_CLASS_PRIVATE = 3,
};
enum {
	ASN1_Primitive = 0,
	ASN1_Constructed = 1,
};

#define UNIVERSAL_CONSTRUCTED(octet) \
	(octet == 0x30)

static inline int asn1_length(const uint8_t *buf, size_t len, size_t *out_len, 
		size_t *out_hdr_len, uint8_t *indefinite)
{
	if (len < 1) return -1;

	/* In this case length is indefinite.
	 * it is more complecated than it seems,
	 * indefinite length fields end with EOS (End Of Sequence) character,
	 * which is 0x00 0x00, the problem here is than the field can contain
	 * another field inside it which its value contains actually the EOS
	 * character that it shouldn't have be considered as EOS */
	/* TODO: This calculation right here is wast of time and useless,
	 * decoder must be able to ignore length when indefinit flag is set */
	if (buf[0] == 0x80) {
		if(len < 1)
			return -1;
		for(size_t i = len; i-- > 1;)
			if(!(buf[i - 1]) && !buf[i])
				*out_len = i;
		*indefinite = 1;
		*out_hdr_len = 1;
		return 0;
	}

	if ((buf[0] & 0x80) == 0) {
		*out_len = buf[0];
		*out_hdr_len = 1;
		return 0;
	}

	size_t n = buf[0] & 0x7F;
	if (n == 0) {
		*out_len = 0;
		*out_hdr_len = 1;
		return 0;
	}
	if (n > sizeof(size_t) || len < 1 + n)
		return -1;

	size_t v = 0;
	for (size_t i = 0; i < n; i++)
		v = (v << 8) | buf[1 + i];

	*out_len = v;
	*out_hdr_len = 1 + n;
	return 0;
}

static inline  int asn1_tag(const uint8_t *buf, size_t len,
	asn1_tag_t *out_key, size_t *out_hdr_len)
{
	if (len < 1) return -1;

	uint8_t b = buf[0];
	uint32_t cls = b >> 6;
	uint32_t cons = (b >> 5) & 1;
	uint32_t tag = b & 0x1F;

	size_t off = 1;

	if (tag == 0x1F) {
		tag = 0;
		do {
			if (off >= len) return -1;
			b = buf[off++];
			tag = (tag << 7) | (b & 0x7F);
		} while (b & 0x80);
	}

    *out_key = ASN1_TAG_KEY(cls, cons, tag);
    *out_hdr_len = off;
    return 0;
}

static inline int asn1_param_lookup(const struct asn1_param *table, 
		const struct asn1_param **pos,
		asn1_tag_t key)
{
	//TODO : handle non-unique parameters too
	const struct asn1_param *p;
	asn1_tag_t tag;
	if(!table)
		return -1;

	if(!pos)
		return -1;
	if (!(*pos))
		p = table;
	else
		p = (*pos) + 1;

		
	for (; p->type_desc; ++p) {
		if (p->tag == 0)
			tag = p->type_desc->tag;
		else
			tag = p->tag;
		if (tag == key || tag == ASN1_TYPE_ANY) {
			*pos = p;
			return 0;
		}
	}
	for (p = table; p < *pos; ++p) {
		if (p->tag == 0)
			tag = p->type_desc->tag;
		else
			tag = p->tag;
		if (tag == key) {
			*pos = p;
			return 0;
		}
	}
	return -1;
}

#endif //__ASN1_H__
