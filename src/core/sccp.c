#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "hashtable.h"
#include "hash.h"
#include "hlist.h"
#include "jhash.h"
#include "sccp.h"

#define sccp_debug


#define printf_stdout(...) fprintf(stdout, __VA_ARGS__);
#define printf_stderr(...) fprintf(stderr, __VA_ARGS__);

#define err_printf(...)  printf_stdout("(sccp err)"__VA_ARGS__)

#ifdef sccp_debug
#define debug_printf(...)  printf_stdout("(sccp debug):"__VA_ARGS__)
#else
#define debug_printf(...)
#endif

#define XUDT_HASH_BITS 12   /* 4096 buckets */
#define XUDT_REASM_TIMEOUT_NS 1000000

struct xudt_segment {
	uint8_t seg_no;
	uint8_t last;
	size_t len;
	uint8_t *data;
};

/* XUDT Reassembly context */
struct xudt_ctx {
	struct hlist_node hnode;

	uint16_t local_ref;
	uint32_t opc;
	uint32_t dpc;
	uint8_t  ssn;

	uint8_t total_segs;
	uint8_t recv_segs;

	struct xudt_segment seg[16];

	uint64_t last_seen_ns;
};

HASHTABLE_DECLARE(xudt_table, XUDT_HASH_BITS);

struct xudt_key {
	uint16_t local_ref;
	uint32_t opc;
	uint32_t dpc;
	uint8_t  ssn;
} __attribute__((packed));

static inline uint32_t xudt_hash(const struct xudt_key *k)
{
	return hash_32(
	jhash(k, sizeof(*k), 0),
		XUDT_HASH_BITS);
}

static struct xudt_ctx *xudt_ctx_lookup(const struct xudt_key *key)
{
	struct xudt_ctx *ctx;
	uint32_t h = xudt_hash(key);

	hash_for_each_possible(xudt_table, ctx, hnode, h) {
		if (ctx->local_ref == key->local_ref &&
				ctx->opc == key->opc &&
            			ctx->dpc == key->dpc &&
				ctx->ssn == key->ssn)
		return ctx;
	}
	return NULL;
}

static void xudt_ctx_insert(struct xudt_ctx *ctx)
{
	struct xudt_key key = {
		ctx->local_ref, 
		ctx->opc, 
		ctx->dpc, 
		ctx->ssn
	};

	hash_add(xudt_table, &ctx->hnode, xudt_hash(&key));
}

static inline void xudt_ctx_delete(struct xudt_ctx *ctx)
{
	hash_del(&ctx->hnode);
}

static void xudt_gc(uint64_t now_ns)
{
	size_t bkt;
	struct xudt_ctx *ctx;
	struct hlist_node *tmp;

	hash_for_each_safe(xudt_table, bkt, tmp, ctx, hnode) {
		if (now_ns - ctx->last_seen_ns > XUDT_REASM_TIMEOUT_NS) {
			xudt_ctx_delete(ctx);
			free(ctx);
		}
	}
}

typedef enum {
	E164_ENC_BINARY,
	E164_ENC_BCD,
	E164_ENC_UTF8
} e164_encoding_t;

const char tbcd_tbl[] = {
	'0', '1', '2', '3',
	'4', '5', '6', '7',
	'8', '9', '*', '#',
	'a', 'b', 'c', '\0'
};

/* Global title :
 *  Indicator:
 *   0000 : No Global title
 *   0010 : Translation Type 
 *   0100 : Translation type, Numbering Plan, Encoding scheme, Nature of address
 *
 *  Numbering plan:
 *   0001 : ISDN/Telephony
 *   0010 : ISDN/Mobile
 *
 *  Encoding scheme:
 *   0001 : BCD odd
 *   0010 : BCD even
 */
static int sccp_global_title_decode( struct global_title *gt, uint8_t ai, 
		const uint8_t *buf, size_t buflen)
{
	int rem = 0;
	size_t offt = 0;

	if(ai & GT_TT_ONLY)
	{	
		offt++;
	}
	else if( ai & GT_TT_ES_NAI)
	{
		gt->translation_type = buf[offt++];
		gt->numbering_plan = (buf[offt] >> 4);
		gt->encoding_scheme = (buf[offt] & 0xf);
		offt++;
		gt->nature_of_address = buf[offt++]; 
	}

	rem = buflen - offt;
	/* TODO: must be encoded correctly */
	uint8_t ch, cl;
	for(int i = 0; i < rem; i++) {
		ch = buf[offt] >> 4;
		cl = buf[offt] & 0xf;
		gt->digits[gt->digits_len++] = tbcd_tbl[cl];

		if((i == rem - 1) && !(gt->encoding_scheme & GT_ES_EVEN)) {
			offt++;
			break;
		}

		gt->digits[gt->digits_len++] = tbcd_tbl[ch];
		offt++;
	}
	gt->length = offt;
	
	if( offt != buflen)
		fprintf(stderr, "error: sccp_global_title_decode\n");
	return offt;
}

static int sccp_party_address_decode( struct party_address *addr, 
		const uint8_t *buf, size_t buflen)
{
	size_t offt = 0;

	addr->indicator.byte = *buf;
	offt++;
	if(addr->indicator.pc)
	{
		addr->pc = *((uint16_t *)(buf + offt));
		offt += 2;
	}
	if(addr->indicator.ssn)
	{
		addr->pc = *((uint16_t *)(buf + offt));
		offt++;
	}
	if(addr->indicator.gti)
	{
		offt += sccp_global_title_decode(&addr->gt, addr->indicator.gti, 
				buf + offt, buflen - offt);
	}

	if( offt != buflen)
		fprintf(stderr, "error: sccp_party_address_decode\n");
	return offt;
}


/* Extended Unit Data */

static int sccp_msg_decode_xudt(struct ss7_ctx *ctx, const uint8_t *buf, 
		size_t buflen)
{
//	int offt = 0;
//	struct sccp_msg *sccp = (struct sccp_msg *)msg;

//	struct sccp_xudt *xudt = &sccp->data.xudt;

//	xudt->opt_ptr = buf[offt++];
//	xudt->hop_counter = buf[offt++];
//	xudt->opt_ptr = buf[offt++];
	return -1;
}
static int sccp_msg_decode_cr(void *msg, const uint8_t *buf, size_t buflen)
{
	return 0;
}
static int sccp_msg_decode_cc(void *msg, const uint8_t *buf, size_t buflen)
{
	return 0;
}
static int sccp_msg_decode_cref(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	return 0;
}
static int sccp_msg_decode_rlsd(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	return 0;
}
static int sccp_msg_decode_rlc(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	printf("Decode type RLC\n");
	return 0;
}
static int sccp_msg_decode_dt1(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	printf("Decode type DT1\n");
	return 0;
}
static int sccp_msg_decode_dt2(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	printf("Decode type DT2\n");
	return 0;
}
static int sccp_msg_decode_ak(void *msg, const uint8_t *buf, size_t buflen)
{
	printf("Decode type AK\n");
	return 0;
}
static int sccp_msg_decode_udts(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	printf("Decode type UDTS\n");

	return buflen;
}

static int sccp_msg_json_udts_fmt(void *msg, char *buf, size_t rem)
{
	int offt = 0;
	return offt;
}

static int sccp_msg_decode_ed(void *msg, const uint8_t *buf, size_t buflen)
{
	printf("Decode type ED\n");
	return 0;
}
static int sccp_msg_decode_ea(void *msg, const uint8_t *buf, size_t buflen)
{
	printf("Decode type EA\n");
	return 0;
}
static int sccp_msg_decode_rsr(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	printf("Decode type RSR\n");
	return 0;
}
static int sccp_msg_decode_rsc(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	printf("Decode type RSC\n");
	return 0;
}
static int sccp_msg_decode_err(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	printf("Decode type ERR\n");
	return 0;
}
static int sccp_msg_decode_it(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	printf("Decode type IT\n");
	return 0;// buflen;
}
static int sccp_msg_decode_xudts(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	printf("Decode type XUDTS\n");
	return 0;
}
static int sccp_msg_decode_ludt(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	printf("Decode type LUDT\n");
	return 0;
}
static int sccp_msg_decode_ludts(void *msg, const uint8_t *buf, 
		size_t buflen)
{
	printf("Decode type LUDTS\n");
	return 0;
}

/* Unit Data */
static int sccp_msg_udt_decode(struct ss7_ctx *ctx, const uint8_t *buf, size_t buflen)
{
	size_t offt = 0;
	struct sccp_msg *sccp = ctx->sccp;
	struct sccp_udt *udt = &sccp->data.udt;

	udt->ptr1 = buf[offt++];
	udt->ptr2 = buf[offt++];
	udt->ptr3 = buf[offt++];
	
	udt->called.length = *(buf + udt->ptr1);
	offt++;
	offt += sccp_party_address_decode( &udt->called, buf + udt->ptr1 + 1, 
			udt->called.length); 
	udt->calling.length = *(buf + udt->ptr2 + 1);
	offt++;
	offt +=  sccp_party_address_decode( &udt->calling, buf + udt->ptr2 + 2, 
			udt->calling.length); 

	ctx->tcap = &udt->tcap;
	if(tcap_decode(ctx, buf + udt->ptr3 + 3, (size_t)(buf + udt->ptr3 + 2)[0]))
		debug_printf("sccp_msg_udt_decode: tcap decode failure\n");
	
	return 0;
}

dispatch_decode_fn dispatch_decode_tbl[] = {
	0,
//	sccp_msg_decode_cr,
//	sccp_msg_decode_cc,
//	sccp_msg_decode_cref,
//	sccp_msg_decode_rlsd,
//	sccp_msg_decode_rlc,
//	sccp_msg_decode_dt1,
//	sccp_msg_decode_dt2,
//	sccp_msg_decode_ak,
	sccp_msg_udt_decode,
//	sccp_msg_decode_udts,
//	sccp_msg_decode_ed,
//	sccp_msg_decode_ea,
//	sccp_msg_decode_rsr,
//	sccp_msg_decode_rsc,
//	sccp_msg_decode_err,
//	sccp_msg_decode_it,
//	sccp_msg_decode_xudt,
//	sccp_msg_decode_xudts,
//	sccp_msg_decode_ludt,
//	sccp_msg_decode_ludts,
	NULL
};

int sccp_msg_decode(struct ss7_ctx *ss7, const uint8_t *buf, size_t buflen)
{
	size_t offt = 0;
	const uint8_t *hold_ptr;

	struct sccp_msg *msg = ss7->sccp;
	assert(msg);

	msg->type = buf[offt++];
	
	msg->klass = (buf[offt] & 0xf);
	msg->message_handling = ((buf[offt] >> 4));
	offt++;

	hold_ptr = buf + offt;

	if (msg->type != 9)
		return -1;

	offt += sccp_msg_udt_decode(ss7, hold_ptr, buflen - offt);

	//offt += dispatch_decode_tbl[msg->type](ss7, hold_ptr, 
	//		buflen - offt);
	
	return offt;

}

void sccp_free(struct sccp_msg *msg) 
{
	if(msg->type == SCCP_MSG_TYPE_UDT)
		tcap_free(&msg->data.udt.tcap); 
}

const struct tcap_msg *sccp_get_tcap(const struct sccp_msg *msg) 
{
//	const struct tcap_msg *tcap = NULL;
	if( !msg) {
		debug_printf("empty sccp object.\n");
		return NULL;
	}

//	if(msg->type == SCCP_MSG_TYPE_UDT)
//		tcap = msg->data.udt.tcap;

	return NULL; //tcap;
}

/* == Json logs == */

static int sccp_party_address_json_fmt(struct party_address *addr, 
		char *buf, size_t rem, const char *name)
{
	int offt = 0, ret;
	bool pc = addr->indicator.pc;
	bool ssn = addr->indicator.ssn;
	bool gt = addr->indicator.gti;

	ret = snprintf(buf, rem, "\"%s\":{" 
		"\"pc\":\"%d\","
		"\"ssn\":\"%d\","
		"\"gt\":\"%s\"}",
		name,
		(pc) ? pc : 0,
		(ssn)? ssn : 0,
		(gt) ? addr->gt.digits :"no-gt");
	rem -= ret;
	offt += ret;
	
	return offt;
}
/* JSON :
 *  "sccp_udt":{
 *  }
 */
static int sccp_msg_udt_json_fmt(void *msg, 
		char *buf, size_t rem)
{
	struct sccp_msg *sccp = (struct sccp_msg *)msg;
	struct sccp_udt *udt = &sccp->data.udt;
	int offt = 0, ret;

	ret = snprintf(buf, rem, "\"message_type\":\"udt\",");
	rem -= ret;
	offt += ret;
	ret = sccp_party_address_json_fmt(&udt->calling, buf + offt, 
			rem, "calling_party");
	rem -= ret;
	offt += ret;
	buf[offt++] = ',';
	ret = sccp_party_address_json_fmt(&udt->called, buf + offt, 
			rem, "called_party");
	rem -= ret;
	offt += ret;

	buf[offt++] = '}';
	buf[offt++] = ',';
	rem -= 2;

	ret = tcap_json_format(&udt->tcap, buf + offt, rem);
	rem -= ret;
	offt += ret;

	return offt;
}
static int sccp_msg_udts_json_fmt(void *msg, char *buf, size_t rem)
{
	int ret = 0;

	ret = snprintf(buf, rem, "\"message_type\":\"udts\"}");


	return ret;
}

/* XUDT packets must be logged after reassembly */
static int sccp_msg_json_xudt_fmt(void *msg, char *buf, size_t rem)
{
	int ret = 0;
	ret = snprintf(buf, rem, "\"message_type\":\"xudt\"}");
	return ret;
}

dispatch_json_format_fn dispatch_json_format_tbl[] = {
	0,
	0, /* sccp_msg_json_format_cr, */
	0, /* sccp_msg_json_format_cc, */
	0, /* sccp_msg_json_format_cref, */
	0, /* sccp_msg_json_format_rlsd, */
	0, /* sccp_msg_json_format_rlc, */
	0, /* sccp_msg_json_format_dt1, */
	0, /* sccp_msg_json_format_dt2, */
	0, /* sccp_msg_json_format_ak, */
	sccp_msg_udt_json_fmt,
	sccp_msg_json_udts_fmt,
	0, /* sccp_msg_json_format_ed, */
	0, /* sccp_msg_json_format_ea, */
	0, /* sccp_msg_json_format_rsr, */
	0, /* sccp_msg_json_format_rsc, */
	0, /* sccp_msg_json_format_err, */
	0, /* sccp_msg_json_format_it, */
	sccp_msg_json_xudt_fmt, 
	0, /* sccp_msg_json_format_xudts, */
	0, /* sccp_msg_json_format_ludt, */
	0, /* sccp_msg_json_format_ludts, */
	0
};

int sccp_msg_json_fmt(const struct sccp_msg *msg, char *buf, size_t rem)
{
	int offt = 0, ret = 0;

	ret = snprintf(buf, rem, "\"sccp\":{");
	rem -= ret;
	offt += ret;
	
	if(dispatch_json_format_tbl[msg->type]){
		ret = dispatch_json_format_tbl[msg->type](msg, 
				buf + offt, rem);
		rem -= ret;
		offt += ret;
	}

	return offt;
}
