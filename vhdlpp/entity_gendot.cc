// FM. MA
//
// contains implementation of dot conversion functions
// which are members of the classes in entity.h

#include <iostream>
#include <typeinfo>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <vector>
#include <sstream>
#include <map>
#include <cassert>

#include "simple_tree.h"
#include "entity.h"
#include "compiler.h"
#include "architec.h"
#include "vtype.h"

using namespace std;

simple_tree<map<string, string>> *InterfacePort::emit_strinfo_tree() const {
    auto result = new simple_tree<map<string, string>>(
        map<string, string>{
            {"node-type", "InterfacePort"},
            {"port name", "name"},
            {"port mode", (dynamic_cast<stringstream&>(
                stringstream{} << mode)).str()}});

    result->forest.push_back(expr->emit_strinfo_tree());
    result->forest.push_back(type->emit_strinfo_tree());

    return result;
}

simple_tree<map<string, string>> *ComponentBase::emit_strinfo_tree() const {
    auto result = new simple_tree<map<string, string>>(
        map<string, string>{
            {"node-type", "Component"},
            {"component name", name_.str()}});

    for (auto &i : parms_)
        result->forest.push_back(i->emit_strinfo_tree());

    for(auto &i : ports_)
        result->forest.push_back(i->emit_strinfo_tree());

    return result;
}

simple_tree<map<string, string>> *Entity::emit_strinfo_tree() const {
    auto result = new simple_tree<map<string, string>>(
        map<string, string>{
            {"node-type", "Entity"},
            {"name", name_.str()}});

    for (auto &i : parms_)
        result->forest.push_back(i->emit_strinfo_tree());

    for(auto &i : ports_)
        result->forest.push_back(i->emit_strinfo_tree());


    for (auto &i : arch_)
        result->forest.push_back(i.second->emit_strinfo_tree());

//    for (auto &i : declarations_)
//        result->forest.push_back(i.second->emit_strinfo_tree());

    result->forest.push_back(
        new simple_tree<map<string, string>>(
            map<string, string>{{"node-type", "Bound Architecture"}},
            bind_arch_->emit_strinfo_tree()));

    return result;
}