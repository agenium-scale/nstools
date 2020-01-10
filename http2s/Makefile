# Copyright (c) 2019 Agenium Scale
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

.POSIX:
.DEFAULT:
.SUFFIXES:

# Installation directories
PREFIX    = /opt/local
BIN_DIR   = $(PREFIX)/bin
MAN_DIR   = $(PREFIX)/share/man/man1

VERSION   = 0.1
CC        = cc
OFLAGS    = -g
CFLAGS    = -Wall -Wextra -ansi -pedantic-errors
LDFLAGS   = -lpthread
BIN       = http2s
SOURCES   = $(BIN).c
FILES     = $(SOURCES) Makefile

all: $(BIN)

$(BIN): Makefile $(SOURCES)
	$(CC) $(OFLAGS) $(LDFLAGS) $(CFLAGS) $(BIN).c -o $(BIN)

install: all
	mkdir -p $(BIN_DIR)
	cp $(BIN) $(BIN_DIR)/$(BIN)
	mkdir -p $(MAN_DIR)
	cp $(BIN).1 $(MAN_DIR)/$(BIN).1

uninstall:
	rm $(BIN_DIR)/$(BIN)
	rm $(MAN_DIR)/$(BIN).1

clean:
	rm -f $(BIN)

maintainer-clean: clean
	rm -rf *.o *~ *.tar.bz2 pre_dist/

prepare-dist:
	mkdir -p pre_dist
	rm -rf pre_dist/*
	cp $(FILES) pre_dist

dist: prepare-dist
	mv pre_dist $(BIN)-$(VERSION)
	tar cvf $(BIN)-$(VERSION).tar $(BIN)-$(VERSION)/
	bzip2 -f $(BIN)-$(VERSION).tar
	rm -rf $(BIN)-$(VERSION)
