/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "../shared/qtest_quickcontrols.h"
#include "../shared/util.h"
#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

#include <QAbstractItemModelTester>
#include <QtQml/QQmlEngine>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickheaderview_p.h>
#include <private/qquickheaderview_p_p.h>

class TestTableModel : public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount WRITE setRowCount NOTIFY rowCountChanged)
    Q_PROPERTY(int columnCount READ columnCount WRITE setColumnCount NOTIFY columnCountChanged)

public:
    TestTableModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    int rowCount(const QModelIndex & = QModelIndex()) const override
    {
        return m_rows;
    }
    virtual void setRowCount(int count)
    {
        beginResetModel();
        m_rows = count;
        emit rowCountChanged();
        endResetModel();
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override
    {
        return m_cols;
    }
    virtual void setColumnCount(int count)
    {
        beginResetModel();
        m_cols = count;
        emit columnCountChanged();
        endResetModel();
    }

    int indexValue(const QModelIndex &index) const
    {
        return index.row() + (index.column() * rowCount());
    }

    Q_INVOKABLE QModelIndex toQModelIndex(int serialIndex)
    {
        return createIndex(serialIndex % rowCount(), serialIndex / rowCount());
    }

    Q_INVOKABLE QVariant data(int row, int col)
    {
        return data(createIndex(row, col), Qt::DisplayRole);
    }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        switch (role) {
        case Qt::DisplayRole:
            return QString("%1, %2, checked: %3 ")
                .arg(index.row())
                .arg(index.column())
                .arg(m_checkedCells.contains(indexValue(index)));
        case Qt::EditRole:
            return m_checkedCells.contains(indexValue(index));
        default:
            return QVariant();
        }
    }

    bool setData(const QModelIndex &index, const QVariant &value,
        int role = Qt::EditRole) override
    {

        if (role != Qt::EditRole)
            return false;

        int i = indexValue(index);
        bool checked = value.toBool();
        if (checked == m_checkedCells.contains(i))
            return false;

        if (checked)
            m_checkedCells.insert(i);
        else
            m_checkedCells.remove(i);

        emit dataChanged(index, index, { role });
        return true;
    }

    Q_INVOKABLE QHash<int, QByteArray> roleNames() const override
    {
        return {
            { Qt::DisplayRole, "display" },
            { Qt::EditRole, "edit" }
        };
    }

signals:
    void rowCountChanged();
    void columnCountChanged();

private:
    int m_rows = 0;
    int m_cols = 0;

    QSet<int> m_checkedCells;
};

class TestTableModelWithHeader : public TestTableModel {

    Q_OBJECT
public:
    void setRowCount(int count) override
    {
        vData.resize(count);
        TestTableModel::setRowCount(count);
    }

    void setColumnCount(int count) override
    {
        hData.resize(count);
        TestTableModel::setColumnCount(count);
    }
    Q_INVOKABLE QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override
    {
        auto sectionCount = orientation == Qt::Horizontal ? columnCount() : rowCount();
        if (section < 0 || section >= sectionCount)
            return QVariant();
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole: {
            auto &data = orientation == Qt::Horizontal ? hData : vData;
            return data[section].toString();
        }
        default:
            return QVariant();
        }
    }
    Q_INVOKABLE bool setHeaderData(int section, Qt::Orientation orientation,
        const QVariant &value, int role = Qt::EditRole) override
    {
        qDebug() << Q_FUNC_INFO
                 << "section:" << section
                 << "orient:" << orientation
                 << "value:" << value
                 << "role:" << QAbstractItemModel::roleNames()[role];
        auto sectionCount = orientation == Qt::Horizontal ? columnCount() : rowCount();
        if (section < 0 || section >= sectionCount)
            return false;
        auto &data = orientation == Qt::Horizontal ? hData : vData;
        data[section] = value;
        emit headerDataChanged(orientation, section, section);
        return true;
    }

private:
    QVector<QVariant> hData, vData;
};

class tst_QQuickHeaderView : public QQmlDataTest {
    Q_OBJECT

private slots:
    void initTestCase() override;
    void cleanupTestCase();
    void init();
    void cleanup();

    void defaults();
    void testHeaderDataProxyModel();
    void testOrientation();
    void testModel();
    void listModel();

private:
    QQmlEngine *engine;
    QString errorString;

    std::unique_ptr<QObject> rootObjectFromQml(const char *file)
    {
        auto component = new QQmlComponent(engine);
        component->loadUrl(testFileUrl(file));
        auto root = component->create();
        if (!root)
            errorString = component->errorString();
        return std::unique_ptr<QObject>(new QObject(root));
    }
};

void tst_QQuickHeaderView::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<TestTableModel>("TestTableModel", 0, 1, "TestTableModel");
    qmlRegisterType<TestTableModelWithHeader>("TestTableModelWithHeader", 0, 1, "TestTableModelWithHeader");
    qmlRegisterType<QHeaderDataProxyModel>("HeaderDataProxyModel", 0, 1, "HeaderDataProxyModel");
}

void tst_QQuickHeaderView::cleanupTestCase()
{
}

void tst_QQuickHeaderView::init()
{
    engine = new QQmlEngine(this);
}

void tst_QQuickHeaderView::cleanup()
{
    if (engine) {
        delete engine;
        engine = nullptr;
    }
}

void tst_QQuickHeaderView::defaults()
{
    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("Window.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    auto hhv = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);
    auto vhv = root->findChild<QQuickVerticalHeaderView *>("verticalHeader");
    QVERIFY(vhv);
    auto tm = root->findChild<TestTableModel *>("tableModel");
    QVERIFY(tm);
    auto pm = root->findChild<QHeaderDataProxyModel *>("proxyModel");
    QVERIFY(pm);
    auto tv = root->findChild<QQuickTableView *>("tableView");
    QVERIFY(tv);
}

void tst_QQuickHeaderView::testHeaderDataProxyModel()
{
    TestTableModel model;
    model.setColumnCount(10);
    model.setRowCount(7);
    QHeaderDataProxyModel model2;
    model2.setSourceModel(&model);
    QAbstractItemModelTester tester(&model2, QAbstractItemModelTester::FailureReportingMode::QtTest);
}

void tst_QQuickHeaderView::testOrientation()
{
    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("Window.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    auto hhv = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);
    QCOMPARE(hhv->columns(), 10);
    QCOMPARE(hhv->rows(), 1);
    auto vhv = root->findChild<QQuickVerticalHeaderView *>("verticalHeader");
    QVERIFY(vhv);

    hhv->setSyncDirection(Qt::Vertical);
    hhv->flick(10, 20);

    vhv->setSyncDirection(Qt::Horizontal);
    vhv->flick(20, 10);

    QVERIFY(QTest::qWaitForWindowActive(qobject_cast<QWindow *>(root.data())));
    // Explicitly setting a different synDirection is ignored
    QCOMPARE(hhv->syncDirection(), Qt::Horizontal);
    QCOMPARE(hhv->flickableDirection(), QQuickFlickable::HorizontalFlick);
    QCOMPARE(vhv->syncDirection(), Qt::Vertical);
    QCOMPARE(vhv->flickableDirection(), QQuickFlickable::VerticalFlick);
}

void tst_QQuickHeaderView::testModel()
{
    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("Window.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    auto hhv = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);
    auto thm = root->findChild<TestTableModel *>("tableHeaderModel");
    QVERIFY(thm);
    auto pm = root->findChild<QHeaderDataProxyModel *>("proxyModel");
    QVERIFY(pm);

    QSignalSpy modelChangedSpy(hhv, SIGNAL(modelChanged()));
    QVERIFY(modelChangedSpy.isValid());

    hhv->setModel(QVariant::fromValue(thm));
    QCOMPARE(modelChangedSpy.count(), 0);

    hhv->setModel(QVariant::fromValue(pm));
    QCOMPARE(modelChangedSpy.count(), 1);

    TestTableModel ttm2;
    ttm2.setRowCount(100);
    ttm2.setColumnCount(30);
    hhv->setModel(QVariant::fromValue(&ttm2));
    QCOMPARE(modelChangedSpy.count(), 2);
}

void tst_QQuickHeaderView::listModel()
{
    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("ListModel.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    if (!QTest::qWaitForWindowActive(qobject_cast<QWindow *>(root.data())))
        QSKIP("Window failed to become active!");

    auto hhv = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);
    auto vhv = root->findChild<QQuickVerticalHeaderView *>("verticalHeader");
    QVERIFY(vhv);

    auto hhvCell1 = hhv->childAt(0, 0)->childAt(0, 0)->findChild<QQuickText *>();
    QVERIFY(hhvCell1);
    QCOMPARE(hhvCell1->property("text"), "AAA");

    auto hhvCell2 = hhv->childAt(hhvCell1->width() + 5, 0)->
                        childAt(hhvCell1->width() + 5, 0)->findChild<QQuickText *>();
    QVERIFY(hhvCell2);
    QCOMPARE(hhvCell2->property("text"), "BBB");

    auto vhvCell1 = vhv->childAt(0, 0)->childAt(0, 0)->findChild<QQuickText *>();
    QVERIFY(vhvCell1);
    QCOMPARE(vhvCell1->property("text"), "111");

    auto vhvCell2 = vhv->childAt(0, vhvCell1->height() + 5)->
                        childAt(0, vhvCell1->height() + 5)->findChild<QQuickText *>();
    QVERIFY(vhvCell2);
    QCOMPARE(vhvCell2->property("text"), "222");
}

QTEST_MAIN(tst_QQuickHeaderView)

#include "tst_qquickheaderview.moc"
