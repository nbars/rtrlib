#include "rtr_tfw/rtr_test_framework.h"
#include "rtrlib/rtrlib.h"
#include "rtrlib/rtr/rtr.h"
#include "rtrlib/rtr/packets.h"
#include <string.h>


struct tr_socket tr_tcp;
struct tr_tcp_config config = {
	"127.0.0.1",
	"7019",
};
struct rtr_socket rtr;
struct rtr_socket *rtr_socket_p;
struct rtr_mgr_group groups[1];

struct rtr_mgr_config *conf;
struct rtr_tfw_context *ctx;

void *pdu;

//TODO: Fix wrong usage of sizeof() (sizeof(x) -> (sizeof(*x)))

int test_setup()
{
	//Setup the rtr_client
	tr_tcp_init(&config, &tr_tcp);
	rtr.tr_socket = &tr_tcp;
	groups[0].sockets_len = 1;
	groups[0].sockets = &rtr_socket_p;
	groups[0].sockets[0] = &rtr;
	groups[0].preference = 1;

	//Start a cache server on port 7019 and assign it to the context ctx
	rtr_tfw_context_init(ctx, 7019);

	//Register the rtr_client with the context.
	//A rtr_client can be registerd with any number of contexts.
	rtr_tfw_rtrclient_register_with_cache(ctx, groups, 1, conf, 1, 10, 5);

	//This function must be called bevor any send/receive functions
	//will be used!
	rtr_tfw_context_wait_for_rtrclient(ctx, 100);

	return RTR_TFW_SUCCESS;
}

int test_tear_down()
{
	tr_free(&tr_tcp);
}

//Send a cache response where the len field is one byte to large
static int test_cache_response_01()
{
	if (test_setup() != RTR_TFW_SUCCESS)
		return RTR_TFW_ERROR;

	rtr_tfw_pdu_rassert(ctx, RESET_QUERY, RTR_PROTOCOL_VERSION_1, 100);

	//Send a cache response PDU with size + 1
	struct pdu_cache_response *resp = rtr_tfw_zmalloc(sizeof(*resp) + 1);

	resp->len = sizeof(*resp)+1;
	resp->session_id = 55;
	resp->type = CACHE_RESPONSE;
	resp->ver = RTR_PROTOCOL_VERSION_0;

	rtr_tfw_pdu_hton(ctx, resp);
	rtr_tfw_pdu_send(ctx, resp, sizeof(*resp) + 1, 100);
	free(resp);

	rtr_tfw_receive_pdu(ctx, pdu, 100);
	rtr_tfw_pdu_assert_error_code(pdu, CORRUPT_DATA);
	rtr_tfw_free_pdu(pdu);

	rtr_tfw_tear_down();
	test_tear_down();

	return RTR_TFW_SUCCESS;
}

//Send a cache response where the len field is one byte to small
static int test_cache_response_02()
{
	if (test_setup() != RTR_TFW_SUCCESS)
		return RTR_TFW_ERROR;

	rtr_tfw_pdu_rassert(ctx, RESET_QUERY, RTR_PROTOCOL_VERSION_1, 100);

	//Send a cache response PDU with size - 1
	struct pdu_cache_response *resp = rtr_tfw_zmalloc(sizeof(*resp));
	resp->len = sizeof(*resp)-1;
	resp->session_id = 55;
	resp->type = CACHE_RESPONSE;
	resp->ver = RTR_PROTOCOL_VERSION_0;

	rtr_tfw_pdu_hton(ctx, resp);
	rtr_tfw_pdu_send(ctx, resp, sizeof(*resp)-1, 100);
	free(resp);

	//Try to receive a package from the router. We do not expect any
	//package because sizeof(resp)-1 is smaller then the PDU header length
	//and this leads to a transport error (would block) error on the rtrclient side.
	rtr_tfw_try_receive_pdu(ctx, pdu, 500);
	rtr_tfw_assert(pdu == NULL, "We expect to receive no data here!");

	rtr_tfw_tear_down();
	test_tear_down();

	return RTR_TFW_SUCCESS;
}


/*
 * End of data PDU
 */
//Send a v0 end of data pdu where the len field is one byte to large"
static int test_end_of_data_v0_01()
{
	if (test_setup() != RTR_TFW_SUCCESS)
		return RTR_TFW_ERROR;

	rtr_tfw_pdu_rassert(ctx, RESET_QUERY, RTR_PROTOCOL_VERSION_1, 100);

	struct pdu_cache_response *resp = rtr_tfw_zmalloc(sizeof(*resp));
	resp->len = sizeof(*resp);
	resp->session_id = 55;
	resp->type = CACHE_RESPONSE;
	resp->ver = RTR_PROTOCOL_VERSION_0;

	rtr_tfw_pdu_hton(ctx, resp);
	rtr_tfw_pdu_send(ctx, resp, sizeof(*resp), 100);
	free(resp);

	//Send an end of data pdu that is one byte to large
	struct pdu_end_of_data_v0 *eod = rtr_tfw_zmalloc(sizeof(*eod) + 1);
	eod->len = sizeof(*eod)+1;
	eod->session_id = 55;
	eod->sn = 1;
	eod->type = EOD;
	eod->ver = RTR_PROTOCOL_VERSION_0;
	rtr_tfw_pdu_hton(ctx, eod);
	rtr_tfw_pdu_send(ctx, eod, sizeof(*eod) + 1, 100);
	free(eod);

	//We expect an error PDU
	rtr_tfw_receive_pdu(ctx, pdu, 100);
	rtr_tfw_pdu_assert_error_code(pdu, CORRUPT_DATA);
	rtr_tfw_free_pdu(pdu);

	rtr_tfw_tear_down();
	test_tear_down();

	return RTR_TFW_SUCCESS;
}

//Send a v0 end of data pdu where the len field is one byte to small
static int test_end_of_data_v0_02()
{
	if (test_setup() != RTR_TFW_SUCCESS)
		return RTR_TFW_ERROR;

	rtr_tfw_pdu_rassert(ctx, RESET_QUERY, RTR_PROTOCOL_VERSION_1, 100);

	struct pdu_cache_response *resp = rtr_tfw_zmalloc(sizeof(*resp));
	resp->len = sizeof(*resp);
	resp->session_id = 55;
	resp->type = CACHE_RESPONSE;
	resp->ver = RTR_PROTOCOL_VERSION_0;

	rtr_tfw_pdu_hton(ctx, resp);
	rtr_tfw_pdu_send(ctx, resp, sizeof(*resp), 100);
	free(resp);

	//Send an end of data pdu that is one byte to small
	struct pdu_end_of_data_v0 *eod = rtr_tfw_zmalloc(sizeof(*eod));
	eod->len = sizeof(*eod)-1;
	eod->session_id = 55;
	eod->sn = 1;
	eod->type = EOD;
	eod->ver = RTR_PROTOCOL_VERSION_0;
	rtr_tfw_pdu_hton(ctx, eod);
	rtr_tfw_pdu_send(ctx, eod, sizeof(*eod) - 1, 100);
	free(eod);

	//We expect an error PDU
	rtr_tfw_receive_pdu(ctx, pdu, 100);
	rtr_tfw_pdu_assert_error_code(pdu, CORRUPT_DATA);
	rtr_tfw_free_pdu(pdu);

	rtr_tfw_tear_down();
	test_tear_down();

	return RTR_TFW_SUCCESS;
}

//Send a v1 end of data pdu where the len field is one byte to large
static int test_end_of_data_v1_01()
{
	if (test_setup() != RTR_TFW_SUCCESS)
		return RTR_TFW_ERROR;

	rtr_tfw_pdu_rassert(ctx, RESET_QUERY, RTR_PROTOCOL_VERSION_1, 100);

	struct pdu_cache_response *resp = rtr_tfw_zmalloc(sizeof(*resp));
	resp->len = sizeof(*resp);
	resp->session_id = 55;
	resp->type = CACHE_RESPONSE;
	resp->ver = RTR_PROTOCOL_VERSION_1;

	rtr_tfw_pdu_hton(ctx, resp);
	rtr_tfw_pdu_send(ctx, resp, sizeof(*resp), 100);
	free(resp);

	//Send an end of data pdu that is one byte to large
	struct pdu_end_of_data_v1 *eod = rtr_tfw_zmalloc(sizeof(*eod)+1);
	eod->len = sizeof(*eod)+1;
	eod->session_id = 55;
	eod->sn = 1;
	eod->type = EOD;
	eod->ver = RTR_PROTOCOL_VERSION_1;
	eod->expire_interval = 600;
	eod->refresh_interval = 600;
	eod->retry_interval = 600;
	rtr_tfw_pdu_hton(ctx, eod);
	rtr_tfw_pdu_send(ctx, eod, sizeof(*eod) + 1, 100);
	free(eod);

	//We expect an error PDU
	rtr_tfw_receive_pdu(ctx, pdu, 100);
	rtr_tfw_pdu_assert_error_code(pdu, CORRUPT_DATA);
	rtr_tfw_free_pdu(pdu);

	rtr_tfw_tear_down();
	test_tear_down();

	return RTR_TFW_SUCCESS;
}

//Send a v1 end of data pdu where the len field is one byte to small
static int test_end_of_data_v1_02()
{
	if (test_setup() != RTR_TFW_SUCCESS)
		return RTR_TFW_ERROR;

	rtr_tfw_pdu_rassert(ctx, RESET_QUERY, RTR_PROTOCOL_VERSION_1, 100);

	struct pdu_cache_response *resp = rtr_tfw_zmalloc(sizeof(*resp));
	resp->len = sizeof(*resp);
	resp->session_id = 55;
	resp->type = CACHE_RESPONSE;
	resp->ver = RTR_PROTOCOL_VERSION_1;

	rtr_tfw_pdu_hton(ctx, resp);
	rtr_tfw_pdu_send(ctx, resp, sizeof(*resp), 100);
	free(resp);

	//Send an end of data pdu that is one byte to large
	struct pdu_end_of_data_v1 *eod = rtr_tfw_zmalloc(sizeof(*eod));
	eod->len = sizeof(*eod)-1;
	eod->session_id = 55;
	eod->sn = 1;
	eod->type = EOD;
	eod->ver = RTR_PROTOCOL_VERSION_1;
	eod->expire_interval = 600;
	eod->refresh_interval = 600;
	eod->retry_interval = 600;
	rtr_tfw_pdu_hton(ctx, eod);
	rtr_tfw_pdu_send(ctx, eod, sizeof(*eod) - 1, 100);
	free(eod);

	//We expect an error PDU
	rtr_tfw_receive_pdu(ctx, pdu, 100);
	rtr_tfw_pdu_assert_error_code(pdu, CORRUPT_DATA);
	rtr_tfw_free_pdu(pdu);

	rtr_tfw_tear_down();
	test_tear_down();

	return RTR_TFW_SUCCESS;
}

//TODO: We need some way to find out if the client detects the malformed error pdu...

//Send an error pdu where the len field is one byte to large
static int test_error_pdu_01()
{
	if (test_setup() != RTR_TFW_SUCCESS)
		return RTR_TFW_ERROR;

	rtr_tfw_pdu_rassert(ctx, RESET_QUERY, RTR_PROTOCOL_VERSION_1, 100);

	struct pdu_error *err = rtr_tfw_zmalloc(sizeof(*err) + 4 + 1);
	*(err->rest) = 0; //Length of err msg
	err->type = ERROR;
	err->ver = RTR_PROTOCOL_VERSION_1;
	err->error_code = INTERNAL_ERROR;
	err->len_enc_pdu = 0; //No PDU attached
	err->len = sizeof(*err) + 4 + 1; //+4 because of the err txt len field
	rtr_tfw_pdu_hton(ctx, err);
	rtr_tfw_pdu_send(ctx, err, sizeof(*err) + 4 + 1, 100);
	free(err);

	rtr_tfw_try_receive_pdu(ctx, pdu, 500);
	rtr_tfw_assert(pdu == NULL,
		       "We expect that the client drops the connection after receiving a erroneous error report PDU !");

	rtr_tfw_tear_down();
	test_tear_down();

	return RTR_TFW_SUCCESS;
}

//Send an error pdu where the len field is one byte to small
static int test_error_pdu_02()
{
	if (test_setup() != RTR_TFW_SUCCESS)
		return RTR_TFW_ERROR;

	rtr_tfw_pdu_rassert(ctx, RESET_QUERY, RTR_PROTOCOL_VERSION_1, 100);

	struct pdu_error *err = rtr_tfw_zmalloc(sizeof(*err) + 4);
	*(err->rest) = 0; //Length of err msg
	err->type = ERROR;
	err->ver = RTR_PROTOCOL_VERSION_1;
	err->error_code = INTERNAL_ERROR;
	err->len_enc_pdu = 0; //No PDU attached
	err->len = sizeof(*err) + 4 - 1; //+4 because of the err txt len field
	rtr_tfw_pdu_hton(ctx, err);
	rtr_tfw_pdu_send(ctx, err, sizeof(*err) + 4 - 1, 100);
	free(err);

	rtr_tfw_try_receive_pdu(ctx, pdu, 500);
	rtr_tfw_assert(pdu == NULL,
		       "We expect that the client drops the connection \
		       after receiving a erroneous error report PDU !");

	rtr_tfw_tear_down();
	test_tear_down();

	return RTR_TFW_SUCCESS;
}


int main(int argc, char *argv[])
{
	int ret;

	ret = test_cache_response_01();
	rtr_tfw_assert(ret == RTR_TFW_SUCCESS, "Error!");

	ret = test_cache_response_02();
	rtr_tfw_assert(ret == RTR_TFW_SUCCESS, "Error!");

	ret = test_end_of_data_v0_01();
	rtr_tfw_assert(ret == RTR_TFW_SUCCESS, "Error!");

	ret = test_end_of_data_v0_02();
	rtr_tfw_assert(ret == RTR_TFW_SUCCESS, "Error!");

	ret = test_end_of_data_v1_01();
	rtr_tfw_assert(ret == RTR_TFW_SUCCESS, "Error!");

	ret = test_end_of_data_v1_02();
	rtr_tfw_assert(ret == RTR_TFW_SUCCESS, "Error!");

	ret = test_error_pdu_01();
	rtr_tfw_assert(ret == RTR_TFW_SUCCESS, "Error!");

	ret = test_error_pdu_02();
	rtr_tfw_assert(ret == RTR_TFW_SUCCESS, "Error!");

	return 0;
}
