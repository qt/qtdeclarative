// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtCore/QConcatenateTablesProxyModel>
#include <QtGui/QStandardItemModel>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>
#include <QtQmlModels/private/qqmllistmodel_p.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/qquickitem.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtTest/QSignalSpy>

class tst_QQmlDelegateModel : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQmlDelegateModel();

private slots:
    void resettingRolesRespected();
    void valueWithoutCallingObjectFirst_data();
    void valueWithoutCallingObjectFirst();
    void qtbug_86017();
    void filterOnGroup_removeWhenCompleted();
    void contextAccessedByHandler();
    void redrawUponColumnChange();
    void nestedDelegates();
    void typedModelData();
    void deleteRace();
    void persistedItemsStayInCache();
    void doNotUnrefObjectUnderConstruction();
};

class AbstractItemModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    AbstractItemModel()
    {
        for (int i = 0; i < 3; ++i)
            mValues.append(QString::fromLatin1("Item %1").arg(i));
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return QModelIndex();

        return createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex &) const override
    {
        return QModelIndex();
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid())
            return 0;

        return mValues.size();
    }

    int columnCount(const QModelIndex &parent) const override
    {
        if (parent.isValid())
            return 0;

        return 1;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (role != Qt::DisplayRole)
            return QVariant();

        return mValues.at(index.row());
    }

private:
    QVector<QString> mValues;
};

tst_QQmlDelegateModel::tst_QQmlDelegateModel()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qmlRegisterType<AbstractItemModel>("Test", 1, 0, "AbstractItemModel");
}

class TableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    int rowCount(const QModelIndex & = QModelIndex()) const override
    {
        return 1;
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override
    {
        return 1;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        switch (role) {
        case 0:
            return QString("foo: %1, %2").arg(index.column()).arg(index.row());
        case 1:
            return 42;
        default:
            break;
        }

        return QVariant();
    }

    Q_INVOKABLE void change() { beginResetModel(); toggle = !toggle; endResetModel(); }

    QHash<int, QByteArray> roleNames() const override
    {
        if (toggle)
            return { {0, "foo"} };
        else
            return { {1, "bar"} };
    }

    bool toggle = true;
};

void tst_QQmlDelegateModel::resettingRolesRespected()
{
    auto model = std::make_unique<TableModel>();
    QQmlApplicationEngine engine;
    engine.setInitialProperties({ {"model", QVariant::fromValue(model.get()) }} );
    engine.load(testFileUrl("resetModelData.qml"));
    QTRY_VERIFY(!engine.rootObjects().isEmpty());
    QObject *root = engine.rootObjects().constFirst();
    QVERIFY(!root->property("success").toBool());
    model->change();
    QTRY_VERIFY(root->property("success").toBool());
}

void tst_QQmlDelegateModel::valueWithoutCallingObjectFirst_data()
{
    QTest::addColumn<QUrl>("qmlFileUrl");
    QTest::addColumn<int>("index");
    QTest::addColumn<QString>("role");
    QTest::addColumn<QVariant>("expectedValue");

    QTest::addRow("integer") << testFileUrl("integerModel.qml")
        << 50 << QString::fromLatin1("modelData") << QVariant(50);
    QTest::addRow("ListModel") << testFileUrl("listModel.qml")
        << 1 << QString::fromLatin1("name") << QVariant(QLatin1String("Item 1"));
    QTest::addRow("QAbstractItemModel") << testFileUrl("abstractItemModel.qml")
        << 1 << QString::fromLatin1("display") << QVariant(QLatin1String("Item 1"));
}

// Tests that it's possible to call variantValue() without creating
// costly delegate items first via object().
void tst_QQmlDelegateModel::valueWithoutCallingObjectFirst()
{
    QFETCH(const QUrl, qmlFileUrl);
    QFETCH(const int, index);
    QFETCH(const QString, role);
    QFETCH(const QVariant, expectedValue);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(qmlFileUrl);
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QQmlDelegateModel *model = qobject_cast<QQmlDelegateModel*>(root.data());
    QVERIFY(model);
    QCOMPARE(model->variantValue(index, role), expectedValue);
}

void tst_QQmlDelegateModel::qtbug_86017()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl("qtbug_86017.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QTRY_VERIFY(component.isReady());
    QQmlDelegateModel *model = qobject_cast<QQmlDelegateModel*>(root.data());

    QVERIFY(model);
    QCOMPARE(model->count(), 2);
    QCOMPARE(model->filterGroup(), "selected");
}

void tst_QQmlDelegateModel::filterOnGroup_removeWhenCompleted()
{
    QQuickView view(testFileUrl("removeFromGroup.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QQuickItem *root = view.rootObject();
    QVERIFY(root);
    QQmlDelegateModel *model = root->findChild<QQmlDelegateModel*>();
    QVERIFY(model);
    QVERIFY(QTest::qWaitFor([=]{ return model->count() == 2; }));
}

void tst_QQmlDelegateModel::contextAccessedByHandler()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("contextAccessedByHandler.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QVERIFY(root->property("works").toBool());
}

void tst_QQmlDelegateModel::redrawUponColumnChange()
{
    QStandardItemModel m1;
    m1.appendRow({
            new QStandardItem("Banana"),
            new QStandardItem("Coconut"),
    });

    QQuickView view(testFileUrl("redrawUponColumnChange.qml"));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.show();
    QQuickItem *root = view.rootObject();
    root->setProperty("model", QVariant::fromValue<QObject *>(&m1));

    QObject *item = root->property("currentItem").value<QObject *>();
    QVERIFY(item);
    QCOMPARE(item->property("text").toString(), "Banana");

    QVERIFY(root);
    m1.removeColumn(0);

    QCOMPARE(item->property("text").toString(), "Coconut");
}

void tst_QQmlDelegateModel::nestedDelegates()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("nestedDelegates.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QQuickItem *item = qobject_cast<QQuickItem *>(o.data());
    QCOMPARE(item->childItems().size(), 2);
    for (QQuickItem *child : item->childItems()) {
        if (child->objectName() != QLatin1String("loader"))
            continue;

        QCOMPARE(child->childItems().size(), 1);
        QQuickItem *timeMarks = child->childItems().at(0);
        const QList<QQuickItem *> children = timeMarks->childItems();
        QCOMPARE(children.size(), 2);

        // One of them is the repeater, the other one is the rectangle
        QVERIFY(children.at(0)->objectName() == QLatin1String("zap")
                 || children.at(1)->objectName() == QLatin1String("zap"));
        QVERIFY(children.at(0)->objectName().isEmpty() || children.at(1)->objectName().isEmpty());

        return; // loader found
    }
    QFAIL("Loader not found");
}

void tst_QQmlDelegateModel::typedModelData()
{
    QQmlEngine engine;
    const QUrl url = testFileUrl("typedModelData.qml");
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());

    QQmlDelegateModel *delegateModel = qobject_cast<QQmlDelegateModel *>(o.data());
    QVERIFY(delegateModel);

    for (int i = 0; i < 3; ++i) {
        if (i == 0) {
            for (int j = 0; j < 2; ++j) {
                QTest::ignoreMessage(
                    QtWarningMsg,
                    "Could not find any constructor for value type QQmlPointFValueType "
                    "to call with value QVariant(double, 11)");
            }

            QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(url.toString() + ":48:9: Unable to assign double to QPointF"));
        } else if (i == 1) {
            QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(url.toString() + ":44:9: Unable to assign [undefined] to double"));
        }

        delegateModel->setProperty("n", i);
        QObject *delegate = delegateModel->object(0);
        QVERIFY(delegate);
        switch (i) {
        case 0: {
            // list model with 1 role.
            // Does not work, for the most part, because the model is singular
            QCOMPARE(delegate->property("modelX"), 11.0);
            QCOMPARE(delegate->property("modelDataX"), 0.0);
            QCOMPARE(delegate->property("modelSelf"), QPointF(11.0, 0.0));
            QCOMPARE(delegate->property("modelDataSelf"), QPointF());
            QCOMPARE(delegate->property("modelModelData"), QPointF());
            break;
        }
        case 1: {
            // JS array of objects
            QCOMPARE(delegate->property("modelDataX"), 17.0);
            const QPointF modelData = delegate->property("modelDataSelf").value<QPointF>();
            QCOMPARE(modelData, QPointF(17, 18));
            QCOMPARE(delegate->property("modelModelData").value<QPointF>(), modelData);
            break;
        }
        case 2: {
            // single object
            QCOMPARE(delegate->property("modelX"), 21);
            QCOMPARE(delegate->property("modelDataX"), 21);
            const QPointF modelData = delegate->property("modelDataSelf").value<QPointF>();
            QCOMPARE(modelData, QPointF(21, 22));
            QCOMPARE(delegate->property("modelSelf"), QVariant::fromValue(modelData));
            QCOMPARE(delegate->property("modelModelData"), QVariant::fromValue(modelData));
            break;
        }
        default:
            QFAIL("wrong model number");
            break;
        }
    }

}

void tst_QQmlDelegateModel::deleteRace()
{
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("deleteRace.qml"));
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));
    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());
    QTRY_COMPARE(o->property("count").toInt(), 2);
    QTRY_COMPARE(o->property("count").toInt(), 0);
}

void tst_QQmlDelegateModel::persistedItemsStayInCache()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("persistedItemsCache.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);
    const QVariant properyListModel = object->property("testListModel");
    QQmlListModel *listModel = qvariant_cast<QQmlListModel *>(properyListModel);
    QVERIFY(listModel);
    QTRY_COMPARE(object->property("createCount").toInt(), 3);
    listModel->clear();
    QTRY_COMPARE(object->property("destroyCount").toInt(), 3);
}

void tst_QQmlDelegateModel::doNotUnrefObjectUnderConstruction()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("modifyObjectUnderConstruction.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);
    QTRY_COMPARE(object->property("testModel").toInt(), 0);
}

QTEST_MAIN(tst_QQmlDelegateModel)

#include "tst_qqmldelegatemodel.moc"
