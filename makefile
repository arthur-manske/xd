.POSIX:

CC	:= c99
CFLAGS	:= -O0 -s -D_POSIX_C_SOURCE=200809L

SRC = src
OBJ = obj

.PHONY: clean objects 

.SUFFIXES:
.SUFFIXES: .c

all: $(SRC)/xd 

.c:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -lc 

install-local: $(SRC)/xd
	mkdir -p ~/.local/bin
	mkdir -p ~/.local/share/man/man1
	cp $< ~/.local/bin/xd 
	cp man/xd.1 ~/.local/share/man/man1
	### Please ensure that your PATH environment variable contains ~/.local/bin. ###

install: $(SRC)/xd
	mkdir -p /usr/local/bin
	mkdir -p /usr/local/share/man/man1
	cp $< /usr/local/bin/xd 
	cp man/xd.1 /usr/local/share/man/man1

clean:
	rm -rf $(SRC)/xd

