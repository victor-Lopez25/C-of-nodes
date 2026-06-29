#ifndef COMMON_H
#define COMMON_H

#include "raddbg_markup.h"
#include "vl_build.h"

#define SON_NODE_XAR_CHUNK_COUNT VICLIB_EXP_ARRAY_CHUNK_COUNT
#define SON_NODE_XAR_CHUNK_SHIFT VICLIB_EXP_ARRAY_CHUNK_SHIFT

typedef struct SON_Node SON_Node;
typedef struct SON_NodeList SON_NodeList;

typedef enum {
  SON_Node_Unassigned = 0,

  ///////////////////////////////
  // Control nodes
  ///////////////////////////////

  /* Start of the control node kinds
   * This is an invalid node kind (only used to identify a range)
   */
  SON_Node_ControlStart,

  /* Start of function node
   * 
   * Inputs: None
   * 
   * Value: None for now as we do not have function arguments yet
   */
  SON_Node_FunctionStart,
  /* Function return node
   *
   * Inputs: Predecessor control node, data node value
   * 
   * Value: Return value of the function
   */
  SON_Node_FunctionReturn,

  /* End of the control node kinds
   * This is an invalid node kind (only used to identify a range)
   */
  SON_Node_ControlEnd,

  ///////////////////////////////
  // Data nodes
  ///////////////////////////////

  /* Start of data node kinds
   * This is an invalid node kind (only used to identify a range)
   */
  SON_Node_DataStart,

  /* Constant value node, used to represent constants such as integer literals
   *
   * Inputs: None, however start node is set as input node to enable graph walking
   * 
   * Value: Value of constant
   */
  SON_Node_Constant,

  /* Binary operation node
   *
   * Inputs: One data node
   * 
   * Value: <unary_op> in0
   */
  SON_Node_UnaryOperation,

  /* Binary operation node
   *
   * Inputs: Two data nodes
   * 
   * Value: in0 <binary_op> in1
   */
  SON_Node_BinaryOperation,

  /* End of the data node kinds
   * This is an invalid node kind (only used to identify a range)
   */
  SON_Node_DataEnd,

  ///////////////////////////////
  // Other nodes
  ///////////////////////////////

  /* Represents scopes in the graph
   *
   * Type: Symbol table
   * 
   * Inputs: All nodes that define variables
   * 
   * Value: None
   */
  SON_Node_Scope,
} SON_NodeKind;

// TODO: This enum can definitely be simplified
typedef enum {
  SON_Value_Unassigned = 0,

  SON_Value_Bottom = 1,
  SON_Value_All = SON_Value_Bottom,

  SON_Value_Top = 2,
  SON_Value_Any = SON_Value_Top,

  SON_Value_Simple = 3,


  SON_Value_StartCanBeConstant = 4,

  SON_Value_String = 5,
  SON_Value_Integer = 6,
  SON_Value_Floating = 7,

  SON_Value_EndCanBeConstant = 8,
} SON_ValueKind;

typedef struct {
  SON_ValueKind kind;
  bool isConstant;
  union {
    view string;
    int64_t integer;
    double floating;
  } as;
} SON_Value;

struct SON_NodeList {
  SON_Node **items;
  size_t capacity;
  size_t count;
};

typedef enum {
  // unary
  /* unary plus '+' expr */
  Operation_Plus,
  /* unary plus '-' expr */
  Operation_Minus,

  // binary
  Operation_Add, /* order does not matter */
  Operation_Mul, /* order does not matter */
  Operation_Sub, /* order matters */
  Operation_Div, /* order matters */
} OperationKind;

typedef struct {
  OperationKind op;
} SON_UnaryOpNode;

typedef struct {
  OperationKind op;
  bool orderMatters;
} SON_BinaryOpNode;

struct SON_Node {
  // TODO: When the structure is more solid, it might be worth trying to remove some of the nodelists
  //       since some nodes have a fixed amount of nodes (eg. FunctionReturn has 1 control, 1 value node)

  /**
   * Inputs to the node. These are use-def references to Nodes.
   * <p>
   * Generally fixed length, ordered, nulls allowed, no unused trailing space.
   * Ordering is required because e.g. "a/b" is different from "b/a".
   * The first input (offset 0) is often a Control node.
   */
  SON_NodeList inputs;

  /**
   * Outputs reference Nodes that are not null and have this Node as an
   * input.  These nodes are users of this node, thus these are def-use
   * references to Nodes.
   * <p>
   * Outputs directly match inputs, making a directed graph that can be
   * walked in either direction.  These outputs are typically used for
   * efficient optimizations but otherwise have no semantics meaning.
   */
  SON_NodeList outputs;

  ///////////////////////////

  // used to keep a free list of nodes
  SON_Node *nextFree;

  ///////////////////////////

  uint64_t nodeID;

  /* Current computed type for this Node. This value changes as the graph changes
   * and more knowledge is gained about the program
   */
  SON_Value value;

  SON_NodeKind kind;
  union {
    SON_UnaryOpNode unary;
    SON_BinaryOpNode binary;
  } as;
};

/////////////////////////////////////////////

typedef struct {
  memory_arena *arena;

  /* global id counter for nodes */
  uint64_t nodeUniqueID;

  SON_Node *startNode;
  // Should always be filled with 0s and not connected to any other nodes
  SON_Node sentinelNode;

  exp_array(SON_Node, SON_NODE_XAR_CHUNK_COUNT) nodes;
  // struct {
  //  SON_Node *items;
  //  size_t count;
  //  size_t capacity;
  //} nodes;

  SON_Node *nodeFreeList;

  view originalSource;
  view currentSource;
} CompilerContext;

#if COMPILER_CL

// NOTE: This one doesn't work well, I'll try to fix it at some point
//raddbg_type_view(exp_array(SON_Node, SON_NODE_XAR_CHUNK_COUNT), array($.chunks, SON_NODE_XAR_CHUNK_COUNT));

#endif // COMPILER_CL

extern bool DISABLE_PEEPHOLE_OPTIMIZATIONS;
extern bool ENABLE_GRAPH_STEPS;
extern char *GRAPH_STEP_FILE_FMT;
extern uint64_t GRAPH_STEP_IDX;

string_builder Graph_GenerateDotOutput(CompilerContext *ctx, char *name);

#endif // COMMON_H