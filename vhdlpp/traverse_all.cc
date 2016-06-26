#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <cerrno>
#include <limits>
#include <vector>
#include <map>
#include <iostream>
#include <math.h>

// mach7 setup
#include <type_switchN-patterns.hpp> // Support for N-ary Match statement on patterns
#include <address.hpp>      // Address and dereference combinators
#include <bindings.hpp>     // Mach7 support for bindings on arbitrary UDT
#include <constructor.hpp>  // Support for constructor patterns
#include <equivalence.hpp>  // Equivalence combinator +

// code base specific includes
#include "vhdlpp_config.h"
#include "generate_graph.h"
#include "version_base.h"
#include "simple_tree.h"
#include "StringHeap.h"
#include "compiler.h"
#include "sequential.h"
#include "library.h"
#include "std_funcs.h"
#include "std_types.h"
#include "architec.h"
#include "parse_api.h"
#include "traverse_all.h"
#include "vtype.h"
#if defined(HAVE_GETOPT_H)
# include <getopt.h>
#endif
// MinGW only supports mkdir() with a path. If this stops working because
// we need to use _mkdir() for mingw-w32 and mkdir() for mingw-w64 look
// at using the autoconf AX_FUNC_MKDIR macro to figure this all out.
#if defined(__MINGW32__)
# include <io.h>
# define mkdir(path, mode)    mkdir(path)
#endif

/*
// tag: [DESIGN HIERARCHY]
// template specializiation for desing hierarchies
namespace mch {
    template <> struct bindings<Architecture> {
        Members(Architecture::statements_,
                Architecture::name_);
    };

    template <> struct bindings<Entity> {
        Members(Entity::arch_, Entity::name_);
    };
}

// tag: [CONCURRENT STATEMENT]
// template specializations for the Architecture::Statemet class tree
namespace mch {
    template <> struct bindings<Architecture::Statement> {

    };

    template <> struct bindings<GenerateStatement> {
        Members(GenerateStatement::name_,
                GenerateStatement::statements_);
    };

    template <> struct bindings<ForGenerate> {
        Members(ForGenerate::genvar_,
                ForGenerate::msb_,
                ForGenerate::lsb_);
    };

    template <> struct bindings<IfGenerate> {
        Members(IfGenerate::cond_);
    };

    template <> struct bindings<SignalAssignment> {
        Members(SignalAssignment::lval_,
                SignalAssignment::rval_);
    };

    template <> struct bindings<BlockStatement> {
        Members(BlockStatement::label_,
                BlockStatement::header_,
                BlockStatement::concurrent_stmts_);
    };
};

// tag: [EXPRESSION]
// template specializations for the Expression class tree
namespace mch {
    template <> struct bindings<Expression> {
        Members(Expression::type_);
    };

    template <> struct bindings<ExpUnary> {
        Members(ExpUnary::operand1_);
    };

    template <> struct bindings<ExpBinary> {
        Members(ExpBinary::operand1_,
                ExpBinary::operand2_);
    };

    template <> struct bindings<ExpAggregate> {
        Members(ExpAggregate::elements_,
                ExpAggregate::aggregate_)
    };

    template <> struct bindings<ExpAggregate::element_t> {
        Members(ExpAggregate::element_t::fields_,
                ExpAggregate::element_t::val_)
    };

    template <> struct bindings<ExpAggregate::choice_element> {
        Members(ExpAggregate::choice_element::choice,
                ExpAggregate::choice_element::expr,
                ExpAggregate::choice_element::alias_flag);
    };

    template <> struct bindings<ExpAggregate::choice_t> {
        Members(ExpAggregate::choice_t::expr_,
                ExpAggregate::choice_t::range_);
    };

    template <> struct bindings<ExpConcat> {
        Members(ExpConcat::operand1_,
                ExpConcat::operand2_);
    };

    template <> struct bindings<ExpDelay> {
        Members(ExpDelay::expr_,
                ExpDelay::delay_);
    };

    template <> struct bindings<ExpNew> {
        Members(ExpNew::size_);
    };

    template <> struct bindings<ExpCast> {
        Members(ExpCast::base_,
                ExpCast::type_);
    };
    //template <> struct bindings<ExpUNot> {}; not acutally needed
    //template <> struct bindings<ExpUAbs> {}; not acutally needed

    template <> struct bindings<ExpRange> {
        Members(ExpRange::left_,
                ExpRange::right_,
                ExpRange::direction_,
                ExpRange::range_expr_,
                ExpRange::range_base_,
                ExpRange::range_reverse_);
    };

    template <> struct bindings<ExpTime> {
        Members(ExpTime::amount_,
                ExpTime::unit_);
    };

    template <> struct bindings<ExpArithmetic> {
        Members(ExpArithmetic::fun_);
    };

    template <> struct bindings<ExpCharacter> {
        Members(ExpCharacter::value_);
    };

    template <> struct bindings<ExpBitstring> {
        Members(ExpBitstring::value_);
    };

    template <> struct bindings<ExpAttribute> {
        Members(ExpAttribute::name_,
                ExpAttribute::args_);
    };

    template <> struct bindings<ExpTypeAttribute> {
        Members(ExpTypeAttribute::base_);
    };

    template <> struct bindings<ExpObjAttribute> {
        Members(ExpObjAttribute::base_);
    };

    template <> struct bindings<ExpLogical> {
        Members(ExpLogical::fun_);
    };

    template <> struct bindings<ExpInteger> {
        Members(ExpInteger::value_);
    };

    template <> struct bindings<ExpReal> {
        Members(ExpReal::value_);
    };

    template <> struct bindings<ExpEdge> {
        Members(ExpEdge::fun_);
    };

    template <> struct bindings<ExpSelected> {
        Members(ExpSelected::selector_);
    };

    template <> struct bindings<ExpConditional> {
        Members(ExpConditional::options_);
    };

    template <> struct bindings<ExpConditional::case_t> {
        Members(ExpConditional::case_t::cond_,
                ExpConditional::case_t::true_clause_);
    };

    template <> struct bindings<ExpFunc> {
        Members(ExpFunc::name_,
                ExpFunc::argv_,
                ExpFunc::def_);
    };

    template <> struct bindings<ExpName> {
        Members(ExpName::prefix_,
                ExpName::name_,
                ExpName::indices_);
    };

    template <> struct bindings<ExpString> {
        Members(ExpString::value_);
    };

    template <> struct bindings<ExpShift> {
        Members(ExpShift::shift_);
    };

    template <> struct bindings<ExpScopedName> {
        Members(ExpScopedName::scope_name_,
                ExpScopedName::scope_,
                ExpScopedName::name_);
    };

    template <> struct bindings<ExpRelation> {
        Members(ExpRelation::fun_);
    };
};

// tag: [SEQUENTIAL STATEMENT]
// template specializations for the Sequential class tree
namespace mch {
    //template <> struct bindings<> {};
};

using namespace mch;

void traverse(const Entity &top){
    var<std::map<perm_string, Architecture *>> archs;
    var<perm_string> name;

    Match(top){
        Case(C<Entity>(archs, name)){
            cout << "Entity detected: " << name << endl;
            for (auto &i : archs)
                traverse(*(i.second));
            return;
        }
        Otherwise() {
            cout << "No Entity!" << endl;
            return;
        }
    } EndMatch
}

void traverse(const Architecture &arch){
    var<std::list<Architecture::Statement *>> stmts;
    var<perm_string> name;

    Match(arch){
        Case(C<Architecture>(stmts, name)){
            cout << "Architecture detected: " << name << endl;
            for (auto &i : stmts)
                traverse(*i);
            return;
        }
        Otherwise(){
            cout << "No Architecture!" << endl;
            return;
        }
    } EndMatch
}

void traverse(const Architecture::Statement &s){
    var<perm_string> name, label;
    var<list<Architecture::Statement*>> stmts;
    var<list<Architecture::Statement*>&> stmts_ptr;
    var<BlockStatement::BlockHeader&> header;
    var<Expression&> cond;
    var<list<Expression*>> rval;
    var<ExpName&> lval;

    Match(s){
        Case(C<BlockStatement>(label, &header, &stmts_ptr)){
            cout << "Found Block statement: " << label << "\n";
            for (auto &i : (list<Architecture::Statement*>&) stmts_ptr){
                traverse(*i);
            }
        }
        Case(C<ForGenerate>(label, stmts)){
            cout << "Found for generatate statement: " << label << "\n";
            for (auto &i : (list<Architecture::Statement*>) stmts)
                traverse(*i);
        }
        Case(C<IfGenerate>(label, stmts, &cond)){
            cout << "found if generate statement: " << name << "\n";
            for (auto &i : (list<Architecture::Statement*>) stmts)
                traverse(*i);
        }
        Case(C<SignalAssignment>(&lval, rval)){
            cout << "Found SignalAssignment" << endl;
        }
        Otherwise() {
            //cout << "Wildcard pattern for traverse(const Arch::Stmt &s)\n";
            return;
        }
    } EndMatch
}
*/