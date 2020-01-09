#!/bin/sh -e
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

URL_PREFIX=http://llvm.org/releases/${CLANG_VER}
LLVM_XZ=llvm-${CLANG_VER}.src.tar.xz
CFE_XZ=cfe-${CLANG_VER}.src.tar.xz
CXX_XZ=libcxx-${CLANG_VER}.src.tar.xz
CXXABI_XZ=libcxxabi-${CLANG_VER}.src.tar.xz
RT_XZ=compiler-rt-${CLANG_VER}.src.tar.xz
UNWIND_XZ=libunwind-${CLANG_VER}.src.tar.xz
OPENMP_XZ=openmp-${CLANG_VER}.src.tar.xz
LLVM_DIR=$(basename ${LLVM_XZ} .tar.xz)
CFE_DIR=$(basename ${CFE_XZ} .tar.xz)
CXX_DIR=$(basename ${CXX_XZ} .tar.xz)
CXXABI_DIR=$(basename ${CXXABI_XZ} .tar.xz)
RT_DIR=$(basename ${RT_XZ} .tar.xz)
UNWIND_DIR=$(basename ${UNWIND_XZ} .tar.xz)
OPENMP_DIR=$(basename ${OPENMP_XZ} .tar.xz)

# Delete previous attempts
rm -f ${WORK_DIR}/${LLVM_XZ}
rm -f ${WORK_DIR}/${CFE_XZ}
rm -f ${WORK_DIR}/${CXX_XZ}
rm -f ${WORK_DIR}/${CXXABI_XZ}
rm -f ${WORK_DIR}/${RT_XZ}
rm -f ${WORK_DIR}/${UNWIND_XZ}
rm -f ${WORK_DIR}/${OPENMP_XZ}
rm -rf ${WORK_DIR}/${LLVM_DIR}

# Download what is necessary
(cd ${WORK_DIR} && curl -L ${URL_PREFIX}/${LLVM_XZ} -o ${LLVM_XZ} \
                && curl -L ${URL_PREFIX}/${CFE_XZ} -o ${CFE_XZ} \
                && curl -L ${URL_PREFIX}/${CXX_XZ} -o ${CXX_XZ} \
                && curl -L ${URL_PREFIX}/${CXXABI_XZ} -o ${CXXABI_XZ} \
                && curl -L ${URL_PREFIX}/${RT_XZ} -o ${RT_XZ} \
                && curl -L ${URL_PREFIX}/${UNWIND_XZ} -o ${UNWIND_XZ} \
                && curl -L ${URL_PREFIX}/${OPENMP_XZ} -o ${OPENMP_XZ})

# Decompress and put files to the right place 
(cd ${WORK_DIR} && tar xf ${LLVM_XZ})
(cd ${WORK_DIR} && tar xf ${CFE_XZ})
(cd ${WORK_DIR} && mv ${CFE_DIR} ${LLVM_DIR}/tools/clang)
(cd ${WORK_DIR} && tar xf ${CXX_XZ})
(cd ${WORK_DIR} && mv ${CXX_DIR} ${LLVM_DIR}/projects/libcxx)
(cd ${WORK_DIR} && tar xf ${CXXABI_XZ})
(cd ${WORK_DIR} && mv ${CXXABI_DIR} ${LLVM_DIR}/projects/libcxxabi)
(cd ${WORK_DIR} && tar xf ${RT_XZ})
(cd ${WORK_DIR} && mv ${RT_DIR} ${LLVM_DIR}/projects/compiler-rt)
(cd ${WORK_DIR} && tar xf ${UNWIND_XZ})
(cd ${WORK_DIR} && mv ${UNWIND_DIR} ${LLVM_DIR}/projects/libunwind)

# Stage0: need a clang to compile with libstdc++ and libgcc
echo 'stage0' > ${WORK_DIR}/${LLVM_DIR}/current_stage
STAGE0_BUILD_DIR="${WORK_DIR}/${LLVM_DIR}/stage0_build"
STAGE0_PREFIX="${PWD}/${WORK_DIR}/${LLVM_DIR}/stage0_install"
mkdir -p ${STAGE0_BUILD_DIR}
(cd ${STAGE0_BUILD_DIR} \
 && cmake .. -G "Unix Makefiles" \
             -DCMAKE_C_COMPILER=$(which gcc) \
             -DCMAKE_CXX_COMPILER=$(which g++) \
             -DCLANG_DEFAULT_CXX_STDLIB=libc++ \
             -DCMAKE_CXX_LINK_FLAGS="-Wl,-rpath,${LD_LIBRARY_PATH}" \
             -DCMAKE_INSTALL_PREFIX=${STAGE0_PREFIX} \
             -DCMAKE_BUILD_TYPE=Release)
(cd ${STAGE0_BUILD_DIR} && make ${J} && make install)

# Stage1: get rid of libgcc for libc++ and libc++abi
echo 'stage1' > ${WORK_DIR}/${LLVM_DIR}/current_stage
STAGE1_BUILD_DIR="${WORK_DIR}/${LLVM_DIR}/stage1_build"
STAGE1_PREFIX="${PWD}/${WORK_DIR}/${LLVM_DIR}/stage1_install"
mkdir -p ${STAGE1_BUILD_DIR}
(cd ${STAGE1_BUILD_DIR} \
 && cmake .. -G "Unix Makefiles" \
             -DCMAKE_C_COMPILER=${STAGE0_PREFIX}/bin/clang \
             -DCMAKE_CXX_COMPILER=${STAGE0_PREFIX}/bin/clang++ \
             -DCLANG_DEFAULT_CXX_STDLIB=libc++ \
             -DCLANG_DEFAULT_RTLIB=compiler-rt \
             -DLIBCXXABI_USE_LLVM_UNWINDER=YES \
             -DLIBCXX_USE_COMPILER_RT=YES \
             -DLIBCXXABI_USE_COMPILER_RT=YES \
             -DCMAKE_CXX_LINK_FLAGS="-Wl,-rpath,${LD_LIBRARY_PATH}" \
             -DCMAKE_INSTALL_PREFIX=${STAGE1_PREFIX} \
             -DCMAKE_BUILD_TYPE=Release)
(cd ${STAGE1_BUILD_DIR} && make ${J} && make install)

# Stage2: configure, build and install, get rid of libgcc for all clang binaries
echo 'stage2' > ${WORK_DIR}/${LLVM_DIR}/current_stage
BUILD_DIR="${WORK_DIR}/${LLVM_DIR}/build"
mkdir -p ${BUILD_DIR}
(cd ${BUILD_DIR} && cmake .. -G "Unix Makefiles" \
                             -DCMAKE_C_COMPILER=${STAGE1_PREFIX}/bin/clang \
                             -DCMAKE_CXX_COMPILER=${STAGE1_PREFIX}/bin/clang++ \
                             -DCLANG_DEFAULT_CXX_STDLIB=libc++ \
                             -DCLANG_DEFAULT_RTLIB=compiler-rt \
                             -DLIBCXXABI_USE_LLVM_UNWINDER=YES \
                             -DLIBCXX_USE_COMPILER_RT=YES \
                             -DLIBCXXABI_USE_COMPILER_RT=YES \
                             -DCMAKE_INSTALL_PREFIX=${PREFIX} \
                             -DCMAKE_BUILD_TYPE=Release)
(cd ${BUILD_DIR} && make ${J} && mysudo "make install")

# Configure, build and install LLVM's OpenMP, for some reason it
# cannot be compiled at the same time as the rest of LLVM/Clang
echo 'openmp' > ${WORK_DIR}/${LLVM_DIR}/current_stage
(cd ${WORK_DIR} && tar xf ${OPENMP_XZ})
(cd ${WORK_DIR} && mv ${OPENMP_DIR} ${LLVM_DIR}/projects/openmp)
OPENMP_BUILD_DIR="${WORK_DIR}/${LLVM_DIR}/openmp_build"
mkdir -p ${OPENMP_BUILD_DIR}
(cd ${OPENMP_BUILD_DIR} && cmake ${WORK_DIR}/${LLVM_DIR}/projects/openmp \
                                 -G "Unix Makefiles" \
                                 -DCMAKE_C_COMPILER=${PREFIX}/bin/clang \
                                 -DCMAKE_CXX_COMPILER=${PREFIX}/bin/clang++ \
                                 -DCMAKE_INSTALL_PREFIX=${PREFIX} \
                                 -DCMAKE_BUILD_TYPE=Release)
(cd ${OPENMP_BUILD_DIR} && make ${J} omp && mysudo "make install")

# Create corresponding modulefile
mysudo mkdir -p ${MODULE_PATH}
NOW=$(date)

cat >${WORK_DIR}/${LLVM_DIR}/modulefile <<EOF
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

mysudo "cp ${WORK_DIR}/${LLVM_DIR}/modulefile ${MODULE_FILE}"

# Test whether it works
CLANG="${PREFIX}/bin/clang++"
TMP_CPP="${WORK_DIR}/${LLVM_DIR}/tmp.cpp"

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

(cd ${WORK_DIR}/${LLVM_DIR} && ${CLANG} -std=c++11 -Wall -Wextra tmp.cpp)
(cd ${WORK_DIR}/${LLVM_DIR} && LD_LIBRARY_PATH=${PREFIX}/lib ./a.out)
