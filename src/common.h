#ifndef COMMON_H
#define COMMON_H

#include "node.h"
#include "vl_build.h"

typedef struct {
  memory_arena *arena;

  /* global id counter for nodes */
  uint64_t nodeUniqueID;

  SON_Node *startNode;
  // Should always be filled with 0s and not connected to any other nodes
  SON_Node sentinelNode;

  // TODO: This could be an Xarr
  struct {
    SON_Node *items;
    size_t count;
    size_t capacity;
  } nodes;

  view originalSource;
  view currentSource;
} CompilerContext;

#endif // COMMON_H