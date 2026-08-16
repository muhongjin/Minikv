#include "minikv.h"
#include "utils.h"
#include <unordered_map>
#include <sstream>
#include <iostream>
#include <vector>

MiniKV::MiniKV(const std::string& logFile)
{
    filename = logFile;
    // 先恢复未刷盘的数据，再登记磁盘上的 SST 文件。
    ReplayWAL();
    ScanSSTFiles();
    // 追加打开日志，后续操作接着写入。
    walFile_.open(logFile, std::ios::app | std::ios::out);
    if(!walFile_.is_open()){
        std::cerr << "open wal log failed!" << std::endl;
    }
}

MiniKV::~MiniKV()
{
    if(walFile_.is_open()){
        walFile_.flush();
        walFile_.close();
    }
}

Status MiniKV::Set(const std::string& key, const std::string& value){
    AppendSetLog(key, value);
    // 覆盖已有键不增加刷盘计数。
    if(mem_table_.find(key) == mem_table_.end()){
        memEntryCount_ ++;
    }
    mem_table_[key] = value;

    // 新增键达到阈值时写入 SST。
    if(memEntryCount_ >= kMemTableThreshold)
    {
        FlushMemTableToSST();
    }
    return Status::Ok();
}

Status MiniKV::Get(const std::string& key, std::string* val_out)
{
    // 内存命中直接返回，未命中时再查询 SST 文件。
    auto it = mem_table_.find(key);
    if(it != mem_table_.end())
    {
        *val_out = it->second;
        return Status::Ok();
    }
    return SearchInSST(key, val_out);
}

Status MiniKV::Delete(const std::string& key){
    AppendDelLog(key);
    // 删除只更新内存表，已落盘数据仍会保留在旧 SST 中。
    auto it = mem_table_.find(key);
    if(it != mem_table_.end())
    {
        mem_table_.erase(it);
        memEntryCount_ --;
    }
    return Status::Ok();
}

void MiniKV::AppendSetLog(const std::string& key, const std::string& val)
{
    // 一条写入记录保存为 SET|key|value。
    if(!walFile_.is_open()) return;
    walFile_ << "SET|" << key << "|" << val << std::endl;
    walFile_.flush();
}

void MiniKV::AppendDelLog(const std::string& key)
{
    // 一条删除记录只保存操作类型和键。
    if(!walFile_.is_open()) return;
    walFile_ << "DEL|" << key << std::endl;
    walFile_.flush();
}

void MiniKV::ReplayWAL()
{
    // 日志必须按写入顺序回放，后出现的记录覆盖先前状态。
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        return;
    }
    std::string s = "";
    while(getline(inFile,s)){
         std::vector<std::string> vec = split(s, '|');
        if(vec.empty()) continue;

        if(vec[0] == "SET"){
            if(vec.size() >= 3){
                std::string key = vec[1];
                std::string val = vec[2];
                mem_table_[key] = val;
            }
        }
        else if(vec[0] == "DEL"){
            if(vec.size() >= 2){
                std::string key = vec[1];
                mem_table_.erase(key);
            }
        }
    }
    inFile.close();
}

void MiniKV::FlushMemTableToSST()
{
    // 文件名使用递增序号，查询时新文件优先于旧文件。
    std::string sst_name = "sst_" + std::to_string(next_sst_seq_++) + ".sst";
    std::ofstream out(sst_name);

    // SST 每行保存一个 key|value 对。
    for(auto &pair : mem_table_)
    {
        out << pair.first << "|" << pair.second << "\n";
    }
    out.close();

    sstFiles_.push_back(sst_name);

    // SST 写完后清空内存表，再重建 WAL。
    mem_table_.clear();
    memEntryCount_ = 0;

    walFile_.close();
    std::remove(filename.c_str());
    walFile_.open(filename, std::ios::out | std::ios::app);
    if (!walFile_.is_open())
    {
        std::cerr << "reopen wal after flush failed!\n";
    }
}

Status MiniKV::SearchInSST(const std::string& key, std::string* val_out)
{
    // 逆序遍历 SST，先命中新文件里的值。
    for(auto it = sstFiles_.rbegin(); it != sstFiles_.rend(); ++it)
    {
        const auto& sst_name = *it;
        std::ifstream fin(sst_name);
        if(!fin.is_open()) continue;
        std::string line;
        while(std::getline(fin, line))
        {
            // 只在第一个分隔符处分开键和值。
            auto pos = line.find('|');
            if(pos == std::string::npos) continue;
            std::string k = line.substr(0, pos);
            std::string v = line.substr(pos+1);
            if(k == key)
            {
                *val_out = v;
                return Status::Ok();
            }
        }
    }
    return Status::NotFound();
}

void MiniKV::ScanSSTFiles()
{
    uint64_t seq = 1;
    // 当前实现要求序号连续，遇到缺号就停止扫描。
    while(true)
    {
        std::string fname = "sst_" + std::to_string(seq) + ".sst";
        std::ifstream fin(fname);
        if (!fin.is_open())
        {
            break;
        }
        fin.close();
        sstFiles_.push_back(fname);
        seq ++;
    }
}
