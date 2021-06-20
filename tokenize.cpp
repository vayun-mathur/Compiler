#include "tokenize.h"
#include <iostream>
#include <regex>

std::string& ltrim(std::string& str)
{
    auto it2 = std::find_if(str.begin(), str.end(), [](char ch) { return !std::isspace(ch); });
    str.erase(str.begin(), it2);
    return str;
}

struct token_data {
    token_type type;
    std::string regex;
};

token_data token_regex[] = {
    {INT_KEYWORD, R"(int)"},
    {SEMICOLON, R"(;)"},
    {PLUS, R"(\+)"},
    {MINUS, R"(\-)"},
    {ASTERISK, R"(\*)"},
    {SLASH, R"(\/)"},
    {MODULUS, R"(%)"},
    {EQUAL_SIGN, R"(=)"},
    {OPEN_PARENTHESES, R"(\()"},
    {CLOSE_PARENTHESES, R"(\))"},
    {OPEN_BRACES, R"(\{)"},
    {CLOSE_BRACES, R"(\})"},
    {RETURN_KEYWORD, R"(return)"},
    {INT_VALUE, R"([0-9]+)"},
    {NAME, R"([a-zA-Z_$][a-zA-Z_$0-9]*)"}
};

void tokenize(std::string s, std::queue<token>& tokens)
{
    while (!s.empty()) {
        s = ltrim(s);
        for (token_data d : token_regex) {
            std::smatch m;
            if (std::regex_search(s, m, std::regex(d.regex), std::regex_constants::match_flag_type::match_continuous)) {
                    std::string ss = m.str();
                    tokens.push({ d.type, ss });
                    s = s.substr(m[0].second - s.begin());
                    break;
            }
        }
    }
}
