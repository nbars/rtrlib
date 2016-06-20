#include <rtrlib/rtr/rtr.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "pdu.h"

//enum pdu_type {
//    SERIAL_NOTIFY = 0,
//    SERIAL_QUERY = 1,
//    RESET_QUERY = 2,
//    CACHE_RESPONSE = 3,
//    IPV4_PREFIX = 4,
//    RESERVED = 5,
//    IPV6_PREFIX = 6,
//    EOD = 7,
//    CACHE_RESET = 8,
//    ROUTER_KEY = 9,
//    ERROR = 10
//};

static inline enum pdu_type rtr_tfw_get_pdu_type(const void *pdu)
{
	return *((char *) pdu + 1);
}

uint8_t _rtr_tfw_pdu_get_version(void *pdu)
{
	struct pdu_header *header = pdu;
	return header->ver;
}

uint8_t _rtr_tfw_pdu_get_type(void *pdu)
{
	struct pdu_header *header = pdu;
	return header->type;
}

uint32_t _rtr_tfw_pdu_get_length(void *pdu)
{
	struct pdu_header *header = pdu;
	return header->len;
}

//TODO: Add debug ouput on error

/*
 * Check if the PDU is big enough for the PDU type it
 * pretend to be.
 * @param pdu A pointer to a PDU that is at least pdu->len byte large. In addition
 * the PDU header must be in host-byte-order.
 * @return False if the check fails, else true
 */
bool rtr_pdu_check_size (const struct pdu_header *pdu, debug_trace_t *debug_trace)
{
	const enum pdu_type type = rtr_tfw_get_pdu_type(pdu);
	const struct pdu_error * err_pdu = NULL;
	bool retval = false;
	uint64_t min_size = 0;

	switch (type) {
	case SERIAL_NOTIFY:
		if (sizeof(struct pdu_serial_notify) == pdu->len)
			retval = true;
	break;
	case RESET_QUERY:
		if (sizeof(struct pdu_reset_query) == pdu->len)
			retval = true;
	break;
	case CACHE_RESPONSE:
		if (sizeof(struct pdu_cache_response) == pdu->len)
			retval = true;
	break;
	case IPV4_PREFIX:
		if (sizeof(struct pdu_ipv4) == pdu->len)
			retval = true;
	break;
	case IPV6_PREFIX:
		if (sizeof(struct pdu_ipv6) == pdu->len)
			retval = true;
	break;
	case EOD:
		if ((pdu->ver == RTR_PROTOCOL_VERSION_0
		     && (sizeof(struct pdu_end_of_data_v0) == pdu->len))
				||
				(pdu->ver == RTR_PROTOCOL_VERSION_1
				 && (sizeof(struct pdu_end_of_data_v1) == pdu->len)
				 )){
			retval = true;
		}
	break;
	case CACHE_RESET:
		if (sizeof(struct pdu_header) == pdu->len)
			retval = true;
	break;
	case ROUTER_KEY:
		if (sizeof(struct pdu_router_key) == pdu->len)
			retval = true;
	break;
	case ERROR:
		err_pdu = (const struct pdu_error *) pdu;
		// +4 because of the "Length of Error Text" field
		min_size = 4 + sizeof(struct pdu_error);
		if (err_pdu->len < min_size)
			break;

		//Check if the PDU really contains the error PDU
		uint32_t enc_pdu_len = ntohl(err_pdu->len_enc_pdu);
		RTR_DBG("enc_pdu_len: %u", enc_pdu_len);
		min_size += enc_pdu_len;
		if (err_pdu->len < min_size)
			break;

		//Check if the the PDU really contains the error msg
		uint32_t err_msg_len = ntohl(*((uint32_t *)(err_pdu->rest + enc_pdu_len)));
		RTR_DBG("err_msg_len: %u", err_msg_len);
		min_size += err_msg_len;
		if (err_pdu->len != min_size)
			break;

		if (err_msg_len > 0 && ((uint8_t*)err_pdu)[min_size-1] != '\0')
			break;

		retval = true;
	break;
	}

	return retval;
}

//TODO: Check flags field?
void rtr_tfw_pdu_header_to_host_byte_order(void *pdu)
{
	struct pdu_header *header = pdu;

	//The ROUTER_KEY PDU has two 1 Byte fields instead of the 2 Byte reserved field.
	if (header->type != ROUTER_KEY) {
		uint16_t reserved_tmp =  ntohs(header->reserved);
		header->reserved = reserved_tmp;
	}

	header->len = ntohl(header->len);
}

//TODO: Add other types
void rtr_tfw_pdu_footer_to_host_byte_order(struct rtr_tfw_context *ctx, void *pdu)
{
	struct pdu_serial_query *sq;
	struct pdu_header *header = pdu;
	struct pdu_error *err_pdu;

	const enum pdu_type type = header->type;
	switch (type) {
	case ERROR:
		err_pdu = pdu;
		err_pdu->len_enc_pdu =
				ntohl(err_pdu->len_enc_pdu);
		//The length of the error message
		*((uint32_t *)(err_pdu->rest + err_pdu->len_enc_pdu)) =
				ntohl(*((uint32_t *)(err_pdu->rest + err_pdu->len_enc_pdu)));
	break;
	case SERIAL_QUERY:
		sq = pdu;
		sq->sn = ntohl(sq->sn);
		//TODO: This is already done in rtr_tfw_pdu_header_to_host_byte_order
		//sq->session_id = ntohs(sq->session_id);
	break;
	case RESET_QUERY:
	break;
	default:

		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Unkowen PDU type (%u)!"
						      "Malformed PDU received!",
						      header->type);
	break;
	}
}

static void ipv6_addr_to_network_byte_order(const uint32_t *src, uint32_t *dest)
{
	for(int i = 0; i < 4; i++)
		dest[i] = ntohl(src[i]);
}

void rtr_pdu_to_network_byte_order(struct rtr_tfw_context *ctx, void *pdu)
{
	struct pdu_header *header = pdu;
	struct pdu_error *err_pdu;

	//The ROUTER_KEY PDU has two 1 Byte fields instead of the 2 Byte reserved field.
	if (header->type != ROUTER_KEY) {
		header->reserved = htons(header->reserved);
	}

	header->len = htonl(header->len);

	const enum pdu_type type = rtr_tfw_get_pdu_type(pdu);
	switch (type) {
	case SERIAL_NOTIFY:
		((struct pdu_serial_notify*)pdu)->sn = htonl(((struct pdu_serial_notify*)pdu)->sn);
	break;
	case IPV4_PREFIX:
		((struct pdu_ipv4*)pdu)->prefix = htonl(((struct pdu_ipv4*)pdu)->prefix);
		((struct pdu_ipv4*)pdu)->asn = htonl(((struct pdu_ipv4*)pdu)->asn);
	break;
	case IPV6_PREFIX:
		((struct pdu_ipv6*)pdu)->asn = htonl(((struct pdu_ipv6*)pdu)->asn);
		ipv6_addr_to_network_byte_order(((struct pdu_ipv6*)pdu)->prefix, ((struct pdu_ipv6*)pdu)->prefix);
	break;
	case EOD:
		if (header->ver == RTR_PROTOCOL_VERSION_1) {
			((struct pdu_end_of_data_v1 *) pdu)->expire_interval =
					htonl(((struct pdu_end_of_data_v1 *) pdu)->expire_interval);

			((struct pdu_end_of_data_v1 *) pdu)->refresh_interval =
					htonl(((struct pdu_end_of_data_v1 *) pdu)->refresh_interval);

			((struct pdu_end_of_data_v1 *) pdu)->retry_interval =
					htonl(((struct pdu_end_of_data_v1 *) pdu)->retry_interval);

			((struct pdu_end_of_data_v1 *) pdu)->sn =
					htonl(((struct pdu_end_of_data_v1 *) pdu)->sn);
		} else {
			((struct pdu_end_of_data_v0 *) pdu)->sn =
					htonl(((struct pdu_end_of_data_v0 *) pdu)->sn);
		}
	break;
	case ROUTER_KEY:
		((struct pdu_router_key*)pdu)->asn = htonl(((struct pdu_router_key*)pdu)->asn);
	break;
	case ERROR:
		err_pdu = pdu;
		err_pdu->len_enc_pdu =
				htonl(err_pdu->len_enc_pdu);
		//The length of the error message
		*((uint32_t *)(err_pdu->rest + err_pdu->len_enc_pdu)) =
				htonl(*((uint32_t *)(err_pdu->rest + err_pdu->len_enc_pdu)));
	break;
	case CACHE_RESPONSE:
	case CACHE_RESET:
	break;
	default:
		_rtr_tfw_debug_assert_add_debug_trace(ctx, false,
						      "Unkowen PDU type (%u)!"
						      "Please call this function only with well formed PDUs!",
						      header->type);
	break;
	}
}
