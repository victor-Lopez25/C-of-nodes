#include "common.h"
#include "node.h"

#include "lexer_util.c"

// TODO: An error is marked if there was one and stop parsing at a reasonable moment

static void Parse_SyntaxError(CompilerContext *ctx, view v)
{
  view next = LEX_GetAnyNextToken(ctx);
  VL_Log(VL_ERROR, "Syntax Error, expected "VIEW_FMT". Got '"VIEW_FMT"'",
         VIEW_ARG(v), VIEW_ARG(next));
}

static void Parse_SyntaxError_Expected(CompilerContext *ctx, view expected, view got)
{
  (void)ctx;
  VL_Log(VL_ERROR, "Syntax Error, expected "VIEW_FMT". Got '"VIEW_FMT"'",
         VIEW_ARG(expected), VIEW_ARG(got));
}

static SON_Node *Parse_RequireNode(CompilerContext *ctx, SON_Node *node, view v)
{
  if(LEX_Match(ctx, v)) return node;
  //Parse_SyntaxError(ctx, v);
  VL_Log(VL_ERROR, "Syntax Error: Expected '"VIEW_FMT"' after %s node", VIEW_ARG(v), SON_NodeKindToString(node));
  return &ctx->sentinelNode;
}

static bool Parse_Require(CompilerContext *ctx, view v, view before)
{
  if(LEX_Match(ctx, v)) {
    return true;
  } else {
    VL_Log(VL_ERROR, "Syntax Error: Expected '"VIEW_FMT"' after "VIEW_FMT,
      VIEW_ARG(v), VIEW_ARG(before));
    return false;
  }
}

static view Parse_RequireId(CompilerContext *ctx)
{
  view id = LEX_MatchId(ctx);
  if((id.count > 0) && !stbds_hmgetp_null(ctx->keywords, id)) {
    return id;
  }
  Parse_SyntaxError_Expected(ctx, VIEW_STATIC("an identifier"), id);
  return VIEW_STATIC("");
}

static SON_Node *Parse_IntegerLiteral(CompilerContext *ctx)
{
  SON_Value val = {
    .kind = SON_Value_Integer,
    .isConstant = true,
  };
  if(!LEX_ParseNumber(ctx, &val.as.integer)) {
    return &ctx->sentinelNode;
  }
  return SON_Peephole(ctx, SON_AllocConstant(ctx, val));
}

static SON_Node *Parse_Expression(CompilerContext *ctx);

static SON_Node *Parse_Primary(CompilerContext *ctx)
{
  LEX_SkipWhiteSpace(ctx);
  if(LEX_IsNumber(LEX_Peek(ctx))) return Parse_IntegerLiteral(ctx);
  if(LEX_Match(ctx, VIEW_STATIC("("))) return Parse_RequireNode(ctx, Parse_Expression(ctx), VIEW_STATIC(")"));
  view name = LEX_MatchId(ctx);
  if(name.count == 0) {
    Parse_SyntaxError(ctx, VIEW_STATIC("an identifier or expression"));
    return &ctx->sentinelNode;
  }
  SON_Node *node = Scope_Lookup(ctx->scope, name);
  if(!node) {
    VL_Log(VL_ERROR, "Undefined identifier '"VIEW_FMT"'", VIEW_ARG(name));
    return &ctx->sentinelNode;
  }
  return node;
}

static SON_Node *Parse_Unary(CompilerContext *ctx)
{
  if(LEX_Match(ctx, VIEW_STATIC("-"))) {
    return SON_Peephole(ctx, SON_AllocMinus(ctx, Parse_Unary(ctx)));
  }
  return Parse_Primary(ctx);
}

/* Parse a multiplicative expression
 * 
 * multiplicative: unary (('*' | '/') expr)*
 */
static SON_Node *Parse_Multiplication(CompilerContext *ctx)
{
  SON_Node *lhs = Parse_Unary(ctx);
  if(LEX_Match(ctx, VIEW_STATIC("*"))) return SON_Peephole(ctx, SON_AllocMul(ctx, lhs, Parse_Multiplication(ctx)));
  if(LEX_Match(ctx, VIEW_STATIC("/"))) return SON_Peephole(ctx, SON_AllocDiv(ctx, lhs, Parse_Multiplication(ctx)));
  return lhs;
}

static SON_Node *Parse_Binary(CompilerContext *ctx)
{
  SON_Node *lhs = Parse_Multiplication(ctx);
  if(LEX_Match(ctx, VIEW_STATIC("+"))) return SON_Peephole(ctx, SON_AllocAdd(ctx, lhs, Parse_Binary(ctx)));
  if(LEX_Match(ctx, VIEW_STATIC("-"))) return SON_Peephole(ctx, SON_AllocSub(ctx, lhs, Parse_Binary(ctx)));
  return lhs;
}

static SON_Node *Parse_Expression(CompilerContext *ctx)
{
  return Parse_Binary(ctx);
}

static SON_Node *Parse_Return(CompilerContext *ctx)
{
  SON_Node *expr = Parse_RequireNode(ctx, Parse_Expression(ctx), VIEW_STATIC(";"));
  return SON_Peephole(ctx, SON_AllocReturn(ctx, ctx->startNode, expr));
}

/* name is assumed to be a valid id */
static SON_Node *Parse_ExpressionStatement(CompilerContext *ctx, view name)
{
  if(!Parse_Require(ctx, VIEW_STATIC("="), VIEW_STATIC("an identifier"))) {
    return &ctx->sentinelNode;
  }

  SON_Node *expr = Parse_RequireNode(ctx, Parse_Expression(ctx), VIEW_STATIC(";"));
  if(!Scope_Update(ctx, ctx->scope, name, expr)) {
    VL_Log(VL_ERROR, "Undefined identifier '"VIEW_FMT"'", VIEW_ARG(name));
    return &ctx->sentinelNode;
  }
  return expr;
}

/* Parses a definition and declaration
 * Possibilities:
 *  - <name> ':' <type>;
 *  - <name> ':' '=' <expr>;
 *  - <name> ':' <type> = <expr>;
 * If <name> '=' <expr> is encountered, Parse_ExpressionStatement(ctx, name) is called
 */
static SON_Node *Parse_Definition(CompilerContext *ctx)
{
  view name = Parse_RequireId(ctx);
  if(name.count == 0) {
    // NOTE: This could also be an expression statement but only if I have stuff like:
    //       *<name> = x;
    return &ctx->sentinelNode;
  }

  SON_ValueKind kind = SON_Value_Unassigned;

  if(LEX_Match(ctx, VIEW_STATIC(":"))) {
    if(LEX_MatchEx(ctx, VIEW_STATIC("int"))) {
      kind = SON_Value_Integer;
    }
  } else {
    return Parse_ExpressionStatement(ctx, name);
  }

  SON_Node *expr;
  if(LEX_Match(ctx, VIEW_STATIC("="))) {
    // TODO: Type check here...
    expr = Parse_RequireNode(ctx, Parse_Expression(ctx), VIEW_STATIC(";"));
  } else if(kind == SON_Value_Unassigned) {
    Parse_SyntaxError(ctx, VIEW_STATIC("a type or a value assignment"));
    return &ctx->sentinelNode;
  } else {
    expr = Parse_RequireNode(ctx, ctx->defaultValues[kind], VIEW_STATIC(";"));
  }

  if(!Scope_Define(ctx, ctx->scope, name, expr)) {
    VL_Log(VL_ERROR, "Error: Redifining variable '"VIEW_FMT"'", VIEW_ARG(name));
  }
  return expr;
}

static SON_Node *Parse_Block(CompilerContext *ctx);

static SON_Node *Parse_Statement(CompilerContext *ctx)
{
  if(LEX_MatchEx(ctx, VIEW_STATIC("return"))) return Parse_Return(ctx);
  else if(LEX_Match(ctx, VIEW_STATIC("{"))) return Parse_RequireNode(ctx, Parse_Block(ctx), VIEW_STATIC("}"));
  // empty statement
  else if(LEX_Match(ctx, VIEW_STATIC(";"))) return 0;
  else return Parse_Definition(ctx);
}

/* parses a block: '{' statements '}' */
static SON_Node *Parse_Block(CompilerContext *ctx)
{
  Scope_Push(ctx, ctx->scope);
  SON_Node *lastStmt = 0;
  while((LEX_Peek(ctx) != '}') && !LEX_IsEOF(ctx)) {
    SON_Node *stmt = Parse_Statement(ctx);
    if(stmt == &ctx->sentinelNode) {
      VL_Log(VL_ERROR, "Aborting compilation...");
      Scope_Pop(ctx, ctx->scope);
      return stmt;
    }
    if(stmt) {
      lastStmt = stmt;
    }
  }
  Scope_Pop(ctx, ctx->scope);
  return lastStmt;
}

/* Returns the SON_Node_FunctionReturn of the current context */
SON_Node *Parse_CurrentContext(CompilerContext *ctx)
{
  SON_Node *ret = Parse_Block(ctx);
  if(!LEX_IsEOF(ctx)) {
    VL_Log(VL_ERROR, "Syntax error, unexpected "VIEW_FMT, VIEW_ARG(LEX_GetAnyNextToken(ctx)));
  }
  return ret;
}

