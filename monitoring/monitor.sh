#!/bin/bash
#
# Copyright (c) 2021 Agenium Scale
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# -----------------------------------------------------------------------------
# init

set -e

SSH="ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
         -o LogLevel=error"
SCP="scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
         -o LogLevel=error"

THRESHOLD=75

SCRIPT_NAME=`basename ${0}`

# -----------------------------------------------------------------------------
# Do the monitoring, for now we only need RAM and disk usage
# $1 = threshold for warning for RAM
# $2 = threshold for warning for disk

monitor() {
  ret=""

  # We first get RAM usage
  total=`free -b | grep Mem: | sed 's/  */ /g' | cut -d' ' -f2`
  used=`free -b | grep Mem: | sed 's/  */ /g' | cut -d' ' -f3`
  pcent=`expr ${used}00 / ${total} || true`

  if [ ${pcent} -ge ${1} ]; then
    ret="`printf '%3s' ${pcent}`"
  else
    ret="..."
  fi

  # Then disk usage
  list=`mount -l -t ext4 | cut -d' ' -f3 | grep -v boot`
  mp=""
  for m in ${list}; do
    pcent=`df --output=pcent ${m} | grep -v Use | sed 's/[ %]//g'`
    ret="${ret} "
    if [ ${pcent} -ge ${2} ]; then
      ret="${ret}`printf '%3s' ${pcent}`"
      mp="${mp}|${m}"
    else
      ret="${ret}..."
    fi
  done
  mp=`echo ${mp} | sed -e 's/^|/  /g' -e 's/|/, /g'`

  # Return result
  echo "${ret}${mp}"
}

# -----------------------------------------------------------------------------
# Test access to the machine: 1. host 2. ping 3. ssh
# $1 = url to server

network_test() {
  # First try DNS
  ret=`host ${1} 1>/dev/null 2>/dev/null && echo ... || echo DNS`

  # Then test ping
  ret="${ret} `ping -4 -c 4 ${1} 1>/dev/null 2>/dev/null \
               && echo .... || echo PING`"

  # Then ssh
  ret="${ret} `nc -z -4 -v ${1} 22 1>/dev/null 2>/dev/null \
               && echo ... || echo SSH`"

  echo ${ret}
}

# -----------------------------------------------------------------------------
# Branch here

if [ "${1}" == "do_it" ]; then
  monitor ${THRESHOLD} ${THRESHOLD}
  exit
fi

# -----------------------------------------------------------------------------
# Read configuration file

if [ "${1}" == "" ]; then
  CONF_FILE="machines.txt"
else
  CONF_FILE="${1}"
fi

LIST=`cat ${CONF_FILE}`

# -----------------------------------------------------------------------------
# Before monitoring we have to copy this script on all the machines

for m in ${LIST}; do
  echo "Copying script to ${m}..."
  ${SCP} ${0} ${m}:
done

# -----------------------------------------------------------------------------
# Now do the monitoring

echo2() {
  printf "%-${COLUMNS}s" " "
  printf "\r"
  echo "${1}"
}

rm -rf _monitoring
mkdir _monitoring

clear
while true; do
  for m in ${LIST}; do
    (
      STATUS_NET=`network_test ${m}`
      STATUS_MON=`${SSH} ${m} bash ${SCRIPT_NAME} do_it || echo ""`
      printf '%-30s ' ${m} >_monitoring/${m}.txt
      echo "${STATUS_NET}    ${STATUS_MON}" >>_monitoring/${m}.txt
    ) </dev/null &
  done
  wait
  tput cup 0 0
  echo2 "`printf '%-30s' 'Hostname'` DNS PING SSH    RAM DISK"
  echo2
  for m in ${LIST}; do
    echo2 "`cat _monitoring/${m}.txt`"
  done
  sleep 5
done
