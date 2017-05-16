#include "lexer.h"
#include "token.h"
#include "symbol_table.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <stack>

class parser {

private:
	bool is_passing_by_reference = false;
	bool got_token = false;
	bool in_function = false;
	bool found_return = false;
	bool eval_state = false;
	bool bool_eval_state = true;
	bool in_proc_or_func = false;
	bool nest_set = false;
	bool in_main = false;
	bool negative_expr = false;

	int offset = 0;
	int current_processing_type = 0;
	int num_locals;
	int curr_reg = 1;
	int last_compare;
	int if_count = 0;
	int or_count = 1;
	int while_count = 0;
	int creating_if_or_while = -1;

	LEXER lex;
	TOKEN tok;
	SymbolTable st;
	Param param;

	std::stack<int> if_stack;
	std::stack<int> while_stack;
	std::string original_type;
	std::string current_proc_or_func;
	std::string current_nest;
	std::string call_name;

	std::vector<std::string> temp_storage;
	std::ofstream output;
	std::unordered_map<std::string, bool>::const_iterator find;
	std::unordered_map<std::string, bool> reserved_words = {
		{ "AND", true },
		{ "ARRAY", true },
		{ "BEGIN", true },
		{ "BOOLEAN", true },
		{ "BREAK", true },
		{ "CASE", true },
		{ "CHAR", true },
		{ "CONST", true },
		{ "DO" , true },
		{ "ELSE", true },
		{ "END", true },
		{ "FALSE", true },
		{ "FOR", true },
		{ "FUNCTION", true },
		{ "GOTO", true },
		{ "IF", true },
		{ "INTEGER", true },
		{ "MOD", true },
		{ "OR", true },
		{ "PROCEDURE", true },
		{ "PROGRAM", true },
		{ "SET", true },
		{ "STRING", true },
		{ "THEN", true },
		{ "TRUE", true },
		{ "VAR", true },
		{ "WHILE", true },
		{ "XOR", true }
	};

public:
	parser(char * f) {
		output.open("../Out/file.cpp");
		start_output();
		lex.set(f);
		got_token = lex.getToken(tok);
		BEGIN();

		if (!lex.hasNext()) {
			printf("Valid!\n");
			output.close();
		}
		else {
			printf("Extra characters or declarations after end of program\n");
			PARSINGERROR();
		}
	}

private:
	void BEGIN() {
		BLOCK();
		if (!got_token || tok.id != ".") {
			printf("Error on line #%d: ", lex.getLine());
			if (!got_token) printf("Expected \".\" but found nothing\n");
			else printf("Expected \".\" but found \"%s\"\n", tok.id.c_str());
			PARSINGERROR();
		}

		pop_all_registers();
		output << "}\n\n";
		output << "return 0;\n}\n";
	}

	void BLOCK() {
		num_locals = 0;
		PFV();

		if (in_proc_or_func) {
			output << "\tsub esp, " << (num_locals * 4) << "\n\n";
			push_all_registers();
		}

		else {
			output << "kmain:\n";
			in_main = true;
		}

		if (got_token && UPPERCASE(tok.id) != "BEGIN") {
			printf("Error on line #%d: ", lex.getLine());
			if (!got_token) printf("Exptected \"begin\" but found nothing\n");
			else printf("Expected \"begin\" but found \"%s\"\n", tok.id.c_str());
			PARSINGERROR();
		}

		got_token = lex.getToken(tok);
		STATEMENT();
		MSTATEMENT();

		if (got_token && UPPERCASE(tok.id) != "END") {
			printf("Error on line #%d: ", lex.getLine());
			if (!got_token) printf("Exptected \"end\" but found nothing\n");
			else printf("Expected \"end\" but found \"%s\"\n", tok.id.c_str());
			PARSINGERROR();
		}

		else {
			if (in_function && !found_return) {
				printf("Error on line #%d: ", lex.getLine());
				printf("No return value specified from function \"%s\"\n", st.getCurrentMethodName().c_str());
				PARSINGERROR();
			}

			else if (in_function && found_return) {
				in_function = false;
				found_return = false;
			}
		}

		got_token = lex.getToken(tok);
	}

	void STATEMENT() {
		if (got_token && UPPERCASE(tok.id) == "WHILE") {
			creating_if_or_while = 2;
			while_count++;
			while_stack.push(while_count);
			output << "top_while_" << while_count << ":\n";
			original_type = st.varToType(UPPERCASE(tok.id));
			got_token = lex.getToken(tok);
			E();

			if (((original_type == "INTEGER" || original_type == "INTEGER_PARAMETER") && eval_state == false) || ((original_type == "BOOLEAN" || original_type == "BOOLEAN_PARAMETER") && (bool_eval_state == true && eval_state == true))) {
				printf("Error on line #%d: ", lex.getLine());
				printf("Invalid if expression given\n");
				PARSINGERROR();
			}

			eval_state = false;
			bool_eval_state = true;

			if (!got_token || UPPERCASE(tok.id) != "DO") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \"do\" but found nothing\n");
				else printf("Expected \"do\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			if (last_compare == 0)
				output << "\tjge end_while_" << while_stack.top() << "\n";

			else if (last_compare == 1)
				output << "\tjne end_while_" << while_stack.top() << "\n";

			else if (last_compare == 2)
				output << "\tjle end_while_" << while_stack.top() << "\n";

			output << "begin_while_" << while_stack.top() << ":\n";

			got_token = lex.getToken(tok);
			STATEMENT();

			output << "\tjmp top_while_" << while_stack.top() << "\n";
			output << "end_while_" << while_stack.top() << ":\n";
			while_stack.pop();
		}

		else if (got_token && UPPERCASE(tok.id) == "BEGIN") {
			got_token = lex.getToken(tok);
			STATEMENT();
			MSTATEMENT();

			if (got_token && UPPERCASE(tok.id) != "END") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \"end\" but found nothing\n");
				else printf("Expected \"end\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
		}

		else if (got_token && UPPERCASE(tok.id) == "IF") {
			creating_if_or_while = 1;
			if_count++;
			got_token = lex.getToken(tok);
			original_type = st.varToType(UPPERCASE(tok.id));
			if_stack.push(if_count);
			E();

			if (((original_type == "INTEGER" || original_type == "INTEGER_PARAMETER") && eval_state == false) || ((original_type == "BOOLEAN" || original_type == "BOOLEAN_PARAMETER") && (bool_eval_state == true && eval_state == true))) {
				printf("Error on line #%d: ", lex.getLine());
				printf("Invalid if expression given\n");
				PARSINGERROR();
			}

			eval_state = false;
			bool_eval_state = true;

			if (UPPERCASE(tok.id) != "THEN") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \"then\" but found nothing\n");
				else printf("Expected \"then\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			if (last_compare == 0)
				output << "\tjl if_begin_" << if_stack.top() << "\n\n";

			else if (last_compare == 1)
				output << "\tje if_begin_" << if_stack.top() << "\n\n";

			else if (last_compare == 2)
				output << "\tjg if_begin_" << if_stack.top() << "\n\n";

			output << "or_" << or_count++ << ":\n";
			output << "\tjmp else_" << if_stack.top() << "\n\n";
			output << "if_begin_" << if_count << ":\n";

			got_token = lex.getToken(tok);

			STATEMENT();

			output << "\tjmp end_if_" << if_stack.top() << "\n\n";
			output << "else_" << if_stack.top() << ":\n";

			STATEMENTPRIME();

			output << "end_if_" << if_stack.top() << ":\n";
			if_stack.pop();
		}

		else if (got_token && UPPERCASE(tok.type) == "WORD" && UPPERCASE(tok.id) != "END" && UPPERCASE(tok.id) != "ELSE") {
			if (!st.FindVariable(UPPERCASE(tok.id), offset)) {
				printf("Error on line #%d: ", lex.getLine());
				printf("Identifier \"%s\" never declared\n", tok.id.c_str());
				PARSINGERROR();
			}

			if (in_proc_or_func == true && nest_set == false) {
				current_nest = current_proc_or_func;
				nest_set = true;
			}

			current_proc_or_func = UPPERCASE(tok.id);
			original_type = st.varToType(UPPERCASE(tok.id));
			if (original_type == "") {
				original_type = st.varToType(UPPERCASE(tok.id), st.getCurrentMethodName());

				if (original_type == "") {
					printf("Error on line #%d: ", lex.getLine());
					printf("Identifier \"%s\" never declared\n", tok.id.c_str());
					PARSINGERROR();
				}
			}

			call_name = tok.id;
			got_token = lex.getToken(tok);

			if (tok.id != ":=" && tok.id != "(" && tok.id != "[") {
				printf("Error on line #%d: ", lex.getLine());
				printf("Expected method call or assignment\n");
				PARSINGERROR();
			}

			if (st.dimensionOfAttribute(UPPERCASE(call_name)) == 1) {
				if (tok.id == "[") {
					int off;
					got_token = lex.getToken(tok);
					E();
					output << "\tsub ";
					current_register(--curr_reg);
					output << ", ";
					output << st.getArraySub(UPPERCASE(call_name), 1);
					output << "\n\timul ";
					current_register(curr_reg);
					output << ", " << st.getSizeOfArrayElement(UPPERCASE(call_name)) << "\n";
					output << "\tadd ";
					current_register(curr_reg);
					output << ", ";
					st.FindVariable(UPPERCASE(call_name), off);
					output << off << "\n";
					output << "\tadd ";
					current_register(curr_reg);
					output << ", ebp\n";
					got_token = lex.getToken(tok);

					if (tok.id != ":=") PARSINGERROR();
					got_token = lex.getToken(tok);

					curr_reg++;
					E();
					output << "\tmov [";
					curr_reg -= 2;
					current_register(curr_reg);
					output << "], ";
					current_register(curr_reg + 1);
					output << "\n";
				}

			}

			else if (st.dimensionOfAttribute(UPPERCASE(call_name)) == 2) {
				if (tok.id == "[") {
					int off;
					got_token = lex.getToken(tok);
					E();
					if (tok.id != ",") PARSINGERROR();
					got_token = lex.getToken(tok);
					E();
					if (tok.id != "]") PARSINGERROR();

					curr_reg--;
					output << "\tsub ";
					current_register(curr_reg - 1);
					output << ", ";
					output << st.getArraySub(UPPERCASE(call_name), 1);

					output << "\n\tsub ";
					current_register(curr_reg);
					output << ", ";
					output << st.getArraySub(UPPERCASE(call_name), 2);

					output << "\n\timul ";
					current_register(curr_reg - 1);
					output << ", " << st.arrayRowSize(UPPERCASE(call_name)) << "\n";

					output << "\n\timul ";
					current_register(curr_reg);
					output << ", 4\n";

					output << "\tadd ";
					current_register(curr_reg - 1);
					output << ", ";
					current_register(curr_reg);

					output << "\n\tadd ";
					current_register(curr_reg - 1);
					output << ", ";
					st.FindVariable(UPPERCASE(call_name), off);
					output << off << "\n";

					output << "\tadd ";
					current_register(curr_reg - 1);
					output << ", ebp\n";
					got_token = lex.getToken(tok);

					if (tok.id != ":=") PARSINGERROR();
					got_token = lex.getToken(tok);

					curr_reg = 2;
					E();
					output << "\tmov [";
					curr_reg -= 2;
					current_register(curr_reg);
					output << "], ";
					current_register(curr_reg + 1);
					output << "\n";
				}
			}

			else {

				if (tok.id == "(" && !(original_type == "PROCEDURE" || original_type == "FUNCTION")) {
					printf("Error on line #%d: ", lex.getLine());
					printf("Variable treated as method or function\n");
					PARSINGERROR();
				}

				else if (tok.id == "(") {
					got_token = lex.getToken(tok);
					CALL();
					if (tok.id != ")") {
						printf("Error on line #%d: ", lex.getLine());
						printf("Expected \")\" but found \"%s\" instead\n", tok.id.c_str());
						PARSINGERROR();
					}

					got_token = lex.getToken(tok);
				}

				else {
					got_token = lex.getToken(tok);

					if (in_function && !found_return) {
						if (current_proc_or_func == st.getCurrentMethodName()) {
							original_type = st.getFunctionReturn();
							found_return = true;
						}
					}

					E();

					if (in_main) {
						st.FindVariable(UPPERCASE(current_proc_or_func), offset);
						output << "\tmov [ebp + " << offset << "], ";
						current_register(--curr_reg);
						output << "\n\n";
					}

					else {
						if (st.glob_loc_param(current_proc_or_func, current_nest) == 0) {
							st.FindVariable(UPPERCASE(current_proc_or_func), offset);
							output << "\tmov [edi - " << (4 + offset) << "], ";
							current_register(--curr_reg);
							output << "\n\n";
						}

						else if (st.glob_loc_param(current_proc_or_func, current_nest) == 1) {
							Param da = st.getParamData(st.getCurrentMethodName(), st.paramOffset(UPPERCASE(current_nest), UPPERCASE(current_proc_or_func)));
							if (da.pass_by_reference == false) {
								st.FindVariable(UPPERCASE(current_proc_or_func), offset);
								output << "\tmov [edi + " << (8 + offset) << "], ";
								current_register(--curr_reg);
								output << "\n\n";
							}

							else if (da.pass_by_reference == true) {
								output << "\tmov esi, [edi + " << (8 + 4 * st.paramOffset(UPPERCASE(current_nest), UPPERCASE(current_proc_or_func))) << "]\n";
								output << "\tmov [esi], ";
								current_register(--curr_reg);
								output << "\n\n";
							}
						}

						else {
							st.FindVariable(UPPERCASE(current_proc_or_func), offset);
							output << "\tmov [ebp + " << offset << "], ";
							current_register(--curr_reg);
							output << "\n\n";
						}
					}
				}
			}
		}
	}

	void STATEMENTPRIME() {
		if (UPPERCASE(tok.id) != "ELSE")
			return;

		got_token = lex.getToken(tok);
		STATEMENT();
	}

	void MSTATEMENT() {
		if (tok.id != ";")
			return;

		got_token = lex.getToken(tok);

		STATEMENT();
		MSTATEMENT();
	}

	void CALL() {
		int num_params = st.NumOfParams(current_proc_or_func);
		std::vector<int> offsets;
		int x, c_z, c_o;

		if (num_params < 0) {
			printf("Error on line #%d: ", lex.getLine());
			printf("Invalid identifier, cannot find \"%s\"\n", current_proc_or_func.c_str());
			PARSINGERROR();
		}

		if (tok.id == ")") {
			if (num_params != 0) {
				printf("Error on line #%d: ", lex.getLine());
				printf("Wrong number of arguments for call to \"%s\"\n", current_proc_or_func.c_str());
				PARSINGERROR();
			}
		}

		else {
			for (int i = 0; i < num_params; i++) {
				param = st.getParamData(current_proc_or_func, i);
				if (param.pass_by_reference == false) {
					if (tok.type == "word") {
						string df = st.varToType(UPPERCASE(tok.id));
						if (df == "") df = st.varToType(UPPERCASE(tok.id), st.getCurrentMethodName());
						if (param.type == "BOOLEAN" && (df == "BOOLEAN" || df == "BOOLEAN_PARAMETER" || tok.id == "true" || tok.id == "false")) goto jal;
						else if (param.type == "INTEGER" && (df == "INTEGER" || df == "INTEGER_PARAMETER")) {
							st.FindVariable(UPPERCASE(tok.id), offset);
							offsets.push_back(0);
							offsets.push_back(offset);
							goto jal;
						}

						printf("Error on line #%d: ", lex.getLine());
						printf("Invalid Parameter passed\n", tok.id.c_str());
						PARSINGERROR();
					}

					else if (param.type == "INTEGER" && tok.type != "integer" && tok.type != "real") {
						printf("Error on line #%d: ", lex.getLine());
						printf("Expected a constant value, but instead found \"%s\"\n", tok.id.c_str());
						PARSINGERROR();
					}

					else if (param.type == "BOOLEAN" && tok.id != "true" && tok.id != "false") {
						printf("Error on line #%d: ", lex.getLine());
						printf("Expected a boolean value, but instead found \"%s\"\n", tok.id.c_str());
						PARSINGERROR();
					}
				}
				else {
					if (tok.type == "word") {
						find = reserved_words.find(UPPERCASE(tok.id));
						if (find != reserved_words.end()) {
							printf("Error on line #%d: ", lex.getLine());
							printf("Invalid placement of identifier \"%s\"\n", tok.id.c_str());
							PARSINGERROR();
						}

						else if (st.FindVariable(UPPERCASE(tok.id), offset)) {
							if (tok.type == "word") {
								string df = st.varToType(UPPERCASE(tok.id));
								if (df == "") df = st.varToType(tok.id, st.getCurrentMethodName());
								if (param.type == "BOOLEAN" && (df == "BOOLEAN" || df == "BOOLEAN_PARAMETER" || tok.id == "true" || tok.id == "false")) goto jal;
								else if (param.type == "INTEGER" && (df == "INTEGER" || df == "INTEGER_PARAMETER")) {
									st.FindVariable(UPPERCASE(tok.id), offset);
									offsets.push_back(1);
									offsets.push_back(offset);
									goto jal;
								}

								printf("Error on line #%d: ", lex.getLine());
								printf("Invalid Parameter passed\n", tok.id.c_str());
								PARSINGERROR();
							}
							printf("Error on line #%d: ", lex.getLine());
							printf("Identifier \"%s\" is undefined\n", tok.id.c_str());
							PARSINGERROR();
						}
					}

					else {
						printf("Error on line #%d: ", lex.getLine());
						printf("Wrong number of arguments for call to \"%s\"\n", current_proc_or_func.c_str());
						PARSINGERROR();
					}
				}

			jal:
				if (tok.type == "integer") {
					offsets.push_back(2);
					offsets.push_back(stoi(tok.id));
				}

				got_token = lex.getToken(tok);

				if (i + 1 < num_params) {
					if (tok.id != ",") {
						printf("Error on line #%d: ", lex.getLine());
						printf("Expected to find \",\" but instead found \"%s\"\n", tok.id.c_str());
						PARSINGERROR();
					}

					got_token = lex.getToken(tok);
				}
			}

			x = offsets.size() / 2;
			for (int i = 0; i < x; i++) {
				c_o = offsets.back();
				offsets.pop_back();

				c_z = offsets.back();
				offsets.pop_back();

				if (c_z == 0) {
					output << "\tmov ";
					current_register(curr_reg++);
					output << ", [ebp + " << c_o << "]\n";
					output << "\tpush ";
					current_register(--curr_reg);
					output << "\n\n";
				}

				else if (c_z == 1) {
					output << "\tmov ";
					current_register(curr_reg++);
					output << ", " << c_o << "\n";
					output << "\tadd ";
					current_register(--curr_reg);
					output << ", ebp\n";
					output << "\tpush ";
					current_register(curr_reg);
					output << "\n\n";
				}

				else if (c_z == 2) {
					output << "\tpush " << c_o << "\n\n";
				}
			}

		}

		output << "\tcall " << call_name << "\n\n";
	}

	void E() {
		T();
		EP();
	}

	void EP() {
		if (tok.id == "<" || tok.id == ">" || tok.id == "<=" || tok.id == ">=" || tok.id == "!=" || tok.id == "=") {
			eval_state = true;
			if (tok.id == "!=" || tok.id == "=") bool_eval_state = false;
			if (tok.id == "<") last_compare = 0;
			else if (tok.id == "=") last_compare = 1;
			else last_compare = 2;

			got_token = lex.getToken(tok);
			E();
			compare_registers();
		}
	}

	void T() {
		TR();
		TP();
	}

	void TP() {
		std::string previous_t;
		if (got_token && (tok.id == "(" || (tok.type != "special" && UPPERCASE(tok.id) != "OR")))
			return;

		if (got_token && ((tok.id == "-" || tok.id == "+")) || UPPERCASE(tok.id) == "OR") {
			previous_t = tok.id;
			got_token = lex.getToken(tok);

			if (UPPERCASE(previous_t) == "OR") {
				if (creating_if_or_while == 1) {
					if (last_compare == 0)
						output << "\tjl if_begin_" << if_stack.top() << "\n";

					else if (last_compare == 1)
						output << "\tje if_begin_" << if_stack.top() << "\n";

					else if (last_compare == 2)
						output << "\tjg if_begin_" << if_stack.top() << "\n";

					output << "\nor_" << or_count++ << ":\n";
				}

				else if (creating_if_or_while == 2) {
					if (last_compare == 0)
						output << "\tjl begin_while_" << while_stack.top() << "\n";

					else if (last_compare == 1)
						output << "\tje begin_while_" << while_stack.top() << "\n";

					else if (last_compare == 2)
						output << "\tjg begin_while_" << while_stack.top() << "\n";

					output << "\nor_" << or_count++ << ":\n";
				}
			}

			TR();

			if (previous_t == "+") {
				output << "\tadd ";
				current_register(curr_reg - 2);
				output << ", ";
				current_register(curr_reg - 1);
				output << "\n";
				curr_reg--;
			}

			else if (previous_t == "-") {
				output << "\tsub ";
				current_register(curr_reg - 2);
				output << ", ";
				current_register(curr_reg - 1);
				output << "\n";
				curr_reg--;
			}

			TP();
		}
	}

	void TR() {
		F();
	}

	void F() {
		FR();
		FP();
	}

	void FP() {
		std::string previous_t;

		if (got_token && (tok.id == "(" || (tok.type != "special" && UPPERCASE(tok.id) != "AND"))) return;
		if (got_token && ((tok.id == "*" || tok.id == "/")) || UPPERCASE(tok.id) == "AND") {
			previous_t = tok.id;
			got_token = lex.getToken(tok);

			if (UPPERCASE(previous_t) == "AND") {
				if (last_compare == 0)
					output << "\tjge or_" << or_count << "\n";

				else if (last_compare == 1)
					output << "\tjne or_" << or_count << "\n";

				else if (last_compare == 2)
					output << "\tjle or_" << or_count << "\n";
			}

			FR();

			if (previous_t == "*") {
				output << "\timul ";
				current_register(curr_reg - 2);
				output << ", ";
				current_register(curr_reg - 1);
				output << "\n";
				curr_reg--;
			}

			else if (previous_t == "/") {
				output << "\n\tpush edx\n\tcdq\n\tidiv ";
				current_register(curr_reg - 1);
				output << "\n\tpop edx\n\n";
				curr_reg--;
			}

			FP();
		}
	}

	void FR() {
		if ((tok.type == "real" || tok.type == "integer" || tok.type == "word") && got_token) {
			if (tok.type == "word" && (UPPERCASE(tok.id) != "TRUE" && UPPERCASE(tok.id) != "FALSE")) {
				if (!st.FindVariable(UPPERCASE(tok.id), offset)) {
					printf("Error on line #%d: ", lex.getLine());
					printf("Identifier \"%s\" never declared\n", tok.id.c_str());
					PARSINGERROR();
				}

				else {

					if (st.dimensionOfAttribute(UPPERCASE(tok.id)) == 1) {
						std::string remember_var_name = tok.id;
						int off;

						got_token = lex.getToken(tok);
						if (!got_token) PARSINGERROR();
						if (tok.id != "[") PARSINGERROR();
						got_token = lex.getToken(tok);
						if (!got_token) PARSINGERROR();
						E();
						if (tok.id != "]") PARSINGERROR();

						output << "\tsub ";
						current_register(curr_reg - 1);
						output << ", " << st.getArraySub(UPPERCASE(remember_var_name), 1) << "\n";

						output << "\timul ";
						current_register(curr_reg - 1);
						output << ", 4\n";

						output << "\tadd ";
						current_register(curr_reg - 1);
						output << ", ";
						st.FindVariable(UPPERCASE(remember_var_name), off);
						output << off << "\n";

						output << "\tmov ";
						current_register(curr_reg - 1);
						output << ", [ebp + ";
						current_register(curr_reg - 1);
						output << "]\n";

						goto valid;
					}
					

					else if (st.dimensionOfAttribute(UPPERCASE(tok.id)) == 2) {
						std::string remember_var_name = tok.id;
						int off;
						got_token = lex.getToken(tok);
						if (!got_token) PARSINGERROR();
						if (tok.id != "[") PARSINGERROR();
						got_token = lex.getToken(tok);
						if (!got_token) PARSINGERROR();
						E();
						if (tok.id != ",") PARSINGERROR();
						got_token = lex.getToken(tok);
						if (!got_token) PARSINGERROR();
						E();
						if (tok.id != "]") PARSINGERROR();

						curr_reg--;

						output << "\tsub ";
						current_register(curr_reg - 1);
						output << ", " << st.getArraySub(UPPERCASE(remember_var_name), 1) << "\n";

						output << "\timul ";
						current_register(curr_reg - 1);
						output << ", " << st.arrayRowSize(UPPERCASE(remember_var_name)) << "\n";

						output << "\tsub ";
						current_register(curr_reg);
						output << ", " << st.getArraySub(UPPERCASE(remember_var_name), 2) << "\n";

						output << "\timul ";
						current_register(curr_reg);
						output << ", 4\n";

						output << "\tadd ";
						current_register(curr_reg - 1);
						output << ", ";
						current_register(curr_reg);
						output << "\n";

						output << "\tadd ";
						current_register(curr_reg - 1);
						output << ", ";
						st.FindVariable(UPPERCASE(remember_var_name), off);
						output << off << "\n";

						output << "\tmov ";
						current_register(curr_reg - 1);
						output << ", [ebp + ";
						current_register(curr_reg - 1);
						output << "]\n";

						goto valid;
					}

					else {

						output << "\tmov ";
						current_register(curr_reg++);

						if (in_main)
							output << ", [ebp + " << offset << "]\n";

						else {
							if (st.glob_loc_param(UPPERCASE(tok.id), UPPERCASE(current_nest)) == 0)
								output << ", [edi - " << (4 + offset) << "]\n";

							else if (st.glob_loc_param(UPPERCASE(tok.id), UPPERCASE(current_nest)) == 1)
								output << ", [edi + " << (8 + 4 * st.paramOffset(UPPERCASE(current_nest), UPPERCASE(tok.id))) << "]\n";

							else
								output << ", [ebp + " << offset << "]\n";
						}
					}
				}
			}

			if (original_type == "BOOLEAN") {
				if (UPPERCASE(tok.id) == "TRUE" || UPPERCASE(tok.id) == "FALSE") goto valid;
				if (st.varToType(UPPERCASE(tok.id)) == "BOOLEAN") goto valid;
				if (st.varToType(UPPERCASE(tok.id), st.getCurrentMethodName()) == "PARAMETER_BOOLEAN") goto valid;

				printf("Error on line #%d: ", lex.getLine());
				printf("Type Mismatch\n");
				PARSINGERROR();
			}

			else if (eval_state && tok.type == "integer") {
				output << "\tmov ";
				current_register(curr_reg++);
				output << ", " << tok.id << "\n";

				if (negative_expr == true) {
					output << "\tneg ";
					current_register(curr_reg - 1);
					output << "\n";
				}

				goto valid;
			}

			else if (original_type == "INTEGER") {
				if (tok.type == "real" || tok.type == "integer") {
					output << "\tmov ";
					current_register(curr_reg++);
					output << ", " << tok.id << "\n";

					if (negative_expr == true) {
						output << "\tneg ";
						current_register(curr_reg - 1);
						output << "\n";
					}

					goto valid;
				}

				if (st.varToType(UPPERCASE(tok.id)) == "INTEGER") goto valid;
				if (st.varToType(UPPERCASE(tok.id), st.getCurrentMethodName()) == "INTEGER") goto valid;

				printf("Error on line #%d: ", lex.getLine());
				printf("Type Mismatch\n");
				PARSINGERROR();
			}

			else if (original_type == "")
				original_type = st.varToType(UPPERCASE(tok.id), st.getCurrentMethodName());

		valid:
			negative_expr = false;
			got_token = lex.getToken(tok);
		}

		else if ((tok.id == "+" || tok.id == "-") && got_token) {
			if (tok.id == "-")
				negative_expr = !negative_expr;

			got_token = lex.getToken(tok);
			TR();
		}

		else if (tok.id == "(" && got_token) {
			got_token = lex.getToken(tok);
			E();
			(tok.id != ")" || !got_token) ? PARSINGERROR() : got_token = lex.getToken(tok);
		}

		else {
			printf("Error on line #%d: ", lex.getLine());
			printf("Invalid statement\n");
			PARSINGERROR();
		}

	}

	void PFV() {
		if (got_token && UPPERCASE(tok.id) == "FUNCTION") {
			got_token = lex.getToken(tok);
			FUNCTION();
			PFV();
		}

		else if (got_token && UPPERCASE(tok.id) == "PROCEDURE") {
			got_token = lex.getToken(tok);
			PROCEDURE();
			PFV();
		}

		else if (got_token && UPPERCASE(tok.id) == "VAR") {
			got_token = lex.getToken(tok);
			VARIABLE();
			PFV();
		}

		else return;
	}

	void FUNCTION() {
		in_function = true;
		if (got_token && tok.type == "word") {
			find = reserved_words.find(UPPERCASE(tok.id));
			if (find != reserved_words.end()) {
				printf("Error on line #%d: ", lex.getLine());
				printf("Invalid placement of identifier \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			if (!st.AddProcFunc(UPPERCASE(tok.id), "FUNCTION")) {
				printf("Error on line #%d: ", lex.getLine());
				printf("Duplicate identifier\n", tok.id.c_str());
				PARSINGERROR();
			}

			current_proc_or_func = UPPERCASE(tok.id);
			got_token = lex.getToken(tok);

			if (!got_token || tok.id != "(") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \"(\" but found nothing\n");
				else printf("Expected \"(\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			PARAM();

			if (!got_token || tok.id != ")") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \")\" but found nothing\n");
				else printf("Expected \")\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			if (!got_token || tok.id != ":") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \":\" but found nothing\n");
				else printf("Expected \":\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			current_processing_type = 3;
			DATATYPE();

			if (!got_token || tok.id != ";") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \";\" but found nothing\n");
				else printf("Expected \";\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			BLOCK();

			if (!got_token || tok.id != ";") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \";\" but found nothing\n");
				else printf("Expected \";\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			st.BackOut();
		}
	}

	void PROCEDURE() {
		if (got_token && tok.type == "word") {
			find = reserved_words.find(UPPERCASE(tok.id));
			if (find != reserved_words.end()) {
				printf("Error on line #%d: ", lex.getLine());
				printf("Invalid use of reserved word \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			if (!st.AddProcFunc(UPPERCASE(tok.id), "PROCEDURE")) {
				printf("Error on line #%d: ", lex.getLine());
				printf("Duplicate identifier\n", tok.id.c_str());
				PARSINGERROR();
			}

			output << tok.id << ":\n";
			output << "\tpush edi\n";
			output << "\tmov edi, esp\n";

			in_proc_or_func = true;
			nest_set = false;
			current_proc_or_func = UPPERCASE(tok.id);
			got_token = lex.getToken(tok);

			if (!got_token || tok.id != "(") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \"(\" but found nothing\n");
				else printf("Expected \"(\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			PARAM();

			if (!got_token || tok.id != ")") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \")\" but found nothing\n");
				else printf("Expected \")\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);

			if (!got_token || tok.id != ";") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \";\" but found nothing\n");
				else printf("Expected \";\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			BLOCK();

			pop_all_registers();
			output << "\tadd esp, " << (num_locals * 4) << "\n";
			output << "\tpop edi\n";
			output << "\tret " << (st.NumOfParams(st.getCurrentMethodName())) * 4 << "\n\n";

			if (!got_token || tok.id != ";") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \";\" but found nothing\n");
				else printf("Expected \";\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			in_proc_or_func = false;
			st.BackOut();
			got_token = lex.getToken(tok);
		}
	}

	void PARAM() {
		if (got_token && (UPPERCASE(tok.id) != "VAR" && tok.type != "word")) return;

		else if (got_token && UPPERCASE(tok.id) == "VAR") {
			got_token = lex.getToken(tok);
			is_passing_by_reference = true;
			current_processing_type = 1;

			if (got_token && tok.type == "word") {
				find = reserved_words.find(UPPERCASE(tok.id));
				if (find != reserved_words.end()) {
					printf("Error on line #%d: ", lex.getLine());
					printf("Invalid placement of reserved word \"%s\"\n", tok.id.c_str());
					PARSINGERROR();
				}

				temp_storage.push_back(UPPERCASE(tok.id));
			}

			else {
				printf("Error on line #%d: ", lex.getLine());
				printf("Found invalid identifier.\n");
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			VARLIST();

			if (!got_token || tok.id != ":") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \":\" but found nothing\n");
				else printf("Expected \":\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			DATATYPE();
			MPARAM();
		}

		else if (got_token && (tok.type == "word" && (find = reserved_words.find(UPPERCASE(tok.id))) == reserved_words.end())) {
			is_passing_by_reference = false;
			current_processing_type = 1;
			temp_storage.push_back(UPPERCASE(tok.id));
			got_token = lex.getToken(tok);
			VARLIST();

			if (!got_token || tok.id != ":") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \":\" but found nothing\n");
				else printf("Expected \":\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			DATATYPE();
			MPARAM();
		}
	}

	void MPARAM() {
		if (tok.id != ";") return;
		got_token = lex.getToken(tok);

		if (got_token && UPPERCASE(tok.id) == "VAR") {
			got_token = lex.getToken(tok);
			current_processing_type = 1;
			temp_storage.push_back(UPPERCASE(tok.id));
			is_passing_by_reference = true;

			if (got_token && tok.type == "word") {
				find = reserved_words.find(UPPERCASE(tok.id));
				if (find != reserved_words.end()) {
					printf("Error on line #%d: ", lex.getLine());
					printf("Invalid placement of reserved word \"%s\"\n", tok.id.c_str());
					PARSINGERROR();
				}
			}

			else {
				printf("Error on line #%d: ", lex.getLine());
				printf("Invalid placement of character \";\"\n");
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			VARLIST();

			if (got_token && tok.id != ":") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \":\" but found nothing\n");
				else printf("Expected \":\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			DATATYPE();
			MPARAM();
		}


		else if (got_token && tok.type == "word" && (find = reserved_words.find(UPPERCASE(tok.id))) == reserved_words.end()) {
			temp_storage.push_back(UPPERCASE(tok.id));
			got_token = lex.getToken(tok);
			current_processing_type = 1;
			is_passing_by_reference = false;
			VARLIST();

			if (got_token && tok.id != ":") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \":\" but found nothing\n");
				else printf("Expected \":\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			DATATYPE();
			MPARAM();
		}

		else {
			printf("Error on line #%d: ", lex.getLine());
			printf("Found invalid identifier.\n");
			PARSINGERROR();
		}
	}

	void DATATYPE() {
		if (!got_token || (UPPERCASE(tok.id) != "INTEGER" && UPPERCASE(tok.id) != "BOOLEAN" && UPPERCASE(tok.id) != "CHAR" && UPPERCASE(tok.id) != "ARRAY")) {
			printf("Error on line #%d: ", lex.getLine());
			printf("Unsupported or invalid data type for identifier\n");
			PARSINGERROR();
		}

		else if (current_processing_type == 0) {
			num_locals += temp_storage.size();
			for (int i = 0; i <= temp_storage.size() - 1; i++) {
				if (UPPERCASE(tok.id) == "INTEGER") {
					if (!st.AddVariable(temp_storage.at(i), UPPERCASE(tok.id), 4)) {
						printf("Error on line #%d: ", lex.getLine());
						printf("Duplicate identifier\n");
						PARSINGERROR();
					}
				}

				else if (UPPERCASE(tok.id) == "BOOLEAN" || UPPERCASE(tok.id) == "CHAR") {
					if (!st.AddVariable(temp_storage.at(i), UPPERCASE(tok.id), 1)) {
						printf("Error on line #%d: ", lex.getLine());
						printf("Duplicate identifier\n");
						PARSINGERROR();
					}
				}

				else if (UPPERCASE(tok.id) == "ARRAY") {
					int start_1, end_1, start_2, end_2, dim, d_size;
					if (lex.getToken(tok) == false)
						ERROR_NOT_FOUND("[");

					if (tok.id != "[") {
						printf("Error on line #%d: ", lex.getLine());
						printf("Exptected \"[\" but found %s\n", tok.id.c_str());
						PARSINGERROR();
					}

					if (lex.getToken(tok) == false)
						ERROR_NOT_FOUND("integer");

					if (tok.type != "array index" && tok.type != "integer") {
						printf("Error on line #%d: ", lex.getLine());
						printf("Was expecting an integer for an array index, but found %s\n", tok.type.c_str());
						PARSINGERROR();
					}

					start_1 = stoi(tok.id);

					if (lex.getToken(tok) == false)
						ERROR_NOT_FOUND("array seperator");

					if (tok.id != "..") {
						printf("Error on line #%d: ", lex.getLine());
						printf("Was expecting an array seperator, but found %s\n", tok.id.c_str());
						PARSINGERROR();
					}

					if (lex.getToken(tok) == false)
						ERROR_NOT_FOUND("integer");

					if (tok.type != "array_index" && tok.type != "integer") {
						printf("Error on line #%d: ", lex.getLine());
						printf("Was expecting an integer for an array index, but found %s\n", tok.type.c_str());
						PARSINGERROR();
					}

					end_1 = stoi(tok.id);

					if (lex.getToken(tok) == false)
						ERROR_NOT_FOUND("]");

					if (tok.id != "]" && tok.id != ",") {
						printf("Error on line #%d: ", lex.getLine());
						printf("Exptected \"]\" but found %s\n", tok.id.c_str());
						PARSINGERROR();
					}

					if (tok.id == "]") {
						start_2 = end_2 = -1;
						dim = 1;
					}

					else if (tok.id == ",") {
						dim = 2;
						if (lex.getToken(tok) == false)
							ERROR_NOT_FOUND("integer");

						if (tok.type != "array index" && tok.type != "integer") {
							printf("Error on line #%d: ", lex.getLine());
							printf("Was expecting an integer for an array index, but found %s\n", tok.type.c_str());
							PARSINGERROR();
						}

						start_2 = stoi(tok.id);

						if (lex.getToken(tok) == false)
							ERROR_NOT_FOUND("array seperator");

						if (tok.id != "..") {
							printf("Error on line #%d: ", lex.getLine());
							printf("Was expecting an array seperator, but found %s\n", tok.id.c_str());
							PARSINGERROR();
						}

						if (lex.getToken(tok) == false)
							ERROR_NOT_FOUND("integer");

						if (tok.type != "array index" && tok.type != "integer") {
							printf("Error on line #%d: ", lex.getLine());
							printf("Was expecting an integer for an array index, but found %s\n", tok.type.c_str());
							PARSINGERROR();
						}

						end_2 = stoi(tok.id);

						if (lex.getToken(tok) == false)
							ERROR_NOT_FOUND("]");

						if (tok.id != "]") {
							printf("Error on line #%d: ", lex.getLine());
							printf("Exptected \"]\" but found %s\n", tok.id.c_str());
							PARSINGERROR();
						}
					}

					if (lex.getToken(tok) == false)
						ERROR_NOT_FOUND("of");

					if (tok.id != "of") {
						printf("Error on line #%d: ", lex.getLine());
						printf("Exptected \"of\" but found %s\n", tok.id.c_str());
						PARSINGERROR();
					}

					if (lex.getToken(tok) == false)
						ERROR_NOT_FOUND("data type");

					if (UPPERCASE(tok.id) != "INTEGER" && UPPERCASE(tok.id) != "BOOLEAN") {
						printf("Error on line #%d: ", lex.getLine());
						printf("Exptected \"of\" but found %s\n", tok.id.c_str());
						PARSINGERROR();
					}

					if (UPPERCASE(tok.id) == "INTEGER") d_size = 4;
					else d_size = 2;

					if (!st.AddVariable(temp_storage.at(i), UPPERCASE(tok.id), d_size, dim, start_1, end_1, start_2, end_2)) {
						printf("Error on line #%d: ", lex.getLine());
						printf("Duplicate identifier\n");
						PARSINGERROR();
					}
				}
			}
		}

		else if (current_processing_type == 1) {
			for (int i = 0; i <= temp_storage.size() - 1; i++) {
				if (UPPERCASE(tok.id) == "INTEGER") {
					if (!st.AddParam(current_proc_or_func, temp_storage.at(i), UPPERCASE(tok.id), 4, is_passing_by_reference)) {
						printf("Error on line #%d: ", lex.getLine());
						printf("Duplicate identifier\n");
						PARSINGERROR();
					}
				}

				else if (UPPERCASE(tok.id) == "BOOLEAN" || UPPERCASE(tok.id) == "CHAR") {
					if (!st.AddParam(current_proc_or_func, temp_storage.at(i), UPPERCASE(tok.id), 1, is_passing_by_reference)) {
						printf("Error on line #%d: ", lex.getLine());
						printf("Duplicate identifier\n");
						PARSINGERROR();
					}
				}
			}
		}

		else if (current_processing_type == 3)
			st.setFunctionReturn(UPPERCASE(tok.id));

		for (int i = 0; i < temp_storage.size(); )
			temp_storage.pop_back();

		got_token = lex.getToken(tok);
	}

	void ERROR_NOT_FOUND(char * s) {
		printf("Error on line #%d: ", lex.getLine());
		printf("Exptected \"%s\" but found nothing\n", s);
		PARSINGERROR();
	}

	void VARLIST() {
		if (!got_token || tok.id != ",") return;
		if (tok.id == ",") {
			got_token = lex.getToken(tok);
			if (got_token && tok.type == "word" && (find = reserved_words.find(UPPERCASE(tok.id))) == reserved_words.end()) {
				temp_storage.push_back(UPPERCASE(tok.id));
				got_token = lex.getToken(tok);
				VARLIST();
			}

			else if ((find = reserved_words.find(UPPERCASE(tok.id))) != reserved_words.end()) {
				printf("Error on line #%d: ", lex.getLine());
				printf("Invalid placement of reserved word \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			else {
				printf("Error on line #%d: ", lex.getLine());
				printf("Invalid placement of character \",\"\n");
				PARSINGERROR();
			}
		}
	}

	void VARIABLE() {
		if (got_token && tok.type == "word") {
			find = reserved_words.find(UPPERCASE(tok.id));
			if (find != reserved_words.end()) {
				printf("Error on line #%d: ", lex.getLine());
				printf("Invalid placement of reserved word \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			temp_storage.push_back(UPPERCASE(tok.id));
			current_processing_type = 0;
			got_token = lex.getToken(tok);
			VARLIST();

			if (got_token && tok.id != ":") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \":\" but found nothing\n");
				else printf("Expected \":\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			DATATYPE();

			if (got_token && tok.id != ";") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \";\" but found nothing\n");
				else printf("Expected \";\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			MULTIVARIABLE();
		}
	}

	void MULTIVARIABLE() {
		if (got_token && tok.type == "word") {
			find = reserved_words.find(UPPERCASE(tok.id));
			if (find != reserved_words.end()) return;

			temp_storage.push_back(UPPERCASE(tok.id));
			current_processing_type = 0;
			got_token = lex.getToken(tok);
			VARLIST();

			if (got_token && tok.id != ":") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \":\" but found nothing\n");
				else printf("Expected \":\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			DATATYPE();

			if (got_token && tok.id != ";") {
				printf("Error on line #%d: ", lex.getLine());
				if (!got_token) printf("Exptected \";\" but found nothing\n");
				else printf("Expected \";\" but found \"%s\"\n", tok.id.c_str());
				PARSINGERROR();
			}

			got_token = lex.getToken(tok);
			MULTIVARIABLE();
		}
	}

	std::string UPPERCASE(std::string str) {
		int i = 0;
		std::string ret_value;
		while (str[i])
			ret_value += toupper(str[i++]);

		return ret_value;
	}

	void PARSINGERROR() {
		output.close();
		output.open("../Out/file.cpp");
		output.close();

		printf("Parsing error.\n");
		system("pause");
		exit(1);
	}

	void push_all_registers() {
		output << "\tpush eax\n";
		output << "\tpush ebx\n";
		output << "\tpush ecx\n";
		output << "\tpush edx\n";
		output << "\tpush ebp\n";
		output << "\tpush edi\n";
		output << "\tpush esi\n";
		output << "\tpush esp\n\n";
	}

	void pop_all_registers() {
		output << "\tpop esp\n";
		output << "\tpop esi\n";
		output << "\tpop edi\n";
		output << "\tpop ebp\n";
		output << "\tpop edx\n";
		output << "\tpop ecx\n";
		output << "\tpop ebx\n";
		output << "\tpop eax\n\n";
	}

	void start_output() {
		output << "#include <iostream>\n";
		output << "using namespace std;\n";
		output << "char DataSegment[65536];\n\n";
		output << "int main() {\n";
		output << "__asm {\n";
		push_all_registers();
		output << "\tlea eax, DataSegment\n";
		output << "\tmov ebp, eax\n";
		output << "\tjmp kmain\n\n";
	}

	void compare_registers() {
		output << "\tcmp ";
		current_register(curr_reg - 2);
		output << ", ";
		current_register(curr_reg - 1);
		output << "\n\n";
		curr_reg -= 2;
	}

	void current_register(int in) {
		switch (in) {
		case 1: output << "eax";
			break;
		case 2: output << "ebx";
			break;
		case 3: output << "ecx";
			break;
		case 4: output << "edx";
			break;
		default:
			printf("Current Register = %d\n", in);
			printf("Error on line #%d: ", lex.getLine());
			printf("Expression too complicated.\n");
			PARSINGERROR();
		}
	}
};