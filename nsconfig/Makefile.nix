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
CXX_FLAGS    = $(OFLAGS) -std=c++98 -pedantic \
               -Wall -Wextra -Wconversion \
               -I../ns2/include -DNS_NO_DLLSPEC \
               -c

all: nsconfig nstest

string.o: ../ns2/lib/string.cpp ../ns2/include/ns2/string.hpp
	$(CXX) $(CXX_FLAGS) ../ns2/lib/string.cpp -o $@

fs.o: ../ns2/lib/fs.cpp ../ns2/include/ns2/fs.hpp
	$(CXX) $(CXX_FLAGS) ../ns2/lib/fs.cpp -o $@

process.o: ../ns2/lib/process.cpp ../ns2/include/ns2/process.hpp
	$(CXX) $(CXX_FLAGS) ../ns2/lib/process.cpp -o $@

backtrace.o: ../ns2/lib/backtrace.cpp ../ns2/include/ns2/backtrace.hpp
	$(CXX) $(CXX_FLAGS) ../ns2/lib/backtrace.cpp -o $@

levenshtein.o: ../ns2/lib/levenshtein.cpp ../ns2/include/ns2/levenshtein.hpp
	$(CXX) $(CXX_FLAGS) ../ns2/lib/levenshtein.cpp -o $@

backend_ninja.o: backend_ninja.cpp backend_ninja.hpp
	$(CXX) $(CXX_FLAGS) backend_ninja.cpp -o $@

backend_make.o: backend_make.cpp backend_make.hpp
	$(CXX) $(CXX_FLAGS) backend_make.cpp -o $@

compiler.o: compiler.cpp compiler.hpp
	$(CXX) $(CXX_FLAGS) compiler.cpp -o $@

nsconfig.o: nsconfig.cpp nsconfig.hpp
	$(CXX) $(CXX_FLAGS) nsconfig.cpp -o $@

lambda.o: lambda.cpp lambda.hpp
	$(CXX) $(CXX_FLAGS) lambda.cpp -o $@

parser.o: parser.cpp parser.hpp
	$(CXX) $(CXX_FLAGS) parser.cpp -o $@

shell.o: shell.cpp shell.hpp
	$(CXX) $(CXX_FLAGS) shell.cpp -o $@

find_exe_lib_header.o: find_exe_lib_header.cpp find_exe_lib_header.hpp
	$(CXX) $(CXX_FLAGS) find_exe_lib_header.cpp -o $@

nstest.o: nstest.cpp
	$(CXX) $(CXX_FLAGS) nstest.cpp -o $@

nsconfig: string.o fs.o process.o backtrace.o levenshtein.o backend_ninja.o \
          backend_make.o compiler.o nsconfig.o parser.o shell.o \
          find_exe_lib_header.o lambda.o
	$(CXX) string.o fs.o process.o backtrace.o levenshtein.o \
	       backend_ninja.o backend_make.o compiler.o nsconfig.o parser.o \
	       lambda.o shell.o find_exe_lib_header.o -o $@

nstest: string.o fs.o process.o backtrace.o nstest.o
	$(CXX) string.o fs.o process.o backtrace.o nstest.o -o $@

install: all
	mkdir -p ~/.local/bin
	-cp nsconfig ~/.local/bin/nsconfig
	-cp nstest ~/.local/bin/nstest
	mkdir -p ~/.vim/syntax
	-cp nsconfig.syntax.vim ~/.vim/syntax/nsconfig.vim
	mkdir -p ~/.vim/ftdetect
	-cp nsconfig.ftdetect.vim ~/.vim/ftdetect/nsconfig.vim

clean:
	rm -f nsconfig nstest *.o
