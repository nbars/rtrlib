#include "rtr_tfw/rtr_test_framework.h"
#include "rtrlib/rtrlib.h"
#include "rtrlib/rtr/rtr.h"
#include <string.h>

struct tr_socket tr_tcp;
struct tr_tcp_config config = {
	"127.0.0.1",
	"7019",
};
struct rtr_socket rtr;
struct rtr_mgr_group groups[1];
void *pdu;

//Define a test case
static test01()
{

	//Test case description
	//Cache                         Router
	//  ~                             ~
	//  | <----- Reset Query -------- | (Check if we receive a version 1 reset query)
	//  |                             |
	//  | ----- Cache Response -----> | (We send a version 0 CR PDU, thus the client must downgrade)
	//  | ------- End of Data ------> | (Send a EoD PDU (version 0))
	//  | <---- Serial Query -------- | (Receive serial query and check
	//                                   session_id and serial number and protocol version)
	//  ~


	//Setup the rtr_client
	tr_tcp_init(&config, &tr_tcp);
	rtr.tr_socket = &tr_tcp;
	groups[0].sockets_len = 1;
	groups[0].sockets = malloc(1 * sizeof(struct rtr_socket));
	groups[0].sockets[0] = &rtr;
	groups[0].preference = 1;
	struct rtr_mgr_config *conf;

	struct rtr_tfw_context *ctx;

	//Start a cache server on port 7019 and assign it to the context ctx
	rtr_tfw_context_init(ctx, 7019);

	//Register the rtr_client with the context.
	//A rtr_client can be registerd with any number of contexts.
	rtr_tfw_rtrclient_register_with_cache(ctx, groups, 1, conf, 1, 10, 5);

	//This function must be called bevor any send/receive functions
	//will be used!
	rtr_tfw_context_wait_for_rtrclient(ctx, 100);

	//(r)assert: receive and assert a PDU.
	//The cache that belongs to the context ctx will wait 100ms for a
	//PDU that has the type RESET_QUERY and the version RTR_PROTOCOL_VERSION_1.
	rtr_tfw_pdu_rassert(ctx, RESET_QUERY, RTR_PROTOCOL_VERSION_1, 100);

	//Construct a response PDU that will be send by the cache
	struct pdu_cache_response resp;
	resp.len = sizeof(resp);
	resp.session_id = 55;
	resp.type = CACHE_RESPONSE;
	resp.ver = RTR_PROTOCOL_VERSION_0;
	//Convert to network-byte-order
	rtr_tfw_pdu_hton(ctx, &resp);
	//Send the PDU with a timeout of 1000ms
	rtr_tfw_pdu_send(ctx, &resp, sizeof(resp), 100);

	struct pdu_end_of_data_v0 eod;
	eod.len = sizeof(eod);
	eod.session_id = 55;
	eod.sn = 1;
	eod.type = EOD;
	eod.ver = RTR_PROTOCOL_VERSION_0;
	rtr_tfw_pdu_hton(ctx, &eod);
	rtr_tfw_pdu_send(ctx, &eod, sizeof(eod), 1000);

	//We need to wait 1500ms because the refresh interval of the rtr_client is set
	//to 1000ms (see rtr_tfw_rtrclient_register_with_cache())
	rtr_tfw_receive_pdu(ctx, pdu, 1500);
	rtr_tfw_pdu_assert_type(pdu, SERIAL_QUERY);
	rtr_tfw_pdu_assert_version(pdu, RTR_PROTOCOL_VERSION_0);
	struct pdu_serial_query *sq = pdu;
	rtr_tfw_assert(sq->session_id == 55, "Wrong session_id! (got:%u, expected:%u)", sq->session_id, 55);
	rtr_tfw_assert(sq->sn == 1, "Wrong serial number! (got:%u, expected:%u)", sq->sn, 1);
	rtr_tfw_free_pdu(pdu);

	//This function must be called, if the test is finished.
	rtr_tfw_tear_down();
	return RTR_TFW_SUCCESS;
}

int main(int argc, char *argv[])
{
	int ret;

	ret = test01();
	//Check the return value of the test function. The rtr_tfw_assert functions
	//inside test01() will return RTR_TFW_ERROR if the assertion fails.
	rtr_tfw_assert(ret == RTR_TFW_SUCCESS, "Error while running test!");

	return 0;
}

