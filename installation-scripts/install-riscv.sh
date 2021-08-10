#!/bin/sh -e
# Copyright (c) 2021 Agenium Scale
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
PREFIX="/opt/local/riscv"
NUM=1
while true; do
  if [ -d "${PREFIX}/${NUM}" ]; then
    NUM=`expr ${NUM} + 1`
  else
    PREFIX="${PREFIX}/${NUM}"
    break
  fi
done
MODULE_PATH="$(dirname $(module path null))/riscv"
MODULE_FILE=${MODULE_PATH}/${NUM}
GCC=riscv-gnu-toolchain
SPIKE=riscv-isa-sim
PK=riscv-pk
WORK_DIR="${PWD}/tmp"
J="-j$(nproc)"
mkdir -p ${WORK_DIR}

# Delete previsous attempts
rm -rf "${WORK_DIR}/${GCC}"
rm -rf "${WORK_DIR}/${SPIKE}"
rm -rf "${WORK_DIR}/${PK}"

# Download what is necessary: no releases so git clone
(cd "${WORK_DIR}" && git clone https://github.com/riscv/${GCC} ${GCC})
(cd "${WORK_DIR}/${GCC}" && git checkout rvv-intrinsic)
(cd "${WORK_DIR}" && git clone https://github.com/riscv/${SPIKE} ${SPIKE})
(cd "${WORK_DIR}" && git clone https://github.com/riscv/${PK} ${PK})

# Build GCC first
mysudo mkdir -p "${PREFIX}"
mysudo chown -R ${USER}:${USER} "${PREFIX}"
(cd "${WORK_DIR}/${GCC}" \
 && ./configure --prefix="${PREFIX}" --with-arch=rv64gcv_zfh \
 && make)
mysudo chown -R root:root ${PREFIX}

# Then build and install spike
(cd "${WORK_DIR}/${SPIKE}" \
 && mkdir build \
 && cd build \
 && ../configure --prefix="${PREFIX}" \
 && make ${J} \
 && mysudo make install)

# And finally pk
(cd "${WORK_DIR}/${PK}" \
 && mkdir build \
 && cd build \
 && ../configure --prefix=${PREFIX} --host=riscv64-unknown-elf \
 && make ${J} \
 && mysudo make install)

# Create corresponding modulefile
mysudo mkdir -p ${MODULE_PATH}
NOW=$(date)

cat >${WORK_DIR}/modulefile <<EOF
#%Module -*- tcl -*-
## Module for RISC-V cross development suite
## Automatically generated on ${NOW}

proc ModulesHelp {} {
  puts stderr "Setup RISC-V cross development suite #${NUM}"
}

module-whatis "Setup RISC-V cross development suite #${NUM}"

prepend-path  PATH               ${PREFIX}/bin
prepend-path  MANPATH            ${PREFIX}/share/man
prepend-path  LD_LIBRARY_PATH    ${PREFIX}/lib
EOF

mysudo "cp ${WORK_DIR}/modulefile ${MODULE_FILE}"

