#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd "${DIR}"
rm -rf build
mkdir build
cd build
cmake -G Xcode ..
read -n1 -r -p "Press any key to continue..." key