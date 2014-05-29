OBJS   := cmdserv_tokenize.o          \
	  cmdserv_helpers.o           \
	  cmdserv_logger.o            \
	  cmdserv_config.o            \
	  cmdserv_connection_config.o \
	  cmdserv_connection.o        \
	  cmdserv.o
CFLAGS := -Wall -Wextra -pedantic -Werror \
	  -std=c99 -D_POSIX_C_SOURCE=200809L
TESTS  := t/test_cmdserv_tokenize \
          t/test-cmdserv-helpers  \
          t/minimal_cmdserv       \
          t/test_cmdserv

default: $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: clean $(BIN) test doc

t/test_cmdserv: t/test_cmdserv.c $(OBJS)
	$(CC) $(CFLAGS) $< $(OBJS) -o $@

t/minimal_cmdserv: t/minimal_cmdserv.c $(OBJS)
	$(CC) $(CFLAGS) -Wno-unused-parameter $< $(OBJS) -o $@

t/test_cmdserv_tokenize: t/test_cmdserv_tokenize.c cmdserv_tokenize.o
	$(CC) $(CFLAGS) $< cmdserv_tokenize.o -o $@

t/test-cmdserv-helpers: t/test-cmdserv-helpers.c cmdserv_helpers.o
	$(CC) $(CFLAGS) $< cmdserv_helpers.o -o $@

.PHONY: doc
doc:
	which doxygen && doxygen || true

test: tests

tests: $(TESTS)
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

.PHONY: clean
clean:
	rm -f $(TESTS)
	rm -rf doc/*
	find . \(    -name '*~'       \
                  -o -name '*.o'      \
	          -o -name '*.tmp' \) \
               -exec rm '{}' \; -print
