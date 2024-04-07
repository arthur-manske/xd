.POSIX:

CC := clang
CFLAGS := -Iinc -fmerge-all-constants #-fomit-frame-pointer 
LDFLAGS := -O3 -march=native -flto -Wl,-s,--gc-sections,--strip-all,--strip-debug,-z,relro,-z,defs,-z,noexecstack,--hash-style=sysv,--discard-all,--no-ld-generated-unwind-info,-O,3 -lcurses -static
SRC = src
OBJ = obj

.PHONY: clean objects 

.SUFFIXES:
.SUFFIXES: .c

all: $(SRC)/xd

.c:
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@ -lcurses

install: all
	mkdir -p /usr/local/bin
	mkdir -p /usr/local/share/man/man1
	cp src/xd /usr/local/bin
	chmod +x /usr/local/bin/xd
	cp man/xd.1 /usr/local/share/man/man1/
	cp src/xd /usr/local/bin

local-install: all
	mkdir -p ~/.local/bin
	mkdir -p ~/.local/share/man/man1
	cp src/xd ~/.local/bin
	chmod +x ~/.local/bin/xd
	cp man/xd.1 ~/.local/share/man/man1/
	echo "Please ensure that your PATH enviroment variable contains ~/.local/bin."

clean:
	rm -rf $(SRC)/xd
