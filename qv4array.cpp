
#include "qv4array_p.h"
#include "qmljs_objects.h"

using namespace QQmlJS::VM;

bool ArrayElementLessThan::operator()(const Value &v1, const Value &v2) const
{
    if (v1.isUndefined())
        return false;
    if (v2.isUndefined())
        return true;
    if (!m_comparefn.isUndefined()) {
        Value args[] = { v1, v2 };
        Value result;
        __qmljs_call_value(m_context, &result, 0, &m_comparefn, args, 2);
        return result.toNumber(m_context) <= 0;
    }
    return v1.toString(m_context)->toQString() < v2.toString(m_context)->toQString();
}
