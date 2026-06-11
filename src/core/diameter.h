#ifndef __DIAMETER_H__
#define __DIAMETER_H__

/* Table 1 - Data Formats

	OctetString 	- Variable length and must be padded to aligned in 32 bits
	Integer32	12(16)
	Integer64	16(20)
	Unsigned32	12(16)
	Unsigned64	16(20)
	Float32		12(16)
	Float64		16(20)
	Grouped		8(12) + the total length of included avps
*/

/* Table 3 - Result-Code AVP answers
	1xxx - Informational
	2xxx - Success
	3xxx - Protocol errors
	4xxx - Transient errors
	5xxx - Permanent failure
*/

#include <stdint.h>
#include <string.h>
#include <sys/queue.h>
#include <jansson.h>
#include "avp.h"

#define PAD4(x) ((x) + ((4 - (x)) & 3))
/* L4 Protos */
#define DIAMETER_OVER_SCTP 0
#define DIAMETER_OVER_TCP 1

#define DIAMETER_MULTI_ROUND_AUTH 1001

#define DIAMETER_SUCCESS 2001
#define DIAMETER_LIMITED_SUCCESS 2002

#define DIAMETER_COMMAND_UNSUPPORTED 3001
#define DIAMETER_UNABLE_TO_DELIVER 3002
#define DIAMETER_REALM_NOT_SERVED 3003
#define DIAMETER_TOO_BUSY 3004
#define DIAMETER_LOOP_DETECTED 3005
#define DIAMETER_REDIRECT_INDICATION 3006
#define DIAMETER_APPLICATION_UNSUPPORTED 3007
#define DIAMETER_INVALID_HDR_BITS 3008
#define DIAMETER_INVALID_AVP_BITS 3009
#define DIAMETER_UNKNOWN_PEER 3010

#define DIAMETER_AUTHENTICATION_REJECTED 4001
#define DIAMETER_OUT_OF_SPACE 4002
#define ELECTION_LOST 4003

#define DIAMETER_AVP_UNSUPPORTED 5001
#define DIAMETER_AVP_UNKNOWN_SESSION_ID 5002
#define DIAMETER_AUTHORIZATION_REJECTED 5003
#define DIAMETER_INVALID_AVP_VALUE 5004
#define DIAMETER_MISSING_AVP 5005
#define DIAMETER_RESOURCE_EXCEEDED 5006
#define DIAMETER_CONTRADICTING_AVPS 5007
#define DIAMETER_AVP_NOT_ALLOWED 5008
#define DIAMETER_AVP_OCCURS_TOO_MANY_TIMES 5009
#define DIAMETER_NO_COMMON_APPLICATION 5010
#define DIAMETER_UNSUPPORTED_VERSION 5011
#define DIAMETER_UNABLE_TO_COMPLY 5012
#define DIAMETER_INVALID_BIT_IN_HEADER 5013
#define DIAMETER_INVALID_AVP_LENGTH 5014
#define DIAMETER_INVALID_MESSAGE_LENGTH 5015
#define DIAMETER_INVALID_AVP_BIT_COMBO 5016
#define DIAMETER_NO_COMMON_SECURITY 5017

typedef enum {
	Invalid = -1,
	Unknown,
	OctetString,
	Integer32,
	Integer64,
	Unsigned32,
	Unsigned64,
	Float32,
	Float64,
	Grouped
} avp_type;

typedef enum {
	OS_Invalid = -1,
	OS_Unknown,
	OS_OctetString,
	OS_Address,
	OS_Time,
	OS_UTF8String,
	OS_DiameterIdentity,
	OS_DiameterURI,
	OS_IPFilterRule,
	OS_QoSFilterRule
} os_derived;

#define SIZEOF_UNSIGNED64 8
#define SIZEOF_UNSIGNED32 4
#define SIZEOF_INTEGER64 8
#define SIZEOF_INTEGER32 4
#define SIZEOF_FLOAT64 8
#define SIZEOF_FLOAT32 8

#define SIZEOF_VENDOR_ID 4
//unsigned int 	r:1; /*	(1)Request/
//			(0)Answer */
//unsigned int 	p:1; /*	(1)Proxied,Relayed,Redirected/
//			(0)*/
//unsigned int	e:1; /* (1)Request caused error/
//		(0)*/
//unsigned int 	t:1;
//unsigned int 	reserved:4;

#define FLAG_REQUEST 0x80
#define FLAG_PROXYABLE 0x40
#define FLAG_ERROR 0x20
#define FLAG_RE_TRANS 0x10

#define AVP_FLAG_VENDOR 0x80
#define AVP_FLAG_MANDATORY 0x40
#define AVP_FLAG_PROTECTED 0x20
#define AVP_FLAG_RESERVED_4 0x10
#define AVP_FLAG_RESERVED_3 0x08
#define AVP_FLAG_RESERVED_2 0x04
#define AVP_FLAG_RESERVED_1 0x02
#define AVP_FLAG_RESERVED_0 0x01

struct diameter_hdr {
#define DIAMETER_HEADER_LEN
	union {
		unsigned int raw1;
		struct {
			unsigned int version : 8;
			unsigned int length : 24;
		};
	};
	union {
		unsigned int raw2;
		struct {
			unsigned int flags : 8;
			unsigned int command_code : 24;
		};
	};
	unsigned int application_id;
	unsigned int hop_by_hop_id;
	unsigned int end_to_end_id;
};

struct diameter_avp_hdr {
	unsigned int code;
	union {
		unsigned int raw2;
		struct {
			unsigned int flags : 8;
			unsigned int length : 24;
		};
	};
};

#define AVP_HEADER_SIZE 8
#define DIAMETER_AVP_FOREACH(avp) for( struct diameter_avp *h;

struct diameter_avp {
	struct diameter_avp *next;
	unsigned short id;
	avp_type type;
	unsigned short pad;
	unsigned int vendor_id;
	struct diameter_avp_hdr header;
	union {
		uint64_t value;
		union {
			char *octetstring;
			uint64_t unsigned64;
			uint32_t unsigned32;
			int64_t int64;
			int32_t int32;
			float float32;
			double float64;
			void *group;
		} data;
	};
};

struct diameter_pkt {
	struct diameter_hdr header;
	struct diameter_avp *avp_list;
};

#define AVP_HEADER(avp_ptr) (avp_ptr->header)
#define AVP_DATA(avp_ptr) avp_ptr->data + AVP_HEADER_SIZE

#define AVP_DATA_PAD(data) (4 - strlen(data) % 4) % 4

void diameter_init();

void diameter_clean_up();

struct diameter_avp *diameter_new_avp(avp_type type, unsigned int code,
				      unsigned char flags, uint64_t data,
				      int data_sz, unsigned int vendor_id);

struct diameter_pkt *diameter_new_packet();

void diameter_packet_free(struct diameter_pkt *);

void diameter_insert_avp(struct diameter_pkt *pkt, struct diameter_avp *in);

void diameter_insert_avp_after(struct diameter_pkt *pkt,
			       struct diameter_avp *node,
			       struct diameter_avp *newavp);

void diameter_insert_avp_before(struct diameter_pkt *pkt,
				struct diameter_avp *avp);

void diameter_swap_avp(const struct diameter_pkt *pkt,
		       struct diameter_avp **first,
		       struct diameter_avp **second);

void diameter_remove_avp(struct diameter_pkt *pkt, struct diameter_avp *avp);

struct diameter_pkt *diameter_read_json_packet(const char *pkt);

struct diameter_pkt *diameter_read_json_file(const char *filename);

int diameter_serialize_packet(const struct diameter_pkt *pkt, char *buf);

void diameter_deserialize_packet(const char *buf, int buf_size,
				 struct diameter_pkt *pkt);

int diameter_detect_proto(const char *, uint32_t, uint32_t);

int diameter_send(struct diameter_pkt *pkt, const char *addr, uint16_t port,
		  char proto, uint64_t flags);

void diameter_print_packet(const struct diameter_pkt *pkt);

int diameter_parse_json(struct diameter_pkt *pkt, json_t *diam_obj);

struct diameter_pkt *diameter_parse_buffer(const char *buf, size_t buf_len);

/* Inline funcs */

static inline char
diameter_vendor_id_present_avp(const struct diameter_avp *avp)
{
	return (AVP_HEADER(avp).flags & 0x80);
}

static inline char diameter_set_vendor_id_present_avp(struct diameter_avp *avp)
{
	return AVP_HEADER(avp).flags |= 0x80;
}

static inline char diameter_mandatory_avp(const struct diameter_avp *avp)
{
	return (AVP_HEADER(avp).flags & 0x40);
}

static inline void diameter_set_mandatory_avp(struct diameter_avp *avp)
{
	AVP_HEADER(avp).flags |= 0x40;
}

#endif
