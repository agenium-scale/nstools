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
GIT_TAG=rocm-3.10.0
PREFIX="/opt/${GIT_TAG}/llvm"
WORK_DIR="${PWD}/tmp/${GIT_TAG}"
J="-j$(nproc)"
mkdir -p ${WORK_DIR}

URL_PREFIX=https://github.com/RadeonOpenCompute/llvm-project.git
LLVM_PROJECT_DIR=tmp/${GIT_TAG}

# Delete previous attempts
rm -rf ${WORK_DIR}

# Git clone and checkout tag
git clone ${URL_PREFIX} ${WORK_DIR}
git -C ${WORK_DIR} checkout ${GIT_TAG}

# Build LLVM and stuff
PROJECTS="clang;lld;compiler-rt"
BUILD_DIR="${WORK_DIR}/build"
mkdir -p "${BUILD_DIR}"
(cd "${BUILD_DIR}" && cmake ../llvm \
                            -DCMAKE_C_COMPILER=gcc \
                            -DCMAKE_CXX_COMPILER=g++ \
                            -DCMAKE_INSTALL_PREFIX="${PREFIX}" \
                            -DCMAKE_BUILD_TYPE=Release \
                            -DLLVM_ENABLE_PROJECTS="${PROJECTS}")
(cd ${BUILD_DIR} && make ${J} && mysudo make install)

# Test whether it works
CLANG="${PREFIX}/bin/clang++"
TMP_CPP="${WORK_DIR}/tmp.cpp"

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

(cd ${WORK_DIR} && \
 ${CLANG} -std=c++11 -Wall -Wextra tmp.cpp)

(cd ${WORK_DIR} && \
 LD_LIBRARY_PATH=${PREFIX}/lib ./a.out)
