#include "ast.h"
#include <map>

std::map<std::tuple<DataType, binary_operator, DataType>, assembly > binary_operator_assembly;
std::map<std::tuple<DataType, binary_operator, DataType>, DataType > binary_operator_result_type;

std::map<std::tuple<DataType, unary_operator>, assembly > unary_operator_assembly;
std::map<std::tuple<DataType, unary_operator>, DataType > unary_operator_result_type;

token check_token(std::queue<token>& tokens, token_type type) {
	token t = tokens.front();
	tokens.pop();
	if (t.type != type) throw std::exception("Incorrect token");
	return t;
}

DataType getDataType(std::queue<token>& tokens) {
	token t = tokens.front();
	tokens.pop();
	DataType d = DataType();
	if (t.type == INT_KEYWORD) d.id = 0;
	d.pointers = 0;
	while (tokens.front().type == BITWISE_AND) {
		tokens.pop();
		d.reference++;
	}
	return d;
}

inline BinaryOperator* create_binary_operator(Expression* exp1, Expression* exp2, binary_operator op) {
	return new BinaryOperator(op, exp1, exp2, binary_operator_result_type[{exp1->return_type, op, exp2->return_type}]);
}

inline UnaryOperator* create_unary_operator(Expression* exp1, unary_operator op) {
	return new UnaryOperator(op, exp1, unary_operator_result_type[{exp1->return_type, op}]);
}

// the functions with names compile_ followed by a number are named by the order of precedence of their operators
// according to https://en.cppreference.com/w/cpp/language/operator_precedence
Expression* compile_17(std::queue<token>& tokens);
Expression* compile_16(std::queue<token>& tokens);

Expression* compile_0(std::queue<token>& tokens) {
	if (tokens.front().type == INT_VALUE) {
		return new ConstantInt(stoi(check_token(tokens, INT_VALUE).value));
	}
	else {
		return new VariableRef(check_token(tokens, NAME).value);
	}
}

//::
//compile_1

// type()
// type{}
// a()
// a[]
// .
// ->
Expression* compile_2(std::queue<token>& tokens) {
	Expression* exp = compile_0(tokens);
	while (tokens.front().type == INCREMENT || tokens.front().type == DECREMENT || tokens.front().type == OPEN_PARENTHESES) {
		token t = tokens.front();
		tokens.pop();
		if (t.type == INCREMENT) {
			exp = create_unary_operator(exp, postfix_increment);
		}
		else if(t.type == DECREMENT) {
			exp = create_unary_operator(exp, postfix_decrement);
		}
		else {
			exp = new FunctionCall(((VariableRef*)exp)->name, DataType::INT);
			if (tokens.front().type != CLOSE_PARENTHESES) {
				((FunctionCall*)exp)->params.push_back(compile_16(tokens));
				while (tokens.front().type != CLOSE_PARENTHESES) {
					check_token(tokens, COMMA);
					((FunctionCall*)exp)->params.push_back(compile_16(tokens));
				}
			}
			tokens.pop();
		}
	}
	return exp;
}

// +_
// (type)_
// *_
// &_
// sizeof _
// new
// new[]
// delete
// delete[]
Expression* compile_3(std::queue<token>& tokens) {
	if (tokens.front().type == MINUS || tokens.front().type == BITWISE_COMPLEMENT || tokens.front().type == EXCLAMATION
		|| tokens.front().type == INCREMENT || tokens.front().type == DECREMENT) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp = compile_3(tokens);
		if (t.type == MINUS) {
			exp = create_unary_operator(exp, negation);
		}
		else if (t.type == BITWISE_COMPLEMENT) {
			exp = create_unary_operator(exp, bitwise_complement);
		}
		else if (t.type == INCREMENT) {
			exp = create_unary_operator(exp, prefix_increment);
		}
		else if (t.type == DECREMENT) {
			exp = create_unary_operator(exp, prefix_decrement);
		}
		else {
			exp = create_unary_operator(exp, logical_negation);
		}
		return exp;
	}
	else {
		return compile_2(tokens);
	}
}

// .*
// ->*
//compile_4

Expression* compile_5(std::queue<token>& tokens) {
	Expression* exp = compile_3(tokens);
	while (tokens.front().type == ASTERISK || tokens.front().type == SLASH || tokens.front().type == MODULUS) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = compile_3(tokens);
		if (t.type == ASTERISK) {
			exp = create_binary_operator(exp, exp2, multiply);
		}
		else if (t.type == SLASH) {
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

Expression* compile_7(std::queue<token>& tokens) {
	Expression* exp = compile_6(tokens);
	while (tokens.front().type == LEFT_SHIFT || tokens.front().type == RIGHT_SHIFT) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = compile_6(tokens);
		if (t.type == LEFT_SHIFT) {
			exp = create_binary_operator(exp, exp2, left_shift);
		}
		else {
			exp = create_binary_operator(exp, exp2, right_shift);
		}
	}
	return exp;
}

// <=>
//compile_8

Expression* compile_9(std::queue<token>& tokens) {
	Expression* exp = compile_7(tokens);
	while (tokens.front().type == LESS_THAN || tokens.front().type == LESS_OR_EQUAL_TO || tokens.front().type == GREATER_THAN || tokens.front().type == GREATER_OR_EQUAL_TO) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp2 = compile_7(tokens);
		if (t.type == LESS_THAN) {
			exp = create_binary_operator(exp, exp2, less);
		}
		else if (t.type == LESS_OR_EQUAL_TO) {
			exp = create_binary_operator(exp, exp2, less_equal);
		}
		else if (t.type == GREATER_THAN) {
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

Expression* compile_11(std::queue<token>& tokens) {
	Expression* exp = compile_10(tokens);
	while (tokens.front().type == BITWISE_AND) {
		tokens.pop();
		Expression* exp2 = compile_10(tokens);
		exp = create_binary_operator(exp, exp2, bitwise_and);
	}
	return exp;
}

Expression* compile_12(std::queue<token>& tokens) {
	Expression* exp = compile_11(tokens);
	while (tokens.front().type == BITWISE_XOR) {
		tokens.pop();
		Expression* exp2 = compile_11(tokens);
		exp = create_binary_operator(exp, exp2, bitwise_xor);
	}
	return exp;
}

Expression* compile_13(std::queue<token>& tokens) {
	Expression* exp = compile_12(tokens);
	while (tokens.front().type == BITWISE_OR) {
		tokens.pop();
		Expression* exp2 = compile_12(tokens);
		exp = create_binary_operator(exp, exp2, bitwise_or);
	}
	return exp;
}

Expression* compile_14(std::queue<token>& tokens) {
	Expression* exp = compile_13(tokens);
	while (tokens.front().type == LOGICAL_AND) {
		tokens.pop();
		Expression* exp2 = compile_13(tokens);
		exp = create_binary_operator(exp, exp2, logical_and);
	}
	return exp;
}

Expression* compile_15(std::queue<token>& tokens) {
	Expression* exp = compile_14(tokens);
	while (tokens.front().type == LOGICAL_OR) {
		tokens.pop();
		Expression* exp2 = compile_14(tokens);
		exp = create_binary_operator(exp, exp2, logical_or);
	}
	return exp;
}

// a?b:c
// throw _
// co_yield _
Expression* compile_16(std::queue<token>& tokens) {
	Expression* exp = compile_15(tokens);
	if (tokens.front().type == QUESTION_MARK) {
		tokens.pop();
		Expression* exp2 = compile_17(tokens);
		check_token(tokens, COLON);
		Expression* exp3 = compile_16(tokens);
		exp = new TernaryExpression(exp, exp2, exp3, exp2->return_type);
	}
	if (tokens.front().type == EQUAL_SIGN) {
		tokens.pop();
		Expression* exp2 = compile_16(tokens);
		exp = create_binary_operator(exp, exp2, assignment);
	}
	if (tokens.front().type == ADD_ASSIGN) {
		tokens.pop();
		Expression* exp2 = compile_16(tokens);
		exp = create_binary_operator(exp, exp2, add_assign);
	}
	if (tokens.front().type == SUBTRACT_ASSIGN) {
		tokens.pop();
		Expression* exp2 = compile_16(tokens);
		exp = create_binary_operator(exp, exp2, subtract_assign);
	}
	if (tokens.front().type == MULTIPLY_ASSIGN) {
		tokens.pop();
		Expression* exp2 = compile_16(tokens);
		exp = create_binary_operator(exp, exp2, multiply_assign);
	}
	if (tokens.front().type == DIVIDE_ASSIGN) {
		tokens.pop();
		Expression* exp2 = compile_16(tokens);
		exp = create_binary_operator(exp, exp2, divide_assign);
	}
	if (tokens.front().type == MOD_ASSIGN) {
		tokens.pop();
		Expression* exp2 = compile_16(tokens);
		exp = create_binary_operator(exp, exp2, mod_assign);
	}
	if (tokens.front().type == LEFT_SHIFT_ASSIGN) {
		tokens.pop();
		Expression* exp2 = compile_16(tokens);
		exp = create_binary_operator(exp, exp2, left_shift_assign);
	}
	if (tokens.front().type == RIGHT_SHIFT_ASSIGN) {
		tokens.pop();
		Expression* exp2 = compile_16(tokens);
		exp = create_binary_operator(exp, exp2, right_shift_assign);
	}
	if (tokens.front().type == AND_ASSIGN) {
		tokens.pop();
		Expression* exp2 = compile_16(tokens);
		exp = create_binary_operator(exp, exp2, and_assign);
	}
	if (tokens.front().type == OR_ASSIGN) {
		tokens.pop();
		Expression* exp2 = compile_16(tokens);
		exp = create_binary_operator(exp, exp2, or_assign);
	}
	if (tokens.front().type == XOR_ASSIGN) {
		tokens.pop();
		Expression* exp2 = compile_16(tokens);
		exp = create_binary_operator(exp, exp2, xor_assign);
	}

	return exp;
}

Expression* compile_17(std::queue<token>& tokens) {
	Expression* exp = compile_16(tokens);
	while (tokens.front().type == COMMA) {
		tokens.pop();
		exp = compile_16(tokens);
		//TODO: EVALUATE BOTH
	}
	return exp;
}

BlockItem* compile_block_item(std::queue<token>& tokens);

CodeBlock* compile_code_block(std::queue<token>& tokens) {
	check_token(tokens, OPEN_BRACES);
	CodeBlock* cb = new CodeBlock();
	while (tokens.front().type != CLOSE_BRACES) {
		cb->lines.push_back(compile_block_item(tokens));
	}
	check_token(tokens, CLOSE_BRACES);
	return cb;
}

LineOfCode* compile_line(std::queue<token>& tokens) {
	token t = tokens.front();
	if (t.type == RETURN_KEYWORD) {
		check_token(tokens, RETURN_KEYWORD);
		Return* r = new Return(compile_17(tokens));
		check_token(tokens, SEMICOLON);
		return r;
	}
	else if (t.type == IF_KEYWORD) {
		check_token(tokens, IF_KEYWORD);
		check_token(tokens, OPEN_PARENTHESES);
		Expression* cond_exp = compile_17(tokens);
		check_token(tokens, CLOSE_PARENTHESES);
		LineOfCode* if_cond = compile_line(tokens);
		LineOfCode* else_cond = nullptr;
		if (tokens.front().type == ELSE_KEYWORD) {
			check_token(tokens, ELSE_KEYWORD);
			else_cond = compile_line(tokens);
		}
		return new IfStatement(cond_exp, if_cond, else_cond);
	}
	else if (t.type == FOR_KEYWORD) {
		check_token(tokens, FOR_KEYWORD);
		check_token(tokens, OPEN_PARENTHESES);
		BlockItem* initial = nullptr;
		if (tokens.front().type == SEMICOLON) {
			check_token(tokens, SEMICOLON);
		}
		else if (tokens.front().type == INT_KEYWORD) {
			initial = (VariableDeclarationLine*)compile_block_item(tokens);
		}
		else {
			initial = new ExpressionLine(compile_17(tokens));
			check_token(tokens, SEMICOLON);
		}
		Expression* condition = tokens.front().type == SEMICOLON ? nullptr : compile_17(tokens);
		check_token(tokens, SEMICOLON);
		Expression* post = tokens.front().type == CLOSE_PARENTHESES ? nullptr : compile_17(tokens);
		check_token(tokens, CLOSE_PARENTHESES);
		LineOfCode* inner = compile_line(tokens);
		return new ForLoop(initial, condition, post, inner);
	}
	else if (t.type == WHILE_KEYWORD) {
		check_token(tokens, WHILE_KEYWORD);
		check_token(tokens, OPEN_PARENTHESES);
		Expression* condition = compile_17(tokens);
		check_token(tokens, CLOSE_PARENTHESES);
		LineOfCode* inner = compile_line(tokens);
		return new WhileLoop(condition, inner);
	}
	else if (t.type == DO_KEYWORD) {
		check_token(tokens, DO_KEYWORD);
		LineOfCode* inner = compile_line(tokens);
		check_token(tokens, WHILE_KEYWORD);
		check_token(tokens, OPEN_PARENTHESES);
		Expression* condition = compile_17(tokens);
		check_token(tokens, CLOSE_PARENTHESES);
		check_token(tokens, SEMICOLON);
		return new DoWhileLoop(condition, inner);
	}
	else if (t.type == BREAK_KEYWORD) {
		check_token(tokens, BREAK_KEYWORD);
		check_token(tokens, SEMICOLON);
		return new Break();
	}
	else if (t.type == CONTINUE_KEYWORD) {
		check_token(tokens, CONTINUE_KEYWORD);
		check_token(tokens, SEMICOLON);
		return new Continue();
	}
	else if (t.type == OPEN_BRACES) {
		return compile_code_block(tokens);
	}
	else if (t.type == SEMICOLON) {
		return new ExpressionLine(nullptr);
	}
	else {
		Expression* e = compile_17(tokens);
		check_token(tokens, SEMICOLON);
		return new ExpressionLine(e);
	}
}

BlockItem* compile_block_item(std::queue<token>& tokens) {
	token t = tokens.front();
	if (t.type == INT_KEYWORD) {
		DataType d = getDataType(tokens);
		std::string name = check_token(tokens, NAME).value;
		Expression* exp = nullptr;
		if (tokens.front().type == EQUAL_SIGN) {
			check_token(tokens, EQUAL_SIGN);
			exp = compile_17(tokens);
		}
		check_token(tokens, SEMICOLON);
		return new VariableDeclarationLine(exp, d, name);
	}
	else return compile_line(tokens);
}

Function* compile_function(std::queue<token>& tokens)
{
	Function* f = new Function();
	check_token(tokens, INT_KEYWORD);
	f->name = check_token(tokens, NAME).value;
	check_token(tokens, OPEN_PARENTHESES);
	if (tokens.front().type != CLOSE_PARENTHESES) {
		DataType dt = getDataType(tokens);
		std::string name = check_token(tokens, NAME).value;
		f->params.push_back({ name, dt });
		while (tokens.front().type == COMMA) {
			tokens.pop();
			DataType dt = getDataType(tokens);
			std::string name = check_token(tokens, NAME).value;
			f->params.push_back({ name, dt });
		}
	}
	check_token(tokens, CLOSE_PARENTHESES);
	if (tokens.front().type == SEMICOLON) tokens.pop();
	else f->lines = compile_code_block(tokens);
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
	DataType type;
};

struct scope {
	scope* parent;
	int current_variable_location = -8;
	std::map<std::string, variable> variables;
};

scope* curr_scope;

struct function {
	std::string name;
	std::vector<std::pair<std::string, DataType>> params;
	bool operator<(const function& other) const {
		if (name < other.name) return true;
		if (name > other.name) return false;
		return false;
	}
};

std::map<function, bool> functions;

void Application::generateAssembly(assembly& ass)
{
	for (Function* func : functions) {
		func->generateAssembly(ass);
	}
}

void CodeBlock::generateAssembly(assembly& ass) {
	curr_scope = new scope{ curr_scope, curr_scope->current_variable_location };
	for (BlockItem* line : lines) {
		line->generateAssembly(ass);
	}
	ass.add("\taddq $" + std::to_string(8 * curr_scope->variables.size()) + ", %rsp");
	curr_scope = curr_scope->parent;
}

void Function::generateAssembly(assembly& ass)
{
	functions.insert({ { name, params }, lines!=nullptr });
	if (lines == nullptr) return;
	curr_scope = new scope();
	curr_scope->current_variable_location = -8;
	ass.add(".globl " + name);
	ass.add(name + ":");
	ass.add("\tpush %rbp");
	ass.add("\tmovq %rsp, %rbp");
	for (int i = 0; i < params.size(); i++) {
		curr_scope->variables.insert({ params[i].first, {params[i].first, 8 * (i+2), params[i].second} });
	}
	lines->generateAssembly(ass);
}

void Return::generateAssembly(assembly& ass)
{
	expr->generateAssembly(ass);
	if (expr->return_type.reference) {
		ass.add("\tmovl (%eax), %eax");
	}
	ass.add("\tmovq %rbp, %rsp");
	ass.add("\tpop %rbp");
	ass.add("\tret");
}

void addBinaryOperator(DataType type1, binary_operator op, DataType type2, DataType return_type, assembly ass) {
	binary_operator_result_type[{type1, op, type2 }] = return_type;
	binary_operator_assembly[{type1, op, type2 }] = ass;
}

void addUnaryOperator(DataType type1, unary_operator op, DataType return_type, assembly ass) {
	unary_operator_result_type[{type1, op }] = return_type;
	unary_operator_assembly[{type1, op }] = ass;
}

void initAST() {
	addBinaryOperator(DataType::INT, add, DataType::INT, DataType::INT,
		assembly({ "\taddl %ecx, %eax" }));
	addBinaryOperator(DataType::INT, subtract, DataType::INT, DataType::INT,
		assembly({ "\tsubl %ecx, %eax" }));
	addBinaryOperator(DataType::INT, multiply, DataType::INT, DataType::INT,
		assembly({ "\timull %ecx, %eax" }));
	addBinaryOperator(DataType::INT, divide, DataType::INT, DataType::INT,
		assembly({ "\tmovl $0, %edx", "\tidivl %ecx" }));
	addBinaryOperator(DataType::INT, mod, DataType::INT, DataType::INT,
		assembly({ "\tmovl $0, %edx", "\tidivl %ecx", "\tmovl %edx, %eax" }));

	addBinaryOperator(DataType::INT, equal, DataType::INT, DataType::INT,
		assembly({ "\tcmpl %ecx, %eax", "\tmovl $0, %eax", "\tsete %al" }));
	addBinaryOperator(DataType::INT, not_equal, DataType::INT, DataType::INT,
		assembly({ "\tcmpl %ecx, %eax", "\tmovl $0, %eax", "\tsetne %al" }));
	addBinaryOperator(DataType::INT, less, DataType::INT, DataType::INT,
		assembly({ "\tcmpl %ecx, %eax", "\tmovl $0, %eax", "\tsetl %al" }));
	addBinaryOperator(DataType::INT, greater, DataType::INT, DataType::INT,
		assembly({ "\tcmpl %ecx, %eax", "\tmovl $0, %eax", "\tsetg %al" }));
	addBinaryOperator(DataType::INT, less_equal, DataType::INT, DataType::INT,
		assembly({ "\tcmpl %ecx, %eax", "\tmovl $0, %eax", "\tsetle %al" }));
	addBinaryOperator(DataType::INT, greater_equal, DataType::INT, DataType::INT,
		assembly({ "\tcmpl %ecx, %eax", "\tmovl $0, %eax", "\tsetge %al" }));

	addBinaryOperator(DataType::INT, left_shift, DataType::INT, DataType::INT,
		assembly({ "\tsall %cl, %eax" }));
	addBinaryOperator(DataType::INT, right_shift, DataType::INT, DataType::INT,
		assembly({ "\tsarl %cl, %eax" }));

	addBinaryOperator(DataType::INT, bitwise_xor, DataType::INT, DataType::INT,
		assembly({ "\txor %ecx, %eax" }));
	addBinaryOperator(DataType::INT, bitwise_or, DataType::INT, DataType::INT,
		assembly({ "\tor %ecx, %eax" }));
	addBinaryOperator(DataType::INT, bitwise_and, DataType::INT, DataType::INT,
		assembly({ "\tand %ecx, %eax" }));

	addBinaryOperator(DataType::INT_REF, assignment, DataType::INT, DataType::INT_REF,
		assembly({ "\tmovl %ecx, (%eax)" }));
	addBinaryOperator(DataType::INT_REF, add_assign, DataType::INT, DataType::INT_REF,
		assembly({ "\taddl %ecx, (%eax)" }));
	addBinaryOperator(DataType::INT_REF, subtract_assign, DataType::INT, DataType::INT_REF,
		assembly({ "\tsubl %ecx, (%eax)" }));
	addBinaryOperator(DataType::INT_REF, multiply_assign, DataType::INT, DataType::INT_REF,
		assembly({ "\timull %ecx, (%eax)" }));
	addBinaryOperator(DataType::INT_REF, divide_assign, DataType::INT, DataType::INT_REF,
		assembly({ "\tmovl %eax, %r9d", "\tmovl $0, %edx", "\tidivl %ecx", "\tmovl %eax, (%r9d)", "\tmovl %r9d, %eax" }));
	addBinaryOperator(DataType::INT_REF, mod_assign, DataType::INT, DataType::INT_REF,
		assembly({ "\tmovl %eax, %r9d", "\tmovl $0, %edx", "\tidivl %ecx", "\tmovl %edx, (%r9d)", "\tmovl %r9d, %eax" }));

	addBinaryOperator(DataType::INT_REF, left_shift_assign, DataType::INT, DataType::INT_REF,
		assembly({ "\tsall %cl, (%eax)" }));
	addBinaryOperator(DataType::INT_REF, right_shift_assign, DataType::INT, DataType::INT_REF,
		assembly({ "\tsarl %cl, (%eax)" }));

	addBinaryOperator(DataType::INT_REF, xor_assign, DataType::INT, DataType::INT_REF,
		assembly({ "\txor %ecx, (%eax)" }));
	addBinaryOperator(DataType::INT_REF, or_assign, DataType::INT, DataType::INT_REF,
		assembly({ "\tor %ecx, (%eax)" }));
	addBinaryOperator(DataType::INT_REF, and_assign, DataType::INT, DataType::INT_REF,
		assembly({ "\tand %ecx, (%eax)" }));

	addUnaryOperator(DataType::INT, negation, DataType::INT,
		assembly({ "\tneg %eax" }));
	addUnaryOperator(DataType::INT, bitwise_complement, DataType::INT,
		assembly({ "\tnot %eax" }));
	addUnaryOperator(DataType::INT, logical_negation, DataType::INT,
		assembly({ "\tcmpl $0, %eax", "\tmovl $0, %eax", "\tsete %al" }));

	addUnaryOperator(DataType::INT_REF, prefix_increment, DataType::INT_REF,
		assembly({ "\tincl (%eax)" }));
	addUnaryOperator(DataType::INT_REF, prefix_decrement, DataType::INT_REF,
		assembly({ "\tdecl (%eax)" }));

	addUnaryOperator(DataType::INT_REF, postfix_increment, DataType::INT,
		assembly({ "\tmovl (%eax), %ecx", "\tincl (%eax)", "\tmovl %ecx, %eax" }));
	addUnaryOperator(DataType::INT_REF, postfix_decrement, DataType::INT,
		assembly({ "\tmovl (%eax), %ecx", "\tdecl (%eax)", "\tmovl %ecx, %eax" }));
}

void BinaryOperator::generateAssembly(assembly& ass)
{
	static int logical_operator_clause = 0;
	if (op == logical_or) {
		int logical_operator_cl = logical_operator_clause++;
		right->generateAssembly(ass);
		ass.add("\tcmpl $0, %eax");
		ass.add("\tje _loc" + std::to_string(logical_operator_cl));
		ass.add("\tmovl $1, %eax");
		ass.add("jmp _loc_end" + std::to_string(logical_operator_cl));
		ass.add("_loc" + std::to_string(logical_operator_cl) + ":");
		ass.add("\tcmpl $0, %eax");
		ass.add("\tmovl $0, %eax");
		ass.add("\tsetne %al");
		ass.add("_loc_end" + std::to_string(logical_operator_cl) + ":");
		return;
	}
	else if (op == logical_and) {
		int logical_operator_cl = logical_operator_clause++;
		ass.add("\tcmpl $0, %eax");
		ass.add("\tjne _loc" + std::to_string(logical_operator_cl));
		ass.add("jmp _loc_end" + std::to_string(logical_operator_cl));
		ass.add("_loc" + std::to_string(logical_operator_cl) + ":");
		ass.add("\tcmpl $0, %eax");
		ass.add("\tmovl $0, %eax");
		ass.add("\tsetne %al");
		ass.add("_loc_end" + std::to_string(logical_operator_cl) + ":");
		return;

	}
	right->generateAssembly(ass);
	ass.add("\tpush %rax");
	left->generateAssembly(ass);
	ass.add("\tpop %rcx");
	DataType l = left->return_type;
	DataType r = right->return_type;
	while (binary_operator_assembly.find({ l, op, r }) == binary_operator_assembly.end() && r.reference) {
		r.reference--;
		ass.add("\tmovl (%ecx), %ecx");
	}
	while (binary_operator_assembly.find({ l, op, r }) == binary_operator_assembly.end() && l.reference) {
		l.reference--;
		ass.add("\tmovl (%eax), %eax");
	}
	ass.add(binary_operator_assembly[{l, op, r}]);
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
	if(exp)
		exp->generateAssembly(ass);
}

void VariableDeclarationLine::generateAssembly(assembly& ass)
{
	if (init_exp == nullptr) {
		ass.add("\tmovl $0, %eax");
	}
	else {
		init_exp->generateAssembly(ass);
		int ref = init_exp->return_type.reference;
		while (ref > var_type.reference) {
			ass.add("\tmovl (%eax), %eax");
			ref--;
		}
	}
	curr_scope->variables.insert({ name, {name, curr_scope->current_variable_location, var_type} });
	curr_scope->current_variable_location -= 8;
	ass.add("\tpush %rax");
}

void VariableRef::generateAssembly(assembly& ass)
{
	scope* sc = curr_scope;
	while (sc && sc->variables.find(name) == sc->variables.end()) {
		sc = sc->parent;
	}
	if (sc) {
		return_type = sc->variables[name].type;
		return_type.reference++;
		ass.add("\tleal " + std::to_string(sc->variables[name].location) + "(%rbp), %eax");
	}
	else
		; //could not find variable

}

void IfStatement::generateAssembly(assembly& ass)
{
	static int if_clause = 0;
	int if_cl = if_clause++;
	condition->generateAssembly(ass);
	ass.add("\tcmpl $0, %eax");
	ass.add("\tje _e3_if_" + std::to_string(if_cl));
	if_cond->generateAssembly(ass);
	ass.add("\tjmp _post_conditional_if_" + std::to_string(if_cl));
	ass.add("_e3_if_" + std::to_string(if_cl) + ":");
	if (else_cond) else_cond->generateAssembly(ass);
	ass.add("_post_conditional_if_" + std::to_string(if_cl) + ":");
}

void TernaryExpression::generateAssembly(assembly& ass)
{
	static int ternary_clause = 0;
	int ternary_cl = ternary_clause++;
	condition->generateAssembly(ass);
	ass.add("\tcmpl $0, %eax");
	ass.add("\tje _e3_" + std::to_string(ternary_cl));
	if_cond->generateAssembly(ass);
	ass.add("\tjmp _post_conditional_" + std::to_string(ternary_cl));
	ass.add("_e3_" + std::to_string(ternary_cl) + ":");
	else_cond->generateAssembly(ass);
	ass.add("_post_conditional_" + std::to_string(ternary_cl) + ":");
}

struct loop_scope {
	loop_scope* parent;
	LineType type;
	int id;
};

loop_scope* curr_loop_scope;

void WhileLoop::generateAssembly(assembly& ass) {
	static int while_clause = 0;
	int while_cl = while_clause++;
	curr_loop_scope = new loop_scope{ curr_loop_scope, LineType::While, while_cl };

	ass.add("_while_start_" + std::to_string(while_cl) + ":");
	condition->generateAssembly(ass);
	ass.add("\tcmpl $0, %eax");
	ass.add("\tje _while_end_" + std::to_string(while_cl));
	inner->generateAssembly(ass);
	ass.add("\tjmp _while_start_" + std::to_string(while_cl));
	ass.add("_while_end_" + std::to_string(while_cl) + ":");

	curr_loop_scope = curr_loop_scope->parent;
}

void DoWhileLoop::generateAssembly(assembly& ass) {
	static int do_while_clause = 0;
	int do_while_cl = do_while_clause++;
	curr_loop_scope = new loop_scope{ curr_loop_scope, LineType::DoWhile, do_while_cl };

	ass.add("_do_while_start_" + std::to_string(do_while_cl) + ":");
	inner->generateAssembly(ass);
	condition->generateAssembly(ass);
	ass.add("\tcmpl $0, %eax");
	ass.add("\tje _do_while_end_" + std::to_string(do_while_cl));
	ass.add("\tjmp _do_while_start_" + std::to_string(do_while_cl));
	ass.add("_do_while_end_" + std::to_string(do_while_cl) + ":");

	curr_loop_scope = curr_loop_scope->parent;
}

void ForLoop::generateAssembly(assembly& ass) {
	static int for_clause = 0;
	int for_cl = for_clause++;

	curr_scope = new scope{ curr_scope, curr_scope->current_variable_location };
	curr_loop_scope = new loop_scope{ curr_loop_scope, LineType::For, for_cl };

	if (initial) initial->generateAssembly(ass);
	ass.add("_for_start_" + std::to_string(for_cl) + ":");
	if (condition) condition->generateAssembly(ass);
	else ass.add("\tmovl $1, %eax");
	ass.add("\tcmpl $0, %eax");
	ass.add("\tje _for_end_" + std::to_string(for_cl));
	inner->generateAssembly(ass);
	ass.add("_for_continue_" + std::to_string(for_cl) + ":");
	if(post) post->generateAssembly(ass);
	ass.add("\tjmp _for_start_" + std::to_string(for_cl));
	ass.add("_for_end_" + std::to_string(for_cl) + ":");

	ass.add("\taddq $" + std::to_string(8 * curr_scope->variables.size()) + ", %rsp");
	curr_scope = curr_scope->parent;

	curr_loop_scope = curr_loop_scope->parent;
}

void Break::generateAssembly(assembly& ass) {
	if (!curr_loop_scope) return; // trying to break when there is no loop
	if (curr_loop_scope->type == LineType::While) {
		ass.add("\tjmp _while_end_" + std::to_string(curr_loop_scope->id));
	}
	if (curr_loop_scope->type == LineType::DoWhile) {
		ass.add("\tjmp _do_while_end_" + std::to_string(curr_loop_scope->id));
	}
	if (curr_loop_scope->type == LineType::For) {
		ass.add("\tjmp _for_end_" + std::to_string(curr_loop_scope->id));
	}
}

void Continue::generateAssembly(assembly& ass) {
	if (!curr_loop_scope) return; // trying to break when there is no loop
	if (curr_loop_scope->type == LineType::While) {
		ass.add("\tjmp _while_start_" + std::to_string(curr_loop_scope->id));
	}
	if (curr_loop_scope->type == LineType::DoWhile) {
		ass.add("\tjmp _do_while_start_" + std::to_string(curr_loop_scope->id));
	}
	if (curr_loop_scope->type == LineType::For) {
		ass.add("\tjmp _for_continue_" + std::to_string(curr_loop_scope->id));
	}
}

void FunctionCall::generateAssembly(assembly& ass) {
	if (functions.find({ name }) == functions.end()) return; //function does not exist
	function f = functions.find({name})->first;
	ass.add("\tsubq $" + std::to_string(std::max(32u, 8 * params.size())) + ", %rsp");
	for (int i = 0; i < params.size(); i++) {
		params[i]->generateAssembly(ass);
		while (params[i]->return_type.reference > f.params[i].second.reference) {
			params[i]->return_type.reference--;
			ass.add("\tmovl (%eax), %eax");
		}
		ass.add("\tmovq %rax, " + std::to_string(8 * i) + "(%rsp)");
	}
	if(params.size() > 0) ass.add("\tmovq 0(%rsp), %rcx");
	if (params.size() > 1) ass.add("\tmovq 8(%rsp), %rdx");
	if (params.size() > 2) ass.add("\tmovq 16(%rsp), %r8");
	if (params.size() > 3) ass.add("\tmovq 24(%rsp), %r9");
	ass.add("\tcall " + name);
	ass.add("\taddq $" + std::to_string(std::max(32u, 8 * params.size())) + ", %rsp");
}