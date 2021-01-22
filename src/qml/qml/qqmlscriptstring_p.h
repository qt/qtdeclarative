/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQMLSCRIPTSTRING_P_H
#define QQMLSCRIPTSTRING_P_H

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

#include "qqmlscriptstring.h"
#include <QtQml/qqmlcontext.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QQmlScriptStringPrivate : public QSharedData
{
public:
    QQmlScriptStringPrivate() : context(nullptr), scope(nullptr), bindingId(-1), lineNumber(0), columnNumber(0),
        numberValue(0), isStringLiteral(false), isNumberLiteral(false) {}

    //for testing
    static const QQmlScriptStringPrivate* get(const QQmlScriptString &script);

    QQmlContext *context;
    QObject *scope;
    QString script;
    int bindingId;
    quint16 lineNumber;
    quint16 columnNumber;
    double numberValue;
    bool isStringLiteral;
    bool isNumberLiteral;
};

QT_END_NAMESPACE

#endif // QQMLSCRIPTSTRING_P_H
