#ifndef MINIKV_H
#define MINIKV_H

#include <string>
#include <unordered_map>

class Status {
public:
    bool ok() const { return code_ == Code::OK; }
    bool IsNotFound() const { return code_ == Code::NotFound; }

    static Status Ok() { return Status(Code::OK, ""); }
    static Status NotFound(const std::string& msg = "key not found") {
        return Status(Code::NotFound, msg);
    }

    std::string ToString() const { return message_; }

private:
    enum class Code { OK, NotFound };

    Status(Code code, std::string message)
        : code_(code), message_(std::move(message)) {}

    Code code_;
    std::string message_;
};

class MiniKV {
public:
    explicit MiniKV(const std::string& logFile = "minikv.log");

    Status Set(const std::string& key, const std::string& value);
    Status Get(const std::string& key, std::string* value_out);
    Status Delete(const std::string& key);

private:
    std::string filename_;
    std::unordered_map<std::string, std::string> mem_table_;
};

#endif
