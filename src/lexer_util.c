#include "common.h"

/* reasoning: I do not want to fuck up utf8 so I'm not using 0xFF
 * An alternative would be to use a 32bit value instead, could be better idk
 */
#define LEX_SENTINEL_CHAR 0x7F

static inline bool LEX_IsEOF(CompilerContext *ctx)
{
  return ctx->currentSource.count == 0;
}

static inline char LEX_Peek(CompilerContext *ctx)
{
  char result = LEX_IsEOF(ctx) ? LEX_SENTINEL_CHAR : ctx->currentSource.items[0];
  return result;
}

static inline char LEX_NextChar(CompilerContext *ctx)
{
  char ch = LEX_Peek(ctx);
  ctx->currentSource.count--;
  ctx->currentSource.items++;
  return ch;
}

static inline bool LEX_IsWhiteSpace(CompilerContext *ctx)
{
  bool result = is_space(LEX_Peek(ctx));
  return result;
}

static inline bool LEX_IsNumber(char ch)
{
  bool result = isdigit(ch);
  return result;
}

static inline bool LEX_IsIdStart(char ch)
{
  bool result = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
  return result;
}

static inline bool LEX_IsIdLetter(char ch)
{
  bool result = LEX_IsNumber(ch) || LEX_IsIdStart(ch);
  return result;
}

static inline bool LEX_IsOperator(char ch)
{
  bool result = ViewFindCharacter(VIEW("=;[]<>(){}+-/*!"), ch, 0);
  return result;
}

static inline void LEX_SkipWhiteSpace(CompilerContext *ctx)
{
  ctx->currentSource = ViewTrimLeft(ctx->currentSource);
}

// NOTE: Does not check if the start of the id is correct (not a number)
static inline view LEX_ParseId(CompilerContext *ctx)
{
  size_t i = 0;
  view start = ctx->currentSource;
  for(;i < ctx->currentSource.count && LEX_IsIdLetter(LEX_NextChar(ctx));) i += 1;

  ctx->currentSource.items = start.items + i;
  ctx->currentSource.count = start.count - i;
  return ViewFromParts(start.items, i);
}

static inline bool LEX_Match(CompilerContext *ctx, view word)
{
  LEX_SkipWhiteSpace(ctx);
  return ViewChopStartsWith(&ctx->currentSource, word);
}

/* Match exclusive, if there a match and the word doesn't go on, returns true */
static inline bool LEX_MatchEx(CompilerContext *ctx, view word)
{
  if(!LEX_Match(ctx, word)) return false;
  if(!LEX_IsIdLetter(LEX_Peek(ctx))) return true;
  ctx->currentSource.count += word.count;
  ctx->currentSource.items -= word.count;
  return false;
}

static inline view LEX_MatchId(CompilerContext *ctx)
{
  LEX_SkipWhiteSpace(ctx);
  return LEX_IsIdStart(LEX_Peek(ctx)) ? LEX_ParseId(ctx) : VIEW_STATIC("");
}

static inline view LEX_ParseNumberString(CompilerContext *ctx)
{
  view before = ctx->currentSource;

  int64_t val;
  if(!ViewParseS64(ctx->currentSource, &val, &ctx->currentSource)) return VIEW("");

  view result = ViewFromParts(before.items, before.count - ctx->currentSource.count);
  return result;
}

static inline bool LEX_ParseNumber(CompilerContext *ctx, int64_t *val)
{
  return ViewParseS64(ctx->currentSource, val, &ctx->currentSource);
}

static inline view LEX_ParseOperator(CompilerContext *ctx)
{
  // NOTE: All operators are currently one character wide
  view result = ViewFromParts(ctx->currentSource.items, 1);
  LEX_NextChar(ctx);
  return result;
}

view LEX_GetAnyNextToken(CompilerContext *ctx)
{
  if(LEX_IsEOF(ctx)) return VIEW("");

  char ch = LEX_Peek(ctx);
  if(LEX_IsIdStart(ch)) return LEX_ParseId(ctx);
  if(LEX_IsNumber(ch)) return LEX_ParseNumberString(ctx);
  if(LEX_IsOperator(ch)) return LEX_ParseOperator(ctx);
  return ViewFromParts(ctx->currentSource.items, 1);
}

