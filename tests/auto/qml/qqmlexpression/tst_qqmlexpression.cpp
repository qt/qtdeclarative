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

using namespace Qt::StringLiterals;

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
    void scriptStringFromDataWithoutUrl();
    void changingScopeObject_data();
    void changingScopeObject();
    void shadowingInContextHierarchy_data();
    void shadowingInContextHierarchy();
    void contextPropertyNamedLikeGlobal_data();
    void contextPropertyNamedLikeGlobal();
    void idInDifferentContext_data();
    void idInDifferentContext();
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
    std::unique_ptr<QObject> root(c.create());
    TestObject *testObj = qobject_cast<TestObject*>(root.get());
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

void tst_qqmlexpression::scriptStringFromDataWithoutUrl()
{
    qmlRegisterType<TestObject>("Test", 1, 0, "TestObject");

    QQmlEngine engine;
    QQmlComponent c(&engine);

    QFile f(testFile("scriptString.qml"));
    QVERIFY(f.open(QIODevice::ReadOnly));
    c.setData(f.readAll(), QUrl());

    std::unique_ptr<QObject> root(c.create());
    QVERIFY2(root, qPrintable(c.errorString()));
    TestObject *testObj = qobject_cast<TestObject *>(root.get());
    QVERIFY(testObj);

    // The script string has to run from the compiled function. There is no source text.
    QQmlExpression expression(testObj->scriptString());
    QCOMPARE(expression.evaluate().toInt(), 15);
    QVERIFY(!expression.hasError());
}

void tst_qqmlexpression::changingScopeObject_data()
{
    QTest::addColumn<QByteArray>("access");
    QTest::addColumn<QByteArray>("firstScope");
    QTest::addColumn<QByteArray>("firstContextObject");
    QTest::addColumn<QByteArray>("secondScope");
    QTest::addColumn<QByteArray>("secondContextObject");

    for (const QByteArray &access : { "load"_ba, "call"_ba }) {
        const char *a = access.constData();
        QTest::addRow("%s, scope object loses the name", a)
                << access << "withFoo"_ba << "root"_ba << "withoutFoo"_ba << "root"_ba;
        QTest::addRow("%s, scope object gains the name", a)
                << access << "withoutFoo"_ba << "root"_ba << "withFoo"_ba << "root"_ba;
        QTest::addRow("%s, context object loses the name", a)
                << access << "withoutFoo"_ba << "root"_ba << "withoutFoo"_ba << "withoutFoo"_ba;
        QTest::addRow("%s, context object gains the name", a)
                << access << "withoutFoo"_ba << "withoutFoo"_ba << "withoutFoo"_ba << "root"_ba;
        QTest::addRow("%s, scope object gains the name in front of the outer context", a)
                << access << "withoutFoo"_ba << "withoutFoo"_ba << "withFoo"_ba << "withoutFoo"_ba;
    }
}

void tst_qqmlexpression::changingScopeObject()
{
    QFETCH(QByteArray, access);
    QFETCH(QByteArray, firstScope);
    QFETCH(QByteArray, firstContextObject);
    QFETCH(QByteArray, secondScope);
    QFETCH(QByteArray, secondContextObject);

    qmlRegisterType<TestObject>("Test", 1, 0, "TestObject");

    QQmlEngine engine;
    QQmlContext outerContext(engine.rootContext());
    QQmlComponent c(&engine, testFileUrl("changingScopeObject.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    std::unique_ptr<QObject> root(c.create(&outerContext));
    QVERIFY(root);

    const auto objectNamed = [&](const QByteArray &name) {
        return name == "root" ? root.get() : root->property(name).value<QObject *>();
    };

    // The context object of the outer context provides the name once the context object of the
    // evaluation context doesn't. Unlike the component's own context, the evaluation context
    // allows its context object to be replaced.
    outerContext.setContextObject(objectNamed("outer"_ba));
    QQmlContext evaluationContext(&outerContext);

    const QQmlScriptString script
            = objectNamed(access)->property("scriptString").value<QQmlScriptString>();

    // The same script string runs the same function, with the same lookups, in any scope object
    // and context. Where the name is found must not depend on what an earlier run found in a
    // different scope object or context object.
    const QByteArray scopes[] = { firstScope, secondScope };
    const QByteArray contextObjects[] = { firstContextObject, secondContextObject };
    const QByteArray tag = QTest::currentDataTag();
    for (qsizetype run = 0; run < 2; ++run) {
        evaluationContext.setContextObject(objectNamed(contextObjects[run]));
        QObject *scope = objectNamed(scopes[run]);
        QVERIFY(scope);
        const QList<QObject *> candidates
                = { scope, evaluationContext.contextObject(), outerContext.contextObject() };
        const auto holder = std::find_if(candidates.begin(), candidates.end(), [](QObject *o) {
            return o->property("foo").isValid();
        });
        QVERIFY(holder != candidates.end());

        QQmlExpression expression(script, &evaluationContext, scope);
        const QVariant result = expression.evaluate();
        QVERIFY2(!expression.hasError(), qPrintable(expression.error().toString()));

        if (run == 1) {
            if (tag.endsWith("scope object gains the name")) {
                QEXPECT_FAIL("",
                             "The cached lookup of a context object property doesn't check the "
                             "scope object. See QQmlContextWrapper::lookupContextObjectProperty()",
                             Abort);
            } else if (tag.endsWith("context object gains the name")
                       || tag.endsWith("scope object gains the name in front of the outer context")) {
                QEXPECT_FAIL("",
                             "The cached lookup in the parent context hierarchy doesn't check the "
                             "immediate scope object and context object. "
                             "See QQmlContextWrapper::lookupInParentContextHierarchy()",
                             Abort);
            }
        }
        QCOMPARE(result, (*holder)->property("foo"));
    }
}

// The places where an unqualified name can be found, in the order they are searched. Each of them
// provides the value of its position plus one.
static const QByteArray nameHolders[] = {
    "inner.property"_ba, "scope"_ba, "inner.object"_ba,
    "middle.property"_ba, "middle.object"_ba,
    "outer.property"_ba, "outer.object"_ba,
};

static int nameHolderValue(const QByteArray &holder)
{
    const auto it = std::find(std::begin(nameHolders), std::end(nameHolders), holder);
    Q_ASSERT(it != std::end(nameHolders));
    return int(std::distance(std::begin(nameHolders), it)) + 1;
}

static QByteArray nameHolderOf(const QVariant &value)
{
    bool ok = false;
    const int index = value.toInt(&ok) - 1;
    if (!ok || index < 0 || index >= int(std::size(nameHolders)))
        return "<none: " + value.toString().toUtf8() + ">";
    return nameHolders[index];
}

void tst_qqmlexpression::shadowingInContextHierarchy_data()
{
    QTest::addColumn<QByteArray>("access");
    QTest::addColumn<QByteArrayList>("firstHolders");
    QTest::addColumn<QByteArrayList>("secondHolders");

    for (const QByteArray &access : { "load"_ba, "call"_ba }) {
        const char *a = access.constData();
        QTest::addRow("%s, innermost context property shadows the scope object", a)
                << access << QByteArrayList{ "scope" }
                << QByteArrayList{ "inner.property", "scope" };
        QTest::addRow("%s, innermost context property shadows the context object", a)
                << access << QByteArrayList{ "inner.object" }
                << QByteArrayList{ "inner.property", "inner.object" };
        QTest::addRow("%s, innermost context property shadows a parent context", a)
                << access << QByteArrayList{ "middle.object" }
                << QByteArrayList{ "inner.property", "middle.object" };
        QTest::addRow("%s, intermediate context property shadows an outer context", a)
                << access << QByteArrayList{ "outer.object" }
                << QByteArrayList{ "middle.property", "outer.object" };
        QTest::addRow("%s, intermediate context object shadows an outer context", a)
                << access << QByteArrayList{ "outer.object" }
                << QByteArrayList{ "middle.object", "outer.object" };
        QTest::addRow("%s, intermediate context property stops shadowing", a)
                << access << QByteArrayList{ "middle.property", "outer.object" }
                << QByteArrayList{ "outer.object" };
        QTest::addRow("%s, intermediate context object stops shadowing", a)
                << access << QByteArrayList{ "middle.object", "outer.object" }
                << QByteArrayList{ "outer.object" };
        QTest::addRow("%s, outer context property shadows its context object", a)
                << access << QByteArrayList{ "outer.object" }
                << QByteArrayList{ "outer.property", "outer.object" };
    }
}

void tst_qqmlexpression::shadowingInContextHierarchy()
{
    QFETCH(QByteArray, access);
    QFETCH(QByteArrayList, firstHolders);
    QFETCH(QByteArrayList, secondHolders);

    qmlRegisterType<TestObject>("Test", 1, 0, "TestObject");

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("shadowingInContextHierarchy.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    std::unique_ptr<QObject> root(c.create());
    QVERIFY(root);

    QQmlComponent *withName = root->property("withName").value<QQmlComponent *>();
    QVERIFY(withName);
    QQmlComponent *withoutName = root->property("withoutName").value<QQmlComponent *>();
    QVERIFY(withoutName);

    const QQmlScriptString script
            = root->property(access).value<QObject *>()->property("scriptString")
                      .value<QQmlScriptString>();
    const QString name = access == "load" ? u"foo"_s : u"getFoo"_s;

    // The same script string runs the same function, with the same lookups, in any scope object
    // and context. Where the name is found must not depend on what an earlier run found in a
    // different context hierarchy. All objects with the name are of the same type, and so are
    // all objects without it.
    const QByteArray tag = QTest::currentDataTag();
    const QByteArrayList allHolders[] = { firstHolders, secondHolders };
    for (qsizetype run = 0; run < 2; ++run) {
        const QByteArrayList &holders = allHolders[run];
        std::vector<std::unique_ptr<QObject>> objects;
        const auto createObject = [&](const QByteArray &holder) {
            QObject *object = holders.contains(holder)
                    ? withName->createWithInitialProperties(
                              { { u"foo"_s, nameHolderValue(holder) } })
                    : withoutName->create();
            objects.emplace_back(object);
            return object;
        };

        const auto setupContext = [&](QQmlContext *context, const QByteArray &level) {
            context->setContextObject(createObject(level + ".object"));
            const QByteArray property = level + ".property";
            if (!holders.contains(property))
                return;
            const int value = nameHolderValue(property);
            if (access == "load") {
                context->setContextProperty(name, value);
            } else {
                context->setContextProperty(
                        name, engine.evaluate(u"(function() { return %1 })"_s.arg(value))
                                      .toVariant(QJSValue::RetainJSObjects));
            }
        };

        QQmlContext outer(engine.rootContext());
        setupContext(&outer, "outer");
        QQmlContext middle(&outer);
        setupContext(&middle, "middle");
        QQmlContext inner(&middle);
        setupContext(&inner, "inner");
        QObject *scope = createObject("scope");

        const auto holder = std::find_if(
                std::begin(nameHolders), std::end(nameHolders),
                [&](const QByteArray &h) { return holders.contains(h); });
        QVERIFY(holder != std::end(nameHolders));

        QQmlExpression expression(script, &inner, scope);
        const QVariant result = expression.evaluate();
        QVERIFY2(!expression.hasError(), qPrintable(expression.error().toString()));

        if (run == 1) {
            if (tag.endsWith("innermost context property shadows the scope object")
                    || tag.endsWith("innermost context property shadows the context object")) {
                QEXPECT_FAIL("",
                             "The cached lookup of a scope object or context object property "
                             "doesn't check the properties of the immediate context. See "
                             "QQmlContextWrapper::lookupScopeObjectProperty() and "
                             "lookupContextObjectProperty()",
                             Abort);
            } else if (tag.endsWith("innermost context property shadows a parent context")) {
                QEXPECT_FAIL("",
                             "The cached lookup in the parent context hierarchy doesn't check "
                             "the properties of the immediate context. See "
                             "QQmlContextWrapper::lookupInParentContextHierarchy()",
                             Abort);
            }
        }
        QCOMPARE(nameHolderOf(result), *holder);
    }
}

void tst_qqmlexpression::contextPropertyNamedLikeGlobal_data()
{
    QTest::addColumn<int>("depth");

    QTest::addRow("immediate context") << 0;
    QTest::addRow("parent context") << 1;
    QTest::addRow("root context") << 2;
}

void tst_qqmlexpression::contextPropertyNamedLikeGlobal()
{
    QFETCH(int, depth);

    QQmlEngine engine;
    QQmlContext parent(engine.rootContext());
    QQmlContext immediate(&parent);
    QQmlContext *contexts[] = { &immediate, &parent, engine.rootContext() };

    contexts[depth]->setContextProperty(
            u"Math"_s, engine.evaluate(u"({ tag: 'context property' })"_s)
                               .toVariant(QJSValue::RetainJSObjects));

    QQmlExpression expression(&immediate, nullptr, u"Math.tag"_s);
    const QVariant result = expression.evaluate();
    QVERIFY2(!expression.hasError(), qPrintable(expression.error().toString()));

    // The immediate context comes before the global object. The remaining contexts come after it.
    if (depth == 0) {
        QCOMPARE(result.toString(), u"context property"_s);
        return;
    }

    QEXPECT_FAIL("", "The context properties of all contexts are searched before the global "
                     "object, see QQmlContextWrapper::getPropertyAndBase()", Abort);
    QVERIFY(!result.isValid());
}

void tst_qqmlexpression::idInDifferentContext_data()
{
    QTest::addColumn<bool>("firstInComponentContext");

    QTest::addRow("id in parent context, then in innermost context") << false;
    QTest::addRow("id in innermost context, then in parent context") << true;
}

void tst_qqmlexpression::idInDifferentContext()
{
    QFETCH(bool, firstInComponentContext);

    qmlRegisterType<TestObject>("Test", 1, 0, "TestObject");

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("idInDifferentContext.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    std::unique_ptr<QObject> root(c.create());
    QVERIFY(root);

    QObject *named = root->property("named").value<QObject *>();
    QVERIFY(named);
    const QQmlScriptString script
            = root->property("load").value<QObject *>()->property("scriptString")
                      .value<QQmlScriptString>();

    // A context created by the user has no ids. Its parent context does. Both have the same
    // context object, and the scope object is the same, too.
    QQmlContext *componentContext = qmlContext(root.get());
    QQmlContext userContext(componentContext);
    userContext.setContextObject(root.get());

    QQmlContext *contexts[] = { &userContext, componentContext };
    if (firstInComponentContext)
        std::swap(contexts[0], contexts[1]);

    for (QQmlContext *context : contexts) {
        QQmlExpression expression(script, context, root.get());
        const QVariant result = expression.evaluate();
        QVERIFY2(!expression.hasError(), qPrintable(expression.error().toString()));
        if (context == componentContext && !firstInComponentContext) {
            QEXPECT_FAIL("",
                         "The cached lookup in the parent context hierarchy doesn't check the "
                         "ids of the immediate context. "
                         "See QQmlContextWrapper::lookupInParentContextHierarchy()",
                         Abort);
        }
        QCOMPARE(result.value<QObject *>(), named);
    }
}

QTEST_MAIN(tst_qqmlexpression)

#include "tst_qqmlexpression.moc"
