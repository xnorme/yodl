// FM. MA
#ifndef IVL_GENERATE_GRAPH
#define IVL_GENERATE_GRAPH

#include "simple_tree.h"
#include "root_class.h"

#include <string>
#include <map>
#include <iostream>
#include <vector>

class DotGraphGenerator {
public:
    enum class arrowHead {
        BOX, CROW,
        CURVE, ICURVE,
        DIAMOND, DOT,
        INV, NONE,
        NORMAL, TEE, VEE
    };

    enum compassPoint {
        NORTH, NORTHEAST,
        EAST, SOUTHEAST,
        SOUTH, SOUTHWEST,
        WEST, NORTHWEST,
        NIL // defaults to emptystring
    };

    enum class color {INDIGO, BLACK};

    enum shape {RECORD};

    DotGraphGenerator(compassPoint f, compassPoint t)
        : arrowFrom(f) , arrowTo(t) { }

    DotGraphGenerator(arrowHead a, color c, shape s,
                      compassPoint f, compassPoint t)
        : arrowShape(a) , nodeColor(c)
        , edgeColor(c) , nodeShape(s)
        , arrowFrom(f) , arrowTo(t) { }

    DotGraphGenerator(arrowHead a, color c, shape s)
        : arrowShape(a), nodeColor(c)
        , edgeColor(c), nodeShape(s) { }


    explicit DotGraphGenerator();

    // overloads for dot-graph generation
    int operator()(
        std::ostream &, std::string,
        const SimpleTree<std::map<std::string, std::string>> *);

    int operator()(
        std::string,
        const SimpleTree<std::map<std::string, std::string>> *);

    // output ascii data to stream
    int operator()(
        std::ostream &,
        const SimpleTree<std::map<std::string, std::string>> *);

    // outputs ascii tree to std::cout
    int operator()(
        const SimpleTree<std::map<std::string, std::string>> *);

//    int operator()(std::ostream &, std::string, AstNode *);

    DotGraphGenerator setBlacklist(const std::vector<std::string> &b){
        keyBlacklist = b;
        return *this;
    }

private:
    std::string path_to_string(std::vector<int> &path);
    int emit_edges(
        std::ostream &out,
        SimpleTree<std::map<std::string, std::string>> * ast);

    int emit_vertices(
        std::ostream &out,
        SimpleTree<std::map<std::string, std::string>> * ast,
        int depth = 0);

    int add_nodeids(
        SimpleTree<std::map<std::string, std::string>> *ast,
        std::vector<int> path,
        int depth = 0);

    int emit_dotgraph(
        std::ostream &out,
        std::string name,
        SimpleTree<std::map<std::string, std::string>> *ast);

    int emit_ascii_tree(
        std::ostream &,
        const SimpleTree<std::map<std::string, std::string>> *,
        const int);

private:
    arrowHead arrowShape = arrowHead::VEE;
    color nodeColor = color::INDIGO;
    color edgeColor = color::BLACK;
    shape nodeShape = shape::RECORD;

    int indentMult = 2;

    compassPoint arrowFrom = compassPoint::NIL;
    compassPoint arrowTo = compassPoint::NIL;

    std::vector<std::string> keyBlacklist;

    const char *NODEID = "NODEID";
    const char *GRAPH_ATTRIBS = "graph [];\n";
};

#endif /* IVL_GENERATE_GRAPH */
