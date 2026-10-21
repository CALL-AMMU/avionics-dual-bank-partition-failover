@echo off
echo =========================================================
echo   Compiling Avionics Dual-Bank Failover Engine
echo =========================================================
g++ -std=c++14 -Wall -Wextra -Iinclude src/md5.cpp src/partition_manager.cpp src/main.cpp -o avionics_failover.exe
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Compilation successful! Executable created: avionics_failover.exe
    echo Running simulation...
    echo.
    avionics_failover.exe
) else (
    echo.
    echo Compilation failed!
)
pause
