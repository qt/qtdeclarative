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
#ifndef QV4COMPILER_P_H
#define QV4COMPILER_P_H

#include <QtCore/qstring.h>
#include "qv4jsir_p.h"

QT_BEGIN_NAMESPACE

class QQmlPropertyData;

namespace QV4 {

namespace CompiledData {
struct Unit;
struct Lookup;
struct RegExp;
struct JSClassMember;
}

namespace Compiler {

struct Q_QML_EXPORT JSUnitGenerator {
    JSUnitGenerator(QQmlJS::V4IR::Module *module, int headerSize = -1);

    QQmlJS::V4IR::Module *irModule;

    int registerString(const QString &str);
    int getStringId(const QString &string) const;

    uint registerGetterLookup(const QString &name);
    uint registerSetterLookup(const QString &name);
    uint registerGlobalGetterLookup(const QString &name);

    int registerRegExp(QQmlJS::V4IR::RegExp *regexp);

    int registerConstant(ReturnedValue v);

    void registerLineNumberMapping(QQmlJS::V4IR::Function *function, const QVector<uint> &mappings);

    int registerJSClass(QQmlJS::V4IR::ExprList *args);

    QV4::CompiledData::Unit *generateUnit(int *totalUnitSize = 0);
    // Returns bytes written
    int writeFunction(char *f, int index, QQmlJS::V4IR::Function *irFunction);

    QHash<QString, int> stringToId;
    QStringList strings;
    uint stringDataSize;
    QHash<QQmlJS::V4IR::Function *, uint> functionOffsets;
    QList<CompiledData::Lookup> lookups;
    QVector<CompiledData::RegExp> regexps;
    QVector<ReturnedValue> constants;
    QHash<QQmlJS::V4IR::Function *, QVector<uint> > lineNumberMappingsPerFunction;
    QList<QList<CompiledData::JSClassMember> > jsClasses;
    uint jsClassDataSize;
    uint headerSize;
};

}

}

QT_END_NAMESPACE

#endif
