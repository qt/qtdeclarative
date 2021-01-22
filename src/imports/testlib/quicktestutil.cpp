/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "quicktestutil_p.h"

#include <QtQuickTest/private/qtestoptions_p.h>
#include <QtQml/private/qqmltype_p.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQml/private/qv4engine_p.h>
#include <QtQml/private/qv4scopedvalue_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

bool QuickTestUtil::printAvailableFunctions() const
{
    return QTest::printAvailableFunctions;
}

int QuickTestUtil::dragThreshold() const
{
    return QGuiApplication::styleHints()->startDragDistance();
}

QJSValue QuickTestUtil::typeName(const QVariant &v) const
{
    QString name = QString::fromUtf8(v.typeName());
    if (v.canConvert<QObject*>()) {
        QQmlType type;
        const QMetaObject *mo = v.value<QObject*>()->metaObject();
        while (!type.isValid() && mo) {
            type = QQmlMetaType::qmlType(mo);
            mo = mo->superClass();
        }
        if (type.isValid()) {
            name = type.qmlTypeName();
        }
    }

    QQmlEngine *engine = qmlEngine(this);
    QV4::ExecutionEngine *v4 = engine->handle();
    return QJSValue(v4, v4->newString(name)->asReturnedValue());
}

bool QuickTestUtil::compare(const QVariant &act, const QVariant &exp) const {
    return act == exp;
}

QJSValue QuickTestUtil::callerFile(int frameIndex) const
{
    QQmlEngine *engine = qmlEngine(this);
    QV4::ExecutionEngine *v4 = engine->handle();
    QV4::Scope scope(v4);

    QVector<QV4::StackFrame> stack = v4->stackTrace(frameIndex + 2);
    return (stack.size() > frameIndex + 1)
            ? QJSValue(v4, v4->newString(stack.at(frameIndex + 1).source)->asReturnedValue())
            : QJSValue();
}

int QuickTestUtil::callerLine(int frameIndex) const
{
    QQmlEngine *engine = qmlEngine(this);
    QV4::ExecutionEngine *v4 = engine->handle();

    QVector<QV4::StackFrame> stack = v4->stackTrace(frameIndex + 2);
    if (stack.size() > frameIndex + 1)
        return stack.at(frameIndex + 1).line;
    return -1;
}

QT_END_NAMESPACE
