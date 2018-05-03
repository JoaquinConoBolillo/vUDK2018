@echo on
cd %WORKSPACE%
copy msvcrTD.dll %WORKSPACE%\Build\NT32IA32\DEBUG_VS2013x86\IA32
cd %WORKSPACE%\Build\NT32IA32\DEBUG_VS2013x86\IA32
start secmain.exe /debug
