@echo off
set SEAT_ID=%1
if "%SEAT_ID%"=="" set SEAT_ID=5

docker exec -it os-project bash -c "(echo -e 'RESERVE %SEAT_ID%\nQUIT' | ./client 1) & (echo -e 'RESERVE %SEAT_ID%\nQUIT' | ./client 2) & (echo -e 'RESERVE %SEAT_ID%\nQUIT' | ./client 3) & (echo -e 'RESERVE %SEAT_ID%\nQUIT' | ./client 4) & (echo -e 'RESERVE %SEAT_ID%\nSTATUS %SEAT_ID%\nQUIT' | ./client 5) & wait"