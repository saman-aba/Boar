#ifndef __SCCP_H__
#define __SCCP_H__

#include <stdint.h>
#include "val-str.h"

#include "ss7.h"

#include "generated/tcap.h"

#define GT_NO_GT	0
#define GT_TT_ONLY 	2
#define GT_TT_ES_NAI	4

#define GT_ES_EVEN 0x2

#define SCCP_AI_GTI_NO_GT 0x0
#define SCCP_ITU_AI_GTI_NAI 0x1
#define SCCP_AI_GTI_TT 0x2
#define SCCP_ITU_AI_GTI_TT_NP_ES 0x3
#define SCCP_ITU_AI_GTI_TT_NP_ES_NAI 0x4



#define SCCP_MSG_TYPE_CR    0x01
#define SCCP_MSG_TYPE_CC    0x02
#define SCCP_MSG_TYPE_CREF  0x03
#define SCCP_MSG_TYPE_RLSD  0x04
#define SCCP_MSG_TYPE_RLC   0x05
#define SCCP_MSG_TYPE_DT1   0x06
#define SCCP_MSG_TYPE_DT2   0x07
#define SCCP_MSG_TYPE_AK    0x08
#define SCCP_MSG_TYPE_UDT   0x09
#define SCCP_MSG_TYPE_UDTS  0x0a
#define SCCP_MSG_TYPE_ED    0x0b
#define SCCP_MSG_TYPE_EA    0x0c
#define SCCP_MSG_TYPE_RSR   0x0d
#define SCCP_MSG_TYPE_RSC   0x0e
#define SCCP_MSG_TYPE_ERR   0x0f
#define SCCP_MSG_TYPE_IT    0x10
#define SCCP_MSG_TYPE_XUDT  0x11
#define SCCP_MSG_TYPE_XUDTS 0x12
#define SCCP_MSG_TYPE_LUDT  0x13
#define SCCP_MSG_TYPE_LUDTS 0x14


typedef enum _sccp_payload_t {
    SCCP_PLOAD_NONE,
    SCCP_PLOAD_BSSAP,
    SCCP_PLOAD_RANAP,
    SCCP_PLOAD_NUM_PLOADS
} sccp_payload_t;

struct sccp_seg {
	uint32_t first:1;
	uint32_t klass:1;
	uint32_t spare:2;
	uint32_t remaining:4;
	uint32_t local_ref:24;
};

struct addr_indic {
	union {
		struct {
			uint8_t pc:1;
			uint8_t ssn:1;
			uint8_t gti:4;
			uint8_t routing:1;
			uint8_t national:1;
		};
		uint8_t byte;
	};
};

#define GT_DIGIT_MAX 16
struct global_title {
	uint8_t translation_type;
	uint8_t numbering_plan:4;
	uint8_t encoding_scheme:4;
	uint8_t nature_of_address;
	uint32_t length;
	char digits[GT_DIGIT_MAX];
	uint8_t digits_len;
	uint16_t cc;
};

struct party_address {
	struct addr_indic indicator;
	uint8_t byte;
	uint16_t pc;
	uint8_t ssn;
	struct global_title gt;
	uint8_t length;
};

struct sccp_xudt {
	uint8_t ptr1;
	uint8_t ptr2;
	uint8_t ptr3;
	uint8_t opt_ptr;
	uint8_t hop_counter;
	struct sccp_seg segmentation;

};

struct sccp_udt {
	uint8_t ptr1;
	uint8_t ptr2;
	uint8_t ptr3;
	
	struct party_address calling;
	struct party_address called;

	struct tcap_msg tcap;
};

struct sccp_udts {
};

struct sccp_msg {
	uint8_t type;
	uint8_t klass:4;
	uint8_t message_handling:4;
	
	union {
		struct sccp_udt udt;
		struct sccp_udts udts;
		struct sccp_xudt xudt;
	} data;

	uint16_t nb_params;

	/* these pointers expose decoded inner layers to upper layers */
	struct tcap_msg *tcap;

	/* Back pointer to ss7 stack */
	struct ss7 *ss7_stack;
};

struct sccp_association {
	uint32_t id;
	uint32_t calling_dpc;
	uint32_t called_dpc;
	uint8_t calling_ssn;
	uint8_t called_ssn;
	bool has_fw_key;
	bool has_bw_key;
	struct sccp_msg_info* msgs;
	struct sccp_msg_info* curr_msg;

	sccp_payload_t payload;
	char* calling_party;
	char* called_party;
	char* extra_info;
	char* imsi;
	uint32_t app_info;
};

typedef int (*dispatch_decode_fn)(struct ss7_ctx *, const uint8_t *, size_t);
typedef int (*dispatch_json_format_fn)(void *, char *, size_t);

//const struct val_str sccp_address_signal_values[];
int sccp_msg_decode(struct ss7_ctx *msg, const uint8_t *buf, size_t buflen);

void sccp_free(struct sccp_msg *);

const struct tcap_msg *sccp_get_tcap(const struct sccp_msg*msg);

/* Json log */
int sccp_msg_json_fmt(const struct sccp_msg *msg, char *buf, size_t rem);
#endif //__SCCP_H_


