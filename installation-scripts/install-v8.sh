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
V8_VER="${1}"
PREFIX="/opt/local/v8/${V8_VER}"
MODULE_PATH="$(dirname $(module path null))/v8"
MODULE_FILE=${MODULE_PATH}/${V8_VER}
WORK_DIR="${PWD}/tmp"
J="-j$(nproc)"
mkdir -p ${WORK_DIR}

# We need gn for building v8 but as gn is written in C++17 it may not compile
# everywhere
(cd ${WORK_DIR} && \
 git clone https://gn.googlesource.com/gn && \
 cd gn && \
 python3 build/gen.py && \
 ninja -C out)
export PATH=${PATH}:${WORK_DIR}/gn/out

# We also need gclient from the Chromium project
(cd ${WORK_DIR} && \
 git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git)
export PATH=${PATH}:${WORK_DIR}/depot_tools

# Fetch v8
(cd ${WORK_DIR} && rm -rf v8 && fetch v8)

# Compile v8
(cd ${WORK_DIR}/v8 && python3 tools/dev/gm.py x64.release-d8)

# Install v8 (manually for now as I did not find how to it with gm.py)
mysudo mkdir -p ${PREFIX}/bin
(cd ${WORK_DIR}/v8/out/x64.release-d8 && \
 mysudo cp icudtl.dat snapshot_blob.bin ${PREFIX}/bin && \
 mysudo cp d8 ${PREFIX}/bin/real_d8)
cat >${WORK_DIR}/v8/d8 <<-EOF
	#!/bin/sh
	${PREFIX}/bin/real_d8 \\
	    --snapshot_blob=${PREFIX}/bin/snapshot_blob.bin \\
	    --icu-data-file=${PREFIX}/bin/icudtl.dat "\$@"
	EOF
mysudo cp ${WORK_DIR}/v8/d8 ${PREFIX}/bin/d8
mysudo chmod a+x ${PREFIX}/bin/d8

# Create corresponding modulefile
mysudo mkdir -p ${MODULE_PATH}
NOW=$(date)

cat >${WORK_DIR}/v8/modulefile <<EOF
#%Module -*- tcl -*-
## Module for V8
## Automatically generated on ${NOW}

proc ModulesHelp {} {
  puts stderr "Setup V8 ${V8_VER}" 
}

module-whatis "Setup V8 ${V8_VER}"

prepend-path  PATH               ${PREFIX}/bin
EOF

mysudo "cp ${WORK_DIR}/v8/modulefile ${MODULE_FILE}"

