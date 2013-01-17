/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qjsengine.h>
#include "QtQuickTest/private/quicktestresult_p.h"
#include "QtQuickTest/private/quicktestevent_p.h"
#include "private/qtestoptions_p.h"
#include "QtQuick/qquickitem.h"
#include <QtQml/private/qqmlengine_p.h>
#include <QtGui/QGuiApplication>
#include <QtGui/qstylehints.h>

QML_DECLARE_TYPE(QuickTestResult)
QML_DECLARE_TYPE(QuickTestEvent)

#include <QtDebug>

QT_BEGIN_NAMESPACE

class QuickTestUtil : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool printAvailableFunctions READ printAvailableFunctions NOTIFY printAvailableFunctionsChanged)
    Q_PROPERTY(int dragThreshold READ dragThreshold NOTIFY dragThresholdChanged)
public:
    QuickTestUtil(QObject *parent = 0)
        :QObject(parent)
    {}

    ~QuickTestUtil()
    {}
    bool printAvailableFunctions() const
    {
        return QTest::printAvailableFunctions;
    }
    int dragThreshold() const { return qApp->styleHints()->startDragDistance(); }

Q_SIGNALS:
    void printAvailableFunctionsChanged();
    void dragThresholdChanged();

public Q_SLOTS:

    QQmlV8Handle typeName(const QVariant& v) const
    {
        QString name(v.typeName());
        if (v.canConvert<QObject*>()) {
            QQmlType *type = 0;
            const QMetaObject *mo = v.value<QObject*>()->metaObject();
            while (!type && mo) {
                type = QQmlMetaType::qmlType(mo);
                mo = mo->superClass();
            }
            if (type) {
                name = type->qmlTypeName();
            }
        }

        return QQmlV8Handle::fromHandle(v8::String::New(name.toUtf8()));
    }

    bool compare(const QVariant& act, const QVariant& exp) const {
        return act == exp;
    }

    QQmlV8Handle callerFile(int frameIndex = 0) const
    {
        v8::Local<v8::StackTrace> stacks = v8::StackTrace::CurrentStackTrace(10, v8::StackTrace::kDetailed);
        int count = stacks->GetFrameCount();
        if (count >= frameIndex + 1) {
            v8::Local<v8::StackFrame> frame = stacks->GetFrame(frameIndex + 1);
            return QQmlV8Handle::fromHandle(frame->GetScriptNameOrSourceURL());
        }
        return QQmlV8Handle();
    }
    int callerLine(int frameIndex = 0) const
    {
        v8::Local<v8::StackTrace> stacks = v8::StackTrace::CurrentStackTrace(10, v8::StackTrace::kDetailed);
        int count = stacks->GetFrameCount();
        if (count >= frameIndex + 1) {
            v8::Local<v8::StackFrame> frame = stacks->GetFrame(frameIndex + 1);
            return frame->GetLineNumber();
        }
        return -1;
    }
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QuickTestUtil)

QT_BEGIN_NAMESPACE

class QTestQmlModule : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtTest"));
        qmlRegisterType<QuickTestResult>(uri,1,0,"TestResult");
        qmlRegisterType<QuickTestEvent>(uri,1,0,"TestEvent");
        qmlRegisterType<QuickTestUtil>(uri,1,0,"TestUtil");
    }

    void initializeEngine(QQmlEngine *, const char *)
    {
    }
};

QT_END_NAMESPACE

#include "main.moc"
