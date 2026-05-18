PREFIX      ?= /usr/local
BINDIR      ?= $(PREFIX)/bin
MANDIR      ?= $(PREFIX)/share/man/man1
DESTDIR     ?=

CC          ?= gcc
CSTD        ?= -std=c11
WARN        := -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes \
               -Wmissing-prototypes -Wpointer-arith -Wcast-align \
               -Wwrite-strings -Wno-unused-parameter
OPT         ?= -O2
CFLAGS      ?= $(CSTD) $(WARN) $(OPT) -D_POSIX_C_SOURCE=200809L
CPPFLAGS    += -Iinclude -Ithird_party/tomlc99
LDFLAGS     ?=
LDLIBS      ?=

SAN_FLAGS   := -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer

SRC         := $(wildcard src/*.c) third_party/tomlc99/toml.c
OBJ         := $(SRC:.c=.o)
BIN         := zenv

TESTS       := tests/test_quote tests/test_atomic tests/test_vars

.PHONY: all dev test clean install uninstall fmt compile_commands unit-tests

all: $(BIN)

tests/test_quote: tests/test_quote.c src/quote.c src/error.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

tests/test_atomic: tests/test_atomic.c src/atomic.c src/error.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

tests/test_vars: tests/test_vars.c src/vars.c src/atomic.c src/error.c \
                 third_party/tomlc99/toml.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

unit-tests: $(TESTS)
	@fail=0; for t in $(TESTS); do \
	  ./$$t || fail=1; \
	done; exit $$fail

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

dev:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CSTD) $(WARN) $(SAN_FLAGS) -D_POSIX_C_SOURCE=200809L" \
	        LDFLAGS="-fsanitize=address,undefined" all $(TESTS)

test: dev
	@$(MAKE) -s unit-tests
	@./tests/run.sh

clean:
	rm -f $(OBJ) $(BIN) $(TESTS) $(TESTS:=.o)

compile_commands:
	@printf '[\n' > compile_commands.json
	@i=0; total=$(words $(SRC)); \
	for f in $(SRC); do \
	  i=$$((i+1)); \
	  printf '  {\n    "directory": "%s",\n    "file": "%s",\n    "arguments": [' \
	    "$(CURDIR)" "$(CURDIR)/$$f" >> compile_commands.json; \
	  printf '"%s"' "$(CC)" >> compile_commands.json; \
	  for a in $(CFLAGS) $(CPPFLAGS) -c -o $${f%.c}.o $$f; do \
	    printf ', "%s"' "$$a" >> compile_commands.json; \
	  done; \
	  if [ $$i -lt $$total ]; then \
	    printf ']\n  },\n' >> compile_commands.json; \
	  else \
	    printf ']\n  }\n' >> compile_commands.json; \
	  fi; \
	done
	@printf ']\n' >> compile_commands.json
	@echo "wrote compile_commands.json ($(words $(SRC)) entries)"

install: $(BIN)
	install -Dm755 $(BIN)        $(DESTDIR)$(BINDIR)/$(BIN)
	install -Dm644 man/zenv.1    $(DESTDIR)$(MANDIR)/zenv.1
	install -Dm644 LICENSE       $(DESTDIR)$(PREFIX)/share/licenses/$(BIN)/LICENSE

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN)
	rm -f $(DESTDIR)$(MANDIR)/zenv.1
	rm -f $(DESTDIR)$(PREFIX)/share/licenses/$(BIN)/LICENSE
