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
    {RETURN_KEYWORD, R"(return)"},
    {SEMICOLON, R"(;)"},

    {INCREMENT, R"(\+\+)"},
    {DECREMENT, R"(\-\-)"},

    {ADD_ASSIGN, R"(\+=)"},
    {SUBTRACT_ASSIGN, R"(\-=)"},
    {MULTIPLY_ASSIGN, R"(\*=)"},
    {DIVIDE_ASSIGN, R"(\/=)"},
    {MOD_ASSIGN, R"(%=)"},
    {AND_ASSIGN, R"(&=)"},
    {OR_ASSIGN, R"(\|=)"},
    {XOR_ASSIGN, R"(\^=)"},
    {LEFT_SHIFT_ASSIGN, R"(<<=)"},
    {RIGHT_SHIFT_ASSIGN, R"(>>=)"},

    {PLUS, R"(\+)"},
    {MINUS, R"(\-)"},
    {ASTERISK, R"(\*)"},
    {SLASH, R"(\/)"},
    {MODULUS, R"(%)"},

    {LEFT_SHIFT, R"(<<)"},
    {RIGHT_SHIFT, R"(>>)"},

    {EXCLAMATION, R"(!)"},

    {EQUAL_TO, R"(==)"},
    {NOT_EQUAL_TO, R"(!=)"},
    {GREATER_OR_EQUAL_TO, R"(>=)"},
    {GREATER_THAN, R"(>)"},
    {LESS_OR_EQUAL_TO, R"(<=)"},
    {LESS_THAN, R"(<)"},

    {EQUAL_SIGN, R"(=)"},

    {LOGICAL_AND, R"(&&)"},
    {LOGICAL_OR, R"(\|\|)"},

    {OPEN_PARENTHESES, R"(\()"},
    {CLOSE_PARENTHESES, R"(\))"},

    {BITWISE_COMPLEMENT, R"(~)"},
    {BITWISE_AND, R"(&)"},
    {BITWISE_OR, R"(\|)"},
    {BITWISE_XOR, R"(\^)"},

    {COMMA, R"(,)"},

    {OPEN_BRACES, R"(\{)"},
    {CLOSE_BRACES, R"(\})"},

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
