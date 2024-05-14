#ifndef UTIL_COMMAND_LINE_HPP_
#define UTIL_COMMAND_LINE_HPP_

#include <string>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include "../files/engine_paths.hpp"

class ArgsReader {
    int argc;
    char** argv;
    int pos = 0;
    const char* last = "";
public:
    ArgsReader(int argc, char** argv) : argc(argc), argv(argv) {}

    void skip() {
        pos++;
    }

    bool hasNext() const {
        return pos < argc && strlen(argv[pos]);
    }

    bool isKeywordArg() const {
        return last[0] == '-';
    }

    std::string next() {
        if (pos >= argc) {
            throw std::runtime_error("unexpected end");
        }
        last = argv[pos];
        return argv[pos++];
    }
};

/// @return false if engine start can
bool parse_cmdline(int argc, char** argv, EnginePaths& paths);

#endif // UTIL_COMMAND_LINE_HPP_
