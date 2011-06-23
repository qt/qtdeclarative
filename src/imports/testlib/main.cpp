/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtDeclarative/qdeclarativeextensionplugin.h>
#include <QtDeclarative/qdeclarative.h>
#include <QtScript/qscriptvalue.h>
#include <QtScript/qscriptcontext.h>
#include <QtScript/qscriptcontextinfo.h>
#include <QtScript/qscriptengine.h>
#include "QtQuickTest/private/quicktestresult_p.h"
#include "QtQuickTest/private/quicktestevent_p.h"
#include "private/qtestoptions_p.h"
#include "QtDeclarative/qsgitem.h"
#include <QtDeclarative/private/qdeclarativeengine_p.h>
QT_BEGIN_NAMESPACE

QML_DECLARE_TYPE(QuickTestResult)
QML_DECLARE_TYPE(QuickTestEvent)

#include <QtDebug>

class QuickTestUtil : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool printAvailableFunctions READ printAvailableFunctions)
    Q_PROPERTY(bool wrapper READ wrapper)
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
    bool wrapper() const
    {
        return true;
    }

public Q_SLOTS:
    QDeclarativeV8Handle callerFile(int frameIndex = 0) const
    {
        v8::HandleScope scope;

        v8::Local<v8::StackTrace> stacks = v8::StackTrace::CurrentStackTrace(10, v8::StackTrace::kDetailed);
        int count = stacks->GetFrameCount();
        if (count >= frameIndex + 2) {
            v8::Local<v8::StackFrame> frame = stacks->GetFrame(frameIndex + 2);
            return QDeclarativeV8Handle::fromHandle(frame->GetScriptNameOrSourceURL());
        }
        return QDeclarativeV8Handle();
    }
    int callerLine(int frameIndex = 0) const
    {
        v8::HandleScope scope;
        v8::Local<v8::StackTrace> stacks = v8::StackTrace::CurrentStackTrace(10, v8::StackTrace::kDetailed);
        int count = stacks->GetFrameCount();
        if (count >= frameIndex + 2) {
            v8::Local<v8::StackFrame> frame = stacks->GetFrame(frameIndex + 2);
            return frame->GetLineNumber();
        }
        return -1;
    }
};
QML_DECLARE_TYPE(QuickTestUtil)

class QTestQmlModule : public QDeclarativeExtensionPlugin
{
    Q_OBJECT
public:
    virtual void registerTypes(const char *uri)
    {
        Q_ASSERT(QLatin1String(uri) == QLatin1String("QtTest"));
        qmlRegisterType<QuickTestResult>(uri,1,0,"TestResult");
        qmlRegisterType<QuickTestEvent>(uri,1,0,"TestEvent");
        qmlRegisterType<QuickTestUtil>(uri,1,0,"TestUtil");
    }

    void initializeEngine(QDeclarativeEngine *engine, const char *)
    {
    }
};

QT_END_NAMESPACE

#include "main.moc"

Q_EXPORT_PLUGIN2(qmltestplugin, QT_PREPEND_NAMESPACE(QTestQmlModule));
