#include "common.h"

#include "lexer_util.c"

// TODO: An error is marked if there was one and stop parsing at a reasonable moment

static void Parse_SyntaxError(CompilerContext *ctx, view v)
{
  view next = LEX_GetAnyNextToken(ctx);
  VL_Log(VL_ERROR, "Syntax Error, expected "VIEW_FMT". Got '"VIEW_FMT"'",
         VIEW_ARG(v), VIEW_ARG(next));
}

static SON_Node *Parse_RequireNode(CompilerContext *ctx, SON_Node *node, view v)
{
  if(LEX_Match(ctx, v)) return node;
  //Parse_SyntaxError(ctx, v);
  VL_Log(VL_ERROR, "Expected '"VIEW_FMT"' after %s node", VIEW_ARG(v), SON_NodeKindToString(node));
  return &ctx->sentinelNode;
}

//static void Parse_Require(CompilerContext *ctx, view v)
//{
//  Parse_RequireNode(ctx, &ctx->sentinelNode, v);
//}

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
  Parse_SyntaxError(ctx, VIEW_STATIC("integer literal"));
  return &ctx->sentinelNode;
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
 * multiplicative: unary (('*' | '/') unary)*
 */
static SON_Node *Parse_Multiplication(CompilerContext *ctx)
{
  SON_Node *lhs = Parse_Unary(ctx);
  // NOTE: I'm not sure if Parse_Multiplication should call itself or Parse_Binary
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
  return SON_AllocReturn(ctx, ctx->startNode, expr);
}

static SON_Node *Parse_Statement(CompilerContext *ctx)
{
  if(LEX_MatchEx(ctx, VIEW_STATIC("return"))) return Parse_Return(ctx);
  Parse_SyntaxError(ctx, VIEW("keyword 'return' as there are no others"));
  return 0;
}

/* Returns the SON_Node_FunctionReturn of the current context */
SON_Node *Parse_CurrentContext(CompilerContext *ctx)
{
  SON_Node *ret = Parse_Statement(ctx);
  if(!LEX_IsEOF(ctx)) {
    VL_Log(VL_ERROR, "Syntax error, unexpected "VIEW_FMT, VIEW_ARG(LEX_GetAnyNextToken(ctx)));
  }
  return ret;
}

