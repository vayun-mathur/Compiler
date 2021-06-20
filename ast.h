#pragma once
#include <vector>
#include <queue>
#include "tokenize.h"

void initAST();

enum class LineType {
	Return, Expression, VariableDeclaration
};
enum class ExpressionType {
	BinaryOperator, ConstantInt, VariableRef, UnaryOperator
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
		if(id < other.id) return true;
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

struct LineOfCode : ASTNode {
	LineType type;
	LineOfCode(LineType type) : type(type) {
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

struct VariableDeclarationLine : LineOfCode {
	DataType var_type;
	Expression* init_exp;
	std::string name;
	VariableDeclarationLine(Expression* init_exp, DataType var_type, std::string name) : LineOfCode(LineType::VariableDeclaration),
		init_exp(init_exp), var_type(var_type), name(name) {

	}
	virtual void generateAssembly(assembly& ass) override;
};

struct Function : ASTNode {
	std::string name;
	std::vector<LineOfCode*> lines;
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

enum binary_operator {
	add, subtract, multiply, divide, mod,
	logical_and, logical_or,
	equal, not_equal, less, greater, less_equal, greater_equal,
	assignment
};

struct BinaryOperator : Expression {
	BinaryOperator(binary_operator op, Expression* left, Expression* right, DataType return_type) : Expression(ExpressionType::BinaryOperator, return_type), op(op), left(left), right(right) { };
	Expression* left, * right;
	binary_operator op;
	virtual void generateAssembly(assembly& ass) override;
};

enum unary_operator {
	negation, bitwise_complement, logical_negation
};

struct UnaryOperator : Expression {
	UnaryOperator(unary_operator op, Expression* left, DataType return_type) : Expression(ExpressionType::UnaryOperator, return_type), op(op), left(left) { };
	Expression* left;
	unary_operator op;
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