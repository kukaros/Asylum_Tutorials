//=============================================================================================================
#ifndef _TYPES_H_
#define _TYPES_H_

enum symbol_type
{
	Type_Unknown = 0,
	Type_Char = 1,
	Type_Short = 2,
	Type_Integer = 3,
	Type_Float = 4,
	Type_String = 5
};

enum statement_type
{
	Type_Expression = 1,
	Type_Conditional = 2,
	Type_Loop = 3,
	Type_Return = 4
};

enum unary_expr
{
	Expr_Inc,
	Expr_Dec,
	Expr_Neg,
	Expr_Not
};

struct expression_desc
{
	bytestream    bytecode;
	std::string   value;
	int           type;
	int           address;
	bool          constexpr;

	expression_desc()
		: address(0), constexpr(false) {}
};

struct declaration_desc
{
	std::string name;
	bytestream bytecode;
	expression_desc* expr;

	declaration_desc()
		: expr(0) {}
};

struct statement_desc
{
	bytestream bytecode;
	int type;
	int scope;

	statement_desc()
		: type(0), scope(1) {}
};

typedef std::list<expression_desc*> exprlist;
typedef std::list<declaration_desc*> decllist;

struct symbol_desc
{
	std::string  name;
	bytestream   bytecode;
	exprlist*    args;

	int          address;
	int          type;
	bool         isfunc;

	symbol_desc()
		: args(0), address(0), type(Type_Unknown), isfunc(false)
	{
	}
};

struct unresolved_reference
{
	symbol_desc* func;  // if function call
	int offset;         // relative offset to jump

	unresolved_reference()
		: func(0), offset(0) {}
};

typedef std::list<symbol_desc*> symbollist;
typedef std::list<statement_desc*> statlist;
typedef std::map<std::string, symbol_desc*> symboltable;
typedef std::vector<symboltable> scopetable;

#endif
//=============================================================================================================
