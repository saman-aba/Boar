#include "db.h"

#include <stdlib.h>
#include <string.h>

#include "sqlite3.h"

struct boar_db
{
	char *database_path;
	char *last_error;
	sqlite3 *handle;
};

static char *boar_db_strdup(const char *value)
{
	size_t size;
	char *copy;
	if (value == NULL) {
		value = "";
	}
	size = strlen(value);
	copy = (char *)malloc(size + 1U);
	if (copy == NULL) {
		return NULL;
	}
	memcpy(copy, value, size);
	copy[size] = '\0';
	return copy;
}

static unsigned char *boar_db_memdup(const void *value, size_t size)
{
	unsigned char *copy;
	if (value == NULL || size == 0U) {
		return NULL;
	}
	copy = (unsigned char *)malloc(size);
	if (copy == NULL) {
		return NULL;
	}
	memcpy(copy, value, size);
	return copy;
}

static void boar_db_set_error(boar_db *database, const char *message)
{
	char *copy;
	if (database == NULL) {
		return;
	}
	copy = boar_db_strdup(message);
	if (copy == NULL) {
		return;
	}
	free(database->last_error);
	database->last_error = copy;
}

static int boar_db_bind_text(sqlite3_stmt *statement, int index, const char *value)
{
	if (value == NULL || value[0] == '\0') {
		return sqlite3_bind_null(statement, index) == SQLITE_OK;
	}
	return sqlite3_bind_text(statement, index, value, -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

static int boar_db_bind_blob(sqlite3_stmt *statement, int index, const unsigned char *value, size_t size)
{
	if (value == NULL || size == 0U) {
		return sqlite3_bind_null(statement, index) == SQLITE_OK;
	}
	return sqlite3_bind_blob(statement, index, value, (int)size, SQLITE_TRANSIENT) == SQLITE_OK;
}

static int boar_db_exec(boar_db *database, const char *sql)
{
	char *error_message = NULL;
	const int result = sqlite3_exec(database->handle, sql, NULL, NULL, &error_message);
	if (result != SQLITE_OK) {
		boar_db_set_error(database, error_message == NULL ? "Unknown sqlite error" : error_message);
		sqlite3_free(error_message);
		return 0;
	}
	return 1;
}

static int boar_db_ensure_protocol_types(boar_db *database)
{
	static const char *statements[] = {
		"INSERT INTO packet_type(key, name) VALUES('diameter', 'DIAMETER') ON CONFLICT(key) DO NOTHING;",
		"INSERT INTO packet_type(key, name) VALUES('m3ua', 'M3UA') ON CONFLICT(key) DO NOTHING;",
		"INSERT INTO packet_type(key, name) VALUES('gtp', 'GTP') ON CONFLICT(key) DO NOTHING;"
	};
	size_t index;
	for (index = 0; index < sizeof(statements) / sizeof(statements[0]); ++index) {
		if (!boar_db_exec(database, statements[index])) {
			return 0;
		}
	}
	return 1;
}

static int boar_db_protocol_type_id(boar_db *database, const char *key)
{
	static const char *sql = "SELECT id FROM packet_type WHERE key = ? LIMIT 1;";
	sqlite3_stmt *statement = NULL;
	int result = -1;
	if (key == NULL || key[0] == '\0') {
		return -1;
	}
	if (sqlite3_prepare_v2(database->handle, sql, -1, &statement, NULL) != SQLITE_OK) {
		boar_db_set_error(database, sqlite3_errmsg(database->handle));
		return -1;
	}
	if (!boar_db_bind_text(statement, 1, key)) {
		boar_db_set_error(database, sqlite3_errmsg(database->handle));
		sqlite3_finalize(statement);
		return -1;
	}
	if (sqlite3_step(statement) == SQLITE_ROW) {
		result = sqlite3_column_int(statement, 0);
	}
	sqlite3_finalize(statement);
	return result;
}

static void boar_db_packet_record_free_members(boar_db_packet_record *record)
{
	if (record == NULL) {
		return;
	}
	free(record->id);
	free(record->name);
	free(record->protocol);
	free(record->summary);
	free(record->export_base_name);
	free(record->created_at);
	free(record->source_mac);
	free(record->destination_mac);
	free(record->source_ip);
	free(record->destination_ip);
	free(record->transport_protocol);
	free(record->sctp_tag);
	free(record->packet_type_key);
	free(record->payload);
	free(record->frame_bytes);
	free(record->pcap_bytes);
	memset(record, 0, sizeof(*record));
	record->source_port = -1;
	record->destination_port = -1;
}

boar_db *boar_db_create(const char *database_path)
{
	boar_db *database = (boar_db *)calloc(1U, sizeof(*database));
	if (database == NULL) {
		return NULL;
	}
	database->database_path = boar_db_strdup(database_path == NULL ? "" : database_path);
	if (database->database_path == NULL) {
		free(database);
		return NULL;
	}
	database->last_error = boar_db_strdup("");
	if (database->last_error == NULL) {
		free(database->database_path);
		free(database);
		return NULL;
	}
	return database;
}

void boar_db_destroy(boar_db *database)
{
	if (database == NULL) {
		return;
	}
	if (database->handle != NULL) {
		sqlite3_close(database->handle);
	}
	free(database->database_path);
	free(database->last_error);
	free(database);
}

int boar_db_open(boar_db *database)
{
	if (database == NULL) {
		return 0;
	}
	if (database->handle != NULL) {
		return 1;
	}
	if (sqlite3_open(database->database_path, &database->handle) != SQLITE_OK) {
		boar_db_set_error(database, database->handle == NULL ? "Failed to allocate sqlite handle" : sqlite3_errmsg(database->handle));
		if (database->handle != NULL) {
			sqlite3_close(database->handle);
			database->handle = NULL;
		}
		return 0;
	}
	if (!boar_db_exec(database, "PRAGMA foreign_keys = ON;")) {
		sqlite3_close(database->handle);
		database->handle = NULL;
		return 0;
	}
	return boar_db_initialize_schema(database);
}

int boar_db_initialize_schema(boar_db *database)
{
	static const char *statements[] = {
		"CREATE TABLE IF NOT EXISTS packet_type ("
			"id INTEGER PRIMARY KEY, "
			"key TEXT NOT NULL UNIQUE, "
			"name TEXT NOT NULL UNIQUE"
		");",

		"CREATE TABLE IF NOT EXISTS packet ("
			"id TEXT PRIMARY KEY,"
			"name TEXT NOT NULL,"
			"protocol TEXT NOT NULL,"
			"summary TEXT,"
			"export_base_name TEXT,"
			"created_at TEXT NOT NULL,"
			"source_mac TEXT,"
			"destination_mac TEXT,"
			"source_ip TEXT,"
			"destination_ip TEXT,"
			"transport_protocol TEXT,"
			"source_port INTEGER,"
			"destination_port INTEGER,"
			"sctp_tag TEXT,"
			"packet_type_id INTEGER REFERENCES packet_type(id),"
			"payload BLOB,"
			"frame_bytes BLOB,"
			"pcap_bytes BLOB"
		");",

		"CREATE TABLE IF NOT EXISTS diameter_packet ("
			"packet_id TEXT PRIMARY KEY REFERENCES packet(id) ON DELETE CASCADE, "
			"command_code INTEGER, "
			"application_id INTEGER, "
			"hop_by_hop_id INTEGER, "
			"end_to_end_id INTEGER"
		");",

		"CREATE TABLE IF NOT EXISTS m3ua_packet ("
			"packet_id TEXT PRIMARY KEY REFERENCES packet(id) ON DELETE CASCADE, "
			"message_class INTEGER, "
			"message_type INTEGER, "
			"routing_context INTEGER, "
			"traffic_mode INTEGER"
		");",

		"CREATE TABLE IF NOT EXISTS gtp_packet ("
			"packet_id TEXT PRIMARY KEY REFERENCES packet(id) ON DELETE CASCADE, "
			"message_type INTEGER, "
			"teid INTEGER, "
			"version INTEGER"
		");",

		"CREATE VIEW IF NOT EXISTS packet_dashboard_view AS "
		"SELECT p.id, p.name, p.protocol, p.summary, p.created_at, "
		"COALESCE(p.transport_protocol, '') AS transport_protocol, "
		"COALESCE(p.source_ip, '') AS source_ip, "
		"COALESCE(p.destination_ip, '') AS destination_ip, "
		"length(COALESCE(p.payload, p.frame_bytes)) AS payload_size "
		"FROM packet p ORDER BY p.created_at DESC;"
	};
	size_t index;
	if (database == NULL || database->handle == NULL) {
		return 0;
	}
	for (index = 0; index < sizeof(statements) / sizeof(statements[0]); ++index) {
		if (!boar_db_exec(database, statements[index])) {
			return 0;
		}
	}
	return boar_db_ensure_protocol_types(database);
}

int boar_db_load_packets(boar_db *database, boar_db_packet_list *packet_list)
{
	static const char *sql =
		"SELECT p.id, "
			"p.name, "
			"p.protocol, "
			"p.summary, "
			"p.export_base_name, "
			"p.created_at, "
			"p.source_mac, "
			"p.destination_mac, "
			"p.source_ip, "
			"p.destination_ip, "
			"p.transport_protocol, "
			"p.source_port, "
			"p.destination_port, "
			"p.sctp_tag, "
		"COALESCE(pt.key, ''), "
			"p.payload, "
			"p.frame_bytes, "
			"p.pcap_bytes "
		"FROM packet p LEFT JOIN packet_type pt ON pt.id = p.packet_type_id ORDER BY p.created_at DESC;";
	sqlite3_stmt *statement = NULL;
	int capacity = 0;
	if (database == NULL || packet_list == NULL || database->handle == NULL) {
		return 0;
	}
	packet_list->records = NULL;
	packet_list->count = 0;
	if (sqlite3_prepare_v2(database->handle, sql, -1, &statement, NULL) != SQLITE_OK) {
		boar_db_set_error(database, sqlite3_errmsg(database->handle));
		return 0;
	}
	while (sqlite3_step(statement) == SQLITE_ROW) {
		boar_db_packet_record *record;
		void *payload_blob;
		int payload_size;
		void *frame_blob;
		int frame_size;
		void *pcap_blob;
		int pcap_size;
		if (packet_list->count == capacity) {
			int next_capacity = capacity == 0 ? 8 : capacity * 2;
			boar_db_packet_record *records = (boar_db_packet_record *)realloc(packet_list->records, (size_t)next_capacity * sizeof(*records));
			int idx;
			if (records == NULL) {
				boar_db_set_error(database, "Failed to allocate packet list");
				sqlite3_finalize(statement);
				boar_db_packet_list_free(packet_list);
				return 0;
			}
			packet_list->records = records;
			for (idx = capacity; idx < next_capacity; ++idx) {
				memset(&packet_list->records[idx], 0, sizeof(packet_list->records[idx]));
				packet_list->records[idx].source_port = -1;
				packet_list->records[idx].destination_port = -1;
			}
			capacity = next_capacity;
		}
		record = &packet_list->records[packet_list->count];
		payload_blob = (void *)sqlite3_column_blob(statement, 15);
		payload_size = sqlite3_column_bytes(statement, 15);
		frame_blob = (void *)sqlite3_column_blob(statement, 16);
		frame_size = sqlite3_column_bytes(statement, 16);
		pcap_blob = (void *)sqlite3_column_blob(statement, 17);
		pcap_size = sqlite3_column_bytes(statement, 17);
		record->id = boar_db_strdup((const char *)sqlite3_column_text(statement, 0));
		record->name = boar_db_strdup((const char *)sqlite3_column_text(statement, 1));
		record->protocol = boar_db_strdup((const char *)sqlite3_column_text(statement, 2));
		record->summary = boar_db_strdup((const char *)sqlite3_column_text(statement, 3));
		record->export_base_name = boar_db_strdup((const char *)sqlite3_column_text(statement, 4));
		record->created_at = boar_db_strdup((const char *)sqlite3_column_text(statement, 5));
		record->source_mac = boar_db_strdup((const char *)sqlite3_column_text(statement, 6));
		record->destination_mac = boar_db_strdup((const char *)sqlite3_column_text(statement, 7));
		record->source_ip = boar_db_strdup((const char *)sqlite3_column_text(statement, 8));
		record->destination_ip = boar_db_strdup((const char *)sqlite3_column_text(statement, 9));
		record->transport_protocol = boar_db_strdup((const char *)sqlite3_column_text(statement, 10));
		record->source_port = sqlite3_column_type(statement, 11) == SQLITE_NULL ? -1 : sqlite3_column_int(statement, 11);
		record->destination_port = sqlite3_column_type(statement, 12) == SQLITE_NULL ? -1 : sqlite3_column_int(statement, 12);
		record->sctp_tag = boar_db_strdup((const char *)sqlite3_column_text(statement, 13));
		record->packet_type_key = boar_db_strdup((const char *)sqlite3_column_text(statement, 14));
		record->payload_size = payload_size > 0 ? (size_t)payload_size : 0U;
		record->payload = boar_db_memdup(payload_blob, record->payload_size);
		record->frame_bytes_size = frame_size > 0 ? (size_t)frame_size : 0U;
		record->frame_bytes = boar_db_memdup(frame_blob, record->frame_bytes_size);
		record->pcap_bytes_size = pcap_size > 0 ? (size_t)pcap_size : 0U;
		record->pcap_bytes = boar_db_memdup(pcap_blob, record->pcap_bytes_size);
		packet_list->count += 1;
	}
	sqlite3_finalize(statement);
	return 1;
}

void boar_db_packet_list_free(boar_db_packet_list *packet_list)
{
	int index;
	if (packet_list == NULL) {
		return;
	}
	for (index = 0; index < packet_list->count; ++index) {
		boar_db_packet_record_free_members(&packet_list->records[index]);
	}
	free(packet_list->records);
	packet_list->records = NULL;
	packet_list->count = 0;
}

int boar_db_upsert_packet(boar_db *database, const boar_db_packet_record *packet)
{
	static const char *sql =
		"INSERT INTO packet (id, "
			"name, "
			"protocol, "
			"summary, "
			"export_base_name, "
			"created_at, "
			"source_mac, "
			"destination_mac, "
			"source_ip, "
			"destination_ip, "
			"transport_protocol, "
			"source_port, "
			"destination_port, "
			"sctp_tag, "
			"packet_type_id, "
			"payload, "
			"frame_bytes, "
			"pcap_bytes) "
		"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
		"ON CONFLICT(id) DO UPDATE SET "
			"name=excluded.name, "
			"protocol=excluded.protocol, "
			"summary=excluded.summary, "
			"export_base_name=excluded.export_base_name, "
			"created_at=excluded.created_at, "
			"source_mac=excluded.source_mac, "
			"destination_mac=excluded.destination_mac, "
			"source_ip=excluded.source_ip, "
			"destination_ip=excluded.destination_ip, "
			"transport_protocol=excluded.transport_protocol, "
			"source_port=excluded.source_port, "
			"destination_port=excluded.destination_port, "
			"sctp_tag=excluded.sctp_tag, "
			"packet_type_id=excluded.packet_type_id, "
			"payload=excluded.payload, "
			"frame_bytes=excluded.frame_bytes, "
			"pcap_bytes=excluded.pcap_bytes;";
	sqlite3_stmt *statement = NULL;
	int packet_type_id;
	int ok;
	if (database == NULL || packet == NULL || database->handle == NULL) {
		boar_db_set_error(database, "Database is not open");
		return 0;
	}
	packet_type_id = boar_db_protocol_type_id(database, packet->packet_type_key);
	if (packet->packet_type_key != NULL && packet->packet_type_key[0] != '\0' && packet_type_id < 0) {
		return 0;
	}
	if (sqlite3_prepare_v2(database->handle, sql, -1, &statement, NULL) != SQLITE_OK) {
		boar_db_set_error(database, sqlite3_errmsg(database->handle));
		return 0;
	}
	ok = 1;
	ok = ok && boar_db_bind_text(statement, 1, packet->id);
	ok = ok && boar_db_bind_text(statement, 2, packet->name);
	ok = ok && boar_db_bind_text(statement, 3, packet->protocol);
	ok = ok && boar_db_bind_text(statement, 4, packet->summary);
	ok = ok && boar_db_bind_text(statement, 5, packet->export_base_name);
	ok = ok && boar_db_bind_text(statement, 6, packet->created_at);
	ok = ok && boar_db_bind_text(statement, 7, packet->source_mac);
	ok = ok && boar_db_bind_text(statement, 8, packet->destination_mac);
	ok = ok && boar_db_bind_text(statement, 9, packet->source_ip);
	ok = ok && boar_db_bind_text(statement, 10, packet->destination_ip);
	ok = ok && boar_db_bind_text(statement, 11, packet->transport_protocol);
	ok = ok && (packet->source_port >= 0 ?
			sqlite3_bind_int(statement, 12, packet->source_port) == SQLITE_OK :
			sqlite3_bind_null(statement, 12) == SQLITE_OK);
	ok = ok && (packet->destination_port >= 0 ?
			sqlite3_bind_int(statement, 13, packet->destination_port) == SQLITE_OK :
			sqlite3_bind_null(statement, 13) == SQLITE_OK);
	ok = ok && boar_db_bind_text(statement, 14, packet->sctp_tag);
	ok = ok && (packet_type_id >= 0 ?
			sqlite3_bind_int(statement, 15, packet_type_id) == SQLITE_OK :
			sqlite3_bind_null(statement, 15) == SQLITE_OK);
	ok = ok && boar_db_bind_blob(statement, 16, packet->payload, packet->payload_size);
	ok = ok && boar_db_bind_blob(statement, 17, packet->frame_bytes, packet->frame_bytes_size);
	ok = ok && boar_db_bind_blob(statement, 18, packet->pcap_bytes, packet->pcap_bytes_size);

	if (!ok) {
		boar_db_set_error(database, sqlite3_errmsg(database->handle));
		sqlite3_finalize(statement);
		return 0;
	}
	ok = sqlite3_step(statement) == SQLITE_DONE;
	if (!ok) {
		boar_db_set_error(database, sqlite3_errmsg(database->handle));
	}
	sqlite3_finalize(statement);
	return ok;
}

int boar_db_remove_packet(boar_db *database, const char *packet_id)
{
	static const char *sql = "DELETE FROM packet WHERE id = ?;";
	sqlite3_stmt *statement = NULL;
	int ok;
	if (database == NULL || database->handle == NULL) {
		boar_db_set_error(database, "Database is not open");
		return 0;
	}
	if (sqlite3_prepare_v2(database->handle, sql, -1, &statement, NULL) != SQLITE_OK) {
		boar_db_set_error(database, sqlite3_errmsg(database->handle));
		return 0;
	}
	ok = boar_db_bind_text(statement, 1, packet_id);
	ok = ok && sqlite3_step(statement) == SQLITE_DONE &&
			sqlite3_changes(database->handle) > 0;
	if (!ok) {
		boar_db_set_error(database, sqlite3_errmsg(database->handle));
	}
	sqlite3_finalize(statement);
	return ok;
}

const char *boar_db_path(const boar_db *database)
{
	return (database == NULL || database->database_path == NULL) ?
		"" :
		database->database_path;
}

const char *boar_db_last_error(const boar_db *database)
{
	return (database == NULL || database->last_error == NULL) ?
		"" :
		database->last_error;
}
