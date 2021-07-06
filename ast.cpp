#include "ast.h"
#include <map>

std::map<std::tuple<DataType, binary_operator, DataType>, assembly > binary_operator_assembly;
std::map<std::tuple<DataType, binary_operator, DataType>, DataType > binary_operator_result_type;

std::map<std::tuple<DataType, unary_operator>, assembly > unary_operator_assembly;
std::map<std::tuple<DataType, unary_operator>, DataType > unary_operator_result_type;

struct _field {
	std::string name;
	DataType type;
	int offset;
};

struct _struct {
	int id;
	int size; // bytes
	std::string name;
	std::vector<_field> fields;
	std::map<std::string, _field> fields_by_name;
	bool operator<(const _struct& other) const {
		if (name < other.name) return true;
		if (name > other.name) return false;
		return false;
	}
};

std::map<int, _struct> struct_by_data_type_id;
std::map<std::string, _struct> struct_by_name;

token check_token(std::queue<token>& tokens, token_type type) {
	token t = tokens.front();
	tokens.pop();
	if (t.type != type) throw std::exception("Incorrect token");
	return t;
}

DataType getDataType(std::queue<token>& tokens) {
	token t = tokens.front();
	tokens.pop();
	DataType d;
	if (t.type == INT_KEYWORD) d = DataType::INT;
	if (t.type == CHAR_KEYWORD) d = DataType::CHAR;
	if (t.type == SHORT_KEYWORD) d = DataType::SHORT;
	if (t.type == LONG_KEYWORD) d = DataType::LONG;
	if (t.type == NAME) {
		d.id = struct_by_name[t.value].id;
		d.sz = struct_by_name[t.value].size;
	}
	while (tokens.front().type == ASTERISK) {
		tokens.pop();
		d.pointers++;
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
	if (tokens.front().type == OPEN_PARENTHESES) {
		tokens.pop();
		Expression* expr = compile_17(tokens);
		check_token(tokens, CLOSE_PARENTHESES);
		return expr;
	} else if (tokens.front().type == INT_VALUE) {
		return new ConstantInt(stoi(check_token(tokens, INT_VALUE).value));
	}
	else if (tokens.front().type == CHAR_VALUE) {
		std::string s = check_token(tokens, CHAR_VALUE).value;
		s = s.substr(1, s.length() - 2);
		if (s == "\\n") return new ConstantChar('\n');
		if (s == "\\t") return new ConstantChar('\t');
		if (s == "\\r") return new ConstantChar('\r');
		if (s == "\\f") return new ConstantChar('\f');
		return new ConstantChar(s[0]);
	}
	else if (tokens.front().type == STRING_VALUE) {
		std::string s = check_token(tokens, STRING_VALUE).value;
		s = s.substr(1, s.length() - 2);
		std::string s2 = s;
		size_t off = 0;
		while ((off = s2.find('\\', off)) != std::string::npos) {
			s2.erase(s2.begin() + off);
			if (s2[off] == 'n') s2[off] = '\n';
			if (s2[off] == 't') s2[off] = '\t';
			if (s2[off] == 'r') s2[off] = '\r';
			if (s2[off] == 'f') s2[off] = '\f';
		}
		return new ConstantString(s2);
	}
	else if (tokens.front().type == SHORT_VALUE) {
		std::string s = check_token(tokens, SHORT_VALUE).value;
		s.substr(0, s.length() - 1);
		return new ConstantShort(stoll(s));
	}
	else if (tokens.front().type == LONG_VALUE) {
		std::string s = check_token(tokens, LONG_VALUE).value;
		s.substr(0, s.length() - 1);
		return new ConstantLong(stoll(s));
	}
	else {
		return new VariableRef(check_token(tokens, NAME).value);
	}
}

//::
//compile_1

// type()
// type{}
// .
// ->
Expression* compile_2(std::queue<token>& tokens) {
	Expression* exp = compile_0(tokens);
	while (tokens.front().type == INCREMENT || tokens.front().type == DECREMENT || tokens.front().type == OPEN_PARENTHESES
		|| tokens.front().type == OPEN_BRACKET || tokens.front().type == DOT) {
		token t = tokens.front();
		tokens.pop();
		if (t.type == INCREMENT) {
			exp = create_unary_operator(exp, postfix_increment);
		}
		else if (t.type == DECREMENT) {
			exp = create_unary_operator(exp, postfix_decrement);
		}
		else if (t.type == DOT) {
			std::string name = check_token(tokens, NAME).value;
			exp = new MemberAccess(exp, name, DataType::INT);
		}
		else if (t.type == OPEN_BRACKET) {
			Expression* exp2 = compile_17(tokens);
			check_token(tokens, CLOSE_BRACKET);
			exp = create_binary_operator(exp, exp2, add);
			exp = create_unary_operator(exp, dereference);
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

// (type)_
// sizeof _
// new
// new[]
// delete
// delete[]
Expression* compile_3(std::queue<token>& tokens) {
	if (tokens.front().type == MINUS || tokens.front().type == PLUS || tokens.front().type == BITWISE_COMPLEMENT || tokens.front().type == EXCLAMATION
		|| tokens.front().type == INCREMENT || tokens.front().type == DECREMENT || tokens.front().type == BITWISE_AND
		|| tokens.front().type == ASTERISK) {
		token t = tokens.front();
		tokens.pop();
		Expression* exp = compile_3(tokens);
		if (t.type == MINUS) {
			exp = create_unary_operator(exp, negation);
		}
		else if (t.type == PLUS) {
			exp = create_unary_operator(exp, plus);
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
		else if (t.type == BITWISE_AND) {
			exp = create_unary_operator(exp, address);
		}
		else if (t.type == ASTERISK) {
			exp = create_unary_operator(exp, dereference);
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
		else if (tokens.front().type == INT_KEYWORD || tokens.front().type == LONG_KEYWORD ||
			tokens.front().type == CHAR_KEYWORD || tokens.front().type == SHORT_KEYWORD) {
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

VariableDeclarationLine* compile_var_decl(std::queue<token>& tokens) {
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

BlockItem* compile_block_item(std::queue<token>& tokens) {
	token t = tokens.front();
	if (t.type == INT_KEYWORD || t.type == LONG_KEYWORD || t.type == CHAR_KEYWORD || t.type == SHORT_KEYWORD || 
		(t.type==NAME && struct_by_name.find(t.value)!=struct_by_name.end())) {
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

void compile_struct(std::queue<token>& tokens)
{
	static int id = 5;

	_struct struc;
	struc.id = id++;

	check_token(tokens, STRUCT_KEYWORD);

	struc.name = check_token(tokens, NAME).value;

	check_token(tokens, OPEN_BRACES);

	int offset = 0;
	while (tokens.front().type != CLOSE_BRACES) {
		VariableDeclarationLine* var = compile_var_decl(tokens);

		_field f;
		f.name = var->name;
		f.offset = offset;
		offset -= 8;
		f.type = var->var_type;
		struc.fields.push_back(f);
		struc.fields_by_name[f.name] = f;

	}
	struc.size = -offset;

	check_token(tokens, CLOSE_BRACES);
	check_token(tokens, SEMICOLON);

	struct_by_data_type_id[struc.id] = struc;
	struct_by_name[struc.name] = struc;
	
	return;
}
Function* compile_function(std::queue<token>& tokens)
{
	Function* f = new Function();
	check_token(tokens, INT_KEYWORD);
	f->name = check_token(tokens, NAME).value;
	check_token(tokens, OPEN_PARENTHESES);
	if (tokens.front().type != CLOSE_PARENTHESES) {
		DataType dt = getDataType(tokens);
		if (dt.id > 4) dt.lvalue = true;
		std::string name = check_token(tokens, NAME).value;
		f->params.push_back({ name, dt });
		while (tokens.front().type == COMMA) {
			tokens.pop();
			DataType dt = getDataType(tokens);
			if (dt.id > 4) dt.lvalue = true;
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
	while (!tokens.empty()) {
		if (tokens.front().type == INT_KEYWORD)
			a->functions.push_back(compile_function(tokens));
		else
			compile_struct(tokens);
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
	functions.insert({ { name, params }, lines != nullptr });
	if (lines == nullptr) return;
	curr_scope = new scope();
	curr_scope->current_variable_location = -8;
	ass.add(".globl " + name);
	ass.add(name + ":");
	ass.add("\tpush %rbp");
	ass.add("\tmovq %rsp, %rbp");
	for (int i = 0; i < params.size(); i++) {
		curr_scope->variables.insert({ params[i].first, {params[i].first, 8 * (i + 2), params[i].second} });
	}
	lines->generateAssembly(ass);
}

void Return::generateAssembly(assembly& ass)
{
	expr->generateAssembly(ass);
	if (expr->return_type.lvalue) {
		ass.add("\tmovq (%rax), %rax");
	}
	ass.add("\tmovq %rbp, %rsp");
	ass.add("\tpop %rbp");
	ass.add("\tret");
}

void Struct::generateAssembly(assembly& ass) {
	_struct struc;
	struc.name = name;
	int offset = 0;
	for (VariableDeclarationLine* var : fields) {
		_field f;
		f.name = var->name;
		f.offset = offset;
		offset += 8;
		f.type = var->var_type;
		struc.fields.push_back(f);
		struc.fields_by_name[name] = f;
	}
	struct_by_data_type_id[5 + struct_by_data_type_id.size()] = struc;
}

void MemberAccess::generateAssembly(assembly& ass) {
	left->generateAssembly(ass);
	if (left->return_type.lvalue) {
		ass.add("\tsubq $"+ std::to_string(struct_by_data_type_id[left->return_type.id].fields_by_name[right].offset)+", %rax");
		return_type = struct_by_data_type_id[left->return_type.id].fields_by_name[right].type;
		return_type.lvalue = true;
	}
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

	size sizes[] = { i8, i16, i32, i64 };
	DataType normal[] = { DataType::CHAR, DataType::SHORT, DataType::INT, DataType::LONG };
	DataType lvalue[] = { DataType::CHAR, DataType::SHORT, DataType::INT, DataType::LONG };
	for (int i = 0; i < 4; i++) {
		lvalue[i].lvalue = true;
	}
	DataType pointer[] = { DataType::CHAR_PTR, DataType::SHORT_PTR, DataType::INT_PTR, DataType::LONG_PTR };
	DataType lvalue_pointer[] = { DataType::CHAR_PTR, DataType::SHORT_PTR, DataType::INT_PTR, DataType::LONG_PTR };
	for (int i = 0; i < 4; i++) {
		lvalue_pointer[i].lvalue = true;
	}

	for (int i = 0; i < 4; i++) {
		addBinaryOperator(normal[i], add, normal[i], normal[i],
			assembly().add("add", sizes[i], rcx, rax));
		addBinaryOperator(normal[i], subtract, normal[i], normal[i],
			assembly().add("sub", sizes[i], rcx, rax));
		addBinaryOperator(normal[i], multiply, normal[i], normal[i],
			assembly().add("imul", sizes[i], rcx, rax));
		addBinaryOperator(normal[i], divide, normal[i], normal[i],
			assembly().add("mov", sizes[i], 0, rdx).add("idiv", sizes[i], rcx));
		addBinaryOperator(normal[i], mod, normal[i], normal[i],
			assembly().add("mov", sizes[i], 0, rdx).add("idiv", sizes[i], rcx).add("mov", sizes[i], rdx, rax));

		addBinaryOperator(normal[i], equal, normal[i], normal[i],
			assembly().add("cmp", sizes[i], rcx, rax).add("mov", sizes[i], 0, rax).add("sete", i8, rax));
		addBinaryOperator(normal[i], not_equal, normal[i], normal[i],
			assembly().add("cmp", sizes[i], rcx, rax).add("mov", sizes[i], 0, rax).add("setne", i8, rax));
		addBinaryOperator(normal[i], less, normal[i], normal[i],
			assembly().add("cmp", sizes[i], rcx, rax).add("mov", sizes[i], 0, rax).add("setl", i8, rax));
		addBinaryOperator(normal[i], greater, normal[i], normal[i],
			assembly().add("cmp", sizes[i], rcx, rax).add("mov", sizes[i], 0, rax).add("setg", i8, rax));
		addBinaryOperator(normal[i], less_equal, normal[i], normal[i],
			assembly().add("cmp", sizes[i], rcx, rax).add("mov", sizes[i], 0, rax).add("setle", i8, rax));
		addBinaryOperator(normal[i], greater_equal, normal[i], normal[i],
			assembly().add("cmp", sizes[i], rcx, rax).add("mov", sizes[i], 0, rax).add("setge", i8, rax));

		addBinaryOperator(normal[i], left_shift, normal[i], normal[i],
			assembly({ "\tsal" + _suffix(sizes[i]) + " %cl, %" + _register(rax, sizes[i]) }));
		addBinaryOperator(normal[i], right_shift, normal[i], normal[i],
			assembly({ "\tsar" + _suffix(sizes[i]) + " %cl, %" + _register(rax, sizes[i]) }));

		addBinaryOperator(normal[i], bitwise_xor, normal[i], normal[i],
			assembly().add("xor", sizes[i], rcx, rax));
		addBinaryOperator(normal[i], bitwise_or, normal[i], normal[i],
			assembly().add("or", sizes[i], rcx, rax));
		addBinaryOperator(normal[i], bitwise_and, normal[i], normal[i],
			assembly().add("and", sizes[i], rcx, rax));
	}

	for (int i = 0; i < 4; i++) {

		addBinaryOperator(lvalue_pointer[i], assignment, pointer[i], lvalue_pointer[i],
			assembly().add("mov", i64, rcx, rax, false, true));

		addBinaryOperator(lvalue[i], assignment, normal[i], lvalue[i],
			assembly().add("mov", sizes[i], rcx, rax, false, true));
		addBinaryOperator(lvalue[i], add_assign, normal[i], lvalue[i],
			assembly().add("add", sizes[i], rcx, rax, false, true));
		addBinaryOperator(lvalue[i], subtract_assign, normal[i], lvalue[i],
			assembly().add("sub", sizes[i], rcx, rax, false, true));
		addBinaryOperator(lvalue[i], multiply_assign, normal[i], lvalue[i],
			assembly().add("imul", sizes[i], rcx, rax, false, true));
		addBinaryOperator(lvalue[i], divide_assign, normal[i], lvalue[i],
			assembly().add("mov", sizes[i], rax, r9).add("mov", sizes[i], 0, rdx).add("idiv", sizes[i], rcx)
			.add("mov", sizes[i], rax, r9, false, true).add("mov", sizes[i], r9, rax));
		addBinaryOperator(lvalue[i], mod_assign, normal[i], lvalue[i],
			assembly().add("mov", sizes[i], rax, r9).add("mov", sizes[i], 0, rdx).add("idiv", sizes[i], rcx)
			.add("mov", sizes[i], rdx, r9, false, true).add("mov", sizes[i], r9, rax));

		addBinaryOperator(lvalue[i], left_shift_assign, normal[i], lvalue[i],
			assembly({ "\tsal" + _suffix(sizes[i]) + " %cl, (%" + _register(rax, sizes[i]) + ")" }));
		addBinaryOperator(lvalue[i], right_shift_assign, normal[i], lvalue[i],
			assembly({ "\tsar" + _suffix(sizes[i]) + " %cl, (%" + _register(rax, sizes[i]) + ")" }));

		addBinaryOperator(lvalue[i], xor_assign, normal[i], lvalue[i],
			assembly().add("xor", sizes[i], rcx, rax, false, true));
		addBinaryOperator(lvalue[i], or_assign, normal[i], lvalue[i],
			assembly().add("or", sizes[i], rcx, rax, false, true));
		addBinaryOperator(lvalue[i], and_assign, normal[i], lvalue[i],
			assembly().add("and", sizes[i], rcx, rax, false, true));
	}

	for (int i = 0; i < 4; i++) {
		addUnaryOperator(normal[i], negation, normal[i],
			assembly());
		addUnaryOperator(normal[i], negation, normal[i],
			assembly().add("neg", sizes[i], rax));
		addUnaryOperator(normal[i], bitwise_complement, normal[i],
			assembly().add("not", sizes[i], rax));
		addUnaryOperator(normal[i], logical_negation, normal[i],
			assembly().add("cmp", sizes[i], 0, rax).add("mov", sizes[i], 0, rax).add("sete", i8, rax));

		addUnaryOperator(lvalue[i], prefix_increment, lvalue[i],
			assembly().add("inc", sizes[i], rax, true));
		addUnaryOperator(lvalue[i], prefix_decrement, lvalue[i],
			assembly().add("dec", sizes[i], rax, true));

		addUnaryOperator(lvalue[i], postfix_increment, normal[i],
			assembly().add("mov", sizes[i], rax, rcx, true).add("inc", sizes[i], rax, true).add("mov", sizes[i], rcx, rax));
		addUnaryOperator(lvalue[i], postfix_decrement, normal[i],
			assembly().add("mov", sizes[i], rax, rcx, true).add("dec", sizes[i], rax, true).add("mov", sizes[i], rcx, rax));

		addUnaryOperator(lvalue_pointer[i], prefix_increment, lvalue_pointer[i],
			assembly().add("inc", i64, rax, true));
		addUnaryOperator(lvalue_pointer[i], prefix_decrement, lvalue_pointer[i],
			assembly().add("dec", i64, rax, true));

		addUnaryOperator(lvalue_pointer[i], postfix_increment, lvalue_pointer[i],
			assembly().add("mov", i64, rax, rcx, true).add("inc", i64, rax, true).add("mov", i64, rcx, rax));
		addUnaryOperator(lvalue_pointer[i], postfix_decrement, lvalue_pointer[i],
			assembly().add("mov", i64, rax, rcx, true).add("dec", i64, rax, true).add("mov", i64, rcx, rax));
	}
	for (int i = 0; i < 4; i++) {
		addUnaryOperator(lvalue[i], address, pointer[i], assembly());
		addUnaryOperator(pointer[i], dereference, lvalue[i], assembly());
	}
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			addBinaryOperator(pointer[i], add, normal[j], pointer[i],
				assembly().add("imul", i64, _bytes(sizes[i]), rcx).add("add", i64, rcx, rax));
		}
	}
}

void BinaryOperator::generateAssembly(assembly& ass)
{
	static int logical_operator_clause = 0;

	if (left->return_type.id <= 4 && right->return_type.id <= 4) {
		if (op == logical_or) {
			int logical_operator_cl = logical_operator_clause++;
			right->generateAssembly(ass);
			ass.add("\tcmpq $0, %rax");
			ass.add("\tje _loc" + std::to_string(logical_operator_cl));
			ass.add("\tmovl $1, %eax");
			ass.add("jmp _loc_end" + std::to_string(logical_operator_cl));
			ass.add("_loc" + std::to_string(logical_operator_cl) + ":");
			ass.add("\tcmpq $0, %rax");
			ass.add("\tmovl $0, %eax");
			ass.add("\tsetne %al");
			ass.add("_loc_end" + std::to_string(logical_operator_cl) + ":");
			return;
		}
		else if (op == logical_and) {
			int logical_operator_cl = logical_operator_clause++;
			ass.add("\tcmpq $0, %rax");
			ass.add("\tjne _loc" + std::to_string(logical_operator_cl));
			ass.add("jmp _loc_end" + std::to_string(logical_operator_cl));
			ass.add("_loc" + std::to_string(logical_operator_cl) + ":");
			ass.add("\tcmpq $0, %rax");
			ass.add("\tmovl $0, %eax");
			ass.add("\tsetne %al");
			ass.add("_loc_end" + std::to_string(logical_operator_cl) + ":");
			return;
		}
	}

	right->generateAssembly(ass);
	ass.add("\tpush %rax");
	left->generateAssembly(ass);
	ass.add("\tpop %rcx");

	DataType l = left->return_type;
	DataType r = right->return_type;

	if (binary_operator_assembly.find({ l, op, r }) == binary_operator_assembly.end() && r.lvalue) {
		r.lvalue = false;
		ass.add("\tmovq (%rcx), %rcx");
	}
	if (binary_operator_assembly.find({ l, op, r }) == binary_operator_assembly.end() && l.lvalue) {
		l.lvalue = false;
		ass.add("\tmovq (%rax), %rax");
	}

	ass.add(binary_operator_assembly[{l, op, r}]);
	return_type = binary_operator_result_type[{ l, op, r }];
}

void UnaryOperator::generateAssembly(assembly& ass)
{
	left->generateAssembly(ass);
	DataType l = left->return_type;
	if (unary_operator_assembly.find({ l, op }) == unary_operator_assembly.end() && l.lvalue) {
		l.lvalue = false;
		ass.add("\tmovq (%rax), %rax");
	}
	ass.add(unary_operator_assembly[{l, op}]);
	return_type = unary_operator_result_type[{l, op}];
}

void ConstantInt::generateAssembly(assembly& ass)
{
	ass.add("mov", i32, val, rax);
}

void ConstantShort::generateAssembly(assembly& ass)
{
	ass.add("mov", i16, val, rax);
}

void ConstantLong::generateAssembly(assembly& ass)
{
	ass.add("mov", i64, val, rax);
}

void ConstantChar::generateAssembly(assembly& ass)
{
	ass.add("mov", i8, val, rax);
}

void ConstantString::generateAssembly(assembly& ass)
{
	ass.add("\tsubq $32, %rsp");
	ass.add("\tmovq $" + std::to_string(val.length() + 1) + ", %rax");
	ass.add("\tmovq %rax, 0(%rsp)");
	ass.add("\tmovq 0(%rsp), %rcx");
	ass.add("\tcall malloc");
	for (int i = 0; i < val.length(); i++) {
		ass.add("\tmovb $" + std::to_string((int)val[i]) + ", " + std::to_string(i) + "(%rax)");
	}
	ass.add("\tmovb $0, " + std::to_string(val.length()) + "(%rax)");
	ass.add("\taddq $32, %rsp");
}

void ExpressionLine::generateAssembly(assembly& ass)
{
	if (exp)
		exp->generateAssembly(ass);
}

void VariableDeclarationLine::generateAssembly(assembly& ass)
{
	if (init_exp == nullptr) {
		ass.add("\tmovq $0, %rax");
		for (int i = 0; i < (var_type.sz+7) / 8; i++) {
			ass.add("\tpush %rax");
		}
	}
	else {
		init_exp->generateAssembly(ass);
		if (init_exp->return_type.lvalue) {
			ass.add("\tmovq (%rax), %rax");
		}
		ass.add("\tpush %rax");
	}
	curr_scope->variables.insert({ name, {name, curr_scope->current_variable_location, var_type} });
	curr_scope->current_variable_location -= var_type.sz;
}

void VariableRef::generateAssembly(assembly& ass)
{
	scope* sc = curr_scope;
	while (sc && sc->variables.find(name) == sc->variables.end()) {
		sc = sc->parent;
	}
	if (sc) {
		return_type = sc->variables[name].type;
		if (return_type.lvalue) {
			ass.add("\tmovq " + std::to_string(sc->variables[name].location) + "(%rbp), %rax");
		}
		else {
			return_type.lvalue = true;
			ass.add("\tleaq " + std::to_string(sc->variables[name].location) + "(%rbp), %rax");
		}
	}
	else
		; //could not find variable

}

void IfStatement::generateAssembly(assembly& ass)
{
	static int if_clause = 0;
	int if_cl = if_clause++;
	condition->generateAssembly(ass);
	ass.add("\tcmpq $0, %rax");
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
	ass.add("\tcmpq $0, %rax");
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
	if (condition->return_type.lvalue) {
		size size = condition->return_type.pointers > 0 ? i64 : _size(condition->return_type.sz);
		ass.add("mov", size, rax, rcx, true, false);
		ass.add("\tmovl $0, %eax");
		ass.add("mov", size, rcx, rax, false, false);
	}
	ass.add("\tcmpq $0, %rax");
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
	if (condition->return_type.lvalue) {
		size size = condition->return_type.pointers > 0 ? i64 : _size(condition->return_type.sz);
		ass.add("mov", size, rax, rcx, true, false);
		ass.add("\tmovl $0, %eax");
		ass.add("mov", size, rcx, rax, false, false);
	}
	ass.add("\tcmpq $0, %rax");
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
	if (condition) {
		condition->generateAssembly(ass);
		if (condition->return_type.lvalue) {
			size size = condition->return_type.pointers > 0 ? i64 : _size(condition->return_type.sz);
			ass.add("mov", size, rax, rcx, true, false);
			ass.add("\tmovl $0, %eax");
			ass.add("mov", size, rcx, rax, false, false);
		}
	}
	else ass.add("\tmovl $1, %eax");
	ass.add("\tcmpq $0, %rax");
	ass.add("\tje _for_end_" + std::to_string(for_cl));
	inner->generateAssembly(ass);
	ass.add("_for_continue_" + std::to_string(for_cl) + ":");
	if (post) post->generateAssembly(ass);
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
	function f = functions.find({ name })->first;
	ass.add("\tsubq $" + std::to_string(std::max(32u, 8 * params.size())) + ", %rsp");
	for (int i = 0; i < params.size(); i++) {
		params[i]->generateAssembly(ass);
		if (params[i]->return_type.lvalue && !f.params[i].second.lvalue) {
			size size = params[i]->return_type.pointers > 0 ? i64 : _size(params[i]->return_type.sz);
			ass.add("mov", size, rax, rcx, true, false);
			ass.add("\tmovq $0, %rax");
			ass.add("mov", size, rcx, rax, false, false);
		}
		ass.add("\tmovq %rax, " + std::to_string(8 * i) + "(%rsp)");
	}
	if (params.size() > 0) ass.add("\tmovq 0(%rsp), %rcx");
	if (params.size() > 1) ass.add("\tmovq 8(%rsp), %rdx");
	if (params.size() > 2) ass.add("\tmovq 16(%rsp), %r8");
	if (params.size() > 3) ass.add("\tmovq 24(%rsp), %r9");
	ass.add("\tcall " + name);
	ass.add("\taddq $" + std::to_string(std::max(32u, 8 * params.size())) + ", %rsp");
}