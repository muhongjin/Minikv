# Minikv

一个用 C++17 写的简化键值存储练习项目。当前版本支持内存表、WAL 和简单 SST 文件，重点是把持久化流程跑通。

## 当前进度

- 已完成：内存表、`Set`、`Get`、`Delete`、状态返回、WAL 回放、SST 写入和 SST 查询
- 部分完成：启动时会扫描连续的 SST 文件，新文件优先返回结果
- 已完成：交互式命令行的 `SET`、`GET`、`DEL`、`HELP` 和 `EXIT`
- 部分完成：自动化测试覆盖基础 CRUD、WAL 回放、阈值刷盘、版本覆盖和重启读取

## WAL

每次写入或删除都会先追加一行日志。记录格式是 `SET|key|value` 或 `DEL|key`，程序启动时按顺序回放日志。当前分隔符没有转义，键和值不能包含 `|`。

## 刷盘

MemTable 中新增键达到 5 个时，当前内容会写入 `sst_1.sst` 等文件，随后清空 MemTable 并重建 WAL。程序启动时从 `sst_1.sst` 开始扫描，查询按新文件到旧文件的顺序进行。

## 构建

```bash
cmake -S . -B build
cmake --build build
```

项目需要 CMake 3.16 及以上版本和支持 C++17 的编译器。

## 测试

当前测试覆盖基础 CRUD、WAL 回放、MemTable 刷盘、最新 SST 优先和重启读取。测试文件会清理自己生成的日志与 SST 文件，但还没有独立运行目录。

## 命令行

| 命令 | 用途 | 示例 |
| --- | --- | --- |
| `SET key value` | 写入或更新键值 | `SET name Alice` |
| `GET key` | 查询键值 | `GET name` |
| `DEL key` | 删除键 | `DEL name` |
| `HELP` | 查看帮助 | `HELP` |
| `EXIT` | 退出程序 | `EXIT` |

```text
minikv> SET name Alice
OK
minikv> GET name
Alice
minikv> EXIT
Bye.
```

```bash
cmake --build build --target minikv_test
./build/minikv_test
```
