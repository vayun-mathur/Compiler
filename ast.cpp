#include "ast.h"
#include <map>

std::map<std::tuple<int, binary_operator, int>, assembly > binary_operator_assembly;
std::map<std::tuple<int, binary_operator, int>, int > binary_operator_result_type;

token check_token(std::queue<token>& tokens, token_type type) {
	token t = tokens.front();
	tokens.pop();
	if (t.type != type) throw std::exception("Incorrect token");
	return t;
}

// <term> = INT_VALUE { [ + - ] INT_VALUE }
// <exp> = <term> { [ * / ] <term> }
// <line> = RETURN_KEYWORD <exp> SEMICOLON
// <function> = INT_KEYWORD NAME OPEN_PARENTHESES CLOSE_PARENTHESES OPEN_BRACES { <line> } CLOSE_BRACES
// <app> = { <function> }

constexpr BinaryOperator* create_binary_operator(Expression* exp1, Expression* exp2, binary_operator op) {
	return new BinaryOperator(op, exp1, exp2, binary_operator_result_type[{exp1->return_type, op, exp2->return_type}]);
}

Expression* compile_term(std::queue<token>& tokens) {
	Expression* exp = new ConstantInt(stoi(check_token(tokens, INT_VALUE).value));
	while (tokens.front().type == ASTERISK || tokens.front().type == SLASH) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = new ConstantInt(stoi(check_token(tokens, INT_VALUE).value));
		if (t.type == ASTERISK) {
			exp = create_binary_operator(exp, exp2, multiply);
		}
		else if (t.type==SLASH){
			exp = create_binary_operator(exp, exp2, divide);
		}
		else {
			exp = create_binary_operator(exp, exp2, mod);
		}
	}
	return exp;
}

Expression* compile_exp(std::queue<token>& tokens) {
	Expression* exp = compile_term(tokens);
	while (tokens.front().type == PLUS || tokens.front().type == MINUS) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = compile_term(tokens);
		if (t.type == PLUS) {
			exp = create_binary_operator(exp, exp2, add);
		}
		else {
			exp = create_binary_operator(exp, exp2, subtract);
		}
	}
	return exp;
}

LineOfCode* compile_line(std::queue<token>& tokens) {
	check_token(tokens, RETURN_KEYWORD);
	Return* r = new Return(compile_exp(tokens));
	check_token(tokens, SEMICOLON);
	return r;
}

Function* compile_function(std::queue<token>& tokens)
{
	Function* f = new Function();
	check_token(tokens, INT_KEYWORD);
	f->name = check_token(tokens, NAME).value;
	check_token(tokens, OPEN_PARENTHESES);
	check_token(tokens, CLOSE_PARENTHESES);
	check_token(tokens, OPEN_BRACES);
	while (tokens.front().type != CLOSE_BRACES) {
		f->lines.push_back(compile_line(tokens));
	}
	check_token(tokens, CLOSE_BRACES);
	return f;
}
Application* compile_application(std::queue<token>& tokens)
{
	Application* a = new Application();
	while (!tokens.empty() && tokens.front().type == INT_KEYWORD) {
		a->functions.push_back(compile_function(tokens));
	}
	return a;
}



void Application::generateAssembly(assembly& ass)
{
	for (Function* func : functions) {
		func->generateAssembly(ass);
	}
}

void Function::generateAssembly(assembly& ass)
{
	ass.add(".globl	main");
	ass.add(name + ":");
	for (LineOfCode* line : lines) {
		line->generateAssembly(ass);
	}
}

void Return::generateAssembly(assembly& ass)
{
	expr->generateAssembly(ass);
	ass.add("\tret");
}

void initAST() {
	binary_operator_result_type[{DataType::INT, binary_operator::add, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, binary_operator::add, DataType::INT }] = 
		assembly({ "\taddl %ecx, %eax" });

	binary_operator_result_type[{DataType::INT, binary_operator::subtract, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, binary_operator::subtract, DataType::INT }] =
		assembly({ "\tsubl %ecx, %eax" });

	binary_operator_result_type[{DataType::INT, binary_operator::multiply, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, binary_operator::multiply, DataType::INT }] =
		assembly({ "\timull %eax, %ecx" });

	binary_operator_result_type[{DataType::INT, binary_operator::divide, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, binary_operator::divide, DataType::INT }] =
		assembly({ "\tidivl %ecx, %eax" });

	binary_operator_result_type[{DataType::INT, binary_operator::mod, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, binary_operator::mod, DataType::INT }] =
		assembly({ "\tidivl %ecx, %eax", "\tmovl %edx, %eax" });
}

void BinaryOperator::generateAssembly(assembly& ass)
{
	right->generateAssembly(ass);
	ass.add("\tpush %rax");
	left->generateAssembly(ass);
	ass.add("\tpop %rcx");
	ass.add(binary_operator_assembly[{left->return_type, op, right->return_type}]);
}

void ConstantInt::generateAssembly(assembly& ass)
{
	ass.add("\tmovl $" + std::to_string(val) + ", %eax");
}
