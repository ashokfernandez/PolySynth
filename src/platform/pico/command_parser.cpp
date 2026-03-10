#include "command_parser.h"
#include <cstdlib>
#include <cstring>

namespace pico_serial {

bool CommandParser::Feed(char c) {
    if (c == '\n' || c == '\r') {
        if (mPos == 0) return false; // ignore empty lines
        mBuf[mPos] = '\0';
        return true;
    }
    if (mPos < kMaxLineLen - 1) {
        mBuf[mPos++] = c;
    }
    // If buffer is full, silently drop further chars until newline
    return false;
}

// Helper: skip whitespace and return pointer to next token
static const char* skip_ws(const char* p) {
    while (*p == ' ' || *p == '\t') ++p;
    return p;
}

// Helper: read next whitespace-delimited token into dst, return pointer past it
static const char* read_token(const char* p, char* dst, int max_len) {
    p = skip_ws(p);
    int i = 0;
    while (*p && *p != ' ' && *p != '\t' && i < max_len - 1) {
        dst[i++] = *p++;
    }
    dst[i] = '\0';
    return p;
}

Command CommandParser::Parse() {
    Command cmd;
    char token[32] = {};
    const char* p = mBuf;

    p = read_token(p, token, sizeof(token));

    // Reset buffer position for next line (const method, so caller must call Feed again)

    if (std::strcmp(token, "NOTE_ON") == 0) {
        cmd.type = Command::NOTE_ON;
        char arg[16] = {};
        p = read_token(p, arg, sizeof(arg));
        cmd.intArg1 = std::atoi(arg);
        read_token(p, arg, sizeof(arg));
        cmd.intArg2 = (arg[0] != '\0') ? std::atoi(arg) : 100;
    } else if (std::strcmp(token, "NOTE_OFF") == 0) {
        cmd.type = Command::NOTE_OFF;
        char arg[16] = {};
        read_token(p, arg, sizeof(arg));
        cmd.intArg1 = std::atoi(arg);
    } else if (std::strcmp(token, "SET") == 0) {
        cmd.type = Command::SET;
        p = read_token(p, cmd.strArg, sizeof(cmd.strArg));
        char arg[32] = {};
        read_token(p, arg, sizeof(arg));
        cmd.floatArg = std::atof(arg);
    } else if (std::strcmp(token, "GET") == 0) {
        cmd.type = Command::GET;
        read_token(p, cmd.strArg, sizeof(cmd.strArg));
    } else if (std::strcmp(token, "STATUS") == 0) {
        cmd.type = Command::STATUS;
    } else if (std::strcmp(token, "PANIC") == 0) {
        cmd.type = Command::PANIC;
    } else if (std::strcmp(token, "RESET") == 0) {
        cmd.type = Command::RESET;
    } else if (std::strcmp(token, "DEMO") == 0) {
        cmd.type = Command::DEMO;
    } else if (std::strcmp(token, "STOP") == 0) {
        cmd.type = Command::STOP;
    }

    // Reset buffer for next line
    mPos = 0;
    mBuf[0] = '\0';

    return cmd;
}

} // namespace pico_serial
