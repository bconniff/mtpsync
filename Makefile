CC = gcc

MAKE_CFLAGS = ${CFLAGS} -Wall -g
MAKE_LDFLAGS = ${LDFLAGS} -lmtp

COMMON_HEADERS = $(wildcard src/main/*.h)
COMMON_SOURCES = $(wildcard src/main/*.c)
MAIN_SOURCES = $(COMMON_SOURCES) src/main.c
TEST_SOURCES = $(COMMON_SOURCES) $(wildcard src/test/*.c) src/test.c

MAIN_OBJECTS = $(MAIN_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

MAIN = ./bin/mtpsync
TEST = ./bin/mtptest

all: $(MAIN) docs

docs: $(COMMON_HEADERS) $(COMMON_HEADERS) Doxyfile
	doxygen Doxyfile

test: $(TEST)
	valgrind --leak-check=yes $(TEST)

$(MAIN): $(MAIN_OBJECTS)
	mkdir -p ./bin
	$(CC) -o $(MAIN) $(MAIN_OBJECTS) $(MAKE_LDFLAGS)

$(TEST): $(TEST_OBJECTS)
	mkdir -p ./bin
	$(CC) -o $(TEST) $(TEST_OBJECTS) $(MAKE_LDFLAGS)

%.o: %.c
	$(CC) $(MAKE_CFLAGS) -c $< -o $@

clean:
	rm -f $(MAIN) $(TEST) $(MAIN_OBJECTS) $(TEST_OBJECTS) vgcore.* core.*
	rm -rf ./docs/
