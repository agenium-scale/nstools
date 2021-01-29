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
BINARYEN_VER=$1
BINARYEN_URL="https://github.com/WebAssembly/binaryen/archive/version_${BINARYEN_VER}.tar.gz"
EMSCRIPTEN_VER=$2
EMSCRIPTEN_URL="https://github.com/emscripten-core/emscripten/archive/${EMSCRIPTEN_VER}.tar.gz"
PREFIX="/opt/local/wasm/${EMSCRIPTEN_VER}"
MODULE_PATH="$(dirname $(module path null))/wasm"
MODULE_FILE=${MODULE_PATH}/${EMSCRIPTEN_VER}
WORK_DIR="${PWD}/tmp"
J="-j$(nproc)"
mkdir -p ${WORK_DIR}

# Delete previsous attempts
rm -f "${WORK_DIR}/${EMSCRIPTEN_VER}.tar.gz"
rm -f "${WORK_DIR}/version_${BINARYEN_VER}.tar.gz"

# Download what is necessary
(cd ${WORK_DIR} && curl -L ${BINARYEN_URL} -o binaryen-version_${BINARYEN_VER}.tar.gz)
(cd ${WORK_DIR} && curl -L ${EMSCRIPTEN_URL} -o emscripten-${EMSCRIPTEN_VER}.tar.gz)

# Decompress
(cd ${WORK_DIR} && tar xf emscripten-${EMSCRIPTEN_VER}.tar.gz)
(cd ${WORK_DIR} && tar xf binaryen-version_${BINARYEN_VER}.tar.gz)

# Build binaryen first
BUILD_DIR="${WORK_DIR}/binaryen-version_${BINARYEN_VER}/build"
mkdir -p "${BUILD_DIR}"
(cd "${BUILD_DIR}" && cmake .. \
                      -DCMAKE_INSTALL_PREFIX=${PREFIX} \
                      -DCMAKE_C_COMPILER=gcc \
                      -DCMAKE_CXX_COMPILER=g++ \
                      -DCMAKE_BUILD_TYPE=Release)
(cd "${BUILD_DIR}" && make ${J} && mysudo make install)

# Then install emscripten
(cd "${WORK_DIR}/emscripten-${EMSCRIPTEN_VER}" && \
 mkdir -p "${PREFIX}/bin" && \
 mysudo cp -rv * "${PREFIX}/bin")

# Some JS modules are necessary
(cd ${PREFIX}/bin && mysudo npm instlal acorn)

# Create corresponding modulefile
mysudo mkdir -p ${MODULE_PATH}
NOW=$(date)

cat >${WORK_DIR}/${LLVM_PROJECT_DIR}/modulefile <<EOF
#%Module -*- tcl -*-
## Module for Wasm
## Automatically generated on ${NOW}

proc ModulesHelp {} {
  puts stderr "Setup Wasm ${EMSCRIPTEN_VER}" 
}

module-whatis "Setup Wasm ${EMSCRIPTEN_VER}"

prepend-path  PATH               ${PREFIX}/bin
prepend-path  LD_LIBRARY_PATH    ${PREFIX}/lib
EOF

mysudo "cp ${WORK_DIR}/${LLVM_PROJECT_DIR}/modulefile ${MODULE_FILE}"

