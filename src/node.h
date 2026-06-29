#ifndef NODE_H
#define NODE_H

#include "vl_build.h"

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

  /* End of the data node kinds
   * This is an invalid node kind (only used to identify a range)
   */
  SON_Node_DataEnd,
} SON_NodeKind;

typedef enum {
  SON_Literal_Unassigned = 0,
  SON_Literal_String,
  SON_Literal_Integer,
  SON_Literal_Floating,
} SON_LiteralKind;

typedef struct {
  SON_LiteralKind kind;
  union {
    view string;
    int64_t integer;
    double floating;
  } as;
} SON_Constant;

struct SON_NodeList {
  SON_Node **items;
  size_t capacity;
  size_t count;
};

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

  uint64_t nodeID;
  SON_NodeKind kind;
  union {
    SON_Constant constant;
  } as;
};

#endif // NODE_H
