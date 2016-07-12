// FM. MA
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <iostream>

// YOSYS specific headers
#include <kernel/yosys.h>
#include <kernel/rtlil.h>

// code base specific includes
#include "generate_graph.h"
#include "simple_tree.h"
#include "StringHeap.h"
#include "entity.h"
#include "compiler.h"
#include "sequential.h"
#include "library.h"
#include "std_funcs.h"
#include "std_types.h"
#include "architec.h"
#include "parse_api.h"
#include "generic_traverser.h"
#include "vtype.h"
#include "std_types.h"
#include "std_funcs.h"
#include "parse_context.h"
#include "test.h"
#include "root_class.h"
#include "mach7_includes.h"
#include "path_finder.h"


bool verbose_flag = false;
// Where to dump design entities
const char *work_path = "ivl_vhdl_work";
const char *dump_design_entities_path = 0;
const char *dump_libraries_path       = 0;
const char *debug_log_path            = 0;

bool     debug_elaboration = false;
ofstream debug_log_file;

TEST_CASE("Yosys RTLIL construction", "[rtlil usage]"){

}


TEST_CASE("Simple block", "[ast]"){
    int rc;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                       perm_string(), context);

    REQUIRE(rc == 0);
    REQUIRE(context->parse_errors == 0);
    REQUIRE(context->parse_sorrys == 0);
}

TEST_CASE("Simple for loop", "[ast]"){
    int rc;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file(
        "vhdl_testfiles/for_loop_simple.vhd",
        perm_string(), context);

    REQUIRE(rc == 0);
    REQUIRE(context->parse_errors == 0);
    REQUIRE(context->parse_sorrys == 0);

    REQUIRE(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    auto entity1 = iterator->second;

    emit_dotgraph(std::cout, "foo", entity1->emit_strinfo_tree());
}

TEST_CASE("Multiple parses", "[ast]"){
    int rc1, rc2;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    StandardTypes *std_types1 = (new StandardTypes())->init();
    StandardFunctions *std_funcs1 = (new StandardFunctions())->init();
    ParserContext *context1 = (new ParserContext(std_types1, std_funcs1))->init();

    rc1 = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                        perm_string(), context);
    rc2 = ParserUtil::parse_source_file("vhdl_testfiles/adder.vhd",
                                        perm_string(), context1);

    REQUIRE(rc1 == 0);
    REQUIRE(rc2 == 0);
    REQUIRE(context->parse_errors  == 0);
    REQUIRE(context->parse_errors  == 0);
    REQUIRE(context1->parse_sorrys == 0);
    REQUIRE(context1->parse_sorrys == 0);
}

TEST_CASE("Simple clone test", "[clone]"){
    int rc;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                       perm_string(), context);

    REQUIRE(rc == 0);
    REQUIRE(rc == 0);
    REQUIRE(context->parse_errors  == 0);
    REQUIRE(context->parse_errors  == 0);

    REQUIRE(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    REQUIRE(iterator->second != NULL);

    auto cloned_entity = iterator->second->clone();
    REQUIRE(cloned_entity != NULL);
}

TEST_CASE("Simple clone test with dot generation", "[clone]"){
    int rc;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                       perm_string(), context);

    REQUIRE(rc == 0);
    REQUIRE(rc == 0);
    REQUIRE(context->parse_errors == 0);
    REQUIRE(context->parse_errors == 0);

    REQUIRE(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    auto entity1 = iterator->second;
    REQUIRE(entity1 != NULL);

    auto entity2 = iterator->second->clone();
    REQUIRE(entity2 != NULL);

    stringstream a{};
    stringstream b{};

    auto tree1 = entity1->emit_strinfo_tree();
    auto tree2 = entity2->emit_strinfo_tree();

    emit_dotgraph(a, "foo", entity1->emit_strinfo_tree());
    emit_dotgraph(b, "foo", entity2->emit_strinfo_tree());

    REQUIRE((*tree1 == *tree2) == true);
    REQUIRE(a.str() == b.str());
}

TEST_CASE("Test equality of simple tree", "[simple tree]"){
    auto tree1 = new SimpleTree<map<string, string>>(
        map<string, string>({
                {"foo", "bar"},
                {"bar", "foo"}}),
        empty_simple_tree());

    auto tree2 = new SimpleTree<map<string, string>>(
        map<string, string>({
                {"foo", "bar"},
                {"bar", "foo"}}),
        empty_simple_tree());

    REQUIRE((*tree1 == *tree2) == true);

    tree1->root["bar"] = "fnord";
    REQUIRE((*tree1 == *tree2) == false);
}

TEST_CASE("Test simple generic traversal", "[generic traverser]"){
    using namespace mch;

    int rc;
    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                       perm_string(), context);

    REQUIRE(rc == 0);
    REQUIRE(rc == 0);
    REQUIRE(context->parse_errors == 0);
    REQUIRE(context->parse_errors == 0);

    REQUIRE(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    auto entity1 = iterator->second;
    REQUIRE(entity1 != NULL);

    AstNode *root = entity1;

    StatefulLambda<int> state = StatefulLambda<int>(
        0,
        static_cast<function <int (const AstNode *, int &)>>(
        [](const AstNode *, int &env) -> int {
            cout << "[VISITOR] Found node!"  << endl;
            env++;
            return 0;
        }));

    GenericTraverser traverser(
        [=](const AstNode *node){
            Match(node){
                Case(C<BlockStatement>()){ return true; }
                Otherwise(){ return false; }
            } EndMatch;
            return false; //without: compiler warning
        },
        static_cast<function<int (const AstNode *)>>(
            [&state](const AstNode *a) -> int { return state(a); }),
        root,
        GenericTraverser::RECUR);

    traverser.traverse();
    REQUIRE(state.environment == 2);
}

TEST_CASE("Test simple generic traversal on cloned AST", "[generic traverser]"){
    using namespace mch;

    int rc;
    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                       perm_string(), context);

    REQUIRE(rc == 0);
    REQUIRE(rc == 0);
    REQUIRE(context->parse_errors == 0);
    REQUIRE(context->parse_errors == 0);

    REQUIRE(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    auto entity1 = iterator->second;
    REQUIRE(entity1 != NULL);

    AstNode *root = entity1->clone();

    StatefulLambda<int> state = StatefulLambda<int>(
        0,
        static_cast<function<int (const AstNode *, int &)>>(
            [](const AstNode *, int &env) -> int {
                cout << "[VISITOR] Found node!"  << endl;
                env++;
                return 0;
            }));

    GenericTraverser traverser(
        [=](const AstNode *node){
            Match(node){
                Case(C<BlockStatement>()){ return true; }
                Otherwise(){ return false; }
            } EndMatch;
            return false; //without: compiler warning
        },
        // static_cast needed here in order to resolve the
        // overload resolution ambiguity arising from the use
        // of std::function
        static_cast<function<int (const AstNode *)>>(
            [&state](const AstNode *a) -> int { return state(a); }),
        root,
        GenericTraverser::RECUR);

    traverser.traverse();
    REQUIRE(state.environment == 2);
    traverser.emitTraversalMessages(cout, "\n");
    traverser.emitErrorMessages(cout, "\n");
}

TEST_CASE("Test nary traverser", "[generic traverser]"){
    vector<vector<AstNode *>> res1, res2;

    ExpInteger *int1 = new ExpInteger(100);
    ExpInteger *int2 = new ExpInteger(101);
    ExpInteger *int3 = new ExpInteger(102);
    ExpInteger *int4 = new ExpInteger(103);

    ExpArithmetic *arith1 = new ExpArithmetic(ExpArithmetic::PLUS, int1, int2);
    ExpArithmetic *arith2 = new ExpArithmetic(ExpArithmetic::PLUS, int3, int4);

    ExpArithmetic *arith = new ExpArithmetic(ExpArithmetic::MULT, arith1, arith2);
    ExpArithmetic *aUnb = new ExpArithmetic(ExpArithmetic::MULT, arith1, int4);

    cout << "arith: " <<  static_cast<AstNode *>(arith) << endl;
    cout << "aUnb: " <<  static_cast<AstNode *>(aUnb) << endl;
    cout << "arith1: " << static_cast<AstNode *>(arith1) << endl;
    cout << "arith2: " << static_cast<AstNode *>(arith2) << endl;
    cout << "int1: " <<   static_cast<AstNode *>(int1) << endl;
    cout << "int2: " <<   static_cast<AstNode *>(int2) << endl;
    cout << "int3: " <<   static_cast<AstNode *>(int3) << endl;
    cout << "int4: " <<   static_cast<AstNode *>(int4) << endl;

    // check function PathFinder::getListOfchilds
    const std::list<AstNode *> childs1 = PathFinder::getListOfChilds(arith);
    REQUIRE(childs1.front() == arith1);
    REQUIRE(childs1.back() == arith2);

    const std::list<AstNode *> childs2 = PathFinder::getListOfChilds(arith1);
    REQUIRE(childs2.front() == int1);
    REQUIRE(childs2.back() == int2);

    const std::list<AstNode *> childs3 = PathFinder::getListOfChilds(arith2);
    REQUIRE(childs3.front() == int3);
    REQUIRE(childs3.back() == int4);

    // check pathFinder::findPath
    PathFinder pathFinder(1);

    pathFinder.findPath(arith);
    cout << pathFinder;

    PathFinder pathFinder2(2);
    pathFinder2.findPath(arith);
    cout << pathFinder2;

    PathFinder pathFinder3(3);
    pathFinder3.findPath(arith);
    cout << pathFinder3;

    PathFinder pathFinder4(4);
    pathFinder4.findPath(arith);
    cout << pathFinder4;

    PathFinder pathFinderU(3);
    pathFinderU.findPath(aUnb);
    cout << pathFinderU;

    // 1-ary pathFinder
    REQUIRE(pathFinder.getPaths().size() == 1);
    REQUIRE(pathFinder.getPaths()[0].size() == 1);
    REQUIRE(pathFinder.getPaths()[0][0] == arith);

    // 2-ary pathFinder
    REQUIRE(pathFinder2.getPaths().size() == 2);
    REQUIRE(pathFinder2.getPaths()[0].size() == 2);
    REQUIRE(pathFinder2.getPaths()[1].size() == 2);

    REQUIRE(pathFinder2.getPaths()[0][0] == arith);
    REQUIRE(pathFinder2.getPaths()[0][1] == arith1);

    REQUIRE(pathFinder2.getPaths()[1][0] == arith);
    REQUIRE(pathFinder2.getPaths()[1][1] == arith2);

    // 3-ary pathFinder
    REQUIRE(pathFinder3.getPaths().size() == 4);
    for (int i = 0 ; i< 3; i++){
        REQUIRE(pathFinder3.getPaths()[i].size() == 3);
    }

    REQUIRE(pathFinder3.getPaths()[0][0] == arith);
    REQUIRE(pathFinder3.getPaths()[1][0] == arith);
    REQUIRE(pathFinder3.getPaths()[2][0] == arith);
    REQUIRE(pathFinder3.getPaths()[3][0] == arith);

    REQUIRE(pathFinder3.getPaths()[0][1] == arith1);
    REQUIRE(pathFinder3.getPaths()[1][1] == arith1);
    REQUIRE(pathFinder3.getPaths()[2][1] == arith2);
    REQUIRE(pathFinder3.getPaths()[3][1] == arith2);

    REQUIRE(pathFinder3.getPaths()[0][2] == int1);
    REQUIRE(pathFinder3.getPaths()[1][2] == int2);
    REQUIRE(pathFinder3.getPaths()[2][2] == int3);
    REQUIRE(pathFinder3.getPaths()[3][2] == int4);

    // 4-ary pathFinder
    REQUIRE(pathFinder4.getPaths().size() == 0);

    // 3-ary pathFinder on unbalanced tree
    REQUIRE(pathFinderU.getPaths()[0].size() == 3);

    REQUIRE(pathFinderU.getPaths()[0][0] == aUnb);
    REQUIRE(pathFinderU.getPaths()[1][0] == aUnb);
    REQUIRE(pathFinderU.getPaths()[0][1] == arith1);
    REQUIRE(pathFinderU.getPaths()[1][1] == arith1);
    REQUIRE(pathFinderU.getPaths()[0][2] == int1);
    REQUIRE(pathFinderU.getPaths()[1][2] == int2);
}
