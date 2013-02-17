//=============================================================================================================
#ifndef _INTERPRETER_H_
#define _INTERPRETER_H_

#include <iostream>
#include <string>
#include <list>

#define lexer_out(x)      { std::cout << "* LEXER: " << x << "\n"; }
#define parser_out(x)     { std::cout << "* PARSER: " << x << "\n"; }
#define assert(r, e, x)   { if( !(x) ) { std::cout << "* ERROR: " << e << "!\n"; return r; } }
#define nassert(r, e, x)  { if( x ) { std::cout << "* ERROR: " << e << "!\n"; return r; } }

#define NUM_STAT          1
#define OP_PRINT          0x0

#define HEAP_SIZE         262144
#define CODE_SIZE         65536
#define ENTRY_SIZE        (1 + 2 * sizeof(void*))

class Interpreter
{
    friend int yyparse();
    friend int yylex();

    typedef void (*stm_ptr)(void*, void*);
    static stm_ptr op_special[NUM_STAT];

    // special statements
    static void Execute_Print(void* arg1, void* arg2);

private:
    typedef std::list<std::string*> garbagelist;

    garbagelist garbage;
    std::string progname;

    char* bytecode;
    char* heap;
    size_t bytesize;
    size_t heapsize;
    
    void AddCodeEntry(unsigned char opcode, void* arg1, void* arg2);
    void Cleanup();

    template <typename T>
    void* Allocate(T* ptr);

public:
    Interpreter();
    ~Interpreter();

    bool Compile(const std::string& file);
    bool Run();
};

template <typename T>
void* Interpreter::Allocate(T* data)
{
    assert(0, "Interpreter::Allocate<T>(): Heap not yet created", heap);
    nassert(0, "Interpreter::Allocate<T>(): Out of memory",
        (heapsize + sizeof(T)) >= HEAP_SIZE);

    char* ptr = (heap + heapsize);

    new (ptr) T(*data);
    heapsize += sizeof(T);

    return ptr;
}

#endif
//=============================================================================================================
