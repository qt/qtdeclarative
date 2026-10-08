// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "quicktestutil_p.h"

#include <QtQuickTest/private/qtestoptions_p.h>
#include <QtQml/private/qqmltype_p.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQml/private/qv4engine_p.h>
#include <QtQml/private/qv4scopedvalue_p.h>
#include <QtQml/private/qjsvalue_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qclipboard.h>
#include <QtGui/qstylehints.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

bool QuickTestUtil::printAvailableFunctions() const
{
    return QTest::printAvailableFunctions;
}

int QuickTestUtil::dragThreshold() const
{
    return QGuiApplication::styleHints()->startDragDistance();
}

void QuickTestUtil::populateClipboardText(int lineCount)
{
#if QT_CONFIG(clipboard)
    QString fmt(u"%1 bottles of beer on the wall, %1 bottles of beer; "
                "take one down, pass it around, %2 bottles of beer on the wall."_s);
    QStringList lines;
    for (int i = lineCount; i > 0; --i)
        lines << fmt.arg(i).arg(i - 1);
    QGuiApplication::clipboard()->setText(lines.join(u'\n'));
#else
    Q_UNUSED(lineCount)
#endif
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
    return QJSValuePrivate::fromReturnedValue(v4->newString(name)->asReturnedValue());
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
            ? QJSValuePrivate::fromReturnedValue(
                  v4->newString(stack.at(frameIndex + 1).source)->asReturnedValue())
            : QJSValue();
}

int QuickTestUtil::callerLine(int frameIndex) const
{
    QQmlEngine *engine = qmlEngine(this);
    QV4::ExecutionEngine *v4 = engine->handle();

    QVector<QV4::StackFrame> stack = v4->stackTrace(frameIndex + 2);
    if (stack.size() > frameIndex + 1)
        return qAbs(stack.at(frameIndex + 1).line);
    return -1;
}

QT_END_NAMESPACE

#include "moc_quicktestutil_p.cpp"
