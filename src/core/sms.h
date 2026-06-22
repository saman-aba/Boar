#ifndef __SMS_H__
#define __SMS_H__

#include <stddef.h>
#include <stdint.h>

#define SMS_MAX_ADDR_DIGITS 32
#define SMS_MAX_TIMESTAMP 40
#define SMS_MAX_DCS_DESC 64
#define SMS_MAX_USER_DATA_TEXT 512
#define SMS_MAX_USER_DATA_HEX 1024
#define SMS_MAX_UDH_HEX 512

enum sms_tpdu_type {
	SMS_TPDU_UNKNOWN = 0,
	SMS_TPDU_DELIVER,
	SMS_TPDU_SUBMIT,
};

enum sms_alphabet {
	SMS_ALPHABET_GSM7 = 0,
	SMS_ALPHABET_8BIT,
	SMS_ALPHABET_UCS2,
	SMS_ALPHABET_RESERVED,
};

typedef struct sms_addr {
	char digits[SMS_MAX_ADDR_DIGITS];
	uint8_t ton;
	uint8_t npi;
	uint8_t present;
} sms_addr_t;

typedef struct sms_dcs_info {
	uint8_t raw;
	enum sms_alphabet alphabet;
	uint8_t compressed;
	uint8_t has_message_class;
	uint8_t message_class;
	uint8_t class_meaning_valid;
	uint8_t marked_for_deletion;
	char alphabet_name[16];
	char description[SMS_MAX_DCS_DESC];
} sms_dcs_info_t;

typedef struct sms_user_data {
	uint8_t length;
	uint8_t header_present;
	uint8_t header_length;
	uint8_t septet_padding;
	char header_hex[SMS_MAX_UDH_HEX];
	char text[SMS_MAX_USER_DATA_TEXT];
	char data_hex[SMS_MAX_USER_DATA_HEX];
	uint8_t text_valid;
} sms_user_data_t;

typedef struct sms_deliver_flags {
	uint8_t mms;
	uint8_t lp;
	uint8_t sri;
	uint8_t rp;
	uint8_t udhi;
	uint8_t mti;
} sms_deliver_flags_t;

typedef struct sms_submit_flags {
	uint8_t rd;
	uint8_t vpf;
	uint8_t srr;
	uint8_t udhi;
	uint8_t rp;
	uint8_t mti;
} sms_submit_flags_t;

typedef struct sms_message {
	enum sms_tpdu_type type;
	uint8_t first_octet;
	uint8_t valid;
	union {
		sms_deliver_flags_t deliver;
		sms_submit_flags_t submit;
	} flags;
	sms_addr_t tp_originating_address;
	sms_addr_t tp_destination_address;
	uint8_t tp_message_reference;
	uint8_t tp_message_reference_present;
	uint8_t tp_pid;
	sms_dcs_info_t tp_dcs;
	char tp_service_centre_time_stamp[SMS_MAX_TIMESTAMP];
	uint8_t tp_service_centre_time_stamp_present;
	uint8_t tp_validity_period_format;
	uint8_t tp_validity_period_present;
	uint8_t tp_validity_period_relative;
	char tp_validity_period_absolute[SMS_MAX_TIMESTAMP];
	sms_user_data_t tp_user_data;
} sms_message_t;

void sms_hex_encode(const uint8_t *src, size_t len, char *dst, size_t dst_len);
int sms_decode_tpdu(const uint8_t *buf, size_t len, sms_message_t *out);
size_t sms_json_print(const sms_message_t *sms, char *buf, size_t size);

#endif
