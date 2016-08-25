#ifndef IVL_SYNC_PREDICATE
#define IVL_SYNC_PREDICATE

#include <expression.h>
#include <propcalc.h>

class SyncCondPredicate {
public:
    SyncCondPredicate() = default;

    bool operator()(const Expression *e);

public:
    PropcalcFormula *fromExpression(const Expression *,
                                    const Expression *);
};

#endif /* IVL_SYNC_PREDICATE */
