# Minikv

一个用 C++17 写的简化键值存储练习项目。当前版本先把内存中的增删改查接口跑通，数据还不会落盘。

## 当前进度

- 已完成：内存表、`Set`、`Get`、`Delete`、状态返回、WAL 回放和 SST 写入
- 部分完成：MemTable 达到 5 个新键后会生成 SST 文件
- 未完成：启动扫描 SST、SST 查询、命令行和自动化测试

## WAL

每次写入或删除都会先追加一行日志。记录格式是 `SET|key|value` 或 `DEL|key`，程序启动时按顺序回放日志。当前分隔符没有转义，键和值不能包含 `|`。

## 刷盘

MemTable 中新增键达到 5 个时，当前内容会写入 `sst_1.sst` 等文件，随后清空 MemTable 并重建 WAL。SST 的启动扫描和查询会在后续补上。

## 构建

```bash
cmake -S . -B build
cmake --build build
```

项目需要 CMake 3.16 及以上版本和支持 C++17 的编译器。
