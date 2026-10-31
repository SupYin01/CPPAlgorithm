@echo off
REM One-click build and test. Run in cmd or double-click.
REM   build.bat            test practice\red_black_tree.h  - default
REM   build.bat answer     test answer\red_black_tree_answer.h
REM Keep this file ASCII-only: cmd parses .bat with the ANSI codepage,
REM so UTF-8 Chinese comments would break parsing on Chinese Windows.
setlocal
cd /d %~dp0
if not exist build mkdir build

set MODE=%1
if "%MODE%"=="" set MODE=practice
set FLAGS=-Ipractice -Ianswer
set BIN=build\test_rb_practice.exe
if /I "%MODE%"=="answer" (
    set FLAGS=-DUSE_ANSWER -Ianswer
    set BIN=build\test_rb_answer.exe
)

echo ==^> Compiling %MODE% mode ...
g++ -std=c++17 -O2 -Wall -Wextra %FLAGS% tests\test_red_black_tree.cpp -o %BIN%
if errorlevel 1 (
    echo Build FAILED. Install g++ MinGW-w64 and add it to PATH. See README.md
    exit /b 1
)

echo ==^> Running tests. If Chinese text is garbled, run: chcp 65001
%BIN%
