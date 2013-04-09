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

#include <qv4internalclass.h>
#include <qv4string.h>
#include <qv4engine.h>
#include <qv4identifier.h>

namespace QQmlJS {
namespace VM {


InternalClass::InternalClass(const QQmlJS::VM::InternalClass &other)
    : engine(other.engine)
    , propertyTable(other.propertyTable)
    , nameMap(other.nameMap)
    , propertyData(other.propertyData)
    , transitions()
    , size(other.size)
{
}

InternalClass *InternalClass::addMember(String *string, PropertyAttributes data, uint *index)
{
    engine->identifierCache->toIdentifier(string);
    uint id = string->identifier | (data.flags() << 24);

    assert(propertyTable.constFind(id) == propertyTable.constEnd());

    QHash<int, InternalClass *>::const_iterator tit = transitions.constFind(id);

    if (index)
        *index = size;
    if (tit != transitions.constEnd()) {
        return tit.value();
    } else {
        // create a new class and add it to the tree
        InternalClass *newClass = new InternalClass(*this);
        newClass->propertyTable.insert(string->identifier, size);
        newClass->nameMap.append(string);
        newClass->propertyData.append(data);
        ++newClass->size;
        transitions.insert(id, newClass);
        return newClass;
    }
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


}
}
