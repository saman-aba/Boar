#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct boar_db boar_db;

typedef struct boar_db_packet_record
{
	char *id;
	char *name;
	char *protocol;
	char *summary;
	char *export_base_name;
	char *created_at;
	char *source_mac;
	char *destination_mac;
	char *source_ip;
	char *destination_ip;
	char *transport_protocol;
	int source_port;
	int destination_port;
	char *sctp_tag;
	char *packet_type_key;
	unsigned char *payload;
	size_t payload_size;
	unsigned char *frame_bytes;
	size_t frame_bytes_size;
	unsigned char *pcap_bytes;
	size_t pcap_bytes_size;
} boar_db_packet_record;

typedef struct boar_db_packet_list
{
	boar_db_packet_record *records;
	int count;
} boar_db_packet_list;

boar_db *boar_db_create(const char *database_path);
void boar_db_destroy(boar_db *database);
int boar_db_open(boar_db *database);
int boar_db_initialize_schema(boar_db *database);
int boar_db_load_packets(boar_db *database, boar_db_packet_list *packet_list);
void boar_db_packet_list_free(boar_db_packet_list *packet_list);
int boar_db_upsert_packet(boar_db *database, const boar_db_packet_record *packet);
int boar_db_remove_packet(boar_db *database, const char *packet_id);
const char *boar_db_path(const boar_db *database);
const char *boar_db_last_error(const boar_db *database);

#ifdef __cplusplus
}
#endif
