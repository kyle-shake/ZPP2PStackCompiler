/*
File: codegen.cc
Author: Kyle Shake
Special Consideration to Dr. Jerzy Jaromyczk

Last Modified: 12/13/17

12/13 Update:

Globals not working with function args.





12/12 Update:
More expressions added (/, %, &&, ||, !, >, >=, <=)
Statement implementations need to be coded
"If-Else" & "If-Then-Else" Statements working (so far...)

**NEED TO TEST IF-THEN, IF-THEN-ELSE ****

12/2 Update:
"If" statements completed and working

Simple "for" statements completed and working (so far...)
further testing is needed.

Not sure if globals are working... next to test with further for loop testing


12/1 Update:
Working on If loop and Global variables



*/

#include <string>
#include <sstream>
#include "pstcode.h"
#include "codegen.h"
using namespace std;

//Generates the code vector
PstackCode CodeGen::generate(Visitable *vis)
{
    vis->accept(this);
    return code;
}

//Implementation for Program
void CodeGen::visitProg(Prog *prog)
{
    code.begin_prog();
    code.prolog(symbols);

    // Insert call to main(), to be patched up later.
    code.add(I_CALL);
    int patchloc = code.pos();
    code.add(0);
    code.add(0);
    code.add(I_ENDPROG);

    // Generate code for the functions.
    prog->listfunction_->accept(this);

    // Now look up the address of main, and throw if it wasn't defined.
    if (!symbols.exists("main"))
        throw UnknownFunc("main");
    int level = symbols.levelof("main");
    int addr = symbols["main"]->address();

    // Patch up the address of main.
    code.at(patchloc) = level;
    code.at(patchloc + 1) = addr;

    code.end_prog();
}

//Implementation for Function
void CodeGen::visitFun(Fun *fun)
{
    fun->type_->accept(this);
    // return type in currtype, but currently ignored (always int)

    visitIdent(fun->ident_);
    Ident fun_name = currid;

    if (symbols.exists(fun_name))
        throw Redeclared(fun_name);

    symbols.insert(Symbol(fun_name, currtype, code.pos()));

    code.add(I_PROC);
    int patchloc = code.pos(); // to be filled with number of local variables.
    code.add(0);
    code.add(code.pos() + 1); // function code starts next

    symbols.enter(); // since parameters are local to the function
    // Adds entries to symbol table, sets funargs
    fun->listdecl_->accept(this);

    symbols[fun_name]->numargs() = funargs; //set number of arguments for Argument checking

    int startvar = symbols.numvars();

    // Generate code for function body.
    fun->liststm_->accept(this);

    // Fill in number of local variables.
    code.at(patchloc) = symbols.numvars() - startvar;
    symbols.leave();

    // Return, popping off our parameters.
    code.add(I_ENDPPROC);
    code.add(funargs);
}

//Implementation for Global
void CodeGen::visitGlobal(Global *global)
{
    //throw Unimplemented("Global variables are not implemented");
    global->type_->accept(this);

    visitIdent(global->ident_);
    Ident global_name = currid;

    if(symbols.exists(global_name))
        throw Redeclared(global_name);

    symbols.insert(Symbol(global_name, currtype, code.pos(), 0));

}

//Implementation for Declaration
void CodeGen::visitDec(Dec *dec)
{
    dec->type_->accept(this); // sets currtype
    dec->listident_->accept(this); // visitListIdent; uses currtype
}

//Implementation for Statement: Declaration
void CodeGen::visitSDecl(SDecl *sdecl)
{
    sdecl->decl_->accept(this); // visitDec
}

//Implementation for Statement: Expression 
void CodeGen::visitSExp(SExp *sexp)
{
    sexp->exp_->accept(this);

    // Pop and discard the expression's value.  pstack doesn't have a
    // POP instruction, but a conditional jump to the next instruction
    // (PC + 2) will do the trick.
    code.add(I_JR_IF_TRUE);
    code.add(2);
}

//Implementation for Statement: If (SIf)
void CodeGen::visitSIf(SIf *sif)
{
    sif->exp_->accept(this);
    code.add(I_JR_IF_FALSE);
    code.add(0);
    int patchloc = code.pos()-1;

    sif->stm_->accept(this);
    code.at(patchloc) = code.pos() - (patchloc - 1);

}

//Implementation for Statement: If Then (SIfThen)
void CodeGen::visitSIfThen(SIfThen *sifthen)
{
    sifthen->exp_->accept(this);
    code.add(I_JR_IF_FALSE);
    code.add(0);
    int patchloc = code.pos()-1;
    sifthen->stm_->accept(this);
    code.at(patchloc) = code.pos() - (patchloc - 1);
}

//Implementation for Statement: If *STM* Else *STM* (SIfElse)
void CodeGen::visitSIfElse(SIfElse *sifelse)
{
    sifelse->exp_->accept(this);
    code.add(I_JR_IF_FALSE); // Jump past first statement if expression is false
    code.add(0); //Jump location to be set after second statement is found
    int patchloc = code.pos()-1;

    sifelse->stm_1->accept(this); //Body of first statement
    code.add(I_JR); //Jump past second statement if first statement is executed
    code.add(0);  //Jump location to be set after the end of second statement is found
    int patchloc2 = code.pos()-1;

    code.at(patchloc) = code.pos() - (patchloc - 1); //Set the location of Jump if false 

    sifelse->stm_2->accept(this); //Body of second statement

    code.at(patchloc2) = code.pos() - (patchloc2 - 1);// Set the location of jump after first statement
}

//Implementation for Statement: If Then *STM* Else *STM* (SIfThEl)
void CodeGen::visitSIfThEl(SIfThEl *sifthel)
{
    sifthel->exp_->accept(this);
    code.add(I_JR_IF_FALSE); // Jump past first statement if expression is false
    code.add(0); //Jump location to be set after second statement is found
    int patchloc = code.pos()-1;

    sifthel->stm_1->accept(this); //Body of first statement
    code.add(I_JR); //Jump past second statement if first statement is executed
    code.add(0);  //Jump location to be set after the end of second statement is found
    int patchloc2 = code.pos()-1;

    code.at(patchloc) = code.pos() - (patchloc - 1); //Set the location of Jump if false 

    sifthel->stm_2->accept(this); //Body of second statement

    code.at(patchloc2) = code.pos() - (patchloc2 - 1);// Set the location of jump after first statement
}

//Implementation for Statement: Repeat *STM* Until (SRepUnt) 
void CodeGen::visitSRepUnt(SRepUnt *srepunt)
{
    int looploc = code.pos();
    srepunt->stm_->accept(this);
    srepunt->exp_->accept(this);
    code.add(I_JR_IF_FALSE);
    code.add(looploc - (code.pos() -1));
}

//Implementation for Statement Block (SBlock)
void CodeGen::visitSBlock(SBlock *sblock)
{
    sblock->liststm_->accept(this);
}

//Implementation for Statement: For (SFor) *Variables declared before loop, incremented inside*
void CodeGen::visitSFor(SFor *sfor)
{
    int looploc = code.pos(); //Beginning of for loop
    sfor->exp_->accept(this);
    code.add(I_JR_IF_FALSE); //Jump past for loop
    code.add(0);
    int patchloc = code.pos() - 1;

    sfor->stm_->accept(this); //Body of for loop
    code.add(I_JR);
    code.add(looploc - (code.pos() - 1)); //offset to looploc
    code.at(patchloc) = code.pos() - (patchloc - 1);

}

//Implementation for Statement: For (SForSc) *For with variables declared and incremented in scope* 
void CodeGen::visitSForSc(SForSc *sforsc)
{
    code.add(I_PROC);
    int patchloc = code.pos(); // to be filled with number of local variables.
    code.add(0);
    code.add(code.pos() + 1); 

    symbols.enter(); // since parameters are local to the for loop
    // Adds entries to symbol table
    sforsc->exp_1->accept(this);

    int startvar = symbols.numvars();

    int looploc = code.pos();//Beginning of loop at bool expression
    sforsc->exp_2->accept(this);

    //Jump location to be set after body of For Loop
    code.add(I_JR_IF_FALSE);
    code.add(0);
    int patchloc2 = code.pos() - 1;

    sforsc->stm_->accept(this); //Body.
    sforsc->exp_3->accept(this); //Iterator increments after for loop runs

    //Jump back to beginning
    code.add(I_JR);
    code.add(looploc - (code.pos() - 1));

    // Fill in number of local variables.
    code.at(patchloc) = symbols.numvars() - startvar;
    symbols.leave();

    // Return, popping off our parameters.
    code.add(I_ENDPPROC);
    code.add(0);

    //Setting the jump location if bool is found false
    code.at(patchloc2) = code.pos()-(patchloc2 - 1);
}

//Implementation for Statement: While (SWhile)
void CodeGen::visitSWhile(SWhile *swhile)
{
    int looploc = code.pos(); // Beginning of test
    swhile->exp_->accept(this);
    code.add(I_JR_IF_FALSE);  // Jump past the body.
    code.add(0);
    int patchloc = code.pos() - 1;

    swhile->stm_->accept(this); // Body.
    code.add(I_JR);
    code.add(looploc - (code.pos() - 1)); // offset to looploc
    code.at(patchloc) = code.pos() - (patchloc - 1);
}

//Implementation for Statement: Return (SReturn)
void CodeGen::visitSReturn(SReturn *sreturn)
{

    sreturn->exp_->accept(this);
    // Store the top of stack (return value) at (bp-funargs)
    code.add(I_VARIABLE);
    code.add(0);
    code.add(-(funargs+1));

    //Now generate code for the return expression
    sreturn->exp_->accept(this);
    code.add(I_ASSIGN);
    code.add(1);

    // And return, popping off our parameters.
    code.add(I_ENDPPROC);
    code.add(funargs);
}

//Implementation for Expression: Declaration with Assignment(EDecAss)
void CodeGen::visitEDecAss(EDecAss *edecass)
{
    edecass->type_->accept(this); //get currtype
    visitIdent(edecass->ident_); //get currid
   
    symbols.insert(Symbol(currid, currtype, code.pos(), 0));

    //Computing address
    code.add(I_VARIABLE);
    code.add(symbols.levelof(currid));
    code.add(symbols[currid]->address());

    //One copy of address for assignment, one for the result
    code.add_dup();

    //Generate RHS
    edecass->exp_->accept(this);
    
    //Store value at address
    code.add(I_ASSIGN);
    code.add(1);

    code.add(I_VALUE);
    
}

//Implementation for Expression: Assignment (EAss)
void CodeGen::visitEAss(EAss *eass)
{
    visitIdent(eass->ident_); // sets currid
    if (!symbols.exists(currid))
        throw UnknownVar(currid);

    // Compute the address.
    code.add(I_VARIABLE);
    code.add(symbols.levelof(currid));
    code.add(symbols[currid]->address());

    // One copy of the address for the assignment, one for the result.
    code.add_dup();

    // Generate code for the value of the RHS.
    eass->exp_->accept(this);

    // Store the value at the computed address.
    code.add(I_ASSIGN);
    code.add(1);

    // Dereference the address and return its value.
    code.add(I_VALUE);
}

//Implementation for Expression: Less Than (ELt)
void CodeGen::visitELt(ELt *elt)
{
    elt->exp_1->accept(this);
    elt->exp_2->accept(this);
    code.add(I_LESS);
}

//Implementation for Expression: Less Than or Equal To (ELtEq)
//Added p-code for Less than or equal to --KWS 12/12
void CodeGen::visitELtEq(ELtEq *elteq)
{
//    throw Unimplemented("Less than or equal to is not implemented");

    elteq->exp_1->accept(this);
    elteq->exp_2->accept(this);
    code.add(I_LESSEQ);

}

//Implementation for Expression: Greater Than (EGt) 
void CodeGen::visitEGt(EGt *egt)
{
    egt->exp_1->accept(this);
    egt->exp_2->accept(this);
    code.add(I_GREATER);
}

//Implementation for Expression: Greater Than or Equal To (EGtEq) 
//Added p-code for Greater Than or Equal to --KWS 12/12
void CodeGen::visitEGtEq(EGtEq *egteq)
{
//    throw Unimplemented("Greater than or equal to is not implemented");
    egteq->exp_1->accept(this);
    egteq->exp_2->accept(this);
    code.add(I_GREATEREQ);
}

//Implementation for Expression: Equal To (EEqto)
void CodeGen::visitEEqto(EEqto *eeqto)
{
    eeqto->exp_1->accept(this);
    eeqto->exp_2->accept(this);
    code.add(I_EQUAL);
}

//Implementation for Expression: And (EAnd) 
void CodeGen::visitEAnd(EAnd *eand)
{
    eand->exp_1->accept(this);
    eand->exp_2->accept(this);
    code.add(I_AND);
}

//Implementation for Expression: Or (EOr) 
void CodeGen::visitEOr(EOr *eor)
{
    eor->exp_1->accept(this);
    eor->exp_2->accept(this);
    code.add(I_OR);
}

//Implementation for Expression: Not (ENot) 
void CodeGen::visitENot(ENot *enot)
{
    enot->exp_->accept(this);
    //enot->exp_2->accept(this);
    code.add(I_NOT);
}

//Implementation for Expression: Addition (EAdd)
void CodeGen::visitEAdd(EAdd *eadd)
{
    eadd->exp_1->accept(this);
    eadd->exp_2->accept(this);
    code.add(I_ADD);
}

//Implementation for Expression: Subtraction (ESub)
void CodeGen::visitESub(ESub *esub)
{
    esub->exp_1->accept(this);
    esub->exp_2->accept(this);
    code.add(I_SUBTRACT);
}

//Implementation for Expression: Multiplication (EMul)
void CodeGen::visitEMul(EMul *emul)
{
    emul->exp_1->accept(this);
    emul->exp_2->accept(this);
    code.add(I_MULTIPLY);
}

//Implementation for Expression: Division (EDiv) 
void CodeGen::visitEDiv(EDiv *ediv)
{
    ediv->exp_1->accept(this);
    ediv->exp_2->accept(this);
    code.add(I_DIVIDE);
}

//Implementation for Expression: Modulo (EMod) 
void CodeGen::visitEMod(EMod *emod)
{
    emod->exp_1->accept(this);
    emod->exp_2->accept(this);
    code.add(I_MODULO);
}

//Implementation for Call
void CodeGen::visitCall(Call *call)
{
    visitIdent(call->ident_);
    if (!symbols.exists(currid))
        throw UnknownFunc(currid);
    
    //Check Argument Count from Call against Function Arg Count
    int numArgsgiven = call->listexp_->size();
    string ArgsGiven = static_cast<ostringstream*>( &(ostringstream() << numArgsgiven) )->str();
    int numArgsrequired = symbols[currid]->numargs();
    string ArgsRequired = static_cast<ostringstream*>( &(ostringstream() << numArgsrequired) )->str();
    if(numArgsgiven != numArgsrequired)
         throw ArgCountError(currid, ArgsRequired, ArgsGiven);

    int level = symbols.levelof(currid);
    int addr = symbols[currid]->address();

    // Make room on the stack for the return value.  Assumes all functions
    // will return some value.
    code.add(I_CONSTANT);
    code.add(0);

    // Generate code for the expressions (which leaves their values on the
    // stack when executed).
    call->listexp_->accept(this);

    code.add(I_CALL);
    code.add(level);
    code.add(addr);
    
    // The result, if any, is left on the stack.
}

//Implementation for Expression: Variable (EVar)
void CodeGen::visitEVar(EVar *evar)
{
    visitIdent(evar->ident_); // sets currid
    if (!symbols.exists(currid))
        throw UnknownVar(currid);

    // Compute the address.
    code.add(I_VARIABLE);
    code.add(symbols.levelof(currid));
    code.add(symbols[currid]->address());
    // Dereference it.
    code.add(I_VALUE);
}

//Implementation for Expression: String (EStr)
void CodeGen::visitEStr(EStr *estr)
{
    code.add(I_CONSTANT);
    code.add(0); // must be patched for string address
    visitString(estr->string_);
}

//Implementation for Expression: Integer (EInt)
void CodeGen::visitEInt(EInt *eint)
{
    visitInteger(eint->integer_);
}

//Implementation for Expression: Double (EDouble)
void CodeGen::visitEDouble(EDouble *edouble)
{
    visitDouble(edouble->double_);
}

//Implementation for setting type: integer
void CodeGen::visitTInt(TInt *)
{
    currtype = TY_INT;
}

//Implementation for setting type: double
void CodeGen::visitTDouble(TDouble *)
{
    currtype = TY_DOUBLE;
}

void CodeGen::visitListFunction(ListFunction* listfunction)
{
    // Generate code for each function in turn.
    for (ListFunction::iterator i = listfunction->begin() ; i != listfunction->end() ; ++i)
    {
        (*i)->accept(this);
    }
}

void CodeGen::visitListStm(ListStm* liststm)
{
    // Generate code for each statement in turn.
    for (ListStm::iterator i = liststm->begin() ; i != liststm->end() ; ++i)
    {
        (*i)->accept(this);
    }
}

void CodeGen::visitListDecl(ListDecl* listdecl)
{
    // ListDecl is a function parameter list, so we can compute funargs here.
    funargs = listdecl->size();

    int currarg = 0;
    for (ListDecl::iterator i = listdecl->begin() ; i != listdecl->end() ; ++i)
    {
        (*i)->accept(this); // visitDec

        // The first argument (currarg = 0) has address -nargs; the last
        // (currarg = nargs - 1) has address -1.
        symbols[currid]->address() = currarg - funargs;
    }
}

void CodeGen::visitListIdent(ListIdent* listident)
{
    // Add all the identifiers to the symbol table.  Assumes currtype is
    // already set.
    for (ListIdent::iterator i = listident->begin(); i != listident->end(); ++i)
    {
        visitIdent(*i); // sets currid

        // First local variable (numvars = funargs) has address 3, etc.
        // If this ListIdent is actually part of a parameter list, these
        // addresses will be fixed up by visitListDecl.
        symbols.insert(Symbol(currid, currtype, 3 + symbols.numvars() - funargs));
    }
}

void CodeGen::visitListExp(ListExp* listexp)
{
    // Evaluate each expression in turn, leaving all the values on the stack.
    for (ListExp::iterator i = listexp->begin() ; i != listexp->end() ; ++i)
    {
        (*i)->accept(this);
    }
}

//Implementation to add Integer to code
void CodeGen::visitInteger(Integer x)
{
    code.add(I_CONSTANT);
    code.add(x);
}

//Implementation to add Char to code
void CodeGen::visitChar(Char x)
{
    code.add(I_CONSTANT);
    code.add(x);
}

//Implementation to add Double to code
void CodeGen::visitDouble(Double x)
{
    throw Unimplemented("doubles are unimplemented");
}

//Implementatino to add String to code
void CodeGen::visitString(String x)
{
    code.add_string(x, code.pos() - 1);
}

//Implementation to add Identifier to code
void CodeGen::visitIdent(Ident x)
{
    currid = x;
}


