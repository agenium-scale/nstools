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

if [ "${#}"  -ne 2 ]; then
  echo "usage: ${0} VERSION BASE_DIR"
  exit 0
fi
J="-j$(nproc)"
VER="${1}"

# create and cleanup working dir
WORK_DIR="$(cd ${2} && pwd)/gcc-${VER}-deps"
mkdir -p ${WORK_DIR}
cd ${WORK_DIR}
rm -rf *

# download what is necessary
download_and_extract() {
  URL_PREFIX="${1}"
  FILE="${2}"
  wget "${URL_PREFIX}/${FILE}"
  tar xf "${FILE}"
}

# create install prefix
PREFIX="/opt/local/gcc-${VER}"
mkdir -p "${PREFIX}"

# install GMP
download_and_extract "https://gmplib.org/download/gmp" "gmp-6.1.2.tar.xz"
(  cd gmp-6.1.2 \
&& ./configure --prefix="${PREFIX}" \
&& make \
&& su -c "make install")

# install MPFR
download_and_extract "http://www.mpfr.org/mpfr-current" "mpfr-3.1.5.tar.xz"
(  cd mpfr-3.1.5 \
&& ./configure --with-gmp="${PREFIX}" --prefix="${PREFIX}" \
&& make \
&& su -c "make install")

# install MPC
download_and_extract "ftp://ftp.gnu.org/gnu/mpc" "mpc-1.0.3.tar.gz"
(  cd mpc-1.0.3 \
&& ./configure --with-mpfr="${PREFIX}" --with-gmp="${PREFIX}" --prefix="${PREFIX}" \
&& make \
&& su -c "make install")
