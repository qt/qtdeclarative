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
#ifndef QV4INTERNALCLASS_H
#define QV4INTERNALCLASS_H

#include <QHash>
#include <QVector>
#include "qv4global_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct String;
struct ExecutionEngine;
struct Object;

struct InternalClass {
    ExecutionEngine *engine;
    QHash<uint, uint> propertyTable; // id to valueIndex
    QVector<String *> nameMap;

    QVector<PropertyAttributes> propertyData;

    QHash<int, InternalClass *> transitions; // id to next class, positive means add, negative delete

    InternalClass *m_sealed;
    InternalClass *m_frozen;

    uint size;

    InternalClass(ExecutionEngine *engine) : engine(engine), m_sealed(0), m_frozen(0), size(0) {}

    InternalClass *addMember(String *string, PropertyAttributes data, uint *index = 0);
    InternalClass *changeMember(String *string, PropertyAttributes data, uint *index = 0);
    void removeMember(Object *object, uint id);
    uint find(String *s);

    InternalClass *sealed();
    InternalClass *frozen();

private:
    InternalClass(const InternalClass &other);
};


}

QT_END_NAMESPACE

#endif
