#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "Parameter.h"
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

struct Scope;

struct ParamData {
	string type;
	int size;
	int offset;
	bool PassbyRef;
};

struct LocalVar {
	string type;
	int size;
	int offset;
};

struct VarData {
	string type;
	string return_type;
	int size;
	int offset;
	unordered_map<string, ParamData> parameters;
	vector<std::string> order_of_params;
	Scope * NextScope;
	int array_dimension, start_index_1, start_index_2, end_index_1, end_index_2;
};

struct Scope {
	unordered_map<string, VarData> Table;
	int offset;
	string ScopeName;
	Scope * PreviousScope;

};

class SymbolTable {

private:
	Scope * CurrentScope;
	vector<string> storage;

public:
	SymbolTable() {
		CurrentScope = new Scope;
		CurrentScope->offset = 0;
		CurrentScope->ScopeName = "";
		CurrentScope->PreviousScope = NULL;
	}

	//Returns information about the parameter to the call
	Param getParamData(string proc_name, int position) {
		Param return_data;

		//If the procedure's name is in the current scope
		if (CurrentScope->Table.find(proc_name) != CurrentScope->Table.end()) {
			return_data.pass_by_reference = CurrentScope->Table[proc_name].parameters[CurrentScope->Table[proc_name].order_of_params.at(position)].PassbyRef;
			return_data.size = CurrentScope->Table[proc_name].parameters[CurrentScope->Table[proc_name].order_of_params.at(position)].size;
			return_data.type = CurrentScope->Table[proc_name].parameters[CurrentScope->Table[proc_name].order_of_params.at(position)].type;
		}

		//If the procedure's name is in the previous scope
		else if (CurrentScope->PreviousScope->Table.find(proc_name) != CurrentScope->PreviousScope->Table.end()) {
			return_data.pass_by_reference = CurrentScope->PreviousScope->Table[proc_name].parameters[CurrentScope->PreviousScope->Table[proc_name].order_of_params.at(position)].PassbyRef;
			return_data.size = CurrentScope->PreviousScope->Table[proc_name].parameters[CurrentScope->PreviousScope->Table[proc_name].order_of_params.at(position)].size;
			return_data.type = CurrentScope->PreviousScope->Table[proc_name].parameters[CurrentScope->PreviousScope->Table[proc_name].order_of_params.at(position)].type;
		}

		//Else we can assume that the parameter doesnt exist
		else {
			return_data.pass_by_reference = 0;
			return_data.size = 0;
			return_data.type = "";
		}

		return return_data;
	}

	//Used to find the offset of the parameter in its parameter list
	int paramOffset(string proc_name, string param_name) {
		Scope * TravScope = CurrentScope;

		//If the current scope is the procedure then we want to backout one level
		if (TravScope->ScopeName == proc_name) {
			TravScope = TravScope->PreviousScope;
		}

		//Want to traverse through the scopes to try and find the parameter
		//This should usually only one step
		while (TravScope != NULL) {
			if (TravScope->Table.find(proc_name) == TravScope->Table.end())
				TravScope = TravScope->PreviousScope;

			//Step through the list until we find the parameter and return it
			else {
				for (int i = 0; i < TravScope->Table[proc_name].order_of_params.size(); i++) {
					if (param_name == TravScope->Table[proc_name].order_of_params.at(i))
						return i;
				}

				//Incase an invalid parameter gets through we return a negative value to be caught
				return -1;
			}
		}
	}

	//Used to find how many parameters there are for error checking calls
	int NumOfParams(string name) {

		//Nonexistent
		if (CurrentScope->Table.find(name) == CurrentScope->Table.end() && (CurrentScope->PreviousScope == NULL || CurrentScope->PreviousScope->Table.find(name) == CurrentScope->PreviousScope->Table.end()))
			return -1;

		//If its found then return it
		if (CurrentScope->Table.find(name) != CurrentScope->Table.end())
			return CurrentScope->Table[name].order_of_params.size();
		return CurrentScope->PreviousScope->Table[name].order_of_params.size();
	}

	bool AddVariable(string name, string type, int size) {
		int junk;
		if (CurrentScope->Table.find(name) != CurrentScope->Table.end())
			return false;
		else if (FindVariable(name, junk))
			return false;

		CurrentScope->Table[name].type = type;
		CurrentScope->Table[name].size = size;
		CurrentScope->Table[name].offset = CurrentScope->offset;
		CurrentScope->Table[name].array_dimension = 0;
		CurrentScope->Table[name].start_index_1 = CurrentScope->Table[name].end_index_1
			= CurrentScope->Table[name].start_index_2 = CurrentScope->Table[name].end_index_2 = -1;
		CurrentScope->Table[name].NextScope = NULL;
		CurrentScope->offset += size;
		return true;
	}

	bool AddVariable(string name, string type, int size, int dimension, int s1, int e1, int s2, int e2) {
		int junk;
		if (CurrentScope->Table.find(name) != CurrentScope->Table.end())
			return false;
		else if (FindVariable(name, junk))
			return false;

		CurrentScope->Table[name].type = type;
		CurrentScope->Table[name].size = size;
		CurrentScope->Table[name].offset = CurrentScope->offset;
		CurrentScope->Table[name].array_dimension = dimension;
		CurrentScope->Table[name].start_index_1 = s1;
		CurrentScope->Table[name].end_index_1 = e1;
		CurrentScope->Table[name].start_index_2 = s2;
		CurrentScope->Table[name].end_index_2 = e2;
		CurrentScope->Table[name].NextScope = NULL;
		if (s2 == -1) 
			CurrentScope->offset += (size * (e1 - s1 + 1));
		else
			CurrentScope->offset += (size * ((e1 - s1 + 1) * (e2 - s2 + 1)));
		return true;
	}

	int dimensionOfAttribute(string name) {
		return CurrentScope->Table[name].array_dimension;
	}

	//Used for declaring the return type of a function
	void setFunctionReturn(string t) {
		string function_name = getCurrentMethodName();
		CurrentScope->PreviousScope->Table[function_name].return_type = t;
	}

	//Used for error checking the return type of a function
	string getFunctionReturn() {
		return CurrentScope->PreviousScope->Table[getCurrentMethodName()].return_type;
	}

	//Determines if a nest is a procedure or function
	bool isProcOrFunc(string t) {
		if (CurrentScope->PreviousScope->Table[t].type == "procedure" ||
			CurrentScope->PreviousScope->Table[t].type == "function")
			return true;
		return false;
	}

	bool FindVariable(string name, int & offset) {
		Scope * TravScope = CurrentScope;
		while (TravScope != NULL) {
			if (TravScope->Table.find(name) == TravScope->Table.end())
				TravScope = TravScope->PreviousScope;

			else {
				offset = TravScope->Table[name].offset;
				return true;
			}
		}
		return false;
	}

	//Returns the name of the procedure or function
	string getCurrentMethodName() {
		return CurrentScope->ScopeName;
	}

	string varToType(string name) {
		Scope * TravScope = CurrentScope;
		while (TravScope != NULL) {
			if (TravScope->Table.find(name) == TravScope->Table.end())
				TravScope = TravScope->PreviousScope;
			else {
				return TravScope->Table[name].type;
			}
		}

		return "";
	}

	string varToType(string name, string in) {
		Scope * TravScope = CurrentScope;
		if ((int)name[0] >= 0x31 && (int)name[0] <= 0x39)
			return "STD_INTEGER";
		if (in == "") return "INVALID TYPE";
		while (TravScope != NULL) {
			if (TravScope->Table.find(in) == TravScope->Table.end())
				TravScope = TravScope->PreviousScope;
			else
				return TravScope->Table[in].parameters[name].type;
		}
	}

	bool AddParam(string r, string name, string type, int size, bool pbr) {
		if (CurrentScope->Table.find(name) != CurrentScope->Table.end() ||
			(CurrentScope->PreviousScope->Table.find(name) != CurrentScope->PreviousScope->Table.end() && CurrentScope->PreviousScope->Table[name].type != "PARAMETER"))
			return false;

		CurrentScope->PreviousScope->Table[r].parameters[name].PassbyRef = pbr;
		CurrentScope->PreviousScope->Table[r].parameters[name].size = size;
		CurrentScope->PreviousScope->Table[r].parameters[name].type = type;
		CurrentScope->Table[name].size = 0;

		if (type == "integer")
			CurrentScope->Table[name].type = "PARAMETER_INTEGER";
		else if (type == "boolean")
			CurrentScope->Table[name].type = "PARAMETER_BOOLEAN";
		else if (type == "char")
			CurrentScope->Table[name].type = "PARAMETER_CHAR";

		CurrentScope->PreviousScope->Table[r].order_of_params.push_back(name);

		return true;
	}

	bool AddProcedure(string name, string type) {
		if (CurrentScope->Table.find(name) != CurrentScope->Table.end())
			return false;

		CurrentScope->Table[name].type = type;
		CurrentScope->Table[name].size = 0;
		CurrentScope->Table[name].offset = 0;

		Scope * OldCurrent = CurrentScope;

		CurrentScope->Table[name].NextScope = new Scope;
		CurrentScope = CurrentScope->Table[name].NextScope;
		CurrentScope->offset = 0;
		CurrentScope->ScopeName = name;
		CurrentScope->PreviousScope = OldCurrent;

		return true;
	}

	bool AddProcFunc(string name, string type) {
		if (CurrentScope->Table.find(name) != CurrentScope->Table.end())
			return false;

		CurrentScope->Table[name].type = type;
		CurrentScope->Table[name].size = 0;
		CurrentScope->Table[name].offset = 0;

		Scope * OldCurrent = CurrentScope;

		CurrentScope->Table[name].NextScope = new Scope;
		CurrentScope = CurrentScope->Table[name].NextScope;
		CurrentScope->offset = 0;
		CurrentScope->ScopeName = name;
		CurrentScope->PreviousScope = OldCurrent;

		return true;
	}

	void BackOut() {
		if (CurrentScope->PreviousScope) CurrentScope = CurrentScope->PreviousScope;
	}

	int glob_loc_param(string name, string proc_name) {
		//we have a local
		if (CurrentScope->Table.find(name) != CurrentScope->Table.end() && CurrentScope->Table[name].size != 0)
			return 0;

		//found parameter
		else if (CurrentScope->PreviousScope != NULL && CurrentScope->PreviousScope->Table[proc_name].parameters.find(name) != CurrentScope->PreviousScope->Table[proc_name].parameters.end())
			return 1;

		//can assume its a global
		else if (CurrentScope->PreviousScope != NULL && CurrentScope->PreviousScope->Table[name].type != "PROCEDURE")
			return 2;

		return -1;
	}

	int getArraySub(string name, int s) {
		int offset;
		Scope * TravScope = CurrentScope;
		if (FindVariable(name, offset)) {
			while (TravScope != NULL) {
				if (TravScope->Table.find(name) == TravScope->Table.end())
					TravScope = TravScope->PreviousScope;

				else {
					if (s == 1) return TravScope->Table[name].start_index_1;
					return TravScope->Table[name].start_index_2;
				}
			}

		}

		return -1;
	}

	int getSizeOfArrayElement(string name) {
		int offset;
		Scope * TravScope = CurrentScope;
		if (FindVariable(name, offset)) {
			while (TravScope != NULL) {
				if (TravScope->Table.find(name) == TravScope->Table.end())
					TravScope = TravScope->PreviousScope;

				else
					return TravScope->Table[name].size;
			}

		}

		return -1;
	}

	int arrayRowSize(string n) {
		return (CurrentScope->Table[n].end_index_2 - CurrentScope->Table[n].start_index_2 + 1) * CurrentScope->Table[n].size;
	}

	int getOffset(string name, string proc_name, int opt) {
		//locals
		if (opt == 0)
			return CurrentScope->Table[name].offset;

		else if (opt == 1)
			return CurrentScope->PreviousScope->Table[proc_name].parameters[name].offset;
	}
};