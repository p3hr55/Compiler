#include "parser.h"
#include <iostream>
#define SIZE 50

void main()
{
	char fname[SIZE];
	std::cout << "Enter filename: ";
	std::cin >> fname;
 	parser p(fname);
	system("pause");
}