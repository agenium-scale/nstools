.POSIX:
.DEFAULT:
.SUFFIXES:

all:
	cc -O2 -ansi -pedantic -Wall -Werror sshdetach.c -o sshdetach

install:
	mkdir -p ~/.local/bin
	cp sshdetach ~/.local/bin

clean:
	rm -f sshdetach
