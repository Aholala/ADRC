@echo off
set src=ecf_configList.h.example
set dst=ecf_configList.h

if not exist %src% (
    echo example file  %src% not found
    pause
    exit /b 1
)

echo coping %src% to %dst% ...
copy /Y %src% %dst% >nul

echo successful!
pause
