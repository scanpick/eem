ROOT_DIR = $(shell pwd)

AR ?= ar
CC ?= gcc
CFLAGS = -Ideps -lm -lpthread -pedantic -std=c99 -v -Wall -Wextra

ifeq ($(HOPSCOTCH_COMPILE_DEBUG),true)
	CFLAGS += -g -O0
else
	CFLAGS += -O2
endif

PREFIX ?= /usr/local

DEPS += $(wildcard deps/*/*.c)
SRCS += $(wildcard src/*.c)
OBJS += $(DEPS:.c=.o)
OBJS += $(SRCS:.c=.o)

#########################

default: build

#########################

%.o: %.c
	$(CC) $(CFLAGS) -c -Ibuild/include -o $@ $<

build: clean extern-deps/github.com/ivmai/bdwgc build-final

build-final: build/lib/libhopscotch.so
	mkdir -pv build/lib/libgc && \
	cd build/lib/libgc && \
	$(AR) -x -Lv ../libgc.a && \
	cd .. && \
	$(AR) -r -sv libhopscotch.a libgc/*.o && \
	mkdir -pv $(ROOT_DIR)/build/include/hopscotch && \
	cp -fv $(ROOT_DIR)/src/hopscotch.h $(ROOT_DIR)/build/include/hopscotch/

build-no-extern-deps: build-final

build/lib/libhopscotch.a: $(OBJS)
	mkdir -pv build/lib && \
	rm -f $@ && \
	$(AR) -r -csv $@ $^

build/lib/libhopscotch.so: build/lib/libhopscotch.a
	$(CC) -o $@ -shared $<

clean:
	rm -frv *.o build deps/*/*.o src/*.o test test.dSYM

extern-deps/github.com/ivmai/bdwgc:
	cd $@ && \
	rm -fv ./libatomic_ops && \
	ln -fsv ../libatomic_ops ./libatomic_ops && \
	./autogen.sh && \
	./configure --enable-threads=posix --prefix=$(ROOT_DIR)/build && \
	make && \
	make install

install: build-no-extern-deps
	mkdir -pv $(PREFIX)/include/hopscotch && \
	mkdir -pv $(PREFIX)/lib && \
	cp -fv src/hopscotch.h $(PREFIX)/include/hopscotch/hopscotch.h && \
	cp -fv build/lib/libhopscotch.a $(PREFIX)/lib/libhopscotch.a

test: build-no-extern-deps
	$(CC) -Ibuild/include -Ideps -lpthread -O2 -o test -pedantic -std=c99 -v -Wall -Wextra test.c build/lib/libhopscotch.a

uninstall:
	rm -frv $(PREFIX)/include/hopscotch/hopscotch.h && \
	rm -frv $(PREFIX)/lib/libhopscotch.a

.PHONY: default
.PHONY: build build-final build-no-extern-deps clean install test uninstall
.PHONY: extern-deps/github.com/ivmai/bdwgc
