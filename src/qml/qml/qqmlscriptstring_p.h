/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLSCRIPTSTRING_P_H
#define QQMLSCRIPTSTRING_P_H

#include "qqmlscriptstring.h"
#include <QtQml/qqmlcontext.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QQmlScriptStringPrivate : public QSharedData
{
public:
    QQmlScriptStringPrivate() : context(0), scope(0), bindingId(-1), lineNumber(-1), columnNumber(-1),
        numberValue(0), isStringLiteral(false), isNumberLiteral(false) {}

    //for testing
    static const QQmlScriptStringPrivate* get(const QQmlScriptString &script);

    QQmlContext *context;
    QObject *scope;
    QString script;
    int bindingId;
    int lineNumber;
    int columnNumber;
    double numberValue;
    bool isStringLiteral;
    bool isNumberLiteral;
};

QT_END_NAMESPACE

#endif // QQMLSCRIPTSTRING_P_H
