// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "delegatemodelkinds.h"
#include "stringmodel.h"

#include <private/qmlutils_p.h>
#include <private/qqmlinstantiator_p.h>

#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlincubator.h>

#include <QtCore/qdebug.h>
#include <QtCore/qsortfilterproxymodel.h>

class tst_qqmlinstantiator: public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qqmlinstantiator();

private slots:
    void createNone();
    void createSingle();
    void createMultiple();
    void stringModel();
    void activeProperty();
    void activeModelChangeInteraction();
    void intModelChange();
    void createAndRemove();
    void removeDuringModelChange();

    void asynchronous_data();
    void asynchronous();

    void handlerWithParent();
    void boundDelegateComponent();

    void listDataDestruction();

    void setDelegateAfterModel();

    void delegateModelAccess_data();
    void delegateModelAccess();

    void replaceDelegate();
};

tst_qqmlinstantiator::tst_qqmlinstantiator()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qmlRegisterNamespaceAndRevisions(&Model::staticMetaObject, "Test", 1);
    qmlRegisterNamespaceAndRevisions(&Delegate::staticMetaObject, "Test", 1);
}

void tst_qqmlinstantiator::createNone()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("createNone.qml"));
    QScopedPointer<QObject> o(component.create());
    QQmlInstantiator *instantiator = qobject_cast<QQmlInstantiator*>(o.data());
    QVERIFY(instantiator != nullptr);
    QCOMPARE(instantiator->isActive(), true);
    QCOMPARE(instantiator->count(), 0);
    QCOMPARE(instantiator->property("success").toBool(), true);
    QVERIFY(instantiator->delegate()->isReady());
}

void tst_qqmlinstantiator::createSingle()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("createSingle.qml"));
    QScopedPointer<QObject> o(component.create());
    QQmlInstantiator *instantiator = qobject_cast<QQmlInstantiator*>(o.data());
    QVERIFY(instantiator != nullptr);
    QCOMPARE(instantiator->isActive(), true);
    QCOMPARE(instantiator->count(), 1);
    QVERIFY(instantiator->delegate()->isReady());

    QObject *object = instantiator->object();
    QVERIFY(object);
    QCOMPARE(object->parent(), instantiator);
    QCOMPARE(object->property("success").toBool(), true);
    QCOMPARE(object->property("idx").toInt(), 0);
}

void tst_qqmlinstantiator::createMultiple()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("createMultiple.qml"));
    QScopedPointer<QObject> o(component.create());
    QQmlInstantiator *instantiator = qobject_cast<QQmlInstantiator*>(o.data());
    QVERIFY(instantiator != nullptr);
    QCOMPARE(instantiator->isActive(), true);
    QCOMPARE(instantiator->count(), 10);

    for (int i=0; i<10; i++) {
        QObject *object = instantiator->objectAt(i);
        QVERIFY(object);
        QCOMPARE(object->parent(), instantiator);
        QCOMPARE(object->property("success").toBool(), true);
        QCOMPARE(object->property("idx").toInt(), i);
    }
}

void tst_qqmlinstantiator::stringModel()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("stringModel.qml"));
    QScopedPointer<QObject> o(component.create());
    QQmlInstantiator *instantiator = qobject_cast<QQmlInstantiator*>(o.data());
    QVERIFY(instantiator != nullptr);
    QCOMPARE(instantiator->isActive(), true);
    QCOMPARE(instantiator->count(), 4);

    for (int i=0; i<4; i++) {
        QObject *object = instantiator->objectAt(i);
        QVERIFY(object);
        QCOMPARE(object->parent(), instantiator);
        QCOMPARE(object->property("success").toBool(), true);
    }
}

void tst_qqmlinstantiator::activeProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("inactive.qml"));
    QScopedPointer<QObject> o(component.create());
    QQmlInstantiator *instantiator = qobject_cast<QQmlInstantiator*>(o.data());
    QVERIFY(instantiator != nullptr);
    QSignalSpy activeSpy(instantiator, SIGNAL(activeChanged()));
    QSignalSpy countSpy(instantiator, SIGNAL(countChanged()));
    QSignalSpy objectSpy(instantiator, SIGNAL(objectChanged()));
    QSignalSpy modelSpy(instantiator, SIGNAL(modelChanged()));
    QCOMPARE(instantiator->isActive(), false);
    QCOMPARE(instantiator->count(), 0);
    QVERIFY(instantiator->delegate()->isReady());

    QCOMPARE(activeSpy.size(), 0);
    QCOMPARE(countSpy.size(), 0);
    QCOMPARE(objectSpy.size(), 0);
    QCOMPARE(modelSpy.size(), 0);

    instantiator->setActive(true);
    QCOMPARE(instantiator->isActive(), true);
    QCOMPARE(instantiator->count(), 1);

    QCOMPARE(activeSpy.size(), 1);
    QCOMPARE(countSpy.size(), 1);
    QCOMPARE(objectSpy.size(), 1);
    QCOMPARE(modelSpy.size(), 0);

    QObject *object = instantiator->object();
    QVERIFY(object);
    QCOMPARE(object->parent(), instantiator);
    QCOMPARE(object->property("success").toBool(), true);
    QCOMPARE(object->property("idx").toInt(), 0);
}

void tst_qqmlinstantiator::activeModelChangeInteraction()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("activeModelChangeInteraction.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY(root);

    // If the instantiator is inactive, a model change does not lead to items being loaded
    bool ok = false;
    int count = root->property("instanceCount").toInt(&ok);
    QVERIFY(ok);
    QCOMPARE(count, 0);

    // When turning the instantiator active, it will however reflect the model
    root->setProperty("active", true);
    count = root->property("instanceCount").toInt(&ok);
    QVERIFY(ok);
    QCOMPARE(count, 3);
}

void tst_qqmlinstantiator::intModelChange()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("createMultiple.qml"));
    QScopedPointer<QObject> o(component.create());
    QQmlInstantiator *instantiator = qobject_cast<QQmlInstantiator*>(o.data());
    QVERIFY(instantiator != nullptr);
    QSignalSpy activeSpy(instantiator, SIGNAL(activeChanged()));
    QSignalSpy countSpy(instantiator, SIGNAL(countChanged()));
    QSignalSpy objectSpy(instantiator, SIGNAL(objectChanged()));
    QSignalSpy modelSpy(instantiator, SIGNAL(modelChanged()));
    QCOMPARE(instantiator->count(), 10);

    QCOMPARE(activeSpy.size(), 0);
    QCOMPARE(countSpy.size(), 0);
    QCOMPARE(objectSpy.size(), 0);
    QCOMPARE(modelSpy.size(), 0);

    instantiator->setModel(QVariant(2));
    QCOMPARE(instantiator->count(), 2);

    QCOMPARE(activeSpy.size(), 0);
    QCOMPARE(countSpy.size(), 1);
    QCOMPARE(objectSpy.size(), 2);
    QCOMPARE(modelSpy.size(), 1);

    for (int i=0; i<2; i++) {
        QObject *object = instantiator->objectAt(i);
        QVERIFY(object);
        QCOMPARE(object->parent(), instantiator);
        QCOMPARE(object->property("success").toBool(), true);
        QCOMPARE(object->property("idx").toInt(), i);
    }
}

void tst_qqmlinstantiator::createAndRemove()
{
    QQmlEngine engine;
    QScopedPointer<StringModel> model {new StringModel("model1")};
    qmlRegisterSingletonInstance("Test", 1, 0, "Model1", model.get());
    QQmlComponent component(&engine, testFileUrl("createAndRemove.qml"));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject != nullptr);

    QQmlInstantiator *instantiator =
        qobject_cast<QQmlInstantiator*>(rootObject->findChild<QObject*>("instantiator1"));
    QVERIFY(instantiator != nullptr);
    model->drop(1);
    QList<QString> names;
    names << "Beta" << "Gamma" << "Delta";
    for (int i=0; i<3; i++) {
        QObject *object = instantiator->objectAt(i);
        QVERIFY(object);
        QCOMPARE(object->property("datum").toString(), names[i]);
    }
}

void tst_qqmlinstantiator::asynchronous_data()
{
    QTest::addColumn<bool>("asyncIncubator");
    QTest::addColumn<QString>("fileName");

    QTest::newRow("Asynchronous Instantiator") << false << "createMultipleAsync.qml";
    QTest::newRow("Nested-asynchronous Instantiator") << true << "createMultiple.qml";
}

void tst_qqmlinstantiator::asynchronous()
{
    QFETCH(bool, asyncIncubator);
    QFETCH(QString, fileName);

    QQmlEngine engine;
    QQmlIncubationController incubationController;
    engine.setIncubationController(&incubationController);
    QQmlComponent component(&engine, testFileUrl(fileName));
    QQmlIncubator incubator(asyncIncubator ? QQmlIncubator::Asynchronous : QQmlIncubator::Synchronous);
    component.create(incubator);
    while (!incubator.isReady())
        incubationController.incubateFor(10);
    QQmlInstantiator *instantiator = qobject_cast<QQmlInstantiator *>(incubator.object());
    while (incubationController.incubatingObjectCount() > 0)
        incubationController.incubateFor(10);
    QVERIFY(instantiator != nullptr);
    QCOMPARE(instantiator->isActive(), true);
    QCOMPARE(instantiator->count(), 10);

    for (int i=0; i<10; i++) {
        QObject *object = instantiator->objectAt(i);
        QVERIFY(object);
        QCOMPARE(object->parent(), instantiator);
        QCOMPARE(object->property("success").toBool(), true);
        QCOMPARE(object->property("idx").toInt(), i);
    }
}

void tst_qqmlinstantiator::handlerWithParent()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("handlerWithParent.qml"));
    QScopedPointer<QObject> rootObject(component.create());
    QVERIFY(rootObject != nullptr);
    const auto handlers = rootObject->findChildren<QObject *>("pointHandler");
    QCOMPARE(handlers.size(), 2);
    for (const auto *h : handlers) {
        QCOMPARE(h->parent(), rootObject.data());
    }
}

void tst_qqmlinstantiator::boundDelegateComponent()
{
    QQmlEngine engine;
    const QUrl url(testFileUrl("boundDelegateComponent.qml"));
    QQmlComponent component(&engine, url);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    for (int i = 0; i < 3; ++i) {
        QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(QLatin1String("%1:12: ReferenceError: modelData is not defined")
                                   .arg(url.toString())));
    }

    QScopedPointer<QObject> o(component.create());
    QVERIFY2(!o.isNull(), qPrintable(component.errorString()));

    QQmlInstantiator *a = qobject_cast<QQmlInstantiator *>(
            qmlContext(o.data())->objectForName(QStringLiteral("undefinedModelData")));
    QVERIFY(a);
    QCOMPARE(a->count(), 3);
    for (int i = 0; i < 3; ++i)
        QCOMPARE(a->objectAt(i)->objectName(), QStringLiteral("rootundefined"));

    QQmlInstantiator *b = qobject_cast<QQmlInstantiator *>(
            qmlContext(o.data())->objectForName(QStringLiteral("requiredModelData")));
    QVERIFY(b);
    QCOMPARE(b->count(), 3);
    QCOMPARE(b->objectAt(0)->objectName(), QStringLiteral("root1"));
    QCOMPARE(b->objectAt(1)->objectName(), QStringLiteral("root2"));
    QCOMPARE(b->objectAt(2)->objectName(), QStringLiteral("root3"));
}

void tst_qqmlinstantiator::listDataDestruction()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("listDataDestruction.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(!o.isNull());
    QCOMPARE(o->objectName(), QLatin1String("A"));
}

void tst_qqmlinstantiator::setDelegateAfterModel()
{
    // TODO: We can't do this trick with DelegateModel on Instantiator because Instantiator
    //       explicitly avoids touching the delegate of a DelegateModel passed in as model.
    //       That's fine, but inconsistent with other views.

    QQmlEngine engine;
    const QUrl url = testFileUrl("setDelegateAfterModel.qml");
    QQmlComponent component(&engine, url);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(!object.isNull());

    // If the model was lost by setting the delegate, the count would be 0.
    QCOMPARE(object->property("count").toInt(), 3);
    object->setProperty("useObjectModel", QVariant::fromValue<bool>(true));
    QCOMPARE(object->property("count").toInt(), 2);
}

class SingleBoolItemModel : public QAbstractListModel
{
    Q_OBJECT

public:
    SingleBoolItemModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return 0;
        return 1;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (index.parent().isValid() || index.row() != 0 || index.column() != 0
                || role != Qt::UserRole)
            return QVariant();

        return m_active;
    }

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role) override {
        if (index.parent().isValid() || index.row() != 0 || index.column() != 0
                || role != Qt::UserRole || m_active == value.toBool())
            return false;

        m_active = value.toBool();
        Q_EMIT dataChanged(index, index, QList<int>{Qt::UserRole});
        return true;
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return { {Qt::UserRole, "active"} };
    }

private:
    bool m_active = true;
};

class FilterBoolRoleProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FilterBoolRoleProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent) {}

    bool filterAcceptsRow(int source_row,
                          const QModelIndex &source_parent) const override
    {
        return sourceModel()->index(source_row, 0, source_parent).data(Qt::UserRole).toBool();
    }
};

void tst_qqmlinstantiator::removeDuringModelChange()
{
    SingleBoolItemModel model;

    FilterBoolRoleProxyModel proxyModel;
    proxyModel.setSourceModel(&model);
    proxyModel.setFilterRole(Qt::UserRole);
    QCOMPARE(proxyModel.rowCount(), 1);

    QQmlEngine engine;
    const QUrl url(testFileUrl("removeDuringModelChange.qml"));
    QQmlComponent component(&engine, url);
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QScopedPointer<QObject> o(component.create());
    QVERIFY2(!o.isNull(), qPrintable(component.errorString()));

    QQmlInstantiator *instantiator = qobject_cast<QQmlInstantiator *>(o.data());

    instantiator->setModel(QVariant::fromValue(&proxyModel));

    QSignalSpy removedSpy(instantiator, &QQmlInstantiator::objectRemoved);
    QMetaObject::invokeMethod(instantiator->object(), "deactivate");

    // We should still be alive at this point.
    QCOMPARE(removedSpy.size(), 1);
    QCOMPARE(proxyModel.rowCount(), 0);
}

template<typename Enum>
const char *enumKey(Enum value) {
    const QMetaObject *mo = qt_getEnumMetaObject(value);
    const QMetaEnum metaEnum = mo->enumerator(mo->indexOfEnumerator(qt_getEnumName(value)));
    return metaEnum.valueToKey(value);
}


void tst_qqmlinstantiator::delegateModelAccess_data()
{
    QTest::addColumn<QQmlDelegateModel::DelegateModelAccess>("access");
    QTest::addColumn<Model::Kind>("modelKind");
    QTest::addColumn<Delegate::Kind>("delegateKind");
    using Access = QQmlDelegateModel::DelegateModelAccess;
    for (auto access : { Access::Qt5ReadWrite, Access::ReadOnly, Access::ReadWrite }) {
        for (auto model : { Model::Singular, Model::List, Model::Array, Model::Object }) {
            for (auto delegate : { Delegate::Untyped, Delegate::Typed }) {
                QTest::addRow("%s-%s-%s", enumKey(access), enumKey(model), enumKey(delegate))
                        << access << model << delegate;
            }
        }
    }
}

void tst_qqmlinstantiator::delegateModelAccess()
{
    QFETCH(QQmlDelegateModel::DelegateModelAccess, access);
    QFETCH(Model::Kind, modelKind);
    QFETCH(Delegate::Kind, delegateKind);

    QQmlEngine engine;
    const QUrl url = testFileUrl("delegateModelAccess.qml");
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> object(c.create());

    QQmlInstantiator *instantiator = qobject_cast<QQmlInstantiator *>(object.data());
    QVERIFY(instantiator);

    QSignalSpy modelChangedSpy(instantiator, &QQmlInstantiator::modelChanged);

    if (delegateKind == Delegate::Untyped && modelKind == Model::Array)
        QSKIP("Properties of objects in arrays are not exposed as context properties");

    if (access == QQmlDelegateModel::ReadOnly) {
        const QRegularExpression message(
                url.toString() + ":[0-9]+: TypeError: Cannot assign to read-only property \"a\"");

        QTest::ignoreMessage(QtWarningMsg, message);
        if (delegateKind == Delegate::Untyped)
            QTest::ignoreMessage(QtWarningMsg, message);
    }
    object->setProperty("delegateModelAccess", access);
    object->setProperty("modelIndex", modelKind);
    object->setProperty("delegateIndex", delegateKind);

    QObject *delegate = instantiator->objectAt(0);
    QVERIFY(delegate);

    const bool modelWritable = access != QQmlDelegateModel::ReadOnly;
    const bool immediateWritable = (delegateKind == Delegate::Untyped)
            ? access != QQmlDelegateModel::ReadOnly
            : access == QQmlDelegateModel::ReadWrite;

    // Only the array is actually updated itself. The other models are pointers
    const bool writeShouldSignal = modelKind == Model::Kind::Array;

    double expected = 11;

    // Initial setting of the model, signals one update
    int expectedModelUpdates = 1;
    QCOMPARE(modelChangedSpy.count(), expectedModelUpdates);

    QCOMPARE(delegate->property("immediateX").toDouble(), expected);
    QCOMPARE(delegate->property("modelX").toDouble(), expected);

    if (modelWritable) {
        expected = 3;
        if (writeShouldSignal)
            ++expectedModelUpdates;
    }

    QMetaObject::invokeMethod(delegate, "writeThroughModel");
    QCOMPARE(delegate->property("immediateX").toDouble(), expected);
    QCOMPARE(delegate->property("modelX").toDouble(), expected);
    QCOMPARE(modelChangedSpy.count(), expectedModelUpdates);

    if (immediateWritable) {
        expected = 1;
        if (writeShouldSignal)
            ++expectedModelUpdates;
    }

    QMetaObject::invokeMethod(delegate, "writeImmediate");

    // Writes to required properties always succeed, but might not be propagated to the model
    QCOMPARE(delegate->property("immediateX").toDouble(),
             delegateKind == Delegate::Untyped ? expected : 1);

    QCOMPARE(delegate->property("modelX").toDouble(), expected);
    QCOMPARE(modelChangedSpy.count(), expectedModelUpdates);
}

void tst_qqmlinstantiator::replaceDelegate()
{
    QTest::failOnWarning();

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("replaceDelegate.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    QTest::ignoreMessage(QtWarningMsg, "created");
    QScopedPointer<QObject> outer(component.create());
    const QUrl url = outer->property("source").value<QUrl>();
    QVERIFY(url.isValid());
    QVERIFY(outer->property("object").value<QObject *>() != nullptr);
    outer->setProperty("source", QUrl());
    QVERIFY(outer->property("object").value<QObject *>() == nullptr);

    // Restoring the delegate causes the object to be created only _once_.
    // See failOnWarning() above.
    QTest::ignoreMessage(QtWarningMsg, "created");
    outer->setProperty("source", url);
    QVERIFY(outer->property("object").value<QObject *>() != nullptr);
}

QTEST_MAIN(tst_qqmlinstantiator)

#include "tst_qqmlinstantiator.moc"
