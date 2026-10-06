@echo off

docker rm -f os-project >nul 2>&1

docker run -it --name os-project -v "%cd%":/app os-env bash -c "make clean && make && ./server"