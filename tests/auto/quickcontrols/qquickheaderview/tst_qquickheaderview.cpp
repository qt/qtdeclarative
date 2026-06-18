// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>
#include <QtQuickTest/quicktest.h>

#include <QAbstractItemModelTester>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickheaderview_p.h>
#include <QtQuickTemplates2/private/qquicklabel_p.h>
#include <private/qquickheaderview_p_p.h>
#include <private/qquickheaderviewdelegate_p.h>
#include <QtQmlModels/private/qqmlsortfilterproxymodel_p.h>

using namespace QQuickVisualTestUtils;

class TestTableModel : public QAbstractTableModel {
    Q_OBJECT
    Q_PROPERTY(int rowCount READ rowCount WRITE setRowCount NOTIFY rowCountChanged)
    Q_PROPERTY(int columnCount READ columnCount WRITE setColumnCount NOTIFY columnCountChanged)

public:
    TestTableModel(QObject *parent = nullptr)
        : QAbstractTableModel(parent)
    {
    }

    int rowCount(const QModelIndex &index = QModelIndex()) const override
    {
        if (index.isValid())
            return 0;
        return m_rows;
    }
    virtual void setRowCount(int count)
    {
        beginResetModel();
        m_rows = count;
        emit rowCountChanged();
        endResetModel();
    }

    int columnCount(const QModelIndex &index = QModelIndex()) const override
    {
        if (index.isValid())
            return 0;
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
    enum Role {
        CustomRole = Qt::UserRole
    };

    void setRowCount(int count) override
    {
        vData.resize(count);
        tableData.resize(count);

        for (auto &row : tableData)
            row.resize(columnCount());

        TestTableModel::setRowCount(count);
    }

    void setColumnCount(int count) override
    {
        hData.resize(count);

        for (auto &row : tableData)
            row.resize(count);

        TestTableModel::setColumnCount(count);
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        switch (role) {
        case CustomRole:
            return QString("%1-%2").arg(index.column()).arg(index.row());
        case Qt::DisplayRole:
            if (index.row() >= 0 && index.row() < tableData.size()
                && index.column() >= 0 && index.column() < tableData.at(index.row()).size()
                && tableData.at(index.row()).at(index.column()).isValid()) {
                return tableData.at(index.row()).at(index.column());
            }
        default:
            return TestTableModel::data(index, role);
        }
    }

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override
    {
        if (!index.isValid())
            return false;

        if (role == Qt::DisplayRole) {
            if (index.row() < 0 || index.row() >= tableData.size())
                return false;

            if (index.column() < 0 || index.column() >= tableData.at(index.row()).size())
                return false;

            if (tableData[index.row()][index.column()] == value)
                return false;

            tableData[index.row()][index.column()] = value;
            emit dataChanged(index, index, { role });
            return true;
        }

        return TestTableModel::setData(index, value, role);
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
        case Qt::InitialSortOrderRole:
            if (orientation == Qt::Horizontal)
                return horizontalSortOrder.value(section);
            return QVariant();
        case CustomRole:
            return (orientation == Qt::Horizontal ? "c" : "r") + QString::number(section);
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
        if (role == Qt::InitialSortOrderRole) {
            if (orientation != Qt::Horizontal)
                return false;

            if (horizontalSortOrder.value(section) == value)
                return false;

            if (value.isValid())
                horizontalSortOrder.insert(section, value);
            else
                horizontalSortOrder.remove(section);

            emit headerDataChanged(orientation, section, section);
            return true;
        }

        auto &data = orientation == Qt::Horizontal ? hData : vData;
        data[section] = value;
        emit headerDataChanged(orientation, section, section);
        return true;
    }

    Q_INVOKABLE QHash<int, QByteArray> roleNames() const override
    {
        auto names = TestTableModel::roleNames();
        names[CustomRole] = "customRole";
        return names;
    }

private:
    QList<QVariant> hData, vData;
    QHash<int, QVariant> horizontalSortOrder;
    QVector<QVector<QVariant>> tableData;
};

class tst_QQuickHeaderView : public QQmlDataTest {
    Q_OBJECT

public:
    tst_QQuickHeaderView();

    QPoint getContextRowAndColumn(const QQuickItem *item) const;

private slots:
    void initTestCase() override;
    void cleanupTestCase();
    void init() override;
    void cleanup();

    void defaults();
    void testHeaderDataProxyModel();
    void testOrientation();
    void testModel();
    void listModel();

    void resizableHandlerBlockingEvents();
    void headerData();
    void warnMissingDefaultRole();
    void dragInvalidItemDuringReorder();
    void horizontalHeaderViewWithListModel_data();
    void horizontalHeaderViewWithListModel();
    void reorderEmptyModel();
    void horizontalHeaderClickedWithoutSyncView();
    void sorting_data();
    void sorting();
    void sortingAfterColumnMoved();

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

tst_QQuickHeaderView::tst_QQuickHeaderView()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::FailOnWarnings)
{
}

QPoint tst_QQuickHeaderView::getContextRowAndColumn(const QQuickItem *item) const
{
    const auto context = qmlContext(item);
    const int row = context->contextProperty("row").toInt();
    const int column = context->contextProperty("column").toInt();
    return QPoint(column, row);
}

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
    QQmlDataTest::init();

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
    // Make sure that the window is shown at this point, so that the test
    // behaves similarly on all platforms
    QVERIFY(QTest::qWaitForWindowActive(qobject_cast<QWindow *>(root.data())));

    // If we want to make use of syncDirection, we need to set syncView as well.
    // For that we need to create a second dummy table view.
    QQuickTableView otherView;

    auto hhv = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);
    QCOMPARE(hhv->columns(), 10);
    QCOMPARE(hhv->rows(), 1);
    auto vhv = root->findChild<QQuickVerticalHeaderView *>("verticalHeader");
    QVERIFY(vhv);

    hhv->setSyncView(&otherView);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*Setting syncDirection other than Qt::Horizontal is invalid."));
    hhv->setSyncDirection(Qt::Vertical);
    QVERIFY(QQuickTest::qWaitForPolish(hhv));

    vhv->setSyncView(&otherView);
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        ".*Setting syncDirection other than Qt::Vertical is invalid."));
    vhv->setSyncDirection(Qt::Horizontal);
    QVERIFY(QQuickTest::qWaitForPolish(vhv));

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
    QCOMPARE(modelChangedSpy.size(), 0);

    hhv->setModel(QVariant::fromValue(pm));
    QCOMPARE(modelChangedSpy.size(), 1);

    TestTableModel ttm2;
    ttm2.setRowCount(100);
    ttm2.setColumnCount(30);
    hhv->setModel(QVariant::fromValue(&ttm2));
    QCOMPARE(modelChangedSpy.size(), 2);
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

    auto getDelegate = [](QQuickHeaderViewBase *headerView, qreal x,
                          qreal y) -> QQuickHeaderViewDelegate * {
        auto *item = headerView->childAt(x, y)->childAt(x, y);
        return qobject_cast<QQuickHeaderViewDelegate*>(item);
    };

    auto hhvCell1 = getDelegate(hhv, 0, 0);
    QVERIFY(hhvCell1);
    QVERIFY(hhvCell1->contentItem());
    QCOMPARE(hhvCell1->contentItem()->property("text"), "AAA");

    auto hhvCell2 = getDelegate(hhv, hhvCell1->width() + 5, 0);
    QVERIFY(hhvCell2);
    QVERIFY(hhvCell2->contentItem());
    QCOMPARE(hhvCell2->contentItem()->property("text"), "BBB");

    auto vhvCell1 = getDelegate(vhv, 0, 0);
    QVERIFY(vhvCell1);
    QVERIFY(vhvCell1->contentItem());
    QCOMPARE(vhvCell1->contentItem()->property("text"), "111");

    auto vhvCell2 = getDelegate(vhv, 0, vhvCell1->height() + 5);
    QVERIFY(vhvCell2);
    QVERIFY(vhvCell2->contentItem());
    QCOMPARE(vhvCell2->contentItem()->property("text"), "222");
}

// A header shouldn't block events outside of itself.
void tst_QQuickHeaderView::resizableHandlerBlockingEvents()
{
    QQuickApplicationHelper helper(this, QStringLiteral("resizableHandlerBlockingEvents.qml"));
    QVERIFY2(helper.errorMessage.isEmpty(), helper.errorMessage);
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto mouseArea = window->findChild<QQuickMouseArea *>("mouseArea");
    QVERIFY(mouseArea);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, mapCenterToWindow(mouseArea));
    QVERIFY(mouseArea->isPressed());
}

void tst_QQuickHeaderView::headerData()
{
    QQuickApplicationHelper helper(this, QStringLiteral("headerData.qml"));
    QVERIFY2(helper.errorMessage.isEmpty(), helper.errorMessage);
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto headerView = window->property("headerView").value<QQuickHeaderViewBase *>();
    QVERIFY(headerView);
    const auto firstHeaderCell = headerView->itemAtIndex(headerView->index(0, 0));
    QVERIFY(firstHeaderCell);
    const auto delegate = qobject_cast<QQuickHeaderViewDelegate *>(firstHeaderCell);
    QVERIFY(delegate);
    QVERIFY(delegate->contentItem());
    QCOMPARE(delegate->contentItem()->property("text").toString(), QStringLiteral("c0"));
}

void tst_QQuickHeaderView::warnMissingDefaultRole()
{
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*toolTip.*"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Required property.*"));
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("TableView.*"));
    QQuickApplicationHelper helper(this, QStringLiteral("DefaultRoles.qml"));
    QVERIFY2(helper.errorMessage.isEmpty(), helper.errorMessage);
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
}

void tst_QQuickHeaderView::dragInvalidItemDuringReorder()
{
    QQuickApplicationHelper helper(this, QStringLiteral("reorderHeader.qml"));
    QVERIFY2(helper.errorMessage.isEmpty(), helper.errorMessage);
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto hhv = window->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);

    const auto item = hhv->itemAtIndex(hhv->index(0, 0));
    QQuickWindow *itemWindow = item->window();

    QSignalSpy columnMovedSpy(hhv, SIGNAL(columnMoved(int,int,int)));
    QVERIFY(columnMovedSpy.isValid());

    const QPoint localPos = QPoint(item->width() - 5, item->height() - 5);
    const QPoint startPos = itemWindow->contentItem()->mapFromItem(item, localPos).toPoint();
    const QPoint startDragDist = QPoint(0, qApp->styleHints()->startDragDistance() + 1);
    const QPoint dragLength(0, 100);

    QTest::mousePress(itemWindow, Qt::LeftButton, Qt::NoModifier, startPos);
    QTest::mouseMove(itemWindow, startPos + startDragDist);
    QTest::mouseMove(itemWindow, startPos + dragLength);
    QTest::mouseRelease(itemWindow, Qt::LeftButton, Qt::NoModifier, startPos + dragLength);

    QVERIFY(!QQuickTest::qIsPolishScheduled(item));
    QCOMPARE(columnMovedSpy.size(), 0);
}

void tst_QQuickHeaderView::horizontalHeaderViewWithListModel_data()
{
    QTest::addColumn<QVariant>("model");

    const QStringList stringModel = {"one", "two", "three"};

    QTest::newRow("Number model") << QVariant::fromValue(3);
    QTest::newRow("List model") << QVariant::fromValue(stringModel);
}

void tst_QQuickHeaderView::horizontalHeaderViewWithListModel()
{
    // Check that HorizontalHeaderView will be transposed when using a list model
    QFETCH(QVariant, model);

    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("Window.qml"));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QVERIFY(QTest::qWaitForWindowActive(qobject_cast<QWindow *>(root.data())));

    auto hhv = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);

    hhv->setModel(model);
    QVERIFY(QQuickTest::qWaitForPolish(hhv));

    // Check that the list elements are laid out in a row-major order
    QCOMPARE(hhv->columns(), 3);
    QCOMPARE(hhv->rows(), 1);

    for (int col = 0; col < 3; ++col) {
        auto item = hhv->itemAtCell({col, 0});
        QVERIFY(item);
        const QPoint contextCell = getContextRowAndColumn(item);
        QCOMPARE(contextCell, QPoint(0, col));
    }
}

void tst_QQuickHeaderView::reorderEmptyModel()
{
    QQuickApplicationHelper helper(this, QStringLiteral("reorderEmptyModel.qml"));
    QVERIFY2(helper.errorMessage.isEmpty(), helper.errorMessage);
    QQuickWindow *window = helper.window;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto hhv = window->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hhv);

    QSignalSpy columnMovedSpy(hhv, SIGNAL(columnMoved(int,int,int)));
    QVERIFY(columnMovedSpy.isValid());
    hhv->moveColumn(0, 1);
    QVERIFY(!columnMovedSpy.isEmpty());
}

void tst_QQuickHeaderView::horizontalHeaderClickedWithoutSyncView()
{
    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("horizontalHeaderClickedWithoutSyncView.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    auto *window = qobject_cast<QWindow *>(root.data());
    QVERIFY(QTest::qWaitForWindowActive(window));

    auto hh = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hh);

    QSignalSpy headerClickedSpy(hh, SIGNAL(headerClicked(int)));
    QVERIFY(headerClickedSpy.isValid());

    const QPointF localPos = QPointF(hh->width() / 6, hh->height() / 2);
    const QPoint point = hh->mapToScene(localPos).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, point);

    QCOMPARE(headerClickedSpy.size(), 1);
}

void tst_QQuickHeaderView::sorting_data()
{
    QTest::addColumn<bool>("sortIndicatorClearable");
    QTest::addColumn<bool>("sortingEnabled");
    QTest::addColumn<QVariant>("initialSortOrder");

    QTest::addColumn<int>("sortColumnAfterFirstClick");
    QTest::addColumn<Qt::SortOrder>("sortOrderAfterFirstClick");
    QTest::addColumn<QStringList>("modelAfterFirstClick");

    QTest::addColumn<int>("sortColumnAfterSecondClick");
    QTest::addColumn<Qt::SortOrder>("sortOrderAfterSecondClick");
    QTest::addColumn<QStringList>("modelAfterSecondClick");

    QTest::addColumn<int>("sortColumnAfterThirdClick");
    QTest::addColumn<Qt::SortOrder>("sortOrderAfterThirdClick");
    QTest::addColumn<QStringList>("modelAfterThirdClick");

    const QStringList defaultOrder = { "cat", "dog", "bird", "fish"};
    const QStringList ascendingOrder = { "bird", "cat", "dog", "fish"};
    const QStringList descendingOrder = { "fish", "dog", "cat", "bird"};

    QTest::newRow("clearable, enabled, default sortOrder")
            << true << true << QVariant()
            << 0 << Qt::AscendingOrder << ascendingOrder
            << 0 << Qt::DescendingOrder << descendingOrder
            << -1 << Qt::AscendingOrder << defaultOrder;

    QTest::newRow("not clearable, enabled, default sortOrder")
            << false << true << QVariant()
            << 0 << Qt::AscendingOrder << ascendingOrder
            << 0 << Qt::DescendingOrder << descendingOrder
            << 0 << Qt::AscendingOrder << ascendingOrder;

    QTest::newRow("clearable, disabled, default sortOrder")
            << true << false << QVariant()
            << -1 << Qt::AscendingOrder << defaultOrder
            << -1 << Qt::AscendingOrder << defaultOrder
            << -1 << Qt::AscendingOrder << defaultOrder;

    QTest::newRow("clearable, enabled, initial descending sortOrder")
            << true << true << QVariant::fromValue(int(Qt::DescendingOrder))
            << 0 << Qt::DescendingOrder << descendingOrder
            << 0 << Qt::AscendingOrder << ascendingOrder
            << -1 << Qt::AscendingOrder << defaultOrder;
}

void tst_QQuickHeaderView::sorting()
{
    QFETCH(bool, sortIndicatorClearable);
    QFETCH(bool, sortingEnabled);
    QFETCH(QVariant, initialSortOrder);
    QFETCH(int, sortColumnAfterFirstClick);
    QFETCH(Qt::SortOrder, sortOrderAfterFirstClick);
    QFETCH(QStringList, modelAfterFirstClick);
    QFETCH(int, sortColumnAfterSecondClick);
    QFETCH(Qt::SortOrder, sortOrderAfterSecondClick);
    QFETCH(QStringList, modelAfterSecondClick);
    QFETCH(int, sortColumnAfterThirdClick);
    QFETCH(Qt::SortOrder, sortOrderAfterThirdClick);
    QFETCH(QStringList, modelAfterThirdClick);

    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("sorting.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    auto *window = qobject_cast<QWindow *>(root.data());
    QVERIFY(QTest::qWaitForWindowActive(window));

    auto hh = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hh);

    auto *tv = root->findChild<QQuickTableView *>("tableView");
    QVERIFY(tv);

    hh->setSortIndicatorClearable(sortIndicatorClearable);
    tv->setSortingEnabled(sortingEnabled);

    auto *sortModel = tv->model().value<QQmlSortFilterProxyModel *>();
    QVERIFY2(sortModel, "The view's model type is not QQmlSortFilterProxyModel");
    auto *tableModel = qobject_cast<TestTableModelWithHeader *>(sortModel->sourceModel());
    QVERIFY2(tableModel, "The sort model's source model is not TestTableModelWithHeader");

    tableModel->setHeaderData(0, Qt::Horizontal, QStringLiteral("Name"), Qt::DisplayRole);
    tableModel->setHeaderData(1, Qt::Horizontal, QStringLiteral("Color"), Qt::DisplayRole);

    if (initialSortOrder.isValid())
        tableModel->setHeaderData(0, Qt::Horizontal, initialSortOrder, Qt::InitialSortOrderRole);

    tableModel->setHeaderData(0, Qt::Vertical, QStringLiteral("1"), Qt::DisplayRole);
    tableModel->setHeaderData(1, Qt::Vertical, QStringLiteral("2"), Qt::DisplayRole);
    tableModel->setHeaderData(2, Qt::Vertical, QStringLiteral("3"), Qt::DisplayRole);
    tableModel->setHeaderData(3, Qt::Vertical, QStringLiteral("4"), Qt::DisplayRole);

    tableModel->setData(tableModel->index(0,0), "cat", Qt::DisplayRole);
    tableModel->setData(tableModel->index(0,1), "black", Qt::DisplayRole);
    tableModel->setData(tableModel->index(1,0), "dog", Qt::DisplayRole);
    tableModel->setData(tableModel->index(1,1), "brown", Qt::DisplayRole);
    tableModel->setData(tableModel->index(2,0), "bird", Qt::DisplayRole);
    tableModel->setData(tableModel->index(2,1), "white", Qt::DisplayRole);
    tableModel->setData(tableModel->index(3,0), "fish", Qt::DisplayRole);
    tableModel->setData(tableModel->index(3,1), "gold", Qt::DisplayRole);

    QCOMPARE(tableModel->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(), "Name");
    QCOMPARE(tableModel->data(tableModel->index(0,0), Qt::DisplayRole).toString(), "cat");

    QCOMPARE(tableModel->rowCount(), 4);
    QCOMPARE(tableModel->columnCount(), 2);

    QCOMPARE(tv->sortColumn(), -1);
    QCOMPARE(tv->sortOrder(), Qt::AscendingOrder);

    auto modelColumnData = [](QAbstractItemModel *model, int column) {
        QStringList list;
        for (int row = 0; row < model->rowCount(); ++row)
            list << model->data(model->index(row, column), Qt::DisplayRole).toString();
        return list;
    };

    const QPointF localPos = QPointF(hh->columnWidth(0) / 2, hh->height() / 2);
    const QPoint pos = hh->mapToScene(localPos).toPoint();

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
    QCOMPARE(tv->sortColumn(), sortColumnAfterFirstClick);
    QCOMPARE(tv->sortOrder(), sortOrderAfterFirstClick);
    QCOMPARE(modelColumnData(sortModel, 0), modelAfterFirstClick);

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
    QCOMPARE(tv->sortColumn(), sortColumnAfterSecondClick);
    QCOMPARE(tv->sortOrder(), sortOrderAfterSecondClick);
    QCOMPARE(modelColumnData(sortModel, 0), modelAfterSecondClick);

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
    QCOMPARE(tv->sortColumn(), sortColumnAfterThirdClick);
    QCOMPARE(tv->sortOrder(), sortOrderAfterThirdClick);
    QCOMPARE(modelColumnData(sortModel, 0), modelAfterThirdClick);
}

void tst_QQuickHeaderView::sortingAfterColumnMoved()
{
    QQmlComponent component(engine);
    component.loadUrl(testFileUrl("sortingAfterColumnMoved.qml"));

    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));

    auto *window = qobject_cast<QWindow *>(root.data());
    QVERIFY(QTest::qWaitForWindowActive(window));

    auto hh = root->findChild<QQuickHorizontalHeaderView *>("horizontalHeader");
    QVERIFY(hh);

    auto *tv = root->findChild<QQuickTableView *>("tableView");
    QVERIFY(tv);

    hh->setSortIndicatorClearable(true);
    tv->setSortingEnabled(true);

    auto *sortModel = tv->model().value<QQmlSortFilterProxyModel *>();
    QVERIFY2(sortModel, "The view's model type is not QQmlSortFilterProxyModel");
    auto *tableModel = qobject_cast<TestTableModelWithHeader *>(sortModel->sourceModel());
    QVERIFY2(tableModel, "The sort model's source model is not TestTableModelWithHeader");

    tableModel->setHeaderData(0, Qt::Horizontal, QStringLiteral("Name"), Qt::DisplayRole);
    tableModel->setHeaderData(1, Qt::Horizontal, QStringLiteral("Color"), Qt::DisplayRole);

    tableModel->setHeaderData(0, Qt::Vertical, QStringLiteral("1"), Qt::DisplayRole);
    tableModel->setHeaderData(1, Qt::Vertical, QStringLiteral("2"), Qt::DisplayRole);

    tableModel->setData(tableModel->index(0, 0), QStringLiteral("cat"), Qt::DisplayRole);
    tableModel->setData(tableModel->index(0, 1), QStringLiteral("brown"), Qt::DisplayRole);
    tableModel->setData(tableModel->index(1, 0), QStringLiteral("dog"), Qt::DisplayRole);
    tableModel->setData(tableModel->index(1, 1), QStringLiteral("black"), Qt::DisplayRole);

    QCOMPARE(tableModel->headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(), "Name");
    QCOMPARE(tableModel->data(tableModel->index(0,0), Qt::DisplayRole).toString(), "cat");

    QCOMPARE(tableModel->rowCount(), 2);
    QCOMPARE(tableModel->columnCount(), 2);

    QCOMPARE(tv->sortColumn(), -1);
    QCOMPARE(tv->sortOrder(), Qt::AscendingOrder);

    QSignalSpy columnMovedSpy(hh, SIGNAL(columnMoved(int,int,int)));
    QVERIFY(columnMovedSpy.isValid());
    hh->moveColumn(0, 1);
    QVERIFY(!columnMovedSpy.isEmpty());

    const QPointF localPos = QPointF(hh->columnWidth(0) / 2, hh->height() / 2);
    const QPoint pos = hh->mapToScene(localPos).toPoint();

    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
    QCOMPARE(tv->sortColumn(), 1);
    QCOMPARE(tv->sortOrder(), Qt::AscendingOrder);
    QCOMPARE(sortModel->data(sortModel->index(0,1), Qt::DisplayRole).toString(), "black");
}

QTEST_MAIN(tst_QQuickHeaderView)

#include "tst_qquickheaderview.moc"
