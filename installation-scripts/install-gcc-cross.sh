#!/bin/bash -e
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

if [ "${#}" -ne 2 ]; then
  echo "usage: ${0} VERSION BASE_DIR"
  exit 0
fi
VER=${1}
J="$(nproc)"
if [ ${J} -ge 20 ]; then
  J=20
fi

# Ask for ROOT password
printf "ROOT PASSWORD: "
read -s ROOT_PASSWORD
echo

# List of architecture that we need
#ARCHIS="arm aarch64 ppc64el"
ARCHIS="aarch64"

# create and cleanup working dir
BASE_DIR="$(cd ${2} && pwd)/cross"

# installation directory
INSTALL_PREFIX="/opt/local/cross"

# Helper function to get package version
get_version() {
  echo $(apt-cache show ${1} | grep Version | sed 's/[+-].*//g' | sed 's/Version: //g')
}

# My sudo
mysudo() {
  #echo "DEBUG: ${ROOT_PASSWORD}"
  #echo "DEBUG: >>$@<<"
  { sleep 3; echo "${ROOT_PASSWORD}"; } | script -q -c "su -c \"$@\"" /dev/null
}

INSTALL_DIR="${INSTALL_PREFIX}/gcc-${VER}"
mysudo "mkdir -p ${INSTALL_DIR}"
mysudo "chown -R ${USER}:${USER} ${INSTALL_DIR}"
WORK_DIR="${BASE_DIR}/${ARCHI}/gcc-${VER}"
mkdir -p "${WORK_DIR}"
cd "${WORK_DIR}"
 
for ARCHI in ${ARCHIS}; do

  # GCC
  if [ ! -e gcc ]; then
    wget ftp://ftp.fu-berlin.de/unix/languages/gcc/releases/gcc-${VER}/gcc-${VER}.tar.bz2
    tar xf gcc-${VER}.tar.bz2 
    rm -rf gcc-${VER}.tar.bz2
    mv gcc-${VER} gcc
  fi
 
  # BINUTILS
  if [ ! -e binutils ]; then
    TMP=$(get_version binutils)
    wget http://ftp.gnu.org/gnu/binutils/binutils-${TMP}.tar.bz2
    tar xf binutils-${TMP}.tar.bz2 
    rm -rf binutils-${TMP}.tar.bz2
    mv binutils-${TMP} binutils
  fi

  # GLIBC
  if [ ! -e glibc ]; then
    TMP=$(get_version libc6)
    wget http://ftp.gnu.org/gnu/glibc/glibc-${TMP}.tar.bz2
    tar xf glibc-${TMP}.tar.bz2
    rm -rf glibc-${TMP}.tar.bz2
    mv glibc-${TMP} glibc
  fi

  # LINUX KERNEL
  if [ ! -e linux ]; then
    TMP=$(get_version linux-image-amd64)
    if [ "$(echo ${TMP} | cut -c1-3)" = "2.6" ]; then
      URL="https://www.kernel.org/pub/linux/kernel/v2.6/linux-${TMP}.tar.gz"
    fi
    if [ "$(echo ${TMP} | cut -c1)" = "3" ]; then
      URL="https://www.kernel.org/pub/linux/kernel/v3.x/linux-${TMP}.tar.gz"
    fi
    if [ "$(echo ${TMP} | cut -c1-3)" = "3.0" ]; then
      URL="https://www.kernel.org/pub/linux/kernel/v3.0/linux-${TMP}.tar.gz"
    fi
    if [ "$(echo ${TMP} | cut -c1)" = "4" ]; then
      URL="https://www.kernel.org/pub/linux/kernel/v4.x/linux-${TMP}.tar.gz"
    fi
    wget ${URL}
    tar xf linux-${TMP}.tar.gz
    rm -rf linux-${TMP}.tar.gz
    mv linux-${TMP} linux
  fi

  # GMP
  if [ ! -e gmp ]; then
    wget https://gmplib.org/download/gmp/gmp-6.1.2.tar.bz2
    tar xf gmp-6.1.2.tar.bz2
    rm -rf gmp-6.1.2.tar.bz2
    mv gmp-6.1.2 gmp
  fi

  # MPFR
  if [ ! -e mpfr ]; then
    wget http://www.mpfr.org/mpfr-current/mpfr-3.1.5.tar.bz2
    tar xf mpfr-3.1.5.tar.bz2
    rm -rf mpfr-3.1.5.tar.bz2
    mv mpfr-3.1.5 mpfr
  fi

  # MPC
  if [ ! -e mpc ]; then
    wget ftp://ftp.gnu.org/gnu/mpc/mpc-1.0.2.tar.gz
    tar xf mpc-1.0.2.tar.gz
    rm -rf mpc-1.0.2.tar.gz
    mv mpc-1.0.2 mpc
  fi

  # Compile BINUTILS
  if [ ! -e "${INSTALL_DIR}/bin/${ARCHI}-linux-ld" ]; then
    rm -rf build-binutils
    mkdir -p build-binutils
    (cd build-binutils && ../binutils/configure --prefix=${INSTALL_DIR} \
                                                --target=${ARCHI}-linux \
                                                --disable-nls \
                                                --disable-multilib)
    (cd build-binutils && make -j${J})
    (cd build-binutils && make install)
  fi

  # Install kernel headers
  if [ ! -e "${INSTALL_DIR}/${ARCHI}-linux/include/linux" ]; then
    (cd linux && make ARCH=${ARCHI} \
                      INSTALL_HDR_PATH=${INSTALL_DIR}/${ARCHI}-linux \
                      headers_install)
  fi

  # First compilation of GCC
  CROSS_GCC="${INSTALL_DIR}/bin/${ARCHI}-linux-gcc"

  if [ ! -e "${CROSS_GCC}" ]; then
    rm -rf build-gcc
    mkdir -p build-gcc
    (cd gcc && ln -s ../gmp)
    (cd gcc && ln -s ../mpfr)
    (cd gcc && ln -s ../mpc)
    (cd build-gcc && ../gcc/configure --prefix=${INSTALL_DIR} \
                                      --target=${ARCHI}-linux \
                                      --enable-languages=c,c++ \
                                      --disable-nls \
                                      --without-headers \
                                      --disable-multilib)
    (cd build-gcc && make -j${J} all-gcc)
    (cd build-gcc && make install-gcc)
  fi

  # Compilation of GLIBC
  if [ ! -e "${INSTALL_DIR}/${ARCHI}-linux/include/gnu/stubs.h" ]; then
    rm -rf build-glibc
    mkdir -p build-glibc
    (cd build-glibc && ../glibc/configure --prefix=${INSTALL_DIR}/${ARCHI}-linux \
                                          --build=${MACHTYPE} \
                                          --host=${ARCHI}-linux \
                                          --target=${ARCHI}-linux \
                                          --with-headers=${INSTALL_DIR}/${ARCHI}-linux/include \
                                          --disable-multilib \
                                          CC=${CROSS_GCC} \
                                          libc_cv_forced_unwind=yes)
    (cd build-glibc && make install-bootstrap-headers=yes install-headers)
    (cd build-glibc && make -j${J} csu/subdir_lib)
    (cd build-glibc && install csu/crt1.o csu/crti.o csu/crtn.o \
                               ${INSTALL_DIR}/${ARCHI}-linux/lib)
    (cd build-glibc && ${CROSS_GCC} -nostdlib \
                                    -nostartfiles \
                                    -shared -x c /dev/null \
                                    -o ${INSTALL_DIR}/${ARCHI}-linux/lib/libc.so)
    mkdir -p ${INSTALL_DIR}/${ARCHI}-linux/include/gnu
    touch ${INSTALL_DIR}/${ARCHI}-linux/include/gnu/stubs.h
  fi

  # Compilation of LIBGCC
  if [ ! -e "${INSTALL_DIR}/${ARCHI}-linux/lib64/libgcc_s.so" ]; then
    (cd build-gcc && make -j${J} all-target-libgcc)
    (cd build-gcc && make install-target-libgcc)
  fi

  # Compilation of GLIBC
  if [ ! -e "${INSTALL_DIR}/${ARCHI}-linux/lib/libc.so" ]; then
    (cd build-glibc && make -j${J})
    (cd build-glibc && make install)
  fi

  # Compilation of LIBSTDC++
  if [ ! -e "${INSTALL_DIR}/${ARCHI}-linux/lib64/libstdc++.so" ]; then
    (cd build-gcc && make -j${J} all)
    (cd build-gcc && make install)
  fi

done

mysudo "chown -R root:root ${INSTALL_DIR}"
