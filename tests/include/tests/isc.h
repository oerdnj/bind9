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

#pragma once

/*! \file */

#include <inttypes.h>
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/hash.h>
#include <isc/log.h>
#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/timer.h>
#include <isc/util.h>
#include <isc/uv.h>

extern isc_mem_t     *mctx;
extern isc_log_t     *lctx;
extern isc_loop_t    *mainloop;
extern isc_loopmgr_t *loopmgr;
extern isc_nm_t	     *netmgr;
extern int	      ncpus;
extern unsigned int   workers;

int
setup_mctx(void **state);
int
teardown_mctx(void **state);

int
setup_loopmgr(void **state);
int
teardown_loopmgr(void **state);

int
setup_netmgr(void **state);
int
teardown_netmgr(void **state);

int
setup_managers(void **state);
int
teardown_managers(void **state);

#ifndef TESTS_DIR
#define TESTS_DIR "./"
#endif

/* clang-format off */
/* Copied from cmocka */
#define ISC_TEST_ENTRY(name)				\
	{ #name, run_test_##name, NULL, NULL, NULL },
#define ISC_TEST_ENTRY_SETUP(name) \
	{ #name, run_test_##name, setup_test_##name, NULL, NULL },
#define ISC_TEST_ENTRY_TEARDOWN(name) \
	{ #name, run_test_##name, NULL, teardown_test_##name, NULL },
#define ISC_TEST_ENTRY_SETUP_TEARDOWN(name) \
	{ #name, run_test_##name, setup_test_##name, teardown_test_##name, NULL },
#define ISC_TEST_ENTRY_CUSTOM(name, setup, teardown) \
	{ #name, run_test_##name, setup, teardown, NULL },
/* clang-format on */

#define ISC_SETUP_TEST_DECLARE(name) \
	int setup_test_##name(void **state __attribute__((unused)));

#define ISC_RUN_TEST_DECLARE(name) \
	void run_test_##name(void **state __attribute__((unused)));

#define ISC_TEARDOWN_TEST_DECLARE(name) \
	int teardown_test_##name(void **state __attribute__((unused)))

#define ISC_LOOP_TEST_CUSTOM_DECLARE(name, setup, teardown)             \
	void run_test_##name(void **state __attribute__((__unused__))); \
	void loop_test_##name(void *arg __attribute__((__unused__)));

#define ISC_LOOP_TEST_DECLARE(name) \
	ISC_LOOP_TEST_CUSTOM_DECLARE(name, NULL, NULL)

#define ISC_LOOP_TEST_SETUP_DECLARE(name) \
	ISC_LOOP_TEST_CUSTOM_DECLARE(name, setup_loop_##name, NULL)

#define ISC_LOOP_TEST_SETUP_TEARDOWN_DECLARE(name)            \
	ISC_LOOP_TEST_CUSTOM_DECLARE(name, setup_loop_##name, \
				     teardown_loop_##name)

#define ISC_LOOP_TEST_TEARDOWN_DECLARE(name) \
	ISC_LOOP_TEST_CUSTOM_DECLARE(name, NULL, teardown_loop_##name)

#define ISC_LOOP_SETUP_DECLARE(name) \
	void setup_loop_##name(void *arg __attribute__((__unused__)));

#define ISC_SETUP_TEST_IMPL(name)                                    \
	int setup_test_##name(void **state __attribute__((unused))); \
	int setup_test_##name(void **state __attribute__((unused)))

#define ISC_RUN_TEST_IMPL(name)                                     \
	void run_test_##name(void **state __attribute__((unused))); \
	void run_test_##name(void **state __attribute__((unused)))

#define ISC_TEARDOWN_TEST_IMPL(name)                                    \
	int teardown_test_##name(void **state __attribute__((unused))); \
	int teardown_test_##name(void **state __attribute__((unused)))

#define ISC_TEST_LIST_START const struct CMUnitTest tests[] = {
#define ISC_TEST_LIST_END \
	}                 \
	;

#define ISC_LOOP_TEST_CUSTOM_IMPL(name, setup, teardown)                 \
	void run_test_##name(void **state __attribute__((__unused__)));  \
	void loop_test_##name(void *arg __attribute__((__unused__)));    \
	void run_test_##name(void **state __attribute__((__unused__))) { \
		isc_job_cb setup_loop = setup;                           \
		isc_job_cb teardown_loop = teardown;                     \
		if (setup_loop != NULL) {                                \
			setup_loop(state);                               \
		}                                                        \
		isc_loop_setup(mainloop, loop_test_##name, state);       \
		isc_loopmgr_run(loopmgr);                                \
		if (teardown_loop != NULL) {                             \
			teardown_loop(state);                            \
		}                                                        \
	}                                                                \
	void loop_test_##name(void *arg __attribute__((__unused__)))

#define ISC_LOOP_TEST_IMPL(name) ISC_LOOP_TEST_CUSTOM_IMPL(name, NULL, NULL)

#define ISC_LOOP_TEST_SETUP_IMPL(name) \
	ISC_LOOP_TEST_CUSTOM_IMPL(name, setup_loop_##name, NULL)

#define ISC_LOOP_TEST_SETUP_TEARDOWN_IMPL(name) \
	ISC_LOOP_TEST_CUSTOM_IMPL(name, setup_loop_##name, teardown_loop_##name)

#define ISC_LOOP_TEST_TEARDOWN_IMPL(name) \
	ISC_LOOP_TEST_CUSTOM_IMPL(name, NULL, teardown_loop_##name)

#define ISC_LOOP_SETUP_IMPL(name)                                      \
	void setup_loop_##name(void *arg __attribute__((__unused__))); \
	void setup_loop_##name(void *arg __attribute__((__unused__)))

#define ISC_LOOP_TEARDOWN_IMPL(name)                                      \
	void teardown_loop_##name(void *arg __attribute__((__unused__))); \
	void teardown_loop_##name(void *arg __attribute__((__unused__)))

#define ISC_TEST_DECLARE(name) void run_test_##name(void **state);

#define ISC_TEST_LIST_START const struct CMUnitTest tests[] = {
#define ISC_TEST_LIST_END \
	}                 \
	;

#define ISC_TEST_MAIN ISC_TEST_MAIN_CUSTOM(NULL, NULL)

#define ISC_TEST_MAIN_CUSTOM(setup, teardown)                       \
	int main(void) {                                            \
		int r;                                              \
                                                                    \
		signal(SIGPIPE, SIG_IGN);                           \
                                                                    \
		isc_mem_debugging |= ISC_MEM_DEBUGRECORD;           \
		isc_mem_create(&mctx);                              \
                                                                    \
		r = cmocka_run_group_tests(tests, setup, teardown); \
                                                                    \
		isc_mem_destroy(&mctx);                             \
                                                                    \
		return (r);                                         \
	}
