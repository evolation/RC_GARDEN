::Post build for SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256
:: arg1 is the build directory
:: arg2 is the elf file path+name
:: arg3 is the bin file path+name
:: arg4 is the firmware Id (1/2/3)
:: arg5 is the version
@echo off
set "projectdir=%1"
set "execname=%~n3"
set "elf=%2"
set "bin=%3"
set "fwid=%4"
set "version=%5"

set "SBSFUBootLoader=%~d0%~p0\\..\\.."
set "userAppBinary=%projectdir%\\Binary"

set "sfb=%userAppBinary%\\%execname%.sfb"
set "sign=%userAppBinary%\\%execname%.sign"
set "headerbin=%userAppBinary%\\%execname%sfuh.bin"
set "bigbinary=%userAppBinary%\\BFU_%execname%.bin"

set "oemkey="
set "ecckey=%SBSFUBootLoader%\\1_Image_SECoreBin\\Binary\\ECCKEY%fwid%.txt"
set "sbsfuelf=%SBSFUBootLoader%\\1_Image_BFU\\MDK-ARM\\STM32WL55JC_Nucleo\\Exe\\BFU.axf"
set "magic=SFU%fwid%"
set "offset=512"

::comment this line to force python
::python is used if windows executable not found
pushd %projectdir%\..\..\..\..\..\..\Middlewares\ST\STM32_Secure_Engine\Utilities\KeysAndImages
set basedir=%cd%
popd
goto exe:
goto py:
:exe
::line for window executable
echo Postbuild with windows executable
set "prepareimage=%basedir%\\win\\prepareimage\\prepareimage.exe"
set "python="
if exist %prepareimage% (
goto postbuild
)
:py
::line for python
echo Postbuild with python script
set "prepareimage=%basedir%\\prepareimage.py"
set "python=python "
:postbuild
echo "%python%%prepareimage%" > %projectdir%\\output.txt

::Make sure we have a Binary sub-folder in UserApp folder
if not exist "%userAppBinary%" (
mkdir "%userAppBinary%" >> %projectdir%\\output.txt 2>&1
IF %ERRORLEVEL% NEQ 0 goto :error
)

:: no encryption
set "command=%python%%prepareimage% sha256 %bin% %sign% >> %projectdir%\output.txt 2>&1"
%command%
IF %ERRORLEVEL% NEQ 0 goto :error

:: no encryption so pack the binary file
set "command=%python%%prepareimage% pack -m %magic% -k %ecckey% -p 1 -r 44 -v %version% -f %bin% -t %sign% %sfb% -o %offset% >> %projectdir%\output.txt 2>&1"
%command%
IF %ERRORLEVEL% NEQ 0 goto :error
set "command=%python%%prepareimage% header -m %magic% -k %ecckey% -p 1 -r 44 -v %version% -f %bin% -t %sign%  -o %offset% %headerbin% >> %projectdir%\output.txt 2>&1"
%command%
IF %ERRORLEVEL% NEQ 0 goto :error

set "command=%python%%prepareimage% merge -i %headerbin% -s %sbsfuelf% -u %elf% %bigbinary% >> %projectdir%\output.txt 2>&1"
%command%
IF %ERRORLEVEL% NEQ 0 goto :error

::backup and clean up the intermediate file
del %sign%
del %headerbin%

exit 0

:error
echo "%command% : failed" >> %projectdir%\\output.txt
:: remove the elf to force the regeneration
if exist %elf%(
  del %elf%
)
echo %command% : failed

pause
exit 1

:nothingtodo
exit 0
