/**
 * The MIT License (MIT).
 *
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

#include "hopscotch.h"

static hopscotch_res_t
_list_can_del_el(bool *, hopscotch_node_t *, uint8_t);

static hopscotch_res_t
_list_default_el_cmp(
	int *,
	byte *,
	size_t,
	byte *,
	size_t
);

// Lock-free node finding helper.
static hopscotch_res_t
_list_find_el(
	uint8_t *,
	hopscotch_node_t **,
	hopscotch_node_t **,
	hopscotch_list_t *,
	byte *,
	size_t
);

_ALWAYS_INLINE static inline hopscotch_res_t
_list_rand_level(uint8_t *, hopscotch_list_t *);

static hopscotch_res_t
_list_can_del_el(bool * ans, hopscotch_node_t * el, uint8_t level) {
	ans[0] = (bool) (
		el->fully_linked &&
		(el->level == level) &&
		(! el->marked)
	);
	// Success!
	return HOPSCOTCH_RES__SUCCESS;
}

static hopscotch_res_t
_list_default_el_cmp(
	int * res,
	byte * val_a,
	size_t val_a_size,
	byte * val_b,
	size_t val_b_size
) {
	// Let's first handle the extrema.
	const char * min_val = (char *) HOPSCOTCH_VAL_LIST_DEFAULT_MIN_VAL;
	const char * max_val = (char *) HOPSCOTCH_VAL_LIST_DEFAULT_MAX_VAL;
	size_t min_val_size = (size_t) (strlen(min_val) + 1);
	size_t max_val_size = (size_t) (strlen(max_val) + 1);
	// If their sizes aren't equal, no need to even compare and waste precious CPU cycles.
	// Due to short-circuit eval, this should behave as expected.
	// NOTE(@jonathanmarvens):
	// When making comparisons against elements already in a list, this logic assumes `val_a` is the element already in the list.
	if (
		(((int) val_a_size) == ((int) min_val_size)) &&
		(memcmp((void *) val_a, (void *) min_val, min_val_size) == 0)
	) {
		res[0] = -1;
		// Success!
		return HOPSCOTCH_RES__SUCCESS;
	}
	if (
		(((int) val_a_size) == ((int) max_val_size)) &&
		(memcmp((void *) val_a, (void *) max_val, max_val_size) == 0)
	) {
		res[0] = 1;
		// Success!
		return HOPSCOTCH_RES__SUCCESS;
	}
	// Make sure the we only compare the buffers up to the smallest buffer's size.
	size_t cmp_size;
	if (((int) val_a_size) < ((int) val_b_size)) {
		cmp_size = val_a_size;
	} else {
		cmp_size = val_b_size;
	}
	// Compare!
	res[0] = memcmp((void *) val_a, (void *) val_b, cmp_size);
	// Success!
	return HOPSCOTCH_RES__SUCCESS;
}

static hopscotch_res_t
_list_find_el(
	uint8_t * level_found,
	hopscotch_node_t ** pred_nodes,
	hopscotch_node_t ** succ_nodes,
	hopscotch_list_t * list,
	byte * val,
	size_t val_size
) {
	bool val_found = false;
	hopscotch_node_t * pred_node = list->head;
	int16_t _level;
	for (_level = ((int16_t) list->opts->max_level) - 1; ((int) _level) >= 0; _level--) {
		hopscotch_node_t * curr_node = pred_node->forward[(int) _level];
		while (true) {
			int _cmp_res_001;
			hopscotch_res_t _tmp_001 = list->opts->cmp(
				&_cmp_res_001,
				curr_node->val.data,
				curr_node->val.size,
				val,
				val_size
			);
			if (_tmp_001 != HOPSCOTCH_RES__SUCCESS) {
				return _tmp_001;
			}
			if (_cmp_res_001 < 0) {
				pred_node = curr_node;
				curr_node = pred_node->forward[(int) _level];
			} else {
				break;
			}
		}
		int _cmp_res_002;
		hopscotch_res_t _tmp_002 = list->opts->cmp(
			&_cmp_res_002,
			curr_node->val.data,
			curr_node->val.size,
			val,
			val_size
		);
		if (_tmp_002 != HOPSCOTCH_RES__SUCCESS) {
			return _tmp_002;
		}
		if (
			(! val_found) &&
			(_cmp_res_002 == 0)
		) {
			val_found = true;
			level_found[0] = (uint8_t) _level;
		}
		pred_nodes[(int) _level] = pred_node;
		succ_nodes[(int) _level] = curr_node;
	}
	if (val_found) {
		// Success!
		return HOPSCOTCH_RES__SUCCESS;
	} else {
		return HOPSCOTCH_RES_LIST__FIND_EL_VAL_NOT_FOUND;
	}
}

_ALWAYS_INLINE static inline hopscotch_res_t
_list_rand_level(uint8_t * level, hopscotch_list_t * list) {
	int16_t _level = 0;
	while (
		(genrand_real2() < list->opts->rand_level_p) &&
		(((int) _level) <= ((int) list->opts->max_level))
	) {
		_level++;
	}
	level[0] = (uint8_t) _level;
	// Success!
	return HOPSCOTCH_RES__SUCCESS;
}

hopscotch_res_t
hopscotch_list_new(hopscotch_list_t ** list, hopscotch_opts_t * opts) {
	// The pointer `list` points to must be initialized to `NULL`!
	// This check is just done for safety reasons.
	if (list[0] != NULL) {
		return HOPSCOTCH_RES_LIST_NEW_INVALID_LIST_PTR;
	}
	// `opts` must not be `NULL`.
	if (opts == NULL) {
		return HOPSCOTCH_RES_LIST_NEW_INVALID_OPTS_PTR;
	}
	// Set the default compare function if one isn't provided.
	if (opts->cmp == NULL) {
		opts->cmp = _list_default_el_cmp;
	}
	// Set the default max level if one isn't provided.
	if (((int) opts->max_level) == 0) {
		opts->max_level = HOPSCOTCH_VAL_LIST_DEFAULT_MAX_LEVEL;
	}
	// Set the default GC if one isn't provided.
	if (opts->gc.malloc == NULL) {
		opts->gc.malloc = __MALLOC;
	}
	// Set the list's default "p" for the random level function if one isn't provided.
	if (opts->rand_level_p == ((double) 0)) {
		opts->rand_level_p = HOPSCOTCH_VAL_LIST_DEFAULT_RAND_LEVEL_P;
	}
	// Set seed for the random number generator.
	init_genrand(
		(unsigned long) timestamp()
	);
	// Allocate some memory for the left sentinel node.
	hopscotch_node_t * list_left_sentinel_node = _MALLOC(opts->gc.malloc, hopscotch_node_t, ((size_t) 1));
	if (list_left_sentinel_node == NULL) {
		return HOPSCOTCH_RES_MEM_ALLOC_FAIL;
	}
	// Initialize the left sentinel node.
	// The left sentinel node's level is, of course, equal to the max level.
	list_left_sentinel_node->level = opts->max_level - 1;
	list_left_sentinel_node->val.data = (byte *) HOPSCOTCH_VAL_LIST_DEFAULT_MIN_VAL;
	list_left_sentinel_node->val.size = (size_t) (strlen((char *) list_left_sentinel_node->val.data) + 1);
	list_left_sentinel_node->marked = false;
	int _tmp_001 = pthread_mutex_init(&(list_left_sentinel_node->lock), NULL);
	if (_tmp_001 != 0) {
		return HOPSCOTCH_RES_PTHREAD_MUTEX_INIT_FAIL;
	}
	// Allocate some memory for the right sentinel node.
	hopscotch_node_t * list_right_sentinel_node = _MALLOC(opts->gc.malloc, hopscotch_node_t, ((size_t) 1));
	if (list_right_sentinel_node == NULL) {
		return HOPSCOTCH_RES_MEM_ALLOC_FAIL;
	}
	// Initialize the right sentinel node.
	// The right sentinel node's level is, of course, also equal to the max level.
	list_right_sentinel_node->level = opts->max_level - 1;
	list_right_sentinel_node->val.data = (byte *) HOPSCOTCH_VAL_LIST_DEFAULT_MAX_VAL;
	list_right_sentinel_node->val.size = (size_t) (strlen((char *) list_right_sentinel_node->val.data) + 1);
	list_right_sentinel_node->marked = false;
	int _tmp_002 = pthread_mutex_init(&(list_right_sentinel_node->lock), NULL);
	if (_tmp_002 != 0) {
		return HOPSCOTCH_RES_PTHREAD_MUTEX_INIT_FAIL;
	}
	// Allocate some memory for the sentinel nodes' forward pointers.
	// We need space for `opts->max_level` forward pointers for both.
	list_left_sentinel_node->forward = _MALLOC(opts->gc.malloc, hopscotch_node_t *, ((size_t) opts->max_level));
	if (list_left_sentinel_node->forward == NULL) {
		return HOPSCOTCH_RES_MEM_ALLOC_FAIL;
	}
	list_right_sentinel_node->forward = _MALLOC(opts->gc.malloc, hopscotch_node_t *, ((size_t) opts->max_level));
	if (list_right_sentinel_node->forward == NULL) {
		return HOPSCOTCH_RES_MEM_ALLOC_FAIL;
	}
	int16_t _level;
	for (_level = 0; ((int) _level) < ((int) opts->max_level); _level++) {
		// All of the right sentinel node's forward pointers point to `NULL`.
		list_right_sentinel_node->forward[(int) _level] = NULL;
		// Initially, all forward pointers of the left sentinel node point to the right sentinel node.
		list_left_sentinel_node->forward[(int) _level] = list_right_sentinel_node;
	}
	// Both sentinel nodes are, initially, fully linked.
	list_left_sentinel_node->fully_linked = true;
	list_right_sentinel_node->fully_linked = true;
	// Allocate some memory for the list structure.
	hopscotch_list_t * _list = _MALLOC(opts->gc.malloc, hopscotch_list_t, ((size_t) 1));
	if (_list == NULL) {
		return HOPSCOTCH_RES_MEM_ALLOC_FAIL;
	}
	// Initialize ...
	_list->head = list_left_sentinel_node;
	_list->opts = opts;
	// Set the result.
	list[0] = _list;
	// Success!
	return HOPSCOTCH_RES__SUCCESS;
}

hopscotch_res_t
hopscotch_list_add_el(
	bool * added,
	hopscotch_list_t * list,
	byte * val,
	size_t val_size
) {
	uint8_t _top_level;
	_list_rand_level(&_top_level, list);
	int16_t top_level = (int16_t) _top_level;
	hopscotch_node_t * pred_nodes[(int) list->opts->max_level];
	hopscotch_node_t * succ_nodes[(int) list->opts->max_level];
	while (true) {
		uint8_t _level_found;
		hopscotch_res_t _tmp_001 = _list_find_el(
			&_level_found,
			pred_nodes,
			succ_nodes,
			list,
			val,
			val_size
		);
		int16_t level_found = (int16_t) _level_found;
		if (_tmp_001 != HOPSCOTCH_RES_LIST__FIND_EL_VAL_NOT_FOUND) {
			hopscotch_node_t * node_found = succ_nodes[(int) level_found];
			if (! node_found->marked) {
				while (! node_found->fully_linked);
				added[0] = false;
				// Success!
				return HOPSCOTCH_RES__SUCCESS;
			}
			continue;
		}
		int16_t highest_level_locked = -1;
		hopscotch_node_t * pred_node;
		hopscotch_node_t * succ_node;
		hopscotch_node_t * prev_pred_node = NULL;
		bool valid = true;
		int16_t _level;
		for (_level = 0; valid && (((int) _level) <= ((int) top_level)); _level++) {
			pred_node = pred_nodes[(int) _level];
			succ_node = succ_nodes[(int) _level];
			if (pred_node != prev_pred_node) {
				int _tmp_002 = pthread_mutex_lock(&(pred_node->lock));
				// TODO(@jonathanmarvens): Figure out a better way to handle this.
				if (_tmp_002 != 0) {
					return HOPSCOTCH_RES_PTHREAD_MUTEX_LOCK_FAIL;
				}
				highest_level_locked = _level;
				prev_pred_node = pred_node;
			}
			if (
				(! pred_node->marked) &&
				(! succ_node->marked) &&
				(pred_node->forward[(int) _level] == succ_node)
			) {
				valid = true;
			} else {
				valid = false;
			}
		}
		if (valid) {
			hopscotch_node_t * new_node = _MALLOC(list->opts->gc.malloc, hopscotch_node_t, ((size_t) 1));
			if (new_node == NULL) {
				return HOPSCOTCH_RES_MEM_ALLOC_FAIL;
			}
			new_node->level = (uint8_t) top_level;
			new_node->val.data = val;
			new_node->val.size = val_size;
			new_node->marked = false;
			int _tmp_003 = pthread_mutex_init(&(new_node->lock), NULL);
			if (_tmp_003 != 0) {
				return HOPSCOTCH_RES_PTHREAD_MUTEX_INIT_FAIL;
			}
			new_node->forward = _MALLOC(list->opts->gc.malloc, hopscotch_node_t *, ((size_t) (top_level + 1)));
			if (new_node->forward == NULL) {
				return HOPSCOTCH_RES_MEM_ALLOC_FAIL;
			}
			int16_t _a;
			for (_a = 0; ((int) _a) <= ((int) top_level); _a++) {
				new_node->forward[(int) _a] = succ_nodes[(int) _a];
				pred_nodes[(int) _a]->forward[(int) _a] = new_node;
			}
			new_node->fully_linked = true;
			added[0] = true;
			// Release locks!
			int16_t _b;
			for (_b = 0; ((int) _b) <= ((int) highest_level_locked); _b++) {
				int _tmp_004 = pthread_mutex_unlock(&(pred_nodes[(int) _b]->lock));
				// TODO(@jonathanmarvens): Figure out a better way to handle this.
				if (_tmp_004 != 0) {
					return HOPSCOTCH_RES_PTHREAD_MUTEX_UNLOCK_FAIL;
				}
			}
			// Success!
			return HOPSCOTCH_RES__SUCCESS;
		} else {
			// Release locks!
			int16_t _c;
			for (_c = 0; ((int) _c) <= ((int) highest_level_locked); _c++) {
				int _tmp_005 = pthread_mutex_unlock(&(pred_nodes[(int) _c]->lock));
				// TODO(@jonathanmarvens): Figure out a better way to handle this.
				if (_tmp_005 != 0) {
					return HOPSCOTCH_RES_PTHREAD_MUTEX_UNLOCK_FAIL;
				}
			}
			// Invalidate `highest_level_locked`.
			highest_level_locked = -1;
			continue;
		}
	}
}

hopscotch_res_t
hopscotch_list_contains_el(
	bool * found,
	hopscotch_list_t * list,
	byte * val,
	size_t val_size
) {
	hopscotch_node_t * pred_nodes[(int) list->opts->max_level];
	hopscotch_node_t * succ_nodes[(int) list->opts->max_level];
	uint8_t _level_found;
	hopscotch_res_t _tmp_001 = _list_find_el(
		&_level_found,
		pred_nodes,
		succ_nodes,
		list,
		val,
		val_size
	);
	int16_t level_found = (int16_t) _level_found;
	found[0] = (bool) (
		(_tmp_001 != HOPSCOTCH_RES_LIST__FIND_EL_VAL_NOT_FOUND) &&
		succ_nodes[(int) level_found]->fully_linked &&
		(! succ_nodes[(int) level_found]->marked)
	);
	// Success!
	return HOPSCOTCH_RES__SUCCESS;
}

hopscotch_res_t
hopscotch_list_del_el(
	bool * deleted,
	hopscotch_list_t * list,
	byte * val,
	size_t val_size
) {
	hopscotch_node_t * node_to_del;
	bool marked = false;
	int16_t top_level;
	hopscotch_node_t * pred_nodes[(int) list->opts->max_level];
	hopscotch_node_t * succ_nodes[(int) list->opts->max_level];
	while (true) {
		uint8_t _level_found;
		hopscotch_res_t _tmp_001 = _list_find_el(
			&_level_found,
			pred_nodes,
			succ_nodes,
			list,
			val,
			val_size
		);
		int16_t level_found = (int16_t) _level_found;
		bool _can_delete;
		_list_can_del_el(&_can_delete, succ_nodes[(int) level_found], (uint8_t) level_found);
		if (
			marked ||
			(
				(_tmp_001 != HOPSCOTCH_RES_LIST__FIND_EL_VAL_NOT_FOUND) &&
				_can_delete
			)
		) {
			if (! marked) {
				node_to_del = succ_nodes[(int) level_found];
				top_level = (int16_t) node_to_del->level;
				int _tmp_002 = pthread_mutex_lock(&(node_to_del->lock));
				// TODO(@jonathanmarvens): Figure out a better way to handle this.
				if (_tmp_002 != 0) {
					return HOPSCOTCH_RES_PTHREAD_MUTEX_LOCK_FAIL;
				}
				if (node_to_del->marked) {
					int _tmp_003 = pthread_mutex_unlock(&(node_to_del->lock));
					// TODO(@jonathanmarvens): Figure out a better way to handle this.
					if (_tmp_003 != 0) {
						return HOPSCOTCH_RES_PTHREAD_MUTEX_UNLOCK_FAIL;
					}
					deleted[0] = false;
					// Success!
					return HOPSCOTCH_RES__SUCCESS;
				}
				node_to_del->marked = true;
				marked = true;
			}
			int16_t highest_level_locked = -1;
			hopscotch_node_t * pred_node;
			hopscotch_node_t * succ_node;
			hopscotch_node_t * prev_pred_node = NULL;
			bool valid = true;
			int16_t _level;
			for (_level = 0; valid && (((int) _level) <= ((int) top_level)); _level++) {
				pred_node = pred_nodes[(int) _level];
				succ_node = succ_nodes[(int) _level];
				if (pred_node != prev_pred_node) {
					int _tmp_004 = pthread_mutex_lock(&(pred_node->lock));
					// TODO(@jonathanmarvens): Figure out a better way to handle this.
					if (_tmp_004 != 0) {
						return HOPSCOTCH_RES_PTHREAD_MUTEX_LOCK_FAIL;
					}
					highest_level_locked = (int16_t) _level;
					prev_pred_node = pred_node;
				}
				if (
					(! pred_node->marked) &&
					(pred_node->forward[(int) _level] == succ_node)
				) {
					valid = true;
				} else {
					valid = false;
				}
			}
			if (valid) {
				int16_t _a;
				for (_a = top_level; ((int) _a) >= 0; _a--) {
					pred_nodes[(int) _a]->forward[(int) _a] = node_to_del->forward[(int) _a];
				}
				int _tmp_005 = pthread_mutex_unlock(&(node_to_del->lock));
				// TODO(@jonathanmarvens): Figure out a better way to handle this.
				if (_tmp_005 != 0) {
					return HOPSCOTCH_RES_PTHREAD_MUTEX_UNLOCK_FAIL;
				}
				// Release locks!
				int16_t _b;
				for (_b = 0; ((int) _b) <= ((int) highest_level_locked); _b++) {
					int _tmp_006 = pthread_mutex_unlock(&(pred_nodes[(int) _b]->lock));
					// TODO(@jonathanmarvens): Figure out a better way to handle this.
					if (_tmp_006 != 0) {
						return HOPSCOTCH_RES_PTHREAD_MUTEX_UNLOCK_FAIL;
					}
				}
				deleted[0] = true;
				// Success!
				return HOPSCOTCH_RES__SUCCESS;
			} else {
				// Release locks!
				int16_t _c;
				for (_c = 0; ((int) _c) <= ((int) highest_level_locked); _c++) {
					int _tmp_007 = pthread_mutex_unlock(&(pred_nodes[(int) _c]->lock));
					// TODO(@jonathanmarvens): Figure out a better way to handle this.
					if (_tmp_007 != 0) {
						return HOPSCOTCH_RES_PTHREAD_MUTEX_UNLOCK_FAIL;
					}
				}
				// Invalidate `highest_level_locked`.
				highest_level_locked = -1;
				continue;
			}
		} else {
			deleted[0] = false;
			// Success!
			return HOPSCOTCH_RES__SUCCESS;
		}
	}
}

hopscotch_res_t
hopscotch_list_free(_UNUSED_VAR hopscotch_list_t * list) {
	// Since we use a GC, this function is essentially NOP.
	// Success!
	return HOPSCOTCH_RES__SUCCESS;
}
