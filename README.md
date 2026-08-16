# Minikv

一个使用 C++17 编写的简化版持久化键值存储引擎，用于学习 WAL、MemTable、SSTable 和日志结构合并树的基本工作方式。

项目采用单进程、单线程设计，重点放在存储流程和代码可读性，不追求完整数据库产品的工程复杂度。

## 项目简介

Minikv 对外提供简单的键值操作接口和交互式命令行程序：

- 写入和删除操作先追加到 WAL 日志。
- 数据暂存在内存表中，达到阈值后写入 SST 文件。
- 查询优先访问内存表，再从新到旧查询 SST 文件。
- 程序重新启动时自动回放 WAL，并扫描已有 SST 文件。
- 测试程序覆盖基础操作、日志恢复、刷盘、版本覆盖和重启读取。

## 项目状态与后续拓展计划

| 状态 | 事项 | 当前情况 |
| --- | --- | --- |
| 已完成 | 基础键值操作 | 支持写入、查询、更新和删除 |
| 已完成 | 统一接口 | 提供 `Set`、`Get`、`Delete` 和 `Status` |
| 已完成 | WAL 日志 | 支持 `SET`、`DEL` 日志追加和启动回放 |
| 已完成 | MemTable | 使用 `unordered_map` 保存当前可写数据 |
| 已完成 | 自动刷盘 | MemTable 新增 5 个键后生成 SST 文件 |
| 已完成 | SST 查询 | 支持从内存表和多个 SST 文件查询 |
| 已完成 | 版本覆盖 | 查询时优先返回较新的 SST 文件中的值 |
| 已完成 | 交互式命令行 | 支持 `SET`、`GET`、`DEL`、`HELP`、`EXIT` |
| 已完成 | 自动化测试 | 5 个场景：基础操作、WAL 回放、刷盘、版本覆盖、重启读取 |
| 部分完成 | 持久化安全 | 有日志刷新，但没有 `fsync` 和校验和 |
| 部分完成 | SST 文件管理 | 可以生成和读取 SST，但没有目录索引和元数据清单 |
| 未完成 | 删除标记 | 增加 Tombstone，解决已落盘键无法正确删除的问题 |
| 未完成 | 文件压缩合并 | 实现 Compaction，合并 SST 并清理旧版本 |
| 未完成 | SST 索引 | 查询仍然是顺序扫描，没有稀疏索引或块索引 |
| 未完成 | 并发访问 | 当前没有读写锁和多线程访问控制 |
| 未完成 | 日志压缩 | WAL 会持续追加，尚未实现日志合并 |
| 未完成 | 重启后的文件序号管理 | 避免新生成的 SST 文件覆盖已有文件 |
| 未完成 | 异常恢复测试 | 增加半截日志、异常退出和并发读写测试 |

## 运行与使用

### 启动流程

程序启动时按以下顺序工作：

```text
打开 Minikv
    ↓
回放 WAL，恢复内存表
    ↓
扫描 sst_*.sst，加载文件列表
    ↓
以追加模式打开 WAL
    ↓
等待命令行输入
```

### 写入与刷盘流程

```text
SET / DEL
    ↓
追加写入 WAL
    ↓
更新 MemTable
    ↓
新增键数量达到 5 个
    ↓
写入 SST 文件并清空 MemTable
    ↓
删除旧 WAL，创建新的空 WAL
```

### 查询流程

```text
GET key
    ↓
查询 MemTable
    ↓ 未找到
从最新 SST 到最旧 SST 依次查询
    ↓
返回找到的第一个结果
```

### 命令行使用

启动 `minikv` 后，在 `minikv>` 提示符下输入：

| 命令 | 用途 | 示例 |
| --- | --- | --- |
| `SET key value` | 写入或更新键值 | `SET name Alice` |
| `GET key` | 查询键值 | `GET name` |
| `DEL key` | 删除键 | `DEL name` |
| `HELP` | 查看帮助 | `HELP` |
| `EXIT` / `QUIT` | 退出程序 | `EXIT` |

示例：

```text
minikv> SET name Alice
OK
minikv> GET name
Alice
minikv> DEL name
OK
minikv> GET name
(nil)
minikv> EXIT
Bye.
```

当前 WAL 使用 `|` 分隔日志字段，因此键和值不能包含 `|`。运行时生成的 `minikv.log`、`sst_*.sst` 等文件会保存到当前工作目录。

## 目录结构

```text
Minikv/
├── inc/
│   ├── minikv.h        # 对外接口和内部成员声明
│   └── utils.h         # 字符串分割函数声明
├── src/
│   ├── minikv.cpp      # WAL、MemTable、SSTable 和查询实现
│   └── utils.cpp       # 字符串分割函数实现
├── cli_main.cpp        # 命令行程序入口
├── minikv_test.cpp     # 自动化测试入口
├── CMakeLists.txt      # 构建配置
├── README.md           # 项目说明
└── .gitignore          # 忽略构建和运行生成文件
```

## 代码关系

```text
cli_main.cpp ─────┐
                  ├──> Minikv::Set / Get / Delete
minikv_test.cpp ──┘              │
                                 ├──> WAL
                                 ├──> MemTable
                                 └──> SST 文件
```

## Build Guide
### Windows（MSVC）

环境要求：

- CMake 3.16 或更高版本
- Visual Studio 或安装了“使用 C++ 的桌面开发”组件的 Visual Studio Build Tools

在项目根目录打开 PowerShell，执行：

```powershell
cmake -S . -B build-windows
cmake --build build-windows --config Release --parallel
```

运行 CLI：

```powershell
.\build-windows\Release\minikv.exe
```

运行单元测试：

```powershell
.\build-windows\Release\minikv_test.exe
```

### Linux

环境要求：

```bash
sudo apt update
sudo apt install -y build-essential cmake
```

在项目根目录执行：

```bash
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux -j$(nproc)
```

运行 CLI：

```bash
./build-linux/minikv
```

运行单元测试：

```bash
./build-linux/minikv_test
```

## 许可证

项目许可证见仓库根目录下的 `LICENSE` 文件。
