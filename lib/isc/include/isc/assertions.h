/*
 * Copyright (c) 1997, 1998 by Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*
 * $Id: assertions.h,v 1.1 1998/10/21 01:08:12 halley Exp $
 */

#ifndef ISC_ASSERTIONS_H
#define ISC_ASSERTIONS_H	1

typedef enum {
	isc_assertiontype_require,
	isc_assertiontype_ensure,
	isc_assertiontype_insist,
	isc_assertiontype_invariant
} isc_assertiontype_t;

typedef void (*isc_assertioncallback_t)(char *, int, isc_assertiontype_t,
					char *);

extern isc_assertioncallback_t		__isc_assertion_failed;

#define isc_assertion_setcallback	__isc_assertion_setcallback
#define isc_assertion_typetotext	__isc_assertion_typetotext

void isc_assertion_setcallback(isc_assertioncallback_t);
char *isc_assertion_typetotext(isc_assertiontype_t type);

#ifdef CHECK_ALL
#define CHECK_REQUIRE		1
#define CHECK_ENSURE		1
#define CHECK_INSIST		1
#define CHECK_INVARIANT		1
#endif

#ifdef CHECK_NONE
#define CHECK_REQUIRE		0
#define CHECK_ENSURE		0
#define CHECK_INSIST		0
#define CHECK_INVARIANT		0
#endif

#ifndef CHECK_REQUIRE
#define CHECK_REQUIRE		1
#endif

#ifndef CHECK_ENSURE
#define CHECK_ENSURE		1
#endif

#ifndef CHECK_INSIST
#define CHECK_INSIST		1
#endif

#ifndef CHECK_INVARIANT
#define CHECK_INVARIANT		1
#endif

#if CHECK_REQUIRE != 0
#define REQUIRE(cond) \
	((void) ((cond) || \
		 ((__isc_assertion_failed)(__FILE__, __LINE__, \
					   isc_assertiontype_require, \
					   #cond), 0)))
#else
#define REQUIRE(cond)		((void) 0)
#endif /* CHECK_REQUIRE */

#if CHECK_ENSURE != 0
#define ENSURE(cond) \
	((void) ((cond) || \
		 ((__isc_assertion_failed)(__FILE__, __LINE__, \
					   isc_assertiontype_ensure, \
					   #cond), 0)))
#else
#define ENSURE(cond)		((void) 0)
#endif /* CHECK_ENSURE */

#if CHECK_INSIST != 0
#define INSIST(cond) \
	((void) ((cond) || \
		 ((__isc_assertion_failed)(__FILE__, __LINE__, \
					   isc_assertiontype_insist, \
					   #cond), 0)))
#else
#define INSIST(cond)		((void) 0)
#endif /* CHECK_INSIST */

#if CHECK_INVARIANT != 0
#define INVARIANT(cond) \
	((void) ((cond) || \
		 ((__isc_assertion_failed)(__FILE__, __LINE__, \
					   isc_assertiontype_invariant, \
					   #cond), 0)))
#else
#define INVARIANT(cond)		((void) 0)
#endif /* CHECK_INVARIANT */

#endif /* ISC_ASSERTIONS_H */
