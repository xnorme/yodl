// FM. MA
// This is just a simple test suite so I
// don't care about deleting parsers or
// parser contexts
////
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <iostream>

// YOSYS specific headers
#include <kernel/yosys.h>
#include <kernel/rtlil.h>
#include <backends/ilang/ilang_backend.h>

// code base specific includes
#include "generate_graph.h"
#include "simple_tree.h"
#include "StringHeap.h"
#include "entity.h"
#include "compiler.h"
#include "sequential.h"
#include "library.h"
#include "stateful_lambda.h"
#include "std_funcs.h"
#include "std_types.h"
#include "architec.h"
#include "parse_api.h"
#include "generic_traverser.h"
#include "vtype.h"
#include "std_types.h"
#include "std_funcs.h"
#include "parse_context.h"
#include "root_class.h"
#include "mach7_includes.h"
#include "path_finder.h"
#include "predicate_generators.h"
#include "signal_extractor.h"
#include "elsif_eliminator.h"
#include "clock_edge_recognizer.h"
#include "csa_lifter.h"

#include <CppUTest/TestHarness.h>

TEST_GROUP(DefaultGroup){};

TEST(DefaultGroup, TypePredicateMetaFunctionsTest){
    ExpInteger *int1 = new ExpInteger(100);
    ExpString *str = new ExpString("fnord");
    ExpReal *real = new ExpReal(0.0123);

    function<bool (const AstNode *)> e1 =
        makeNaryTypePredicate<ExpInteger, ExpString>();

    CHECK(e1(int1) == true);
    CHECK(e1(str) == true);
    CHECK(e1(real) == false);

    delete int1;
    delete str;
    delete real;
}

TEST(DefaultGroup,Typepredicatecombinatorstest){
    ExpInteger *int1 = new ExpInteger(100);
    ExpString *str = new ExpString("fnord");
    ExpReal *real = new ExpReal(0.0123);

    function<bool (const AstNode *)> t1 =
        makeNaryTypePredicate<ExpInteger>();

    function<bool (const AstNode *)> t2 =
        makeNaryTypePredicate<ExpString>();

    auto e1 = t1 || t2;
    auto e2 = t1 && t2;

    CHECK(e1(int1) == true);
    CHECK(e1(str) == true);
    CHECK(e1(real) == false);

    // neither int1, str or real inherits
    // from both ExpInteger and ExpString
    CHECK(e2(int1) == false);
    CHECK(e2(str) == false);
    CHECK(e2(real) == false);

    auto e3 = ! e2;
    CHECK(e3(int1) == true);
    CHECK(e3(str) == true);
    CHECK(e3(real) == true);

    delete int1;
    delete str;
    delete real;
}

// this test case demponstrates the use of cliffords
// RTLIL API. The cells get wired up to form a full adder.
TEST(DefaultGroup,YosysRTLILconstruction){
    using namespace Yosys::RTLIL;
    using namespace Yosys::ILANG_BACKEND;

    Yosys::log_streams.push_back(&std::cout);
    Yosys::log_error_stderr = true;

    Design *design = new Design();
    Module *module = new Module();

    module->name = "\\testmod";
    design->add(module);

    Wire *cin = module->addWire("\\cin", 1);
    Wire *cout = module->addWire("\\cout", 1);
    Wire *a0 = module->addWire("\\a0", 1);
    Wire *a1 = module->addWire("\\a1", 1);
    Wire *b0 = module->addWire("\\b0", 1);
    Wire *b1 = module->addWire("\\b1", 1);

    Wire *xor1f_outT = module->addWire("\\xor1foutT", 1);
    Wire *xor1f_out = module->addWire("\\xor1fout", 1);

    Cell *xor1f = module->addCell("\\xor1f", "$xor");
    Cell *xor2f = module->addCell("\\xor2f", "$xor");

    xor1f->setPort("\\A", cin);
    xor1f->setPort("\\B", a0);
    xor1f->setPort("\\Y", xor1f_outT);

    xor2f->setPort("\\A", xor1f_outT);
    xor2f->setPort("\\B", b0);
    xor2f->setPort("\\Y", xor1f_out);

    xor1f->fixup_parameters();
    xor2f->fixup_parameters();

    Cell *and11 = module->addCell("\\and11", "$and");
    Cell *and12 = module->addCell("\\and12", "$and");
    Cell *and13 = module->addCell("\\and13", "$and");

    Wire *and11_out = module->addWire("\\and11out", 1);
    Wire *and12_out = module->addWire("\\and12out", 1);
    Wire *and13_out = module->addWire("\\and13out", 1);

    and11->setPort("\\A", a0);
    and11->setPort("\\B", b0);
    and11->setPort("\\Y", and11_out);

    and12->setPort("\\A", a0);
    and12->setPort("\\B", cin);
    and12->setPort("\\Y", and12_out);

    and13->setPort("\\A", a0);
    and13->setPort("\\B", cin);
    and13->setPort("\\Y", and13_out);

    and11->fixup_parameters();
    and12->fixup_parameters();
    and13->fixup_parameters();

    Cell *or1f = module->addCell("\\or1f", "$or");
    Cell *or2f = module->addCell("\\or2f", "$or");

    Wire *or1f_outT = module->addWire("\\or1foutT", 1);
    Wire *or1f_out = module->addWire("\\or1fout", 1);

    or1f->setPort("\\A", and11_out);
    or1f->setPort("\\B", and12_out);
    or1f->setPort("\\Y", or1f_outT);

    or2f->setPort("\\A", or1f_outT);
    or2f->setPort("\\B", and13_out);
    or2f->setPort("\\Y", or1f_out);

    or1f->fixup_parameters();
    or2f->fixup_parameters();

    stringstream ilangBuffer;
    dump_design(ilangBuffer, design, false);

//    std::cout << ilangBuffer.str();

    CHECK(ilangBuffer.str() != "");
}


TEST(DefaultGroup,Simpleblock){
    int rc;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                       perm_string(), context);

    CHECK(rc == 0);
    CHECK(context->parse_errors == 0);
    CHECK(context->parse_sorrys == 0);
}

TEST(DefaultGroup,Simpleforloop){
    int rc;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file(
        "vhdl_testfiles/for_loop_simple.vhd",
        perm_string(), context);

    CHECK(rc == 0);
    CHECK(context->parse_errors == 0);
    CHECK(context->parse_sorrys == 0);

    CHECK(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    auto entity1 = iterator->second;

    //emit_dotgraph(std::cout, "foo", entity1->emit_strinfo_tree());
}

TEST(DefaultGroup,Multipleparses){
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

    CHECK(rc1 == 0);
    CHECK(rc2 == 0);
    CHECK(context->parse_errors  == 0);
    CHECK(context->parse_errors  == 0);
    CHECK(context1->parse_sorrys == 0);
    CHECK(context1->parse_sorrys == 0);
}

TEST(DefaultGroup,Simpleclonetest){
    int rc;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                       perm_string(), context);

    CHECK(rc == 0);
    CHECK(rc == 0);
    CHECK(context->parse_errors  == 0);
    CHECK(context->parse_errors  == 0);

    CHECK(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    CHECK(iterator->second != NULL);

    auto cloned_entity = iterator->second->clone();
    CHECK(cloned_entity != NULL);
}

TEST(DefaultGroup,Simpleclonetestwithdotgeneration){
    int rc;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                       perm_string(), context);

    CHECK(rc == 0);
    CHECK(rc == 0);
    CHECK(context->parse_errors == 0);
    CHECK(context->parse_errors == 0);

    CHECK(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    auto entity1 = iterator->second;
    CHECK(entity1 != NULL);

    auto entity2 = iterator->second->clone();
    CHECK(entity2 != NULL);

    stringstream a{};
    stringstream b{};

    auto tree1 = entity1->emit_strinfo_tree();
    auto tree2 = entity2->emit_strinfo_tree();

    DotGraphGenerator()(a, "foo", entity1->emit_strinfo_tree());
    DotGraphGenerator()(b, "foo", entity2->emit_strinfo_tree());

    CHECK((*tree1 == *tree2) == true);
    CHECK(a.str() == b.str());
}

TEST(DefaultGroup,Testequalityofsimpletree){
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

    CHECK((*tree1 == *tree2) == true);

    tree1->root["bar"] = "fnord";
    CHECK((*tree1 == *tree2) == false);
}

TEST(DefaultGroup,Testsimplegenerictraversal){
    using namespace mch;

    int rc;
    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                       perm_string(), context);

    CHECK(rc == 0);
    CHECK(rc == 0);
    CHECK(context->parse_errors == 0);
    CHECK(context->parse_errors == 0);

    CHECK(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    auto entity1 = iterator->second;
    CHECK(entity1 != NULL);

    const AstNode *root = entity1;

    StatefulLambda<int> state = StatefulLambda<int>(
        0,
        static_cast<function <int (const AstNode *, int &)>>(
        [](const AstNode *, int &env) -> int {
            cout << "[VISITOR] Found node!"  << endl;
            env++;
            return 0;
        }));

    GenericTraverser traverser(
        makeTypePredicate<BlockStatement>(),
        static_cast<function<int (const AstNode *)>>(
            [&state](const AstNode *a) -> int { return state(a); }),
        GenericTraverser::RECUR);

    traverser(root);
    CHECK(state.environment == 2);
}

TEST(DefaultGroup,GenericTraverserclassconstructortest){
    using namespace mch;

    GenericTraverser traverserConst(
        [=](const AstNode *node){
            Match(node){
                Case(C<BlockStatement>()){ return true; }
                Otherwise(){ return false; }
            } EndMatch;
            return false; //without: compiler warning
        },
        static_cast<function<int (const AstNode *)>>(
            [](const AstNode *) -> int { return 0; }),
        GenericTraverser::RECUR);

    CHECK(traverserConst.isNaryTraverser() == false);
    CHECK(traverserConst.wasError() == false);
    CHECK(traverserConst.isMutatingTraverser() == false);

    GenericTraverser traverserMutating(
        [=](const AstNode *node){
            Match(node){
                Case(C<BlockStatement>()){ return true; }
                Otherwise(){ return false; }
            } EndMatch;
            return false; //without: compiler warning
        },
        static_cast<function<int (AstNode *)>>([](AstNode *) -> int {
                return 0; }),
        GenericTraverser::RECUR);

    CHECK(traverserMutating.isNaryTraverser() == false);
    CHECK(traverserMutating.wasError() == false);
    CHECK(traverserMutating.isMutatingTraverser() == true);

    GenericTraverser traverserNary(
        [=](const AstNode *node){
            Match(node){
                Case(C<BlockStatement>()){ return true; }
                Otherwise(){ return false; }
            } EndMatch;
            return false; //without: compiler warning
        },
        static_cast<function<int (
            const AstNode *, const std::vector<const AstNode *> &)>>(
                [](const AstNode *, const std::vector<const AstNode *> &)
                -> int { return 0; }),
        GenericTraverser::RECUR);

    CHECK(traverserNary.isNaryTraverser() == true);
    CHECK(traverserNary.wasError() == false);
    CHECK(traverserNary.isMutatingTraverser() == false);

    GenericTraverser traverserNaryMutating(
        [=](const AstNode *node){
            Match(node){
                Case(C<BlockStatement>()){ return true; }
                Otherwise(){ return false; }
            } EndMatch;
            return false; //without: compiler warning
        },
        static_cast<function<int (
            AstNode *, const std::vector<AstNode *> &)>>(
                [](AstNode *, const std::vector<AstNode *> &)
                -> int { return 0; }),
        GenericTraverser::RECUR);

    CHECK(traverserNaryMutating.isNaryTraverser() == true);
    CHECK(traverserNaryMutating.wasError() == false);
    CHECK(traverserNaryMutating.isMutatingTraverser() == true);

}

template<typename T> void f(T);

TEST(DefaultGroup,TestsimplegenerictraversalonclonedAST){
    using namespace mch;

    int rc;
    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                       perm_string(), context);

    CHECK(rc == 0);
    CHECK(rc == 0);
    CHECK(context->parse_errors == 0);
    CHECK(context->parse_errors == 0);

    CHECK(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    auto entity1 = iterator->second;
    CHECK(entity1 != NULL);

    const AstNode *root = entity1->clone();

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
            [&state](const AstNode *a) -> int {
                return state(a); }),
        GenericTraverser::RECUR);

    traverser(root);
    CHECK(state.environment == 2);
    traverser.emitTraversalMessages(cout, "\n");
    traverser.emitErrorMessages(cout, "\n");
}

TEST(DefaultGroup,Testpathfinder){
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
    const std::list<AstNode *> childs1 =
        PathFinder::getListOfChilds(static_cast<AstNode *>(arith));
    CHECK(childs1.front() == arith1);
    CHECK(childs1.back() == arith2);

    const std::list<AstNode *> childs2 =
        PathFinder::getListOfChilds(static_cast<AstNode *>(arith1));
    CHECK(childs2.front() == int1);
    CHECK(childs2.back() == int2);

    const std::list<AstNode *> childs3 =
        PathFinder::getListOfChilds(static_cast<AstNode *>(arith2));
    CHECK(childs3.front() == int3);
    CHECK(childs3.back() == int4);

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
    CHECK(pathFinder.getPaths().size() == 1);
    CHECK(pathFinder.getPaths()[0].size() == 1);
    CHECK(pathFinder.getPaths()[0][0] == arith);

    // 2-ary pathFinder
    CHECK(pathFinder2.getPaths().size() == 2);
    CHECK(pathFinder2.getPaths()[0].size() == 2);
    CHECK(pathFinder2.getPaths()[1].size() == 2);

    CHECK(pathFinder2.getPaths()[0][0] == arith);
    CHECK(pathFinder2.getPaths()[0][1] == arith1);

    CHECK(pathFinder2.getPaths()[1][0] == arith);
    CHECK(pathFinder2.getPaths()[1][1] == arith2);

    // 3-ary pathFinder
    CHECK(pathFinder3.getPaths().size() == 4);
    for (int i = 0 ; i< 3; i++){
        CHECK(pathFinder3.getPaths()[i].size() == 3);
    }

    CHECK(pathFinder3.getPaths()[0][0] == arith);
    CHECK(pathFinder3.getPaths()[1][0] == arith);
    CHECK(pathFinder3.getPaths()[2][0] == arith);
    CHECK(pathFinder3.getPaths()[3][0] == arith);

    CHECK(pathFinder3.getPaths()[0][1] == arith1);
    CHECK(pathFinder3.getPaths()[1][1] == arith1);
    CHECK(pathFinder3.getPaths()[2][1] == arith2);
    CHECK(pathFinder3.getPaths()[3][1] == arith2);

    CHECK(pathFinder3.getPaths()[0][2] == int1);
    CHECK(pathFinder3.getPaths()[1][2] == int2);
    CHECK(pathFinder3.getPaths()[2][2] == int3);
    CHECK(pathFinder3.getPaths()[3][2] == int4);

    // 4-ary pathFinder
    CHECK(pathFinder4.getPaths().size() == 0);

    // 3-ary pathFinder on unbalanced tree
    CHECK(pathFinderU.getPaths()[0].size() == 3);

    CHECK(pathFinderU.getPaths()[0][0] == aUnb);
    CHECK(pathFinderU.getPaths()[1][0] == aUnb);
    CHECK(pathFinderU.getPaths()[0][1] == arith1);
    CHECK(pathFinderU.getPaths()[1][1] == arith1);
    CHECK(pathFinderU.getPaths()[0][2] == int1);
    CHECK(pathFinderU.getPaths()[1][2] == int2);
}

TEST(DefaultGroup,NONRECURtest){
    ExpInteger *int1 = new ExpInteger(100);
    ExpInteger *int2 = new ExpInteger(101);
    ExpInteger *int3 = new ExpInteger(102);
    ExpInteger *int4 = new ExpInteger(103);

    ExpArithmetic *arith1 = new ExpArithmetic(ExpArithmetic::PLUS, int1, int2);
    ExpArithmetic *arith2 = new ExpArithmetic(ExpArithmetic::PLUS, int3, int4);

    ExpArithmetic *arith = new ExpArithmetic(ExpArithmetic::MULT, arith1, arith2);

    StatefulLambda<int> stateLambda(
        0, static_cast<function <int (AstNode *, int &)>>(
            [](AstNode *, int &env) -> int {
                env++; return 0; }));

    GenericTraverser traverser(
        makeTypePredicate<ExpArithmetic>(),
        static_cast<function <int (AstNode *)>>(
            [&stateLambda](AstNode *n) -> int {
                stateLambda(n); return 0; }),
        GenericTraverser::NONRECUR);

    traverser(arith);

    CHECK(stateLambda.environment == 1);
    stateLambda.reset();
    CHECK(stateLambda.environment == 0);
}

TEST(DefaultGroup,RECURtest){
    ExpInteger *int1 = new ExpInteger(100);
    ExpInteger *int2 = new ExpInteger(101);
    ExpInteger *int3 = new ExpInteger(102);
    ExpInteger *int4 = new ExpInteger(103);

    ExpArithmetic *arith1 = new ExpArithmetic(ExpArithmetic::PLUS, int1, int2);
    ExpArithmetic *arith2 = new ExpArithmetic(ExpArithmetic::PLUS, int3, int4);

    ExpArithmetic *arith = new ExpArithmetic(ExpArithmetic::MULT, arith1, arith2);

    StatefulLambda<int> stateLambda(
        0, static_cast<function <int ( AstNode *, int &)>>(
            []( AstNode *, int &env) -> int {
                env++; return 0; }));

    GenericTraverser traverser(
        makeTypePredicate<ExpArithmetic>(),
        static_cast<function <int ( AstNode *)>>(
            [&stateLambda]( AstNode *n) -> int {
                stateLambda(n); return 0; }),
        GenericTraverser::RECUR);

    traverser(arith);

    CHECK(stateLambda.environment == 3);
    stateLambda.reset();
    CHECK(stateLambda.environment == 0);
}

TEST(DefaultGroup,Testnarytraverser){
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

    struct functor_t {
        bool alreadyCalled = false;

        AstNode *i1;
        AstNode *i2;
        AstNode *i3;
        AstNode *i4;
        AstNode *ar;
        AstNode *ar1;
        AstNode *ar2;
        AstNode *au;

        functor_t(AstNode *a, AstNode *b,
                  AstNode *c, AstNode *d,
                  AstNode *e, AstNode *f,
                  AstNode *g, AstNode *h)
            : i1(a), i2(b), i3(c), i4(d)
            , ar(e), ar1(f), ar2(g), au(h) {}

        int operator()(AstNode *node, const std::vector<AstNode *> &parents){
            if (node == i1){
                CHECK(parents[0] == ar1);
                CHECK(parents[1] == ar);
            }

            if (node == i2){
                CHECK(parents[0] == ar1);
                CHECK(parents[1] == ar);
            }

            if (node == i3) {
                CHECK(parents[0] == ar2);
                CHECK(parents[1] == ar);
            }

            if (node == i4) {
                CHECK(parents[0] == ar2);
                CHECK(parents[1] == ar);
            }

            return 0;
        }
    };

    GenericTraverser traverserNaryMutating(
        makeTypePredicate<ExpInteger>(),
        functor_t(int1, int2, int3, int4, arith, arith1, arith2, aUnb),
        GenericTraverser::RECUR);

    traverserNaryMutating(arith);
}

TEST(DefaultGroup,Higherordertraverser){
    int rc;
    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd",
                                       perm_string(), context);

    CHECK(rc == 0);
    CHECK(rc == 0);
    CHECK(context->parse_errors == 0);
    CHECK(context->parse_errors == 0);

    CHECK(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    auto entity1 = iterator->second;
    CHECK(entity1 != NULL);

    struct counter_t {
        int count;

        int operator()(const AstNode *n){
            const SignalAssignment *sig = dynamic_cast<const SignalAssignment*>(n);
            count += sig->rval_.size();
            return 0;
        }
    };

    GenericTraverser trav(
        makeTypePredicate<BlockStatement>(),
        static_cast<function<int (const AstNode *)>>(
        GenericTraverser(
            [](const AstNode *n) -> bool {
                Match(n){ Case(mch::C<SignalAssignment>()){ return true; } }
                EndMatch; return false;
            },
            static_cast<function<int (const AstNode *)>>(counter_t()),
            GenericTraverser::NONRECUR)),
        GenericTraverser::NONRECUR);
}

TEST(DefaultGroup,Parsertestoperatorsymbol){
    int rc1;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc1 = ParserUtil::parse_source_file(
        "vhdl_testfiles/parser_test_operator_symbol.vhd",
        perm_string(), context);

    CHECK(rc1 == 0);
    CHECK(context->parse_errors  == 0);
    CHECK(context->parse_errors  == 0);


    GenericTraverser trav(
        makeTypePredicate<ExpName>(),
        [](AstNode *n) -> int {
            ExpName *temp = dynamic_cast<ExpName*>(n);
            if (temp){
                CHECK(temp->is_operator_symbol_ == true);
            } else {
                std::cout << "Nullpointer in test [parser]" << endl;
            }
            return 0;
        },
        GenericTraverser::NONRECUR);

}

TEST(DefaultGroup,Signalextractionsimpletest){
    int rc1;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc1 = ParserUtil::parse_source_file(
        "vhdl_testfiles/signal_extractor_test.vhd",
        perm_string(), context);

    CHECK(rc1 == 0);
    CHECK(context->parse_errors  == 0);
    CHECK(context->parse_errors  == 0);

    auto iterator = context->design_entities.begin();
    auto entity = iterator->second;
    CHECK(entity != NULL);

    SignalExtractor extractor;

    GenericTraverser traverser(
        makeNaryTypePredicate<Entity, ScopeBase,
                              ExpName, ExpFunc,
                              ExpAggregate>(),
        static_cast<function<int (const AstNode *)>>(
            [&extractor](const AstNode *n) -> int { return extractor(n);}),
        GenericTraverser::RECUR);

    CHECK(traverser.isMutatingTraverser() == false);

    traverser(entity);

//    DotGraphGenerator()(entity->emit_strinfo_tree());
//    DotGraphGenerator()("graph foo", entity->emit_strinfo_tree());

    CHECK(extractor.signals.size() == 2);

    auto sIter = extractor.signals.begin();
    CHECK(dynamic_cast<const Signal *>(*sIter)->name_ ==
            perm_string::literal("f"));
    ++sIter;
    CHECK(dynamic_cast<const Signal *>(*sIter)->name_ ==
            perm_string::literal("foo"));
}

TEST(DefaultGroup,ElsifEliminatortest){
    int rc1;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc1 = ParserUtil::parse_source_file(
        "vhdl_testfiles/elsif_eliminator_nested_test.vhd",
        perm_string(), context);

    CHECK(rc1 == 0);
    CHECK(context->parse_errors  == 0);
    CHECK(context->parse_errors  == 0);

    auto iterator = context->design_entities.begin();
    auto entity = iterator->second;
    CHECK(entity != NULL);

    ElsifEliminator elsifEliminator;

    GenericTraverser traverser(
        makeTypePredicate<IfSequential>(),
        static_cast<function<int (AstNode *)>>(elsifEliminator),
        GenericTraverser::RECUR);


    StatefulLambda<int> cnt(
        0, static_cast<function<int (AstNode *, int &)>>(
            [](AstNode *, int &env) -> int { env++; return 0; }));

    GenericTraverser counter(
        makeTypePredicate<IfSequential>(),
        [&cnt](AstNode *n) -> int { return cnt(n); },
        GenericTraverser::RECUR);

    traverser(entity);
    counter(entity);

    // I manually counted the if statements in the desugared AST...
    CHECK(cnt.environment == 8);
}

TEST(DefaultGroup,Simplecsaliftertest){
    int rc1;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc1 = ParserUtil::parse_source_file(
        "vhdl_testfiles/process_lifting_test.vhd",
        perm_string(), context);

    CHECK(rc1 == 0);
    CHECK(context->parse_errors  == 0);
    CHECK(context->parse_errors  == 0);

    auto iterator = context->design_entities.begin();
    auto entity = iterator->second;
    CHECK(entity != NULL);

    CsaLifter lifter;

    GenericTraverser traverser(
        makeNaryTypePredicate<Architecture, Entity, BlockStatement>(),
        lifter,
        GenericTraverser::RECUR);

    traverser(entity);

    StatefulLambda<int> cnt(
        0, static_cast<function<int (const AstNode *, int &)>>(
            [](const AstNode *, int &env) -> int { env++; return 0; }));

    GenericTraverser counter(
        makeNaryTypePredicate<ProcessStatement, WaitStmt>(),
        static_cast<function<int(const AstNode *)>>(
            [&cnt](const AstNode *n) -> int { return cnt(n); }),
        GenericTraverser::RECUR);

    counter(entity);

    // There must be 3 processes containing 3 wait statements
    CHECK(cnt.environment == 6);
}

TEST(DefaultGroup,Nestedblockscsaliftertest){
    int rc1;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc1 = ParserUtil::parse_source_file(
        "vhdl_testfiles/process_lifting_test_blocks.vhd",
        perm_string(), context);

    CHECK(rc1 == 0);
    CHECK(context->parse_errors  == 0);
    CHECK(context->parse_errors  == 0);

    auto iterator = context->design_entities.begin();
    auto entity = iterator->second;
    CHECK(entity != NULL);

    CsaLifter lifter;

    GenericTraverser traverser(
        makeNaryTypePredicate<Architecture, Entity, BlockStatement>(),
        lifter,
        GenericTraverser::RECUR);

    traverser(entity);

    StatefulLambda<int> cnt(
        0, static_cast<function<int (const AstNode *, int &)>>(
            [](const AstNode *, int &env) -> int { env++; return 0; }));

    GenericTraverser counter(
        makeNaryTypePredicate<ProcessStatement, WaitStmt>(),
        static_cast<function<int(const AstNode *)>>(
            [&cnt](const AstNode *n) -> int { return cnt(n); }),
        GenericTraverser::RECUR);

    counter(entity);

    // There must be 3 processes containing 3 wait statements
    CHECK(cnt.environment == 12);

    GenericTraverser counter2(
        makeNaryTypePredicate<BlockStatement>(),
        static_cast<function<int(const AstNode *)>>(
            [&cnt](const AstNode *n) -> int { return cnt(n); }),
        GenericTraverser::RECUR);

    cnt.reset();
    counter2(entity);

    CHECK(cnt.environment == 2);

    return ;
}
