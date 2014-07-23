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

#include <hopscotch/hopscotch.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

int
main(void) {
	hopscotch_list_t * list = NULL;
	hopscotch_opts_t list_opts = {
		.cmp = NULL,
		.max_level = (uint8_t) 0,
		.rand_level_p = (double) 0,
	};
	list_opts.gc.malloc = NULL;
	hopscotch_list_new(&list, &list_opts);
	bool a;
	hopscotch_list_add_el(
		&a,
		list,
		(hopscotch_byte *) ("hello"),
		(size_t) 6
	);
	bool b;
	hopscotch_list_add_el(
		&b,
		list,
		(hopscotch_byte *) ("hola"),
		(size_t) 5
	);
	bool c;
	hopscotch_list_contains_el(
		&c,
		list,
		(hopscotch_byte *) ("homie"),
		(size_t) 6
	);
	if (c) {
		printf("\"homie\" found!\n");
	}
	bool d;
	hopscotch_list_contains_el(
		&d,
		list,
		(hopscotch_byte *) ("hello"),
		(size_t) 6
	);
	if (d) {
		printf("\"hello\" found!\n");
	}
	bool e;
	hopscotch_list_contains_el(
		&e,
		list,
		(hopscotch_byte *) ("hola"),
		(size_t) 5
	);
	if (e) {
		printf("\"hola\" found!\n");
	}
	bool f;
	hopscotch_list_del_el(
		&f,
		list,
		(hopscotch_byte *) ("hola"),
		(size_t) 5
	);
	bool g;
	hopscotch_list_contains_el(
		&g,
		list,
		(hopscotch_byte *) ("hola"),
		(size_t) 5
	);
	if (g) {
		printf("\"hola\" found!\n");
	}
	hopscotch_list_free(list);
	return EXIT_SUCCESS;
}
