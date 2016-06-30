// FM. MA

#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <cerrno>
#include <limits>
#include <sstream>
#include <vector>
#include <map>
#include <iostream>
#include <math.h>

// code base specific includes
#include "vhdlpp_config.h"
#include "generate_graph.h"
#include "version_base.h"
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
#include "traverse_all.h"
#include "vtype.h"
#include "std_types.h"
#include "std_funcs.h"
#include "parse_context.h"

// testing library
#define CATCH_CONFIG_MAIN
#include "catch.h"

bool verbose_flag = false;
// Where to dump design entities
const char *work_path = "ivl_vhdl_work";
const char *dump_design_entities_path = 0;
const char *dump_libraries_path       = 0;
const char *debug_log_path            = 0;

bool     debug_elaboration = false;
ofstream debug_log_file;

TEST_CASE("Simple block", "[ast]"){
    int rc;

    StandardTypes *std_types = (new StandardTypes())->init();
    StandardFunctions *std_funcs = (new StandardFunctions())->init();
    ParserContext *context = (new ParserContext(std_types, std_funcs))->init();

    rc = ParserUtil::parse_source_file("vhdl_testfiles/block_simple.vhd", perm_string(), context);

    REQUIRE(rc == 0);
    REQUIRE(context->parse_errors == 0);
    REQUIRE(context->parse_sorrys == 0);
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
    REQUIRE(context->parse_errors  == 0);
    REQUIRE(context->parse_errors  == 0);

    REQUIRE(context->design_entities.size() == 1);

    auto iterator = context->design_entities.begin();
    auto entity1 = iterator->second;
    REQUIRE(entity1 != NULL);

    auto entity2 = iterator->second->clone();
    REQUIRE(entity2 != NULL);

    stringstream a();
    stringstream b();

    entity1.emit_strinfo_tree(a, "tree", );
}
