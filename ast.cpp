#include "ast.h"
#include <map>

std::map<std::tuple<int, binary_operator, int>, assembly > binary_operator_assembly;
std::map<std::tuple<int, binary_operator, int>, int > binary_operator_result_type;

std::map<std::tuple<int, unary_operator>, assembly > unary_operator_assembly;
std::map<std::tuple<int, unary_operator>, int > unary_operator_result_type;

token check_token(std::queue<token>& tokens, token_type type) {
	token t = tokens.front();
	tokens.pop();
	if (t.type != type) throw std::exception("Incorrect token");
	return t;
}

inline BinaryOperator* create_binary_operator(Expression* exp1, Expression* exp2, binary_operator op) {
	return new BinaryOperator(op, exp1, exp2, binary_operator_result_type[{exp1->return_type, op, exp2->return_type}]);
}

inline UnaryOperator* create_unary_operator(Expression* exp1, unary_operator op) {
	return new UnaryOperator(op, exp1, unary_operator_result_type[{exp1->return_type, op}]);
}

Expression* compile_0(std::queue<token>& tokens) {
	if (tokens.front().type == INT_VALUE) {
		return new ConstantInt(stoi(check_token(tokens, INT_VALUE).value));
	}
	else {
		return new VariableRef(check_token(tokens, NAME).value, DataType::INT);
	}
}

Expression* compile_1(std::queue<token>& tokens) {
	if (tokens.front().type == MINUS || tokens.front().type == BITWISE_COMPLEMENT || tokens.front().type == EXCLAMATION) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp = compile_1(tokens);
		if (t.type == MINUS) {
			exp = create_unary_operator(exp, negation);
		}
		else if (t.type == BITWISE_COMPLEMENT) {
			exp = create_unary_operator(exp, bitwise_complement);
		}
		else {
			exp = create_unary_operator(exp, logical_negation);
		}
		return exp;
	}
	else {
		return compile_0(tokens);
	}
}

Expression* compile_5(std::queue<token>& tokens) {
	Expression* exp = compile_1(tokens); 
	while (tokens.front().type == ASTERISK || tokens.front().type == SLASH) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = compile_1(tokens);
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

Expression* compile_6(std::queue<token>& tokens) {
	Expression* exp = compile_5(tokens);
	while (tokens.front().type == PLUS || tokens.front().type == MINUS) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = compile_5(tokens);
		if (t.type == PLUS) {
			exp = create_binary_operator(exp, exp2, add);
		}
		else {
			exp = create_binary_operator(exp, exp2, subtract);
		}
	}
	return exp;
}

Expression* compile_9(std::queue<token>& tokens) {
	Expression* exp = compile_6(tokens);
	while (tokens.front().type == LESS_THAN || tokens.front().type == LESS_OR_EQUAL_TO || tokens.front().type == GREATER_THAN || tokens.front().type == GREATER_OR_EQUAL_TO) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = compile_6(tokens);
		if (t.type == LESS_THAN) {
			exp = create_binary_operator(exp, exp2, less);
		}
		else if(t.type == LESS_OR_EQUAL_TO) {
			exp = create_binary_operator(exp, exp2, less_equal);
		}
		else if(t.type == GREATER_THAN) {
			exp = create_binary_operator(exp, exp2, greater);
		}
		else {
			exp = create_binary_operator(exp, exp2, greater_equal);
		}
	}
	return exp;
}

Expression* compile_10(std::queue<token>& tokens) {
	Expression* exp = compile_9(tokens);
	while (tokens.front().type == EQUAL_TO || tokens.front().type == NOT_EQUAL_TO) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = compile_9(tokens);
		if (t.type == EQUAL_TO) {
			exp = create_binary_operator(exp, exp2, equal);
		}
		else {
			exp = create_binary_operator(exp, exp2, not_equal);
		}
	}
	return exp;
}

Expression* compile_14(std::queue<token>& tokens) {
	Expression* exp = compile_10(tokens);
	while (tokens.front().type == LOGICAL_AND) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = compile_10(tokens);
		exp = create_binary_operator(exp, exp2, logical_and);
	}
	return exp;
}

Expression* compile_15(std::queue<token>& tokens) {
	Expression* exp = compile_14(tokens);
	while (tokens.front().type == LOGICAL_OR) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = compile_14(tokens);
		exp = create_binary_operator(exp, exp2, logical_or);
	}
	return exp;
}

LineOfCode* compile_line(std::queue<token>& tokens) {
	token t = tokens.front();
	if (t.type == RETURN_KEYWORD) {
		check_token(tokens, RETURN_KEYWORD);
		Return* r = new Return(compile_15(tokens));
		check_token(tokens, SEMICOLON);
		return r;
	}
	else if (t.type == INT_KEYWORD) {
		check_token(tokens, INT_KEYWORD);
		std::string name = check_token(tokens, NAME).value;
		Expression* exp = nullptr;
		if (tokens.front().type == EQUAL_SIGN) {
			check_token(tokens, EQUAL_SIGN);
			exp = compile_15(tokens);
		}
		check_token(tokens, SEMICOLON);
		return new VariableDeclarationLine(exp, DataType::INT, name);
	} else {
		Expression* e = compile_15(tokens);
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
	binary_operator_result_type[{DataType::INT, add, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, add, DataType::INT }] = 
		assembly({ "\taddl %ecx, %eax" });

	binary_operator_result_type[{DataType::INT, subtract, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, subtract, DataType::INT }] =
		assembly({ "\tsubl %ecx, %eax" });

	binary_operator_result_type[{DataType::INT, multiply, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, multiply, DataType::INT }] =
		assembly({ "\timull %ecx, %eax" });

	binary_operator_result_type[{DataType::INT, divide, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, divide, DataType::INT }] =
		assembly({ "\tmovl $0, %edx", "\tidivl %ecx" });

	binary_operator_result_type[{DataType::INT, mod, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, mod, DataType::INT }] =
		assembly({ "\tmovl $0, %edx", "\tidivl %ecx", "\tmovl %edx, %eax" });

	binary_operator_result_type[{DataType::INT, equal, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, equal, DataType::INT }] =
		assembly({ "\tcmpl %eax, %ecx", "\tmovl $0, %eax", "\tsete %al" });

	binary_operator_result_type[{DataType::INT, not_equal, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, not_equal, DataType::INT }] =
		assembly({ "\tcmpl %eax, %ecx", "\tmovl $0, %eax", "\tsetne %al" });

	binary_operator_result_type[{DataType::INT, less, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, less, DataType::INT }] =
		assembly({ "\tcmpl %eax, %ecx", "\tmovl $0, %eax", "\tsetl %al" });

	binary_operator_result_type[{DataType::INT, greater, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, greater, DataType::INT }] =
		assembly({ "\tcmpl %eax, %ecx", "\tmovl $0, %eax", "\tsetg %al" });

	binary_operator_result_type[{DataType::INT, less_equal, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, less_equal, DataType::INT }] =
		assembly({ "\tcmpl %eax, %ecx", "\tmovl $0, %eax", "\tsetle %al" });

	binary_operator_result_type[{DataType::INT, greater_equal, DataType::INT }] = DataType::INT;
	binary_operator_assembly[{DataType::INT, greater_equal, DataType::INT }] =
		assembly({ "\tcmpl %eax, %ecx", "\tmovl $0, %eax", "\tsetge %al" });

	unary_operator_result_type[{DataType::INT, negation }] = DataType::INT;
	unary_operator_assembly[{DataType::INT, negation }] =
		assembly({ "\tneg %eax" });

	unary_operator_result_type[{DataType::INT, bitwise_complement }] = DataType::INT;
	unary_operator_assembly[{DataType::INT, bitwise_complement }] =
		assembly({ "\tnot %eax" });

	unary_operator_result_type[{DataType::INT, logical_negation }] = DataType::INT;
	unary_operator_assembly[{DataType::INT, logical_negation }] =
		assembly({ "\tcmpl $0, %eax", "\tmovl $0, %eax", "\tsete %al" });
}

void BinaryOperator::generateAssembly(assembly& ass)
{
	static int logical_operator_clause = 0;
	if (op == logical_or) {
		right->generateAssembly(ass);
		ass.add("\tcmpl $0, %eax");
		ass.add("\tje _loc" + std::to_string(logical_operator_clause));
		ass.add("\tmovl $1, %eax");
		ass.add("jmp _loc_end" + std::to_string(logical_operator_clause));
		ass.add("_loc" + std::to_string(logical_operator_clause) + ":");
		ass.add("\tcmpl $0, %eax");
		ass.add("\tmovl $0, %eax");
		ass.add("\tsetne %al");
		ass.add("_loc_end" + std::to_string(logical_operator_clause) + ":");
		logical_operator_clause++;
		return;
	}
	else if (op == logical_and) {
		ass.add("\tcmpl $0, %eax");
		ass.add("\tjne _loc" + std::to_string(logical_operator_clause));
		ass.add("jmp _loc_end" + std::to_string(logical_operator_clause));
		ass.add("_loc" + std::to_string(logical_operator_clause) + ":");
		ass.add("\tcmpl $0, %eax");
		ass.add("\tmovl $0, %eax");
		ass.add("\tsetne %al");
		ass.add("_loc_end" + std::to_string(logical_operator_clause) + ":");
		return;

	}
	right->generateAssembly(ass);
	ass.add("\tpush %rax");
	left->generateAssembly(ass);
	ass.add("\tpop %rcx");
	ass.add(binary_operator_assembly[{left->return_type, op, right->return_type}]);
}

void UnaryOperator::generateAssembly(assembly& ass)
{
	left->generateAssembly(ass);
	ass.add(unary_operator_assembly[{left->return_type, op}]);
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