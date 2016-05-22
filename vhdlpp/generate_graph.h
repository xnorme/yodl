// FM. MA
#ifndef IVL_GENERATE_GRAPH
#define IVL_GENERATE_GRAPH

#include "simple_tree/simple_tree.h"

#include <string>
#include <map>
#include <iostream>
#include <vector>

int emit_vertices(ostream &out, simple_tree<map<string, string>> *ast);
int emit_dotgraph(ostream &out, string name, simple_tree<map<string, string>> *ast);

#endif /* IVL_GENERATE_GRAPH */