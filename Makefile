
CC=cc
CFLAGS=-W -Wall -Wextra
OBJECTS=bvalue.o compile.o compiler_code.o dict.o collect_garbage.o main.o \
	object.o util.o value.o vm.o

all: release

release: CFLAGS+=-Os
release: LDFLAGS+=-s
release: toy

debug: CFLAGS+=-g
debug: toy

toy: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

compiler_code.c: *.js
	node translate_to_c.node.js > $@

clean:
	rm -rf $(OBJECTS) compiler_code.c toy
