#define VL_BUILD_IMPLEMENTATION
//#include "vl_build.h" (included in common.h)
#include "common.h"

#include "node.c"
#include "parser.c"

#define GLOBAL_ARENA_SIZE 32*1024*1024

// https://github.com/SeaOfNodes/Simple/blob/main/chapter01/README.md
// NOTE: Types prefixed with SON_ mean SeaOfNodes_

bool InitCompilerContext(CompilerContext *ctx)
{
  Assert(ctx);

  ArenaInit(ctx->arena, GLOBAL_ARENA_SIZE, malloc(GLOBAL_ARENA_SIZE));
  // NOTE: node idx 0 is reserved
  ctx->nodeUniqueID = 1;
  ctx->startNode = SON_AllocFunctionStart(ctx);

  ctx->sentinelNode.nodeID = 0;
  ctx->sentinelNode.kind = SON_Node_Unassigned;

  return true;
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
  SON_Node *expr = SON_ReturnNodeExpr(ret);
  VL_Log(VL_INFO, "return value: %d", (int)expr->as.constant.as.integer);

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

  char *file = argv[1];
  bool ok = CompileFile(&ctx, file);
  (void)ok;

  return 0;
}
