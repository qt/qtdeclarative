/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4PROPERTYTABLE_H
#define QV4PROPERTYTABLE_H

#include "qmljs_value.h"
#include "qv4propertydescriptor.h"

namespace QQmlJS {
namespace VM {

struct ObjectIterator;

struct PropertyTableEntry {
    PropertyDescriptor descriptor;
    String *name;
    PropertyTableEntry *next;
    int index;

    inline PropertyTableEntry(String *name)
        : name(name),
          next(0),
          index(-1)
    { }

    inline unsigned hashValue() const { return name->hashValue(); }
};

class PropertyTable
{
    Q_DISABLE_COPY(PropertyTable)

public:
    PropertyTable()
        : _properties(0)
        , _buckets(0)
        , _freeList(0)
        , _propertyCount(0)
        , _bucketCount(0)
        , _primeIdx(-1)
        , _allocated(0)
    {}

    ~PropertyTable()
    {
        qDeleteAll(_properties, _properties + _propertyCount);
        delete[] _properties;
        delete[] _buckets;
    }

    typedef PropertyTableEntry **iterator;
    inline iterator begin() const { return _properties; }
    inline iterator end() const { return _properties + _propertyCount; }

    void remove(PropertyTableEntry *prop)
    {
        PropertyTableEntry **bucket = _buckets + (prop->hashValue() % _bucketCount);
        if (*bucket == prop) {
            *bucket = prop->next;
        } else {
            for (PropertyTableEntry *it = *bucket; it; it = it->next) {
                if (it->next == prop) {
                    it->next = it->next->next;
                    break;
                }
            }
        }

        _properties[prop->index] = 0;
        prop->next = _freeList;
        _freeList = prop;
    }

    PropertyTableEntry *findEntry(String *name) const
    {
        if (_properties) {
            for (PropertyTableEntry *prop = _buckets[name->hashValue() % _bucketCount]; prop; prop = prop->next) {
                if (prop && prop->name->isEqualTo(name))
                    return prop;
            }
        }

        return 0;
    }

    PropertyDescriptor *find(String *name) const
    {
        if (_properties) {
            for (PropertyTableEntry *prop = _buckets[name->hashValue() % _bucketCount]; prop; prop = prop->next) {
                if (prop && prop->name->isEqualTo(name))
                    return &prop->descriptor;
            }
        }

        return 0;
    }

    PropertyDescriptor *insert(String *name)
    {
        if (PropertyTableEntry *prop = findEntry(name))
            return &prop->descriptor;

        if (_propertyCount == _allocated) {
            if (! _allocated)
                _allocated = 4;
            else
                _allocated *= 2;

            PropertyTableEntry **properties = new PropertyTableEntry*[_allocated];
            std::copy(_properties, _properties + _propertyCount, properties);
            delete[] _properties;
            _properties = properties;
        }

        PropertyTableEntry *prop;
        if (_freeList) {
            prop = _freeList;
            prop->name = name;
            _freeList = _freeList->next;
        } else {
            prop = new PropertyTableEntry(name);
        }

        prop->index = _propertyCount;
        _properties[_propertyCount] = prop;
        ++_propertyCount;

        if (! _buckets || 3 * _propertyCount >= 2 * _bucketCount) {
            rehash();
        } else {
            PropertyTableEntry *&bucket = _buckets[prop->hashValue() % _bucketCount];
            prop->next = bucket;
            bucket = prop;
        }

        return &prop->descriptor;
    }

private:
    void rehash()
    {
        _bucketCount = nextPrime();

        delete[] _buckets;
        _buckets = new PropertyTableEntry *[_bucketCount];
        std::fill(_buckets, _buckets + _bucketCount, (PropertyTableEntry *) 0);

        for (int i = 0; i < _propertyCount; ++i) {
            PropertyTableEntry *prop = _properties[i];
            if (prop) {
                PropertyTableEntry *&bucket = _buckets[prop->hashValue() % _bucketCount];
                prop->next = bucket;
                bucket = prop;
            }
        }
    }

    inline int nextPrime()
    {
        // IMPORTANT: do not add more primes without checking if _primeIdx needs more bits!
        static const int primes[] = {
            11, 23, 47, 97, 197, 397, 797, 1597, 3203, 6421, 12853, 25717, 51437, 102877
        };

        if (_primeIdx < (int) (sizeof(primes)/sizeof(int)))
            return primes[++_primeIdx];
        else
            return _bucketCount * 2 + 1; // Yes, we're degrading here. But who needs more than about 68000 properties?
    }

private:
    friend struct ObjectIterator;
    PropertyTableEntry **_properties;
    PropertyTableEntry **_buckets;
    PropertyTableEntry *_freeList;
    int _propertyCount;
    int _bucketCount;
    int _primeIdx: 5;
    int _allocated: 27;
};

}
}

#endif
