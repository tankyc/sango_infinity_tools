@echo off
set ndkPath=D:/android-ndk-r15c
cd ./android
call %ndkPath%/ndk-build clean APP_ABI="armeabi-v7a,x86,arm64-v8a"
call %ndkPath%/ndk-build APP_ABI="armeabi-v7a"
copy libs/armeabi-v7a/libtolua.so ../Plugins/Android/libs/armeabi-v7a
call %ndkPath%/ndk-build clean APP_ABI="armeabi-v7a,x86,arm64-v8a"
echo Successfully linked
exit