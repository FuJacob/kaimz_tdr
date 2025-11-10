Make sure to configure `fim_config.yml` correctly by changing `[Username]`

Execute in powershell ran as administrator:
```
./windows_event_sender_local.exe
```

Compiled in Developer Command Prompt in VS 2020

Need to install vcpkg to install yaml-cpp and aws-sdk-cpp
```powershell
vcpkg install yaml-cpp:x64-windows
vcpkg install aws-sdk-cpp[s3,core]:x64-windows


cl /nologo /EHsc /std:c++17 /I "[VCPKG_PATH]\installed\x64-windows\include" fim\windows_event_sender_local.cpp /DFIM_WEVT_STANDALONE /link /LIBPATH:"[VCPKG_PATH]\installed\x64-windows\lib" yaml-cpp.lib wevtapi.lib /out:windows_event_sender_local.exe
```

When executing, you need to be in the directory where `fim_config.yml` is.

