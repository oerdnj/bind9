/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

#include <stdlib.h>
#include <string.h>

#include <isc/base64.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/loop.h>
#include <isc/managers.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/nonce.h>
#include <isc/random.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/util.h>

#include <dns/dispatch.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/request.h>
#include <dns/result.h>
#include <dns/tkey.h>
#include <dns/tsig.h>
#include <dns/view.h>

#define CHECK(str, x)                                        \
	{                                                    \
		if ((x) != ISC_R_SUCCESS) {                  \
			fprintf(stderr, "I:%s: %s\n", (str), \
				isc_result_totext(x));       \
			exit(-1);                            \
		}                                            \
	}

#define RUNCHECK(x) RUNTIME_CHECK((x) == ISC_R_SUCCESS)

#define TIMEOUT 30

static char *ip_address = NULL;
static int port = 0;

static dst_key_t *ourkey = NULL;
static isc_mem_t *mctx = NULL;
static isc_loopmgr_t *loopmgr = NULL;
static dns_tsigkey_t *tsigkey = NULL, *initialkey = NULL;
static dns_tsig_keyring_t *ring = NULL;
static unsigned char noncedata[16];
static isc_buffer_t nonce;
static dns_requestmgr_t *requestmgr = NULL;
static const char *ownername_str = ".";

static void
recvquery(void *arg) {
	dns_request_t *request = (dns_request_t *)arg;
	dns_message_t *query = dns_request_getarg(request);
	dns_message_t *response = NULL;
	isc_result_t result;
	char keyname[256];
	isc_buffer_t keynamebuf;
	int type;

	result = dns_request_getresult(request);
	if (result != ISC_R_SUCCESS) {
		fprintf(stderr, "I:request event result: %s\n",
			isc_result_totext(result));
		exit(-1);
	}

	dns_message_create(mctx, DNS_MESSAGE_INTENTPARSE, &response);

	result = dns_request_getresponse(request, response,
					 DNS_MESSAGEPARSE_PRESERVEORDER);
	CHECK("dns_request_getresponse", result);

	if (response->rcode != dns_rcode_noerror) {
		result = dns_result_fromrcode(response->rcode);
		fprintf(stderr, "I:response rcode: %s\n",
			isc_result_totext(result));
		exit(-1);
	}

	result = dns_tkey_processdhresponse(query, response, ourkey, &nonce,
					    &tsigkey, ring);
	CHECK("dns_tkey_processdhresponse", result);

	/*
	 * Yes, this is a hack.
	 */
	isc_buffer_init(&keynamebuf, keyname, sizeof(keyname));
	result = dst_key_buildfilename(tsigkey->key, 0, "", &keynamebuf);
	CHECK("dst_key_buildfilename", result);
	printf("%.*s\n", (int)isc_buffer_usedlength(&keynamebuf),
	       (char *)isc_buffer_base(&keynamebuf));
	type = DST_TYPE_PRIVATE | DST_TYPE_PUBLIC | DST_TYPE_KEY;
	result = dst_key_tofile(tsigkey->key, type, "");
	CHECK("dst_key_tofile", result);

	dns_message_detach(&query);
	dns_message_detach(&response);
	dns_request_destroy(&request);
	isc_loopmgr_shutdown(loopmgr);
}

static void
sendquery(void *arg) {
	struct in_addr inaddr;
	isc_sockaddr_t address;
	isc_region_t r;
	isc_result_t result;
	dns_fixedname_t keyname;
	dns_fixedname_t ownername;
	isc_buffer_t namestr, keybuf;
	unsigned char keydata[9];
	dns_message_t *query = NULL;
	dns_request_t *request = NULL;
	static char keystr[] = "0123456789ab";

	UNUSED(arg);

	result = ISC_R_FAILURE;
	if (inet_pton(AF_INET, ip_address, &inaddr) != 1) {
		CHECK("inet_pton", result);
	}
	isc_sockaddr_fromin(&address, &inaddr, port);

	dns_fixedname_init(&keyname);
	isc_buffer_constinit(&namestr, "tkeytest.", 9);
	isc_buffer_add(&namestr, 9);
	result = dns_name_fromtext(dns_fixedname_name(&keyname), &namestr, NULL,
				   0, NULL);
	CHECK("dns_name_fromtext", result);

	dns_fixedname_init(&ownername);
	isc_buffer_constinit(&namestr, ownername_str, strlen(ownername_str));
	isc_buffer_add(&namestr, strlen(ownername_str));
	result = dns_name_fromtext(dns_fixedname_name(&ownername), &namestr,
				   NULL, 0, NULL);
	CHECK("dns_name_fromtext", result);

	isc_buffer_init(&keybuf, keydata, 9);
	result = isc_base64_decodestring(keystr, &keybuf);
	CHECK("isc_base64_decodestring", result);

	isc_buffer_usedregion(&keybuf, &r);

	result = dns_tsigkey_create(
		dns_fixedname_name(&keyname), DNS_TSIG_HMACMD5_NAME,
		isc_buffer_base(&keybuf), isc_buffer_usedlength(&keybuf), false,
		NULL, 0, 0, mctx, ring, &initialkey);
	CHECK("dns_tsigkey_create", result);

	dns_message_create(mctx, DNS_MESSAGE_INTENTRENDER, &query);

	result = dns_tkey_builddhquery(query, ourkey,
				       dns_fixedname_name(&ownername),
				       DNS_TSIG_HMACMD5_NAME, &nonce, 3600);
	CHECK("dns_tkey_builddhquery", result);

	result = dns_request_create(requestmgr, query, NULL, &address, NULL,
				    NULL, DNS_REQUESTOPT_TCP, initialkey,
				    TIMEOUT, 0, 0, isc_loop_main(loopmgr),
				    recvquery, query, &request);
	CHECK("dns_request_create", result);
}

int
main(int argc, char *argv[]) {
	char *ourkeyname = NULL;
	isc_nm_t *netmgr = NULL;
	isc_sockaddr_t bind_any;
	dns_dispatchmgr_t *dispatchmgr = NULL;
	dns_dispatch_t *dispatchv4 = NULL;
	dns_view_t *view = NULL;
	dns_tkeyctx_t *tctx = NULL;
	isc_log_t *log = NULL;
	isc_logconfig_t *logconfig = NULL;
	isc_result_t result;
	int type;

	if (argc < 4) {
		fprintf(stderr, "I:no DH key provided\n");
		exit(-1);
	}
	ip_address = argv[1];
	port = atoi(argv[2]);
	ourkeyname = argv[3];

	if (argc >= 5) {
		ownername_str = argv[4];
	}

	isc_mem_debugging = ISC_MEM_DEBUGRECORD;

	isc_managers_create(&mctx, 1, &loopmgr, &netmgr);

	isc_log_create(mctx, &log, &logconfig);

	RUNCHECK(dst_lib_init(mctx, NULL));

	RUNCHECK(dns_dispatchmgr_create(mctx, netmgr, &dispatchmgr));

	isc_sockaddr_any(&bind_any);
	RUNCHECK(dns_dispatch_createudp(dispatchmgr, &bind_any, &dispatchv4));
	RUNCHECK(dns_requestmgr_create(mctx, dispatchmgr, dispatchv4, NULL,
				       &requestmgr));

	RUNCHECK(dns_tsigkeyring_create(mctx, &ring));
	RUNCHECK(dns_tkeyctx_create(mctx, &tctx));

	RUNCHECK(dns_view_create(mctx, 0, "_test", &view));
	dns_view_setkeyring(view, ring);
	dns_tsigkeyring_detach(&ring);

	type = DST_TYPE_PUBLIC | DST_TYPE_PRIVATE | DST_TYPE_KEY;
	result = dst_key_fromnamedfile(ourkeyname, NULL, type, mctx, &ourkey);
	CHECK("dst_key_fromnamedfile", result);

	isc_buffer_init(&nonce, noncedata, sizeof(noncedata));
	isc_nonce_buf(noncedata, sizeof(noncedata));
	isc_buffer_add(&nonce, sizeof(noncedata));

	isc_loopmgr_setup(loopmgr, sendquery, NULL);
	isc_loopmgr_run(loopmgr);

	dns_requestmgr_shutdown(requestmgr);
	dns_requestmgr_detach(&requestmgr);
	dns_dispatch_detach(&dispatchv4);
	dns_dispatchmgr_detach(&dispatchmgr);

	dst_key_free(&ourkey);
	dns_tsigkey_detach(&initialkey);
	dns_tsigkey_detach(&tsigkey);

	dns_tkeyctx_destroy(&tctx);

	dns_view_detach(&view);

	isc_log_destroy(&log);

	dst_lib_destroy();

	isc_managers_destroy(&mctx, &loopmgr, &netmgr);

	return (0);
}
