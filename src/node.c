#include "node.h"

SON_Node *SON_InitNode_Impl(CompilerContext *ctx, SON_Node *node, SON_NodeKind kind, SON_Node **inputs, size_t inputNodeCount)
{
  raddbg_pin(array(inputs, inputNodeCount));
  node->nodeID = ctx->nodeUniqueID++;
  node->kind = kind;

  // TODO: Do we always add all input nodes here? If so, this doesn't need to be a dyn array
  DaReserve(&node->inputs, inputNodeCount);
  mem_copy_non_overlapping(node->inputs.items, inputs, inputNodeCount*sizeof(SON_Node*));
  node->inputs.count = inputNodeCount;

  // Add this node as an output of its input nodes
  for(size_t i = 0; i < inputNodeCount; i++) {
    SON_Node *n = inputs[i];
    SON_AddUse(ctx, n, node);
  }

  return node;
}

void SON_ClearNode(SON_Node *node)
{
  node->inputs.count = 0;
  node->outputs.count = 0;
  node->kind = SON_Node_Unassigned;
  node->value = (SON_Value){
    .kind = SON_Value_Unassigned,
  };
}

SON_Node *SON_AllocNext_Impl(CompilerContext *ctx, SON_NodeKind kind, SON_Node **inputs, size_t inputNodeCount)
{
  SON_Node *node;
  if(!ctx->nodeFreeList) {
    node = ExpArrayAppend(ctx->arena, &ctx->nodes, (SON_Node){0});
  } else {
    node = ctx->nodeFreeList;
    ctx->nodeFreeList = node->nextFree;
  }

  return SON_InitNode_Impl(ctx, node, kind, inputs, inputNodeCount);
}

void SON_FreeNode(CompilerContext *ctx, SON_Node *node)
{
  Assert(SON_IsDead(node));

  SON_Node *thisFree = PushStruct(ctx->arena, SON_Node);
  if(ctx->nodeFreeList) {
    thisFree->nextFree = ctx->nodeFreeList;
  }
  ctx->nodeFreeList = thisFree;
  thisFree->inputs = node->inputs;
  thisFree->outputs = node->outputs;
  thisFree->nodeID = node->nodeID;
  thisFree->value = node->value;
  thisFree->kind = node->kind;
  thisFree->as = node->as;
}

/* Helpers */

/* control nodes */

SON_Node *SON_AllocReturn(CompilerContext *ctx, SON_Node *ctrl, SON_Node *data)
{
  if(!ctrl || !data) {
    VL_Log(VL_ERROR, "Tried to alloc SON_Node_FunctionReturn with no ctrl or data nodes");
    return 0;
  }
  SON_Node *node = SON_AllocNext(ctx, SON_Node_FunctionReturn, ctrl, data);
  SON_GraphStep(ctx);
  return node;
}

// NOTE: This will have more args later...
SON_Node *SON_AllocFunctionStart(CompilerContext *ctx)
{
  SON_Node *node = SON_AllocNext_Impl(ctx, SON_Node_FunctionStart, 0, 0);
  SON_GraphStep(ctx);
  return node;
}

/* data nodes */

SON_Node *SON_AllocConstant(CompilerContext *ctx, SON_Value value)
{
  Assert(SON_ValueIsConstant(value));
  SON_Node *node = SON_AllocNext(ctx, SON_Node_Constant, ctx->startNode);
  node->value = value;
  SON_GraphStep(ctx);
  return node;
}

/* unary operations */

SON_Node *SON_AllocPlus(CompilerContext *ctx, SON_Node *in)
{
  // NOTE: This is a no-op
  // return in;
  SON_Node *node = SON_AllocNext(ctx, SON_Node_UnaryOperation, in);
  node->as.unary.op = Operation_Plus;
  SON_GraphStep(ctx);
  return node;
}

/* This is for unary minus: '-' expr
 * For binary subtraction use SON_AllocSub(ctx, lhs, rhs)
 */
SON_Node *SON_AllocMinus(CompilerContext *ctx, SON_Node *in)
{
  SON_Node *node = SON_AllocNext(ctx, SON_Node_UnaryOperation, in);
  node->as.unary.op = Operation_Minus;
  SON_GraphStep(ctx);
  return node;
}

/* binary operations */

SON_Node *SON_AllocAdd(CompilerContext *ctx, SON_Node *lhs, SON_Node *rhs)
{
  SON_Node *node = SON_AllocNext(ctx, SON_Node_BinaryOperation, lhs, rhs);
  node->as.binary.op = Operation_Add;
  node->as.binary.orderMatters = false;
  SON_GraphStep(ctx);
  return node;
}

SON_Node *SON_AllocMul(CompilerContext *ctx, SON_Node *lhs, SON_Node *rhs)
{
  SON_Node *node = SON_AllocNext(ctx, SON_Node_BinaryOperation, lhs, rhs);
  node->as.binary.op = Operation_Mul;
  node->as.binary.orderMatters = false;
  SON_GraphStep(ctx);
  return node;
}

SON_Node *SON_AllocSub(CompilerContext *ctx, SON_Node *lhs, SON_Node *rhs)
{
  SON_Node *node = SON_AllocNext(ctx, SON_Node_BinaryOperation, lhs, rhs);
  node->as.binary.op = Operation_Sub;
  node->as.binary.orderMatters = true;
  SON_GraphStep(ctx);
  return node;
}

SON_Node *SON_AllocDiv(CompilerContext *ctx, SON_Node *lhs, SON_Node *rhs)
{
  SON_Node *node = SON_AllocNext(ctx, SON_Node_BinaryOperation, lhs, rhs);
  node->as.binary.op = Operation_Div;
  node->as.binary.orderMatters = true;
  SON_GraphStep(ctx);
  return node;
}

/* other nodes */

SON_Node *SON_AllocScope(CompilerContext *ctx)
{
  SON_Node *node = SON_AllocNext_Impl(ctx, SON_Node_Scope, 0, 0);
  node->value.kind = SON_Value_Bottom;
  SON_GraphStep(ctx);
  return node;
}

/* control nodes */

SON_Node *SON_FunctionReturnCtrl(SON_Node *node)
{
  Assert(node->kind == SON_Node_FunctionReturn);
  return node->inputs.items[0];
}
SON_Node *SON_FunctionReturnExpr(SON_Node *node)
{
  Assert(node->kind == SON_Node_FunctionReturn);
  return node->inputs.items[1];
}

/* data nodes */

SON_Node *SON_UnaryExpr(SON_Node *node)
{
  Assert(node->kind == SON_Node_UnaryOperation);
  return node->inputs.items[0];
}

SON_Node *SON_BinaryLhs(SON_Node *node)
{
  Assert(node->kind == SON_Node_BinaryOperation);
  return node->inputs.items[0];
}
SON_Node *SON_BinaryRhs(SON_Node *node)
{
  Assert(node->kind == SON_Node_BinaryOperation);
  return node->inputs.items[1];
}

/* other nodes */

SON_Node *SON_ScopeCtrl(SON_Node *node)
{
  Assert(node->kind == SON_Node_Scope);
  return node->inputs.items[0];
}
SON_Node *SON_ScopeSetCtrl(CompilerContext *ctx, SON_Node *node, SON_Node *n)
{
  Assert(node->kind == SON_Node_Scope);
  SON_Node *newNode = SON_SetDef(ctx, node, 0, n);
  SON_GraphStep(ctx);
  return newNode;
}

/* Get a list of symbol names from all of the scopes in this scopenode
 * the list returned is aligned with the inputs to the node
 */
StringViewList Scope_ReverseNames(SON_Node *scope)
{
  Assert(scope->kind == SON_Node_Scope);

  StringViewList list;
  list.count = scope->inputs.count;
  list.items = malloc(sizeof(view)*list.count);
  list.capacity = list.count;

  for(size_t hmIdx = 0; hmIdx < scope->as.scope.scopes.count; hmIdx++) {
    SymbolHashmap *hm = &scope->as.scope.scopes.items[hmIdx];
    for(size_t keyIdx = 0; keyIdx < stbds_hmlenu(*hm); keyIdx++) {
      SymbolEntry sym = (*hm)[keyIdx];
      list.items[sym.value] = sym.key;
    }
  }

  return list;
}

void Scope_Push(CompilerContext *ctx, SON_Node *scope)
{
  (void)ctx;
  Assert(scope->kind == SON_Node_Scope);
  DaAppend(&scope->as.scope.scopes, (SymbolHashmap)0);
}

void Scope_Pop(CompilerContext *ctx, SON_Node *scope)
{
  Assert(scope->kind == SON_Node_Scope);

  SymbolHashmap *hm = &scope->as.scope.scopes.items[scope->as.scope.scopes.count - 1];
  SON_PopNodes(ctx, scope, stbds_hmlenu(*hm));
  stbds_hmfree(*hm);
}

SON_Node *Scope_Define(CompilerContext *ctx, SON_Node *scope, view name, SON_Node *n)
{
  Assert(scope->kind == SON_Node_Scope);

  SymbolHashmap *hm = &scope->as.scope.scopes.items[scope->as.scope.scopes.count - 1];
  if(stbds_hmgetp_null(*hm, name)) {
    // double define
    return 0;
  }
  stbds_hmput(*hm, name, scope->inputs.count);

  SON_Node *node = SON_AddDef(ctx, scope, n);
  SON_GraphStep(ctx);
  return node;
}

SON_Node *Scope_Lookup(SON_Node *scope, view name)
{
  for(int i = (int)scope->as.scope.scopes.count - 1; i >= 0; i--) {
    SymbolHashmap *hm = &scope->as.scope.scopes.items[i];

    SymbolEntry *entry = stbds_hmgetp_null(*hm, name);
    if(entry) {
      return scope->inputs.items[entry->value];
    }
  }
  return 0;
}

SON_Node *Scope_Update(CompilerContext *ctx, SON_Node *scope, view name, SON_Node *node)
{
  for(int i = (int)scope->as.scope.scopes.count - 1; i >= 0; i--) {
    SymbolHashmap *hm = &scope->as.scope.scopes.items[i];

    SymbolEntry *entry = stbds_hmgetp_null(*hm, name);
    if(entry) {
      SON_Node *newNode = SON_SetDef(ctx, scope, entry->value, node);
      SON_GraphStep(ctx);
      return newNode;
    }
  }
  return 0;
}

const char *OperationKindToString(OperationKind op)
{
  switch(op) {
    /* unary */
    case Operation_Plus: return "+";
    case Operation_Minus: return "-";

    /* binary */
    case Operation_Add: return "+";
    case Operation_Mul: return "*";
    case Operation_Sub: return "-";
    case Operation_Div: return "//";
  }
  return "";
}

/* generic helpers */

bool SON_ValueIsConstant(SON_Value val)
{
  return ((val.kind > SON_Value_StartCanBeConstant) && (val.kind < SON_Value_EndCanBeConstant) && val.isConstant) || 
    (val.kind == SON_Value_Top);
}

// NOTE: In SeaOfNodes/Simple this is called 'isCFG()'
bool SON_IsControl(SON_Node *node)
{
  return node->kind > SON_Node_ControlStart && node->kind < SON_Node_ControlEnd;
}

bool SON_IsUnused(SON_Node *node)
{
  return node->outputs.count == 0;
}

bool SON_IsDead(SON_Node *node)
{
  return SON_IsUnused(node) && node->inputs.count == 0 && node->value.kind == SON_Value_Unassigned;
}

SON_Node *SON_AddUse(CompilerContext *ctx, SON_Node *node, SON_Node *use)
{
  (void)ctx;
  DaAppend(&node->outputs, use);
  return node;
}

SON_Node *SON_AddDef(CompilerContext *ctx, SON_Node *node, SON_Node *newDef)
{
  (void)ctx;
  DaAppend(&node->inputs, newDef);
  if(newDef != 0) {
    SON_AddUse(ctx, newDef, node);
  }
  return newDef;
}

/* Removes use from outputs, returns if the list is empty after deletion */
bool SON_DelUse(CompilerContext *ctx, SON_Node *node, SON_Node *use)
{
  (void)ctx;
  // linear search
  bool found = false;
  size_t i = 0;
  for(; i < node->outputs.count; i++) {
    if(node->outputs.items[i] == use) {
      found = true;
      break;
    }
  }

  if(found) {
    node->outputs.items[i] = node->outputs.items[node->outputs.count - 1];
    node->outputs.count--;
  }

  return node->outputs.count == 0;
}

/* Change a definition into a node, kills old definition if it has no outputs left */
SON_Node *SON_SetDef(CompilerContext *ctx, SON_Node *node, uint64_t idx, SON_Node *newDef)
{
  SON_Node *oldDef = node->inputs.items[idx];
  if(oldDef == newDef) return node;
  // If new def is not null, add the corresponding def->use edge
  // This needs to happen before removing the old node's def->use edge as
  // the newDef might get killed if the old node kills it recursively.
  if(newDef != 0) {
    SON_AddUse(ctx, newDef, node);
  }
  if(oldDef != 0 && SON_DelUse(ctx, oldDef, node)) {
    SON_KillNode(ctx, oldDef);
  }

  node->inputs.items[idx] = newDef;
  return newDef;
}

void SON_KillNode(CompilerContext *ctx, SON_Node *node)
{
  Assert(SON_IsUnused(node));
  // remove all inputs and kill unused nodes
  SON_PopNodes(ctx, node, node->inputs.count);
  node->value.kind = SON_Value_Unassigned;
  SON_FreeNode(ctx, node);
}

/* Pops n nodes from inputs and kills them if they have no outputs left */
void SON_PopNodes(CompilerContext *ctx, SON_Node *node, size_t n)
{
  Assert(node->inputs.count >= n);
  for(size_t i = 0; i < n; i++) {
    SON_Node *oldDef = node->inputs.items[node->inputs.count-- - 1];
    if(oldDef && SON_DelUse(ctx, oldDef, node)) {
      SON_KillNode(ctx, oldDef);
    }
  }
}

/* Get a string representation of the node kind */
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
    case SON_Node_UnaryOperation: return "UnaryOperation";
    case SON_Node_BinaryOperation: return "BinaryOperation";
    case SON_Node_DataEnd: return "DataEnd";
    case SON_Node_Scope: return "Scope";
  }
  return "";
}

const char *SON_NodeLabel(SON_Node *node)
{
  Assert(node);
  switch(node->kind) {
    case SON_Node_Unassigned: return "Unassigned";
    case SON_Node_ControlStart: return "ControlStart";
    case SON_Node_ControlEnd: return "ControlEnd";
    case SON_Node_DataStart: return "DataStart";
    case SON_Node_DataEnd: return "DataEnd";
    case SON_Node_FunctionStart: return "FunctionStart";
    case SON_Node_FunctionReturn: return "Return";

    case SON_Node_Constant: {
      switch(node->value.kind) {
        case SON_Value_Integer: {
          return temp_sprintf("#"S64_Fmt, node->value.as.integer);
        }
        case SON_Value_Floating: {
          return temp_sprintf("#%lf", node->value.as.floating);
        }
        case SON_Value_String: {
          return temp_sprintf("#'"VIEW_FMT"'", node->value.as.string);
        }
        default: {
          return "#Unassigned";
        }
      }
    }

    case SON_Node_UnaryOperation: {
      switch(node->as.unary.op) {
        case Operation_Plus: return "Plus";
        case Operation_Minus: return "Minus";
      }
    } break;

    case SON_Node_BinaryOperation: {
      switch(node->as.binary.op) {
        case Operation_Add: return "Add";
        case Operation_Sub: return "Sub";
        case Operation_Mul: return "Mul";
        case Operation_Div: return "Div";
      }
    } break;

    case SON_Node_Scope: {
      return "Scope";
    } break;
  }
  return "Unassigned";
}

const char *SON_NodeUniqueName(SON_Node *node)
{
  Assert(node);
  switch(node->kind) {
    case SON_Node_Unassigned: return temp_sprintf("Unassigned"U64_Fmt, node->nodeID);
    case SON_Node_ControlStart: return temp_sprintf("ControlStart"U64_Fmt, node->nodeID);
    case SON_Node_ControlEnd: return temp_sprintf("ControlEnd"U64_Fmt, node->nodeID);
    case SON_Node_DataStart: return temp_sprintf("DataStart"U64_Fmt, node->nodeID);
    case SON_Node_DataEnd: return temp_sprintf("DataEnd"U64_Fmt, node->nodeID);
    case SON_Node_FunctionStart: return temp_sprintf("FunctionStart"U64_Fmt, node->nodeID);
    case SON_Node_FunctionReturn: return temp_sprintf("Return"U64_Fmt, node->nodeID);

    /* Constant unique names:
     * integers: Const<id>_<val>i
     * floating: Const<id>_<val>f
     * string: Const<id>_<val>
     * other: Const<id>_Unassigned
     */
    case SON_Node_Constant: {
      switch(node->value.kind) {
        case SON_Value_Integer: {
          return temp_sprintf("Const"U64_Fmt, node->nodeID);
        }
        case SON_Value_Floating: {
          return temp_sprintf("Const"U64_Fmt, node->nodeID);
        }
        case SON_Value_String: {
          return temp_sprintf("Const"U64_Fmt, node->nodeID);
        }
        default: {
          return temp_sprintf("Const"U64_Fmt, node->nodeID);
        }
      }
    }

    case SON_Node_UnaryOperation: {
      switch(node->as.unary.op) {
        case Operation_Plus: return temp_sprintf("Plus"U64_Fmt, node->nodeID);
        case Operation_Minus: return temp_sprintf("Minus"U64_Fmt, node->nodeID);
      }
    } break;

    case SON_Node_BinaryOperation: {
      switch(node->as.binary.op) {
        case Operation_Add: return temp_sprintf("Add"U64_Fmt, node->nodeID);
        case Operation_Sub: return temp_sprintf("Sub"U64_Fmt, node->nodeID);
        case Operation_Mul: return temp_sprintf("Mul"U64_Fmt, node->nodeID);
        case Operation_Div: return temp_sprintf("Div"U64_Fmt, node->nodeID);
      }
    } break;

    case SON_Node_Scope: {
      return temp_sprintf("Scope"U64_Fmt, node->nodeID);
    } break;
  }
  return temp_sprintf("Unassigned"U64_Fmt, node->nodeID);
}

void SON_PrintToBuilder(string_builder *sb, SON_Node *node)
{
  Assert(node);
  switch(node->kind) {
    case SON_Node_Unassigned:
    case SON_Node_ControlStart:
    case SON_Node_ControlEnd:
    case SON_Node_DataStart:
    case SON_Node_DataEnd: {
      SbAppendf(sb, "%s", SON_NodeKindToString(node));
    } break;

    case SON_Node_FunctionStart: {
      SbAppendf(sb, "%s", SON_NodeKindToString(node));
    } break;

    case SON_Node_FunctionReturn: {
      SbAppendf(sb, "return ");
      SON_PrintToBuilder(sb, SON_FunctionReturnExpr(node));
      SbAppendf(sb, ";");
    } break;

    case SON_Node_Constant: {
      switch(node->value.kind) {
        case SON_Value_Integer: {
          SbAppendf(sb, "#"S64_Fmt, node->value.as.integer);
        } break;
        case SON_Value_Floating: {
          SbAppendf(sb, "#%lf", node->value.as.floating);
        } break;
        case SON_Value_String: {
          SbAppendf(sb, "#'"VIEW_FMT"'", node->value.as.string);
        } break;

        default: {
          SbAppendf(sb, "#Unassigned");
        } break;
      }
    } break;

    case SON_Node_UnaryOperation: {
      SbAppendf(sb, "(%s", OperationKindToString(node->as.unary.op));
      SON_PrintToBuilder(sb, SON_UnaryExpr(node));
      SbAppendCstr(sb, ")");
    } break;

    case SON_Node_BinaryOperation: {
      SbAppendCstr(sb, "(");
      SON_PrintToBuilder(sb, SON_BinaryLhs(node));
      SbAppendf(sb, " %s ", OperationKindToString(node->as.unary.op));
      SON_PrintToBuilder(sb, SON_BinaryRhs(node));
      SbAppendCstr(sb, ")");
    } break;

    case SON_Node_Scope: {
      SbAppendCstr(sb, SON_NodeLabel(node));
      for(size_t hmIdx = 0; hmIdx < node->as.scope.scopes.count; hmIdx++) {
        SymbolHashmap *hm = &node->as.scope.scopes.items[hmIdx];
        SbAppendCstr(sb, "[");
        for(size_t keyIdx = 0; keyIdx < stbds_hmlenu(hm); keyIdx++) {
          view name = (*hm)[keyIdx].key;
          if(keyIdx != 0) { SbAppendCstr(sb, ", "); }
          SbAppendBuf(sb, name.items, name.count);
          SbAppendCstr(sb, ":");
          if(keyIdx < node->inputs.count) {
            SON_Node *n = node->inputs.items[(*hm)[keyIdx].value];
            SON_PrintToBuilder(sb, n);
          } else {
            SbAppendCstr(sb, "null");
          }
        }
        SbAppendCstr(sb, "]");
      }
    } break;
  }
}

/* Optimizations */

SON_Value SON_Compute(SON_Node *node)
{
  Assert(node);
  switch(node->kind) {
    case SON_Node_Unassigned:
    case SON_Node_ControlStart:
    case SON_Node_ControlEnd:
    case SON_Node_DataStart:
    case SON_Node_DataEnd: {
      return (SON_Value){0};
    }

    case SON_Node_FunctionStart: {
      return (SON_Value){
        .kind = SON_Value_Bottom,
      };
    }

    case SON_Node_FunctionReturn: {
      return (SON_Value){
        .kind = SON_Value_Bottom,
      };
    }

    case SON_Node_Constant: {
      switch(node->value.kind) {
        case SON_Value_Integer: {
          return (SON_Value){
            .kind = SON_Value_Integer,
            .isConstant = true,
            .as.integer = node->value.as.integer,
          };
        }

        case SON_Value_Floating: {
          // TODO
          return (SON_Value){0};
        }

        case SON_Value_String: {
          // TODO
          return (SON_Value){0};
        }

        default: {
          return (SON_Value){0};
        }
      }
    }

    case SON_Node_UnaryOperation: {
      SON_Node *expr = SON_UnaryExpr(node);
      if(SON_ValueIsConstant(expr->value)) {
        switch(node->as.unary.op) {
          case Operation_Plus: {
            return expr->value;
          }
          case Operation_Minus: {
            return (SON_Value){
              .kind = SON_Value_Integer,
              .isConstant = true,
              .as.integer = -expr->value.as.integer,
            };
          }
        }
      }
      return (SON_Value){
        .kind = SON_Value_Bottom,
      };
    }

    case SON_Node_BinaryOperation: {
      SON_Node *lhs = SON_BinaryLhs(node);
      SON_Node *rhs = SON_BinaryRhs(node);
      if(SON_ValueIsConstant(lhs->value) && SON_ValueIsConstant(rhs->value)) {

#define Operation(op_enum_val, op) \
  case op_enum_val: {              \
    return (SON_Value){            \
      .kind = SON_Value_Integer,   \
      .isConstant = true,          \
      .as.integer = lhs->value.as.integer op rhs->value.as.integer, \
    };                             \
  }

        switch(node->as.binary.op) {
          Operation(Operation_Add, +)
          Operation(Operation_Sub, -)
          Operation(Operation_Mul, *)
          Operation(Operation_Div, /)
        }

#undef Operation
      }

      return (SON_Value){
        .kind = SON_Value_Bottom,
      };
    }

    case SON_Node_Scope: {
      return (SON_Value){
        .kind = SON_Value_Bottom,
      };
    }
  }
  return (SON_Value){0};
}

SON_Node *SON_Idealize(SON_Node *node)
{
  Assert(node);
  switch(node->kind) {
    case SON_Node_Unassigned:
    case SON_Node_ControlStart:
    case SON_Node_ControlEnd:
    case SON_Node_DataStart:
    case SON_Node_DataEnd: {
      return 0;
    } break;

    case SON_Node_FunctionStart: {
      return 0;
    } break;

    case SON_Node_FunctionReturn: {
      return 0;
    } break;

    case SON_Node_Constant: {
      return 0;
    } break;

    case SON_Node_UnaryOperation: {
      return 0;
    } break;

    case SON_Node_BinaryOperation: {
      return 0;
    } break;

    case SON_Node_Scope: {
      return 0;
    } break;
  }
  return 0;
}

SON_Node *SON_Peephole(CompilerContext *ctx, SON_Node *node)
{
  node->value = SON_Compute(node);
  SON_Value value = node->value;

  if(DISABLE_PEEPHOLE_OPTIMIZATIONS) {
    return node;
  }

  if(SON_ValueIsConstant(value) && node->kind != SON_Node_Constant) {
    // TODO: Keep this node just change it to be a new constant node
    SON_KillNode(ctx, node);
    SON_Node *newNode = SON_Peephole(ctx, SON_AllocConstant(ctx, value));
    return newNode;
  }

  SON_Node *n = SON_Idealize(node);
  if(n != 0) {
    return n;
  }

  return node;
}

void SON_GraphStep(CompilerContext *ctx)
{
  if(ENABLE_GRAPH_STEPS) {
    size_t mark = temp_save();
    string_builder sb = Graph_GenerateDotOutput(ctx, temp_sprintf("graph_"U64_Fmt, GRAPH_STEP_IDX));
    WriteEntireFile(temp_sprintf(GRAPH_STEP_FILE_FMT, GRAPH_STEP_IDX++), sb.items, sb.count);
    SbFree(sb);
    temp_rewind(mark);
  }
}
