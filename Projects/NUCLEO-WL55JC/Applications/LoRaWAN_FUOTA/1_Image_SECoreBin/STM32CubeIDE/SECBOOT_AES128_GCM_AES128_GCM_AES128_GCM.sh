#!/bin/bash - 
#Post build for SECBOOT_AES128_GCM_WITH_AES128_GCM
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

sfu=$userAppBinary"/"$execname".sfu"
sfb=$userAppBinary"/"$execname".sfb"
sign=$userAppBinary"/"$execname".sign"
headerbin=$userAppBinary"/"$execname"sfuh.bin"
bigbinary=$userAppBinary"/BFU_"$execname".bin"

nonce=$SBSFUBootLoader"/1_Image_SECoreBin/Binary/nonce.bin"
magic="SFU"$fwid
offset=512
oemkey=$SBSFUBootLoader"/1_Image_SECoreBin/Binary/OEM_KEY_COMPANY"$fwid"_key_AES_GCM.bin"
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

command=$cmd" "$prepareimage" enc -k "$oemkey" -n "$nonce" "$bin" "$sfu
$command >> $projectdir"/output.txt"
ret=$?
if [ $ret -eq 0 ]; then
  command=$cmd" "$prepareimage" sign -k "$oemkey" -n "$nonce" "$bin" "$sign
  $command >> $projectdir"/output.txt"
  ret=$?
  if [ $ret -eq 0 ]; then 
    command=$cmd" "$prepareimage" pack -m "$magic" -k "$oemkey"  -r 112 -v "$version" -n "$nonce" -f "$sfu" -t "$sign" "$sfb" -o "$offset
    $command >> $projectdir"/output.txt"
    ret=$?
    if [ $ret -eq 0 ]; then
      command=$cmd" "$prepareimage" header -m "$magic" -k  "$oemkey" -r 112 -v "$version"  -n "$nonce" -f "$sfu" -t "$sign" -o "$offset" "$headerbin
      $command >> $projectdir"/output.txt"
      ret=$?
      if [ $ret -eq 0 ]; then
        command=$cmd" "$prepareimage" merge -v 0 -e 1 -i "$headerbin" -s "$sbsfuelf" -u "$elf" "$bigbinary
        $command >> $projectdir"/output.txt"
        ret=$?
      fi
    fi
  fi
fi

if [ $ret -eq 0 ]; then
  rm $sign
  rm $sfu
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
