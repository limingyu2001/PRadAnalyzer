#!/bin/bash
# You will need root and evio to build the program
# Setup the path for these dependencies first

source /home/liyuan/software/root/install/bin/thisroot.sh

# get directory of this script
SOURCE="${BASH_SOURCE[0]}"
# resolve $SOURCE until the file is no longer a symlink
while [ -h "$SOURCE" ]; do
    DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"
    SOURCE="$(readlink "$SOURCE")"
    # if $SOURCE was a relative symlink, we need to resolve it relative to the path where the symlink file was located
    [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" >/dev/null 2>&1 && pwd )"

export PRAD_PATH=$(dirname "$DIR")

if [ `uname -m` == 'x86_64' ]; then
    export THIRD_LIB=$PRAD_PATH/thirdparty/lib64
else
    export THIRD_LIB=$PRAD_PATH/thirdparty/lib
fi

#export ET_LIB=/home/liyuan/PRad/coda3.06/Linux-x86_64/lib
#export ET_INC=/home/liyuan/PRad/coda3.06/Linux-x86_64/include

export ET_ROOT=/home/liyuan/software/et/et
export ET_INC=$ET_ROOT/src/libsrc
export ET_LIB=$ET_ROOT/build/lib

#export ET_LIB=$THIRD_LIB
#export ET_INC=$PWD/thirdparty/include

PRAD_LIB=$PRAD_PATH/lib
PRAD_INC=$PRAD_PATH/include

export LD_LIBRARY_PATH=$PRAD_LIB:$THIRD_LIB:$ET_LIB:$LD_LIBRARY_PATH

export QT_QPA_PLATFORM=xcb

export SESSION=temp01
