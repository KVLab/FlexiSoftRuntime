#pragma once
#include "common.h"
#include <map>

struct JVal
{
    enum T { NIL, BOOL, NUM, STR, OBJ, ARR } t;

    bool b;
    double n;
    std::string s;
    std::map<std::string, JVal> o;
    std::vector<JVal> a;

    JVal() : t(NIL), b(false), n(0) {}
};

bool JsonParseFile(const char* path, JVal& root, std::string& err);
bool JsonParseText(const std::string& text, JVal& root, std::string& err);

const JVal* JsonGet(const JVal& o, const char* k);

std::string JsonString(const JVal& o, const char* k, const std::string& d = "");
int JsonInt(const JVal& o, const char* k, int d = 0);
bool JsonBool(const JVal& o, const char* k, bool d = false);

std::string JsonReadFile(const char* path);
