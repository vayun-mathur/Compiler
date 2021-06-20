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

// <val> = INT_VALUE | NAME
// <term> = <val> { [ + - ] <val> }
// <exp> = <term> { [ * / ] <term> }
// <line> = RETURN_KEYWORD <exp> SEMICOLON | <exp> SEMICOLON | INT_KEYWORD NAME (EQUAL_SIGN <exp>) SEMICOLON
// <function> = INT_KEYWORD NAME OPEN_PARENTHESES CLOSE_PARENTHESES OPEN_BRACES { <line> } CLOSE_BRACES
// <app> = { <function> }

inline BinaryOperator* create_binary_operator(Expression* exp1, Expression* exp2, binary_operator op) {
	return new BinaryOperator(op, exp1, exp2, binary_operator_result_type[{exp1->return_type, op, exp2->return_type}]);
}

Expression* compile_val(std::queue<token>& tokens) {
	if (tokens.front().type == INT_VALUE) {
		return new ConstantInt(stoi(check_token(tokens, INT_VALUE).value));
	}
	else {
		return new VariableRef(check_token(tokens, NAME).value, DataType::INT);
	}
}

Expression* compile_term(std::queue<token>& tokens) {
	Expression* exp = compile_val(tokens); 
	while (tokens.front().type == ASTERISK || tokens.front().type == SLASH) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = compile_val(tokens);
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
	token t = tokens.front();
	if (t.type == RETURN_KEYWORD) {
		check_token(tokens, RETURN_KEYWORD);
		Return* r = new Return(compile_exp(tokens));
		check_token(tokens, SEMICOLON);
		return r;
	}
	else if (t.type == INT_KEYWORD) {
		check_token(tokens, INT_KEYWORD);
		std::string name = check_token(tokens, NAME).value;
		Expression* exp = nullptr;
		if (tokens.front().type == EQUAL_SIGN) {
			check_token(tokens, EQUAL_SIGN);
			exp = compile_exp(tokens);
		}
		check_token(tokens, SEMICOLON);
		return new VariableDeclarationLine(exp, DataType::INT, name);
	} else {
		Expression* e = compile_exp(tokens);
		check_token(tokens, SEMICOLON);
		return new ExpressionLine(e);
	}
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







struct variable {
	std::string name;
	int location;
};

std::map<std::string, variable> variables;
int current_variable_location = -8;


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
	ass.add("\tpush %rbp");
	ass.add("\tmovq %rsp, %rbp");
	for (LineOfCode* line : lines) {
		line->generateAssembly(ass);
	}
}

void Return::generateAssembly(assembly& ass)
{
	expr->generateAssembly(ass);
	ass.add("\tmovq %rbp, %rsp");
	ass.add("\tpop %rbp");
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
		assembly({ "\timull %ecx, %eax" });

	binary_operator_result_type[{DataType::INT, binary_operator::divide, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, binary_operator::divide, DataType::INT }] =
		assembly({ "\tmovl $0, %edx", "\tidivl %ecx" });

	binary_operator_result_type[{DataType::INT, binary_operator::mod, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, binary_operator::mod, DataType::INT }] =
		assembly({ "\tmovl $0, %edx", "\tidivl %ecx", "\tmovl %edx, %eax" });
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

void ExpressionLine::generateAssembly(assembly& ass)
{
	exp->generateAssembly(ass);
}

void VariableDeclarationLine::generateAssembly(assembly& ass)
{
	if (init_exp == nullptr) {
		ass.add("\tmovl $0, %eax");
	}
	else {
		init_exp->generateAssembly(ass);
	}
	variables.insert({ name, {name, current_variable_location} });
	current_variable_location -= 8;
	ass.add("\tpush %rax");
}

void VariableRef::generateAssembly(assembly& ass)
{
	ass.add("\tmovl " + std::to_string(variables[name].location) + "(%rbp), %eax");
}
