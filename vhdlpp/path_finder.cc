// FM. MA
#include <vector>
#include <map>

////
// code base specific includes
#include "vhdlpp_config.h"
#include "generate_graph.h"
#include "version_base.h"
#include "simple_tree.h"
#include "scope.h"
#include "StringHeap.h"
#include "compiler.h"
#include "sequential.h"
#include "library.h"
#include "std_funcs.h"
#include "std_types.h"
#include "architec.h"
#include "parse_api.h"
#include "generic_traverser.h"
#include "vtype.h"
#include "root_class.h"
#include "mach7_bindings.h"
#include "path_finder.h"

#pragma clang diagnostic ignored "-Wshadow"

int PathFinder::findPath(AstNode *startNode){
    if (!paths.empty()) { paths.clear(); }

    if (arity <= 0){ return 1; }

    if (arity == 1){
        paths = {{startNode}};
        return 0;
    }

    std::vector<AstNode *> accumulator(arity);
    accumulator[0] = startNode;

    return getNaryPaths(1, getListOfChilds(startNode), accumulator);
}

int PathFinder::findPath(const AstNode *){
    if (!paths.empty()) { paths.clear(); }

    return 0;
}

// return as const, so no copy has to be made
const std::vector<std::vector<AstNode *>> PathFinder::getPaths(){
    return paths;
}

std::ostream &operator<<(std::ostream &out, PathFinder &us){
    int pfad = 0, elem = 0;
    out << "Path finder with arity: [" << us.getArity() << "]" << endl;
    out << endl;

    for (auto &i : us.getPaths()){
        out << "Pfad [" << (pfad++) << "] = [";
        for (auto &j : i)
            out << "(" << (elem++) << ") " << j << " ";
        out << "]" << endl;
        elem = 0;
    }

    return out;
}


int PathFinder::getNaryPaths(size_t depth, const std::list<AstNode *> &childs,
                             std::vector<AstNode *> &accumulator){
    if (accumulator.size() != arity){
        return 1;
    }

    if (depth < arity){
        for (auto &i : childs){
            accumulator[depth] = i;
            getNaryPaths(depth + 1, getListOfChilds(i), accumulator);
        }
    } else {
        paths.push_back(accumulator);
    }

    return 0;
}

const std::list<AstNode *> PathFinder::getListOfChilds(Expression *e){
    using namespace mch;

    var<VType *> type;
    var<Expression *> op1, op2;

    var<vector<ExpAggregate::element_t*>> elements;
    var<vector<ExpAggregate::choice_element>> aggregate;

    var<perm_string> attribName;
    var<list<Expression *> *> attribArgs;

    // for ExpBitstring
    var<vector<char>> bitString;

    // for ExpCharacter
    var<char> charValue;

    // For ExpConcat
    var<Expression*> concLeft, concRight;

    // For ExpConditional
    var<list<ExpConditional::case_t*>> condOptions;
    var<Expression*> selector;

    // For ExpFunc
    var<perm_string> funcName;
    var<SubprogramHeader *> definition;
    var<vector<Expression*>> argVector;

    // For ExpInteger
    var<int64_t> intValue;

    // For ExpReal
    var<double> dblValue;

    // For ExpName
    var<perm_string> nameName;
    var<list<Expression*>*> indices;

    //For ExpScopedName
    var<perm_string> scopeName;
    var<ScopeBase *> scope;
    var<ExpName*> scopeNameName;

    // For ExpCast
    var<Expression*> castExp;
    var<const VType *> castType;

    // For ExpRange
    var<Expression*> rangeLeft, rangeRight;
    var<ExpRange::range_dir_t> direction;
    var<bool> rangeExpr, rangeReverse;
    var<ExpName *> rangeBase;

    // For ExpNew
    var<Expression*> newSize;

    // For ExpTime
    var<uint64_t> timeAmount;
    var<ExpTime::timeunit_t> timeUnit;

    // For ExpString
    var<string> strValue;

    // For ExpDelay
    var<Expression *> delayExpr, delayDelay;

    std::list<AstNode *> result;

    if (e == NULL) { return result; }
    Match(e){
        Case(C<ExpUnary>(op1)){
            var<ExpEdge::fun_t> edgeSpec;

            Match(e){
                Case(C<ExpEdge>(edgeSpec)){
                    return result;
                }
                Case(C<ExpUAbs>()){
                    return result;
                }
                Case(C<ExpUNot>()){
                    return result;
                }
            } EndMatch;
        }

        Case(C<ExpBinary>(op1, op2)){

            var<ExpArithmetic::fun_t> arithOp;
            var<ExpLogical::fun_t> logOp;
            var<ExpRelation::fun_t> relOp;
            var<ExpShift::shift_t> shiftOp;

            Match(e){
                Case(C<ExpArithmetic>(arithOp)){
                    return {op1, op2};
                }
                Case(C<ExpLogical>(logOp)){
                    return {op1, op2};
                }
                Case(C<ExpRelation>(relOp)){
                    return {op1, op2};
                }
                Case(C<ExpShift>(shiftOp)){
                    return {op1, op2};
                }
                Otherwise() {
                    return result;
                }
            } EndMatch;
        }

        Case(C<ExpAggregate>(elements, aggregate)){
            for (auto &i : elements)
                result.push_front(i);

            //TODO: Fix the fact that the list contains
            //      values rather that pointers
            /*for (auto &i : aggregate)
              result.push_front(i);*/

            return result;
        }

        Case(C<ExpAttribute>(attribName, attribArgs)){

            var<ExpName *> attribBase;
            var<const VType *> attribTypeBase;

            for (auto &i : *static_cast<std::list<Expression*> *>(attribArgs))
                result.push_front(i);

            Match(e){
                Case(C<ExpObjAttribute>(attribBase)){
                    result.push_back(attribBase);
                    return result;
                }
                Case(C<ExpTypeAttribute>(attribTypeBase)){
                    //attribTypeBase is const so do nothing
                }
                Otherwise(){
                    return result;
                }
            } EndMatch;
        }

        Case(C<ExpBitstring>(bitString)){
            return result;
        }

        Case(C<ExpCharacter>(charValue)){
            return result;
        }

        Case(C<ExpConcat>(concLeft, concRight)){
            return {concLeft, concRight};
        }

        Case(C<ExpConditional>(condOptions)){
            for (auto &i : condOptions)
                result.push_back(i);

            Match(e){
                Case(C<ExpSelected>(selector)){
                    result.push_back(selector);
                    return result;
                }

                Otherwise(){
                    return result;
                }
            } EndMatch;

        }

        Case(C<ExpFunc>(funcName, definition, argVector)){
            for (auto &i : argVector)
                result.push_back(i);
            //TODO: result.push_back(definition);

            return result;
        }

        Case(C<ExpInteger>(intValue)){
            return result;
        }

        Case(C<ExpReal>(dblValue)){
            return result;
        }

        Case(C<ExpName>(nameName, indices)){
            for (auto &i : *static_cast<list<Expression*>*>(indices))
                result.push_back(i);

            Match(e){
                Case(C<ExpNameALL>()){
                    return result;
                }
                Otherwise(){
                    return result;
                }
            } EndMatch;
        }

        Case(C<ExpScopedName>(scopeName, scope, scopeNameName)){
            return {scope, scopeNameName};
        }

        Case(C<ExpString>(strValue)){
        }

        Case(C<ExpCast>(castExp, castType)){
        }

        Case(C<ExpNew>(newSize)){
            return {newSize};
        }

        Case(C<ExpTime>(timeAmount, timeUnit)){
        }

        Case(C<ExpRange>(rangeLeft, rangeRight,
                         /*direction,*/ rangeExpr,
                         rangeBase/*, rangeReverse*/)){
            return {rangeLeft, rangeRight, rangeBase};
        }

        Case(C<ExpDelay>(delayExpr, delayDelay)){
            return {delayExpr, delayDelay};
        }

        Otherwise(){
            return result;
        }
    } EndMatch;

    return result;
}

const std::list<AstNode *> PathFinder::getListOfChilds(Architecture *arch){
    using namespace mch;

    var<list<Architecture::Statement *>> statements;
    var<ComponentInstantiation *> componentInst;
    var<perm_string> name;
    // propably not needed
    //var<ProcessStatement *> currenProcess;

    Match(arch){
        Case(C<Architecture>(statements, componentInst, name)){

            for (auto &i : statements)
                ;

            if (componentInst)
                ;
        }
        Otherwise(){
        }
    } EndMatch;
}

const std::list<AstNode *> PathFinder::getListOfChilds(AstNode *root){
    using namespace mch;

    Match(root){
        Case(C<Entity>()){
            return getListOfChilds(static_cast<Entity*>(root))
        }
        Case(C<Architecture>()) {
            return getListOfChilds(static_cast<Architecture*>(root))
        }
        Case(C<VType>()){
            return getListOfChilds(static_cast<VType*>(root))
        }
        Case(C<SequentialStmt>()){
            return getListOfChilds(static_cast<SequentialStmt*>(root))
        }
        Case(C<Architecture::Statement>()){
            return getListOfChilds(static_cast<Architecture::Statement*>(root))
        }
        Case(C<Expression>()){
            return getListOfChilds(static_cast<Expression*>(root))
        }
        Case(C<SigVarBase>()){
            return getListOfChilds(static_cast<SigVarBase*>(root))
        }
        Otherwise() {
        }
    } EndMatch;
}

const std::list<AstNode *> PathFinder::getListOfChilds(SequentialStmt *seq){
    using namespace mch;

    var<perm_string> loopName;
    var<list<SequentialStmt*>> loopStmts;

    // For IfSequential
    var<Expression *> ifCond;
    var<list<SequentialStmt*>> ifPath, elsePath;
    var<list<IfSequential::Elsif*>> elsifPaths;
    // For ReturnStmt
    var<Expression*> retValue;

    // For SignalSeqAssignment
    var<Expression*> assignLval;
    var<list<Expression *>> waveform;

    // For CaseStmtAlternative
    var<Expression *> caseCond;
    var<list<CaseSeqStmt::CaseStmtAlternative>> caseAlternatives;
    // For ProcedureCall
    var<perm_string> procName;
    var<list<named_expr_t*> *> procParams;
    var<SubprogramHeader *> procDef;

    // For VariableSeqAssignment
    var<Expression *> varLval, varRval;

    // For ReportStmt
    var<Expression *> reportMsg, assertCond;
    var<ReportStmt::severity_t> reportSeverity;

    // For WaitStmt
    var<WaitStmt::wait_type_t> waitType;
    var<Expression *> waitExpr;
    var<set<ExpName*>> waitSens;

    Match(seq){
        Case(C<LoopStatement>(loopName, loopStmts)){
            traversalMessages.push_back("LoopStatement detected");
            var<Expression *> whileCond;
            var<perm_string> iterVar;
            var<ExpRange*> iterRange;

            Match(seq){
                Case(C<WhileLoopStatement>(whileCond)){
                    traversalMessages.push_back("WhileLoopStatement detected");

                    // run visitor
                    if(predicate(seq)){
                        if (mutatingTraversal) { mutatingVisitor(seq); }
                        else { constVisitor(seq); }

                        if (recurSpec == GenericTraverser::NONRECUR){
                            return;
                        }
                    }

                    // descent
                    if (mutatingTraversal == false) { traverse(whileCond); }

                    for (auto &i : loopStmts)
                        traverse(i);
                }

                Case(C<ForLoopStatement>(iterVar, iterRange)){
                    traversalMessages.push_back("ForLoopStatement detected");

                    // run visitor
                    if(predicate(seq)){
                        if (mutatingTraversal) { mutatingVisitor(seq); }
                        else { constVisitor(seq); }

                        if (recurSpec == GenericTraverser::NONRECUR){
                            return;
                        }
                    }

                    // descent
                    if (mutatingTraversal == false) { traverse(iterRange); }

                    for (auto &i : loopStmts)
                        traverse(i);
                }

                Case(C<BasicLoopStatement>()){
                    traversalMessages.push_back("BasicLoopStatement detected");

                    // run visitor
                    if(predicate(seq)){
                        if (mutatingTraversal) { mutatingVisitor(seq); }
                        else { constVisitor(seq); }

                        if (recurSpec == GenericTraverser::NONRECUR){
                            return;
                        }
                    }

                    // descent
                    for (auto &i : loopStmts)
                        traverse(i);
                }

                Otherwise(){
                    errorFlag = true;
                    traversalErrors.push_back("Raw LoopStatement detected");
                }
            } EndMatch;
        }

        Case(C<IfSequential>(ifCond, ifPath, elsifPaths, elsePath)){
            traversalMessages.push_back("IfSequential detected");

            // run visitor
            if(predicate(seq)){
                if (mutatingTraversal) { mutatingVisitor(seq); }
                else { constVisitor(seq); }

                if (recurSpec == GenericTraverser::NONRECUR){
                    return;
                }
            }

            // descent
            if (mutatingTraversal == false) { traverse(ifCond); }

            for (auto &i : ifPath)
                traverse(i);

            for (auto &i : elsifPaths)
                traverse(i);

            for (auto &i : elsePath)
                traverse(i);
        }

        Case(C<ReturnStmt>(retValue)){
            traversalMessages.push_back("ReturnStmt detected");

            // run visitor
            if(predicate(seq)){
                if (mutatingTraversal) { mutatingVisitor(seq); }
                else { constVisitor(seq); }

                if (recurSpec == GenericTraverser::NONRECUR){
                    return;
                }
            }

            // descent
            if (mutatingTraversal == false) { traverse(retValue); }
        }

        Case(C<SignalSeqAssignment>(assignLval, waveform)){
            traversalMessages.push_back("SignalSeqAssignment detected");

            // run visitor
            if(predicate(seq)){
                if (mutatingTraversal) { mutatingVisitor(seq); }
                else { constVisitor(seq); }

                if (recurSpec == GenericTraverser::NONRECUR){
                    return;
                }
            }

            // descent
            if (mutatingTraversal == false) {
                traverse(assignLval);

                for (auto &i : waveform)
                    traverse(i);
            }
        }

        //FIXME: Ruines build. Don't know why
//        Case(C<CaseSeqStmt>(caseCond, caseAlternatives)){
//            traversalMessages.push_back("CaseSeqStmt detected");
//            //TODO:
//        }

        Case(C<ProcedureCall>(procName, procParams, procDef)){
            traversalMessages.push_back("ProcedureCall detected");

            // run visitor
            if(predicate(seq)){
                if (mutatingTraversal) { mutatingVisitor(seq); }
                else { constVisitor(seq); }

                if (recurSpec == GenericTraverser::NONRECUR){
                    return;
                }
            }

            // descent
            if (mutatingTraversal == false)
                for (auto &i : *static_cast<list<named_expr_t*>*>(procParams))
                    traverse(i);
        }

        Case(C<VariableSeqAssignment>(varLval, varRval)){
            traversalMessages.push_back("VariableSeqAssignment detected");

            // run visitor
            if(predicate(seq)){
                if (mutatingTraversal) { mutatingVisitor(seq); }
                else { constVisitor(seq); }

                if (recurSpec == GenericTraverser::NONRECUR){
                    return;
                }
            }

            // descent
            if (mutatingTraversal == false) {
                traverse(varLval);
                traverse(varRval);
            }
        }

        Case(C<ReportStmt>(reportMsg, reportSeverity)){
            Match(seq){
                Case(C<AssertStmt>(assertCond)){
                    traversalMessages.push_back("AssertStmt detected");

                    // run visitor
                    if(predicate(seq)){
                        if (mutatingTraversal) { mutatingVisitor(seq); }
                        else { constVisitor(seq); }

                        if (recurSpec == GenericTraverser::NONRECUR){
                            return;
                        }
                    }

                    // descent

                    if (mutatingTraversal == false) {
                        traverse(assertCond);
                        traverse(reportMsg);
                    }
                }
                Otherwise(){
                    traversalMessages.push_back("ReturnStmt detected");

                    // run visitor
                    if(predicate(seq)){
                        if (mutatingTraversal) { mutatingVisitor(seq); }
                        else { constVisitor(seq); }

                        if (recurSpec == GenericTraverser::NONRECUR){
                            return;
                        }
                    }

                    // descent
                    if (mutatingTraversal == false) { traverse(reportMsg); }
                }
            } EndMatch;
        }

        Case(C<WaitForStmt>()){
            traversalMessages.push_back("WaitForStmt detected");

            // run visitor
            if(predicate(seq)){
                if (mutatingTraversal) { mutatingVisitor(seq); }
                else { constVisitor(seq); }

                if (recurSpec == GenericTraverser::NONRECUR){
                    return;
                }
            }

            // nothing to descent
        }

        Case(C<WaitStmt>(waitType, waitExpr, waitSens)){
            traversalMessages.push_back("WaitStmt detected");

            // run visitor
            if(predicate(seq)){
                        if (mutatingTraversal) { mutatingVisitor(seq); }
                        else { constVisitor(seq); }

                if (recurSpec == GenericTraverser::NONRECUR){
                    return;
                }
            }

            // descent
            if (mutatingTraversal == false) {
                traverse(waitExpr);
                for (auto &i : waitSens)
                    traverse(i);
            }
        }

        Otherwise(){
            errorFlag = true;
            traversalErrors.push_back("Raw SequentialStat detected");
        }
    } EndMatch;
}
