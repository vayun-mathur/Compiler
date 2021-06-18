#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

#include "tokenize.h"
#include "ast.h"

std::string slurp(std::ifstream& in) {
	std::ostringstream sstr;
	sstr << in.rdbuf();
	return sstr.str();
}

int main(int argc, char* argv[]) {
	initAST();

	std::ifstream openfile = std::ifstream(argv[1]);
	std::string s = slurp(openfile);
	std::queue<token> token_queue;
	tokenize(s, token_queue);
	assembly ass;
	Application* ast = compile_application(token_queue);
	ast->generateAssembly(ass);
	std::cout << ass.str() << std::endl;
	std::ofstream outfile = std::ofstream(argv[2]);
	outfile << ass.str();
	outfile.close();
}