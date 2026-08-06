// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtCore/QScopedPointer>
#include <QtQml/private/qqmlanybinding_p.h>
#include <QtQml/private/qqmlcontextdata_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtCore/private/qobject_p.h>
#include "withbindable.h"

class tst_qqmlanybinding : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qqmlanybinding();

private slots:
    void basicActions_data();
    void basicActions();
    void unboundQQmlPropertyBindingDoesNotCrash();
    void ofDynamicMetaObject();
    void installOnAliasOfBindable();
    void installOnAliasOfInterceptedBindable();
    void installOnBindableInterceptedInBaseType();
};

tst_qqmlanybinding::tst_qqmlanybinding()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qqmlanybinding::basicActions_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addRow("old style") << QStringLiteral("oldstylebinding.qml");
    QTest::addRow("new style") << QStringLiteral("newstylebinding.qml");
}

static int getRefCount(const QQmlAnyBinding &binding)
{
    if (binding.isAbstractPropertyBinding()) {
        return binding.asAbstractBinding()->ref;
    } else {
        // this temporarily adds a refcount because we construc a new untypedpropertybinding
        // thus -1
        return QPropertyBindingPrivate::get(binding.asUntypedPropertyBinding())->refCount() - 1;
    }
}

void tst_qqmlanybinding::basicActions()
{
    QQmlEngine engine;
    QFETCH(QString, fileName);
    QQmlComponent comp(&engine, testFileUrl(fileName));
    QScopedPointer<QObject> root(comp.create());
    QVERIFY2(root, qPrintable(comp.errorString()));

    QQmlProperty prop(root.get(), "prop");
    QQmlProperty trigger(root.get(), "trigger");

    // sanity check
    QCOMPARE(prop.read().toInt(), 1);

    // We can take the binding from a property
    auto binding = QQmlAnyBinding::ofProperty(prop);
    if (fileName == u"oldstylebinding.qml") {
        // and it is a QQmlAbstractBinding,
        QVERIFY(binding.isAbstractPropertyBinding());
    } else {
        // and it is a new style binding,
        QVERIFY(binding.isUntypedPropertyBinding());
    }
    // which is not null.
    QVERIFY(binding);
    // Getting the binding did not remove it.
    trigger.write(42);
    QCOMPARE(prop.read().toInt(), 42);
    // remove does that,
    QQmlAnyBinding::removeBindingFrom(prop);
    QVERIFY(!QQmlAnyBinding::ofProperty(prop));
    // but does not change the value.
    QCOMPARE(prop.read().toInt(), 42);
    // As there is no binding anymore, there won't be any change if trigger changes.
    trigger.write(2);
    QCOMPARE(prop.read().toInt(), 42);
    // The binding we took is still valid (and we are currently the sole owner).
    QVERIFY(getRefCount(binding) == 1);
    // We can reinstall the binding
    binding.installOn(prop);
    // This changes the value to reflect that trigger has changed
    QCOMPARE(prop.read().toInt(), 2);
    // The binding behaves normally.
    trigger.write(3);
    QCOMPARE(prop.read().toInt(), 3);
    // "binding" still has a reference to the binding
    QVERIFY(getRefCount(binding) == 2);

    // aliases are resolved correctly
    QQmlProperty a2(root.get(), "a2");
    QCOMPARE(QQmlAnyBinding::ofProperty(a2), binding);
}

void tst_qqmlanybinding::unboundQQmlPropertyBindingDoesNotCrash()
{
    QQmlEngine engine;
    QQmlComponent comp(&engine, testFileUrl("unboundBinding.qml"));
    QScopedPointer<QObject> root(comp.create());
    QVERIFY2(root, qPrintable(comp.errorString()));

    QQmlProperty prop(root.get(), "prop");
    QQmlProperty trigger(root.get(), "trigger");

    // Store a reference to the binding of prop.
    auto binding = QQmlAnyBinding::ofProperty(prop);
    QVERIFY(binding.isUntypedPropertyBinding());
    QVERIFY(!binding.asUntypedPropertyBinding().isNull());

    // If we break the binding,
    prop.write(42);
    QCOMPARE(prop.read(), 42);
    {
        auto noBinding = QQmlAnyBinding::ofProperty(prop);
        QVERIFY(noBinding.asUntypedPropertyBinding().isNull());
    }
    // and trigger binding reevaluation
    trigger.write(1);
    // there is no crash.
    // When we reinstall the binding,
    binding.installOn(prop);
    // the value is correctly updated.
    QCOMPARE(prop.read(), 1);
}

class QObjectDynamicMetaObject : public QDynamicMetaObjectData
{
public:
#if QT_VERSION >= QT_VERSION_CHECK(7, 0, 0)
    const QMetaObject *toDynamicMetaObject(QObject *) const final
    {
        return &QObject::staticMetaObject;
    }
#else
    QMetaObject *toDynamicMetaObject(QObject *) final
    {
        return const_cast<QMetaObject *>(&QObject::staticMetaObject);
    }
#endif

    int metaCall(QObject *o, QMetaObject::Call c, int id, void **argv) final
    {
        return o->qt_metacall(c, id, argv);
    }
};

void tst_qqmlanybinding::ofDynamicMetaObject()
{
    QObject o;
    const QQmlPropertyIndex objectNameIndex(
            QObject::staticMetaObject.indexOfProperty("objectName"));
    QObjectPrivate::get(&o)->metaObject = new QObjectDynamicMetaObject;

    QQmlAnyBinding a = QQmlAnyBinding::ofProperty(&o, objectNameIndex);
    QVERIFY(!a);

    QPropertyBinding<QString> binding
            = Qt::makePropertyBinding([]() { return QStringLiteral("foo"); });
    o.bindableObjectName().setBinding(binding);
    QQmlAnyBinding b = QQmlAnyBinding::ofProperty(&o, objectNameIndex);
    QVERIFY(b);

    QCOMPARE(QPropertyBindingPrivate::get(b.asUntypedPropertyBinding()),
             QPropertyBindingPrivate::get(binding));
}

void tst_qqmlanybinding::installOnAliasOfBindable()
{
    QQmlEngine engine;
    QQmlComponent comp(&engine, testFileUrl("aliasOfBindable.qml"));
    QScopedPointer<QObject> root(comp.create());
    QVERIFY2(root, qPrintable(comp.errorString()));

    QQmlProperty aliasProp(root.get(), "aliasProp");
    QVERIFY(aliasProp.isValid());
    QVERIFY(aliasProp.isBindable());

    QQmlAnyBinding binding = QQmlAnyBinding::createFromCodeString(
            aliasProp, QStringLiteral("trigger + 1"), root.get(),
            QQmlContextData::get(qmlContext(root.get())), QString(), 0);
    QVERIFY(binding.isUntypedPropertyBinding());

    binding.installOn(aliasProp);
    QCOMPARE(aliasProp.read().toInt(), 2);

    QQmlProperty trigger(root.get(), "trigger");
    trigger.write(41);
    QCOMPARE(aliasProp.read().toInt(), 42);
}

void tst_qqmlanybinding::installOnAliasOfInterceptedBindable()
{
    QQmlEngine engine;
    QQmlComponent comp(&engine, testFileUrl("aliasOfInterceptedBindable.qml"));
    QScopedPointer<QObject> root(comp.create());
    QVERIFY2(root, qPrintable(comp.errorString()));

    QObject *inner = qvariant_cast<QObject *>(root->property("inner"));
    QVERIFY(inner);
    auto *interceptor = inner->findChild<BindableInterceptor *>();
    QVERIFY(interceptor);

    QQmlProperty aliasProp(root.get(), "aliasProp");
    QVERIFY(aliasProp.isBindable());

    QQmlAnyBinding binding = QQmlAnyBinding::createFromCodeString(
            aliasProp, QStringLiteral("trigger + 1"), root.get(),
            QQmlContextData::get(qmlContext(root.get())), QString(), 0);
    QVERIFY(binding.isUntypedPropertyBinding());

    binding.installOn(aliasProp);
    QCOMPARE(interceptor->bindableCount, 0);
    QCOMPARE(aliasProp.read().toInt(), 2);

    QQmlProperty trigger(root.get(), "trigger");
    trigger.write(41);
    QCOMPARE(aliasProp.read().toInt(), 42);

    // RespectInterceptors still routes through the interceptor.
    QQmlAnyBinding intercepted = QQmlAnyBinding::createFromCodeString(
            aliasProp, QStringLiteral("trigger + 2"), root.get(),
            QQmlContextData::get(qmlContext(root.get())), QString(), 0);
    intercepted.installOn(aliasProp, QQmlAnyBinding::RespectInterceptors);
    QCOMPARE(interceptor->bindableCount, 1);
}

void tst_qqmlanybinding::installOnBindableInterceptedInBaseType()
{
    QQmlEngine engine;
    QQmlComponent comp(&engine, testFileUrl("derivedOfInterceptedBindable.qml"));
    QScopedPointer<QObject> root(comp.create());
    QVERIFY2(root, qPrintable(comp.errorString()));

    auto *interceptor = root->findChild<BindableInterceptor *>();
    QVERIFY(interceptor);

    QQmlProperty prop(root.get(), "prop");
    QVERIFY(prop.isBindable());

    QQmlAnyBinding binding = QQmlAnyBinding::createFromCodeString(
            prop, QStringLiteral("trigger + 1"), root.get(),
            QQmlContextData::get(qmlContext(root.get())), QString(), 0);
    QVERIFY(binding.isUntypedPropertyBinding());

    binding.installOn(prop);
    QCOMPARE(interceptor->bindableCount, 0);
    QCOMPARE(prop.read().toInt(), 2);

    // Same, but reached through an alias, so that the alias resolution and the
    // parent chain walk have to cooperate.
    QQmlProperty aliasProp(root.get(), "aliasProp");
    QVERIFY(aliasProp.isBindable());
    QQmlAnyBinding aliasBinding = QQmlAnyBinding::createFromCodeString(
            aliasProp, QStringLiteral("trigger + 2"), root.get(),
            QQmlContextData::get(qmlContext(root.get())), QString(), 0);
    aliasBinding.installOn(aliasProp);
    QCOMPARE(interceptor->bindableCount, 0);
    QCOMPARE(prop.read().toInt(), 3);

    QQmlProperty trigger(root.get(), "trigger");
    trigger.write(41);
    QCOMPARE(prop.read().toInt(), 43);

    // RespectInterceptors still routes through the interceptor.
    QQmlAnyBinding intercepted = QQmlAnyBinding::createFromCodeString(
            prop, QStringLiteral("trigger + 3"), root.get(),
            QQmlContextData::get(qmlContext(root.get())), QString(), 0);
    intercepted.installOn(prop, QQmlAnyBinding::RespectInterceptors);
    QCOMPARE(interceptor->bindableCount, 1);
}

QTEST_MAIN(tst_qqmlanybinding)

#include "tst_qqmlanybinding.moc"
