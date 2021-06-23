#pragma once
#include <vector>
#include <queue>
#include "tokenize.h"
#include "register.h"

void initAST();

enum class LineType {
	Return, Expression, VariableDeclaration, If, Block, For, While, DoWhile, Break, Continue
};
enum class ExpressionType {
	BinaryOperator, ConstantInt, VariableRef, UnaryOperator, Ternary, FunctionCall
};

class DataType {
public:
	static const DataType INT;
	static const DataType INT_REF;
	static const DataType INT_PTR;
	static const DataType CHAR;
	static const DataType CHAR_REF;
	static const DataType CHAR_PTR;
	static const DataType SHORT;
	static const DataType SHORT_REF;
	static const DataType SHORT_PTR;
	static const DataType LONG;
	static const DataType LONG_REF;
	static const DataType LONG_PTR;
	int id;
	int pointers;
	bool lvalue;
	DataType()
		: id(0), pointers(0), lvalue(false) {

	}
	DataType(const DataType& other)
		: id(other.id), pointers(other.pointers), lvalue(other.lvalue) {

	}
	DataType(int id, int pointers, bool lvalue)
		: id(id), pointers(pointers), lvalue(lvalue) {

	}
	bool operator==(const DataType& other) {
		return id == other.id && pointers == other.pointers && lvalue == other.lvalue;
	}
	bool operator<(const DataType& other) const {
		if (id < other.id) return true;
		if (id > other.id) return false;
		if (pointers < other.pointers) return true;
		if (pointers > other.pointers) return false;
		if (lvalue < other.lvalue) return true;
		return false;
	}
};

inline const DataType DataType::CHAR = DataType(1, 0, false);
inline const DataType DataType::CHAR_PTR = DataType(1, 1, false);
inline const DataType DataType::SHORT = DataType(2, 0, false);
inline const DataType DataType::SHORT_PTR = DataType(2, 1, false);
inline const DataType DataType::INT = DataType(3, 0, false);
inline const DataType DataType::INT_PTR = DataType(3, 1, false);
inline const DataType DataType::LONG = DataType(4, 0, false);
inline const DataType DataType::LONG_PTR = DataType(4, 1, false);

struct assembly {
	std::vector<std::string> lines;

	assembly() {
	}

	assembly(std::initializer_list<std::string> l) {
		for (std::string s : l) add(s);
	}

	assembly& add(std::string s) {
		lines.push_back(s);
		return *this;
	}

	assembly& add(std::string instruction, size s, reg dst, bool index = false) {
		lines.push_back("\t" + instruction + _suffix(s) + (index ? " (%" + _register(dst, s) + ")" : " %" + _register(dst, s)));
		return *this;
	}

	assembly& convert(size s1, size s2, reg r) {
		/*
		switch (s1) {
		case i8:
			switch (s2) {
			case i8:
			case i16:
			case i32:
			case i64:
			}
		case i16:
			switch (s2) {
			case i8:
			case i16:
			case i32:
			case i64:
			}
		case i32:
			switch (s2) {
			case i8:
			case i16:
			case i32:
			case i64:
			}
		case i64:
			switch (s2) {
			case i8:
			case i16:
			case i32:
			case i64:
			}
		}*/
		return *this;
	}

	assembly& add(std::string instruction, size s, reg src, reg dst, bool src_index = false, bool dst_index = false) {
		lines.push_back("\t" + instruction + _suffix(s) +
			(src_index ? " (%" + _register(src, s) + ")" : " %" + _register(src, s)) +
			(dst_index ? ", (%" + _register(dst, s) + ")" : ", %" + _register(dst, s)));
		return *this;
	}

	assembly& add(std::string instruction, size s, int src, reg dst) {
		lines.push_back("\t" + instruction + _suffix(s) + " $" + std::to_string(src) + ", %" + _register(dst, s));
		return *this;
	}

	void add(const assembly& other) {
		for (std::string l : other.lines) {
			add(l);
		}
	}

	std::string str() {
		std::string s;
		for (std::string l : lines) {
			s += l + "\n";
		}
		return s;
	}
};

struct ASTNode {
	virtual void generateAssembly(assembly& ass) = 0;
};

struct BlockItem : ASTNode {
	LineType type;
	BlockItem(LineType type) : type(type) {
	}
};

struct LineOfCode : BlockItem {
	LineOfCode(LineType type) : BlockItem(type) {
	}
};

struct Expression : ASTNode {
	DataType return_type;
	ExpressionType type;
	Expression(ExpressionType type, DataType return_type) : type(type), return_type(return_type) {
	}
};

struct ExpressionLine : LineOfCode {
	Expression* exp;
	ExpressionLine(Expression* exp) : LineOfCode(LineType::Expression), exp(exp) {
	}
	virtual void generateAssembly(assembly& ass) override;
};

struct CodeBlock : LineOfCode {
	std::vector<BlockItem*> lines;
	CodeBlock() : LineOfCode(LineType::Block) {}
	virtual void generateAssembly(assembly& ass) override;
};

struct VariableDeclarationLine : BlockItem {
	DataType var_type;
	Expression* init_exp;
	std::string name;
	VariableDeclarationLine(Expression* init_exp, DataType var_type, std::string name) : BlockItem(LineType::VariableDeclaration),
		init_exp(init_exp), var_type(var_type), name(name) {

	}
	virtual void generateAssembly(assembly& ass) override;
};

struct Function : ASTNode {
	std::string name;
	std::vector<std::pair<std::string, DataType>> params;
	CodeBlock* lines;
	virtual void generateAssembly(assembly& ass) override;
};

struct FunctionCall : Expression {
	std::string name;
	std::vector<Expression*> params;
	FunctionCall(std::string name, DataType return_type) : Expression(ExpressionType::FunctionCall, return_type), name(name) {}
	virtual void generateAssembly(assembly& ass) override;
};

struct Application : ASTNode {
	std::vector<Function*> functions;
	virtual void generateAssembly(assembly& ass) override;
};

struct Return : LineOfCode {
	Return(Expression* expr) : LineOfCode(LineType::Return), expr(expr) { };
	Expression* expr;
	virtual void generateAssembly(assembly& ass) override;
};

struct IfStatement : LineOfCode {
	Expression* condition;
	LineOfCode* if_cond, * else_cond;
	IfStatement(Expression* condition, LineOfCode* if_cond, LineOfCode* else_cond) : LineOfCode(LineType::If),
		condition(condition), if_cond(if_cond), else_cond(else_cond) { };

	virtual void generateAssembly(assembly& ass) override;
};

struct ForLoop : LineOfCode {
	BlockItem* initial;
	Expression* condition;
	Expression* post;
	LineOfCode* inner;
	ForLoop(BlockItem* initial, Expression* condition, Expression* post, LineOfCode* inner) : LineOfCode(LineType::For),
		initial(initial), condition(condition), post(post), inner(inner) { };

	virtual void generateAssembly(assembly& ass) override;
};

struct Break : LineOfCode {
	Break() : LineOfCode(LineType::Break) {}
	virtual void generateAssembly(assembly& ass) override;
};

struct Continue : LineOfCode {
	Continue() : LineOfCode(LineType::Continue) {}
	virtual void generateAssembly(assembly& ass) override;
};

struct WhileLoop : LineOfCode {
	Expression* condition;
	LineOfCode* inner;
	WhileLoop(Expression* condition, LineOfCode* inner) : LineOfCode(LineType::While),
		condition(condition), inner(inner) { };

	virtual void generateAssembly(assembly& ass) override;
};

struct DoWhileLoop : LineOfCode {
	Expression* condition;
	LineOfCode* inner;
	DoWhileLoop(Expression* condition, LineOfCode* inner) : LineOfCode(LineType::DoWhile),
		condition(condition), inner(inner) { };

	virtual void generateAssembly(assembly& ass) override;
};

enum binary_operator {
	add, subtract, multiply, divide, mod,
	logical_and, logical_or,
	bitwise_and, bitwise_or, bitwise_xor,
	equal, not_equal, less, greater, less_equal, greater_equal,
	left_shift, right_shift,
	assignment,
	add_assign, subtract_assign, multiply_assign, divide_assign, mod_assign,
	left_shift_assign, right_shift_assign,
	and_assign, or_assign, xor_assign
};

struct BinaryOperator : Expression {
	BinaryOperator(binary_operator op, Expression* left, Expression* right, DataType return_type) : Expression(ExpressionType::BinaryOperator, return_type), op(op), left(left), right(right) { };
	Expression* left, * right;
	binary_operator op;
	virtual void generateAssembly(assembly& ass) override;
};

enum unary_operator {
	negation, bitwise_complement, logical_negation,
	prefix_increment, prefix_decrement,
	postfix_increment, postfix_decrement,
	address, dereference
};

struct UnaryOperator : Expression {
	UnaryOperator(unary_operator op, Expression* left, DataType return_type) : Expression(ExpressionType::UnaryOperator, return_type), op(op), left(left) { };
	Expression* left;
	unary_operator op;
	virtual void generateAssembly(assembly& ass) override;
};

struct TernaryExpression : Expression {
	TernaryExpression(Expression* condition, Expression* if_cond, Expression* else_cond, DataType return_type) :
		Expression(ExpressionType::Ternary, return_type), condition(condition), if_cond(if_cond), else_cond(else_cond) { };
	Expression* condition;
	Expression* if_cond;
	Expression* else_cond;
	virtual void generateAssembly(assembly& ass) override;
};

struct VariableRef : Expression {
	VariableRef(std::string name) : Expression(ExpressionType::VariableRef, DataType::INT),
		name(name) {
	};
	std::string name;
	virtual void generateAssembly(assembly& ass) override;
};

struct ConstantChar : Expression {
	ConstantChar(char val) : Expression(ExpressionType::ConstantInt, DataType::CHAR), val(val) { };
	char val;
	virtual void generateAssembly(assembly& ass) override;
};

struct ConstantShort : Expression {
	ConstantShort(short val) : Expression(ExpressionType::ConstantInt, DataType::SHORT), val(val) { };
	short val;
	virtual void generateAssembly(assembly& ass) override;
};

struct ConstantInt : Expression {
	ConstantInt(int val) : Expression(ExpressionType::ConstantInt, DataType::INT), val(val) { };
	int val;
	virtual void generateAssembly(assembly& ass) override;
};

struct ConstantLong : Expression {
	ConstantLong(long long val) : Expression(ExpressionType::ConstantInt, DataType::LONG), val(val) { };
	long long val;
	virtual void generateAssembly(assembly& ass) override;
};

Application* compile_application(std::queue<token>& tokens);