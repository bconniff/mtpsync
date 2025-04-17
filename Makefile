CC = gcc
CFLAGS = -Wall -g

COMMON_SOURCES = $(wildcard src/main/*.c)
MAIN_SOURCES = $(COMMON_SOURCES) src/main.c
TEST_SOURCES = $(COMMON_SOURCES) $(wildcard src/test/*.c) src/test.c

MAIN_OBJECTS = $(MAIN_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

ALL_CLEAN = $(MAIN) $(TEST) $(MAIN_OBJECTS) $(TEST_OBJECTS)

LIBS = -lmtp
MAIN = ./bin/mtpsync
TEST = ./bin/mtptest

all: $(MAIN)

test: $(TEST)
	valgrind --leak-check=yes $(TEST)

$(MAIN): $(MAIN_OBJECTS)
	mkdir -p ./bin
	$(CC) $(CFLAGS) -o $(MAIN) $(MAIN_OBJECTS) $(LIBS)

$(TEST): $(TEST_OBJECTS)
	mkdir -p ./bin
	$(CC) $(CFLAGS) -o $(TEST) $(TEST_OBJECTS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(MAIN) $(TEST) $(MAIN_OBJECTS) $(TEST_OBJECTS) vgcore.* core.*
