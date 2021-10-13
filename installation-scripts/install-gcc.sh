#!/bin/bash
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

set -e
#set -x

# Ask for ROOT password
printf "ROOT PASSWORD: "
read -s ROOT_PASSWORD
echo

# My sudo
mysudo() {
  echo "${ROOT_PASSWORD}" | sudo -E -S $@
}

# Set variables
TARGET="${2}"
if [ "${TARGET}" != "" ]; then
  SYSROOT="--target=${TARGET} --with-sysroot=/"
  MODULE_NAME="${TARGET}-gcc"
else
  SYSROOT=""
  MODULE_NAME="gcc"
fi
MODULE_PATH_NULL=`module path null`
MODULE_PATH="`dirname ${MODULE_PATH_NULL}`/${MODULE_NAME}"
SVN_URL="https://gcc.gnu.org/svn/gcc/trunk" 
WORK_DIR=tmp
J="-j$(nproc)"
if [ "${1}" = "trunk" ]; then
  GCC_VER="trunk"
  REV=`svn info ${SVN_URL} | grep Revision | cut -d' ' -f2`
  PREFIX="/opt/local/${MODULE_NAME}/trunk-${REV}"
  MODULE_FILE=${MODULE_PATH}/trunk-${REV}
else
  GCC_VER="${1}"
  PREFIX="/opt/local/${MODULE_NAME}/${GCC_VER}"
  MODULE_FILE=${MODULE_PATH}/${GCC_VER}
fi
mkdir -p ${WORK_DIR}

#### Dependencies first

# GMP
TAR_GZ=gmp-6.1.2.tar.bz2
SRC_DIR=gmp-6.1.2

rm -f ${WORK_DIR}/${TAR_GZ}
rm -rf ${WORK_DIR}/${SRC_DIR}
(cd ${WORK_DIR} && curl -L https://ftp.gnu.org/gnu/gmp/${TAR_GZ} -o ${TAR_GZ})
(cd ${WORK_DIR} && tar xf ${TAR_GZ})
(cd ${WORK_DIR}/${SRC_DIR} && ./configure --prefix="${PREFIX}" \
                           && make ${J} \
                           && mysudo "make install")

# MPFR
TAR_GZ=mpfr-4.0.2.tar.gz
SRC_DIR=mpfr-4.0.2

rm -f ${WORK_DIR}/${TAR_GZ}
rm -rf ${WORK_DIR}/${SRC_DIR}
(cd ${WORK_DIR} && curl -L https://ftp.gnu.org/gnu/mpfr/${TAR_GZ} -o ${TAR_GZ})
(cd ${WORK_DIR} && tar xf ${TAR_GZ})
(cd ${WORK_DIR}/${SRC_DIR} && ./configure --with-gmp="${PREFIX}" \
                                          --prefix="${PREFIX}" \
                           && make ${J} \
                           && mysudo "make install")

# MPC
TAR_GZ=mpc-1.1.0.tar.gz
SRC_DIR=mpc-1.1.0

rm -f ${WORK_DIR}/${TAR_GZ}
rm -rf ${WORK_DIR}/${SRC_DIR}
(cd ${WORK_DIR} && curl -L https://ftp.gnu.org/gnu/mpc/${TAR_GZ} -o ${TAR_GZ})
(cd ${WORK_DIR} && tar xf ${TAR_GZ})
(cd ${WORK_DIR}/${SRC_DIR} && ./configure --with-mpfr="${PREFIX}" \
                                          --with-gmp="${PREFIX}" \
                                          --prefix="${PREFIX}" \
                           && make ${J} \
                           && mysudo "make install")

# Binutils
TAR_GZ=binutils-2.33.1.tar.gz
SRC_DIR=binutils-2.33.1

rm -f ${WORK_DIR}/${TAR_GZ}
rm -rf ${WORK_DIR}/${SRC_DIR}
(cd ${WORK_DIR} && curl -L https://ftp.gnu.org/gnu/binutils/${TAR_GZ} -o ${TAR_GZ})
(cd ${WORK_DIR} && tar xf ${TAR_GZ})
(cd ${WORK_DIR}/${SRC_DIR} && ./configure --prefix="${PREFIX}" ${SYSROOT} \
                           && make ${J} \
                           && mysudo "make install")

#### GCC now

if [ "${GCC_VER}" = "trunk" ]; then
  # Set variables
  SRC_DIR="trunk-${REV}"

  # Delete previous attempts
  rm -rf ${WORK_DIR}/${SRC_DIR}
  
  # SVN checkout trunk
  (cd ${WORK_DIR} && svn checkout ${SVN_URL} ${SRC_DIR})
else
  # Set variables
  URL_PREFIX=ftp://ftp.fu-berlin.de/unix/languages/gcc/releases/gcc-${GCC_VER}
  TAR_GZ=gcc-${GCC_VER}.tar.gz
  SRC_DIR=$(basename ${TAR_GZ} .tar.gz)

  # Delete previous attempts
  rm -f ${WORK_DIR}/${TAR_GZ}
  rm -rf ${WORK_DIR}/${SRC_DIR}
  
  # Download and untar
  (cd ${WORK_DIR} && curl -L ${URL_PREFIX}/${TAR_GZ} -o ${TAR_GZ})
  (cd ${WORK_DIR} && tar xf ${TAR_GZ})
fi

# Find target
if [ "${TARGET}" != "" ]; then
  SYSROOT="${SYSROOT} --with-native-system-header-dir=/usr/${TARGET}/include"
fi

# Specific flags w.r.t wanted version of GCC
MAJOR=`echo ${1} | cut -f1 -d'.'`
if [ "${MAJOR}" == "5" ]; then
  # GCC 5 libsanitizer uses ustat.h which does not exist anymore. As we do
  # not need this library we simply disable its compilation.
  CONFIGURE_FLAGS="--disable-libsanitizer"
  # GCC 5 is capable of C++11
  CPP_STD="-std=c++11"
elif [ "${MAJOR}" == "4" ]; then
  # GCC 4 texi files are not compatible with recent versions of
  # As we do not need the documentation we empty them.
  for i in `find "${WORK_DIR}/${SRC_DIR}" -iname '*.texi'`; do
    echo > ${i}
  done
  for i in `find "${WORK_DIR}/${SRC_DIR}" -iname '*.texi.in'`; do
    echo > ${i}
  done
  sed -i -e 's/: s-tm-texi;/: ;/g' ${WORK_DIR}/${SRC_DIR}/gcc/Makefile.in
  # GCC 4 needs the GCC_VERSION macro to be defined properly
  CONFIGURE_FLAGS="CFLAGS=-fgnu89-inline --disable-libcilkrts \
                   --disable-libsanitizer"
  # GCC 4 uses definitions of old header files
  find "${WORK_DIR}/${SRC_DIR}" -iname '*.[ch]' -type f -exec \
      sed -i -e 's/struct ucontext/struct ucontext_t/g' {} +
  # GCC 4.8.5 has a problem of declaration of libc_name_p
  MINOR=`echo ${1} | cut -f2 -d'.'`
  if [ "${MINOR}" == "8" ]; then
    sed -i 's,__attribute__ ((__gnu_inline__)),// no gnu_inline,g' \
        ${WORK_DIR}/${SRC_DIR}/gcc/cp/cfns.h
  fi
  # GCC 4 is not capable of C++11
  CPP_STD="-std=c++98"
else
  CONFIGURE_FLAGS=""
  CPP_STD="-std=c++11"
fi

# Configure, make and install
BUILD_DIR="${WORK_DIR}/${SRC_DIR}/build"
mkdir -p ${BUILD_DIR}
(cd ${BUILD_DIR} && ../configure --prefix=${PREFIX} \
                                 --with-gmp=${PREFIX} \
                                 --with-mpfr=${PREFIX} \
                                 --with-mpc=${PREFIX} \
                                 ${SYSROOT} \
                                 --disable-multilib \
                                 --enable-multiarch \
                                 ${CONFIGURE_FLAGS} \
                                 --enable-languages=c,c++,fortran \
                                 --disable-bootstrap)
(cd ${BUILD_DIR} && make ${J})
(cd ${BUILD_DIR} && mysudo "make install")

# Create corresponding modulefile
mysudo mkdir -p ${MODULE_PATH}
NOW=$(date)

cat >${WORK_DIR}/${SRC_DIR}/modulefile <<EOF
#%Module -*- tcl -*-
## Module for GCC
## Automatically generated on ${NOW}

proc ModulesHelp {} {
  puts stderr "Setup GCC ${GCC_VER}" 
}

module-whatis "Setup GCC ${GCC_VER}"

prepend-path  PATH               ${PREFIX}/bin
prepend-path  MANPATH            ${PREFIX}/share/man
prepend-path  LD_LIBRARY_PATH    ${PREFIX}/lib64
prepend-path  C_INCLUDE_PATH     ${PREFIX}/include
prepend-path  CPLUS_INCLUDE_PATH ${PREFIX}/include
EOF

mysudo "cp ${WORK_DIR}/${SRC_DIR}/modulefile ${MODULE_FILE}"

# Test whether it works

if [ "${TARGET}" != "" ]; then
  GCC=${PREFIX}/bin/${TARGET}-g++
else
  GCC=${PREFIX}/bin/g++
fi
TMP_CPP=${WORK_DIR}/${SRC_DIR}/tmp.cpp

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

(cd ${WORK_DIR}/${SRC_DIR} && ${GCC} ${CPP_STD} -Wall -Wextra tmp.cpp)
(cd ${WORK_DIR}/${SRC_DIR} && ./a.out)
