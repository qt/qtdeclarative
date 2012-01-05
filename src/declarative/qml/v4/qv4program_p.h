/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV4PROGRAM_P_H
#define QV4PROGRAM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qv4instruction_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

struct QV4Program {
    quint32 bindings;
    quint32 dataLength;
    quint32 signalTableOffset;
    quint32 exceptionDataOffset;
    quint16 subscriptions;
    quint16 identifiers;
    quint16 instructionCount;

    struct BindingReference {
        quint32 binding;
        quint32 blockMask;
    };
    
    struct BindingReferenceList {
        quint32 count;
        BindingReference bindings[];
    };

    inline const char *data() const;
    inline const char *instructions() const;
    inline BindingReferenceList *signalTable(int signalIndex) const;
};

enum QDeclarativeRegisterType { 
    UndefinedType,
    QObjectStarType,
    QRealType,
    IntType,
    BoolType,

    PODValueType,

    FirstCleanupType, 
    QStringType = FirstCleanupType,
    QUrlType,
    QVariantType,
};

const char *QV4Program::data() const 
{ 
    return ((const char *)this) + sizeof(QV4Program); 
}

const char *QV4Program::instructions() const
{ 
    return (const char *)(data() + dataLength);
}

QV4Program::BindingReferenceList *QV4Program::signalTable(int signalIndex) const 
{ 
    quint32 *signalTable = (quint32 *)(data() + signalTableOffset);
    return (BindingReferenceList *)(signalTable + signalTable[signalIndex]);
}

QT_END_NAMESPACE

QT_END_HEADER

#endif // QV4PROGRAM_P_H

