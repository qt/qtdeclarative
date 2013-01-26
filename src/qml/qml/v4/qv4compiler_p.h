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

#include <private/qqmlexpression_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlcompiler_p.h>

#include <private/qv8_p.h>

Q_DECLARE_METATYPE(v8::Handle<v8::Value>)

QT_BEGIN_NAMESPACE

class QQmlTypeNameCache;
class QV4CompilerPrivate;
class Q_AUTOTEST_EXPORT QV4Compiler
{
public:
    QV4Compiler();
    ~QV4Compiler();

    // Returns true if bindings were compiled
    bool isValid() const;

    struct Expression
    {
        Expression(const QQmlImports &imp) : imports(imp) {}
        QQmlScript::Object *component;
        QQmlScript::Object *context;
        QQmlScript::Property *property;
        QQmlScript::Variant expression;
        QQmlCompilerTypes::IdList *ids;
        QQmlTypeNameCache *importCache;
        QQmlImports imports;
    };

    // -1 on failure, otherwise the binding index to use
    int compile(const Expression &, QQmlEnginePrivate *, bool *);

    // Returns the compiled program
    QByteArray program() const;

    static void dump(const QByteArray &);
    static void enableBindingsTest(bool);
    static void enableV4(bool);
private:
    QV4CompilerPrivate *d;
};

QT_END_NAMESPACE

#endif // QV4COMPILER_P_H

