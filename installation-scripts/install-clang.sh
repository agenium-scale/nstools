#!/bin/sh -e
# Copyright (c) 2020 Agenium Scale
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

# Ask for ROOT password
printf "ROOT PASSWORD: "
read -s ROOT_PASSWORD
echo

# My sudo
mysudo() {
  echo "${ROOT_PASSWORD}" | sudo -E -S $@
}

# Set variables
CLANG_VER="${1}"
PREFIX="/opt/local/clang/${CLANG_VER}"
MODULE_PATH="$(dirname $(module path null))/clang"
MODULE_FILE=${MODULE_PATH}/${CLANG_VER}
WORK_DIR="${PWD}/tmp"
J="-j$(nproc)"
mkdir -p ${WORK_DIR}

URL_PREFIX=https://github.com/llvm/llvm-project/releases/download/llvmorg-${CLANG_VER}
LLVM_PROJECT_XZ=llvm-project-${CLANG_VER}.tar.xz
LLVM_PROJECT_DIR=$(basename ${LLVM_PROJECT_XZ} .tar.xz)

# Delete previous attempts
rm -f ${WORK_DIR}/${LLVM_PROJECT_XZ}

# Download what is necessary
(cd ${WORK_DIR} && \
 curl -L ${URL_PREFIX}/${LLVM_PROJECT_XZ} -o ${LLVM_PROJECT_XZ})

# Decompress and put files to the right place 
(cd ${WORK_DIR} && tar xf ${LLVM_PROJECT_XZ})

# Build LLVM and stuff
PROJECTS="clang;libcxx;libcxxabi;libunwind;lldb;compiler-rt;lld"
BUILD_DIR="${WORK_DIR}/${LLVM_PROJECT_DIR}/build"
mkdir -p "${BUILD_DIR}"
(cd "${BUILD_DIR}" && cmake ../llvm \
                            -DCMAKE_C_COMPILER=gcc \
                            -DCMAKE_CXX_COMPILER=g++ \
                            -DCMAKE_INSTALL_PREFIX=${PREFIX} \
                            -DCMAKE_BUILD_TYPE=Release \
                            -DLLVM_ENABLE_PROJECTS="${PROJECTS}")
(cd ${BUILD_DIR} && make ${J} && mysudo make install)

# Create corresponding modulefile
mysudo mkdir -p ${MODULE_PATH}
NOW=$(date)

cat >${WORK_DIR}/${LLVM_PROJECT_DIR}/modulefile <<EOF
#%Module -*- tcl -*-
## Module for Clang
## Automatically generated on ${NOW}

proc ModulesHelp {} {
  puts stderr "Setup Clang ${CLANG_VER}" 
}

module-whatis "Setup Clang ${CLANG_VER}"

prepend-path  PATH               ${PREFIX}/bin
prepend-path  MANPATH            ${PREFIX}/share/man
prepend-path  LD_LIBRARY_PATH    ${PREFIX}/lib
prepend-path  C_INCLUDE_PATH     ${PREFIX}/include
prepend-path  CPLUS_INCLUDE_PATH ${PREFIX}/include
EOF

mysudo "cp ${WORK_DIR}/${LLVM_PROJECT_DIR}/modulefile ${MODULE_FILE}"

# Test whether it works
CLANG="${PREFIX}/bin/clang++"
TMP_CPP="${WORK_DIR}/${LLVM_PROJECT_DIR}/tmp.cpp"

cat >${TMP_CPP} <<EOF
#include <iostream>

int main() {
  std::cout << std::endl
            << "======================================" << std::endl
            << "|                                    |" << std::endl
            << "|      H E L L O   W O R L D         |" << std::endl
            << "|                                    |" << std::endl
            << "======================================" << std::endl
            << std::endl;
  for (int i = 0; i < 10; i++) {
    std::cout << "  " << i;
  }
  std::cout << std::endl;
  std::cout << std::endl
            << "======================================" << std::endl
            << "|                                    |" << std::endl
            << "|        SEEMS TO BE WORKING         |" << std::endl
            << "|                                    |" << std::endl
            << "======================================" << std::endl
            << std::endl;
  return 0;
}
EOF

(cd ${WORK_DIR}/${LLVM_PROJECT_DIR} && \
 ${CLANG} -std=c++11 -Wall -Wextra tmp.cpp)

(cd ${WORK_DIR}/${LLVM_PROJECT_DIR} && \
 LD_LIBRARY_PATH=${PREFIX}/lib ./a.out)
