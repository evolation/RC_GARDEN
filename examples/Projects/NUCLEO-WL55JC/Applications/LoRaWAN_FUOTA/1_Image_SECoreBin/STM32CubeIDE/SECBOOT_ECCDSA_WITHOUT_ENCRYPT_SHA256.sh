#!/bin/bash - 
#Post build for SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256
# arg1 is the build directory
# arg2 is the elf file path+name
# arg3 is the bin file path+name
# arg4 is the firmware Id (1/2/3)
# arg5 is the version
projectdir=$1
FileName=${3##*/}
execname=${FileName%.*}
elf=$2
bin=$3
fwid=$4
version=$5

SBSFUBootLoader=$projectdir"/../.."
userAppBinary=$projectdir"/Binary"

sfb=$userAppBinary"/"$execname".sfb"
sign=$userAppBinary"/"$execname".sign"
headerbin=$userAppBinary"/"$execname"sfuh.bin"
bigbinary=$userAppBinary"/BFU_"$execname".bin"

magic="SFU"$fwid
offset=512
ecckey=$SBSFUBootLoader"/1_Image_SECoreBin/Binary/ECCKEY"$fwid".txt"
sbsfuelf=$SBSFUBootLoader"/1_Image_BFU/STM32CubeIDE/Debug/BFU.elf"

current_directory=`pwd`
cd $1/../../../../../../Middlewares/ST/STM32_Secure_Engine/Utilities/KeysAndImages
basedir=`pwd`
cd $current_directory
# test if window executable usable
prepareimage=$basedir"/win/prepareimage/prepareimage.exe"
uname | grep -i -e windows -e mingw > /dev/null 2>&1

if [ $? -eq 0 ] && [ -e "$prepareimage" ]; then
  echo "prepareimage with windows executable"
  export PATH=$basedir"\win\prepareimage";$PATH > /dev/null 2>&1
  cmd=""
  prepareimage="prepareimage.exe"
else
  # line for python
  echo "prepareimage with python script"
  prepareimage=$basedir/prepareimage.py
  cmd="python"
fi
echo "$cmd $prepareimage" > $projectdir"/output.txt"

# Make sure we have a Binary sub-folder in UserApp folder
if [ ! -e $userAppBinary ]; then
mkdir $userAppBinary
fi

command=$cmd" "$prepareimage" sha256 "$bin" "$sign
$command >> $projectdir"/output.txt"
ret=$?
if [ $ret -eq 0 ]; then 
  command=$cmd" "$prepareimage" pack -m "$magic" -k "$ecckey" -p 1 -r 44 -v "$version" -f "$bin" -t "$sign" "$sfb" -o "$offset
  $command >> $projectdir"/output.txt"
  ret=$?
  if [ $ret -eq 0 ]; then
    command=$cmd" "$prepareimage" header -m "$magic" -k  "$ecckey"  -p 1 -r 44 -v "$version"  -f "$bin" -t "$sign" -o "$offset" "$headerbin
    $command >> $projectdir"/output.txt"
    ret=$?
    if [ $ret -eq 0 ]; then
      command=$cmd" "$prepareimage" merge -v 0 -e 1 -i "$headerbin" -s "$sbsfuelf" -u "$elf" "$bigbinary
      $command >> $projectdir"/output.txt"
      ret=$?
    fi
  fi
fi

if [ $ret -eq 0 ]; then
  rm $sign
  rm $headerbin
  exit 0
else 
  echo "$command : failed" >> $projectdir"/output.txt"
  if [ -e  "$elf" ]; then
    rm  $elf
  fi
  echo $command : failed
  read -n 1 -s
  exit 1
fi
