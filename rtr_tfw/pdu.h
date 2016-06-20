#ifndef RTR_TFW_PDU_H
#define RTR_TFW_PDU_H

#include "debug.h"
#include "rtrlib/rtr/rtr.h"
#include <stdint.h>
#include "rtr_test_framework.h"

enum pdu_error_type {
	CORRUPT_DATA = 0,
	INTERNAL_ERROR = 1,
	NO_DATA_AVAIL = 2,
	INVALID_REQUEST = 3,
	UNSUPPORTED_PROTOCOL_VER = 4,
	UNSUPPORTED_PDU_TYPE = 5,
	WITHDRAWAL_OF_UNKNOWN_RECORD = 6,
	DUPLICATE_ANNOUNCEMENT = 7,
	UNEXPECTED_PROTOCOL_VERSION = 8,
	PDU_TOO_BIG = 32
};

enum pdu_type {
	SERIAL_NOTIFY = 0,
	SERIAL_QUERY = 1,
	RESET_QUERY = 2,
	CACHE_RESPONSE = 3,
	IPV4_PREFIX = 4,
	RESERVED = 5,
	IPV6_PREFIX = 6,
	EOD = 7,
	CACHE_RESET = 8,
	ROUTER_KEY = 9,
	ERROR = 10
};

struct pdu_header {
	uint8_t ver;
	uint8_t type;
	uint16_t reserved;
	uint32_t len;
};

struct pdu_cache_response {
	uint8_t ver;
	uint8_t type;
	uint16_t session_id;
	uint32_t len;
};

struct pdu_serial_notify {
	uint8_t ver;
	uint8_t type;
	uint16_t session_id;
	uint32_t len;
	uint32_t sn;
};

struct pdu_serial_query {
	uint8_t ver;
	uint8_t type;
	uint16_t session_id;
	uint32_t len;
	uint32_t sn;
};

struct pdu_ipv4 {
	uint8_t ver;
	uint8_t type;
	uint16_t reserved;
	uint32_t len;
	uint8_t flags;
	uint8_t prefix_len;
	uint8_t max_prefix_len;
	uint8_t zero;
	uint32_t prefix;
	uint32_t asn;
};

struct pdu_ipv6 {
	uint8_t ver;
	uint8_t type;
	uint16_t reserved;
	uint32_t len;
	uint8_t flags;
	uint8_t prefix_len;
	uint8_t max_prefix_len;
	uint8_t zero;
	uint32_t prefix[4];
	uint32_t asn;
};

struct pdu_error {
	uint8_t ver;
	uint8_t type;
	uint16_t error_code;
	uint32_t len;
	uint32_t len_enc_pdu;
	uint8_t rest[];
};

struct pdu_router_key {
	uint8_t ver;
	uint8_t type;
	uint8_t flags;
	uint8_t zero;
	uint32_t len;
	//    uint8_t ski[SKI_SIZE];
	uint32_t asn;
	//    uint8_t spki[SPKI_SIZE];
};

struct pdu_reset_query {
	uint8_t ver;
	uint8_t type;
	uint16_t flags;
	uint32_t len;
};

struct pdu_end_of_data_v0 {
	uint8_t ver;
	uint8_t type;
	uint16_t session_id;
	uint32_t len;
	uint32_t sn;
};

struct pdu_end_of_data_v1 {
	uint8_t ver;
	uint8_t type;
	uint16_t session_id;
	uint32_t len;
	uint32_t sn;
	uint32_t refresh_interval;
	uint32_t retry_interval;
	uint32_t expire_interval;
};

//TODO: Replace rtr_tfw_context with debug_trace_t (so we get rid of the dependency cycle!)

bool rtr_pdu_check_size (const struct pdu_header *pdu, debug_trace_t *debug_trace);

void rtr_tfw_pdu_header_to_host_byte_order(void *pdu);
void rtr_tfw_pdu_footer_to_host_byte_order(struct rtr_tfw_context *ctx, void *pdu);

void rtr_pdu_to_network_byte_order(struct rtr_tfw_context *ctx, void *pdu);

//PDU functions
uint8_t _rtr_tfw_pdu_get_version(void *pdu);
uint8_t _rtr_tfw_pdu_get_type(void *pdu);
uint32_t _rtr_tfw_pdu_get_length(void *pdu);

#endif
