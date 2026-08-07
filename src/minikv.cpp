#include "minikv.h"
#include "utils.h"

#include <fstream>
#include <vector>

MiniKV::MiniKV(const std::string& logFile) : filename_(logFile)
{
    ReplayWAL();
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
    mem_table_[key] = value;
    return Status::Ok();
}

Status MiniKV::Get(const std::string& key, std::string* value_out)
{
    const auto it = mem_table_.find(key);
    if (it == mem_table_.end()) {
        return Status::NotFound();
    }

    *value_out = it->second;
    return Status::Ok();
}

Status MiniKV::Delete(const std::string& key)
{
    AppendDeleteLog(key);
    mem_table_.erase(key);
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
