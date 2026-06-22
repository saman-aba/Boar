#include <stdio.h>
#include <stdint.h>
#include "val-str.h"
#include <assert.h>
#include <string.h>
#include "m3ua.h"
#define m3ua_debug
//#define TIME_PROBE
#include <time.h>

#define printf_stdout(...) fprintf(stdout, __VA_ARGS__);
#define printf_stderr(...) fprintf(stderr, __VA_ARGS__);

#define err_printf(...) 
//printf_stdout("(m3ua err): "__VA_ARGS__)


//#ifdef m3ua_debug
//#define debug_printf(...)  printf_stdout("(m3ua debug): "__VA_ARGS__)	
//#else
#define debug_printf(...)
//#endif

const struct val_str m3ua_err_names[] = {
	{ M3UA_ERROR_INVALID_VERSION, "Invalid Version" },
	{ M3UA_ERROR_UNSUPPORTED_MESSAGE_CLASS, "Unsupported Message Class" },
	{ M3UA_ERROR_UNSUPPORTED_MESSAGE_TYPE, "Unsupported Message Type" },
	{ M3UA_ERROR_UNSUPPORTED_TRAFFIC_MODE_TYPE, "Unsupported Traffic Mode Type" },
	{ M3UA_ERROR_UNEXPECTED_MESSAGE, "Unexpected Message" },

	{ M3UA_ERROR_PROTOCOL_ERROR, "Protocol Error" },
	{ M3UA_ERROR_INVALID_STREAM_IDENTIFIER, "Invalid Stream Identifier" },
	{ M3UA_ERROR_REFUSED_MANAGEMENT_BLOCKING, "Refused - Management Blocking" },
	{ M3UA_ERROR_ASP_IDENTIFIER_REQUIRED, "ASP Identifier Required" },
	{ M3UA_ERROR_INVALID_ASP_IDENTIFIER, "Invalid ASP Identifier" },
	{ M3UA_ERROR_INVALID_PARAMETER_VALUE, "Invalid Parameter Value" },
	{ M3UA_ERROR_PARAMETER_FIELD_ERROR, "Parameter Field Error" },
	{ M3UA_ERROR_UNEXPECTED_PARAMETER, "Unexpected Parameter" },
	{ M3UA_ERROR_DESTINATION_STATUS_UNKNOWN, "Destination Status Unknown" },
	{ M3UA_ERROR_INVALID_NETWORK_APPEARANCE, "Invalid Network Appearance" },
	{ M3UA_ERROR_MISSING_PARAMETER, "Missing Parameter" },
	{ M3UA_ERROR_INVALID_ROUTING_CONTEXT, "Invalid Routing Context" },
	{ M3UA_ERROR_NO_CONFIGURED_AS_FOR_ASP,"No Configured AS for ASP" },
//	{ SUA_ERR_SUBSYSTEM_STATUS_UNKNOWN, "Subsystem Status Unknown" },
//	{ SUA_ERR_INVAL_LOADSHARING_LEVEL, "Invalid loadsharing level" },
	{ 0, NULL }
};

__attribute__((unused)) static const struct val_str m3ua_iei_names[] = {
	{ COMMON_PARAM_INFO_STRING, "INFO String" },
	{ COMMON_PARAM_ROUTING_CONTEXT, "Routing Context" },
	{ COMMON_PARAM_DIAGNOSTIC_INFORMATION, "Diagnostic Info" },
	{ COMMON_PARAM_HEARTBEAT_DATA, "Heartbeat Data" },
	{ COMMON_PARAM_TRAFFIC_MODE_TYPE, "Traffic Mode Type" },
	{ COMMON_PARAM_ERROR_CODE, "Error Code" },
	{ COMMON_PARAM_STATUS, "Status" },
	{ COMMON_PARAM_ASP_IDENTIFIER, "ASP Identifier" },
	{ COMMON_PARAM_AFFECTED_POINT_CODE, "Affected Point Code" },
	{ COMMON_PARAM_CORRELATION_ID, "Correlation Id" },

	{ M3UA_PARAM_NETWORK_APPEARANCE, "Network Appearance" },
	{ M3UA_PARAM_USER_CAUSE, "User/Cause" },
	{ M3UA_PARAM_CONGESTION_INDICATIONS, "Congestion Indication" },
	{ M3UA_PARAM_CONCERNED_DESTINATION, "Concerned Destination" },
	{ M3UA_PARAM_ROUTING_KEY, "Routing Key" },
	{ M3UA_PARAM_REGISTRATION_RESULT, "Registration Result" },
	{ M3UA_PARAM_DEREGISTRATION_RESULT, "De-Registration Result" },
	{ M3UA_PARAM_LOCAL_ROUTING_KEY_IDENTIFIER, "Local Routing-Key Identifier" },
	{ M3UA_PARAM_DESTINATION_POINT_CODE, "Destination Point Code" },
	{ M3UA_PARAM_SERVICE_INDICATORS, "Service Indicators" },
	{ M3UA_PARAM_ORIGINATING_POINT_CODE_LIST, "Originating Point Code List" },
	{ M3UA_PARAM_PROTOCOL_DATA, "Protocol Data" },
	{ M3UA_PARAM_REGISTRATION_STATUS, "Registration Status" },
	{ M3UA_PARAM_DEREGISTRATION_STATUS, "De-Registration Status" },
	{ 0, NULL }
};

static const struct val_str m3ua_param_type_name_tbl[] = {
	{ COMMON_PARAM_INFO_STRING, "Info String"},
	{ COMMON_PARAM_ROUTING_CONTEXT, "Routing Context"},
	{ COMMON_PARAM_DIAGNOSTIC_INFORMATION, "Diagnostic Information"},
	{ COMMON_PARAM_HEARTBEAT_DATA, "Heartbeat data"},
	{ COMMON_PARAM_TRAFFIC_MODE_TYPE, "Traffic Mode Type"},
	{ COMMON_PARAM_ERROR_CODE, "Error Code"},
	{ COMMON_PARAM_STATUS, "Status"},
	{ COMMON_PARAM_ASP_IDENTIFIER, "ASP Identifier"},
	{ COMMON_PARAM_AFFECTED_POINT_CODE, "Affected Point Code"},
	{ COMMON_PARAM_CORRELATION_ID, "Correlation Id"},
	{ M3UA_PARAM_NETWORK_APPEARANCE, "Network Appearance"},
	{ M3UA_PARAM_USER_CAUSE, "User Cause"},
	{ M3UA_PARAM_CONGESTION_INDICATIONS, "Congestion Indication"},
	{ M3UA_PARAM_CONCERNED_DESTINATION, "Concerned Destination"},
	{ M3UA_PARAM_ROUTING_KEY, "routing Key"},
	{ M3UA_PARAM_REGISTRATION_RESULT, "Registration Result"},
	{ M3UA_PARAM_DEREGISTRATION_RESULT, "Deregistration Result"},
	{ M3UA_PARAM_LOCAL_ROUTING_KEY_IDENTIFIER, "Local Routing Key Identifier"},
	{ M3UA_PARAM_DESTINATION_POINT_CODE, "Destination Point Code"},
	{ M3UA_PARAM_SERVICE_INDICATORS, "Service Indicators"},
	{ M3UA_PARAM_ORIGINATING_POINT_CODE_LIST, "Point Code List"},
	{ M3UA_PARAM_PROTOCOL_DATA, "Protocol Data"},
	{ M3UA_PARAM_REGISTRATION_STATUS, "Registeration Status"},
	{ M3UA_PARAM_DEREGISTRATION_STATUS, "Deregistration Status"},
	{0, NULL}
};

static const struct val_str m3ua_class_name_tbl[] = {
	{ M3UA_CLASS_MGMT, "Management"}, 
	{ M3UA_CLASS_TRANSFER, "Transfer"},
	{ M3UA_CLASS_SSNM, "SSNM"},
	{ M3UA_CLASS_ASPSM, "ASPSM"},
	{ M3UA_CLASS_ASPTM, "ASPTM"},
	{0, NULL}

};

static int m3ua_class_decode_mgmt(struct ss7_ctx *msg, const uint8_t *buf, int len)
{
	return 0;
}

static int m3ua_param_decode_data(struct ss7_ctx *ss7, const uint8_t *buf, int len)
{
	int ret = 0;
	int offt = 0;
	struct m3ua_param *par;
	struct m3ua_proto_data *pdata;
	struct m3ua_msg *msg = ss7->m3ua;

	par = SS7_CALLOC(ss7, 1, sizeof(struct m3ua_param));
	
	INIT_LIST_HEAD(&par->entry);

	par->hdr.tag = ntohs(((struct m3ua_param_hdr *)buf)->tag);
	par->hdr.length = ntohs(((struct m3ua_param_hdr *)buf)->length);

	offt += sizeof(struct m3ua_param_hdr);

	pdata = (struct m3ua_proto_data *)(buf + offt);

	par->u.data.hdr.opc = ntohl(pdata->hdr.opc);
	par->u.data.hdr.dpc = ntohl(pdata->hdr.dpc);
	par->u.data.hdr.si = pdata->hdr.si;
	par->u.data.hdr.ni = pdata->hdr.ni;
	par->u.data.hdr.mp = pdata->hdr.mp;
	par->u.data.hdr.sls = pdata->hdr.sls;

	offt += sizeof(struct m3ua_proto_data_hdr);

	switch( par->u.data.hdr.si)
	{
	case M3UA_DATA_SCCP: {
		ss7->sccp = &par->u.data.u.sccp;
		ret = sccp_msg_decode(ss7, 
				buf + offt, len - offt);
		if(ret < 0) {
			ss7->sccp = NULL;
			SS7_FREE(ss7, par);
			return -1;
		}
		break;
	}
	case M3UA_DATA_TUP:
	case M3UA_DATA_ISUP:
	case M3UA_DATA_BB_ISUP:
	case M3UA_DATA_ST_ISUP:
	case M3UA_DATA_AAL_TYPE2_SIG:
	case M3UA_DATA_BICC:
	case M3UA_DATA_GCP:
	default:
		break;
	}

	list_add(&par->entry, &msg->list);

	return ret;
}

static int m3ua_param_decode_network_appearence(struct ss7_ctx *ss7, 
		const uint8_t *buf, int len)
{
//	struct m3ua_param *par;
//	struct m3ua_msg *msg = ss7->m3ua; 
	
//	par = calloc(1, sizeof(struct m3ua_param));
//	INIT_LIST_HEAD(&par->entry);

	//par->hdr = (struct m3ua_param_hdr *)buf;
//	list_add(&par->entry, &msg->list);

	return 0;
}

static int m3ua_param_decode_routing_ctx(struct ss7_ctx *ss7, 
		const uint8_t *buf, int len)
{
//	struct m3ua_param *par;
//	struct m3ua_msg *msg = ss7->m3ua;
//	par = calloc(1,sizeof(struct m3ua_param));
//	INIT_LIST_HEAD(&par->entry);

	//par->hdr = (struct m3ua_param_hdr *)buf;
//	list_add(&par->entry, &msg->list);
	return 0;
}

static int m3ua_param_decode_correlation_id(struct ss7_ctx *ss7, 
		const uint8_t *buf, int len)
{
	return 0;
}


static int m3ua_class_decode_transfer(struct ss7_ctx *ss7, 
		const uint8_t *buf, int len)
{
	int ret = 0;
	int nb_params = 0;
	int offt = 0;
	struct m3ua_param_hdr *p_hdr;

	struct m3ua_msg *msg = ss7->m3ua;

	if( msg->hdr.type != M3UA_TYPE_DATA)
		return -1;
	
	offt = 0;

	do{
		p_hdr = (struct m3ua_param_hdr *)(buf + offt);
		if( ntohs(p_hdr->length) == 0 || (len - offt) < ntohs(p_hdr->length)){
			break;
		}
		switch(ntohs(p_hdr->tag)) {
		case M3UA_PARAM_PROTOCOL_DATA:
			ret = m3ua_param_decode_data(ss7, buf + offt, 
					ntohs(p_hdr->length));
			return 1;
		case M3UA_PARAM_NETWORK_APPEARANCE:
		case COMMON_PARAM_ROUTING_CONTEXT:
//			ret = m3ua_param_decode_routing_ctx(ss7, buf + offt, 
//					ntohs(p_hdr->length));
//			offt += ntohs(p_hdr->length);
//			break;
		case COMMON_PARAM_CORRELATION_ID:
//			printf( "Param - Correlation Id\n");
//			ret = m3ua_param_decode_correlation_id(ss7, buf + offt, 
//					ntohs(p_hdr->length));
//			break;
		default:
			offt += ntohs(p_hdr->length);
			break;
		}
		if(ret < 0)
			
		nb_params++;
	} while(offt < (len - 3));

	return nb_params;
}

static int m3ua_class_decode_ssnm(struct ss7_ctx *msg, const uint8_t *buf, int len)
{
	
	return 0;
}

static int m3ua_class_decode_aspsm(struct ss7_ctx *msg, const uint8_t *buf, int len)
{
	return 0;
}

static int m3ua_class_decode_asptm(struct ss7_ctx *msg, const uint8_t *buf, int len)
{
	return 0;
}

int m3ua_msg_decode(struct ss7_ctx *ss7, const uint8_t *buf, size_t buf_len)
{
	const uint8_t *pl;
	uint32_t pl_len;
	struct m3ua_msg *msg;

	assert(ss7);
#ifdef TIME_PROBE
struct timespec start, end;
unsigned long long diff;
clock_gettime(CLOCK_MONOTONIC, &start);
#endif
	
	msg = ss7->m3ua; 
	msg->hdr.version = ((struct m3ua_hdr *)buf)->version;
	msg->hdr.klass = ((struct m3ua_hdr *)buf)->klass;
	msg->hdr.type = ((struct m3ua_hdr *)buf)->type;
	msg->hdr.length = ntohl(((struct m3ua_hdr *)buf)->length);
	INIT_LIST_HEAD( &msg->list);

	pl = buf + sizeof(struct m3ua_hdr);
	pl_len = buf_len - sizeof(struct m3ua_hdr);

	if( buf_len != m3ua_msg_length(msg)) {
		debug_printf("M3UA err: invalid buffer length: %ld, header: %ld\n",
			buf_len, m3ua_msg_length(msg));
		return -1;
	}

	switch( msg->hdr.klass){
	case M3UA_CLASS_MGMT:{
		msg->lsize = m3ua_class_decode_mgmt(ss7, pl, pl_len);
		break;
	}
	case M3UA_CLASS_TRANSFER:{
		msg->lsize = m3ua_class_decode_transfer(ss7, pl, pl_len);
		break;
	}
	case M3UA_CLASS_SSNM:{
		msg->lsize = m3ua_class_decode_ssnm(ss7, pl, pl_len);
		break;
	}
	case M3UA_CLASS_ASPSM:{
		msg->lsize = m3ua_class_decode_aspsm(ss7, pl, pl_len);
		break;
	}
	case M3UA_CLASS_ASPTM:{
		msg->lsize = m3ua_class_decode_asptm(ss7, pl, pl_len);
		break;
	}
	default:
		break;
	}
#ifdef TIME_PROBE
clock_gettime(CLOCK_MONOTONIC, &end);
diff = (end.tv_sec - start.tv_sec) * 1000000000LL +
	(end.tv_nsec - start.tv_nsec);
fprintf(stdout, "-- M3UA decode time: %llu ns\n", diff);
#endif

	return 0;
}

const char *m3ua_param_type_str(const struct m3ua_param *par)
{
	return string_from_value(par->hdr.tag, m3ua_param_type_name_tbl);
}
const char *m3ua_msg_class_str(const struct m3ua_msg *msg)
{
	return string_from_value(msg->hdr.klass, m3ua_class_name_tbl);
}


/* ++ JSON formats ++ */

const char m3ua_transfer_data_name[] = "Protocol_data";

 static const struct val_str m3ua_mgmt_types_name_tbl[] = {
	{ M3UA_TYPE_ERR, "ERR"},
	{ M3UA_TYPE_NTFY, "NTFY"},
	{ 0, NULL}
};

static const struct val_str m3ua_ssnm_types_name_tbl[] = {
	{ M3UA_TYPE_DUNA, "DUNA"},
	{ M3UA_TYPE_DAVA, "DAVA"},
	{ M3UA_TYPE_DAUD, "DAUD"},
	{ M3UA_TYPE_SCON, "SCON"},
	{ M3UA_TYPE_DUPU, "DUPU"},
	{ M3UA_TYPE_DRST, "DRST"},
	{ 0, NULL,}
};

static const struct val_str m3ua_aspsm_types_name_tbl[] = {
	{ M3UA_TYPE_ASPUP, "ASPUP"},
	{ M3UA_TYPE_ASPDN, "ASPDN"},
	{ M3UA_TYPE_BEAT, "BEAT"},
	{ M3UA_TYPE_ASPUP_ACK, "ASPUP_ACK"},
	{ M3UA_TYPE_ASPDN_ACK, "ASPDN_ACK"},
	{ M3UA_TYPE_BEAT_ACK, "BEAT_ACK"},
	{ 0, NULL}
};

static const struct val_str m3ua_asptm_types_name_tbl[] = {
	{ M3UA_TYPE_ASPAC, "ASPAC"},
	{ M3UA_TYPE_ASPIA, "ASPIA"},
	{ M3UA_TYPE_ASPAC_ACK, "ASPAC_ACK"},
	{ M3UA_TYPE_ASPIA_ACK, "ASPIA_ACK"},
	{ 0, NULL}
};

static const char *m3ua_msg_type_str(const struct m3ua_msg *msg)
{
	switch(msg->hdr.klass) {
	case M3UA_CLASS_MGMT:
		return string_from_value(msg->hdr.type, m3ua_mgmt_types_name_tbl);
	case M3UA_CLASS_TRANSFER:
		return m3ua_transfer_data_name;
	case M3UA_CLASS_SSNM:
		return string_from_value(msg->hdr.type, m3ua_ssnm_types_name_tbl);
	case M3UA_CLASS_ASPSM:
		return string_from_value(msg->hdr.type, m3ua_aspsm_types_name_tbl);
	case M3UA_CLASS_ASPTM:
		return string_from_value(msg->hdr.type, m3ua_asptm_types_name_tbl);
	default:
		return NULL;
	}
}

static int m3ua_param_data_json_fmt(const struct m3ua_proto_data *data, 
		char *buf, size_t rem)
{
	int off = 0, ret = 0;

	ret = snprintf(buf, rem,
			",\"m3ua_opc\":%d"
			",\"m3ua_dpc\":%d"
			",\"m3ua_si\":%d"
			",\"m3ua_ni\":%d"
			",\"m3ua_mp\":%d"
			",\"m3ua_sls\":%d},",
			data->hdr.opc,
			data->hdr.dpc,
			data->hdr.si,
			data->hdr.ni,
			data->hdr.mp,
			data->hdr.sls);
	off += ret;
	rem -= ret;

	switch( data->hdr.si) {
	case M3UA_DATA_SCCP:
		ret = sccp_msg_json_fmt(&data->u.sccp, buf + off, rem);
		off += ret;
		rem -= ret;
		break;
	default:
		break;
	}

	return off;
}
static int m3ua_param_json_fmt(const struct m3ua_param *par, char *buf, size_t rem)
{
	int off = 0;

	switch (par->hdr.tag) {
	case M3UA_PARAM_PROTOCOL_DATA:
		off = m3ua_param_data_json_fmt(&par->u.data, buf, rem);
		break;
	default:
		break;
	}
	return off;
}

int m3ua_msg_json_fmt(const struct m3ua_msg *msg, char *buf, size_t rem)
{
	int off = 0, ret = 0;
	struct m3ua_param *par, *tmp; 

	ret = snprintf(buf, rem, "\"m3ua\":{"
			"\"m3ua_class\":\"%s\""
			",\"m3ua_type\":\"%s\"",
			m3ua_msg_class_str(msg),
			m3ua_msg_type_str(msg));
	off += ret;
	rem -= ret;

	list_foreach_entry_safe(par, tmp, &msg->list, entry) {
		ret = m3ua_param_json_fmt(par, buf + off, rem);
		rem -= ret;
		off += ret;
	}

	return off;

}

