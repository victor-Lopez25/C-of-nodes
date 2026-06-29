#include "common.h"

#define NodeEq(A, B) ((A)->nodeID == (B)->nodeID)

static void Graph_Walk(SON_NodeList *list, SON_Node *n)
{
  // TODO: This could probably use a hashmap instead, will be quicker to search for a node by node id
  bool found = false;
  LinearSearch(list, n, NodeEq);
  if(found) return;
  DaAppend(list, n);
  for(size_t i = 0; i < n->inputs.count; i++) {
    Graph_Walk(list, n->inputs.items[i]);
  }
  for(size_t i = 0; i < n->outputs.count; i++) {
    Graph_Walk(list, n->outputs.items[i]);
  }
}

static SON_NodeList Graph_FindAll(SON_Node *startNode)
{
  SON_NodeList list = {0};
  for(size_t i = 0; i < startNode->outputs.count; i++) {
    Graph_Walk(&list, startNode->outputs.items[i]);
  }
  return list;
}

static void Graph_Nodes(string_builder *sb, SON_NodeList list)
{
  SbAppendCstr(sb, "\tsubgraph cluster_Nodes {\n"); // Magic "cluster_" in the subgraph name
  for(size_t i = 0; i < list.count; i++) {
    SON_Node *n = list.items[i];
    SbAppendf(sb, "\t\t%s [ ", SON_NodeUniqueName(n));
    const char *label = SON_NodeLabel(n);

    if(SON_IsControl(n)) {
      SbAppendCstr(sb, "shape=box style=filled fillcolor=yellow ");
    }
    SbAppendf(sb, "label=\"%s\" ", label);
    SbAppendCstr(sb, "];\n");
  }
  SbAppendCstr(sb, "\t}\n");
}

static char *Graph_MakeScopeName(SON_Node *scope, uint64_t level)
{
  return temp_sprintf("%s_"U64_Fmt, SON_NodeUniqueName(scope), level);
}

static char *Graph_MakePortName(SON_Node *scope, view name)
{
  return temp_sprintf("%s_"VIEW_FMT, SON_NodeUniqueName(scope), VIEW_ARG(name));
}

static void Graph_Scopes(string_builder *sb, SON_Node *scope)
{
  SbAppendCstr(sb, "\tnode [shape=plaintext];\n");

  size_t level = 0;
  for(; level < scope->as.scope.scopes.count; level++) {
    SymbolHashmap *hm = &scope->as.scope.scopes.items[level];
    char *scopeName = Graph_MakeScopeName(scope, level);
    // Magic "cluster_" in the subgraph name
    SbAppendf(sb, "\tsubgraph cluster_%s {\n", scopeName);
    SbAppendf(sb, "\t\t%s [label=<\n", scopeName);
    SbAppendCstr(sb, "\t\t\t<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">\n");
    // Add the scope level
    SbAppendf(sb, "\t\t\t<TR><TD BGCOLOR=\"cyan\">"U64_Fmt"</TD>", level);
    for(size_t i = 0; i < stbds_hmlenu(*hm); i++) {
      SymbolEntry sym = (*hm)[i];
      SbAppendf(sb, "<TD PORT=\"%s\">"VIEW_FMT"</TD>", Graph_MakePortName(scope, sym.key), VIEW_ARG(sym.key));
    }
    SbAppendCstr(sb, "</TR>\n");
    SbAppendCstr(sb, "\t\t\t</TABLE>>];\n");
  }

  // Scope clusters nest, so the graphics shows the nested scopes, so
  // they are not closed as they are printed; so they just keep nesting.
  // We close them all at once here.
  for(size_t i = 0; i < level; i++) {
    SbAppendCstr(sb, "\t}\n");
  }
}

static void Graph_NodeEdges(string_builder *sb, SON_NodeList list)
{
  SbAppendCstr(sb, "\tedge [ fontname=Helvetica, fontsize=8 ];\n");
  for(size_t nodeIdx = 0; nodeIdx < list.count; nodeIdx++) {
    SON_Node *n = list.items[nodeIdx];
    // In this chapter we do display the Constant->Start edge;
    for(size_t inIdx = 0; inIdx < n->inputs.count; inIdx++) {
      SON_Node *def = n->inputs.items[inIdx];
      // Most edges land here use->def
      SbAppendf(sb, "\t%s -> %s", SON_NodeUniqueName(n), SON_NodeUniqueName(def));
      // Number edges, so we can see how they track
      SbAppendf(sb, "[taillabel=%d", (int)inIdx);
      if(n->kind == SON_Node_Constant && def->kind == SON_Node_FunctionStart) {
        SbAppendCstr(sb, " style=dotted");
      } else if(SON_IsControl(def)) {
        // control edges are colored red
        SbAppendCstr(sb, " color=red");
      }
      SbAppendCstr(sb, "];\n");
    }
  }
}

static void Graph_ScopeEdges(string_builder *sb, SON_Node *scope)
{
  SbAppendCstr(sb, "\tedge [style=dashed color=cornflowerblue];\n");
  for(size_t level = 0; level < scope->as.scope.scopes.count; level++) {
    SymbolHashmap *hm = &scope->as.scope.scopes.items[level];
    char *scopeName = Graph_MakeScopeName(scope, level);
    for(size_t i = 0; i < stbds_hmlenu(*hm); i++) {
      SymbolEntry sym = (*hm)[i];
      if((sym.value > scope->inputs.count) || (scope->inputs.items[sym.value] == 0)) {
        continue;
      }
      SON_Node *def = scope->inputs.items[sym.value];
      SbAppendf(sb, "\t%s:\"%s\" -> %s;\n", scopeName, Graph_MakePortName(scope, sym.key), SON_NodeUniqueName(def));
    }
  }
}

string_builder Graph_GenerateDotOutput(CompilerContext *ctx, char *name)
{
  string_builder sb = {0};
  SON_NodeList all = Graph_FindAll(ctx->startNode);
  SbAppendf(&sb, 
    "digraph %s {\n"
    "/*\n"
    VIEW_FMT"\n"
    "*/\n", name, VIEW_ARG(ctx->originalSource));

  // To keep the Scopes below the graph and pointing up into the graph we
  // need to group the Nodes in a subgraph cluster, and the scopes into a
  // different subgraph cluster.  THEN we can draw edges between the
  // scopes and nodes.  If we try to cross subgraph cluster borders while
  // still making the subgraphs DOT gets confused.
  SbAppendCstr(&sb, "\trankdir=BT;\n"); // Force Nodes before Scopes

  // Preserve node input order
  SbAppendCstr(&sb, "\tordering=\"in\";\n");

  // Merge multiple edges hitting the same node.  Makes common shared
  // nodes much prettier to look at.
  SbAppendCstr(&sb, "\tconcentrate=\"true\";\n");

  Graph_Nodes(&sb, all);

  Graph_Scopes(&sb, ctx->scope);

  Graph_NodeEdges(&sb, all);

  Graph_ScopeEdges(&sb, ctx->scope);

  SbAppendCstr(&sb, "}\n");

  DaFree(all);
  return sb;
}