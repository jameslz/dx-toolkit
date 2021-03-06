#!/bin/bash -e
#
# Copyright (C) 2013 DNAnexus, Inc.
#
# This file is part of dx-toolkit (DNAnexus platform client libraries).
#
#   Licensed under the Apache License, Version 2.0 (the "License"); you may not
#   use this file except in compliance with the License. You may obtain a copy
#   of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#   WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#   License for the specific language governing permissions and limitations
#   under the License.

ostype=$(uname)

# Hide any existing Python packages from the build process.
export PYTHONPATH=

source "$(dirname $0)/../environment"
cd "${DNANEXUS_HOME}"
make clean
make
rm Makefile
rm -r debian
mv build/Prebuilt-Readme.md Readme.md

# setuptools bakes the path of the Python interpreter into all installed Python scripts. Rewrite it back to the more
# portable form "/usr/bin/env python2.7", since we don't always know where the right interpreter is on the target
# system.
# Also, insert a stub that tries to detect when the user hasn't sourced the environment file and prints a warning.
py_header='#!/usr/bin/env python2.7
import os, sys
if "DNANEXUS_HOME" not in os.environ:
    sys.stderr.write("""***\n*** WARNING: DNANEXUS_HOME is not set. Please source the environment file at the root of dx-toolkit.\n***\n""")
'

for f in bin/*; do
    if head -n 1 "$f" | egrep -iq "(python|pypy)"; then
        perl -i -pe 's|^#!/.+|'"$py_header"'| if $. == 1' "$f"
    fi
done

if [[ "$ostype" == 'Linux' ]]; then
  osversion=$(lsb_release -c | sed s/Codename:.//)
  rhmajorversion=$(cat /etc/system-release | sed "s/^\(CentOS\) release \([0-9]\)\.[0-9]\+.*$/\2/")
  # TODO: detect versions that do and don't support mktemp --suffix more
  # reliably. What is known:
  #
  # Supports --suffix:
  #   Ubuntu 12.04
  #   CentOS 6
  #
  # Doesn't support --suffix:
  #   Ubuntu 10.04
  if [[ "$osversion" == 'precise' || "$rhmajorversion" == '6' ]]; then
    temp_archive=$(mktemp --suffix .tar.gz)
  else
    temp_archive=$(mktemp -t dx-toolkit.tar.gz.XXXXXXXXXX)
  fi
elif [[ "$ostype" == 'Darwin' ]]; then # Mac OS
  temp_archive=$(mktemp -t dx-toolkit.tar.gz)
else
  echo "Unsupported OS $ostype"
  exit 1
fi

# TODO: what if the checkout is not named dx-toolkit? The tar commands
# below will fail.
if [[ "$ostype" == 'Linux' ]]; then
  cd "${DNANEXUS_HOME}/.."
  tar --exclude-vcs -czf $temp_archive dx-toolkit
elif [[ "$ostype" == 'Darwin' ]]; then # Mac OS

  # BSD tar has no --exclude-vcs, so we do the same thing ourselves in a
  # temp dir.
  cd "${DNANEXUS_HOME}/.."
  tempdir=$(mktemp -d -t dx-packaging-workdir)
  cp -a dx-toolkit $tempdir
  cd $tempdir
  rm -rf dx-toolkit/.git
  tar -czf $temp_archive dx-toolkit
fi

cd "${DNANEXUS_HOME}"

if [[ "$ostype" == 'Linux' ]]; then
  dest_tarball="${DNANEXUS_HOME}/dx-toolkit-$(git describe).tar.gz"
elif [[ "$ostype" == 'Darwin' ]]; then # Mac OS
  dest_tarball="${DNANEXUS_HOME}/dx-toolkit-$(git describe)-osx.tar.gz"
fi

mv $temp_archive $dest_tarball
if [[ "$ostype" == 'Darwin' ]]; then
  rm -rf $tempdir
fi

chmod 664 "${dest_tarball}"
echo "---"
echo "--- Package in ${dest_tarball}"
echo "---"
