#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

#include "minikv.h"

namespace {

std::string TrimLeft(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t");
    return first == std::string::npos ? "" : value.substr(first);
}

void NormalizeCommand(std::string& command)
{
    for (char& ch : command) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
}

bool HasSeparator(const std::string& value)
{
    return value.find('|') != std::string::npos;
}

void PrintHelp()
{
    std::cout << "Commands:\n"
              << "  SET key value  Set or update a key\n"
              << "  GET key        Get a value\n"
              << "  DEL key        Delete a key\n"
              << "  HELP           Show this help\n"
              << "  EXIT           Exit MiniKV\n";
}

void HandleSet(MiniKV& kv, const std::string& arguments)
{
    std::istringstream input(arguments);
    std::string key;
    if (!(input >> key)) {
        std::cout << "Usage: SET key value\n";
        return;
    }

    std::string value;
    std::getline(input, value);
    value = TrimLeft(value);
    if (value.empty() || HasSeparator(key) || HasSeparator(value)) {
        std::cout << "SET requires a non-empty key and value without '|'.\n";
        return;
    }

    const Status status = kv.Set(key, value);
    std::cout << (status.ok() ? "OK\n" : status.ToString() + "\n");
}

void HandleGet(MiniKV& kv, const std::string& arguments)
{
    std::istringstream input(arguments);
    std::string key;
    std::string extra;
    if (!(input >> key) || (input >> extra)) {
        std::cout << "Usage: GET key\n";
        return;
    }

    std::string value;
    const Status status = kv.Get(key, &value);
    if (status.ok()) {
        std::cout << value << "\n";
    } else if (status.IsNotFound()) {
        std::cout << "(nil)\n";
    } else {
        std::cout << status.ToString() << "\n";
    }
}

void HandleDelete(MiniKV& kv, const std::string& arguments)
{
    std::istringstream input(arguments);
    std::string key;
    std::string extra;
    if (!(input >> key) || (input >> extra)) {
        std::cout << "Usage: DEL key\n";
        return;
    }

    const Status status = kv.Delete(key);
    std::cout << (status.ok() ? "OK\n" : status.ToString() + "\n");
}

}

int main()
{
    MiniKV kv("minikv.log");
    std::string line;

    std::cout << "MiniKV CLI\n";
    std::cout << "Type HELP for available commands.\n";

    while (true) {
        std::cout << "minikv> ";
        if (!std::getline(std::cin, line)) {
            break;
        }

        std::istringstream input(line);
        std::string command;
        if (!(input >> command)) {
            continue;
        }

        std::string arguments;
        std::getline(input, arguments);
        arguments = TrimLeft(arguments);
        NormalizeCommand(command);

        if (command == "SET") {
            HandleSet(kv, arguments);
        } else if (command == "GET") {
            HandleGet(kv, arguments);
        } else if (command == "DEL" || command == "DELETE") {
            HandleDelete(kv, arguments);
        } else if (command == "HELP") {
            PrintHelp();
        } else if (command == "EXIT" || command == "QUIT") {
            std::cout << "Bye.\n";
            break;
        } else {
            std::cout << "Unknown command. Type HELP for help.\n";
        }
    }

    return 0;
}
