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

#include <qv4internalclass_p.h>
#include <qv4string_p.h>
#include <qv4engine_p.h>
#include <qv4identifier_p.h>

namespace QQmlJS {
namespace VM {


InternalClass::InternalClass(const QQmlJS::VM::InternalClass &other)
    : engine(other.engine)
    , propertyTable(other.propertyTable)
    , nameMap(other.nameMap)
    , propertyData(other.propertyData)
    , transitions()
    , m_sealed(0)
    , m_frozen(0)
    , size(other.size)
{
}

// ### Should we build this up from the empty class to avoid duplication?
InternalClass *InternalClass::changeMember(String *string, PropertyAttributes data, uint *index)
{
//    qDebug() << "InternalClass::changeMember()" << string->toQString() << hex << (uint)data.m_all;
    data.resolve();
    uint idx = find(string);
    if (index)
        *index = idx;

    assert(idx != UINT_MAX);

    if (data == propertyData[idx])
        return this;

    uint tid = string->identifier | (data.flags() << 27);

    QHash<int, InternalClass *>::const_iterator tit = transitions.constFind(tid);
    if (tit != transitions.constEnd())
        return tit.value();

    // create a new class and add it to the tree
    InternalClass *newClass = new InternalClass(*this);
    newClass->propertyData[idx] = data;
    return newClass;

}

InternalClass *InternalClass::addMember(String *string, PropertyAttributes data, uint *index)
{
//    qDebug() << "InternalClass::addMember()" << string->toQString() << size << hex << (uint)data.m_all << data.type();
    data.resolve();
    engine->identifierCache->toIdentifier(string);
    uint id = string->identifier | (data.flags() << 27);

    assert(propertyTable.constFind(id) == propertyTable.constEnd());

    QHash<int, InternalClass *>::const_iterator tit = transitions.constFind(id);

    if (index)
        *index = size;
    if (tit != transitions.constEnd())
        return tit.value();

    // create a new class and add it to the tree
    InternalClass *newClass = new InternalClass(*this);
    newClass->propertyTable.insert(string->identifier, size);
    newClass->nameMap.append(string);
    newClass->propertyData.append(data);
    ++newClass->size;
    transitions.insert(id, newClass);
    return newClass;
}

void InternalClass::removeMember(Object *object, uint id)
{
    assert (propertyTable.constFind(id) != propertyTable.constEnd());
    int propIdx = propertyTable.constFind(id).value();
    assert(propIdx < size);

    int toRemove = - (int)id;
    QHash<int, InternalClass *>::const_iterator tit = transitions.constFind(toRemove);

    if (tit != transitions.constEnd()) {
        object->internalClass = tit.value();
        return;
    }

    // create a new class and add it to the tree
    object->internalClass = engine->emptyClass;
    for (int i = 0; i < nameMap.size(); ++i) {
        if (i == propIdx)
            continue;
        object->internalClass = object->internalClass->addMember(nameMap.at(i), propertyData.at(i));
    }

    transitions.insert(toRemove, object->internalClass);
}

uint InternalClass::find(String *string)
{
    engine->identifierCache->toIdentifier(string);
    uint id = string->identifier;

    QHash<uint, uint>::const_iterator it = propertyTable.constFind(id);
    if (it != propertyTable.constEnd())
        return it.value();

    return UINT_MAX;
}

InternalClass *InternalClass::sealed()
{
    if (m_sealed)
        return m_sealed;

    m_sealed = engine->emptyClass;
    for (int i = 0; i < nameMap.size(); ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        attrs.setConfigurable(false);
        m_sealed = m_sealed->addMember(nameMap.at(i), attrs);
    }

    m_sealed->m_sealed = m_sealed;
    return m_sealed;
}

InternalClass *InternalClass::frozen()
{
    if (m_frozen)
        return m_frozen;

    m_frozen = engine->emptyClass;
    for (int i = 0; i < nameMap.size(); ++i) {
        PropertyAttributes attrs = propertyData.at(i);
        attrs.setWritable(false);
        attrs.setConfigurable(false);
        m_frozen = m_frozen->addMember(nameMap.at(i), attrs);
    }

    m_frozen->m_frozen = m_frozen;
    return m_frozen;
}


}
}
