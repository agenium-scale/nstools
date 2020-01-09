#!/bin/sh
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
WORK_DIR=tmp
TAR_GZ=patchelf-0.9.tar.gz
SRC_DIR=patchelf-0.9
PREFIX="/opt/local/patchelf/0.9"
MODULE_PATH="$(dirname $(module path null))/patchelf"
MODULE_FILE=${MODULE_PATH}/0.9

mkdir -p ${WORK_DIR}

# Delete previous attempts
rm -f ${WORK_DIR}/${TAR_GZ}
rm -rf ${WORK_DIR}/${SRC_DIR}

# Download and untar
(cd ${WORK_DIR} && curl -L https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz -o ${TAR_GZ})
(cd ${WORK_DIR} && tar xf ${TAR_GZ})

# Configure, compile and install
(cd ${WORK_DIR}/${SRC_DIR} && ./configure --prefix=${PREFIX})
(cd ${WORK_DIR}/${SRC_DIR} && make)
(cd ${WORK_DIR}/${SRC_DIR} && mysudo "make install")

# Create corresponding modulefile
mysudo mkdir -p ${MODULE_PATH}
NOW=$(date)

cat >${WORK_DIR}/${SRC_DIR}/modulefile <<EOF
#%Module -*- tcl -*-
## Module for patchelf
## Automatically generated on ${NOW}

proc ModulesHelp {} {
  puts stderr "Setup patchelf" 
}

module-whatis "Setup patchelf"

prepend-path  PATH             ${PREFIX}/bin
prepend-path  MANPATH          ${PREFIX}/share/man
EOF

mysudo "cp ${WORK_DIR}/${SRC_DIR}/modulefile ${MODULE_FILE}"
