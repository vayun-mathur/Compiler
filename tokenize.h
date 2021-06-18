#pragma once
#include <queue>
#include <string>

enum token_type {
	INT_KEYWORD, SEMICOLON, OPEN_PARENTHESES, CLOSE_PARENTHESES, OPEN_BRACES, CLOSE_BRACES, RETURN_KEYWORD, INT_VALUE, NAME, PLUS, MINUS, ASTERISK, SLASH, MODULUS
};

struct token {
	token_type type;
	std::string value;
};

void tokenize(std::string s, std::queue<token>& token_queue);