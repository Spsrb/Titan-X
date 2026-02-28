# Titan-X 1.0

Titan-X 是一个轻量级命令执行器（command runner），支持：
- 执行外部命令（如 /bin，/echo、ls）。
- 内建命令（cd、pwd、exit）。
- 基于 `PATH` 的命令路径查找与缓存（提升重复执行效率）。

---

## 1. 环境要求

- Linux（推荐 x86_64）
- C 编译器（例如 GCC）
- `make`
- 常见系统工具：`tar`、`ldd`、`readelf`

---

## 2. 快速构建

在仓库根目录执行：

```bash
make clean all
```

构建成功后会生成可执行文件：

- `./titanx`

---

## 3. TitanX 1.0 打包说明

### 3.1 一键打包（推荐）

```bash
make package
```

该命令会调用 `scripts/package.sh 1.0`，完成以下动作：
1. 重新编译项目（`make clean all`）。
2. 在 `dist/` 下组装发布目录。
3. 生成压缩包：

- `dist/TitanX1.0-linux-x86_64.tar.gz`

> 注意：`dist/` 为本地产物目录，已在 `.gitignore` 中忽略，不提交二进制发布包到仓库。

### 3.2 自定义版本打包

如需自定义版本号：

```bash
./scripts/package.sh 1.1
```

会生成：

- `dist/TitanX1.1-linux-x86_64.tar.gz`

---

## 4. 功能展示（示例）

### 4.1 执行外部命令

```bash
./titanx /bin/echo "Hello TitanX"
```

预期输出：

```text
Hello TitanX
```

### 4.2 执行内建命令 `pwd`

```bash
./titanx pwd
```

预期输出：当前工作目录绝对路径。

### 4.3 执行内建命令 `cd`

```bash
./titanx cd /tmp
```

说明：这是在 TitanX 进程内部切换目录，命令结束后不会影响你当前 shell 的工作目录（与子进程模型一致）。

---

## 5. Linux 可用性检查（建议）

可按以下步骤验证 1.0 是否可在 Linux 运行：

```bash
make clean all
./titanx /bin/echo TitanX1.0
./titanx pwd
ldd ./titanx
readelf -h ./titanx | head
```

检查点：
- 能成功构建并执行基本命令。
- `ldd` 能正常解析依赖（如 `libc.so.6`）。
- `readelf` 显示 ELF 头信息，确认为 Linux 可执行文件格式。

---

## 6. 项目目录（核心）

- `src/main.c`：入口与参数处理。
- `src/exec.c`：命令执行逻辑、内建命令、PATH 缓存。
- `scripts/package.sh`：打包脚本。
- `docs/linux-compatibility-1.0.md`：Linux 可用性审查记录。

---

## 7. 常见问题

### Q1：为什么 PR 里看不到 `dist/*.tar.gz`？
A：仓库策略为“源码入库、产物本地生成”，避免提交二进制文件，便于代码审查和版本管理。

### Q2：`make package` 后哪里拿包？
A：在本地目录：`dist/TitanX1.0-linux-x86_64.tar.gz`。
