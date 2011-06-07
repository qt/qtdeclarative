/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <qtest.h>
#include "../../../../src/3rdparty/v8/include/v8.h"

using namespace v8;

class tst_v8 : public QObject
{
    Q_OBJECT
public:
    tst_v8() {}

private slots:
    void initTestCase() {}
    void cleanupTestCase();

    void eval();

private:
    Persistent<Context> context;
};

void tst_v8::eval()
{
    HandleScope handle_scope;
    context = Context::New();
    Context::Scope context_scope(context);

    Local<Object> qmlglobal = Object::New();
    qmlglobal->Set(String::New("a"), v8::Integer::New(1922));

    Local<Script> script = Script::Compile(String::New("eval(\"a\")"), NULL, NULL, 
                                           Handle<String>(), Script::QmlMode);
    
    TryCatch tc;
    Local<Value> result = script->Run(qmlglobal);

    QVERIFY(!tc.HasCaught());
    QCOMPARE(result->Int32Value(), 1922);
}

void tst_v8::cleanupTestCase()
{
    context.Dispose();
    context = Persistent<Context>();
}

int main(int argc, char *argv[]) 
{ 
    V8::SetFlagsFromCommandLine(&argc, argv, true);

    QCoreApplication app(argc, argv); 
    tst_v8 tc; 
    return QTest::qExec(&tc, argc, argv); 
}

#include "tst_v8.moc"
