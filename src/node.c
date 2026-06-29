#include "common.h"

#define SON_AllocNext(ctx, kind, ...) \
  SON_AllocNext_Impl(ctx, kind, \
      ((SON_Node*[]){__VA_ARGS__}), \
      (sizeof((SON_Node*[]){__VA_ARGS__})/sizeof(SON_Node*)))

SON_Node *SON_AllocNext_Impl(CompilerContext *ctx, SON_NodeKind kind, SON_Node **inputs, size_t inputNodeCount)
{
  DaAppend(&ctx->nodes, (SON_Node){0});
  SON_Node *node = ctx->nodes.items + ctx->nodes.count - 1;
  node->nodeID = ctx->nodeUniqueID++;
  node->kind = kind;

  // TODO: Do we always add all input nodes here? If so, this doesn't need to be a dyn array
  DaReserve(&node->inputs, inputNodeCount);
  mem_copy_non_overlapping(node->inputs.items, inputs, inputNodeCount*sizeof(SON_Node*));

  // Add this node as an output of its input nodes
  for(size_t i = 0; i < inputNodeCount; i++) {
    SON_Node *n = inputs[i];
    DaAppend(&n->outputs, node);
  }

  return node;
}

bool NodeIsUnused(SON_Node *node)
{
  return node->outputs.count == 0;
}

// NOTE: In SeaOfNodes/Simple this is called 'isCFG()'
bool NodeIsControl(SON_Node *node)
{
  return node->kind > SON_Node_ControlStart && node->kind < SON_Node_ControlEnd;
}

/* Helpers */

SON_Node *SON_AllocConstant(CompilerContext *ctx, SON_Constant value)
{
  SON_Node *node = SON_AllocNext(ctx, SON_Node_Constant, ctx->startNode);
  node->as.constant = value;
  return node;
}

SON_Node *SON_AllocReturn(CompilerContext *ctx, SON_Node *ctrl, SON_Node *data)
{
  if(!ctrl || !data) {
    VL_Log(VL_ERROR, "Tried to alloc SON_Node_FunctionReturn with no ctrl or data nodes");
    return 0;
  }
  SON_Node *node = SON_AllocNext(ctx, SON_Node_FunctionReturn, ctrl, data);
  return node;
}

// NOTE: This will have more args later...
SON_Node *SON_AllocFunctionStart(CompilerContext *ctx)
{
  return SON_AllocNext_Impl(ctx, SON_Node_FunctionStart, 0, 0);
}

SON_Node *SON_ReturnNodeCtrl(SON_Node *node)
{
  Assert(node->kind == SON_Node_FunctionReturn);
  return node->inputs.items[0];
}
SON_Node *SON_ReturnNodeExpr(SON_Node *node)
{
  Assert(node->kind == SON_Node_FunctionReturn);
  return node->inputs.items[1];
}

const char *SON_NodeKindToString(SON_Node *node)
{
  switch(node->kind) {
    case SON_Node_Unassigned: return "Unassigned";
    case SON_Node_ControlStart: return "ControlStart";
    case SON_Node_FunctionStart: return "FunctionStart";
    case SON_Node_FunctionReturn: return "FunctionReturn";
    case SON_Node_ControlEnd: return "ControlEnd";
    case SON_Node_DataStart: return "DataStart";
    case SON_Node_Constant: return "Constant";
    case SON_Node_DataEnd: return "DataEnd";
  }
  return "";
}
