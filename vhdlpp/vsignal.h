#ifndef IVL_vsignal_H
#define IVL_vsignal_H

/*
 * Copyright (c) 2011-2014 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
# include <map>

# include "StringHeap.h"
# include "LineInfo.h"
# include "vtype.h"
# include "simple_tree.h"
# include "root_class.h"

class Architecture;
class ScopeBase;
class Entity;
class Expression;

using namespace std;

class SigVarBase : public LineInfo, public AstNode {
public:
    SigVarBase(perm_string name,
               VType *type,
               Expression *init_expr);
    virtual ~SigVarBase();

    VType *peek_type(void) const {
        return type_;
    }

    // Call this method for each occasion where this signal is the
    // l-value of a sequential assignment.
    void count_ref_sequ();

    void dump(ostream& out, int indent = 0) const;

    // Elaborates type & initializer expressions.
    void elaborate(Entity *ent, ScopeBase *scope);

    perm_string peek_name() const {
        return name_;
    }

    // FM. MA
    virtual SimpleTree<map<string, string>> *emit_strinfo_tree() const = 0;

public:
    unsigned peek_refcnt_sequ_() const {
        return refcnt_sequ_;
    }

    void type_elaborate_(VType::decl_t& decl);

    Expression *peek_init_expr() const {
        return init_expr_;
    }


    perm_string name_;
    VType *type_;
    Expression  *init_expr_;

    unsigned refcnt_sequ_;

public:     // Not implemented
    SigVarBase(const SigVarBase&);
    SigVarBase& operator =(const SigVarBase&);
};

class Signal : public SigVarBase {
public:
    Signal(perm_string name, VType *type,
           Expression *init_expr);

    // FM. MA
    SimpleTree<map<string, string>> *emit_strinfo_tree() const;

    int emit(ostream& out, Entity *ent, ScopeBase *scope);
};

class Variable : public SigVarBase {
public:
    Variable(perm_string name, VType *type,
             Expression *init_expr = NULL);

    int emit(ostream& out, Entity *ent, ScopeBase *scope);
    void write_to_stream(ostream& fd);

    // FM. MA
    SimpleTree<map<string, string>> *emit_strinfo_tree() const;
};

inline void SigVarBase::count_ref_sequ() {
    refcnt_sequ_ += 1;
}


inline Signal::Signal(perm_string name,
                      VType *type,
                      Expression *init_expr)
    : SigVarBase(name, type, init_expr) {}


inline Variable::Variable(perm_string name,
                          VType *type,
                          Expression *init_expr)
    : SigVarBase(name, type, init_expr) {}


#endif /* IVL_vsignal_H */
