S3 Version

Make sure that the go server is running and has the AWS credentials

setup sysmon using this: https://learn.microsoft.com/en-us/sysinternals/downloads/sysmon

You will need a .env in the same directory as the executable structured like this:
```
FIM_API_URL=http://localhost:1514/api/logs/upload
FIM_API_TOKEN=<JWT from /auth/login>
```

Make sure that the yaml.dll is in the same directory.

When you run `.\fim_sender.exe`, make sure that you are running it from an ADMIN powershell otherwise it won't have sufficient permission to view Sysmon logs.

Compile command:
```powershell
cl /nologo /EHsc /std:c++17 /I "[VCPKG_PATH]\installed\x64-windows\include" fim\windows_event_sender.cpp /DFIM_WEVT_STANDALONE /link /LIBPATH:"[VCPKG_PATH]\installed\x64-windows\lib" yaml-cpp.lib wevtapi.lib /out:fim_sender.exe
```