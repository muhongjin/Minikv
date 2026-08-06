#include "minikv.h"

MiniKV::MiniKV(const std::string& logFile) : filename_(logFile) {}

Status MiniKV::Set(const std::string& key, const std::string& value)
{
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
