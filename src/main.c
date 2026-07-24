#define VL_BUILD_IMPLEMENTATION
#define STB_DS_IMPLEMENTATION
#define STB_SLICEHM_IMPLEMENTATION
//#include "vl_build.h" (included in common.h)
#include "common.h"
#include "time.h" /* time() */

#include "node.c"
#include "parser.c"
#include "graph_vis.c"

#define GLOBAL_ARENA_SIZE 32*1024*1024

bool DISABLE_PEEPHOLE_OPTIMIZATIONS = false;
bool ENABLE_GRAPH_STEPS = false;
char *GRAPH_STEP_FILE_FMT = U64_Fmt;
uint64_t GRAPH_STEP_IDX = 0;

// NOTE: Types prefixed with SON_ mean SeaOfNodes_

bool InitCompilerContext(CompilerContext *ctx)
{
  Assert(ctx);

  stbds_rand_seed(time(0));

  ArenaInit(ctx->arena, GLOBAL_ARENA_SIZE, malloc(GLOBAL_ARENA_SIZE));
  ExpArrayInit(ctx->nodes, SON_NODE_XAR_CHUNK_SHIFT);
  // NOTE: node idx 0 is reserved
  ctx->nodeUniqueID = 1;
  ctx->startNode = SON_AllocFunctionStart(ctx);

  ctx->sentinelNode.nodeID = 0;
  ctx->sentinelNode.kind = SON_Node_Unassigned;

  // TODO: Do I want these to be normal constant nodes?
  ctx->defaultValues[SON_Value_Integer] = SON_AllocNext_Impl(ctx, SON_Node_Constant, 0, 0);
  ctx->defaultValues[SON_Value_Integer]->value.kind = SON_Value_Integer;
  ctx->defaultValues[SON_Value_Integer]->value.as.integer = 0;

  ctx->defaultValues[SON_Value_Floating] = SON_AllocNext_Impl(ctx, SON_Node_Constant, 0, 0);
  ctx->defaultValues[SON_Value_Floating]->value.kind = SON_Value_Floating;
  ctx->defaultValues[SON_Value_Floating]->value.as.floating = 0.0;

  ctx->defaultValues[SON_Value_String] = SON_AllocNext_Impl(ctx, SON_Node_Constant, 0, 0);
  ctx->defaultValues[SON_Value_String]->value.kind = SON_Value_String;
  ctx->defaultValues[SON_Value_String]->value.as.string = VIEW_STATIC("");

  // NOTE: I don't care about the value in this hashmap (for now...)
  slicehm_put(ctx->keywords, VIEW_STATIC("int"), 1);
  slicehm_put(ctx->keywords, VIEW_STATIC("return"), 1);

  return true;
}

void DeleteCompilerContext(CompilerContext *ctx)
{
  Assert(ctx);

  for(size_t i = 0; i < ctx->nodes.hdr.n; i++) {
    SON_Node *n = ExpArrayGet(&ctx->nodes, i);
    if(n->inputs.items) {
      DaFree(n->inputs);
      n->inputs.items = 0;
      n->inputs.capacity = 0;
      n->inputs.count = 0;
    }
    if(n->outputs.items) {
      DaFree(n->outputs);
      n->outputs.items = 0;
      n->outputs.capacity = 0;
      n->outputs.count = 0;
    }
  }

  // Clear this as it tries to allocate memory from the arena when it is 0ed
  // since we're clearing the arena we must clear this too
  for(int i = 0; i < ArrayLen(ctx->nodes.chunks); i++) {
    ctx->nodes.chunks[i] = 0;
  }

  if(ctx->scope) {
    for(size_t i = 0; i < ctx->scope->as.scope.scopes.count; i++) {
      slicehm_free(ctx->scope->as.scope.scopes.items[i]);
    }
    DaFree(ctx->scope->as.scope.scopes);
  }

  // This gets allocated from ctx->nodes so probably not a good idea to use it after clearing it
  ctx->nodeFreeList = 0;

  ctx->nodes.hdr.n = 0;
  // NOTE: node idx 0 is reserved
  ctx->nodeUniqueID = 1;
  ArenaClear(ctx->arena, true);
}

void ClearCompilerContext(CompilerContext *ctx)
{
  DeleteCompilerContext(ctx);

  ctx->scope = SON_AllocScope(ctx);
  ctx->startNode = SON_AllocFunctionStart(ctx);
}

#define GenerateGraphFile(ctx) \
  do {                         \
    string_builder sb_##__LINE__ = Graph_GenerateDotOutput(ctx, __FUNCTION__); \
    char *graph_file = temp_sprintf("bin/%s.dot", __FUNCTION__); \
    WriteEntireFile(graph_file,                               \
      sb_##__LINE__.items,                   \
      sb_##__LINE__.count);                  \
    VL_Log(VL_INFO, "Graph written to file: %s", graph_file); \
    SbFree(sb_##__LINE__);                   \
  } while(0)

#define EnableGraphStepsForTest()   \
  do {                              \
    ENABLE_GRAPH_STEPS = true;      \
    GRAPH_STEP_FILE_FMT = temp_sprintf("bin/steps/%s/%"U64_Fmt".dot", __FUNCTION__); \
    GRAPH_STEP_IDX = 0;             \
    vl_log_level prevLevel = VL_MinimalLogLevel; \
    VL_MinimalLogLevel = VL_ERROR;  \
    MkdirIfNotExist("bin/steps");   \
    MkdirIfNotExist(temp_sprintf("bin/steps/%s", __FUNCTION__)); \
    VL_MinimalLogLevel = prevLevel; \
  } while(0)

void PrintUsage(char *program)
{
  printf("Usage: %s <file>\n", program);
}

bool CompileRawData(CompilerContext *ctx, view data)
{
  ClearCompilerContext(ctx);

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
  memory_arena arena = {0};
  ArenaInit(&arena, 1024*1024, malloc(1024*1024));
  char *data = ReadEntireFile(&arena, filename, &fileSize);

  view filedata = ViewFromParts(data, fileSize);
  bool ok = CompileRawData(ctx, filedata);
  if(ok) {
    VL_Log(VL_INFO, "Compiled %s successfully", filename);
  } else {
    VL_Log(VL_ERROR, "Could not compile %s", filename);
  }
  free(arena.base);
  return ok;
}

void TestParseGrammar(CompilerContext *ctx)
{
  ClearCompilerContext(ctx);
  DISABLE_PEEPHOLE_OPTIMIZATIONS = true;
  EnableGraphStepsForTest();

  view data = VIEW("return 1+2*3+-5;");
  ctx->originalSource = data;
  ctx->currentSource = data;

  SON_Node *ret = Parse_CurrentContext(ctx);

  string_builder sb = {0};
  SON_PrintToBuilder(&sb, ret);

  VL_Log(VL_INFO, "output: "VIEW_FMT, VIEW_ARG(sb));
  Assert(ViewEq(VIEW("return (#1 + ((#2 * #3) + (-#5)));"), ViewFromParts(sb.items, sb.count)));

  SbFree(sb);

  GenerateGraphFile(ctx);

  DISABLE_PEEPHOLE_OPTIMIZATIONS = false;
  ENABLE_GRAPH_STEPS = false;
}

void TestPeepholeExample(CompilerContext *ctx)
{
  ClearCompilerContext(ctx);
  EnableGraphStepsForTest();

  // view data = VIEW("return 1+2*3;");
  view data = VIEW("return 1+2*3+-5;");
  ctx->originalSource = data;
  ctx->currentSource = data;

  SON_Node *ret = Parse_CurrentContext(ctx);

  string_builder sb = {0};
  SON_PrintToBuilder(&sb, ret);

  VL_Log(VL_INFO, "output: "VIEW_FMT, VIEW_ARG(sb));
  Assert(ViewEq(VIEW("return #2;"), ViewFromParts(sb.items, sb.count)));

  SbFree(sb);

  GenerateGraphFile(ctx);

  ENABLE_GRAPH_STEPS = false;
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

void testVariableDecl(CompilerContext *ctx)
{
  ClearCompilerContext(ctx);

  view data = VIEW("a: int = 1; return a;");
  ctx->originalSource = data;
  ctx->currentSource = data;

  SON_Node *ret = Parse_CurrentContext(ctx);
  string_builder sb = {0};
  SON_PrintToBuilder(&sb, ret);
  VL_Log(VL_INFO, "output: "VIEW_FMT, VIEW_ARG(sb));
  Assert(ViewEq(VIEW("return #1;"), ViewFromParts(sb.items, sb.count)));
  SbFree(sb);
}

void testVariableAdd(CompilerContext *ctx)
{
  ClearCompilerContext(ctx);

  view data = VIEW("a: int = 1; b: int = 2; return a+b;");
  ctx->originalSource = data;
  ctx->currentSource = data;

  SON_Node *ret = Parse_CurrentContext(ctx);
  string_builder sb = {0};
  SON_PrintToBuilder(&sb, ret);
  VL_Log(VL_INFO, "output: "VIEW_FMT, VIEW_ARG(sb));
  Assert(ViewEq(VIEW("return #3;"), ViewFromParts(sb.items, sb.count)));
  SbFree(sb);
}

void testVariableScope(CompilerContext *ctx)
{
  ClearCompilerContext(ctx);
  // DISABLE_PEEPHOLE_OPTIMIZATIONS = true;
  EnableGraphStepsForTest();

  view data = VIEW(
    "a := 1; b := 2; c := 0;"
    "{"
      "b: int = 3; c = a + b;"
    "}"
    "return c;"
  );
  ctx->originalSource = data;
  ctx->currentSource = data;

  SON_Node *ret = Parse_CurrentContext(ctx);
  string_builder sb = {0};
  SON_PrintToBuilder(&sb, ret);
  VL_Log(VL_INFO, "output: "VIEW_FMT, VIEW_ARG(sb));
  Assert(ViewEq(VIEW("return #4;"), ViewFromParts(sb.items, sb.count)));
  SbFree(sb);

  // GenerateGraphFile(ctx);
  // DISABLE_PEEPHOLE_OPTIMIZATIONS = false;
  ENABLE_GRAPH_STEPS = false;
}

void DoTests(CompilerContext *ctx)
{
  TestParseGrammar(ctx);
  TestAddPeephole(ctx);
  TestPeepholeExample(ctx);
  testVariableDecl(ctx);
  testVariableAdd(ctx);
  testVariableScope(ctx);


  DeleteCompilerContext(ctx);
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
