#ifndef QV4ARRAY_P_H
#define QV4ARRAY_P_H

#include "qmljs_runtime.h"
#include <QtCore/QMap>
#include <QtCore/QVector>

namespace QQmlJS {
namespace VM {

class Array
{
public:
    inline Array();
    inline Array(const Array &other);
    inline ~Array();

    inline Array &operator = (const Array &other);

    inline bool isEmpty() const;
    inline uint size() const;
    inline uint count() const;
    inline Value at(uint index) const;
    inline void assign(uint index, const Value &v);
    inline void clear();
    inline void resize(uint size);
    inline void concat(const Array &other);
    inline Value pop();
    inline void sort(Context *context, const Value &comparefn);
    inline void splice(double start, double deleteCount,
                       const QVector<Value> &items,
                       Array &other);
    inline QList<uint> keys() const;

private:
    enum Mode {
        VectorMode,
        MapMode
    };

    Mode m_mode;
    int m_instances;

    union {
        QMap<uint, Value> *to_map;
        QVector<Value> *to_vector;
    };
};

class ArrayElementLessThan
{
public:
    inline ArrayElementLessThan(Context *context, const Value &comparefn)
        : m_context(context), m_comparefn(comparefn) {}

    bool operator()(const Value &v1, const Value &v2) const;

private:
    Context *m_context;
    Value m_comparefn;
};

inline Array::Array()
    : m_mode(VectorMode)
    , m_instances(0)
{
    to_vector = new QVector<Value>();
}

inline Array::Array(const Array &other):
    m_mode(other.m_mode),
    m_instances(other.m_instances)
{
    if (m_mode == VectorMode)
        to_vector = new QVector<Value> (*other.to_vector);
    else
        to_map = new QMap<uint, Value> (*other.to_map);
}

inline Array::~Array()
{
    if (m_mode == VectorMode)
        delete to_vector;
    else
        delete to_map;
}

inline Array &Array::operator = (const Array &other)
{
    m_instances = other.m_instances;
    if (m_mode != other.m_mode) {
        if (m_mode == VectorMode)
            delete to_vector;
        else
            delete to_map;
        m_mode = other.m_mode;

        if (m_mode == VectorMode)
            to_vector = new QVector<Value> (*other.to_vector);
        else
            to_map = new QMap<uint, Value> (*other.to_map);
    }

    if (m_mode == VectorMode)
        *to_vector = *other.to_vector;
    else
        *to_map = *other.to_map;

    return *this;
}

inline bool Array::isEmpty() const
{
    if (m_mode == VectorMode)
        return to_vector->isEmpty();

    return to_map->isEmpty();
}

inline uint Array::size() const
{
    if (m_mode == VectorMode)
        return to_vector->size();

    if (to_map->isEmpty())
        return 0;

    return (--to_map->constEnd()).key();
}

inline uint Array::count() const
{
    return size();
}

inline Value Array::at(uint index) const
{
    if (m_mode == VectorMode) {
        if (index < uint(to_vector->size()))
            return to_vector->at(index);
        return Value();
    } else {
        return to_map->value(index, Value());
    }
}

inline void Array::assign(uint index, const Value &v)
{
    if (index >= size()) {
        resize(index + 1);
    }

    const Value &oldv = at(index);
    if (oldv.isObject() || oldv.isString())
        --m_instances;

    if (v.isObject() || v.isString())
        ++m_instances;

    if (m_mode == VectorMode) {
        to_vector->replace(index, v);
    } else {
        if (v.isUndefined())
            to_map->remove(index);
        else
            to_map->insert(index, v);
    }
}

inline void Array::clear()
{
    m_instances = 0;

    if (m_mode == VectorMode)
        to_vector->clear();

    else
        to_map->clear();
}

inline void Array::resize(uint s)
{
    const uint oldSize = size();
    if (oldSize == s)
        return;

    const uint N = 10 * 1024;

    if (m_mode == VectorMode) {
        if (s < N) {
            to_vector->resize (s); // ### init
        } else {
            // switch to MapMode
            QMap<uint, Value> *m = new QMap<uint, Value>();
            for (uint i = 0; i < oldSize; ++i) {
                if (! to_vector->at(i).isUndefined())
                    m->insert(i, to_vector->at(i));
            }
            m->insert(s, Value());
            delete to_vector;
            to_map = m;
            m_mode = MapMode;
        }
    }

    else {
        if (s < N) {
            // switch to VectorMode
            QVector<Value> *v = new QVector<Value> (s, Value::undefinedValue());
            QMap<uint, Value>::const_iterator it = to_map->constBegin();
            for ( ; (it != to_map->constEnd()) && (it.key() < s); ++it)
                (*v) [it.key()] = it.value();
            delete to_map;
            to_vector = v;
            m_mode = VectorMode;
        } else {
            if (!to_map->isEmpty()) {
                QMap<uint, Value>::iterator it = --to_map->end();
                if (oldSize > s) {
                    // shrink
                    while ((it != to_map->end()) && (it.key() >= s)) {
                        it = to_map->erase(it);
                        --it;
                    }
                } else {
                    if ((it.key() == oldSize) && !it.value().isUndefined())
                        to_map->erase(it);
                }
            }
            to_map->insert(s, Value());
        }
    }
}

inline void Array::concat(const Array &other)
{
    uint k = size();
    resize (k + other.size());
    for (uint i = 0; i < other.size(); ++i) {
        Value v = other.at(i);
        if (! v.isUndefined())
            assign(k + i, v);
    }
}

inline Value Array::pop()
{
    if (isEmpty())
        return Value();

    Value v;

    if (m_mode == VectorMode)
        v = to_vector->last();
    else
        v = *--to_map->end();

    resize(size() - 1);

    return v;
}

inline void Array::sort(Context *context, const Value &comparefn)
{
    ArrayElementLessThan lessThan(context, comparefn);
    if (m_mode == VectorMode) {
        qSort(to_vector->begin(), to_vector->end(), lessThan);
    } else {
        QList<uint> keys = to_map->keys();
        QList<Value> values = to_map->values();
        qStableSort(values.begin(), values.end(), lessThan);
        const uint len = keys.size();
        for (uint i = 0; i < len; ++i)
            to_map->insert(keys.at(i), values.at(i));
    }
}

inline void Array::splice(double start, double deleteCount,
                          const QVector<Value> &items,
                          Array &other)
{
    const double len = size();
    if (start < 0)
        start = qMax(len + start, double(0));
    else if (start > len)
        start = len;
    deleteCount = qMax(qMin(deleteCount, len - start), double(0));

    const uint st = uint(start);
    const uint dc = uint(deleteCount);
    other.resize(dc);

    const uint itemsSize = uint(items.size());

    if (m_mode == VectorMode) {
        for (uint i = 0; i < dc; ++i)
            other.assign(i, to_vector->at(st + i));
        if (itemsSize > dc)
            to_vector->insert(st, itemsSize - dc, Value());
        else if (itemsSize < dc)
            to_vector->remove(st, dc - itemsSize);
        for (uint i = 0; i < itemsSize; ++i)
            to_vector->replace(st + i, items.at(i));
    } else {
        for (uint i = 0; i < dc; ++i)
            other.assign(i, to_map->take(st + i));
        uint del = itemsSize - dc;
        if (del != 0) {
            for (uint i = st; i < uint(len); ++i) {
                if (to_map->contains(i))
                    to_map->insert(i + del, to_map->take(i));
            }
            resize(uint(len) + del);
        }
        for (uint i = 0; i < itemsSize; ++i)
            to_map->insert(st + i, items.at(i));
    }
}

inline QList<uint> Array::keys() const
{
    if (m_mode == VectorMode)
        return QList<uint>();
    else
        return to_map->keys();
}

} // end of namespace VM
} // end of namespace QQmlJS

#endif // QV4ARRAY_P_H
