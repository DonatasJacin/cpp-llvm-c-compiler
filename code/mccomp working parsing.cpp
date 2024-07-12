#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <map>
#include <memory>
#include <queue>
#include <string.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using namespace llvm;
using namespace llvm::sys;

FILE *pFile;


//===----------------------------------------------------------------------===//
// Lexer
//===----------------------------------------------------------------------===//

// The lexer returns one of these for known things.
enum TOKEN_TYPE {

  IDENT = -1,        // [a-zA-Z_][a-zA-Z_0-9]*
  ASSIGN = int('='), // '='

  // delimiters
  LBRA = int('{'),  // left brace
  RBRA = int('}'),  // right brace
  LPAR = int('('),  // left parenthesis
  RPAR = int(')'),  // right parenthesis
  SC = int(';'),    // semicolon
  COMMA = int(','), // comma

  // types
  INT_TOK = -2,   // "int"
  VOID_TOK = -3,  // "void"
  FLOAT_TOK = -4, // "float"
  BOOL_TOK = -5,  // "bool"

  // keywords
  EXTERN = -6,  // "extern"
  IF = -7,      // "if"
  ELSE = -8,    // "else"
  WHILE = -9,   // "while"
  RETURN = -10, // "return"
  // TRUE   = -12,     // "true"
  // FALSE   = -13,     // "false"

  // literals
  INT_LIT = -14,   // [0-9]+
  FLOAT_LIT = -15, // [0-9]+.[0-9]+
  BOOL_LIT = -16,  // "true" or "false" key words

  // logical operators
  AND = -17, // "&&"
  OR = -18,  // "||"

  // operators
  PLUS = int('+'),    // addition or unary plus
  MINUS = int('-'),   // substraction or unary negative
  ASTERIX = int('*'), // multiplication
  DIV = int('/'),     // division
  MOD = int('%'),     // modular
  NOT = int('!'),     // unary negation

  // comparison operators
  EQ = -19,      // equal
  NE = -20,      // not equal
  LE = -21,      // less than or equal to
  LT = int('<'), // less than
  GE = -23,      // greater than or equal to
  GT = int('>'), // greater than

  // special tokens
  EOF_TOK = 0, // signal end of file

  // invalid
  INVALID = -100 // signal invalid token
};

// TOKEN struct is used to keep track of information about a token
struct TOKEN {
  int type = -100;
  std::string lexeme;
  int lineNo;
  int columnNo;
};

static std::string IdentifierStr; // Filled in if IDENT
static int IntVal;                // Filled in if INT_LIT
static bool BoolVal;              // Filled in if BOOL_LIT
static float FloatVal;            // Filled in if FLOAT_LIT
static std::string StringVal;     // Filled in if String Literal
static int lineNo, columnNo;

static TOKEN returnTok(std::string lexVal, int tok_type) {
  TOKEN return_tok;
  return_tok.lexeme = lexVal;
  return_tok.type = tok_type;
  return_tok.lineNo = lineNo;
  return_tok.columnNo = columnNo - lexVal.length() - 1;
  return return_tok;
}

// Read file line by line -- or look for \n and if found add 1 to line number
// and reset column number to 0
/// gettok - Return the next token from standard input.
static TOKEN gettok() {

  static int LastChar = ' ';
  static int NextChar = ' ';

  // Skip any whitespace.
  while (isspace(LastChar)) {
    if (LastChar == '\n' || LastChar == '\r') {
      lineNo++;
      columnNo = 1;
    }
    LastChar = getc(pFile);
    columnNo++;
  }

  if (isalpha(LastChar) ||
      (LastChar == '_')) { // identifier: [a-zA-Z_][a-zA-Z_0-9]*
    IdentifierStr = LastChar;
    columnNo++;

    while (isalnum((LastChar = getc(pFile))) || (LastChar == '_')) {
      IdentifierStr += LastChar;
      columnNo++;
    }

    if (IdentifierStr == "int")
      return returnTok("int", INT_TOK);
    if (IdentifierStr == "bool")
      return returnTok("bool", BOOL_TOK);
    if (IdentifierStr == "float")
      return returnTok("float", FLOAT_TOK);
    if (IdentifierStr == "void")
      return returnTok("void", VOID_TOK);
    if (IdentifierStr == "bool")
      return returnTok("bool", BOOL_TOK);
    if (IdentifierStr == "extern")
      return returnTok("extern", EXTERN);
    if (IdentifierStr == "if")
      return returnTok("if", IF);
    if (IdentifierStr == "else")
      return returnTok("else", ELSE);
    if (IdentifierStr == "while")
      return returnTok("while", WHILE);
    if (IdentifierStr == "return")
      return returnTok("return", RETURN);
    if (IdentifierStr == "true") {
      BoolVal = true;
      return returnTok("true", BOOL_LIT);
    }
    if (IdentifierStr == "false") {
      BoolVal = false;
      return returnTok("false", BOOL_LIT);
    }

    return returnTok(IdentifierStr.c_str(), IDENT);
  }

  if (LastChar == '=') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // EQ: ==
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("==", EQ);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("=", ASSIGN);
    }
  }

  if (LastChar == '{') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("{", LBRA);
  }
  if (LastChar == '}') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("}", RBRA);
  }
  if (LastChar == '(') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok("(", LPAR);
  }
  if (LastChar == ')') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(")", RPAR);
  }
  if (LastChar == ';') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(";", SC);
  }
  if (LastChar == ',') {
    LastChar = getc(pFile);
    columnNo++;
    return returnTok(",", COMMA);
  }

  if (isdigit(LastChar) || LastChar == '.') { // Number: [0-9]+.
    std::string NumStr;

    if (LastChar == '.') { // Floatingpoint Number: .[0-9]+
      do {
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      FloatVal = strtof(NumStr.c_str(), nullptr);
      return returnTok(NumStr, FLOAT_LIT);
    } else {
      do { // Start of Number: [0-9]+
        NumStr += LastChar;
        LastChar = getc(pFile);
        columnNo++;
      } while (isdigit(LastChar));

      if (LastChar == '.') { // Floatingpoint Number: [0-9]+.[0-9]+)
        do {
          NumStr += LastChar;
          LastChar = getc(pFile);
          columnNo++;
        } while (isdigit(LastChar));

        FloatVal = strtof(NumStr.c_str(), nullptr);
        return returnTok(NumStr, FLOAT_LIT);
      } else { // Integer : [0-9]+
        IntVal = strtod(NumStr.c_str(), nullptr);
        return returnTok(NumStr, INT_LIT);
      }
    }
  }

  if (LastChar == '&') {
    NextChar = getc(pFile);
    if (NextChar == '&') { // AND: &&
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("&&", AND);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("&", int('&'));
    }
  }

  if (LastChar == '|') {
    NextChar = getc(pFile);
    if (NextChar == '|') { // OR: ||
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("||", OR);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("|", int('|'));
    }
  }

  if (LastChar == '!') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // NE: !=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("!=", NE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("!", NOT);
      ;
    }
  }

  if (LastChar == '<') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // LE: <=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok("<=", LE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok("<", LT);
    }
  }

  if (LastChar == '>') {
    NextChar = getc(pFile);
    if (NextChar == '=') { // GE: >=
      LastChar = getc(pFile);
      columnNo += 2;
      return returnTok(">=", GE);
    } else {
      LastChar = NextChar;
      columnNo++;
      return returnTok(">", GT);
    }
  }

  if (LastChar == '/') { // could be division or could be the start of a comment
    LastChar = getc(pFile);
    columnNo++;
    if (LastChar == '/') { // definitely a comment
      do {
        LastChar = getc(pFile);
        columnNo++;
      } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

      if (LastChar != EOF)
        return gettok();
    } else
      return returnTok("/", DIV);
  }

  // Check for end of file.  Don't eat the EOF.
  if (LastChar == EOF) {
    columnNo++;
    return returnTok("0", EOF_TOK);
  }

  // Otherwise, just return the character as its ascii value.
  int ThisChar = LastChar;
  std::string s(1, ThisChar);
  LastChar = getc(pFile);
  columnNo++;
  return returnTok(s, int(ThisChar));
}

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.
static TOKEN CurTok;
static std::deque<TOKEN> tok_buffer;

static TOKEN getNextToken() {

  if (tok_buffer.size() == 0)
    tok_buffer.push_back(gettok());

  TOKEN temp = tok_buffer.front();
  tok_buffer.pop_front();

  return CurTok = temp;
}

static void putBackToken(TOKEN tok) {tok_buffer.push_front(tok);}

// Function to check if a value is a member of an array, used to simplify
// very long if statements
static bool CheckMembership(int arr[], int size, int value) {
  for (int i = 0; i < size; i++) {
    if (arr[i] == value) {
      return true;
    }
  }
  return false;
}

//===----------------------------------------------------------------------===//
// AST nodes
//===----------------------------------------------------------------------===//

/// ASTnode - Base class for all AST nodes.
class ASTnode {
public:
  virtual ~ASTnode() {}
  virtual Value *codegen(int block_index) = 0;
  virtual std::string to_string(std::string ident_level) const {};
};

/// IntASTnode - Class for integer literals like 1, 2, 10
class IntASTnode : public ASTnode {
  int Val;

public:
  IntASTnode(int val) : Val(val) {}
  //virtual Value *codegen() override;
  virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      oss << ident_level << Val;
      return oss.str();
  };
  Value *codegen(int block_index) override;
};

// FloatASTNode - Class for float iterals like 1.5, 2.1, 31.5
class FloatASTnode : public ASTnode {
  float Val;

  public:
    FloatASTnode(float val) : Val(val) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      oss << ident_level << Val;
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

// BoolASTnode - Class for boolean literals true and false 
class BoolASTnode : public ASTnode {
  bool Val;

  public:
    BoolASTnode(bool val) : Val(val) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      oss << ident_level << Val;
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

// VariableASTnode - Class for referencing a variable like "x"
class VariableASTnode : public ASTnode {
  std::string Name;

  public:
    // Passes in the actual address of the argument name, and we "promise" not to change it with const
    VariableASTnode(const std::string &name) : Name(name) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      oss << ident_level << Name;
      return oss.str();
    }
    std::string getName() {
      return Name;
    }
    Value *codegen(int block_index) override;
};

class VariableAssignmentASTnode : public ASTnode {
  std::unique_ptr<VariableASTnode> Variable;
  std::unique_ptr<ASTnode> Val;

  public:
    VariableAssignmentASTnode(std::unique_ptr<VariableASTnode> variable, std::unique_ptr<ASTnode> val)
    : Variable(std::move(variable)), Val(std::move(val)) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";
      oss << ident_level << "Assigned identifier \n" << Variable->to_string(child_ident_level) << "\n" << Val->to_string(child_ident_level);
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

class VariableDeclarationASTnode : public ASTnode {
  std::string Name;
  std::string Type;

  public:
    VariableDeclarationASTnode(const std::string &name, const std::string &type)
    : Name(name), Type(type) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      oss << ident_level << "Declared " << Type << " " << Name;
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

class BlockASTnode : public ASTnode {
  std::vector<std::unique_ptr<VariableDeclarationASTnode>> Declarations;
  std::vector<std::unique_ptr<ASTnode>>Statements;

  public:
    BlockASTnode(std::vector<std::unique_ptr<VariableDeclarationASTnode>> declarations, std::vector<std::unique_ptr<ASTnode>> statements)
    : Declarations(std::move(declarations)), Statements(std::move(statements)) {} 
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";
      oss << ident_level << "Block";
      for (auto &decl : Declarations) {
        oss << "\n" << decl->to_string(child_ident_level);
      }
      for (auto &stmt : Statements) {
        oss << "\n" << stmt->to_string(child_ident_level);
      }
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

// BinaryASTnode - Class for binary operators like + * - /
class BinaryASTnode : public ASTnode {
  std::string Op; // Stores what operator this is such as + * - /
  std::unique_ptr<ASTnode> LHS,RHS; // Smart pointers for AST nodes of left and right operands

  public:
    BinaryASTnode(const std::string &op, std::unique_ptr<ASTnode> LHS, std::unique_ptr<ASTnode> RHS)
    : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {} // Moves memory from arguments to attributes
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";
      oss << ident_level << "Binary operation" << "\n" << LHS->to_string(child_ident_level) << "\n" + child_ident_level << Op << "\n" <<  RHS->to_string(child_ident_level);
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

// Unary operators - and ! ?
class UnaryASTnode : public ASTnode {
  std::string Op; // Stores what operator this is such as ! and ?
  std::unique_ptr<ASTnode> Val; // Smart pointer for AST nodes of operand
  public:
    UnaryASTnode(const std::string &op, std::unique_ptr<ASTnode> val) : Op(op), Val(std::move(val)) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";
      oss << ident_level << "Unary operation of " << Op << Val->to_string(child_ident_level);
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

// CallASTnode - Class for function calls such as fib(8)
class CallASTnode : public ASTnode {
  std::string CallFunc; //Name of function thats called
  std::vector<std::unique_ptr<ASTnode>> Args; //Dynamically allocated array of smart pointers to AST objects

  public:
    CallASTnode(const std::string &callfunc, std::vector<std::unique_ptr<ASTnode>> args)
    : CallFunc(callfunc), Args(std::move (args)) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";
      oss << ident_level << "Calling function " << CallFunc << " with arguments ";
      for (auto &arg : Args) {
        oss << "\n" << arg->to_string(child_ident_level);
      }
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

class FunctionParamASTnode : public ASTnode {
  std::string Name;
  std::string Type;

  public:
    FunctionParamASTnode(const std::string &name, const std::string &type)
    : Name(name), Type(type) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      oss << "\n" << ident_level << "Function parameter " << Type << " " << Name;
      return oss.str();
    }
    std::string getName() {
      return Name;
    }
    std::string getType() {
      return Type;
    }
    Value *codegen(int block_index) override;
};

// FunctionPrototypeASTnode - Class for capturing name, and argument names a function takes
class FunctionPrototypeASTnode : public ASTnode {
  std::string Name;
  std::string Type;
  std::vector<std::unique_ptr<FunctionParamASTnode>> Args; //Dynamically allocated array of smart pointers to function parameter AST objects

  public:
    FunctionPrototypeASTnode(const std::string &name, const std::string &type, std::vector<std::unique_ptr<FunctionParamASTnode>> args)
    : Name(name), Type(type), Args(std::move(args)) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";
      oss << ident_level << "Function Prototype " << Type << " " << Name << " with parameters ";
      for (auto &arg : Args) {
        oss << arg->to_string(child_ident_level);
      }
      return oss.str();
    }
    std::string getName() {
      return Name;
    }
    std::string getType() {
      return Type;
    }
    std::string getArgType (int index) {
      std::string arg_type = Args[index]->getType();
      return arg_type;
    }
    Function *codegen(int block_index) override;
};

// FunctionDefASTnode - Class for representing function definitions
class FunctionDefASTnode : public ASTnode {
  std::unique_ptr<FunctionPrototypeASTnode> Prototype;
  std::unique_ptr<BlockASTnode> Body;

  public: 
    FunctionDefASTnode(std::unique_ptr<FunctionPrototypeASTnode> prototype, std::unique_ptr<BlockASTnode> body)
    : Prototype(std::move (prototype)), Body(std::move (body)) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";
      oss << ident_level << "Function Definition \n" << Prototype->to_string(child_ident_level) << "\n" << Body->to_string(child_ident_level);
      return oss.str();
    }
    Function *codegen(int block_index) override;
};

class ExternASTnode: public ASTnode {
  std::string Name;
  std::string Type;
  std::vector<std::unique_ptr<FunctionParamASTnode>> Params;

  public:
    ExternASTnode(const std::string &name, const std::string &type, std::vector<std::unique_ptr<FunctionParamASTnode>> params)
    : Name(name), Type(type), Params(std::move(params)) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";
      oss << ident_level << "Extern " << Type << " " << Name << " with parameters";
      for (auto &param : Params) {
        oss << param->to_string(child_ident_level);
      }
      oss << "\n";
      return oss.str();
    }
    Function *codegen(int block_index) override;
};

class IfExprASTnode : public ASTnode {
  std::unique_ptr<ASTnode> Cond;
  std::unique_ptr<BlockASTnode> Then, Else;

  public:
    IfExprASTnode(std::unique_ptr<ASTnode> Cond, std::unique_ptr<BlockASTnode> Then,
                  std::unique_ptr<BlockASTnode> Else)
                  : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";
      oss << ident_level << "If \n" << Cond->to_string(child_ident_level) << "\n" << Then->to_string(child_ident_level);
      if (Else != nullptr) {
        oss << "\n" << Else->to_string(child_ident_level);
      }
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

class WhileExprASTnode : public ASTnode {
  std::unique_ptr<ASTnode> Cond, Then;

  public:
    WhileExprASTnode(std::unique_ptr<ASTnode> Cond, std::unique_ptr<ASTnode> Then)
                    : Cond(std::move(Cond)), Then(std::move(Then)) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";
      oss << ident_level << "While \n" << Cond->to_string(child_ident_level) << "\n" << Then->to_string(child_ident_level);
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

class ReturnExprASTnode : public ASTnode {
  std::unique_ptr<ASTnode> ReturnValue;

  public:
    ReturnExprASTnode(std::unique_ptr<ASTnode> returnvalue) : ReturnValue(std::move(returnvalue)) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";;
      oss << ident_level << "Return expression";
      if (ReturnValue != nullptr) {
        oss << "\n" << ReturnValue->to_string(child_ident_level);
      }
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

class RootASTnode : public ASTnode {
  std::vector<std::unique_ptr<ExternASTnode>> Ext_List;
  std::vector<std::unique_ptr<ASTnode>> Decl_List;

  public:
    RootASTnode(std::vector<std::unique_ptr<ExternASTnode>> ext_list, std::vector<std::unique_ptr<ASTnode>> decl_list)
    : Ext_List(std::move(ext_list)), Decl_List(std::move(decl_list)) {}
    virtual std::string to_string(std::string ident_level) const override {
      std::ostringstream oss;
      std::string child_ident_level = ident_level + " |-";
      oss << ident_level << "Program root \n";
      for (auto &ext : Ext_List) {
        oss << ext->to_string(child_ident_level);
      }
      for (auto &decl : Decl_List) {
        oss << decl->to_string(child_ident_level);
      }
      return oss.str();
    }
    Value *codegen(int block_index) override;
};

/* add other AST nodes as nessasary */

//===----------------------------------------------------------------------===//
// Recursive Descent Parser - Function call for each production
//===----------------------------------------------------------------------===//

// Helper functions for error handling - taken from llvm tutorial
std::unique_ptr<ASTnode> LogError(std::string Str) {
  fprintf(stderr, "\nLogError: %s\n\n", Str.c_str());
  return nullptr;
}
std::unique_ptr<FunctionPrototypeASTnode> LogErrorP(std::string Str) {
  LogError(Str);
  return nullptr;
}

/* Add function calls for each production */


// Function declarations
static std::unique_ptr<RootASTnode> ParseProgram();
static std::vector<std::unique_ptr<ExternASTnode>> ParseExternList();
static std::vector<std::unique_ptr<ASTnode>> ParseDeclList();
static std::unique_ptr<ExternASTnode> ParseExtern();
static std::vector<std::unique_ptr<ExternASTnode>> ParseExternListPrime(std::vector<std::unique_ptr<ExternASTnode>> ext_list);
static std::string ParseTypeSpec();
static std::vector<std::unique_ptr<FunctionParamASTnode>> ParseParams();
static std::string ParseVarType();
static std::vector<std::unique_ptr<FunctionParamASTnode>> ParseParamList();
static std::unique_ptr<FunctionParamASTnode> ParseParam();
static std::vector<std::unique_ptr<FunctionParamASTnode>> ParseParamListPrime(std::vector<std::unique_ptr<FunctionParamASTnode>> params);
static std::unique_ptr<ASTnode> ParseDecl();
static std::vector<std::unique_ptr<ASTnode>> ParseDeclListPrime(std::vector<std::unique_ptr<ASTnode>> decl_list);
static std::unique_ptr<FunctionDefASTnode> ParseVoidFunDecl();
static std::unique_ptr<ASTnode> ParseTypeNameDecl();
static std::unique_ptr<BlockASTnode> ParseBlock();
static std::unique_ptr<FunctionDefASTnode> ParseVarFunDecl(std::string type, std::string identifier);
static std::vector<std::unique_ptr<VariableDeclarationASTnode>> ParseLocalDecls();
static std::vector<std::unique_ptr<ASTnode>> ParseStmtList();
static std::unique_ptr<VariableDeclarationASTnode> ParseLocalDecl();
static std::vector<std::unique_ptr<VariableDeclarationASTnode>> ParseLocalDeclsPrime(std::vector<std::unique_ptr<VariableDeclarationASTnode>> declarations);
static std::unique_ptr<ASTnode> ParseStmt();
static std::vector<std::unique_ptr<ASTnode>> ParseStmtListPrime(std::vector<std::unique_ptr<ASTnode>> stmt_list);
static std::unique_ptr<ASTnode> ParseExprStmt();
static std::unique_ptr<IfExprASTnode> ParseIf();
static std::unique_ptr<WhileExprASTnode> ParseWhile();
static std::unique_ptr<ReturnExprASTnode> ParseReturn();
static std::unique_ptr<ASTnode> ParseExpr();
static std::unique_ptr<ASTnode> ParseRval();
static std::unique_ptr<BlockASTnode> ParseElse();
static std::unique_ptr<ASTnode> ParseRvalOne();
static std::unique_ptr<BinaryASTnode>  ParseRvalPrime(std::unique_ptr<ASTnode> lhs);
static std::unique_ptr<ASTnode> ParseRvalTwo();
static std::unique_ptr<BinaryASTnode> ParseRvalOnePrime(std::unique_ptr<ASTnode> lhs);
static std::unique_ptr<ASTnode> ParseRvalThree();
static std::unique_ptr<BinaryASTnode> ParseRvalTwoPrime(std::unique_ptr<ASTnode> lhs);
static std::unique_ptr<ASTnode> ParseRvalFour();
static std::unique_ptr<BinaryASTnode> ParseRvalThreePrime(std::unique_ptr<ASTnode> lhs);
static std::unique_ptr<ASTnode> ParseRvalFive();
static std::unique_ptr<BinaryASTnode> ParseRvalFourPrime(std::unique_ptr<ASTnode> lhs);
static std::unique_ptr<ASTnode> ParseRvalSix();
static std::unique_ptr<BinaryASTnode> ParseRvalFivePrime(std::unique_ptr<ASTnode> lhs);
static std::unique_ptr<ASTnode> ParseRvalSeven();
static std::unique_ptr<ASTnode> ParseRvalEight();
static std::unique_ptr<ASTnode> ParseRvalNine();
static std::vector<std::unique_ptr<ASTnode>> ParseArgs();
static std::vector<std::unique_ptr<ASTnode>> ParseArgList();
static std::vector<std::unique_ptr<ASTnode>> ParseArgListPrime(std::vector<std::unique_ptr<ASTnode>> args);

// program_prime ::= program eof
static std::unique_ptr<RootASTnode> parser() {
  // Root of the parser and AST tree, prepares first token, creates root ast node, and calls first production
  CurTok = getNextToken();
  std::unique_ptr<RootASTnode> program;
  if (CurTok.type != EOF_TOK){
    program = ParseProgram();
    return std::move(program);
  }
  return nullptr;
}

// program ::= extern_list decl_list
//          | decl_list
static std::unique_ptr<RootASTnode> ParseProgram() {
  // Creates dynamically allocated arrays for both externs and declarations
  std::vector<std::unique_ptr<ExternASTnode>> ext_list;
  std::vector<std::unique_ptr<ASTnode>> decl_list;
  // Checks if current token is in FIRST set of extern_list or decl_list
  // Calls appropriate procedures for productions
  if (CurTok.type == EXTERN) {
    ext_list = ParseExternList();
    decl_list = ParseDeclList();
  } else if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK || CurTok.type == VOID_TOK) {
    decl_list = ParseDeclList();
  } else {
    throw LogError("Syntax Error: Expected extern for extern or type int, float, bool, or void");
  }
  // Creates root AST node and returns
  std::unique_ptr<RootASTnode> root = std::make_unique<RootASTnode>(std::move(ext_list), std::move(decl_list));
  return std::move(root);
}

// extern_list ::= extern extern_list_prime
static std::vector<std::unique_ptr<ExternASTnode>> ParseExternList() {
  // Creates AST node for first extern and dynamically allocated array of AST nodes for next externs
  std::unique_ptr<ExternASTnode> ext;
  std::vector<std::unique_ptr<ExternASTnode>> ext_list;
  // Calls procedures corresponding to production in order
  ext = ParseExtern();
  // After receiving first extern, moves it to front of extern list
  ext_list.push_back(std::move(ext));
  // Extern list with first extern is passed to production which generates further externs
  ext_list = ParseExternListPrime(std::move(ext_list));
  return(std::move(ext_list));
}

// extern_list_prime ::= extern extern_list_prime
//                    | epsilon
static std::vector<std::unique_ptr<ExternASTnode>> ParseExternListPrime(std::vector<std::unique_ptr<ExternASTnode>> ext_list) {
  // Checks if next token is extern or if in the follow set of extern_list_prime, otherwise, throws error
  if (CurTok.type == EXTERN) {
    // Creates AST node for new extern and assigns to it by calling ParseExtern production
    std::unique_ptr<ExternASTnode> ext;
    ext = ParseExtern();
    // Pushes new extern onto end of extern list
    ext_list.push_back(std::move(ext));
    // Extern list passed to production recursively to generate further externs
    ext_list = ParseExternListPrime(std::move(ext_list));
    return std::move(ext_list);
  } else if (CurTok.type == VOID_TOK || CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
    // Current token is in the FOLLOW set of extern_list_prime, so we return and stop generating externs here
    return std::move(ext_list); 
  } else {
    throw LogError("Syntax Error: Expected an extern or declaration after extern");
  }
}

// extern ::= "extern" type_spec IDENT "(" params ")" ";"
static std::unique_ptr<ExternASTnode> ParseExtern() {
  // Creates variables to hold type and identifier of extern
  CurTok = getNextToken(); // eat extern
  std::string type = ParseTypeSpec();
  std::string identifier;
  if (CurTok.type == IDENT){
    identifier = CurTok.lexeme;
    CurTok = getNextToken(); // eat IDENT
  } else {
    throw LogError("Syntax Error: Expected identifier after type");
  }
  // Creates dynamically allocated array of smart pointers to function parameter AST nodes
  std::vector<std::unique_ptr<FunctionParamASTnode>> params;
  if (CurTok.type == LPAR) {
    CurTok = getNextToken(); // eat (
    // Array of function parameter AST nodes is generated by ParseParams production and returned
    params = ParseParams();
  } else {
    throw LogError("Syntax Error: Expected ( after identifier");
  }
  if (CurTok.type == RPAR) {
    CurTok = getNextToken(); // eat )
  } else {
    throw LogError("Syntax Error: Expected ) after parameters");
  }
  if (CurTok.type == SC) {
    CurTok = getNextToken(); // eat ;
    // Finally, construct the extern AST node given all the information captured in variables and param array
    // then return pointer to it
    std::unique_ptr<ExternASTnode> ext = std::make_unique<ExternASTnode>(identifier, type, std::move(params));
    return std::move(ext);
  } else {
    throw LogError("Syntax Error: Expected ; after )");
  }
}

// type_spec ::= "void"
//            |  var_type
static std::string ParseTypeSpec() {
  // This production simply matches void (specifically for void functions) or any of the 3 remaining types
  // int, float, or bool
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
    std::string type;
    type = ParseVarType();
    return type;
  } else if (CurTok.type == VOID_TOK) {
    std::string type = CurTok.lexeme;
    CurTok = getNextToken(); // eat VOID
    return type;
  } else {
    throw LogError("Syntax Error: Expected 'void' or variable type 'int', 'float', or 'bool");
  }
}

// var_type  ::= "int" |  "float" |  "bool"
static std::string ParseVarType() {
  // Match any of int, float, or bool and return outcome
  std::string type;
  switch (CurTok.type) {
    case INT_TOK: {
      CurTok = getNextToken();
      type = "int";
      return type;
    }
    case FLOAT_TOK: {
      CurTok = getNextToken();
      type = "float";
      return type;
    }
    case BOOL_TOK: {
      CurTok = getNextToken();
      type = "bool";
      return type;
    }
    default: {
      throw LogError("Syntax Error: Expected 'int', 'float', or 'bool'");
    }
  }
}

// params ::= param_list  
//        |  "void" | epsilon
static std::vector<std::unique_ptr<FunctionParamASTnode>> ParseParams() {
  // Parse the parameter list of a function or extern
  // Either its empty (epsilon), is void, or is a list of parameters
  // Create dynamically allocated array of smart pointers to function parameter AST nodes
  std::vector<std::unique_ptr<FunctionParamASTnode>> params;
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
    // We have a parameter list so we the call appropriate production and store the return array in params
    params = ParseParamList();
    return std::move(params);
  } else if (CurTok.type == VOID_TOK) {
    // Parameter of function is void, so return a singleton array containing a parameter of name "void" and type "VOID"
    std::string identifier, type;
    identifier = CurTok.lexeme;
    type = "VOID";
    CurTok = getNextToken();
    std::unique_ptr<FunctionParamASTnode> void_param = std::make_unique<FunctionParamASTnode>(identifier, type);
    params.push_back(std::move(void_param));
    return std::move(params);
  } else if (CurTok.type == RPAR) {
    // Current token is in follow set of params, so there are no parameters and this is valid
    return std::move(params);
  } else {
    // Current token did not match any previous case, report syntax error
    throw LogError("Syntax Error: Expected 'void', variable type 'int', 'float', or 'bool', or )");
  }
}

// param_list ::= param param_list_prime
static std::vector<std::unique_ptr<FunctionParamASTnode>> ParseParamList() {
  // Parses list of parameters recursively
  // Creates AST node of first param and array of smart pointers to function param AST nodes
  std::unique_ptr<FunctionParamASTnode> param;
  std::vector<std::unique_ptr<FunctionParamASTnode>> params;
  // Matches and stores first param, then adds to end of param array
  param = ParseParam();
  params.push_back(std::move(param));
  // Parse remaining params recursively and assign to param arry, then return
  params = ParseParamListPrime(std::move(params));
  return std::move(params);
}

// param_list_prime ::= "," param param_list_prime
//                    | epsilon
static std::vector<std::unique_ptr<FunctionParamASTnode>> ParseParamListPrime(std::vector<std::unique_ptr<FunctionParamASTnode>> params) {
  // Match either a comma or right parenthesis ( ) is in FOLLOW set)
  if (CurTok.type == COMMA) {
    // We have a comma call the appropriate productions to param and param_list_prime
    getNextToken(); // eat comma
    // Create AST node for new param
    std::unique_ptr<FunctionParamASTnode> param;
    param = ParseParam();
    // Add new param to end of param array, generate any further params recursively then return array
    params.push_back(std::move(param));
    params = ParseParamListPrime(std::move(params));
    return std::move(params);
  } else if (CurTok.type == RPAR) {
    return std::move(params);
  } else {
    throw LogError("Syntax Error: Expected ) or ,");
  }
}

// param ::= var_type IDENT
static std::unique_ptr<FunctionParamASTnode> ParseParam() {
  std::string type;
  type = ParseVarType();
  if (CurTok.type == IDENT) {
    std::string identifier = CurTok.lexeme;
    std::unique_ptr<FunctionParamASTnode> param = std::make_unique<FunctionParamASTnode>(identifier, type);
    CurTok = getNextToken();
    return std::move(param);
  } else {
    throw LogError("Syntax Error: Expected identifier after var_type");
  }
}

// decl_list ::= decl decl_list_prime
static std::vector<std::unique_ptr<ASTnode>> ParseDeclList() {
  std::vector<std::unique_ptr<ASTnode>> decl_list;
  std::unique_ptr<ASTnode> decl;
  decl = ParseDecl();
  decl_list.push_back(std::move(decl));
  decl_list = ParseDeclListPrime(std::move(decl_list));
  return std::move(decl_list);
}

// decl_list_prime ::= decl decl_list_prime
//                    | epsilon
static std::vector<std::unique_ptr<ASTnode>> ParseDeclListPrime(std::vector<std::unique_ptr<ASTnode>> decl_list) {
  if (CurTok.type == EOF_TOK) {
    return std::move(decl_list); // reached end of file, EOF is in FOLLOW set of decl_list_prime
  } else if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK || CurTok.type == VOID_TOK) {
    std::unique_ptr<ASTnode> decl;
    decl = ParseDecl();
    decl_list.push_back(std::move(decl));
    decl_list = ParseDeclListPrime(std::move(decl_list));
    return std::move(decl_list);
  } else {
    throw LogError("Syntax Error: Expected eof or type 'int', 'float', 'bool', or 'void'");
  }

}

// decl ::= voidfun_decl
//     |  typename_decl
static std::unique_ptr<ASTnode> ParseDecl() {
  if (CurTok.type == VOID_TOK) {
    std::unique_ptr<FunctionDefASTnode> decl;
    decl = ParseVoidFunDecl();
    return decl;
  } else if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
    std::unique_ptr<ASTnode> decl;
    decl = ParseTypeNameDecl();
    return decl;
  } else {
    throw LogError("Syntax Error: Expected type 'void' or variable type 'int', 'float', or 'bool'");
  }
}

// voidfun_decl ::= "void" IDENT "(" params ")" block
static std::unique_ptr<FunctionDefASTnode> ParseVoidFunDecl() {
  std::string func_type = CurTok.lexeme;
  std::string func_identifier;
  CurTok = getNextToken(); // eat void
  if (CurTok.type == IDENT) {
    func_identifier = CurTok.lexeme;
    CurTok = getNextToken(); // eat IDENT
  } else {
    throw LogError("Syntax Error: Expected identifier after type 'void'");
  }
  std::vector<std::unique_ptr<FunctionParamASTnode>> func_params;
  if (CurTok.type == LPAR) {
    CurTok = getNextToken(); // eat (
    func_params = ParseParams();
  } else {
    throw LogError("Syntax Error: Expected ( after identifier");
  }
  if (CurTok.type == RPAR) {
    CurTok = getNextToken(); // eat )
    std::unique_ptr<BlockASTnode> func_block;
    func_block = ParseBlock();
    std::unique_ptr<FunctionPrototypeASTnode> func_proto = std::make_unique<FunctionPrototypeASTnode>(func_identifier, func_type, std::move(func_params));
    std::unique_ptr<FunctionDefASTnode> func = std::make_unique<FunctionDefASTnode>(std::move(func_proto), std::move(func_block));
    return std::move(func);
  } else {
    throw LogError("Syntax Error: Expected ) after parameters");
  }
}

// typename_decl ::= var_type IDENT varfun_decl
static std::unique_ptr<ASTnode> ParseTypeNameDecl() {
  std::string type;
  type = ParseVarType();
  if (CurTok.type == IDENT) {
    std::string identifier;
    identifier = CurTok.lexeme;
    CurTok = getNextToken();
    std::unique_ptr<FunctionDefASTnode> func = ParseVarFunDecl(type, identifier);
    if (func != nullptr){
      return std::move(func);
    } else {
      std::unique_ptr<VariableDeclarationASTnode> variable = std::make_unique<VariableDeclarationASTnode>(identifier, type);
      return std::move(variable);
    }
  } else {
    throw LogError("Syntax Error: Expected identifier after variable/function type");
  }
}

// varfun_decl ::= "(" params ")" block
//                | ";"
static std::unique_ptr<FunctionDefASTnode> ParseVarFunDecl(std::string type, std::string identifier) {
  if (CurTok.type == LPAR) {
    CurTok = getNextToken(); // eat (
    std::vector<std::unique_ptr<FunctionParamASTnode>> parameters;
    parameters = ParseParams();
    if (CurTok.type == RPAR) {
      CurTok = getNextToken(); // eat )
      std::unique_ptr<BlockASTnode> block;
      block = ParseBlock();
      std::unique_ptr<FunctionPrototypeASTnode> func_proto = std::make_unique<FunctionPrototypeASTnode>(identifier, type, std::move(parameters));
      std::unique_ptr<FunctionDefASTnode> func = std::make_unique<FunctionDefASTnode>(std::move(func_proto), std::move(block));
      return std::move(func);
    } else {
      throw LogError("Syntax Error: Expected ) after parameters");
    }
  } else if (CurTok.type == SC) {
    CurTok = getNextToken(); // eat ;
    // This is a variable definition, so return nullptr
    return nullptr;
  } else {
    // This is invalid syntax, return nullptr
    throw LogError("Expected ( after function identifier or ; after variable identifier");
  }
}

// block ::= "{" local_decls stmt_list "}"
static std::unique_ptr<BlockASTnode> ParseBlock() {
  std::vector<std::unique_ptr<VariableDeclarationASTnode>> declarations;
  std::vector<std::unique_ptr<ASTnode>> statements;
  if (CurTok.type == LBRA) {
    CurTok = getNextToken(); // eat {
  } else {
    throw LogError("Syntax Error: Expected { at start of block");
  }
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
    declarations = ParseLocalDecls();
  }
  statements = ParseStmtList();
  if (CurTok.type == RBRA) {
      CurTok = getNextToken(); // eat }
  } else {
    throw LogError("Syntax Error: Expected } at end of block");
  }
  std::unique_ptr<BlockASTnode> block = std::make_unique<BlockASTnode>(std::move(declarations), std::move(statements));
  return std::move(block);
}

// local_decls ::= local_decl local_decls_prime
static std::vector<std::unique_ptr<VariableDeclarationASTnode>> ParseLocalDecls() {
  std::vector<std::unique_ptr<VariableDeclarationASTnode>> declarations;
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
    std::unique_ptr<VariableDeclarationASTnode> local_decl;
    std::vector<std::unique_ptr<VariableDeclarationASTnode>> declarations;
    local_decl = ParseLocalDecl();
    declarations.push_back(std::move(local_decl));
    declarations = ParseLocalDeclsPrime(std::move(declarations));
    return std::move(declarations);
  } else {
    throw LogError("Syntax Error: Expected variable type int, float, or bool");
  }
}

// local_decls_prime ::= local_decl local_decls_prime
//                    | epsilon
static std::vector<std::unique_ptr<VariableDeclarationASTnode>> ParseLocalDeclsPrime(std::vector<std::unique_ptr<VariableDeclarationASTnode>> declarations) {
  int stmt_token_array[] = {LPAR, MINUS, NOT, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, 
  SC, WHILE, IF, RETURN, LBRA};
  int size = 12;
  if (CurTok.type == INT_TOK || CurTok.type == FLOAT_TOK || CurTok.type == BOOL_TOK) {
    std::unique_ptr<VariableDeclarationASTnode> local_decl;
    local_decl = ParseLocalDecl();
    declarations.push_back(std::move(local_decl));
    declarations = ParseLocalDeclsPrime(std::move(declarations));
    return std::move(declarations);
  } else if (CheckMembership(stmt_token_array, size, CurTok.type)) {
    return std::move(declarations); // CurTok is in FOLLOW set of local_decls_prime, so valid
  } else {
    throw LogError("Syntax Error: Expected variable type int, float, or bool for declaration or (, -, !, identifier, int literal, float literal, bool literal, ;, while, if, return, { for statement");
  }
}

// local_decl ::= var_type IDENT ";"
static std::unique_ptr<VariableDeclarationASTnode> ParseLocalDecl() {
  std::string var_type;
  std::unique_ptr<VariableDeclarationASTnode> empty_ptr;
  var_type = ParseVarType();
  std::string var_name;
  if (CurTok.type == IDENT) {
    var_name = CurTok.lexeme;
    std::unique_ptr<VariableASTnode> variable = std::make_unique<VariableASTnode>(var_name);
    CurTok = getNextToken(); // eat IDENT
  } else {
    throw LogError("Syntax Error: Expected identifier after variable type");
  }
  if (CurTok.type == SC) {
    CurTok = getNextToken(); // eat ;
    std::unique_ptr<VariableDeclarationASTnode> return_ptr = std::make_unique<VariableDeclarationASTnode>(var_name, var_type);
    return std::move(return_ptr);
  } else {
    throw LogError("Syntax Error: Expected ; after identifier");
  }
}

// stmt_list ::= stmt stmt_list_prime
static std::vector<std::unique_ptr<ASTnode>>  ParseStmtList() {
  std::unique_ptr<ASTnode> stmt;
  std::vector<std::unique_ptr<ASTnode>> stmt_list;
  stmt = ParseStmt();
  stmt_list.push_back(std::move(stmt));
  stmt_list = ParseStmtListPrime(std::move(stmt_list));
  return std::move(stmt_list);
}

// stmt_list_prime ::= stmt stmt_list_prime
//                    | epsilon
static std::vector<std::unique_ptr<ASTnode>> ParseStmtListPrime(std::vector<std::unique_ptr<ASTnode>> stmt_list) {
  int stmt_token_array[] = {LPAR, MINUS, NOT, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, 
  SC, WHILE, IF, RETURN, LBRA};
  int size = 12;
  if (CheckMembership(stmt_token_array, size, CurTok.type)) {
    std::unique_ptr<ASTnode> stmt;
    stmt = ParseStmt();
    stmt_list.push_back(std::move(stmt));
    stmt_list = ParseStmtListPrime(std::move(stmt_list));
    return std::move(stmt_list);
  } else if (CurTok.type == RBRA) {
    return std::move(stmt_list); // CurTok is in FOLLOW set of stmt_list_prime, so valid
  } else {
    throw LogError("Syntax Error: Expected (, -, !, identifier, int literal, float literal, bool literal, ;, while, if, return, { for statement or } for end of statements");
  }
}

// stmt ::= expr_stmt 
//    |  block 
//    |  if_stmt 
//    |  while_stmt 
//    |  return_stmt
static std::unique_ptr<ASTnode> ParseStmt() {
  int expr_stmt_array[] = {LPAR, MINUS, NOT, IDENT, INT_LIT, FLOAT_LIT, 
  BOOL_LIT, SC};
  int size = 8;
  if (CheckMembership(expr_stmt_array, size, CurTok.type)) {
    std::unique_ptr<ASTnode> ptr;
    ptr = ParseExprStmt();
    return std::move(ptr);
  } else if (CurTok.type == LBRA) {
    std::unique_ptr<BlockASTnode> ptr;
    ptr = ParseBlock();
    return std::move(ptr);
  } else if (CurTok.type == IF) {
    std::unique_ptr<IfExprASTnode> ptr;
    ptr = ParseIf();
    return std::move(ptr);
  } else if (CurTok.type == WHILE) {
    std::unique_ptr<WhileExprASTnode> ptr;
    ptr = ParseWhile();
    return std::move(ptr);
  } else if (CurTok.type == RETURN) {
    std::unique_ptr<ReturnExprASTnode> ptr;
    ptr = ParseReturn();
    return std::move(ptr);
  } else {
    throw LogError("Syntax Error: Expected (, -, !, identifier, int literal, float literal, bool literal, ; for expression statement, { for block statement, if for if statement, while for while statement, or return for return statement");
  }
}

// expr_stmt ::= expr ";" 
//            |  ";"
static std::unique_ptr<ASTnode> ParseExprStmt() {
  int expr_array[] = {LPAR, MINUS, NOT, IDENT, INT_LIT, FLOAT_LIT, 
  BOOL_LIT};
  int size = 7;
  if (CheckMembership(expr_array, size, CurTok.type)) {
    std::unique_ptr<ASTnode> ptr = ParseExpr();
    if (CurTok.type == SC) {
      CurTok = getNextToken(); // eat ;
      return std::move(ptr);
    } else {
      throw LogError("Syntax Error: Expected ; after expression");
    }
  } else if (CurTok.type == SC) {
    CurTok = getNextToken(); // eat ;
    return nullptr;
  } else {
    throw LogError("Syntax Error: Expected ;");
  }
}

// expr ::= IDENT "=" expr
//     | rval
static std::unique_ptr<ASTnode> ParseExpr() {
  int rval_array[] = {LPAR, MINUS, NOT, INT_LIT, FLOAT_LIT, 
  BOOL_LIT};
  int size = 6;
  std::unique_ptr<ASTnode> ptr;
  if (CurTok.type == IDENT) {
    TOKEN last_token = CurTok;
    std::string variable_name = CurTok.lexeme;
    CurTok = getNextToken();
    if (CurTok.type == ASSIGN) {
      std::unique_ptr<VariableASTnode> variable = std::make_unique<VariableASTnode>(variable_name);
      CurTok = getNextToken(); // eat =
      ptr = ParseExpr();
      std::unique_ptr<VariableAssignmentASTnode> assignment;
      assignment = std::make_unique<VariableAssignmentASTnode>(std::move(variable), std::move(ptr));
      return std::move(assignment);
    } else {
      putBackToken(CurTok);
      CurTok = last_token;
      ptr = ParseRval();
      return std::move(ptr);
    }
  } else if (CheckMembership(rval_array, size, CurTok.type)) {
    ptr = ParseRval();
    return std::move(ptr);
  } else {
    throw LogError("Syntax Error: Expected assignment of form 'identifier =' or expression");
  }
}

// if_stmt ::= "if" "(" expr ")" block else_stmt
static std::unique_ptr<IfExprASTnode> ParseIf() {
  CurTok = getNextToken(); // eat if
  if (CurTok.type == LPAR) {
    CurTok = getNextToken(); // eat (
  } else {
    throw LogError("Syntax Error: Expected ( after if");
  }
  int expr_array[] = {LPAR, MINUS, NOT, IDENT, INT_LIT, FLOAT_LIT, 
  BOOL_LIT};
  int size = 7;
  std::unique_ptr<ASTnode> condition;
  if (CheckMembership(expr_array, size, CurTok.type)) {
    condition = ParseExpr();
  } else {
    throw LogError("Syntax Error: Expected expression after (");
  }
  if (CurTok.type == RPAR) {
    CurTok = getNextToken(); // eat )
  } else {
    throw LogError("Syntax Error: Expected ) after expression");
  }
  std::unique_ptr<BlockASTnode> block;
  block = ParseBlock();
  std::unique_ptr<BlockASTnode> else_expression;
  else_expression = ParseElse();
  std::unique_ptr<IfExprASTnode> if_expression = std::make_unique<IfExprASTnode>(std::move(condition), std::move(block), std::move(else_expression));
  return std::move(if_expression);
}

// else_stmt  ::= "else" block
//             |  epsilon
static std::unique_ptr<BlockASTnode> ParseElse() {
  // This is an array of possible tokens that can follow an if statement
  // either another statement, or } (RBRA) which is what follows stmt_list
  // If "else" is not seen, then one of these tokens must be
  int stmt_token_array[] = {RBRA, LPAR, MINUS, NOT, IDENT, INT_LIT, FLOAT_LIT, BOOL_LIT, 
  SC, WHILE, IF, RETURN, LBRA};
  int size = 13;
  std::unique_ptr<BlockASTnode> else_expression;
  if (CurTok.type == ELSE) {
    CurTok = getNextToken(); // eat else
    else_expression = ParseBlock();
  } else if (CheckMembership(stmt_token_array, size, CurTok.type)) {
    return nullptr; // CurTok in FOLLOW set of else_stmt
  } else {
    throw LogError("Syntax Error: Expected else for else statement or }, (, !, identifier, int literal, float literal, bool literal, ;, while, if, return, { for statement");
  }
  return std::move(else_expression);
}

// while_stmt ::= "while" "(" expr ")" stmt
static std::unique_ptr<WhileExprASTnode> ParseWhile() {
  CurTok = getNextToken(); // eat while
  if (CurTok.type == LPAR) {
    CurTok = getNextToken(); // eat (
  } else {
    throw LogError("Syntax Error: Expected ( after while");
  }
  int expr_array[] = {LPAR, MINUS, NOT, IDENT, INT_LIT, FLOAT_LIT, 
  BOOL_LIT};
  int size = 7;
  std::unique_ptr<ASTnode> condition;
  if (CheckMembership(expr_array, size, CurTok.type)) {
    condition = ParseExpr();
  } else {
    throw LogError("Syntax Error: Expected expression after (");
  }
  if (CurTok.type == RPAR) {
    CurTok = getNextToken(); // eat )
    std::unique_ptr<ASTnode> statement;
    statement = ParseStmt();
    std::unique_ptr<WhileExprASTnode> return_ptr = std::make_unique<WhileExprASTnode>(std::move(condition), std::move(statement));
    return std::move(return_ptr);
  } else {
    throw LogError("Syntax Error: Expected ) after expression");
  }
}

// return_stmt ::= "return" ";" 
//             |  "return" expr ";" 
static std::unique_ptr<ReturnExprASTnode> ParseReturn() {
  CurTok = getNextToken(); // eat return
  if (CurTok.type == SC) {
    CurTok = getNextToken(); // eat ;
    // No expression to return so return nullptr
    std::unique_ptr<ReturnExprASTnode> return_ptr = std::make_unique<ReturnExprASTnode>(nullptr);
    return std::move(return_ptr);
  } else {
    std::unique_ptr<ASTnode> return_expr;
    return_expr = ParseExpr();
    if (CurTok.type == SC) {
      CurTok = getNextToken(); // eat ;
      std::unique_ptr<ReturnExprASTnode> return_ptr = std::make_unique<ReturnExprASTnode>(std::move(return_expr));
      return std::move(return_ptr);
    } else {
      throw LogError("Syntax Error: Expected ; after expression or after return");
    }
  }
}

// rval ::= rval_one rval_prime
static std::unique_ptr<ASTnode> ParseRval() {
  std::unique_ptr<ASTnode> ptr;
  ptr = ParseRvalOne();
  if (CurTok.type == OR) {
    std::unique_ptr<BinaryASTnode> return_ptr;
    return_ptr = ParseRvalPrime(std::move(ptr));
    return std::move(return_ptr);
  } else {
    return std::move(ptr);
  }
}

// rval_prime ::= "||" rval_one rval_prime | epsilon
static std::unique_ptr<BinaryASTnode> ParseRvalPrime(std::unique_ptr<ASTnode> lhs) {
  if (CurTok.type == OR) {
    std::unique_ptr<BinaryASTnode> return_ptr;
    std::string op = CurTok.lexeme;
    CurTok = getNextToken(); // eat ||
    std::unique_ptr<ASTnode> rhs;
    rhs = ParseRvalOne();
    std::unique_ptr<BinaryASTnode> ptr = std::make_unique<BinaryASTnode>(op, std::move(lhs), std::move(rhs));
    if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
      return std::move(ptr);
    } else if (CurTok.type == OR) {
      return_ptr = ParseRvalPrime(std::move(ptr));
      return std::move(return_ptr);
    } else {
      throw LogError("Syntax Error: Expected expression or ;, ), or , after expression");
    }
  } else if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
    // No more OR operators follow, return nullptr 
    return nullptr; // CurTok in FOLLOW set of rval_one_prime
  } else {
    // No more OR operators follow, and next symbol not in FOLLOW set - error reported
    throw LogError("Syntax Error: Expected ;, ), or , after || expression");
  }
}

// rval_one ::= rval_two rval_one_prime
static std::unique_ptr<ASTnode> ParseRvalOne() {
  std::unique_ptr<ASTnode> ptr;
  ptr = ParseRvalTwo();
  if (CurTok.type == AND) {
    std::unique_ptr<BinaryASTnode> return_ptr;
    return_ptr = ParseRvalOnePrime(std::move(ptr));
    return std::move(return_ptr);
  } else {
    return std::move(ptr);
  }
}

// rval_one_prime ::= "&&" rval_two rval_one_prime | epsilon
static std::unique_ptr<BinaryASTnode> ParseRvalOnePrime(std::unique_ptr<ASTnode> lhs) {
  if (CurTok.type == AND) {
    std::unique_ptr<BinaryASTnode> return_ptr;
    std::string op = CurTok.lexeme;
    CurTok = getNextToken(); // eat &&
    std::unique_ptr<ASTnode> rhs;
    rhs = ParseRvalTwo();
    std::unique_ptr<BinaryASTnode> ptr = std::make_unique<BinaryASTnode>(op, std::move(lhs), std::move(rhs));
    if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
      return std::move(ptr);
    } else if (CurTok.type == AND) {
      return_ptr = ParseRvalOnePrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == OR) {
      return_ptr = ParseRvalPrime(std::move(ptr));
      return std::move(return_ptr);
    } else {
      throw LogError("Syntax Error: Expected expression or ;, ), or , after expression");
    }
  } else if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
    // No more AND operators follow, return nullptr 
    return nullptr; // CurTok in FOLLOW set of rval_one_prime
  } else {
    // No more AND operators follow, and next symbol not in FOLLOW set - error reported and nullptr returned
    throw LogError("Syntax Error: Expected ;, ), or , after && expression");
  }
}

// rval_two ::= rval_three rval_two_prime
static std::unique_ptr<ASTnode> ParseRvalTwo() {
  std::unique_ptr<ASTnode> ptr;
  ptr = ParseRvalThree();
  if (CurTok.type == EQ || CurTok.type == NE) {
    std::unique_ptr<BinaryASTnode> return_ptr;
    return_ptr = ParseRvalTwoPrime(std::move(ptr));
    return std::move(return_ptr);
  } else {
    return std::move(ptr);
  }
}

// rval_two_prime ::= "==" rval_three rval_two_prime | "!=" rval_three rval_two_prime | epsilon
static std::unique_ptr<BinaryASTnode> ParseRvalTwoPrime(std::unique_ptr<ASTnode> lhs) {
  if (CurTok.type == EQ || CurTok.type == NE) {
    std::unique_ptr<BinaryASTnode> return_ptr;
    std::string op = CurTok.lexeme;
    CurTok = getNextToken(); // eat == or !=
    std::unique_ptr<ASTnode> rhs;
    rhs = ParseRvalThree();
    std::unique_ptr<BinaryASTnode> ptr = std::make_unique<BinaryASTnode>(op, std::move(lhs), std::move(rhs));
    if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
      return std::move(ptr);
    } else if (CurTok.type == EQ || CurTok.type == NE) {
      return_ptr = ParseRvalTwoPrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == AND) {
      return_ptr = ParseRvalOnePrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == OR) {
      return_ptr = ParseRvalPrime(std::move(ptr));
      return std::move(return_ptr);
    } else {
      throw LogError("Syntax Error: Expected expression or ;, ), or , after expression");
    }
  } else if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
    // No more equality operators follow, return nullptr 
    return nullptr; // CurTok in FOLLOW set of rval_two_prime
  } else {
    // No more equality operators follow, and next symbol not in FOLLOW set - error reported and nullptr returned
    throw LogError("Syntax Error: Expected ;, ), or , after == or != expression");
  }
}

// rval_three ::= rval_four rval_three_prime
static std::unique_ptr<ASTnode> ParseRvalThree() {
  std::unique_ptr<ASTnode> ptr;
  ptr = ParseRvalFour();
  if (CurTok.type == LE || CurTok.type == LT || CurTok.type == GE || CurTok.type == GT) {
    std::unique_ptr<BinaryASTnode> return_ptr;
    return_ptr = ParseRvalThreePrime(std::move(ptr));
    return std::move(return_ptr);
  } else {
    return std::move(ptr);
  }
}

// rval_three_prime ::= "<=" rval_four rval_three_prime | "<" rval_four rval_three_prime | ">=" rval_four rval_three_prime | ">" rval_four rval_three_prime | epsilon
static std::unique_ptr<BinaryASTnode> ParseRvalThreePrime(std::unique_ptr<ASTnode> lhs) {
  if (CurTok.type == LE || CurTok.type == LT || CurTok.type == GE || CurTok.type == GT) {
    std::unique_ptr<BinaryASTnode> return_ptr;
    std::string op = CurTok.lexeme;
    CurTok = getNextToken(); // eat <=, <, >, or >=
    std::unique_ptr<ASTnode> rhs;
    rhs = ParseRvalFour();
    std::unique_ptr<BinaryASTnode> ptr = std::make_unique<BinaryASTnode>(op, std::move(lhs), std::move(rhs));
    if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
      return std::move(ptr);
    } else if (CurTok.type == LE || CurTok.type == LT || CurTok.type == GE || CurTok.type == GT){
      return_ptr = ParseRvalThreePrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == EQ || CurTok.type == NE) {
      return_ptr = ParseRvalTwoPrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == AND) {
      return_ptr = ParseRvalOnePrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == OR) {
      return_ptr = ParseRvalPrime(std::move(ptr));
      return std::move(return_ptr);
    } else {
      throw LogError("Syntax Error: Expected expression or ;, ), or , after expression");
    }
  } else if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
    // No more comparison operators follow, return nullptr 
    return nullptr; // CurTok in FOLLOW set of rval_three_prime
  } else {
    // No more comparison operators follow, and next symbol not in FOLLOW set - error reported and nullptr returned
    throw LogError("Syntax Error: Expected ;, ), or , after <=, <, >, or >= expression");
  }
}

// rval_four ::= rval_five rval_four_prime
static std::unique_ptr<ASTnode> ParseRvalFour() {
  std::unique_ptr<ASTnode> ptr;
  ptr = ParseRvalFive();
  if (CurTok.type == PLUS || CurTok.type == MINUS) {
    std::unique_ptr<BinaryASTnode> return_ptr;
    return_ptr = ParseRvalFourPrime(std::move(ptr));
    return std::move(return_ptr);
  } else {
    return std::move(ptr);
  }
}

// rval_four_prime ::= "+" rval_five rval_four_prime | "-" rval_five rval_four_prime | epsilon
static std::unique_ptr<BinaryASTnode> ParseRvalFourPrime(std::unique_ptr<ASTnode> lhs) {
  if (CurTok.type == PLUS || CurTok.type == MINUS) {
    std::unique_ptr<BinaryASTnode> return_ptr;
    std::string op = CurTok.lexeme;
    CurTok = getNextToken(); // eat + or -
    std::unique_ptr<ASTnode> rhs;
    rhs = ParseRvalFive();
    std::unique_ptr<BinaryASTnode> ptr = std::make_unique<BinaryASTnode>(op, std::move(lhs), std::move(rhs));
    if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
      return std::move(ptr);
    } else if (CurTok.type == PLUS || CurTok.type == MINUS) { // IF ITS PLUS, PASS PTR AS LHS TO RVALFOURPRIME
      return_ptr = ParseRvalFourPrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == LE || CurTok.type == LT || CurTok.type == GE || CurTok.type == GT){
      return_ptr = ParseRvalThreePrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == EQ || CurTok.type == NE) {
      return_ptr = ParseRvalTwoPrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == AND) {
      return_ptr = ParseRvalOnePrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == OR) {
      return_ptr = ParseRvalPrime(std::move(ptr));
      return std::move(return_ptr);
    } else {
      throw LogError("Syntax Error: Expected expression or ;, ), or , after expression");
    }
  } else if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
    // No more addition/subtraction follows, return nullptr 
    return nullptr; // CurTok in FOLLOW set of rval_four_prime
  } else {
    // No more addition/subtraction follows, and next symbol not in FOLLOW set - error reported and nullptr returned
    throw LogError("Syntax Error: Expected ;, ), or , after + or - expression");
  }
}

// rval_five ::= rval_six rval_five_prime 
static std::unique_ptr<ASTnode> ParseRvalFive() {
  std::unique_ptr<ASTnode> ptr;
  ptr = ParseRvalSix();
  if (CurTok.type == ASTERIX || CurTok.type == DIV || CurTok.type == MOD) {
    std::unique_ptr<BinaryASTnode> return_ptr;
    return_ptr = ParseRvalFivePrime(std::move(ptr));
    return std::move(return_ptr);
  } else {
    return std::move(ptr);
  }
}

// rval_five_prime ::= "*" rval_six rval_five_prime | "/" rval_six rval_five_prime | "%" rval_six rval_five_prime | epsilon
static std::unique_ptr<BinaryASTnode> ParseRvalFivePrime(std::unique_ptr<ASTnode> lhs) {
  if (CurTok.type == ASTERIX || CurTok.type == DIV || CurTok.type == MOD) {
    std::unique_ptr<BinaryASTnode> return_ptr; 
    std::string op = CurTok.lexeme;
    CurTok = getNextToken(); // eat *, /, or %
    std::unique_ptr<ASTnode> rhs;
    rhs = ParseRvalSix();
    std::unique_ptr<BinaryASTnode> ptr = std::make_unique<BinaryASTnode>(op, std::move(lhs), std::move(rhs));
    if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
      return std::move(ptr);
    } else if (CurTok.type == ASTERIX || CurTok.type == DIV || CurTok.type == MOD) {
      return_ptr = ParseRvalFivePrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == PLUS || CurTok.type == MINUS) { // IF ITS PLUS, PASS PTR AS LHS TO RVALFOURPRIME
      return_ptr = ParseRvalFourPrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == LE || CurTok.type == LT || CurTok.type == GE || CurTok.type == GT){
      return_ptr = ParseRvalThreePrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == EQ || CurTok.type == NE) {
      return_ptr = ParseRvalTwoPrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == AND) {
      return_ptr = ParseRvalOnePrime(std::move(ptr));
      return std::move(return_ptr);
    } else if (CurTok.type == OR) {
      return_ptr = ParseRvalPrime(std::move(ptr));
      return std::move(return_ptr);
    } else {
      throw LogError("Syntax Error: Expected expression or ;, ), or , after expression");
    }
  } else if (CurTok.type == SC || CurTok.type == RPAR || CurTok.type == COMMA) {
    // No more multipliation/division/modulo follows, return nullptr 
    return nullptr; // CurTok in FOLLOW set of rval_five_prime
  } else {
    // No more multipliation/division/modulo follows, and next symbol not in FOLLOW set - error reported and nullptr returned
    throw LogError("Syntax Error: Expected ;, ), or , after *, /, or % expression");
  }
}

// rval_six ::= "-" rval_seven | "!" rval_seven | rval_seven
static std::unique_ptr<ASTnode> ParseRvalSix() {
  std::unique_ptr<ASTnode> ptr;
  if (CurTok.type == MINUS || CurTok.type == NOT) {
    std::string op = CurTok.lexeme;
    CurTok = getNextToken(); // eat - or !
    if (CurTok.type == MINUS || CurTok.type == NOT) {
      ptr = ParseRvalSix();
    } else {
      ptr = ParseRvalSeven();
    }
    std::unique_ptr<UnaryASTnode> return_ptr = std::make_unique<UnaryASTnode>(op, std::move(ptr));
    return std::move(return_ptr);
  } else {
    ptr = ParseRvalSeven();
    return std::move(ptr);
  }
}

// rval_seven ::= "(" expr ")" | rval_eight
static std::unique_ptr<ASTnode> ParseRvalSeven() {
  std::unique_ptr<ASTnode> ptr;
  if (CurTok.type == LPAR) {
    CurTok = getNextToken(); // eat (
    ptr = ParseExpr();
    if (CurTok.type == RPAR) {
      CurTok = getNextToken(); // eat )
      return std::move(ptr);
    } else {
      throw LogError("Syntax Error: Expected ) after expression");
    }
  } else {
    ptr = ParseRvalEight();
    return std::move(ptr);
  }
}

// rval_eight ::= IDENT | IDENT "(" args ")" | rval_nine
static std::unique_ptr<ASTnode> ParseRvalEight() {
  if (CurTok.type == IDENT) {
    std::string identifier_name = CurTok.lexeme;
    CurTok = getNextToken(); // eat IDENT
    if (CurTok.type == LPAR) {
      CurTok = getNextToken(); // eat (
      std::vector<std::unique_ptr<ASTnode>> args;
      args = ParseArgs();
      if (CurTok.type == RPAR) {
        CurTok = getNextToken(); // eat )
        std::unique_ptr<CallASTnode> ptr = std::make_unique<CallASTnode>(identifier_name, std::move(args));
        return std::move(ptr);
      } else {
        throw LogError("Syntax Error: Expected ) after arguments");
      }
    } else {
      // No ( so this is a simple variable call, create variable AST node and return pointer
      std::unique_ptr<VariableASTnode> ptr = std::make_unique<VariableASTnode>(identifier_name);
      return std::move(ptr);
    }
  } else {
    std::unique_ptr<ASTnode> ptr;
    ptr = ParseRvalNine();
    return std::move(ptr);
  }
}

// rval_nine ::= INT_LIT | FLOAT_LIT | BOOL_LIT
static std::unique_ptr<ASTnode> ParseRvalNine() {
  // Capture literal value
  switch (CurTok.type) {
    case INT_LIT: {
      // Read token lexeme and convert to integer, then create an integer literal AST node, return pointer to node
      int token_value = stoi(CurTok.lexeme);
      std::unique_ptr<IntASTnode> ptr = std::make_unique<IntASTnode>(token_value);
      CurTok = getNextToken(); // eat integer
      return std::move(ptr);
    }
    case FLOAT_LIT: {
      // Read token lexeme and convert to float, then create a float literal AST node, return pointer to node
      float token_value = stof(CurTok.lexeme);
      std::unique_ptr<FloatASTnode> ptr = std::make_unique<FloatASTnode>(token_value);
      CurTok = getNextToken(); // eat float
      return std::move(ptr);
    }
    case BOOL_LIT: {
      // Read token lexeme and convert to boolean, then create a boolean literal AST node, return pointer to node
      std::string token_value = CurTok.lexeme;
      bool op;
      std::istringstream(token_value) >> std::boolalpha >> op;
      std::unique_ptr<BoolASTnode> ptr = std::make_unique<BoolASTnode>(op);
      CurTok = getNextToken(); // eat boolean
      return std::move(ptr);
    }
    default: {
      // No matches for any rval, syntax error - expression is invalid
      throw LogError("Syntax Error: Expected paranthesis, binary operation, unary operation, identifier, integer literal, float literal, or bool literal for expression"); // ! No matches for any rvals if it has reached here
    }
  }
}

// args ::= arg_list 
//     |  epsilon

static std::vector<std::unique_ptr<ASTnode>> ParseArgs() {
  int arg_array[] = {LPAR, MINUS, NOT, IDENT, INT_LIT, FLOAT_LIT, 
  BOOL_LIT};
  int size = 7;
  std::vector<std::unique_ptr<ASTnode>> args;
  if (CheckMembership(arg_array, size, CurTok.type)) {
    args = ParseArgList();
    return std::move(args);
  } else if (CurTok.type == RPAR) {
    // CurTok in FOLLOW set of args, no arguments but valid syntax - return empty array of pointers to arguments
    return std::move(args);
  } else {
    // Invalid syntax, next token not in follow set
    throw LogError("Syntax Error: Expected (, -, !, identifier, integer literal, float literal, or bool literal for argument or ) for end of arguments"); // no matches for argument, and no closing paranthesis
  }
}

// arg_list ::= expr arg_list_prime
static std::vector<std::unique_ptr<ASTnode>> ParseArgList() {
  std::unique_ptr<ASTnode> arg;
  std::vector<std::unique_ptr<ASTnode>> args;
  arg = ParseExpr();
  args.push_back(std::move(arg));
  args = ParseArgListPrime(std::move(args));
  return std::move(args);
}

// arg_list_prime ::= "," expr arg_list_prime
//                 | epsilon
static std::vector<std::unique_ptr<ASTnode>> ParseArgListPrime(std::vector<std::unique_ptr<ASTnode>> args) {
  if (CurTok.type == COMMA) {
    CurTok = getNextToken(); // eat ,
    std::unique_ptr<ASTnode> arg;
    arg = ParseExpr();
    args.push_back(std::move(arg));
    args = ParseArgListPrime(std::move(args));
    return std::move(args);
  } else if (CurTok.type == RPAR) {
    return std::move(args); // CurTok in FOLLOW set of arg_list_prime
  } else {
    // Invalid argument list, return nullptr
    throw LogError("Syntax Error: Expected , for next argument or ) for end of arguments");
  }
}

//===----------------------------------------------------------------------===//
// Code Generation
//===----------------------------------------------------------------------===//

static LLVMContext TheContext;
static IRBuilder<> Builder(TheContext);
static std::unique_ptr<Module> TheModule;

// NamedValuesArray stores each different local scope that is created during
// runtime. Each codegen function takes block_index as a parameter, this block_index
// is used to access the correct scope stored in NamedValuesArray
// New scopes are created at function definitions, if statements, else statements, and while
// statements
static std::vector<std::map<std::string, AllocaInst *>> NamedValuesArray;
static std::map<std::string, Value *> GlobalValues;

Value *LogErrorV(std::string Str) {
  LogError(Str);
  return nullptr;
}

// Taken from Finnbar's tutorial lecture - thank you :)
static AllocaInst *CreateEntryBlockAlloca(Function *TheFunction, const std::string &VarName, const std::string &VarType) { // make work for other types
  if (VarType == "int") {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Type::getInt32Ty(TheContext), 0, VarName.c_str());
  } else if (VarType == "float") {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Type::getFloatTy(TheContext), 0, VarName.c_str());
  } else if (VarType == "bool") {
    IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
    return TmpB.CreateAlloca(Type::getInt1Ty(TheContext), 0, VarName.c_str());
  }
}

Value *IntASTnode::codegen(int block_index) {
  return ConstantInt::get(TheContext, APInt(32, Val, true));
}

Value *FloatASTnode::codegen(int block_index) {
  return ConstantFP::get(TheContext, APFloat(Val));
}

Value *BoolASTnode::codegen(int block_index) {
  return ConstantInt::get(TheContext, APInt(1, Val, false));
}

Value *VariableASTnode::codegen(int block_index) {
  // Defines temporary block index
  int try_index = block_index;
  // Uses this block index to try find named variable in current block
  // If it cannot find the named variable, try_index is decremented and the previous block is
  // checked - if no block has the named variable, it is assumed to be a global variable
  AllocaInst *A = NamedValuesArray[try_index][Name];
  while (A == nullptr && (try_index != 0)) {
    try_index = try_index - 1;
    A = NamedValuesArray[try_index][Name];
  }
  if (A == nullptr) {
    GlobalVariable *g = TheModule->getNamedGlobal(Name);
    // Now check if this global variable exists
    if (g != nullptr) {
      Type *var_type;
      if (g->getInitializer()->getType()->isFloatTy()) {
        // This is a float global variable
        var_type = Type::getFloatTy(TheContext);
      } else if (g->getInitializer()->getType()->isIntegerTy(32)) {
        // This is an integer global variable
        var_type = Type::getInt32Ty(TheContext);
      } else if (g->getInitializer()->getType()->isIntegerTy(1)) {
        // This is a bool global variable
        var_type = Type::getInt1Ty(TheContext);
      }
      // :Load and return this global variable
      return Builder.CreateLoad(var_type, g, Name.c_str()); 

    } else {
      // Global variable does not exist, and variable also does not exist in any block
      // This is an undefined variable
      throw LogErrorV("Semantic Error: Undefined variable name " + Name);
    }
  }
  // If a local variable was found in some block, it is loaded and returned
  return Builder.CreateLoad(A->getAllocatedType(), A, Name.c_str());
}

Value *VariableDeclarationASTnode::codegen(int block_index) {
  if (Builder.GetInsertBlock() == nullptr) {
    // This must be a global variable declaration since there is no insert block
    llvm::Type *var_type;
    // Find type of variable and create as LLVM Type
    if (Type == "int") {
      var_type = Type::getInt32Ty(TheContext);
    } else if (Type == "float") {
      var_type = Type::getFloatTy(TheContext);
    } else if (Type == "bool") {
      var_type = Type::getInt1Ty(TheContext);
    }
    // Create global variable and set alignment
    GlobalVariable *g = new GlobalVariable(*(TheModule.get()), var_type, false, GlobalValue::CommonLinkage, Constant::getNullValue(var_type), Name);
    g->setAlignment(MaybeAlign(4));
    GlobalValues[Name] = g;
  } else {
    // This is a local variable since there is an insert block
    Function *TheFunction = Builder.GetInsertBlock()->getParent();
    // Allocate memory for this variable and assign to current block
    AllocaInst *Variable = CreateEntryBlockAlloca(TheFunction, Name, Type);
    NamedValuesArray[block_index][Name] = Variable;
  }
  return nullptr;
}

Value *VariableAssignmentASTnode::codegen(int block_index) {
    VariableASTnode *target_variable = dynamic_cast<VariableASTnode *>(Variable.get());
    if (target_variable == nullptr) {
      throw LogErrorV("Semantic Error: LHS of assignment '=' must be a variable");
    }
    // Generate IR for assigned value
    Value *assigned_val = Val->codegen(block_index);
    if (assigned_val == nullptr) {
      return nullptr;
    }

    // Defines temporary block index
    int try_index = block_index;
    // Uses this block index to try find named variable in current block
    // If it cannot find the named variable, try_index is decremented and the previous block is
    // checked - if no block has the named variable, it is assumed to be a global variable
    AllocaInst *Variable = NamedValuesArray[try_index][target_variable->getName()];
    while (Variable == nullptr && (try_index != 0)) {
      try_index = try_index - 1;
      Variable = NamedValuesArray[try_index][target_variable->getName()];
    }

    if (Variable == nullptr) {
      // Check if this global variable exists
      GlobalVariable *g = TheModule->getNamedGlobal(target_variable->getName());
      if (g != nullptr) {
        Builder.CreateStore(assigned_val, g);
        return assigned_val;
      }
      // Global variable does not exist, and variable also does not exist in any block
      // This is an undefined variable
      throw LogErrorV("Semantic Error: Undefined variable name " + target_variable->getName());
    }
    // Check if declared type of variable is the same as the type of attempted value to assign
    if (assigned_val->getType() != Variable->getAllocatedType()){
      // Types not same, e.g. trying to assign float to a variable declared as int
      // Change type of assigned value to be equal to declared type of variable
      // Then warn of this implicit type conversion
      if (Variable->getAllocatedType()->isIntegerTy()) {
        assigned_val = Builder.CreateFPToSI(assigned_val, Variable->getAllocatedType());
        LogErrorV("Warning: implicit type conversion from float to int while assigning value to variable");
      } else if (Variable->getAllocatedType()->isFloatTy()) {
        assigned_val = Builder.CreateSIToFP(assigned_val, Variable->getAllocatedType());
        LogErrorV("Warning: implicit type conversion from int to float while assigning value to variable");
      }
    }
    Builder.CreateStore(assigned_val, Variable);
    return assigned_val;
}

Value *BinaryASTnode::codegen(int block_index) {
  // Generate IR code for LHS and RHS
  Value *L = LHS->codegen(block_index);
  Value *R = RHS->codegen(block_index);

  if (L == nullptr || R == nullptr) {
    return nullptr;
  }
  // Get types of LHS and RHS
  Type *L_type = L->getType();
  Type *R_type = R->getType();

  // If either operand is of type float, perform float operations (implicit conversion of int to float and warning)
  if (L_type->isFloatTy() || R_type->isFloatTy()) {
    if (!L_type->isFloatTy()) {
      // Convert to float and give warning
      L = Builder.CreateSIToFP(L, Type::getFloatTy(TheContext), "convtmp");
      LogErrorV("Warning: implicit type conversion from int to float while performing binary operation");
    } else if (!R_type->isFloatTy()) {
      // Convert to float and give warning
      R = Builder.CreateSIToFP(R, Type::getFloatTy(TheContext), "convtmp");
      LogErrorV("Warning: implicit type conversion from int to float while performing binary operation");
    }
    // Match the correct binary operator and build corresponding IR
    if (Op == "+") {
      return Builder.CreateFAdd(L, R, "addftmp");
    } else if (Op == "-") {
      return Builder.CreateFSub(L, R, "subftmp");
    } else if (Op == "*") {
      return Builder.CreateFMul(L, R, "mulftmp");
    } else if (Op == "/") {
      return Builder.CreateFDiv(L, R, "divftmp");
    } else if (Op == "%") {
      return Builder.CreateFRem(L, R, "remftmp");
    } else if (Op == "<") {
      return Builder.CreateFCmpULT(L, R, "sltftmp");
    } else if (Op == "<=") {
      return Builder.CreateFCmpULE(L, R, "sleftmp");
    } else if (Op == ">=") {
      return Builder.CreateFCmpUGE(L, R, "sgeftmp");
    } else if (Op == ">") {
      return Builder.CreateFCmpUGT(L, R, "sgtftmp");
    } else if (Op == "==") {
      return Builder.CreateFCmpUEQ(L, R, "eqftmp");
    } else if (Op == "!=") {
      return Builder.CreateFCmpUNE(L, R, "neftmp");
    } else if (Op == "&&") {
      return Builder.CreateAnd(L, R, "andftmp");
    } else if (Op == "||") {
      return Builder.CreateOr(L, R, "orftmp");
    } else {
      // Currently this would never be reached as the parser would catch any undefined
      // binary operators, but this allows for an extension allowing user-defined binary operators
      // similar to the Kaleidoscope tutorial
      throw LogErrorV("Syntax Error: Invalid binary operator"); 
    }
  } else {
    // Match the correct binary operator and build corresponding IR
    if (Op == "+") {
      return Builder.CreateAdd(L, R, "addtmp");
    } else if (Op == "-") {
      return Builder.CreateSub(L, R, "subtmp");
    } else if (Op == "*") {
      return Builder.CreateMul(L, R, "multmp");
    } else if (Op == "/") {
      return Builder.CreateSDiv(L, R, "divtmp");
    } else if (Op == "%") {
      return Builder.CreateURem(L, R, "remtmp");
    } else if (Op == "<") {
      return Builder.CreateICmpSLT(L, R, "slttmp");
    } else if (Op == "<=") {
      return Builder.CreateICmpSLE(L, R, "sletmp");
    } else if (Op == ">=") {
      return Builder.CreateICmpSGE(L, R, "sgetmp");
    } else if (Op == ">") {
      return Builder.CreateICmpSGT(L, R, "sgttmp");
    } else if (Op == "==") {
      return Builder.CreateICmpEQ(L, R, "eqtmp");
    } else if (Op == "!=") {
      return Builder.CreateICmpNE(L, R, "netmp");
    } else if (Op == "&&") {
      return Builder.CreateAnd(L, R, "andtmp");
    } else if (Op == "||") {
      return Builder.CreateOr(L, R, "ortmp");
    } else {
      // Currently this would never be reached as the parser would catch any undefined
      // binary operators, but this allows for an extension allowing user-defined binary operators
      // similar to the Kaleidoscope tutorial
      throw LogErrorV("Syntax Error: Invalid binary operator");
    }
  }
}

Value *UnaryASTnode::codegen(int block_index) {
  // Generate IR code for operand and pass in block_index for correct scope
  Value *Operand = Val->codegen(block_index);
  Type *Operand_type = Operand->getType();
  // Check type of operand and make corresponding calls to generate IR code
  if (Operand_type->isFloatTy()) {
    if (Op == "-") {
      return Builder.CreateFNeg(Operand, "negftmp");
    } else if (Op == "!") {
      return Builder.CreateNot(Operand, "nottmp");
    } else {
      // Currently this would never be reached as the parser would catch any undefined
      // unary operators, but this allows for an extension allowing user-defined unary operators
      // similar to the Kaleidoscope tutorial
      // The same applies to any further calls in this function
      throw LogErrorV("Syntax Error: Invalid unary operator");
    }
  } else if (Operand_type->isIntegerTy(32)) {
    if (Op == "-") {
      return Builder.CreateNeg(Operand, "negtmp");
    } else if (Op == "!") {
      return Builder.CreateNot(Operand, "nottmp");
    } else {
      throw LogErrorV("Syntax Error: Invalid unary operator");
    }
  } else if (Operand_type->isIntegerTy(1)) {
    if (Op == "-") {
      return Builder.CreateNeg(Operand, "negftmp");
    } else if (Op == "!") {
      return Builder.CreateNot(Operand, "nottmp");
    } else {
      LogErrorV("Invalid unary operator");
      return nullptr;
    }
  } else {
    throw LogErrorV("Syntax Error: Invalid unary operand type");
  }
}

Value *BlockASTnode::codegen(int block_index) {
  // Loop through all declarations and statements and generate IR code
  // Pass in block_index to each to allow further function calls to have access
  // to the correct scope
  Value *RetVal;
  for (auto &Decl : Declarations) {
    RetVal = Decl->codegen(block_index);
  }
  for (auto &Stmt : Statements) {
    RetVal = Stmt->codegen(block_index);
  }
  return RetVal;
}

Value *FunctionParamASTnode::codegen(int block_index) {
  // This does not need to generate any IR
  return nullptr;
}

Function *FunctionPrototypeASTnode::codegen(int block_index) {
  // Get types of all parameters
  std::vector<llvm::Type *> params;
  for (auto &Arg : Args) {
    if (Arg->getType() == "int") {
      params.push_back(Type::getInt32Ty(TheContext));
    } else if (Arg->getType() == "float") {
      params.push_back(Type::getFloatTy(TheContext));
    } else if (Arg->getType() == "bool") {
      params.push_back(Type::getInt1Ty(TheContext));
    }
  }

  // Get return type of function
  FunctionType *FT;
  if (Type == "float") {
    FT = FunctionType::get(Type::getFloatTy(TheContext), params, false);
  } else if (Type == "bool") {
    FT = FunctionType::get(Type::getInt1Ty(TheContext), params, false);
  } else if (Type == "int") {
    FT = FunctionType::get(Type::getInt32Ty(TheContext), params, false);
  } else if (Type == "void") {
    FT = FunctionType::get(Type::getVoidTy(TheContext), params, false);
  }
  // Construct function given its FunctionType
  Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());
  // Set argument names
  unsigned Idx = 0;
  for (auto &Arg : F->args()) {
    Arg.setName(Args[Idx]->getName());
    Idx++;
  }

  return F;
}

Function *ExternASTnode::codegen(int block_index) {
  // Exact same as PrototypeASTnode
  std::vector<llvm::Type *> params;
  for (auto &Arg : Params) {
    if (Arg->getType() == "int") {
      params.push_back(Type::getInt32Ty(TheContext));
    } else if (Arg->getType() == "float") {
      params.push_back(Type::getFloatTy(TheContext));
    } else if (Arg->getType() == "bool") {
      params.push_back(Type::getInt1Ty(TheContext));
    }
  }

  FunctionType *FT;
  if (Type == "float") {
    FT = FunctionType::get(Type::getFloatTy(TheContext), params, false);
  } else if (Type == "bool") {
    FT = FunctionType::get(Type::getInt1Ty(TheContext), params, false);
  } else if (Type == "int") {
    FT = FunctionType::get(Type::getInt32Ty(TheContext), params, false);
  } else if (Type == "void") {
    FT = FunctionType::get(Type::getVoidTy(TheContext), params, false);
  }

  Function *F = Function::Create(FT, Function::ExternalLinkage, Name, TheModule.get());

  unsigned Idx = 0;
  for (auto &Arg : F->args()) {
    Arg.setName(Params[Idx]->getName());
    Idx++;
  }

  return F;
}

Function *FunctionDefASTnode::codegen(int block_index) {
  // block_index stores the index at which this blocks local variables are stored in NamedValuesArray
  // set to 0 when a new function is being defined as all variables are out of scope - except global variables
  // which are accessed directly with TheModule->getNamedGlobal()
  block_index = 0;
  // Create new local variable table and clear array of local variable tables for new function
  std::map<std::string, AllocaInst *> NamedValues;
  NamedValuesArray.clear();
  // Add new local variable table to array
  NamedValuesArray.push_back(NamedValues);
  Function *TheFunction = TheModule->getFunction(Prototype->getName());
  if (TheFunction == nullptr) {
    // Generate IR code for function prototype
    TheFunction = Prototype->codegen(block_index);
  }
  if (TheFunction == nullptr) {
    return nullptr;
  }
  // Create entry for function definition
  BasicBlock *BB = BasicBlock::Create(TheContext, "entry", TheFunction);
  Builder.SetInsertPoint(BB);

  // Get passed in arguments to function and create alloca blocks for each
  int count = 0;
  for (auto &Arg: TheFunction->args()) {
    std::string arg_type = Prototype->getArgType(count);
    AllocaInst *Alloca = CreateEntryBlockAlloca(TheFunction, Arg.getName().data(), arg_type);
    Builder.CreateStore(&Arg, Alloca);
    NamedValuesArray[block_index][std::string(Arg.getName())] = Alloca;
    count = count + 1;
  }
  // Check if there is a return value, if so create a non-void return, otherwise create
  // a void return
  Value *RetVal = Body->codegen(block_index);
  if (RetVal) {
    if (Prototype->getType() == "void") {
      Builder.CreateRetVoid();
    } else {
      Builder.CreateRet(RetVal);
    }
  }

  verifyFunction(*TheFunction);
  return TheFunction;
}

Value *CallASTnode::codegen(int block_index) {
  // Look up function name in the global module table
  Function *CalleeF = TheModule->getFunction(CallFunc);
  if (CalleeF == nullptr) {
    throw LogErrorV("Semantic Error: Undefined function referenced " + CallFunc);
  }
  // Check if number of arguments is correct
  if (CalleeF->arg_size() != Args.size()){
    throw LogErrorV("Semantic Error: Incorrect number of arguments passed into function, expected " + 
    std::to_string(CalleeF->arg_size()) + " but got " + std::to_string(Args.size()));
  }
  // Generate IR code for each function argument and append to func_args array
  std::vector<Value *> func_args;
  for (unsigned i = 0, e = Args.size(); i != e; i++) {
    func_args.push_back(Args[i]->codegen(block_index));
    if (func_args.back() == nullptr) {
      return nullptr;
    }
  }
  return Builder.CreateCall(CalleeF, func_args, "calltmp");
}

Value *IfExprASTnode::codegen(int block_index) {
  block_index = block_index + 1;
  std::map<std::string, AllocaInst *> NamedValues;
  NamedValuesArray.push_back(NamedValues);
  Function *TheFunction = Builder.GetInsertBlock()->getParent();
  BasicBlock *true_ = BasicBlock::Create(TheContext, "if then", TheFunction);
  BasicBlock *else_ = BasicBlock::Create(TheContext, "else then");
  BasicBlock *end_ = BasicBlock::Create(TheContext, "end");
  Value *cond = Cond->codegen(block_index);
  Value *comp_int = ConstantInt::get(TheContext, APInt(1,0,false));
  Value *comp = Builder.CreateICmpNE(cond, comp_int, "ifcond");
  if (Else == nullptr) {
    Builder.CreateCondBr(comp, true_, end_);
    Builder.SetInsertPoint(true_);
    Then->codegen(block_index);
    TheFunction->getBasicBlockList().push_back(end_);
    Builder.CreateBr(end_);
    Builder.SetInsertPoint(end_);
  } else {
    Builder.CreateCondBr(comp, true_, else_);
    Builder.SetInsertPoint(true_);
    Then->codegen(block_index);
    Builder.CreateBr(end_);
    TheFunction->getBasicBlockList().push_back(else_);
    Builder.SetInsertPoint(else_);
    Else->codegen(block_index);
    TheFunction->getBasicBlockList().push_back(end_);
    Builder.CreateBr(end_);
    Builder.SetInsertPoint(end_);
  }
  // Erase the local variables table for this if statement and all tables formed as part of this if block
  // e.g. erase any blocks formed by nested if statements or while loops
  NamedValuesArray.erase(NamedValuesArray.begin()+block_index, NamedValuesArray.end());
  return nullptr;
}

Value *WhileExprASTnode::codegen(int block_index) {
  LogErrorV("WhileExprASTnode");
  block_index = block_index + 1;
  std::map<std::string, AllocaInst *> NamedValues;
  NamedValuesArray.push_back(NamedValues);
  Function *TheFunction = Builder.GetInsertBlock()->getParent();
  BasicBlock *while_header = BasicBlock::Create(TheContext, "header", TheFunction);
  BasicBlock *body_ = BasicBlock::Create(TheContext, "body");
  BasicBlock *end_ = BasicBlock::Create(TheContext, "end");
  Builder.CreateBr(while_header);
  Builder.SetInsertPoint(while_header);
  Value *cond = Cond->codegen(block_index);
  Value *comp_int = ConstantInt::get(TheContext, APInt(1,0,false));
  Value *comp = Builder.CreateICmpNE(cond, comp_int, "whilecond");
  Builder.CreateCondBr(comp, body_, end_);
  TheFunction->getBasicBlockList().push_back(body_);
  Builder.SetInsertPoint(body_);
  Then->codegen(block_index);
  Builder.CreateBr(while_header);
  TheFunction->getBasicBlockList().push_back(end_);
  Builder.CreateBr(end_);
  Builder.SetInsertPoint(end_);
  return nullptr;
}

Value *ReturnExprASTnode::codegen(int block_index) {
  if (ReturnValue == nullptr) {
    // Return expression does not return a value, only pass control flow
    return Builder.CreateRetVoid();
  } else {
    Value *return_val = ReturnValue->codegen(block_index);
    Type *return_val_type = return_val->getType();
    if (return_val_type->isIntegerTy()) {
      LogErrorV("INTEGER TYPE");
    }


    Builder.CreateRet(return_val);
    return nullptr;
  }
}

Value *RootASTnode::codegen(int block_index) {
  Value *RetVal;
  Function *RetFunc;
  for (auto &Ext : Ext_List) {
    RetFunc = Ext->codegen(block_index);
  }
  for (auto &Decl : Decl_List) {
    RetVal = Decl->codegen(block_index);
  }
  return RetFunc;
}

//===----------------------------------------------------------------------===//
// AST Printer
//===----------------------------------------------------------------------===//

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &os,
                                     const ASTnode &ast) {
  std::string ident_level = "";
  os << ast.to_string(ident_level);
  return os;
}

//===----------------------------------------------------------------------===//
// Main driver code.
//===----------------------------------------------------------------------===//

int main(int argc, char **argv) {
  if (argc == 2) {
    pFile = fopen(argv[1], "r");
    if (pFile == NULL)
      perror("Error opening file");
  } else {
    std::cout << "Usage: ./code InputFile\n";
    return 1;
  }

  // initialize line number and column numbers to zero
  lineNo = 1;
  columnNo = 1;

  // Make the module, which holds all the code.
  TheModule = std::make_unique<Module>("mini-c", TheContext);
  // Disables llvm creating opaque pointers in IR code
  TheContext.setOpaquePointers(false);
  // Run the parser now.

  std::unique_ptr<RootASTnode> program;
  program = parser();
  std::string ident_level = "";
  llvm::outs() << program->to_string(ident_level) << "\n";
  fprintf(stderr, "Parsing Finished\n");
  int block_index = 0;
  program->codegen(block_index);

  //********************* Start printing final IR **************************
  // Print out all of the generated code into a file called output.ll
  auto Filename = "output.ll";
  std::error_code EC;
  raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

  if (EC) {
    errs() << "Could not open file: " << EC.message();
    return 1;
  }
  TheModule->print(errs(), nullptr); // print IR to terminal
  TheModule->print(dest, nullptr);
  //********************* End printing final IR ****************************

  fclose(pFile); // close the file that contains the code that was parsed
  return 0;
}
