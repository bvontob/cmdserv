OBJS   := cmdserv_tokenize.o          \
	  cmdserv_helpers.o           \
	  cmdserv_logger.o            \
	  cmdserv_config.o            \
	  cmdserv_connection_config.o \
	  cmdserv_connection.o        \
	  cmdserv.o                   \
	  interceptors.o
TESTS  := t/test_cmdserv_tokenize \
          t/test-cmdserv-helpers  \
          t/minimal_cmdserv       \
          t/test_cmdserv          \
	  t/too-many-connections  \
	  t/close-no-read

FORCE_FLAGS := -Wall -Wextra -pedantic -Werror \
	       -Wwrite-strings -Wshadow -Wundef -Wformat \
	       -Wcast-align -Wcast-qual -Wfloat-equal \
	       -D_POSIX_C_SOURCE=200809L \
               -fstack-protector-all

# Compiler compatibility: We support gcc, pcc, clang, and tcc (although the
# generated code crashes currenly using tcc on Ubuntu 12.04)
ifeq ($(CC),pcc)
  # The Portable C Compiler pcc needs this on Ubuntu 12.04 as of 2014-07-02
  # to compile against glibc, as it won't find bits/predefs.h included from
  # features.h
  FORCE_FLAGS += -I/usr/include/x86_64-linux-gnu
endif
ifeq ($(CC),tcc)
  # The Tiny C Compiler tcc should warn about features from gcc it ignores
  FORCE_FLAGS += -Wunsupported
else
  # gcc, clang, and pcc all understand (or at leat ignore) and need this to
  # compile our C99 code. Only tcc chokes on it but compiles our C99 fine
  # without.
  FORCE_FLAGS += -std=c99
endif

default: $(OBJS)

%.o: %.c
	$(CC) $(FORCE_FLAGS) $(CFLAGS) -c -o $@ $<

all: clean test docs

gcov: FORCE_FLAGS += -fprofile-arcs -ftest-coverage -O0
gcov: clean $(OBJS) test
	gcov *.c *.h

t/test_cmdserv: t/test_cmdserv.c $(OBJS)
	$(CC) $(FORCE_FLAGS) $(CFLAGS) $< $(OBJS) -o $@

t/minimal_cmdserv: t/minimal_cmdserv.c $(OBJS)
	$(CC) $(FORCE_FLAGS) $(CFLAGS) -Wno-unused-parameter $< $(OBJS) -o $@

t/test_cmdserv_tokenize: t/test_cmdserv_tokenize.c cmdserv_tokenize.o
	$(CC) $(FORCE_FLAGS) $(CFLAGS) $< cmdserv_tokenize.o -o $@

t/test-cmdserv-helpers: t/test-cmdserv-helpers.c cmdserv_helpers.o
	$(CC) $(FORCE_FLAGS) $(CFLAGS) $< cmdserv_helpers.o -o $@

t/too-many-connections: t/too-many-connections.c t/clientlib.o
	$(CC) $(FORCE_FLAGS) $(CFLAGS) $< t/clientlib.o -o $@

t/close-no-read: t/close-no-read.c t/clientlib.o
	$(CC) $(FORCE_FLAGS) $(CFLAGS) $< t/clientlib.o -o $@

.PHONY: doc
doc: docs

.PHONY: docs
docs:
	which doxygen && doxygen || true

test: check
tests: check

check: CFLAGS += -DINTERCEPT
check: $(TESTS)
	`which valgrind >/dev/null && echo "valgrind -q --error-exitcode=99"` \
		./t/test_cmdserv_tokenize \
		< t/test_cmdserv_tokenize.in \
		> t/test_cmdserv_tokenize.out
	diff -u t/test_cmdserv_tokenize.exp t/test_cmdserv_tokenize.out \
		&& rm t/test_cmdserv_tokenize.out

	`which valgrind >/dev/null && echo "valgrind -q --error-exitcode=99"` \
		./t/test-cmdserv-helpers \
		< t/test-cmdserv-helpers.data \
		> t/test-cmdserv-helpers.out
	diff -u t/test-cmdserv-helpers.data t/test-cmdserv-helpers.out \
		&& rm t/test-cmdserv-helpers.out

	t/test_cmdserv.sh

.PHONY: clean
clean:
	rm -f $(TESTS)
	rm -rf doc/*
	find . \(    -name '*~'       	\
                  -o -name '*.o'      	\
	          -o -name '*.tmp' 	\
	          -o -name '*.gcda'     \
	          -o -name '*.gcno'     \
	          -o -name '*.gcov' \) 	\
               -exec rm '{}' \; -print
