#pragma once
#define ROWS 12
#define IN "DFA.txt"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include "token.h"

class LEXER {
private:
	int table[ROWS][128];
	std::ifstream file;
	int line_num = 1;

public:
	LEXER(char * fname) {
		set(fname);
	}

	LEXER() {};

	void set(char * fname)
	{
		std::ifstream f(IN);
		std::string temp, temp2;

		for (int i = 0; i < 128; i++) {
			//puts next line of input file into temp and replaces tabs with spaces
			std::getline(f, temp, '\n');
			for (int x = 0; x < temp.size(); x++) {
				if (temp[x] == '\t')
					temp[x] = ' ';
			}

			std::stringstream s;
			s << temp;
			for (int x = 0; x < ROWS; x++)
				s >> table[x][i];
		}

		file.open(fname);
		if (file.fail()) {
			printf("DFA.txt removed or renamed.\n");
			system("pause");
			exit(0);
		}
	}


	//Sees if we have anymore input to process
	bool hasNext()
	{
		int n = -1;

		//Comments are allowed after 'end.' but no more code
		while ((((n = file.get()) == 10 || n == 32 || n == 47)) && file.peek() != -1) {
			if (n == 47 && file.peek() != 47)
				return 1;
			else if (n == 47 && file.peek() == 47)
				while ((n = file.get()) != 10 && file.peek() != -1);
		}

		if (file.peek() != -1)
			return 1;

		return 0;
	}

	//Returns line number for debug purposes
	int getLine() {
		return line_num;
	}

	//Retrieves current token from source code
	bool getToken(TOKEN &d) {
		int current = 0, previous = 0;
		char last_char = ' ';
		bool cont_read = true;

		//leaves when we're out of input
		if (file.eof())
			return false;

		d.id = d.type = "";

		do { //appends character by character until dead or accept state

			 //When we process a newline that means we just passed one
			if ((int)last_char == 10)
				line_num++;

			//Checks for single line comments
			if (last_char == '/' && file.peek() == 47) {
				while (file.peek() != 10 && file.peek() != -1)
					file.get();
				current = 0;
				d.id = "";
			}
			if (file.peek() == '/')
				d.id = d.id;
			previous = current;
			current = table[current][file.peek()];

			//If at the end of the input
			if (file.peek() == -1)
				break;

			//No point to return on a lexing error, instead we will exit after returning the error.
			//In future put debug info.
			if (current == 99) {
				if (file.peek() == '.' && last_char == '.' && d.id.length() != 1) {
					file.seekg(-1, std::ios::cur);
					last_char = ' ';
					d.id = d.id.substr(0, d.id.length() - 1);
					d.type = "array index";
					cont_read = false;
				}

				else if (file.peek() == '.' && last_char == '.' && d.id.length() == 1) {
					cont_read = false;
					d.id += file.get();
					d.type = "array split";
				}

				else {
					printf("Lexing error.\n");
					system("pause");
					exit(1);
				}
			}

			//Append
			if (current != 55 && current != 0 && cont_read)
				d.id += file.peek();

		} while (current != 99 && current != 55 && (last_char = file.get()));

		//Determines the type of data that was supplied to lexer
		switch (previous) {
		case 0: d.type = "special";
			break;
		case 1: d.type = "word";
			break;
		case 2: d.type = "integer";
			break;
		case 4:
		case 9: d.type = "real";
			break;
		case 11:
		case 10: d.type = "special";
			break;
		}

		return true;
	}
};