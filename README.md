# Minikv

一个用 C++17 写的简化键值存储练习项目。当前版本先把内存中的 `Set` 和 `Get` 接口跑通，数据还不会落盘。

## 当前进度

- 已完成：内存表、写入和查询接口
- 未完成：删除、WAL、SSTable、命令行和自动化测试

## 构建

```bash
cmake -S . -B build
cmake --build build
```

项目需要 CMake 3.16 及以上版本和支持 C++17 的编译器。
