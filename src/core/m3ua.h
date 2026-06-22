#ifndef __M3UA_H__
#define __M3UA_H__

#include "list.h"
#include <stdlib.h>
#include <arpa/inet.h>
#include "ss7.h"
#include "sccp.h"

#include "generated/tcap.h"

struct map_ctx;
#include <stdio.h>

#define PAD4(x) (((4 - (x)) & 3))

struct ss7;

typedef uint32_t __be32;
typedef uint16_t __be16;
#define M3UA_IE_MAX32 32
/* Definitions are based on RFC 4666 */

/* M3UA Common Header
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    Version    |   Reserved    | Message Class | Message Type  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        Message Length                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 * Version: 
 * Current support: 1
*/
#define M3UA_VERSION		0x01

/* Class:		
 *  0     Management (MGMT) Messages
 *  1     Transfer Messages
 *  2     SS7 Signalling Network Management (SSNM) Messages
 *  3     ASP State Maintenance (ASPSM) Messages
 *  4     ASP Traffic Maintenance (ASPTM) Messages
 *  5     Reserved for Other SIGTRAN Adaptation Layers
 *  6     Reserved for Other SIGTRAN Adaptation Layers
 *  7     Reserved for Other SIGTRAN Adaptation Layers
 *  8     Reserved for Other SIGTRAN Adaptation Layers
 *  9     Routing Key Management (RKM) Messages
 *  10 to 127 Reserved by the IETF
 * 128 to 255 Reserved for IETF-Defined Message Class extensions
**/

#define M3UA_CLASS_MGMT 	0x00
#define M3UA_CLASS_TRANSFER 	0x01
#define M3UA_CLASS_SSNM 	0x02
#define M3UA_CLASS_ASPSM 	0x03
#define M3UA_CLASS_ASPTM	0x04

/* Type */
/* Management (MGMT) Messages:
 *  0     Error (ERR)
 *  1     Notify (NTFY)
 *  2 to 127 Reserved by the IETF
 *  128 to 255 Reserved for IETF-Defined MGMT extensions
**/
#define M3UA_TYPE_ERR		0x00
#define M3UA_TYPE_NTFY		0x01

/* Transfer Messages:
 *  0     Reserved
 *  1     Payload Data (DATA)
 *  2 to 127 Reserved by the IETF
 *  128 to 255 Reserved for IETF-Defined Transfer extensions
**/
#define M3UA_TYPE_DATA		0x01

/* SS7 Signalling Network Management (SSNM) Messages:
 *  0     Reserved
 *  1     Destination Unavailable (DUNA)
 *  2     Destination Available (DAVA)
 *  3     Destination State Audit (DAUD)
 *  4     Signalling Congestion (SCON)
 *  5     Destination User Part Unavailable (DUPU)
 *  6     Destination Restricted (DRST)
 *  7 to 127 Reserved by the IETF
 *  128 to 255 Reserved for IETF-Defined SSNM extensions
**/
#define M3UA_TYPE_DUNA		0x01
#define M3UA_TYPE_DAVA		0x02
#define M3UA_TYPE_DAUD		0x03
#define M3UA_TYPE_SCON		0x04
#define M3UA_TYPE_DUPU		0x05
#define M3UA_TYPE_DRST		0x06

/* ASP State Maintenance (ASPSM) Messages:
 *  0        Reserved
 *  1        ASP Up (ASPUP)
 *  2        ASP Down (ASPDN)
 *  3        Heartbeat (BEAT)
 *  4        ASP Up Acknowledgement (ASPUP ACK)
 *  5        ASP Down Acknowledgement (ASPDN ACK)
 *  6        Heartbeat Acknowledgement (BEAT ACK)
 *  7 to 127    Reserved by the IETF
 *  128 to 255    Reserved for IETF-Defined ASPSM extensions
**/
#define M3UA_TYPE_ASPUP 	0x01
#define M3UA_TYPE_ASPDN 	0x02
#define M3UA_TYPE_BEAT  	0x03
#define M3UA_TYPE_ASPUP_ACK 	0x04
#define M3UA_TYPE_ASPDN_ACK 	0x05
#define M3UA_TYPE_BEAT_ACK  	0x06	

/* ASP Traffic Maintenance (ASPTM) Messages:
 *  0        Reserved
 *  1        ASP Active (ASPAC)
 *  2        ASP Inactive (ASPIA)
 *  3        ASP Active Acknowledgement (ASPAC ACK)
 *  4        ASP Inactive Acknowledgement (ASPIA ACK)
 *  5 to 127    Reserved by the IETF
 *  128 to 255    Reserved for IETF-Defined ASPTM extensions
**/
#define M3UA_TYPE_ASPAC 	0x01
#define M3UA_TYPE_ASPIA 	0x02
#define M3UA_TYPE_ASPAC_ACK 	0x03
#define M3UA_TYPE_ASPIA_ACK 	0x04

/* Routing Key Management (RKM) Messages:
 *  0        Reserved
 *  1        Registration Request (REG REQ)
 *  2        Registration Response (REG RSP)
 *  3        Deregistration Request (DEREG REQ)
 *  4        Deregistration Response (DEREG RSP)
 *  5 to 127    Reserved by the IETF
 *  128 to 255    Reserved for IETF-Defined RKM extensions
**/

#define M3UA_TYPE_REG_REQ 	0x01
#define M3UA_TYPE_REG_RSP 	0x02
#define M3UA_TYPE_DEREG_REQ 	0x03
#define M3UA_TYPE_DEREG_RSP 	0x04

struct m3ua_hdr {
	uint8_t version;
	uint8_t reserved;
	uint8_t klass;
	uint8_t type;
	__be32  length;
};

/* Common Parameters */
/* Reserved					0x0000 */
/* Not Used in M3UA				0x0001 */
/* Not Used in M3UA				0x0002 */
/* Not Used in M3UA				0x0003 */
#define COMMON_PARAM_INFO_STRING		0x0004
/* Not Used in M3UA				0x0005 */
#define COMMON_PARAM_ROUTING_CONTEXT		0x0006
#define COMMON_PARAM_DIAGNOSTIC_INFORMATION	0x0007
/* Not Used in M3UA				0x0008 */
#define COMMON_PARAM_HEARTBEAT_DATA		0x0009
/* Not Used in M3UA				0x000a */
#define COMMON_PARAM_TRAFFIC_MODE_TYPE		0x000b
#define COMMON_PARAM_ERROR_CODE			0x000c
#define COMMON_PARAM_STATUS			0x000d
/* Not Used in M3UA				0x000e */
/* Not Used in M3UA				0x000f */
/* Not Used in M3UA				0x0010 */
#define COMMON_PARAM_ASP_IDENTIFIER		0x0011
#define COMMON_PARAM_AFFECTED_POINT_CODE	0x0012
#define COMMON_PARAM_CORRELATION_ID		0x0013

/* M3UA Specific Parameters */
#define M3UA_PARAM_NETWORK_APPEARANCE		0x0200
/* Reserved					0x0201 */
/* Reserved					0x0202 */
/* Reserved					0x0203 */
#define M3UA_PARAM_USER_CAUSE            	0x0204
#define M3UA_PARAM_CONGESTION_INDICATIONS	0x0205
#define M3UA_PARAM_CONCERNED_DESTINATION 	0x0206
#define M3UA_PARAM_ROUTING_KEY			0x0207
#define M3UA_PARAM_REGISTRATION_RESULT		0x0208
#define M3UA_PARAM_DEREGISTRATION_RESULT	0x0209
#define M3UA_PARAM_LOCAL_ROUTING_KEY_IDENTIFIER	0x020a
#define M3UA_PARAM_DESTINATION_POINT_CODE	0x020b
#define M3UA_PARAM_SERVICE_INDICATORS		0x020c
/* Reserved					0x020d */
#define M3UA_PARAM_ORIGINATING_POINT_CODE_LIST	0x020e
/* Reserved					0x020f */
#define M3UA_PARAM_PROTOCOL_DATA		0x0210
/* Reserved					0x0211 */
#define M3UA_PARAM_REGISTRATION_STATUS		0x0212
#define M3UA_PARAM_DEREGISTRATION_STATUS	0x0213
/* Reserved by the IETF               0x0214 to 0xffff */

struct m3ua_param_hdr {
	uint16_t tag;
	uint16_t length;
};

/* Payload Data Message (DATA) :
 *  Network Appearance       Optional
 *  Routing Context          Conditional
 *  Protocol Data            Mandatory
 *  Correlation Id           Optional
**/

/* Protocol Data :
 *  Originating Point Code
 *  Destination Point Code
 *  Service Indicator
 *  Network Indicator
 *  Message Priority
 *  Signalling Link Selection Code (SLS)

 *  User Protocol Data, which includes
 *  MTP3-User protocol elements (e.g., ISUP, SCCP, or TUP parameters)
*/
/* 	Service Indicator (SI):
 *    		0 to 2   Reserved
 *      	   3     SCCP
 *       	   4     TUP
 *       	   5     ISUP
 *    		6 to 8   Reserved
 *       	   9     Broadband ISUP
 *      	  10     Satellite ISUP
 *      	  11     Reserved
 *      	  12     AAL type 2 Signalling
 *      	  13     Bearer Independent Call Control (BICC)
 *      	  14     Gateway Control Protocol
 *      	  15     Reserved
*/

#define M3UA_DATA_SCCP 		0x03
#define M3UA_DATA_TUP 		0x04
#define M3UA_DATA_ISUP		0x05
#define M3UA_DATA_BB_ISUP	0x09
#define M3UA_DATA_ST_ISUP	0x0a
#define M3UA_DATA_AAL_TYPE2_SIG	0x0c
#define M3UA_DATA_BICC		0x0d
#define M3UA_DATA_GCP		0x0e

struct m3ua_proto_data_hdr {
	uint32_t opc;
	uint32_t dpc;
	uint8_t si;
	uint8_t ni;
	uint8_t mp;
	uint8_t sls;
};

struct m3ua_proto_data {
	struct m3ua_proto_data_hdr hdr;
	union {
		struct sccp_msg sccp;
	} u;
};


/* Payload Data Message (DATA) */
/* Destination Available (DAVA) */
/* Destination State Audit (DAUD) */
/*
 *  Network Appearance      Optional
 *  Routing Context         Conditional
 *  Affected Point Code     Mandatory
 *  INFO String             Optional
**/

/* ANSI 24-bit Point Code */
struct ansi_pc {
	uint8_t mask;
	uint8_t network;
	uint8_t cluster;
	uint8_t member;
};

/* ITU 14-bit Point Code */
struct itu_pc {
	uint8_t mask;
	uint8_t zone;
	uint8_t region;
	uint8_t sp;
};

/* Signalling Congestion (SCON) :
 *  Network Appearance       Optional
 *  Routing Context          Conditional
 *  Affected Point Code      Mandatory
 *  Concerned Destination    Optional
 *  Congestion Indications   Optional
 *  INFO String              Optional
**/


/* Management Messages (MGMT) */

/* Error Codes */
#define M3UA_ERROR_INVALID_VERSION 			0x01
/* NOT_USED_IN_M3UA 					0x02 */
#define M3UA_ERROR_UNSUPPORTED_MESSAGE_CLASS 		0x03
#define M3UA_ERROR_UNSUPPORTED_MESSAGE_TYPE 		0x04
#define M3UA_ERROR_UNSUPPORTED_TRAFFIC_MODE_TYPE	0x05
#define M3UA_ERROR_UNEXPECTED_MESSAGE 			0x06
#define M3UA_ERROR_PROTOCOL_ERROR 			0x07
/* NOT USED IN M3UA		 			0x08 */
#define M3UA_ERROR_INVALID_STREAM_IDENTIFIER 		0x09
/* NOT USED IN M3UA 					0x0a */
/* NOT USED IN M3UA 					0x0b */
/* NOT USED IN M3UA 					0x0c */
#define M3UA_ERROR_REFUSED_MANAGEMENT_BLOCKING 		0x0d
#define M3UA_ERROR_ASP_IDENTIFIER_REQUIRED 		0x0e
#define M3UA_ERROR_INVALID_ASP_IDENTIFIER 		0x0f
/* NOT USED IN M3UA		 			0x10 */
#define M3UA_ERROR_INVALID_PARAMETER_VALUE 		0x11
#define M3UA_ERROR_PARAMETER_FIELD_ERROR 		0x12
#define M3UA_ERROR_UNEXPECTED_PARAMETER 		0x13
#define M3UA_ERROR_DESTINATION_STATUS_UNKNOWN 		0x14
#define M3UA_ERROR_INVALID_NETWORK_APPEARANCE 		0x15
#define M3UA_ERROR_MISSING_PARAMETER 			0x16
/* NOT USED IN M3UA	 				0x17 */
/* NOT USED IN M3UA 					0x18 */
#define M3UA_ERROR_INVALID_ROUTING_CONTEXT 		0x19
#define M3UA_ERROR_NO_CONFIGURED_AS_FOR_ASP 		0x1a


/* M3UA message */
struct m3ua_param {
	struct list_head entry;
	struct m3ua_param_hdr hdr;
	union {
		struct m3ua_proto_data data;
	} u;
};

#define M3UA_MAX_PARAM 8
struct m3ua_msg {
	struct m3ua_hdr hdr;
	struct list_head list;
	uint32_t lsize;

	struct sccp_msg *sccp;

	/* Back pointer to ss7 stack */
	struct ss7 *ss7_stack;
};

static inline size_t m3ua_msg_length(const struct m3ua_msg *msg)
{
	return msg->hdr.length;
}
static inline size_t m3ua_param_length(const struct m3ua_param *ent)
{
	return ent->hdr.length;
}

const char *m3ua_msg_class_str(const struct m3ua_msg *);

const char *m3ua_param_type_str(const struct m3ua_param *);

int m3ua_msg_decode(struct ss7_ctx *, const uint8_t *buf, size_t buf_len);

void m3ua_msg_cleanup(struct m3ua_msg *msg);

int m3ua_msg_json_fmt(const struct m3ua_msg *msg, char *buf, size_t rem);

static inline void m3ua_params_free(struct m3ua_msg *msg)
{
	struct m3ua_param *par, *tmp; 
	if(!list_empty(&msg->list)) {
		list_foreach_entry_safe(par, tmp, &msg->list, entry) {
        		list_delete_init(&par->entry);
			if(par){
				if(par->hdr.tag == M3UA_PARAM_PROTOCOL_DATA &&
					par->u.data.hdr.si == M3UA_DATA_SCCP)
				{
					sccp_free(&par->u.data.u.sccp);
				}
				free(par);
			}
		}
	}
}
static inline void m3ua_msg_free(struct m3ua_msg *msg)
{
	if(msg) {
		m3ua_params_free(msg);
		free(msg);
		msg = NULL;
	}
	return;
}

static inline const struct sccp_msg *m3ua_get_sccp(const struct m3ua_msg *msg)
{
	return msg->sccp;
}
static inline const struct tcap_msg *m3ua_get_tcap(const struct m3ua_msg *msg)
{
	const struct sccp_msg *sccp = NULL;
	sccp = m3ua_get_sccp(msg);
	if(sccp)
		return NULL; //sccp->tcap;
}
static inline const struct map_ctx *const *m3ua_get_map(const struct m3ua_msg *msg, uint16_t *nb_map)
{
	const struct map_ctx *const*map = NULL;
//	const struct sccp_msg *sccp = NULL;
//	const struct tcap_msg *tcap = NULL;

//	sccp = m3ua_get_sccp(msg);
//	if(sccp)
//		tcap = m3ua_get_tcap(msg);
	/* if(tcap)
		map = tcap->map; */
	
	return map;
}


/* Utility Functions 
const struct sccp_msg *m3ua_get_sccp(const struct m3ua_msg *msg);

const struct tcap_msg *m3ua_get_tcap(struct m3ua_msg *msg);

int m3ua_get_component_opcode();

const struct map_ctx *m3ua_get_map(const struct m3ua_msg *msg);

uint32_t m3ua_get_map_opcode(const struct m3ua_msg *msg)
{

	const struct map_ctx *map = NULL;
	map = m3ua_get_map(msg);
	if(map)
		return map->opcode;
	return 0;
}

int m3ua_get_map_imsi(const struct m3ua_msg *msg, char *imsi)
{
	const struct map_ctx *map;
	map = m3ua_get_map(msg);
	return -1;
}

int m3ua_get_map_msisdn(const struct m3ua_msg *msg, char *imsi)
{
//	const struct map_ctx *map;
	return -1;
}

*/

#endif //__M3UA_H__
