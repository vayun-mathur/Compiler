#pragma once
#include <vector>
#include <queue>
#include "tokenize.h"

void initAST();

enum class LineType {
	Return, Expression, VariableDeclaration, If, Block, For, While, DoWhile, Break, Continue
};
enum class ExpressionType {
	BinaryOperator, ConstantInt, VariableRef, UnaryOperator, Ternary
};

class DataType {
public:
	static const DataType INT;
	static const DataType INT_REF;
	int id;
	int pointers;
	bool reference;
	DataType()
		: id(0), pointers(0), reference(false) {

	}
	DataType(const DataType& other)
		: id(other.id), pointers(other.pointers), reference(other.reference) {

	}
	DataType(int id, int pointers, bool reference)
		: id(id), pointers(pointers), reference(reference) {

	}
	bool operator==(const DataType& other) {
		return id == other.id && pointers == other.pointers && reference == other.reference;
	}
	bool operator<(const DataType& other) const {
		if (id < other.id) return true;
		if (id > other.id) return false;
		if (pointers < other.pointers) return true;
		if (pointers > other.pointers) return false;
		if (reference < other.reference) return true;
		return false;
	}
};

inline const DataType DataType::INT = DataType(0, 0, false);
inline const DataType DataType::INT_REF = DataType(0, 0, true);

struct assembly {
	std::vector<std::string> lines;

	assembly() {
	}

	assembly(std::initializer_list<std::string> l) {
		for (std::string s : l) add(s);
	}

	void add(std::string s) { lines.push_back(s); }

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
	CodeBlock* lines;
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
	postfix_increment, postfix_decrement
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
	VariableRef(std::string name, DataType var_type) : Expression(ExpressionType::VariableRef, var_type),
		name(name) {
		return_type.reference = true;
	};
	std::string name;
	virtual void generateAssembly(assembly& ass) override;
};

struct ConstantInt : Expression {
	ConstantInt(int val) : Expression(ExpressionType::ConstantInt, DataType::INT), val(val) { };
	int val;
	virtual void generateAssembly(assembly& ass) override;
};

Application* compile_application(std::queue<token>& tokens);