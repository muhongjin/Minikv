#include <iostream>
#include <sstream>
#include <string>

#include "minikv.h"

namespace {

void PrintHelp()
{
    std::cout << "Commands:\n"
              << "  SET key value  Set or update a key\n"
              << "  GET key        Get a value\n"
              << "  DEL key        Delete a key\n"
              << "  HELP           Show this help\n"
              << "  EXIT           Exit MiniKV\n";
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

        if (command == "SET") {
            std::string key;
            std::string value;
            if (!(input >> key >> value)) {
                std::cout << "Usage: SET key value\n";
                continue;
            }
            std::cout << (kv.Set(key, value).ok() ? "OK\n" : "ERROR\n");
        } else if (command == "GET") {
            std::string key;
            if (!(input >> key)) {
                std::cout << "Usage: GET key\n";
                continue;
            }

            std::string value;
            const Status status = kv.Get(key, &value);
            std::cout << (status.ok() ? value : "(nil)") << "\n";
        } else if (command == "DEL") {
            std::string key;
            if (!(input >> key)) {
                std::cout << "Usage: DEL key\n";
                continue;
            }
            std::cout << (kv.Delete(key).ok() ? "OK\n" : "ERROR\n");
        } else if (command == "HELP") {
            PrintHelp();
        } else if (command == "EXIT") {
            std::cout << "Bye.\n";
            break;
        } else {
            std::cout << "Unknown command. Type HELP for help.\n";
        }
    }

    return 0;
}
