/**
 * The MIT License (MIT).
 *
 * Hopscotch - A generic concurrent skip list library.
 * https://github.com/jonathanmarvens/hopscotch
 *
 * Copyright (c) 2014 Jonathan Barronville (jonathan@scrapum.photos) and contributors.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __JONATHANMARVENS_HOPSCOTCH_HOPSCOTCH_H__INCLUDED__
#define __JONATHANMARVENS_HOPSCOTCH_HOPSCOTCH_H__INCLUDED__

/**
 * The following are good resources for understanding how Hopscotch works and why I created it.
 * All of them helped me.
 * ----- ----- ----- ----- -----
 * Main:
 *   https://web.archive.org/web/20140722235349/http://www.cl.cam.ac.uk/teaching/0506/Algorithms/skiplists.pdf
 *   https://web.archive.org/web/20140325194058/http://igoro.com/archive/skip-lists-are-fascinating
 *   https://web.archive.org/web/20130922055201/http://www.cs.tau.ac.il/~shanir/nir-pubs-web/Papers/OPODIS2006-BA.pdf
 * ----- ----- ----- ----- -----
 * Others:
 *   https://web.archive.org/web/20130718023424/http://drum.lib.umd.edu/bitstream/1903/542/2/CS-TR-2222.pdf
 *   https://web.archive.org/web/20140723023116/http://rsea.rs/skiplist.html
 *   https://web.archive.org/web/20140723023207/https://stasis.googlecode.com/svn/trunk/stasis/util/concurrentSkipList.h
 *   https://web.archive.org/web/20140723023236/https://code.google.com/p/stasis/source/browse/trunk/stasis/util/hazard.h
 *   https://web.archive.org/web/20140412013333/http://diyhpl.us/~bryan/papers2/distributed/distributed-systems/lock-free-linked-lists-and-skip-lists.2004.pdf
 *   https://web.archive.org/web/20140705203948/http://docs.oracle.com/javase/7/docs/api/java/util/concurrent/ConcurrentSkipListSet.html
 *   https://web.archive.org/web/20140705203948/http://docs.oracle.com/javase/7/docs/api/java/util/concurrent/ConcurrentSkipListMap.html
 *   https://web.archive.org/web/20131103134210/http://www.research.ibm.com/people/m/michael/spaa-2002.pdf
 */

#ifdef __cplusplus
#include <cinttypes>
#include <cstdbool>
#include <cstddef>
#include <cstdlib>
#include <cstring>

using namespace std;
#else
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <gc/gc.h>
#include <pthread.h>

#include "mt19937ar/mt19937ar.h"
#include "timestamp/timestamp.h"

#if defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
#define _ALWAYS_INLINE __attribute__ ((always_inline))
#else
#define _ALWAYS_INLINE
#endif

#if defined(__GNUC__) && ((__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 7)))
#define _UNUSED_VAR __attribute__ ((unused))
#else
#define _UNUSED_VAR
#endif

#define __MALLOC GC_malloc

#ifdef __cplusplus
#define _MALLOC(func, type, count) ((type *) func((size_t) (sizeof(type) * count)))
#else
#define _MALLOC(func, type, count) (func((size_t) (sizeof(type) * count)))
#endif

#if defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 3)))
#define HOPSCOTCH_ABI_EXPORT __attribute__ ((visibility ("default")))
#define HOPSCOTCH_ABI_HIDDEN __attribute__ ((visibility ("hidden")))
#else
#define HOPSCOTCH_ABI_EXPORT
#define HOPSCOTCH_ABI_HIDDEN
#endif

// The default list val extrema.
#define HOPSCOTCH_VAL_LIST_DEFAULT_MIN_VAL "<<<-INFINITY>>>"
#define HOPSCOTCH_VAL_LIST_DEFAULT_MAX_VAL "<<<+INFINITY>>>"

// Other defaults.
#define HOPSCOTCH_VAL_LIST_DEFAULT_MAX_LEVEL 16
#define HOPSCOTCH_VAL_LIST_DEFAULT_RAND_LEVEL_P 0.5

typedef unsigned char hopscotch_byte;

// Almost every Hopscotch function returns this type. `0` always represents success.
typedef enum {
	HOPSCOTCH_RES__SUCCESS = 0,
	HOPSCOTCH_RES_LIST__FIND_EL_VAL_NOT_FOUND,
	HOPSCOTCH_RES_LIST_NEW_INVALID_LIST_PTR,
	HOPSCOTCH_RES_LIST_NEW_INVALID_OPTS_PTR,
	HOPSCOTCH_RES_MEM_ALLOC_FAIL,
	HOPSCOTCH_RES_PTHREAD_MUTEX_INIT_FAIL,
	HOPSCOTCH_RES_PTHREAD_MUTEX_LOCK_FAIL,
	HOPSCOTCH_RES_PTHREAD_MUTEX_UNLOCK_FAIL,
} hopscotch_res_t;

// C-string values that represent results of type `hopscotch_res_t`.
#define HOPSCOTCH_RES__SUCCESS_VAL "Success!"
#define HOPSCOTCH_RES_LIST__FIND_EL_VAL_NOT_FOUND_VAL "`val` wasn't found!"
#define HOPSCOTCH_RES_LIST_NEW_INVALID_LIST_PTR_VAL "The `list` pointer provided isn't `NULL`!"
#define HOPSCOTCH_RES_LIST_NEW_INVALID_OPTS_PTR_VAL "The `opts` pointer provided is `NULL`!"
#define HOPSCOTCH_RES_MEM_ALLOC_FAIL_VAL "Memory allocation failed!"
#define HOPSCOTCH_RES_PTHREAD_MUTEX_INIT_FAIL_VAL "`pthread_mutex_init` failed!"
#define HOPSCOTCH_RES_PTHREAD_MUTEX_LOCK_FAIL_VAL "`pthread_mutex_lock` failed!"
#define HOPSCOTCH_RES_PTHREAD_MUTEX_UNLOCK_FAIL_VAL "`pthread_mutex_unlock` failed!"

#define HOPSCOTCH_RES_VAL(res_code) res_code##_VAL

typedef struct _hopscotch_list hopscotch_list_t;
typedef struct _hopscotch_node hopscotch_node_t;
typedef struct _hopscotch_opts hopscotch_opts_t;

struct _hopscotch_list {
	hopscotch_node_t * head;
	hopscotch_opts_t * opts;
};

struct _hopscotch_node {
	hopscotch_node_t ** forward;
	bool fully_linked;
	uint8_t level;
	pthread_mutex_t lock;
	bool marked;
	struct {
		hopscotch_byte * data;
		size_t size;
	} val;
};

struct _hopscotch_opts {
	hopscotch_res_t (* cmp)(
		int *,
		hopscotch_byte *,
		size_t,
		hopscotch_byte *,
		size_t
	);
	struct {
		void * (* malloc)(size_t);
	} gc;
	uint8_t max_level;
	double rand_level_p;
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate a new Hopscotch list.
 * \param list A pointer to where the Hopscotch list pointer should be stored.
 * \param opts A pointer to a `hopscotch_opts_t` containing initialization options.
 * \return `hopscotch_res_t` is `0` on success and otherwise on failure.
 */
HOPSCOTCH_ABI_EXPORT hopscotch_res_t
hopscotch_list_new(hopscotch_list_t ** list, hopscotch_opts_t * opts);

/**
 * Adds an element to a Hopscotch list.
 * \param added A pointer to a boolean variable, which will be set to true if `val` is added to `list` and false if `val` is already in `list`.
 * \param list The Hopscotch list the to add the element to.
 * \param val The element.
 * \param val_size The element's size.
 * \return `hopscotch_res_t` is `0` on success and otherwise on failure.
 */
HOPSCOTCH_ABI_EXPORT hopscotch_res_t
hopscotch_list_add_el(
	bool * added,
	hopscotch_list_t * list,
	hopscotch_byte * val,
	size_t val_size
);

/**
 * Searches a Hopscotch list for an element.
 * \param found A pointer to a boolean variable, which will be set to true if `val` is in `list`.
 * \param list The Hopscotch list to search in.
 * \param val The element to search for.
 * \param val_size The element's size.
 * \return `hopscotch_res_t` is `0` on success and otherwise on failure.
 */
HOPSCOTCH_ABI_EXPORT hopscotch_res_t
hopscotch_list_contains_el(
	bool * found,
	hopscotch_list_t * list,
	hopscotch_byte * val,
	size_t val_size
);

/**
 * Delete an element from a Hopscotch list.
 * \param deleted A pointer to a boolean variable, which will be set to true if `val` was successfully deleted and false otherwise.
 * \param list The Hopscotch list to delete the element from.
 * \param val The element.
 * \param val_size The element's size.
 * \return `hopscotch_res_t` is `0` on success and otherwise on failure.
 */
HOPSCOTCH_ABI_EXPORT hopscotch_res_t
hopscotch_list_del_el(
	bool * deleted,
	hopscotch_list_t * list,
	hopscotch_byte * val,
	size_t val_size
);

/**
 * Free a Hopscotch list.
 * \param list The Hopscotch list to free.
 * \return `hopscotch_res_t` is `0` on success and otherwise on failure.
 */
HOPSCOTCH_ABI_EXPORT hopscotch_res_t
hopscotch_list_free(hopscotch_list_t * list);

#ifdef __cplusplus
}
#endif

#endif
