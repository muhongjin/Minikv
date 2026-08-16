#ifndef MINIKV_H
#define MINIKV_H

#include <string>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <cstdint>


// 区分操作成功和键不存在。
class Status {
public:
    bool ok() const { return code_ == Code::OK; }

    bool IsNotFound() const { return code_ == Code::NotFound; }

    static Status Ok() { return Status(Code::OK, ""); }
    static Status NotFound(const std::string& msg = "key not found") { return Status(Code::NotFound, msg); }

    std::string ToString() const { return msg_; }

private:
    enum class Code {
        OK,
        NotFound
    };

    Status(Code c, std::string m) : code_(c), msg_(std::move(m)) {}
    Code code_;
    std::string msg_;
};

class MiniKV {
public:
    MiniKV(const std::string& logFile = "minikv.log");
    ~MiniKV();


    Status Set(const std::string& key, const std::string& value);
    Status Get(const std::string& key, std::string* value_out);
    Status Delete(const std::string& key);

private:
    std::unordered_map<std::string, std::string> mem_table_;

    std::ofstream walFile_;    // 追加写入 WAL 记录。
    std::string filename;

    void ReplayWAL();          // 启动时按顺序恢复内存表。
    void AppendSetLog(const std::string& key, const std::string& val);
    void AppendDelLog(const std::string& key);

    const size_t kMemTableThreshold = 5;   // 新增键达到阈值后触发刷盘。
    size_t memEntryCount_ = 0;             // 当前 MemTable 中的键数量。
    std::vector<std::string> sstFiles_;    // 按发现顺序保存 SST 文件名。
    uint64_t next_sst_seq_{1};

    // 将当前 MemTable 写入新的 SST 文件，并重建 WAL。
    void FlushMemTableToSST();
    // 按新文件到旧文件的顺序查找键。
    Status SearchInSST(const std::string& key, std::string* val_out);
    // 启动时扫描现有 SST 文件。
    void ScanSSTFiles();
    
};

#endif
