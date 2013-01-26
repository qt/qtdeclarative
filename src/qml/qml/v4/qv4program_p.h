/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifdef Q_CC_MSVC
// nonstandard extension used : zero-sized array in struct/union.
#  pragma warning( disable : 4200 )
#endif

QT_BEGIN_NAMESPACE

struct QV4Program {
    quint32 bindings;
    quint32 dataLength;
    quint32 signalTableOffset;
    quint32 exceptionDataOffset;
    quint16 subscriptions;
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

enum QQmlRegisterType { 
    UndefinedType,
    NullType,
    QObjectStarType,
    NumberType,
    FloatType,
    IntType,
    BoolType,
    SpecialNumericType,

    PODValueType,

    FirstCleanupType, 
    QStringType = FirstCleanupType,
    QUrlType,
    QVariantType,
    QColorType,
    V8HandleType,
    QJSValueType
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

#endif // QV4PROGRAM_P_H

