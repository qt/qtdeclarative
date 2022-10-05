// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtCore/QConcatenateTablesProxyModel>
#include <QtGui/QStandardItemModel>
#include <QtQml/qqmlcomponent.h>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>
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
    void valueWithoutCallingObjectFirst_data();
    void valueWithoutCallingObjectFirst();
    void qtbug_86017();
    void filterOnGroup_removeWhenCompleted();
    void contextAccessedByHandler();
    void redrawUponColumnChange();
    void nestedDelegates();
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

QTEST_MAIN(tst_QQmlDelegateModel)

#include "tst_qqmldelegatemodel.moc"
