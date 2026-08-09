#include "minikv.h"
#include "utils.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

MiniKV::MiniKV(const std::string& logFile) : filename_(logFile)
{
    ReplayWAL();
    ScanSSTFiles();
    wal_file_.open(filename_, std::ios::out | std::ios::app);
}

MiniKV::~MiniKV()
{
    if (wal_file_.is_open()) {
        wal_file_.flush();
        wal_file_.close();
    }
}

Status MiniKV::Set(const std::string& key, const std::string& value)
{
    AppendSetLog(key, value);
    if (mem_table_.find(key) == mem_table_.end()) {
        ++mem_entry_count_;
    }
    mem_table_[key] = value;

    if (mem_entry_count_ >= kMemTableThreshold) {
        FlushMemTableToSST();
    }

    return Status::Ok();
}

Status MiniKV::Get(const std::string& key, std::string* value_out)
{
    const auto it = mem_table_.find(key);
    if (it == mem_table_.end()) {
        return SearchInSST(key, value_out);
    }

    *value_out = it->second;
    return Status::Ok();
}

Status MiniKV::Delete(const std::string& key)
{
    AppendDeleteLog(key);
    const auto it = mem_table_.find(key);
    if (it != mem_table_.end()) {
        mem_table_.erase(it);
        --mem_entry_count_;
    }
    return Status::Ok();
}

void MiniKV::AppendSetLog(const std::string& key, const std::string& value)
{
    if (!wal_file_.is_open()) {
        return;
    }

    wal_file_ << "SET|" << key << "|" << value << '\n';
    wal_file_.flush();
}

void MiniKV::AppendDeleteLog(const std::string& key)
{
    if (!wal_file_.is_open()) {
        return;
    }

    wal_file_ << "DEL|" << key << '\n';
    wal_file_.flush();
}

void MiniKV::ReplayWAL()
{
    std::ifstream input(filename_);
    if (!input.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        const std::vector<std::string> fields = split(line, '|');
        if (fields.empty()) {
            continue;
        }

        if (fields[0] == "SET" && fields.size() >= 3) {
            mem_table_[fields[1]] = fields[2];
        } else if (fields[0] == "DEL" && fields.size() >= 2) {
            mem_table_.erase(fields[1]);
        }
    }
}

void MiniKV::FlushMemTableToSST()
{
    const std::string sst_name =
        "sst_" + std::to_string(next_sst_seq_++) + ".sst";
    std::ofstream output(sst_name);
    for (const auto& entry : mem_table_) {
        output << entry.first << "|" << entry.second << '\n';
    }
    output.close();

    sst_files_.push_back(sst_name);
    mem_table_.clear();
    mem_entry_count_ = 0;

    wal_file_.close();
    std::remove(filename_.c_str());
    wal_file_.open(filename_, std::ios::out | std::ios::app);
    if (!wal_file_.is_open()) {
        std::cerr << "reopen wal after flush failed!\n";
    }
}

Status MiniKV::SearchInSST(const std::string& key, std::string* value_out)
{
    for (auto file = sst_files_.rbegin(); file != sst_files_.rend(); ++file) {
        std::ifstream input(*file);
        if (!input.is_open()) {
            continue;
        }

        std::string line;
        while (std::getline(input, line)) {
            const auto separator = line.find('|');
            if (separator == std::string::npos) {
                continue;
            }

            if (line.substr(0, separator) == key) {
                *value_out = line.substr(separator + 1);
                return Status::Ok();
            }
        }
    }

    return Status::NotFound();
}

void MiniKV::ScanSSTFiles()
{
    for (uint64_t sequence = 1;; ++sequence) {
        const std::string name =
            "sst_" + std::to_string(sequence) + ".sst";
        std::ifstream input(name);
        if (!input.is_open()) {
            break;
        }

        sst_files_.push_back(name);
    }
}
