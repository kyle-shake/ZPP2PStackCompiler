/*
File: codegen.h
Author: Kyle Shake with special consideration to Dr. Jerzy Jaromcyzk

Last Modified: 12/12/17




*/
#ifndef CODEGEN_H_INCLUDED
#define CODEGEN_H_INCLUDED

#include "Absyn.H"
#include "pstcode.h"
#include <stdexcept>
#include <string>
#include <stdio.h>
#include <stdlib.h>

class UnknownVar : public std::logic_error
{
public:
    UnknownVar(const std::string &name)
        : logic_error("Unknown variable \"" + name + "\"")
    {}
};

class UnknownFunc : public std::logic_error
{
public:
    UnknownFunc(const std::string &name)
        : logic_error("Unknown function \"" + name + "\"")
    {}
};

class ArgCountError : public std::logic_error
{
public:
    ArgCountError(const std::string &name, const std::string &numargs, const std::string &numargsgiven)
        : logic_error("Function \""+ name + "\" requires " + numargs + " arguments. " + numargsgiven + " arguments given.")
    {}
};

class Redeclared : public std::logic_error
{
public:
    Redeclared(const std::string &name)
        : logic_error("Symbol \"" + name + "\" redeclared")
    {}
};

class Unimplemented : public std::logic_error
{
public:
    Unimplemented(const std::string &what) : logic_error(what) {}
};

class CodeGen : public Visitor
{
private:
    Ident currid;        // identifier from last visitIdent
    type_t currtype;     // type from last visitT{Int,Double}
    PstackCode code;     // buffer to hold generated code
    SymbolTable symbols; // symbol table
    int funargs;         // number of parameters in current function.
public:
    CodeGen()
        : currid(""), currtype(TY_BAD), code(), symbols(), funargs(-1)
    {}
    PstackCode generate(Visitable *vis);

    // These will never actually be called; instead, node->accept(this) will
    // call the method for the concrete class (visitProg rather than
    // visitProgram, visitTInt or visitTDouble rather than visitType, etc.)
    void visitProgram(Program *) {}
    void visitFunction(Function *) {}
    void visitDecl(Decl *) {}
    void visitStm(Stm *) {}
    void visitExp(Exp *) {}
    void visitType(Type *) {}

    void visitProg(Prog *p);
    void visitFun(Fun *p);
    void visitGlobal(Global *p);
    void visitDec(Dec *p);
    void visitListFunction(ListFunction* p);
    void visitListExp(ListExp* p);
    void visitListStm(ListStm* p);
    void visitListDecl(ListDecl* p);
    void visitListIdent(ListIdent* p);

    // Statements
    void visitSDecl(SDecl *p);
    void visitSExp(SExp *p);
    void visitSIf(SIf *p);
    void visitSIfThen(SIfThen *p);
    void visitSIfElse(SIfElse *p); //Added & implemented 12/12
    void visitSIfThEl(SIfThEl *p); //Added & implemented 12/12
    void visitSRepUnt(SRepUnt *p); //Added & implemented 12/12
    void visitSBlock(SBlock *p);
    void visitSFor(SFor *p);
    void visitSForSc(SForSc *p); //Added, not implemented
    void visitSWhile(SWhile *p);
    void visitSReturn(SReturn *p);

    // Expressions
    void visitEDecAss(EDecAss *p);
    void visitEAss(EAss *p);
    void visitELt(ELt *p);
    void visitELtEq(ELtEq *p); //Added & implemented 12/12
    void visitEGt(EGt *p); //Added & implemented 12/12
    void visitEGtEq(EGtEq *p); //Added & implemented 12/12
    void visitEEqto(EEqto *p); //Added & implemented 12/12
    void visitEAnd(EAnd *p); //Added & implemented 12/12
    void visitEOr(EOr *p); //Added & implemented 12/12
    void visitENot(ENot *p); //Added, not implemented
    void visitEAdd(EAdd *p);
    void visitESub(ESub *p);
    void visitEMul(EMul *p);
    void visitEDiv(EDiv *p); //Added & implemented 12/12
    void visitEMod(EMod *p); //Added & implemented 12/12
    void visitCall(Call *p);
    void visitEVar(EVar *p);
    void visitEStr(EStr *p);
    void visitEInt(EInt *p);
    void visitEDouble(EDouble *p);


    // Types
    void visitTInt(TInt *p);
    void visitTDouble(TDouble *p);

    // Literals and identifiers.
    void visitInteger(Integer i);
    void visitDouble(Double d);
    void visitChar(Char c);
    void visitString(String s);
    void visitIdent(String s);
};

#endif

