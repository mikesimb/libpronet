@echo off
set THIS_DIR=%~sdp0

if exist %THIS_DIR%libs\armeabi          rmdir /s /q %THIS_DIR%libs\armeabi
if exist %THIS_DIR%libs\armeabi-v7a      rmdir /s /q %THIS_DIR%libs\armeabi-v7a
if exist %THIS_DIR%libs\x86              rmdir /s /q %THIS_DIR%libs\x86
if exist %THIS_DIR%libs\mips             rmdir /s /q %THIS_DIR%libs\mips
if exist %THIS_DIR%libs\arm64-v8a        rmdir /s /q %THIS_DIR%libs\arm64-v8a
if exist %THIS_DIR%libs\x86_64           rmdir /s /q %THIS_DIR%libs\x86_64
if exist %THIS_DIR%libs\mips64           rmdir /s /q %THIS_DIR%libs\mips64
if exist %THIS_DIR%obj\local\armeabi     rmdir /s /q %THIS_DIR%obj\local\armeabi
if exist %THIS_DIR%obj\local\armeabi-v7a rmdir /s /q %THIS_DIR%obj\local\armeabi-v7a
if exist %THIS_DIR%obj\local\x86         rmdir /s /q %THIS_DIR%obj\local\x86
if exist %THIS_DIR%obj\local\mips        rmdir /s /q %THIS_DIR%obj\local\mips
if exist %THIS_DIR%obj\local\arm64-v8a   rmdir /s /q %THIS_DIR%obj\local\arm64-v8a
if exist %THIS_DIR%obj\local\x86_64      rmdir /s /q %THIS_DIR%obj\local\x86_64
if exist %THIS_DIR%obj\local\mips64      rmdir /s /q %THIS_DIR%obj\local\mips64

@echo on
call ndk-build V=1 APP_ABI="armeabi armeabi-v7a x86 mips arm64-v8a x86_64 mips64"
@echo off

copy /y %THIS_DIR%obj\local\armeabi\libmbedtls.a      %THIS_DIR%libs\armeabi\
copy /y %THIS_DIR%obj\local\armeabi-v7a\libmbedtls.a  %THIS_DIR%libs\armeabi-v7a\
copy /y %THIS_DIR%obj\local\x86\libmbedtls.a          %THIS_DIR%libs\x86\
copy /y %THIS_DIR%obj\local\mips\libmbedtls.a         %THIS_DIR%libs\mips\
copy /y %THIS_DIR%obj\local\arm64-v8a\libmbedtls.a    %THIS_DIR%libs\arm64-v8a\
copy /y %THIS_DIR%obj\local\x86_64\libmbedtls.a       %THIS_DIR%libs\x86_64\
copy /y %THIS_DIR%obj\local\mips64\libmbedtls.a       %THIS_DIR%libs\mips64\

copy /y %THIS_DIR%obj\local\armeabi\libpro_util.a     %THIS_DIR%libs\armeabi\
copy /y %THIS_DIR%obj\local\armeabi-v7a\libpro_util.a %THIS_DIR%libs\armeabi-v7a\
copy /y %THIS_DIR%obj\local\x86\libpro_util.a         %THIS_DIR%libs\x86\
copy /y %THIS_DIR%obj\local\mips\libpro_util.a        %THIS_DIR%libs\mips\
copy /y %THIS_DIR%obj\local\arm64-v8a\libpro_util.a   %THIS_DIR%libs\arm64-v8a\
copy /y %THIS_DIR%obj\local\x86_64\libpro_util.a      %THIS_DIR%libs\x86_64\
copy /y %THIS_DIR%obj\local\mips64\libpro_util.a      %THIS_DIR%libs\mips64\

@echo on
pause
