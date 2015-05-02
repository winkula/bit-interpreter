#include <iostream>
#include <string>
#include <ctype.h>
#include <list>
#include <map>
#include <algorithm>

using namespace std;

/// <summary>
/// This is a interpreter for the esotheric programming language "BIT".
/// http://www.dangermouse.net/esoteric/bit.html
/// This is the languages grammar:
/// <![CDATA[
///
/// <code>         ::= <line> [ <line> ]...
/// <line>         ::= "LINE NUMBER" <bits> "CODE" <instruction> [ <goto> ]
/// <instruction>  ::= <command>
///                  | <assignment>
/// <goto>         ::= "GOTO" [ "VARIABLE" ] <bits> [ "IF THE JUMP REGISTER IS" [ "EQUAL TO" ] <bit>
///                  [ "GOTO" [ "VARIABLE" ] <bits> "IF THE JUMP REGISTER IS" [ "EQUAL TO" ] <bit> ] ]
///
/// <command>      ::= "PRINT" <bit>
///                  | "READ"
/// <assignment>   ::= (<variable> | <expression>) "EQUALS" <expression>
///
/// <expression>   ::= <expression2> [ "NAND" <expression2> ]
/// <expression2>  ::= [ "THE ADDRESS OF" ] <expression3>
/// <expression3>  ::= [ "THE VALUE BEYOND" ] <expression4>
/// <expression4>  ::= [ "THE VALUE AT" ] <expression5>
/// <expression5>  ::= <variable>
///                  | <bits>
///                  | "OPEN PARENTHESIS" <expression> "CLOSE PARENTHESIS"
///
/// <variable>     ::= "VARIABLE" <bits>
///                  | "THE JUMP REGISTER"
///
/// <bits>         ::= <bit> [ <bit> ]...
/// <bit>          ::= "ZERO"
///                  | "ONE"
///
/// ]]>
/// </summary>

#pragma region Types

enum ValueType {
	UNDEFINED = 0,
	BIT = 1,
	ADDRESS_OF_A_BIT = 2
};

struct Value {
	int value;
	ValueType type;
};

#pragma endregion

#pragma region Declarations

class LineNode;
class InstructionNode;
class CommandNode;
class AssignmentNode;
class GotoNode;
class ExpressionNode;
class Expression1Node;
class Expression2Node;
class Expression3Node;
class Expression4Node;
class Expression5Node;
class VariableNode;

class Node {
};

class CodeNode : public Node {
public:
	map<int, LineNode> lines;
	int first_line_number;

	CodeNode() : first_line_number(-1) {};
	void run();
};

class LineNode : public Node {
public:
	int line_number;
	InstructionNode* instruction;
	GotoNode* go;

	LineNode() : line_number(-1), instruction(NULL), go(NULL) {};
	~LineNode() { delete instruction; delete go; };
	int get_next_line_number();
};

class InstructionNode : public Node {
public:
	virtual void run() = 0;
};

class CommandNode : public InstructionNode {
public:
	int print_value;

	CommandNode() : print_value(-1) {};
	void run();
};

class AssignmentNode : public InstructionNode {
public:
	int address;
	ExpressionNode* address_expression;
	ExpressionNode* expression;

	AssignmentNode() : address(INT_MIN), address_expression(NULL), expression(NULL) {};
	~AssignmentNode() { delete address_expression, delete expression; };
	void run();
};

class GotoNode : public Node {
public:
	Value next;
	int next_if_zero;
	int next_if_one;

	GotoNode() : next({ -1, BIT }), next_if_zero(-1), next_if_one(-1) {};
	int next_line_number();
};

class ExpressionNode : public Node {
public:
	virtual Value value() = 0;
};

class Expression1Node : public ExpressionNode {
public:
	ExpressionNode* left;
	ExpressionNode* right;

	Expression1Node() : left(NULL), right(NULL) {};
	~Expression1Node() { delete left; delete right; };
	Value value();
};

class Expression2Node : public ExpressionNode {
public:
	ExpressionNode* child;

	Expression2Node() : child(NULL) {};
	~Expression2Node() { delete child; };
	Value value();
};

class Expression3Node : public ExpressionNode {
public:
	ExpressionNode* child;

	Expression3Node() : child(NULL) {};
	~Expression3Node() { delete child; };
	Value value();
};

class Expression4Node : public ExpressionNode {
public:
	ExpressionNode* child;

	Expression4Node() : child(NULL) {};
	~Expression4Node() { delete child; };
	Value value();
};

class Expression5Node : public ExpressionNode {
public:
	int constant;

	Expression5Node() : constant(0) {};
	Value value();
};

class VariableNode : public ExpressionNode {
public:
	int address;

	Value value();
};

Value memory_read(int address);
void memory_write(int address, Value value);
void print_bit(int value);
int read_bit();
void show_parser_error(string message);
void show_runtime_error(string message);

CodeNode* parse_code();
LineNode* parse_line();
InstructionNode* parse_instruction();
InstructionNode* parse_command();
InstructionNode* parse_assignment();
GotoNode* parse_goto();
ExpressionNode* parse_expression();
ExpressionNode* parse_expression2();
ExpressionNode* parse_expression3();
ExpressionNode* parse_expression4();
ExpressionNode* parse_expression5();
VariableNode* parse_variable();
int parse_bits();
int parse_bit();

#pragma endregion

#pragma region Variables

static const int jump_register_address = -1;
static const bool print_ascii = false;

list<int> input_buffer;
list<int> output_buffer;

int jump_register;
map<int, Value> memory;

string* input = NULL;
int position = 0;
int length = 0;

#pragma endregion

#pragma region Implementations

void CodeNode::run(){
	LineNode* line = &lines.at(first_line_number);
	line->instruction->run();
	if (line->go != NULL) {
		int next_line_number = line->go->next_line_number();
		while (next_line_number >= 0) {
			if (lines.count(next_line_number) == 0) {
				show_runtime_error("No line exists with number " + to_string(next_line_number) + ".");
			}
			line = &lines.at(next_line_number);
			line->instruction->run();
			if (line->go == NULL) {
				break;
			}
			next_line_number = line->go->next_line_number();
		}
	}
}

void CommandNode::run() {
	if (print_value == 0 || print_value == 1) {
		print_bit(print_value);
	}
	else {
		memory_write(jump_register_address, { read_bit(), BIT });
	}
}

void AssignmentNode::run() {
	if (address >= jump_register_address) {
		memory_write(address, expression->value());
	}
	else {
		memory_write(address_expression->value().value, expression->value());
	}
}

int GotoNode::next_line_number() {
	if (next.value > -1) {
		if (next.type == ADDRESS_OF_A_BIT) {
			return memory_read(next.value).value;
		}
		return next.value;
	}
	if (next_if_zero > -1 && jump_register == 0) {
		return next_if_zero;
	}
	if (next_if_one > -1 && jump_register == 1) {
		return next_if_one;
	}
	return -1;
}

Value Expression1Node::value() {
	Value value_left = left->value();
	if (right != NULL) {
		Value value_right = right->value();
		if (value_left.value == ADDRESS_OF_A_BIT || value_right.value == ADDRESS_OF_A_BIT) {
			show_runtime_error("The NAND operator requires bit values.");
		}
		return{ ~(value_left.value & value_right.value), BIT };
	}
	return value_left;
}

Value Expression2Node::value() {
	Value value = child->value();
	if (value.value == ADDRESS_OF_A_BIT) {
		show_runtime_error("The THE ADDRESS OF operator requires a bit value.");
	}
	if (value.value < jump_register_address) {
		show_runtime_error("Invalid memory address: " + to_string(value.value) + ".");
	}
	if (value.value == jump_register_address) {
		show_runtime_error("The THE ADDRESS OF operator can't be used with the jump register.");
	}
	return{ value.value, ADDRESS_OF_A_BIT };
}

Value Expression3Node::value() {
	Value value = child->value();
	if (value.value == BIT) {
		show_runtime_error("The THE VALUE BEYOND operator requires an address-of-a-bit value.");
	}
	if (value.value < 0) {
		show_runtime_error("Invalid memory address: " + to_string(value.value) + ".");
	}
	Value result = memory_read(value.value + 1);
	if (result.value == ADDRESS_OF_A_BIT) {
		show_runtime_error("Variable must contain a bit value.");
	}
	return result;
}

Value Expression4Node::value() {
	Value value = child->value();
	if (value.value == BIT) {
		show_runtime_error("The THE VALUE BEYOND operator requires an address-of-a-bit value.");
	}
	if (value.value < 0) {
		show_runtime_error("Invalid memory address: " + to_string(value.value) + ".");
	}
	Value result = memory_read(value.value);
	if (result.value == ADDRESS_OF_A_BIT) {
		show_runtime_error("Variable must contain a bit value.");
	}
	return result;
}

Value Expression5Node::value() {
	return{ constant, UNDEFINED };
}

Value VariableNode::value() {
	if (address >= jump_register_address) {
		return memory_read(address);
	}
	show_runtime_error("Illegal address: " + to_string(address) + ".");
}

#pragma endregion

#pragma region Inputs

string helloworld = "LINENUMBERZEROCODEPRINTZEROGOTOONELINENUMBERONECODEPRINTONEGOTOONEZEROLINENUMBERONEZEROCODEPRINTZEROGOTOONEONELINENUMBERONEONECODEPRINTZEROGOTOONEZEROZEROLINENUMBERONEZEROZEROCODEPRINTONEGOTOONEZEROONELINENUMBERONEZEROONECODEPRINTZEROGOTOONEONEZEROLINENUMBERONEONEZEROCODEPRINTZEROGOTOONEONEONELINENUMBERONEONEONECODEPRINTZEROGOTOONEZEROZEROZEROLINENUMBERONEZEROZEROZEROCODEPRINTZEROGOTOONEZEROZEROONELINENUMBERONEZEROZEROONECODEPRINTONEGOTOONEZEROONEZEROLINENUMBERONEZEROONEZEROCODEPRINTONEGOTOONEZEROONEONELINENUMBERONEZEROONEONECODEPRINTZEROGOTOONEONEZEROZEROLINENUMBERONEONEZEROZEROCODEPRINTZEROGOTOONEONEZEROONELINENUMBERONEONEZEROONECODEPRINTONEGOTOONEONEONEZEROLINENUMBERONEONEONEZEROCODEPRINTZEROGOTOONEONEONEONELINENUMBERONEONEONEONECODEPRINTONEGOTOONEZEROZEROZEROZEROLINENUMBERONEZEROZEROZEROZEROCODEPRINTZEROGOTOONEZEROZEROZEROONELINENUMBERONEZEROZEROZEROONECODEPRINTONEGOTOONEZEROZEROONEZEROLINENUMBERONEZEROZEROONEZEROCODEPRINTONEGOTOONEZEROZEROONEONELINENUMBERONEZEROZEROONEONECODEPRINTZEROGOTOONEZEROONEZEROZEROLINENUMBERONEZEROONEZEROZEROCODEPRINTONEGOTOONEZEROONEZEROONELINENUMBERONEZEROONEZEROONECODEPRINTONEGOTOONEZEROONEONEZEROLINENUMBERONEZEROONEONEZEROCODEPRINTZEROGOTOONEZEROONEONEONELINENUMBERONEZEROONEONEONECODEPRINTZEROGOTOONEONEZEROZEROZEROLINENUMBERONEONEZEROZEROZEROCODEPRINTZEROGOTOONEONEZEROZEROONELINENUMBERONEONEZEROZEROONECODEPRINTONEGOTOONEONEZEROONEZEROLINENUMBERONEONEZEROONEZEROCODEPRINTONEGOTOONEONEZEROONEONELINENUMBERONEONEZEROONEONECODEPRINTZEROGOTOONEONEONEZEROZEROLINENUMBERONEONEONEZEROZEROCODEPRINTONEGOTOONEONEONEZEROONELINENUMBERONEONEONEZEROONECODEPRINTONEGOTOONEONEONEONEZEROLINENUMBERONEONEONEONEZEROCODEPRINTZEROGOTOONEONEONEONEONELINENUMBERONEONEONEONEONECODEPRINTZEROGOTOONEZEROZEROZEROZEROZEROLINENUMBERONEZEROZEROZEROZEROZEROCODEPRINTZEROGOTOONEZEROZEROZEROZEROONELINENUMBERONEZEROZEROZEROZEROONECODEPRINTONEGOTOONEZEROZEROZEROONEZEROLINENUMBERONEZEROZEROZEROONEZEROCODEPRINTONEGOTOONEZEROZEROZEROONEONELINENUMBERONEZEROZEROZEROONEONECODEPRINTZEROGOTOONEZEROZEROONEZEROZEROLINENUMBERONEZEROZEROONEZEROZEROCODEPRINTONEGOTOONEZEROZEROONEZEROONELINENUMBERONEZEROZEROONEZEROONECODEPRINTONEGOTOONEZEROZEROONEONEZEROLINENUMBERONEZEROZEROONEONEZEROCODEPRINTONEGOTOONEZEROZEROONEONEONELINENUMBERONEZEROZEROONEONEONECODEPRINTONEGOTOONEZEROONEZEROZEROZEROLINENUMBERONEZEROONEZEROZEROZEROCODEPRINTZEROGOTOONEZEROONEZEROZEROONELINENUMBERONEZEROONEZEROZEROONECODEPRINTZEROGOTOONEZEROONEZEROONEZEROLINENUMBERONEZEROONEZEROONEZEROCODEPRINTONEGOTOONEZEROONEZEROONEONELINENUMBERONEZEROONEZEROONEONECODEPRINTZEROGOTOONEZEROONEONEZEROZEROLINENUMBERONEZEROONEONEZEROZEROCODEPRINTZEROGOTOONEZEROONEONEZEROONELINENUMBERONEZEROONEONEZEROONECODEPRINTZEROGOTOONEZEROONEONEONEZEROLINENUMBERONEZEROONEONEONEZEROCODEPRINTZEROGOTOONEZEROONEONEONEONELINENUMBERONEZEROONEONEONEONECODEPRINTZEROGOTOONEONEZEROZEROZEROZEROLINENUMBERONEONEZEROZEROZEROZEROCODEPRINTZEROGOTOONEONEZEROZEROZEROONELINENUMBERONEONEZEROZEROZEROONECODEPRINTONEGOTOONEONEZEROZEROONEZEROLINENUMBERONEONEZEROZEROONEZEROCODEPRINTONEGOTOONEONEZEROZEROONEONELINENUMBERONEONEZEROZEROONEONECODEPRINTONEGOTOONEONEZEROONEZEROZEROLINENUMBERONEONEZEROONEZEROZEROCODEPRINTZEROGOTOONEONEZEROONEZEROONELINENUMBERONEONEZEROONEZEROONECODEPRINTONEGOTOONEONEZEROONEONEZEROLINENUMBERONEONEZEROONEONEZEROCODEPRINTONEGOTOONEONEZEROONEONEONELINENUMBERONEONEZEROONEONEONECODEPRINTONEGOTOONEONEONEZEROZEROZEROLINENUMBERONEONEONEZEROZEROZEROCODEPRINTZEROGOTOONEONEONEZEROZEROONELINENUMBERONEONEONEZEROZEROONECODEPRINTONEGOTOONEONEONEZEROONEZEROLINENUMBERONEONEONEZEROONEZEROCODEPRINTONEGOTOONEONEONEZEROONEONELINENUMBERONEONEONEZEROONEONECODEPRINTZEROGOTOONEONEONEONEZEROZEROLINENUMBERONEONEONEONEZEROZEROCODEPRINTONEGOTOONEONEONEONEZEROONELINENUMBERONEONEONEONEZEROONECODEPRINTONEGOTOONEONEONEONEONEZEROLINENUMBERONEONEONEONEONEZEROCODEPRINTONEGOTOONEONEONEONEONEONELINENUMBERONEONEONEONEONEONECODEPRINTONEGOTOONEZEROZEROZEROZEROZEROZEROLINENUMBERONEZEROZEROZEROZEROZEROZEROCODEPRINTZEROGOTOONEZEROZEROZEROZEROZEROONELINENUMBERONEZEROZEROZEROZEROZEROONECODEPRINTONEGOTOONEZEROZEROZEROZEROONEZEROLINENUMBERONEZEROZEROZEROZEROONEZEROCODEPRINTONEGOTOONEZEROZEROZEROZEROONEONELINENUMBERONEZEROZEROZEROZEROONEONECODEPRINTONEGOTOONEZEROZEROZEROONEZEROZEROLINENUMBERONEZEROZEROZEROONEZEROZEROCODEPRINTZEROGOTOONEZEROZEROZEROONEZEROONELINENUMBERONEZEROZEROZEROONEZEROONECODEPRINTZEROGOTOONEZEROZEROZEROONEONEZEROLINENUMBERONEZEROZEROZEROONEONEZEROCODEPRINTONEGOTOONEZEROZEROZEROONEONEONELINENUMBERONEZEROZEROZEROONEONEONECODEPRINTZEROGOTOONEZEROZEROONEZEROZEROZEROLINENUMBERONEZEROZEROONEZEROZEROZEROCODEPRINTZEROGOTOONEZEROZEROONEZEROZEROONELINENUMBERONEZEROZEROONEZEROZEROONECODEPRINTONEGOTOONEZEROZEROONEZEROONEZEROLINENUMBERONEZEROZEROONEZEROONEZEROCODEPRINTONEGOTOONEZEROZEROONEZEROONEONELINENUMBERONEZEROZEROONEZEROONEONECODEPRINTZEROGOTOONEZEROZEROONEONEZEROZEROLINENUMBERONEZEROZEROONEONEZEROZEROCODEPRINTONEGOTOONEZEROZEROONEONEZEROONELINENUMBERONEZEROZEROONEONEZEROONECODEPRINTONEGOTOONEZEROZEROONEONEONEZEROLINENUMBERONEZEROZEROONEONEONEZEROCODEPRINTZEROGOTOONEZEROZEROONEONEONEONELINENUMBERONEZEROZEROONEONEONEONECODEPRINTZEROGOTOONEZEROONEZEROZEROZEROZEROLINENUMBERONEZEROONEZEROZEROZEROZEROCODEPRINTZEROGOTOONEZEROONEZEROZEROZEROONELINENUMBERONEZEROONEZEROZEROZEROONECODEPRINTONEGOTOONEZEROONEZEROZEROONEZEROLINENUMBERONEZEROONEZEROZEROONEZEROCODEPRINTONEGOTOONEZEROONEZEROZEROONEONELINENUMBERONEZEROONEZEROZEROONEONECODEPRINTZEROGOTOONEZEROONEZEROONEZEROZEROLINENUMBERONEZEROONEZEROONEZEROZEROCODEPRINTZEROGOTOONEZEROONEZEROONEZEROONELINENUMBERONEZEROONEZEROONEZEROONECODEPRINTONEGOTOONEZEROONEZEROONEONEZEROLINENUMBERONEZEROONEZEROONEONEZEROCODEPRINTZEROGOTOONEZEROONEZEROONEONEONELINENUMBERONEZEROONEZEROONEONEONECODEPRINTZEROGOTOONEZEROONEONEZEROZEROZEROLINENUMBERONEZEROONEONEZEROZEROZEROCODEPRINTZEROGOTOONEZEROONEONEZEROZEROONELINENUMBERONEZEROONEONEZEROZEROONECODEPRINTZEROGOTOONEZEROONEONEZEROONEZEROLINENUMBERONEZEROONEONEZEROONEZEROCODEPRINTONEGOTOONEZEROONEONEZEROONEONELINENUMBERONEZEROONEONEZEROONEONECODEPRINTZEROGOTOONEZEROONEONEONEZEROZEROLINENUMBERONEZEROONEONEONEZEROZEROCODEPRINTZEROGOTOONEZEROONEONEONEZEROONELINENUMBERONEZEROONEONEONEZEROONECODEPRINTZEROGOTOONEZEROONEONEONEONEZEROLINENUMBERONEZEROONEONEONEONEZEROCODEPRINTZEROGOTOONEZEROONEONEONEONEONELINENUMBERONEZEROONEONEONEONEONECODEPRINTONE";
string helloworldshort = "LINE NUMBER ZERO CODE PRINT ZERO GOTO ONE ONE ZERO ONE  LINE NUMBER ONE CODE PRINT ZERO GOTO ONE ZERO  LINE NUMBER ONE ONE CODE PRINT ZERO GOTO ONE ZERO ZERO ONE ZERO  LINE NUMBER ONE ZERO CODE PRINT ONE GOTO ONE ONE  LINE NUMBER ONE ONE ONE CODE PRINT ONE GOTO ONE ZERO ONE  LINE NUMBER ONE ZERO ONE CODE PRINT ZERO GOTO ONE ONE ZERO  LINE NUMBER ONE ONE ZERO CODE PRINT ONE GOTO ONE ZERO ZERO  LINE NUMBER ONE ZERO ZERO CODE PRINT ONE GOTO ONE ONE ONE ONE  LINE NUMBER ONE ONE ONE ONE CODE PRINT ZERO GOTO ONE ZERO ONE ONE  LINE NUMBER ONE ZERO ONE ONE CODE PRINT ZERO GOTO VARIABLE ONE  LINE NUMBER ONE ONE ZERO ONE CODE PRINT ONE GOTO ONE ONE ONE ZERO  LINE NUMBER ONE ONE ONE ZERO CODE PRINT ZERO GOTO ONE ZERO ZERO ONE  LINE NUMBER ONE ZERO ZERO ONE CODE PRINT ZERO GOTO ONE ZERO ONE ZERO  LINE NUMBER ONE ZERO ONE ZERO CODE PRINT ONE GOTO ONE ONE ZERO ZERO  LINE NUMBER ONE ONE ZERO ZERO CODE PRINT ZERO GOTO ONE ZERO ZERO ZERO  LINE NUMBER ONE ZERO ZERO ZERO CODE PRINT ZERO GOTO ONE ONE ONE ONE ONE  LINE NUMBER ONE ONE ONE ONE ONE CODE PRINT ZERO GOTO ONE ZERO ONE ONE ONE  LINE NUMBER ONE ZERO ONE ONE ONE CODE PRINT ZERO GOTO ONE ONE ZERO ONE ONE  LINE NUMBER ONE ONE ZERO ONE ONE CODE PRINT ONE GOTO ONE ONE ONE ZERO ONE  LINE NUMBER ONE ONE ONE ZERO ONE CODE PRINT ONE GOTO ONE ONE ONE ONE ZERO  LINE NUMBER ONE ONE ONE ONE ZERO CODE PRINT ZERO GOTO ONE ZERO ZERO ONE ONE  LINE NUMBER ONE ZERO ZERO ONE ONE CODE PRINT ZERO GOTO ONE ZERO ONE ZERO ONE  LINE NUMBER ONE ZERO ONE ZERO ONE CODE PRINT ONE GOTO ONE ZERO ONE ONE ZERO  LINE NUMBER ONE ZERO ONE ONE ZERO CODE PRINT ZERO GOTO ONE ONE ZERO ZERO ONE  LINE NUMBER ONE ONE ZERO ZERO ONE CODE PRINT ONE GOTO ONE ONE ZERO ONE ZERO  LINE NUMBER ONE ONE ZERO ONE ZERO CODE VARIABLE ONE EQUALS ONE ONE ONEZERO ZERO GOTO ONE  LINE NUMBER ONE ONE ONE ZERO ZERO CODE VARIABLE ONE EQUALS ONE ZEROZERO ZERO ONE GOTO ONE  LINE NUMBER ONE ZERO ZERO ZERO ONE CODE VARIABLE ONE EQUALS ONE ONEONE ONE ZERO ONE GOTO ONE ONE  LINE NUMBER ONE ZERO ZERO ONE ZERO CODE PRINT ONE GOTO ONE ZERO ONE ZERO ZERO  LINE NUMBER ONE ZERO ONE ZERO ZERO CODE PRINT ONE GOTO ONE ONE ZERO ZERO ZERO  LINE NUMBER ONE ONE ZERO ZERO ZERO CODE PRINT ZERO GOTO ONE ZERO ZERO ZERO ZERO  LINE NUMBER ONE ZERO ZERO ZERO ZERO CODE PRINT ONE GOTO ONE ONE ONE ONE ONE ONE  LINE NUMBER ONE ONE ONE ONE ONE ONE CODE PRINT ONE GOTO ONE ZERO ONE ONE ONE ONE  LINE NUMBER ONE ZERO ONE ONE ONE ONE CODE PRINT ONE GOTO ONE ONE ZEROONE ONE ONE  LINE NUMBER ONE ONE ZERO ONE ONE ONE CODE PRINT ONE GOTO ONE ONE ONEZERO ONE ONE  LINE NUMBER ONE ONE ONE ZERO ONE ONE CODE PRINT ZERO GOTO VARIABLE ONE  LINE NUMBER ONE ONE ONE ONE ZERO ONE CODE PRINT ZERO GOTO ONE ONE ONEONE ONE ZERO  LINE NUMBER ONE ONE ONE ONE ONE ZERO CODE PRINT ZERO GOTO ONE ZEROZERO ONE ONE ONE  LINE NUMBER ONE ZERO ZERO ONE ONE ONE CODE PRINT ONE GOTO ONE ZERO ONEZERO ONE ONE  LINE NUMBER ONE ZERO ONE ZERO ONE ONE CODE PRINT ZERO GOTO ONE ZEROONE ONE ZERO ONE  LINE NUMBER ONE ZERO ONE ONE ZERO ONE CODE PRINT ZERO GOTO ONE ZEROONE ONE ONE ZERO  LINE NUMBER ONE ZERO ONE ONE ONE ZERO CODE PRINT ZERO GOTO ONE ONEZERO ZERO ONE ONE  LINE NUMBER ONE ONE ZERO ZERO ONE ONE CODE PRINT ZERO GOTO ONE ONEZERO ONE ZERO ONE  LINE NUMBER ONE ONE ZERO ONE ZERO ONE CODE PRINT ZERO GOTO ONE ONEZERO ONE ONE ZERO  LINE NUMBER ONE ONE ZERO ONE ONE ZERO CODE PRINT ZERO GOTO ONE ONE ONEZERO ZERO ONE  LINE NUMBER ONE ONE ONE ZERO ZERO ONE CODE PRINT ONE GOTO ONE ONE ONEZERO ONE ZERO  LINE NUMBER ONE ONE ONE ZERO ONE ZERO CODE PRINT ONE GOTO ONE ONE ONEONE ZERO ZERO  LINE NUMBER ONE ONE ONE ONE ZERO ZERO CODE PRINT ONE GOTO ONE ZEROZERO ZERO ONE ONE  LINE NUMBER ONE ZERO ZERO ZERO ONE ONE CODE PRINT ZERO GOTO ONE ZEROZERO ONE ZERO ONE  LINE NUMBER ONE ZERO ZERO ONE ZERO ONE CODE PRINT ONE GOTO ONE ZEROZERO ONE ONE ZERO  LINE NUMBER ONE ZERO ZERO ONE ONE ZERO CODE PRINT ONE GOTO ONE ZEROONE ZERO ZERO ONE  LINE NUMBER ONE ZERO ONE ZERO ZERO ONE CODE PRINT ONE GOTO ONE ZEROONE ZERO ONE ZERO  LINE NUMBER ONE ZERO ONE ZERO ONE ZERO CODE VARIABLE ONE EQUALS ONEZERO ONE ONE ZERO ZERO GOTO ONE ONE  LINE NUMBER ONE ZERO ONE ONE ZERO ZERO CODE PRINT ZERO GOTO ONE ONEZERO ZERO ZERO ONE  LINE NUMBER ONE ONE ZERO ZERO ZERO ONE CODE PRINT ONE GOTO ONE ONEZERO ZERO ONE ZERO  LINE NUMBER ONE ONE ZERO ZERO ONE ZERO CODE PRINT ONE GOTO ONE ONEZERO ONE ZERO ZERO  LINE NUMBER ONE ONE ZERO ONE ZERO ZERO CODE PRINT ONE GOTO ONE ONE ONEZERO ZERO ZERO  LINE NUMBER ONE ONE ONE ZERO ZERO ZERO CODE PRINT ZERO GOTO ONE ZEROZERO ZERO ZERO ONE  LINE NUMBER ONE ZERO ZERO ZERO ZERO ONE CODE PRINT ZERO GOTO ONE ZEROZERO ZERO ONE ZERO  LINE NUMBER ONE ZERO ZERO ZERO ONE ZERO CODE PRINT ONE GOTO ONE ZEROZERO ONE ZERO ZERO  LINE NUMBER ONE ZERO ZERO ONE ZERO ZERO CODE PRINT ZERO GOTO ONE ZEROONE ZERO ZERO ZERO  LINE NUMBER ONE ZERO ONE ZERO ZERO ZERO CODE VARIABLE ONE EQUALS ONEONE ZERO ZERO ZERO ZERO GOTO ONE  LINE NUMBER ONE ONE ZERO ZERO ZERO ZERO CODE PRINT ZERO GOTO ONE ONEONE ONE ONE ONE ONE  LINE NUMBER ONE ONE ONE ONE ONE ONE ONE CODE PRINT ONE GOTO ONE ZEROZERO ZERO ZERO ZERO  LINE NUMBER ONE ZERO ZERO ZERO ZERO ZERO CODE PRINT ONE GOTO ONE ZEROONE ONE ONE ONE ONE  LINE NUMBER ONE ZERO ONE ONE ONE ONE ONE CODE PRINT ZERO GOTO ONE ONEZERO ONE ONE ONE ONE  LINE NUMBER ONE ONE ZERO ONE ONE ONE ONE CODE PRINT ZERO GOTO ONE ONEONE ZERO ONE ONE ONE  LINE NUMBER ONE ONE ONE ZERO ONE ONE ONE CODE PRINT ONE GOTO ONE ONEONE ONE ZERO ONE ONE  LINE NUMBER ONE ONE ONE ONE ZERO ONE ONE CODE PRINT ZERO GOTO ONE ONEONE ONE ONE ZERO ONE  LINE NUMBER ONE ONE ONE ONE ONE ZERO ONE CODE PRINT ZERO GOTO ONE ONEONE ONE ONE ONE ZERO  LINE NUMBER ONE ONE ONE ONE ONE ONE ZERO CODE PRINT ZERO GOTO ONE ZEROZERO ONE ONE ONE ONE  LINE NUMBER ONE ZERO ZERO ONE ONE ONE ONE CODE PRINT ZERO GOTO ONEZERO ONE ZERO ONE ONE ONE  LINE NUMBER ONE ZERO ONE ZERO ONE ONE ONE CODE PRINT ONE GOTO ONE ZEROONE ONE ZERO ONE ONE  LINE NUMBER ONE ZERO ONE ONE ZERO ONE ONE CODE PRINT ZERO GOTO ONEZERO ONE ONE ONE ZERO ONE  LINE NUMBER ONE ZERO ONE ONE ONE ZERO ONE CODE PRINT ZERO GOTO ONEZERO ONE ONE ONE ONE ZERO  LINE NUMBER ONE ZERO ONE ONE ONE ONE ZERO CODE PRINT ZERO GOTO ONE ONEZERO ZERO ONE ONE ONE  LINE NUMBER ONE ONE ZERO ZERO ONE ONE ONE CODE PRINT ZERO GOTO ONE ONEZERO ONE ZERO ONE ONE  LINE NUMBER ONE ONE ZERO ONE ZERO ONE ONE CODE PRINT ONE";
string bitaddition = "LINE NUMBER ONE CODE READ GOTO ONE ZERO LINE NUMBER ONE ZERO CODE VARIABLE ZERO EQUALS THE JUMP REGISTER GOTO ONE ONE LINE NUMBER ONE ONE CODE READ GOTO ONE ZERO ZERO LINE NUMBER ONE ZERO ZERO CODE VARIABLE ONE EQUALS THE JUMP REGISTER GOTO ONE ZERO ONE LINE NUMBER ONE ZERO ONE CODE THE JUMP REGISTER EQUALS OPEN PARENTHESIS VARIABLE ZERO NAND VARIABLE ONE CLOSE PARENTHESIS NAND OPEN PARENTHESIS VARIABLE ZERO NAND VARIABLE ONE CLOSE PARENTHESIS GOTO ONE ONE ZERO IF THE JUMP REGISTER IS EQUAL TO ONE GOTO ONE ZERO ZERO ZERO IF THE JUMP REGISTER IS EQUAL TO ZERO LINE NUMBER ONE ONE ZERO CODE PRINT ONE GOTO ONE ONE ONE LINE NUMBER ONE ONE ONE CODE PRINT ZERO LINE NUMBER ONE ZERO ZERO ZERO CODE THE JUMP REGISTER EQUALS OPEN PARENTHESIS VARIABLE ZERO NAND VARIABLE ZERO CLOSE PARENTHESIS NAND OPEN PARENTHESIS VARIABLE ONE NAND VARIABLE ONE CLOSE PARENTHESIS GOTO ONE ZERO ZERO ONE IF THE JUMP REGISTER IS EQUAL TO ZERO GOTO ONE ZERO ONE ZERO IF THE JUMP REGISTER IS EQUAL TO ONE LINE NUMBER ONE ZERO ZERO ONE CODE PRINT ZERO LINE NUMBER ONE ZERO ONE ZERO CODE PRINT ONE";
string repeatones_original = "LINE NUMBER ZERO CODE VARIABLE ONE EQUALS THE ADDRESS OF VARIABLE ZERO GOTO ONE LINE NUMBER ONE CODE READ GOTO ONE ZERO LINE NUMBER ONE ZERO CODE THE VALUE AT VARIABLE ONE EQUALS THE JUMP REGISTER GOTO ONE ONE IF THE JUMP REGISTER IS ONE GOTO ONE ZERO ZERO IF THE JUMP REGISTER IS ZERO LINE NUMBER ONE ONE CODE VARIABLE ONE EQUALS THE ADDRESS OF THE VALUE BEYOND VARIABLE ONE GOTO ONE LINE NUMBER ONE ZERO ZERO CODE VARIABLE ONE EQUALS THE ADDRESS OF VARIABLE ZERO GOTO ONE ZERO ONE LINE NUMBER ONE ZERO ONE CODE THE JUMP REGISTER EQUALS THE VALUE AT VARIABLE ONE GOTO ONE ONE ZERO IF THE JUMP REGISTER IS ONE GOTO ONE ZERO ZERO ZERO IF THE JUMP REGISTER IS ZERO LINE NUMBER ONE ONE ZERO CODE PRINT ONE GOTO ONE ONE ONE LINE NUMBER ONE ONE ONE CODE VARIABLE ONE EQUALS THE ADDRESS OF THE VALUE BEYOND VARIABLE ONE GOTO ONE ZERO ONE LINE NUMBER ONE ZERO ZERO ZERO CODE PRINT ZERO";
string repeatones = "LINE NUMBER ZERO CODE VARIABLE ONE EQUALS THE ADDRESS OF VARIABLE ZERO GOTO ONE LINE NUMBER ONE CODE READ GOTO ONE ZERO LINE NUMBER ONE ZERO CODE THE JUMP REGISTER EQUALS THE VALUE AT VARIABLE ONE GOTO ONE ONE IF THE JUMP REGISTER IS ONE GOTO ONE ZERO ZERO IF THE JUMP REGISTER IS ZERO LINE NUMBER ONE ONE CODE VARIABLE ONE EQUALS THE ADDRESS OF THE VALUE BEYOND VARIABLE ONE GOTO ONE LINE NUMBER ONE ZERO ZERO CODE VARIABLE ONE EQUALS THE ADDRESS OF VARIABLE ZERO GOTO ONE ZERO ONE LINE NUMBER ONE ZERO ONE CODE THE JUMP REGISTER EQUALS THE VALUE AT VARIABLE ONE GOTO ONE ONE ZERO IF THE JUMP REGISTER IS ONE GOTO ONE ZERO ZERO ZERO IF THE JUMP REGISTER IS ZERO LINE NUMBER ONE ONE ZERO CODE PRINT ONE GOTO ONE ONE ONE LINE NUMBER ONE ONE ONE CODE VARIABLE ONE EQUALS THE ADDRESS OF THE VALUE BEYOND VARIABLE ONE GOTO ONE ZERO ONE LINE NUMBER ONE ZERO ZERO ZERO CODE PRINT ZERO";
string number = "ONE ZERO ZERO ONE ZERO";

int readposition = 0;
int readbuffer[5] = { 1, 0, 1, 1, 1 };

int writeposition = 0;
int writebuffer[500];

#pragma endregion

#pragma region Helpers

void show_parser_error(string message) {
	cout << "ERROR: " << message.c_str() << ". Position " << position << "\n";
	const int preview_length = 60;
	int from = max(position - preview_length / 2, 0);
	int to = min(position + preview_length / 2, length);
	cout << "  " << input->substr(from, to - from) << "\n";
	cout << "  " << string(position - from, ' ') << "^" << "\n";
	exit(1);
}

void show_runtime_error(string message) {
	cout << "RUNTIME ERROR: " << message.c_str() << "\n";
	exit(1);
}

inline char peek() {
	return input->at(position);
}

inline int characters_left() {
	return length - position;
}

inline bool is_whitespace(char character) {
	return isspace(character) != 0;
}

void skip_whitespace() {
	while (position < length) {
		if (!is_whitespace(peek())) {
			return;
		}
		position++;
	}
}

bool consume(char character) {
	if (position < length && peek() == character) {
		position++;
		return true;
	}
	return false;
}

void consume(string symbol) {
	for (char& character : symbol) {
		skip_whitespace();
		if (!consume(character)) {
			show_parser_error("Illegal symbol found. " + symbol + " was expected.");
		}
	}
}

bool check(string symbol) {
	if (position >= length){
		return false;
	}
	int positionTemp = position;
	for (char& character : symbol) {
		while (is_whitespace(input->at(positionTemp))) {
			positionTemp++;
			if (positionTemp >= length){
				return false;
			}
		}
		if (input->at(positionTemp) != character) {
			return false;
		}
		positionTemp++;
	}
	return true;
}

#pragma endregion

#pragma region Symbols

static const string LINE_NUMBER = "LINENUMBER";
static const string CODE = "CODE";
static const string GOTO = "GOTO";
static const string IF_THE_JUMP_REGISTER_IS = "IFTHEJUMPREGISTERIS";
static const string EQUAL_TO = "EQUALTO";
static const string PRINT = "PRINT";
static const string READ = "READ";
static const string EQUALS = "EQUALS";
static const string VARIABLE = "VARIABLE";
static const string THE_JUMP_REGISTER = "THEJUMPREGISTER";
static const string NAND = "NAND";
static const string THE_ADDRESS_OF = "THEADDRESSOF";
static const string THE_VALUE_BEYOND = "THEVALUEBEYOND";
static const string THE_VALUE_AT = "THEVALUEAT";
static const string OPEN_PARENTHESIS = "OPENPARENTHESIS";
static const string CLOSE_PARENTHESIS = "CLOSEPARENTHESIS";
static const string ZERO = "ZERO";
static const string ONE = "ONE";

#pragma endregion

#pragma region Interpreter

void print_bit(int value) {
	if (print_ascii) {
		output_buffer.push_back(value);
		if (output_buffer.size() == 8) {
			int pos = 0;
			char character = 0;
			for (list<int>::reverse_iterator rit = output_buffer.rbegin(); rit != output_buffer.rend(); ++rit) {
				character |= (*rit << pos);
				++pos;
			}
			cout << character;
			output_buffer.clear();
		}
	}
	else {
		cout << value;
	}
}

int read_bit() {
	int value;
	cin >> value;
	if (value != 0 && value != 1) {
		show_runtime_error("Invalid value read.");
	}
	return value;
}

Value memory_read(int address) {
	if (address == jump_register_address) {
		return Value{ jump_register, BIT };
	}
	else {
		if (address > jump_register_address) {
			if (memory.count(address) == 0) {
				return memory[address] = { 0, UNDEFINED };
			}
			else {
				return memory.at(address);
			}
		}
		else {
			show_runtime_error("Invalid memory address: " + to_string(address) + ".");
		}
	}
}

void memory_write(int address, Value value) {
	if (value.type == BIT && value.value != 0 && value.value != 1) {
		show_runtime_error("Illegal value: " + to_string(value.value));
	}
	if (address == jump_register_address) {
		if (value.type == ADDRESS_OF_A_BIT) {
			show_runtime_error("The jump register can't store address-of-a-bit values.");
		}
		jump_register = value.value;
	}
	else {
		if (address > jump_register_address) {
			memory[address] = value;
		}
		else {
			show_runtime_error("Invalid memory address: " + to_string(address) + ".");
		}
	}
}

void run_code(CodeNode* code) {
	if (code != NULL) {
		code->run();
	}
}

#pragma endregion

#pragma region Parser

CodeNode* parse(string* toparse) {
	if (toparse == NULL) {
		throw exception("Input was null.");
	}
	input = toparse;
	length = toparse->length();
	position = 0;
	memory.clear();
	jump_register = 0;
	return parse_code();
}

CodeNode* parse_code() {
	CodeNode* node = new CodeNode();
	LineNode* line = parse_line();
	node->lines[line->line_number] = *line;
	node->first_line_number = line->line_number;
	skip_whitespace();
	while (check(LINE_NUMBER)) {
		line = parse_line();
		if (node->lines.count(line->line_number) > 0) {
			show_parser_error("Line number is " + to_string(line->line_number) + " already defined.");
		}
		node->lines[line->line_number] = *line;
	}
	return node;
}

LineNode* parse_line() {
	LineNode* node = new LineNode();
	consume(LINE_NUMBER);
	node->line_number = parse_bits();
	consume(CODE);
	node->instruction = parse_instruction();
	if (check(GOTO)) {
		node->go = parse_goto();
	}
	return node;
}

InstructionNode* parse_instruction() {
	skip_whitespace();
	if (check(PRINT) || check(READ)) {
		return parse_command();
	}
	return parse_assignment();	
}

InstructionNode* parse_command() {
	CommandNode* node = new CommandNode();
	skip_whitespace();
	if (check(PRINT)) {
		consume(PRINT);
		node->print_value = parse_bit();
		return node;
	}
	if (check(READ)) {
		consume(READ);
		return node;
	}
	show_parser_error("Illegal symbol found. Command was expected.");
}

InstructionNode* parse_assignment() {
	AssignmentNode* node = new AssignmentNode();
	skip_whitespace();
	if (check(VARIABLE) || check(THE_JUMP_REGISTER)) {
		VariableNode* temp = parse_variable();
		node->address = temp->address;
		delete temp;
	}
	else {
		node->address_expression = parse_expression();
	}
	consume(EQUALS);
	node->expression = parse_expression();
	return node;
}

GotoNode* parse_goto() {
	GotoNode* node = new GotoNode();
	consume(GOTO);
	if (check(VARIABLE)) {
		consume(VARIABLE);
		// TODO: ist das korrekt?
		node->next.type == ADDRESS_OF_A_BIT;
	}
	int address1 = parse_bits();
	if (check(IF_THE_JUMP_REGISTER_IS)) {
		consume(IF_THE_JUMP_REGISTER_IS);
		if (check(EQUAL_TO)){
			consume(EQUAL_TO);
		}
		int bit1 = parse_bit();
		(bit1 == 0) ? (node->next_if_zero = address1) : (node->next_if_one = address1);
		if (check(GOTO)) {
			consume(GOTO);
			int address2 = parse_bits();
			consume(IF_THE_JUMP_REGISTER_IS);
			if (check(EQUAL_TO)){
				consume(EQUAL_TO);
			}
			int bit2 = parse_bit();
			if (bit1 == bit2) {
				show_parser_error("Illegal symbol found. Conditional goto with different bit constant was expected.");
			}
			(bit2 == 0) ? (node->next_if_zero = address2) : (node->next_if_one = address2);
		}
		return node;
	}
	node->next.value = address1;
	return node;
}

ExpressionNode* parse_expression() {
	Expression1Node* node = new Expression1Node();
	skip_whitespace();
	node->left = parse_expression2();
	if (check(NAND)) {
		consume(NAND);
		node->right = parse_expression2();
	}
	return node;
}

ExpressionNode* parse_expression2() {
	skip_whitespace();
	if (check(THE_ADDRESS_OF)) {
		consume(THE_ADDRESS_OF);
		Expression2Node* node = new Expression2Node();
		node->child = parse_expression3();
		return node;
	}
	return parse_expression3();
}

ExpressionNode* parse_expression3() {
	skip_whitespace();
	if (check(THE_VALUE_BEYOND)) {
		consume(THE_VALUE_BEYOND);
		Expression3Node* node = new Expression3Node();
		node->child = parse_expression4();
		return node;
	}
	return parse_expression4();
}

ExpressionNode* parse_expression4() {
	skip_whitespace();
	if (check(THE_VALUE_AT)) {
		consume(THE_VALUE_AT);
		Expression4Node* node = new Expression4Node();
		node->child = parse_expression5();
		return node;
	}
	return parse_expression5();
}

ExpressionNode* parse_expression5() {
	skip_whitespace();
	if (check(VARIABLE) || check(THE_JUMP_REGISTER)) {
		return parse_variable();
	}
	if (check(ZERO) || check(ONE)) {
		Expression5Node* node = new Expression5Node();
		node->constant = parse_bits();
		return node;
	}
	if (check(OPEN_PARENTHESIS)) {
		consume(OPEN_PARENTHESIS);
		ExpressionNode* node = parse_expression();
		consume(CLOSE_PARENTHESIS);
		return node;
	}
	show_parser_error("Illegal symbol found. Expression was expected.");
}

// TODO: hier muss ein Value zurückgegeben werden, nicht ein node
// TODO: vielleicht den expression node um die funktion address() erweitern, damit auch die adresse falls nötig zur runtime ausgewertet werden kann
VariableNode* parse_variable() {
	if (check(VARIABLE)) {
		consume(VARIABLE);
		VariableNode* node = new VariableNode();
		node->address = parse_bits();
		return node;
	}
	if (check(THE_JUMP_REGISTER)) {
		consume(THE_JUMP_REGISTER);
		VariableNode* node = new VariableNode();
		node->address = jump_register_address;
		return node;
	}
	show_parser_error("Illegal symbol found. Variable was expected.");
}

int parse_bits() {
	skip_whitespace();
	int bit = parse_bit();
	int bits = bit;
	while (check(ZERO) || check(ONE)) {
		int bit = parse_bit();
		bits = (bits << 1) | bit;
	}
	return bits;
}

int parse_bit() {
	skip_whitespace();
	if (check(ZERO)) {
		consume(ZERO);
		return 0;
	}
	if (check(ONE)) {
		consume(ONE);
		return 1;
	}
	show_parser_error("Illegal symbol found. Bit constant was expected.");
}

#pragma endregion

int main() {
	//run_code(parse(&helloworld));
	//run_code(parse(&helloworldshort));
	//run_code(parse(&bitaddition));
	run_code(parse(&repeatones_original));
}