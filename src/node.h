#ifndef NODE_H
#define NODE_H

#include "common.h"

#define SON_InitNode(ctx, node, kind, ...) \
  SON_InitNode_Impl(ctx, node, kind, \
      ((SON_Node*[]){__VA_ARGS__}), \
      (sizeof((SON_Node*[]){__VA_ARGS__})/sizeof(SON_Node*)))

#define SON_AllocNext(ctx, kind, ...) \
  SON_AllocNext_Impl(ctx, kind, \
      ((SON_Node*[]){__VA_ARGS__}), \
      (sizeof((SON_Node*[]){__VA_ARGS__})/sizeof(SON_Node*)))

SON_Node *SON_InitNode_Impl(CompilerContext *ctx, SON_Node *node, SON_NodeKind kind, SON_Node **inputs, size_t inputNodeCount);
SON_Node *SON_AllocNext_Impl(CompilerContext *ctx, SON_NodeKind kind, SON_Node **inputs, size_t inputNodeCount);

void SON_ClearNode(SON_Node *node);
void SON_FreeNode(CompilerContext *ctx, SON_Node *node);

/* helpers */

SON_Node *SON_AllocReturn(CompilerContext *ctx, SON_Node *ctrl, SON_Node *data);
SON_Node *SON_AllocFunctionStart(CompilerContext *ctx);

SON_Node *SON_AllocConstant(CompilerContext *ctx, SON_Value value);

SON_Node *SON_AllocPlus(CompilerContext *ctx, SON_Node *in);
SON_Node *SON_AllocMinus(CompilerContext *ctx, SON_Node *in);

SON_Node *SON_AllocAdd(CompilerContext *ctx, SON_Node *lhs, SON_Node *rhs);
SON_Node *SON_AllocMul(CompilerContext *ctx, SON_Node *lhs, SON_Node *rhs);
SON_Node *SON_AllocSub(CompilerContext *ctx, SON_Node *lhs, SON_Node *rhs);
SON_Node *SON_AllocDiv(CompilerContext *ctx, SON_Node *lhs, SON_Node *rhs);

/* getters */

SON_Node *SON_FunctionReturnCtrl(SON_Node *node);
SON_Node *SON_FunctionReturnExpr(SON_Node *node);

SON_Node *SON_UnaryExpr(SON_Node *node);
SON_Node *SON_BinaryLhs(SON_Node *node);
SON_Node *SON_BinaryRhs(SON_Node *node);

bool SON_ValueIsConstant(SON_Value val);
bool SON_IsControl(SON_Node *node);
bool SON_IsUnused(SON_Node *node);
bool SON_IsDead(SON_Node *node);

/* modifications */

SON_Node *SON_SetDef(CompilerContext *ctx, SON_Node *node, uint64_t idx, SON_Node *new_def);
void SON_KillNode(CompilerContext *ctx, SON_Node *node);

/* printing */

const char *OperationKindToString(OperationKind op);
const char *SON_NodeKindToString(SON_Node *node);
const char *SON_NodeLabel(SON_Node *node);
const char *SON_NodeUniqueName(SON_Node *node);
void SON_PrintToBuilder(string_builder *sb, SON_Node *node);

/* optimizations */

SON_Value SON_Compute(SON_Node *node);
SON_Node *SON_Idealize(SON_Node *node);
SON_Node *SON_Peephole(CompilerContext *ctx, SON_Node *node);

/* debugging */

void SON_GraphStep(CompilerContext *ctx);

#endif // NODE_H
