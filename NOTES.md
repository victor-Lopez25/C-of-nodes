B is a definition, A uses B, then there is a def-use edge from B to A:

```mermaid
flowchart BT
  A["A"] --> B["B"]
```

There are two categories of nodes in the ir:
 - Control nodes
 - Data nodes