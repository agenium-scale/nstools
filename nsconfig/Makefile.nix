# MIT License
#
# Copyright (c) 2019 Agenium Scale
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
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

CXX          = g++
OFLAGS       = -O3 -g
CXX_FLAGS    = $(OFLAGS) -pthread -std=c++98 -pedantic \
               -Wall -Wextra -Wconversion \
               -I../ns2/include -DNS_NO_DLLSPEC
NS2_SRC      = ../ns2/src/string.cpp ../ns2/src/fs.cpp ../ns2/src/process.cpp \
               ../ns2/src/backtrace.cpp ../ns2/src/levenshtein.cpp
NSCONFIG_SRC = backend_ninja.cpp \
               backend_make.cpp \
               compiler.cpp \
               nsconfig.cpp \
               parser.cpp \
               shell.cpp \
               find_exe_lib_header.cpp

all:
	$(CXX) $(CXX_FLAGS) $(NS2_SRC) $(NSCONFIG_SRC) -o nsconfig
	$(CXX) $(CXX_FLAGS) $(NS2_SRC) nstest.cpp -o nstest

nsconfig: ../.git/logs/HEAD
	$(CXX) $(CXX_FLAGS) $(NS2_SRC) $(NSCONFIG_SRC) -o nsconfig

nstest: ../.git/logs/HEAD
	$(CXX) $(CXX_FLAGS) $(NS2_SRC) nstest.cpp -o nstest

install: all
	mkdir -p ~/.local/bin
	-cp nsconfig ~/.local/bin/nsconfig
	-cp nstest ~/.local/bin/nstest
	mkdir -p ~/.vim/syntax
	-cp nsconfig.syntax.vim ~/.vim/syntax/nsconfig.vim
	mkdir -p ~/.vim/ftdetect
	-cp nsconfig.ftdetect.vim ~/.vim/ftdetect/nsconfig.vim

clean:
	rm -f nsconfig nstest
