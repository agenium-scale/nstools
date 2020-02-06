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
QEMU_VER="${1}"
PREFIX="/opt/local/qemu/${QEMU_VER}"
MODULE_PATH="$(dirname $(module path null))/qemu"
MODULE_FILE=${MODULE_PATH}/${QEMU_VER}
WORK_DIR="${PWD}/tmp"
J="-j$(nproc)"
TAR_XZ="qemu-${QEMU_VER}.tar.xz"
URL_PREFIX="https://download.qemu.org"

# Delete previous attempts
rm -rf ${WORK_DIR}
mkdir -p ${WORK_DIR}

# Download what is necessary
(cd ${WORK_DIR} && curl -L ${URL_PREFIX}/${TAR_XZ} -o ${TAR_XZ})

# Decompress files 
(cd ${WORK_DIR} && tar xf ${TAR_XZ})

# Compile it
SRC_DIR=`basename ${TAR_XZ} .tar.xz`
(cd ${WORK_DIR}/${SRC_DIR} && \
 ./configure --prefix="${PREFIX}" \
             --disable-glusterfs)
(cd ${WORK_DIR}/${SRC_DIR} && make ${J} && mysudo "make install")

# Create corresponding modulefile
mysudo mkdir -p ${MODULE_PATH}
NOW=$(date)

cat >${WORK_DIR}/${SRC_DIR}/modulefile <<EOF
#%Module -*- tcl -*-
## Module for QEMU
## Automatically generated on ${NOW}

proc ModulesHelp {} {
  puts stderr "Setup QEMU ${QEMU_VER}" 
}

module-whatis "Setup QEMU ${QEMU_VER}"

prepend-path  PATH   ${PREFIX}/bin
EOF

mysudo "cp ${WORK_DIR}/${SRC_DIR}/modulefile ${MODULE_FILE}"
