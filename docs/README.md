# Titan-X 首版说明

## 最小依赖

- Linux
- gcc
- make

## 启动方式

```bash
make
make run
```

## 核心功能范围（首版）

当前仅保留高性能、低复杂度的核心能力：

- 输入循环（REPL）
- 命令分发
- 退出命令：`exit`

默认提示符为 `TitanX>`，输入 `exit` 即可退出。
