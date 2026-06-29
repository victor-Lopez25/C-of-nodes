#define VL_BUILD_IMPLEMENTATION
//#include "vl_build.h" (included in common.h)
#include "common.h"

#include "node.c"
#include "parser.c"
#include "graph_vis.c"

#define GLOBAL_ARENA_SIZE 32*1024*1024

bool DISABLE_PEEPHOLE_OPTIMIZATIONS = false;

// https://github.com/SeaOfNodes/Simple/blob/main/chapter01/README.md
// NOTE: Types prefixed with SON_ mean SeaOfNodes_

bool InitCompilerContext(CompilerContext *ctx)
{
  Assert(ctx);

  ArenaInit(ctx->arena, GLOBAL_ARENA_SIZE, malloc(GLOBAL_ARENA_SIZE));
  ExpArrayInit(ctx->nodes, SON_NODE_XAR_CHUNK_SHIFT);
  // NOTE: node idx 0 is reserved
  ctx->nodeUniqueID = 1;
  ctx->startNode = SON_AllocFunctionStart(ctx);

  ctx->sentinelNode.nodeID = 0;
  ctx->sentinelNode.kind = SON_Node_Unassigned;

  return true;
}

void ClearCompilerContext(CompilerContext *ctx)
{
  Assert(ctx);

  for(size_t i = 0; i < ctx->nodes.hdr.n; i++) {
    SON_Node *n = ExpArrayGet(&ctx->nodes, i);
    if(n->inputs.items) DaFree(n->inputs);
    if(n->outputs.items) DaFree(n->outputs);
  }
  // This gets allocated from ctx->nodes so probably not a good idea to use it after clearing it
  ctx->nodeFreeList = 0;

  ctx->nodes.hdr.n = 0;
  // NOTE: node idx 0 is reserved
  ctx->nodeUniqueID = 1;
  ArenaClear(ctx->arena, false);

  ctx->startNode = SON_AllocFunctionStart(ctx);
}

void PrintUsage(char *program)
{
  printf("Usage: %s <file>\n", program);
}

bool CompileRawData(CompilerContext *ctx, view data)
{
  printf("SOURCE_START_HERE\n"VIEW_FMT"\nSOURCE_END_HERE\n", VIEW_ARG(data));

  ctx->originalSource = data;
  ctx->currentSource = data;

  SON_Node *ret = Parse_CurrentContext(ctx);
  SON_Node *expr = SON_FunctionReturnExpr(ret);
  VL_Log(VL_INFO, "return value: %d", (int)expr->value.as.integer);

  return true;
}

bool CompileFile(CompilerContext *ctx, char *filename)
{
  size_t fileSize;
  char *data = ReadEntireFile(ctx->arena, filename, &fileSize);

  view filedata = ViewFromParts(data, fileSize);
  if(CompileRawData(ctx, filedata)) {
    VL_Log(VL_INFO, "Compiled %s successfully", filename);
  } else {
    VL_Log(VL_ERROR, "Could not compile %s", filename);
    return false;
  }
  return true;
}

void TestParseGrammar(CompilerContext *ctx)
{
  ClearCompilerContext(ctx);
  DISABLE_PEEPHOLE_OPTIMIZATIONS = true;

  view data = VIEW("return 1+2*3+-5;");
  ctx->originalSource = data;
  ctx->currentSource = data;

  SON_Node *ret = Parse_CurrentContext(ctx);

  string_builder sb = {0};
  SON_PrintToBuilder(&sb, ret);

  VL_Log(VL_INFO, "output: "VIEW_FMT, VIEW_ARG(sb));
  Assert(ViewEq(VIEW("return (#1 + ((#2 * #3) + (-#5)));"), ViewFromParts(sb.items, sb.count)));

  SbFree(sb);

  sb = Graph_GenerateDotOutput(ctx);
  //VL_Log(VL_INFO, "graph: \n"
  //       VIEW_FMT, VIEW_ARG(sb));
  SbFree(sb);

  DISABLE_PEEPHOLE_OPTIMIZATIONS = false;
}

void TestAddPeephole(CompilerContext *ctx)
{
  ClearCompilerContext(ctx);

  view data = VIEW("return 1+2;");
  ctx->originalSource = data;
  ctx->currentSource = data;

  SON_Node *ret = Parse_CurrentContext(ctx);
  string_builder sb = {0};
  SON_PrintToBuilder(&sb, ret);
  VL_Log(VL_INFO, "output: "VIEW_FMT, VIEW_ARG(sb));
  Assert(ViewEq(VIEW("return #3;"), ViewFromParts(sb.items, sb.count)));
  SbFree(sb);
}

void DoTests(CompilerContext *ctx)
{
  TestParseGrammar(ctx);
  TestAddPeephole(ctx);
}

int main(int argc, char **argv)
{
  if(argc < 2) {
    PrintUsage(*argv);
    return 1;
  }

  view arg = ViewFromCstr(argv[1]);
  if(ViewEq(arg, VIEW("-h")) || 
      ViewEq(arg, VIEW("--help")) || 
      ViewEq(arg, VIEW("/?")))
  {
    PrintUsage(*argv);
    return 1;
  }

  memory_arena globalArena;
  CompilerContext ctx = {
    .arena = &globalArena,
  };
  InitCompilerContext(&ctx);

#if 0
  char *file = argv[1];
  bool ok = CompileFile(&ctx, file);
  (void)ok;
#else
  DoTests(&ctx);
#endif

  return 0;
}
