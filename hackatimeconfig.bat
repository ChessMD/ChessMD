@echo off
set WAKATIME_PATH=C:\Users\Daniel\.wakatime\wakatime-cli-windows-amd64.exe
set PROJECT_NAME=ChessMD
set API_KEY=249ba70e-a0e2-4c29-9377-62bc9520f0dd
set WATCH_DIR=C:\ChessMD

chokidar "%WATCH_DIR%/*.{cpp,h,ui,qml,js,py,txt}" -c ^
"%WAKATIME_PATH% --entity {path} --entity-type file --project %PROJECT_NAME% --plugin chokidar/1.0 --write --key %API_KEY%"
