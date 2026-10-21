@echo off
echo =========================================================
echo   Pushing Avionics Dual-Bank Failover to GitHub
echo =========================================================
echo.
cd /d "D:\Amit_Advanced_Projects\avionics-dual-bank-partition-failover"

echo Checking Git status...
git status
echo.

echo Staging all project files...
git add .
git commit -m "feat: initial release of Avionics Dual-Bank (A/B) Flash Failover & Partition Manager"
echo.

echo Pushing to GitHub (https://github.com/CALL-AMMU/avionics-dual-bank-partition-failover)...
echo [NOTE: Please ensure you have created the repository 'avionics-dual-bank-partition-failover' on github.com/new]
git push -u origin main

echo.
echo =========================================================
echo   Done!
echo =========================================================
pause
