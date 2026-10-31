#!/usr/bin/env bash
# 一键编译并运行红黑树测试（Git Bash / Linux / macOS 通用）
# 用法:
#   ./run_tests.sh          测练习版 practice/red_black_tree.h（默认）
#   ./run_tests.sh answer   测参考答案 answer/red_black_tree_answer.h
set -e
cd "$(dirname "$0")"
mkdir -p build

MODE="${1:-practice}"
if [ "$MODE" = "answer" ]; then
  FLAGS="-DUSE_ANSWER -Ianswer"
  BIN=build/test_rb_answer
else
  FLAGS="-Ipractice -Ianswer"
  BIN=build/test_rb_practice
fi

echo "==> 编译（$MODE 模式）..."
g++ -std=c++17 -O2 -Wall -Wextra $FLAGS tests/test_red_black_tree.cpp -o "$BIN"

echo "==> 运行测试..."
"$BIN"
