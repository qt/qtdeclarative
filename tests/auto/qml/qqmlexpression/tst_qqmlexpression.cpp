// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlscriptstring.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qqmlexpression : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlexpression() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void scriptString();
    void syntaxError();
    void exception();
    void expressionFromDataComponent();
    void emptyScriptString();
};

class TestObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlScriptString scriptString READ scriptString WRITE setScriptString)
    Q_PROPERTY(QQmlScriptString scriptStringError READ scriptStringError WRITE setScriptStringError)
public:
    TestObject(QObject *parent = nullptr) : QObject(parent) {}

    QQmlScriptString scriptString() const { return m_scriptString; }
    void setScriptString(QQmlScriptString scriptString) { m_scriptString = scriptString; }

    QQmlScriptString scriptStringError() const { return m_scriptStringError; }
    void setScriptStringError(QQmlScriptString scriptString) { m_scriptStringError = scriptString; }

private:
    QQmlScriptString m_scriptString;
    QQmlScriptString m_scriptStringError;
};

QML_DECLARE_TYPE(TestObject)

void tst_qqmlexpression::scriptString()
{
    qmlRegisterType<TestObject>("Test", 1, 0, "TestObject");

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("scriptString.qml"));
    TestObject *testObj = qobject_cast<TestObject*>(c.create());
    QVERIFY(testObj != nullptr);

    QQmlScriptString script = testObj->scriptString();
    QVERIFY(!script.isEmpty());

    QQmlExpression expression(script);
    QVariant value = expression.evaluate();
    QCOMPARE(value.toInt(), 15);

    QQmlScriptString scriptError = testObj->scriptStringError();
    QVERIFY(!scriptError.isEmpty());

    //verify that the expression has the correct error location information
    QQmlExpression expressionError(scriptError);
    QVariant valueError = expressionError.evaluate();
    QVERIFY(!valueError.isValid());
    QVERIFY(expressionError.hasError());
    QQmlError error = expressionError.error();
    QCOMPARE(error.url(), c.url());
    QCOMPARE(error.line(), 8);
}

// QTBUG-21310 - crash test
void tst_qqmlexpression::syntaxError()
{
    QQmlEngine engine;
    QQmlExpression expression(engine.rootContext(), nullptr, "asd asd");
    bool isUndefined = false;
    QVariant v = expression.evaluate(&isUndefined);
    QCOMPARE(v, QVariant());
    QVERIFY(expression.hasError());
    QCOMPARE(expression.error().description(), "SyntaxError: Expected token `;'");
    QVERIFY(isUndefined);
}

void tst_qqmlexpression::exception()
{
    QQmlEngine engine;
    QQmlExpression expression(engine.rootContext(), nullptr, "abc=123");
    QVariant v = expression.evaluate();
    QCOMPARE(v, QVariant());
    QVERIFY(expression.hasError());
}

void tst_qqmlexpression::expressionFromDataComponent()
{
    qmlRegisterType<TestObject>("Test", 1, 0, "TestObject");

    QQmlEngine engine;
    QQmlComponent c(&engine);

    const QString fn(QLatin1String("expressionFromDataComponent.qml"));
    QUrl url = testFileUrl(fn);
    QString path = testFile(fn);

    {
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        c.setData(f.readAll(), url);
    }

    QScopedPointer<TestObject> object;
    object.reset(qobject_cast<TestObject*>(c.create()));
    Q_ASSERT(!object.isNull());

    QQmlExpression expression(object->scriptString());
    QVariant result = expression.evaluate();
    QCOMPARE(result.typeId(), QMetaType::QString);
    QCOMPARE(result.toString(), QStringLiteral("success"));
}

void tst_qqmlexpression::emptyScriptString()
{
    QQmlEngine engine;
    QQmlContext *context = engine.rootContext();
    QVERIFY(context);
    QVERIFY(context->isValid());

    QQmlScriptString empty;
    QVERIFY(empty.isEmpty());

    QQmlExpression expression(empty, context, this);
    QCOMPARE(expression.context(), context);
    QCOMPARE(expression.scopeObject(), this);
    QCOMPARE(expression.expression(), QString());

    const QVariant result = expression.evaluate();
    QVERIFY(!result.isValid());

    QQmlComponent c(&engine, testFileUrl("scriptString.qml"));
    std::unique_ptr<QObject> root { c.create() };
    TestObject *testObj = qobject_cast<TestObject*>(root.get());
    QVERIFY(testObj != nullptr);

    QQmlScriptString script = testObj->scriptString();
    QVERIFY(!script.isEmpty());

    // verify that comparing against an empty script string does not crash
    QVERIFY(script != empty);
}

QTEST_MAIN(tst_qqmlexpression)

#include "tst_qqmlexpression.moc"
