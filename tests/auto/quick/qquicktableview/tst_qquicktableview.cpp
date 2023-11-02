// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtQuickTest/quicktest.h>

#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicktableview_p.h>
#include <QtQuick/private/qquicktableview_p_p.h>
#include <QtQuick/private/qquickloader_p.h>
#include <QtQuick/private/qquickdraghandler_p.h>
#include <QtQuick/private/qquicktextinput_p.h>

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlincubator.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQmlModels/private/qqmlobjectmodel_p.h>
#include <QtQmlModels/private/qqmllistmodel_p.h>

#include "testmodel.h"

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

using namespace QQuickViewTestUtils;
using namespace QQuickVisualTestUtils;

static const char* kDelegateObjectName = "tableViewDelegate";
static const char *kDelegatesCreatedCountProp = "delegatesCreatedCount";
static const char *kModelDataBindingProp = "modelDataBinding";

Q_DECLARE_METATYPE(QMarginsF);

#define GET_QML_TABLEVIEW(PROPNAME) \
    auto PROPNAME = view->rootObject()->property(#PROPNAME).value<QQuickTableView *>(); \
    QVERIFY(PROPNAME); \
    auto PROPNAME ## Private = QQuickTableViewPrivate::get(PROPNAME); \
    Q_UNUSED(PROPNAME ## Private)

#define LOAD_TABLEVIEW(fileName) \
    view->setSource(testFileUrl(fileName)); \
    view->show(); \
    view->requestActivate(); \
    QVERIFY(QTest::qWaitForWindowActive(view)); \
    GET_QML_TABLEVIEW(tableView)

#define LOAD_TABLEVIEW_ASYNC(fileName) \
    view->setSource(testFileUrl("asyncloader.qml")); \
    view->show(); \
    QVERIFY(QTest::qWaitForWindowActive(view)); \
    auto loader = view->rootObject()->property("loader").value<QQuickLoader *>(); \
    loader->setSourceWithoutResolve(testFileUrl(fileName)); \
    QTRY_VERIFY(loader->item()); \
    QCOMPARE(loader->status(), QQuickLoader::Status::Ready); \
    GET_QML_TABLEVIEW(tableView)

#define WAIT_UNTIL_POLISHED_ARG(item) \
    QVERIFY(QQuickTest::qIsPolishScheduled(item)); \
    QVERIFY(QQuickTest::qWaitForPolish(item))
#define WAIT_UNTIL_POLISHED WAIT_UNTIL_POLISHED_ARG(tableView)

class tst_QQuickTableView : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickTableView();

    QQuickTableViewAttached *getAttachedObject(const QObject *object) const;
    QPoint getContextRowAndColumn(const QQuickItem *item) const;

private:
    QQuickView *view = nullptr;

private slots:
    void initTestCase() override;
    void cleanupTestCase();

    void setAndGetModel_data();
    void setAndGetModel();
    void emptyModel_data();
    void emptyModel();
    void checkPreload_data();
    void checkPreload();
    void checkZeroSizedDelegate();
    void checkImplicitSizeDelegate();
    void checkZeroSizedTableView();
    void checkZeroSizedViewPort();
    void checkColumnWidthWithoutProvider();
    void checkColumnWidthAndRowHeightFunctions();
    void checkDelegateWithAnchors();
    void checkColumnWidthProvider();
    void checkColumnWidthProviderInvalidReturnValues();
    void checkColumnWidthProviderNegativeReturnValue();
    void checkColumnWidthProviderNotCallable();
    void checkColumnWidthBoundToViewWidth();
    void checkRowHeightWithoutProvider();
    void checkRowHeightProvider();
    void checkRowHeightProviderInvalidReturnValues();
    void checkRowHeightProviderNegativeReturnValue();
    void checkRowHeightProviderNotCallable();
    void isColumnLoadedAndIsRowLoaded();
    void checkForceLayoutFunction();
    void checkForceLayoutEndUpDoingALayout();
    void checkForceLayoutInbetweenAddingRowsToModel();
    void checkForceLayoutWhenAllItemsAreHidden();
    void checkLayoutChangedSignal();
    void checkContentWidthAndHeight();
    void checkContentWidthAndHeightForSmallTables();
    void checkPageFlicking();
    void checkExplicitContentWidthAndHeight();
    void checkExtents_origin();
    void checkExtents_endExtent();
    void checkExtents_moveTableToEdge();
    void checkContentXY();
    void noDelegate();
    void changeDelegateDuringUpdate();
    void changeModelDuringUpdate();
    void countDelegateItems_data();
    void countDelegateItems();
    void checkLayoutOfEqualSizedDelegateItems_data();
    void checkLayoutOfEqualSizedDelegateItems();
    void checkFocusRemoved_data();
    void checkFocusRemoved();
    void fillTableViewButNothingMore_data();
    void fillTableViewButNothingMore();
    void checkInitialAttachedProperties_data();
    void checkInitialAttachedProperties();
    void checkSpacingValues();
    void checkDelegateParent();
    void flick_data();
    void flick();
    void flickOvershoot_data();
    void flickOvershoot();
    void checkRowColumnCount();
    void modelSignals();
    void checkModelSignalsUpdateLayout();
    void dataChangedSignal();
    void checkThatPoolIsDrainedWhenReuseIsFalse();
    void checkIfDelegatesAreReused_data();
    void checkIfDelegatesAreReused();
    void checkIfDelegatesAreReusedAsymmetricTableSize();
    void checkContextProperties_data();
    void checkContextProperties();
    void checkContextPropertiesQQmlListProperyModel_data();
    void checkContextPropertiesQQmlListProperyModel();
    void checkRowAndColumnChangedButNotIndex();
    void checkThatWeAlwaysEmitChangedUponItemReused();
    void checkChangingModelFromDelegate();
    void checkRebuildViewportOnly();
    void useDelegateChooserWithoutDefault();
    void checkTableviewInsideAsyncLoader();
    void hideRowsAndColumns_data();
    void hideRowsAndColumns();
    void hideAndShowFirstColumn();
    void hideAndShowFirstRow();
    void checkThatRevisionedPropertiesCannotBeUsedInOldImports();
    void checkSyncView_rootView_data();
    void checkSyncView_rootView();
    void checkSyncView_childViews_data();
    void checkSyncView_childViews();
    void checkSyncView_differentSizedModels();
    void checkSyncView_differentGeometry();
    void checkSyncView_connect_late_data();
    void checkSyncView_connect_late();
    void checkSyncView_pageFlicking();
    void checkSyncView_emptyModel();
    void checkSyncView_topLeftChanged();
    void delegateWithRequiredProperties();
    void checkThatFetchMoreIsCalledWhenScrolledToTheEndOfTable();
    void replaceModel();
    void cellAtPos_data();
    void cellAtPos();
    void positionViewAtRow_data();
    void positionViewAtRow();
    void positionViewAtColumn_data();
    void positionViewAtColumn();
    void positionViewAtRowClamped_data();
    void positionViewAtRowClamped();
    void positionViewAtColumnClamped_data();
    void positionViewAtColumnClamped();
    void positionViewAtCellWithAnimation();
    void positionViewAtCell_VisibleAndContain_data();
    void positionViewAtCell_VisibleAndContain();
    void positionViewAtCell_VisibleAndContain_SubRect_data();
    void positionViewAtCell_VisibleAndContain_SubRect();
    void positionViewAtCellForLargeCells_data();
    void positionViewAtCellForLargeCells();
    void positionViewAtCellForLargeCellsUsingSubrect();
    void positionViewAtLastRow_data();
    void positionViewAtLastRow();
    void positionViewAtLastColumn_data();
    void positionViewAtLastColumn();
    void itemAtCell_data();
    void itemAtCell();
    void leftRightTopBottomProperties_data();
    void leftRightTopBottomProperties();
    void leftRightTopBottomUpdatedBeforeSignalEmission();
    void checkContentSize_data();
    void checkContentSize();
    void checkSelectionModelWithRequiredSelectedProperty_data();
    void checkSelectionModelWithRequiredSelectedProperty();
    void checkSelectionModelWithUnrequiredSelectedProperty();
    void removeAndAddSelectionModel();
    void warnOnWrongModelInSelectionModel();
    void selectionBehaviorCells_data();
    void selectionBehaviorCells();
    void selectionBehaviorRows();
    void selectionBehaviorColumns();
    void selectionBehaviorDisabled();
    void testSelectableStartPosEndPosOutsideView();
    void testSelectableScrollTowardsPos();
    void setCurrentIndexFromSelectionModel();
    void clearSelectionOnTap_data();
    void clearSelectionOnTap();
    void moveCurrentIndexUsingArrowKeys();
    void moveCurrentIndexUsingHomeAndEndKeys();
    void moveCurrentIndexUsingPageUpDownKeys();
    void moveCurrentIndexUsingTabKey_data();
    void moveCurrentIndexUsingTabKey();
    void respectActiveFocusOnTabDisabled();
    void setCurrentIndexOnFirstKeyPress_data();
    void setCurrentIndexOnFirstKeyPress();
    void setCurrentIndexFromMouse();
    void showMarginsWhenNavigatingToEnd();
    void disableKeyNavigation();
    void disablePointerNavigation();
    void selectUsingArrowKeys();
    void selectUsingHomeAndEndKeys();
    void selectUsingPageUpDownKeys();
    void testDeprecatedApi();
    void alternatingRows();
    void boundDelegateComponent();
    void setColumnWidth_data();
    void setColumnWidth();
    void setColumnWidthWhenProviderIsSet();
    void setColumnWidthForInvalidColumn();
    void setColumnWidthWhenUsingSyncView();
    void resetColumnWidth();
    void clearColumnWidths();
    void setRowHeight_data();
    void setRowHeight();
    void setRowHeightWhenProviderIsSet();
    void setRowHeightForInvalidRow();
    void setRowHeightWhenUsingSyncView();
    void resetRowHeight();
    void clearRowHeights();
    void deletedDelegate();
    void columnResizing_data();
    void columnResizing();
    void tableViewInteractive();
    void rowResizing_data();
    void rowResizing();
    void rowAndColumnResizing_data();
    void rowAndColumnResizing();
    void columnResizingDisabled();
    void rowResizingDisabled();
    void dragFromCellCenter();
    void tapOnResizeArea_data();
    void tapOnResizeArea();
    void editUsingEditTriggers_data();
    void editUsingEditTriggers();
    void editUsingTab();
    void editDelegateComboBox();
    void editOnNonEditableCell_data();
    void editOnNonEditableCell();
    void noEditDelegate_data();
    void noEditDelegate();
    void editAndCloseEditor();
    void editWarning_noEditDelegate();
    void editWarning_invalidIndex();
    void editWarning_nonEditableModelItem();
    void attachedPropertiesOnEditDelegate();
    void requiredPropertiesOnEditDelegate();
    void resettingRolesRespected();
    void checkScroll_data();
    void checkScroll();
    void checkRebuildJsModel();
};

tst_QQuickTableView::tst_QQuickTableView()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickTableView::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<TestModel>("TestModel", 0, 1, "TestModel");
    view = createView();
}

void tst_QQuickTableView::cleanupTestCase()
{
    delete view;
}

QQuickTableViewAttached *tst_QQuickTableView::getAttachedObject(const QObject *object) const
{
    QObject *attachedObject = qmlAttachedPropertiesObject<QQuickTableView>(object);
    return static_cast<QQuickTableViewAttached *>(attachedObject);
}

QPoint tst_QQuickTableView::getContextRowAndColumn(const QQuickItem *item) const
{
    const auto context = qmlContext(item);
    const int row = context->contextProperty("row").toInt();
    const int column = context->contextProperty("column").toInt();
    return QPoint(column, row);
}

static void sendWheelEvent(QQuickWindow *window, QPoint pos, QPoint angleDelta,
                           QPoint pixelDelta,
                           Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                           Qt::ScrollPhase phase = Qt::NoScrollPhase,
                           bool inverted = false) {
    QWheelEvent wheelEvent(pos, window->mapToGlobal(pos), pixelDelta,
                           angleDelta, Qt::NoButton, modifiers, phase,
                           inverted);
    QGuiApplication::sendEvent(window, &wheelEvent);
    qApp->processEvents();
}

void tst_QQuickTableView::setAndGetModel_data()
{
    QTest::addColumn<QVariant>("model");

    QTest::newRow("QAIM 1x1") << TestModelAsVariant(1, 1);
    QTest::newRow("Number model 1") << QVariant::fromValue(1);
    QTest::newRow("QStringList 1") << QVariant::fromValue(QStringList() << "one");
}

void tst_QQuickTableView::setAndGetModel()
{
    // Test that we can set and get different kind of models
    QFETCH(QVariant, model);
    LOAD_TABLEVIEW("plaintableview.qml");

    tableView->setModel(model);
    QCOMPARE(model, tableView->model());
}

void tst_QQuickTableView::emptyModel_data()
{
    QTest::addColumn<QVariant>("model");

    QTest::newRow("QAIM") << TestModelAsVariant(0, 0);
    QTest::newRow("Number model") << QVariant::fromValue(0);
    QTest::newRow("QStringList") << QVariant::fromValue(QStringList());
}

void tst_QQuickTableView::emptyModel()
{
    // Check that if we assign an empty model to
    // TableView, no delegate items will be created.
    QFETCH(QVariant, model);
    LOAD_TABLEVIEW("plaintableview.qml");

    tableView->setModel(model);
    if (QQuickTest::qIsPolishScheduled(tableView))
        WAIT_UNTIL_POLISHED;
    QCOMPARE(tableViewPrivate->loadedItems.size(), 0);
}

void tst_QQuickTableView::checkPreload_data()
{
    QTest::addColumn<bool>("reuseItems");

    QTest::newRow("reuse") << true;
    QTest::newRow("not reuse") << false;
}

void tst_QQuickTableView::checkPreload()
{
    // Check that the reuse pool is filled up with one extra row and
    // column (pluss corner) after rebuilding the table, but only if we reuse items.
    QFETCH(bool, reuseItems);
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);
    tableView->setReuseItems(reuseItems);

    WAIT_UNTIL_POLISHED;

    if (reuseItems) {
        const int rowCount = tableViewPrivate->loadedRows.count();
        const int columnCount = tableViewPrivate->loadedColumns.count();
        const int expectedPoolSize = rowCount + columnCount + 1;
        QCOMPARE(tableViewPrivate->tableModel->poolSize(), expectedPoolSize);
    } else {
        QCOMPARE(tableViewPrivate->tableModel->poolSize(), 0);
    }
}

void tst_QQuickTableView::checkZeroSizedDelegate()
{
    // Check that if we assign a delegate with empty width and height, we
    // fall back to use kDefaultColumnWidth and kDefaultRowHeight as
    // column/row sizes.
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    view->rootObject()->setProperty("delegateWidth", 0);
    view->rootObject()->setProperty("delegateHeight", 0);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*implicit"));

    WAIT_UNTIL_POLISHED;

    auto items = tableViewPrivate->loadedItems;
    QVERIFY(!items.isEmpty());

    for (auto fxItem : tableViewPrivate->loadedItems) {
        auto item = fxItem->item;
        QCOMPARE(item->width(), kDefaultColumnWidth);
        QCOMPARE(item->height(), kDefaultRowHeight);
    }
}

void tst_QQuickTableView::checkImplicitSizeDelegate()
{
    // Check that we can set the size of delegate items using
    // implicit width/height, instead of forcing the user to
    // create an attached object by using implicitWidth/Height.
    LOAD_TABLEVIEW("tableviewimplicitsize.qml");

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    auto items = tableViewPrivate->loadedItems;
    QVERIFY(!items.isEmpty());

    for (auto fxItem : tableViewPrivate->loadedItems) {
        auto item = fxItem->item;
        QCOMPARE(item->width(), 90);
        QCOMPARE(item->height(), 60);
    }
}

void tst_QQuickTableView::checkZeroSizedTableView()
{
    // Check that we don't load any delegates if TableView
    // itself has zero size.
    LOAD_TABLEVIEW("zerosizedtableview.qml");

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    QVERIFY(tableViewPrivate->loadedItems.isEmpty());

    // Resize TableView. This should load delegate. Since
    // the delegate's implicitWidth is bound to TableView.width,
    // we expect the delegates to now get the same width.
    tableView->setWidth(200);
    tableView->setHeight(100);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedItems.size(), 2);
    const auto item = tableView->itemAtIndex(tableView->index(0, 0));
    QVERIFY(item);
    QCOMPARE(item->width(), 200);

    // Hide TableView again, and check that all items are
    // unloaded, except the topLeft corner item.
    tableView->setWidth(0);
    tableView->setHeight(0);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedItems.size(), 1);
}

void tst_QQuickTableView::checkZeroSizedViewPort()
{
    LOAD_TABLEVIEW("zerosizedviewport.qml");

    auto model = TestModelAsVariant(20, 20);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    QVERIFY(!tableViewPrivate->loadedItems.isEmpty());
}

void tst_QQuickTableView::checkColumnWidthWithoutProvider()
{
    // Checks that a function isn't assigned to the columnWidthProvider property
    // and that the column width is then equal to sizeHintForColumn.
    LOAD_TABLEVIEW("alternatingrowheightcolumnwidth.qml");

    auto model = TestModelAsVariant(10, 10);

    tableView->setModel(model);
    QVERIFY(tableView->columnWidthProvider().isUndefined());

    WAIT_UNTIL_POLISHED;

    for (const int column : tableViewPrivate->loadedColumns) {
        const qreal expectedColumnWidth = tableViewPrivate->sizeHintForColumn(column);
        for (const int row : tableViewPrivate->loadedRows) {
            const auto item = tableViewPrivate->loadedTableItem(QPoint(column, row))->item;
            QCOMPARE(item->width(), expectedColumnWidth);
        }
    }
}

void tst_QQuickTableView::checkColumnWidthAndRowHeightFunctions()
{
    // Checks that the column width and row height functions return
    // the correct sizes. When we have row-, or columnWidthProviders
    // the actual row and column sizes will normally differ from the
    // minimum row and column sizes (which is the maximum implicit
    // size found among the delegates).
    LOAD_TABLEVIEW("userowcolumnprovider.qml");

    const int count = 4;
    auto model = TestModelAsVariant(count, count);

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    const qreal expectedimplicitSize = 20;

    for (int i = 0; i < count; ++i) {
        const qreal expectedSize = i + 10;
        QCOMPARE(tableView->columnWidth(i), expectedSize);
        QCOMPARE(tableView->rowHeight(i), expectedSize);
        QCOMPARE(tableView->implicitColumnWidth(i), expectedimplicitSize);
        QCOMPARE(tableView->implicitRowHeight(i), expectedimplicitSize);
    }
}

void tst_QQuickTableView::checkDelegateWithAnchors()
{
    // Checks that we issue a warning if the delegate has anchors
    LOAD_TABLEVIEW("delegatewithanchors.qml");

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*anchors"));

    auto model = TestModelAsVariant(1, 1);
    tableView->setModel(model);
    WAIT_UNTIL_POLISHED;
}

void tst_QQuickTableView::checkColumnWidthProvider()
{
    // Check that you can assign a function to the columnWidthProvider property, and
    // that it's used to control (and override) the width of the columns.
    LOAD_TABLEVIEW("userowcolumnprovider.qml");

    auto model = TestModelAsVariant(10, 10);

    tableView->setModel(model);
    QVERIFY(tableView->columnWidthProvider().isCallable());

    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems) {
        // expectedWidth mirrors the expected return value of the assigned javascript function
        qreal expectedWidth = fxItem->cell.x() + 10;
        QCOMPARE(fxItem->item->width(), expectedWidth);
    }
}

void tst_QQuickTableView::checkColumnWidthProviderInvalidReturnValues()
{
    // Check that we fall back to use default columns widths, if you
    // assign a function to columnWidthProvider that returns invalid values.
    LOAD_TABLEVIEW("usefaultyrowcolumnprovider.qml");

    auto model = TestModelAsVariant(10, 10);

    tableView->setModel(model);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*implicit.*zero"));

    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->width(), kDefaultColumnWidth);
}

void tst_QQuickTableView::checkColumnWidthProviderNegativeReturnValue()
{
    // Check that we fall back to use the implicit width of the delegate
    // items if the columnWidthProvider return a negative number.
    LOAD_TABLEVIEW("userowcolumnprovider.qml");

    auto model = TestModelAsVariant(10, 10);
    view->rootObject()->setProperty("returnNegativeColumnWidth", true);

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->width(), 20);
}

void tst_QQuickTableView::checkColumnWidthProviderNotCallable()
{
    // Check that we fall back to use default columns widths, if you
    // assign something to columnWidthProvider that is not callable.
    LOAD_TABLEVIEW("usefaultyrowcolumnprovider.qml");

    auto model = TestModelAsVariant(10, 10);

    tableView->setModel(model);
    tableView->setRowHeightProvider(QJSValue());
    tableView->setColumnWidthProvider(QJSValue(10));

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".Provider.*function"));

    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->width(), kDefaultColumnWidth);
}

void tst_QQuickTableView::checkColumnWidthBoundToViewWidth()
{
    // Check that you can bind the width of a delegate to the
    // width of TableView, and that it updates when TableView is resized.
    LOAD_TABLEVIEW("columnwidthboundtoviewwidth.qml");

    auto model = TestModelAsVariant(10, 1);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->width(), tableView->width());

    tableView->setWidth(200);
    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->width(), 200);
}

void tst_QQuickTableView::checkRowHeightWithoutProvider()
{
    // Checks that a function isn't assigned to the rowHeightProvider property
    // and that the row height is then equal to sizeHintForRow.
    LOAD_TABLEVIEW("alternatingrowheightcolumnwidth.qml");

    auto model = TestModelAsVariant(10, 10);
    QVERIFY(tableView->rowHeightProvider().isUndefined());

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    for (const int row : tableViewPrivate->loadedRows) {
        const qreal expectedRowHeight = tableViewPrivate->sizeHintForRow(row);
        for (const int column : tableViewPrivate->loadedColumns) {
            const auto item = tableViewPrivate->loadedTableItem(QPoint(column, row))->item;
            QCOMPARE(item->height(), expectedRowHeight);
        }
    }
}

void tst_QQuickTableView::checkRowHeightProvider()
{
    // Check that you can assign a function to the columnWidthProvider property, and
    // that it's used to control (and override) the width of the columns.
    LOAD_TABLEVIEW("userowcolumnprovider.qml");

    auto model = TestModelAsVariant(10, 10);

    tableView->setModel(model);
    QVERIFY(tableView->rowHeightProvider().isCallable());

    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems) {
        // expectedWidth mirrors the expected return value of the assigned javascript function
        qreal expectedHeight = fxItem->cell.y() + 10;
        QCOMPARE(fxItem->item->height(), expectedHeight);
    }
}

void tst_QQuickTableView::checkRowHeightProviderInvalidReturnValues()
{
    // Check that we fall back to use default row heights, if you
    // assign a function to rowHeightProvider that returns invalid values.
    LOAD_TABLEVIEW("usefaultyrowcolumnprovider.qml");

    auto model = TestModelAsVariant(10, 10);

    tableView->setModel(model);

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*implicit.*zero"));

    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->height(), kDefaultRowHeight);
}

void tst_QQuickTableView::checkRowHeightProviderNegativeReturnValue()
{
    // Check that we fall back to use the implicit height of the delegate
    // items if the rowHeightProvider return a negative number.
    LOAD_TABLEVIEW("userowcolumnprovider.qml");

    auto model = TestModelAsVariant(10, 10);
    view->rootObject()->setProperty("returnNegativeRowHeight", true);

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->height(), 20);
}

void tst_QQuickTableView::checkRowHeightProviderNotCallable()
{
    // Check that we fall back to use default row heights, if you
    // assign something to rowHeightProvider that is not callable.
    LOAD_TABLEVIEW("usefaultyrowcolumnprovider.qml");

    auto model = TestModelAsVariant(10, 10);

    tableView->setModel(model);

    tableView->setColumnWidthProvider(QJSValue());
    tableView->setRowHeightProvider(QJSValue(10));

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*Provider.*function"));

    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->height(), kDefaultRowHeight);
}

void tst_QQuickTableView::isColumnLoadedAndIsRowLoaded()
{
    // Check that all the delegate items are loaded and available from
    // the columnWidthProvider/rowHeightProvider when 'isColumnLoaded()'
    // and 'isRowLoaded()' returns true.
    LOAD_TABLEVIEW("iscolumnloaded.qml");

    auto model = TestModelAsVariant(4, 5);

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    const int itemsInColumnAfterLoaded = view->rootObject()->property("itemsInColumnAfterLoaded").toInt();
    const int itemsInRowAfterLoaded = view->rootObject()->property("itemsInRowAfterLoaded").toInt();

    QCOMPARE(itemsInColumnAfterLoaded, tableView->rows());
    QCOMPARE(itemsInRowAfterLoaded, tableView->columns());
}

void tst_QQuickTableView::checkForceLayoutFunction()
{
    // When we set the 'columnWidths' property in the test file, the
    // columnWidthProvider should return other values than it did during
    // start-up. Check that this takes effect after a call to the 'forceLayout()' function.
    LOAD_TABLEVIEW("forcelayout.qml");

    const char *propertyName = "columnWidths";
    auto model = TestModelAsVariant(10, 10);

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    // Check that the initial column widths are as specified in the QML file
    const qreal initialColumnWidth = view->rootObject()->property(propertyName).toReal();
    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->width(), initialColumnWidth);

    // Change the return value from the columnWidthProvider to something else
    const qreal newColumnWidth = 100;
    view->rootObject()->setProperty(propertyName, newColumnWidth);
    tableView->forceLayout();
    // We don't have to polish; The re-layout happens immediately

    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->width(), newColumnWidth);
}

void tst_QQuickTableView::checkForceLayoutEndUpDoingALayout()
{
    // QTBUG-77074
    // Check that we change the implicit size of the delegate after
    // the initial loading, and at the same time hide some rows or
    // columns, and then do a forceLayout(), we end up with a
    // complete relayout that respects the new implicit size.
    LOAD_TABLEVIEW("tweakimplicitsize.qml");

    auto model = TestModelAsVariant(10, 10);

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    const qreal newDelegateSize = 20;
    view->rootObject()->setProperty("delegateSize", newDelegateSize);
    // Hide a row, just to force the following relayout to
    // do a complete reload (and not just a relayout)
    view->rootObject()->setProperty("hideRow", 1);
    tableView->forceLayout();

    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->height(), newDelegateSize);

    // Check that the content height has been updated as well
    const qreal rowSpacing = tableView->rowSpacing();
    const qreal colSpacing = tableView->columnSpacing();
    QCOMPARE(tableView->contentWidth(), (10 * (newDelegateSize + colSpacing)) - colSpacing);
    QCOMPARE(tableView->contentHeight(), (9 * (newDelegateSize + rowSpacing)) - rowSpacing);
}

void tst_QQuickTableView::checkForceLayoutInbetweenAddingRowsToModel()
{
    // Check that TableView doesn't assert if we call forceLayout() while waiting
    // for a callback from the model that the row count has changed. Also make sure
    // that we don't move the contentItem while doing so.
    LOAD_TABLEVIEW("plaintableview.qml");

    const int initialRowCount = 10;
    TestModel model(initialRowCount, 10);
    tableView->setModel(QVariant::fromValue(&model));

    connect(&model, &QAbstractItemModel::rowsInserted, [=](){
        QCOMPARE(tableView->rows(), initialRowCount);
        tableView->forceLayout();
        QCOMPARE(tableView->rows(), initialRowCount + 1);
    });

    WAIT_UNTIL_POLISHED;

    const int contentY = 10;
    tableView->setContentY(contentY);
    QCOMPARE(tableView->rows(), initialRowCount);
    QCOMPARE(tableView->contentY(), contentY);
    model.addRow(0);
    QCOMPARE(tableView->rows(), initialRowCount + 1);
    QCOMPARE(tableView->contentY(), contentY);
}

void tst_QQuickTableView::checkForceLayoutWhenAllItemsAreHidden()
{
    // Check that you can have a TableView where all columns are
    // initially hidden, and then show some columns and call
    // forceLayout(). This should make the columns become visible.
    LOAD_TABLEVIEW("forcelayout.qml");

    // Tell all columns to be hidden
    const char *propertyName = "columnWidths";
    view->rootObject()->setProperty(propertyName, 0);

    const int rows = 3;
    const int columns = 3;
    auto model = TestModelAsVariant(rows, columns);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    // Check that the we have no items loaded
    QCOMPARE(tableViewPrivate->loadedColumns.count(), 0);
    QCOMPARE(tableViewPrivate->loadedRows.count(), 0);
    QCOMPARE(tableViewPrivate->loadedItems.size(), 0);

    // Tell all columns to be visible
    view->rootObject()->setProperty(propertyName, 10);
    tableView->forceLayout();

    QCOMPARE(tableViewPrivate->loadedRows.count(), rows);
    QCOMPARE(tableViewPrivate->loadedColumns.count(), columns);
    QCOMPARE(tableViewPrivate->loadedItems.size(), rows * columns);
}

void tst_QQuickTableView::checkLayoutChangedSignal()
{
    // Check that the layoutChanged signal is emitted
    // when the layout has changed.
    LOAD_TABLEVIEW("plaintableview.qml");

    const QSignalSpy layoutChanges(tableView, &QQuickTableView::layoutChanged);
    TestModel model(100, 100);
    tableView->setModel(QVariant::fromValue(&model));

    WAIT_UNTIL_POLISHED;

    QCOMPARE(layoutChanges.size(), 1);

    tableView->forceLayout();
    QCOMPARE(layoutChanges.size(), 2);

    tableView->setRowHeight(1, 10);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(layoutChanges.size(), 3);

    tableView->setColumnWidth(1, 10);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(layoutChanges.size(), 4);

    tableView->setContentX(30);
    QCOMPARE(layoutChanges.size(), 5);

    tableView->setContentY(30);
    QCOMPARE(layoutChanges.size(), 6);

    tableView->setContentX(0);
    QCOMPARE(layoutChanges.size(), 7);

    tableView->setContentY(0);
    QCOMPARE(layoutChanges.size(), 8);

    model.addRow(1);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(layoutChanges.size(), 9);

    model.removeRow(1);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(layoutChanges.size(), 10);
}

void tst_QQuickTableView::checkContentWidthAndHeight()
{
    // Check that contentWidth/Height reports the correct size of the
    // table, based on knowledge of the rows and columns that has been loaded.
    LOAD_TABLEVIEW("contentwidthheight.qml");

    // Vertical and horizontal properties should be mirrored, so we only have
    // to do the calculations once, and use them for both axis, below.
    QCOMPARE(tableView->width(), tableView->height());
    QCOMPARE(tableView->rowSpacing(), tableView->columnSpacing());

    const int tableSize = 100;
    const int cellSizeSmall = 100;
    const int spacing = 1;
    auto model = TestModelAsVariant(tableSize, tableSize);

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    const qreal expectedSizeInit = (tableSize * cellSizeSmall) + ((tableSize - 1) * spacing);
    QCOMPARE(tableView->contentWidth(), expectedSizeInit);
    QCOMPARE(tableView->contentHeight(), expectedSizeInit);
    QCOMPARE(tableViewPrivate->averageEdgeSize.width(), cellSizeSmall);
    QCOMPARE(tableViewPrivate->averageEdgeSize.height(), cellSizeSmall);

    // Flick to the end, and check that content width/height stays unchanged
    tableView->setContentX(tableView->contentWidth() - tableView->width());
    tableView->setContentY(tableView->contentHeight() - tableView->height());

    QCOMPARE(tableView->contentWidth(), expectedSizeInit);
    QCOMPARE(tableView->contentHeight(), expectedSizeInit);

    // Flick back to start
    tableView->setContentX(0);
    tableView->setContentY(0);

    // Since we move the viewport more than a page, tableview
    // will jump to the new position and do a rebuild.
    QVERIFY(tableViewPrivate->polishScheduled);
    QVERIFY(tableViewPrivate->scheduledRebuildOptions);
    WAIT_UNTIL_POLISHED;

    // We should still have the same content width/height as when we started
    QCOMPARE(tableView->contentWidth(), expectedSizeInit);
    QCOMPARE(tableView->contentHeight(), expectedSizeInit);
}

void tst_QQuickTableView::checkContentWidthAndHeightForSmallTables()
{
    // For tables where all the columns in the model are loaded, we know
    // the exact table width, and can therefore update the content width
    // if e.g new rows are added or removed. The same is true for rows.
    // This test will check that we do so.
    LOAD_TABLEVIEW("sizefromdelegate.qml");

    TestModel model(3, 3);
    tableView->setModel(QVariant::fromValue(&model));
    WAIT_UNTIL_POLISHED;

    const qreal initialContentWidth = tableView->contentWidth();
    const qreal initialContentHeight = tableView->contentHeight();
    const QString longText = QStringLiteral("Adding a row with a very long text");
    model.insertRow(0);
    model.setModelData(QPoint(0, 0), QSize(1, 1), longText);

    WAIT_UNTIL_POLISHED;

    QVERIFY(tableView->contentWidth() > initialContentWidth);
    QVERIFY(tableView->contentHeight() > initialContentHeight);
}

void tst_QQuickTableView::checkPageFlicking()
{
    // Check that we rebuild the table instead of refilling edges, if the viewport moves
    // more than a page (the size of TableView).
    LOAD_TABLEVIEW("plaintableview.qml");

    const int cellWidth = 100;
    const int cellHeight = 50;
    auto model = TestModelAsVariant(10000, 10000);
    const auto &loadedRows = tableViewPrivate->loadedRows;
    const auto &loadedColumns = tableViewPrivate->loadedColumns;

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    // Sanity check startup table
    QCOMPARE(tableViewPrivate->topRow(), 0);
    QCOMPARE(tableViewPrivate->leftColumn(), 0);
    QCOMPARE(loadedRows.count(), tableView->height() / cellHeight);
    QCOMPARE(loadedColumns.count(), tableView->width() / cellWidth);

    // Since all cells have the same size, the average row/column
    // size found by TableView should be exactly equal to this.
    QCOMPARE(tableViewPrivate->averageEdgeSize.width(), cellWidth);
    QCOMPARE(tableViewPrivate->averageEdgeSize.height(), cellHeight);

    QCOMPARE(tableViewPrivate->scheduledRebuildOptions, QQuickTableViewPrivate::RebuildOption::None);

    // Flick 5000 columns to the right, and check that this triggers a
    // rebuild, and that we end up at the expected top-left.
    const int flickToColumn = 5000;
    const qreal columnSpacing = tableView->columnSpacing();
    const qreal flickToColumnInPixels = ((cellWidth + columnSpacing) * flickToColumn) - columnSpacing;
    tableView->setContentX(flickToColumnInPixels);

    QVERIFY(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::ViewportOnly);
    QVERIFY(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::CalculateNewTopLeftColumn);
    QVERIFY(!(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::CalculateNewTopLeftRow));

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->topRow(), 0);
    QCOMPARE(tableViewPrivate->leftColumn(), flickToColumn);
    QCOMPARE(loadedColumns.count(), tableView->width() / cellWidth);
    QCOMPARE(loadedRows.count(), tableView->height() / cellHeight);

    // Flick 5000 rows down as well. Since flicking down should only calculate a new row (but
    // keep the current column), we deliberatly change the average width to check that it's
    // actually ignored by the rebuild, and that the column stays the same.
    tableViewPrivate->averageEdgeSize.rwidth() /= 2;

    const int flickToRow = 5000;
    const qreal rowSpacing = tableView->rowSpacing();
    const qreal flickToRowInPixels = ((cellHeight + rowSpacing) * flickToRow) - rowSpacing;
    tableView->setContentY(flickToRowInPixels);

    QVERIFY(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::ViewportOnly);
    QVERIFY(!(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::CalculateNewTopLeftColumn));
    QVERIFY(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::CalculateNewTopLeftRow);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->topRow(), flickToColumn);
    QCOMPARE(tableViewPrivate->leftColumn(), flickToRow);
    QCOMPARE(loadedRows.count(), tableView->height() / cellHeight);
    QCOMPARE(loadedColumns.count(), tableView->width() / cellWidth);
}

void tst_QQuickTableView::checkExplicitContentWidthAndHeight()
{
    // Check that you can set a custom contentWidth/Height, and that
    // TableView doesn't override it while loading more rows and columns.
    LOAD_TABLEVIEW("contentwidthheight.qml");

    tableView->setContentWidth(1000);
    tableView->setContentHeight(1000);
    QCOMPARE(tableView->contentWidth(), 1000);
    QCOMPARE(tableView->contentHeight(), 1000);

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);
    WAIT_UNTIL_POLISHED;

    // Flick somewhere. It should not affect the contentWidth/Height
    tableView->setContentX(500);
    tableView->setContentY(500);
    QCOMPARE(tableView->contentWidth(), 1000);
    QCOMPARE(tableView->contentHeight(), 1000);
}

void tst_QQuickTableView::checkExtents_origin()
{
    // Check that if the beginning of the content view doesn't match the
    // actual size of the table, origin will be adjusted to make it fit.
    LOAD_TABLEVIEW("contentwidthheight.qml");

    const int rows = 10;
    const int columns = rows;
    const qreal columnWidth = 100;
    const qreal rowHeight = 100;
    const qreal actualTableSize = columns * columnWidth;

    // Set a content size that is far too large
    // compared to the size of the table.
    tableView->setContentWidth(actualTableSize * 2);
    tableView->setContentHeight(actualTableSize * 2);
    tableView->setRowSpacing(0);
    tableView->setColumnSpacing(0);
    tableView->setLeftMargin(0);
    tableView->setRightMargin(0);
    tableView->setTopMargin(0);
    tableView->setBottomMargin(0);

    auto model = TestModelAsVariant(rows, columns);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    // Flick slowly to column 5 (to avoid rebuilds). Flick two columns at a
    // time to ensure that we create a gap before TableView gets a chance to
    // adjust endExtent first. This gap on the right side will make TableView
    // move the table to move to the edge. Because of this, the table will not
    // be aligned at the start of the content view when we next flick back again.
    // And this will cause origin to move.
    for (int x = 0; x <= 6; x += 2) {
        tableView->setContentX(x * columnWidth);
        tableView->setContentY(x * rowHeight);
    }

    // Check that the table has now been moved one column to the right
    // (One column because that's how far outside the table we ended up flicking above).
    QCOMPARE(tableViewPrivate->loadedTableOuterRect.right(), actualTableSize + columnWidth);

    // Flick back one column at a time so that TableView detects that the first
    // column is not at the origin before the "table move" logic kicks in. This
    // will make TableView adjust the origin.
    for (int x = 6; x >= 0; x -= 1) {
        tableView->setContentX(x * columnWidth);
        tableView->setContentY(x * rowHeight);
    }

    // The origin will be moved with the same offset that the table was
    // moved on the right side earlier, which is one column length.
    QCOMPARE(tableViewPrivate->origin.x(), columnWidth);
    QCOMPARE(tableViewPrivate->origin.y(), rowHeight);
}

void tst_QQuickTableView::checkExtents_endExtent()
{
    // Check that if we the content view size doesn't match the actual size
    // of the table, endExtent will be adjusted to make it fit (so that
    // e.g the the flicking will bounce to a stop at the edge of the table).
    LOAD_TABLEVIEW("contentwidthheight.qml");

    const int rows = 10;
    const int columns = rows;
    const qreal columnWidth = 100;
    const qreal rowHeight = 100;
    const qreal actualTableSize = columns * columnWidth;

    // Set a content size that is far too large
    // compared to the size of the table.
    tableView->setContentWidth(actualTableSize * 2);
    tableView->setContentHeight(actualTableSize * 2);
    tableView->setRowSpacing(0);
    tableView->setColumnSpacing(0);
    tableView->setLeftMargin(0);
    tableView->setRightMargin(0);
    tableView->setTopMargin(0);
    tableView->setBottomMargin(0);

    auto model = TestModelAsVariant(rows, columns);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    // Flick slowly to column 5 (to avoid rebuilds). This will flick the table to
    // the last column in the model. But since there still is a lot space left in
    // the content view, endExtent will be set accordingly to compensate.
    for (int x = 1; x <= 5; x++)
        tableView->setContentX(x * columnWidth);
    QCOMPARE(tableViewPrivate->rightColumn(), columns - 1);
    qreal expectedEndExtentWidth = actualTableSize - tableView->contentWidth();
    QCOMPARE(tableViewPrivate->endExtent.width(), expectedEndExtentWidth);

    for (int y = 1; y <= 5; y++)
        tableView->setContentY(y * rowHeight);
    QCOMPARE(tableViewPrivate->bottomRow(), rows - 1);
    qreal expectedEndExtentHeight = actualTableSize - tableView->contentHeight();
    QCOMPARE(tableViewPrivate->endExtent.height(), expectedEndExtentHeight);
}

void tst_QQuickTableView::checkExtents_moveTableToEdge()
{
    // Check that if we the content view size doesn't match the actual
    // size of the table, and we fast-flick the viewport to outside
    // the table, we end up moving the table back into the viewport to
    // avoid any visual glitches.
    LOAD_TABLEVIEW("contentwidthheight.qml");

    const int rows = 10;
    const int columns = rows;
    const qreal columnWidth = 100;
    const qreal rowHeight = 100;
    const qreal actualTableSize = columns * columnWidth;

    // Set a content size that is far to large
    // compared to the size of the table.
    tableView->setContentWidth(actualTableSize * 2);
    tableView->setContentHeight(actualTableSize * 2);
    tableView->setRowSpacing(0);
    tableView->setColumnSpacing(0);
    tableView->setLeftMargin(0);
    tableView->setRightMargin(0);
    tableView->setTopMargin(0);
    tableView->setBottomMargin(0);

    auto model = TestModelAsVariant(rows, columns);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    // Flick slowly to column 5 (to avoid rebuilds). Flick two columns at a
    // time to ensure that we create a gap before TableView gets a chance to
    // adjust endExtent first. This gap on the right side will make TableView
    // move the table to the edge (in addition to adjusting the extents, but that
    // will happen in a subsequent polish, and is not for this test verify).
    for (int x = 0; x <= 6; x += 2)
        tableView->setContentX(x * columnWidth);
    QCOMPARE(tableViewPrivate->rightColumn(), columns - 1);
    QCOMPARE(tableViewPrivate->loadedTableOuterRect, tableViewPrivate->viewportRect);

    for (int y = 0; y <= 6; y += 2)
        tableView->setContentY(y * rowHeight);
    QCOMPARE(tableViewPrivate->bottomRow(), rows - 1);
    QCOMPARE(tableViewPrivate->loadedTableOuterRect, tableViewPrivate->viewportRect);

    for (int x = 6; x >= 0; x -= 2)
        tableView->setContentX(x * columnWidth);
    QCOMPARE(tableViewPrivate->leftColumn(), 0);
    QCOMPARE(tableViewPrivate->loadedTableOuterRect, tableViewPrivate->viewportRect);

    for (int y = 6; y >= 0; y -= 2)
        tableView->setContentY(y * rowHeight);
    QCOMPARE(tableViewPrivate->topRow(), 0);
    QCOMPARE(tableViewPrivate->loadedTableOuterRect, tableViewPrivate->viewportRect);
}

void tst_QQuickTableView::checkContentXY()
{
    // Check that you can bind contentX and contentY to
    // e.g show the center of the table at start-up
    LOAD_TABLEVIEW("setcontentpos.qml");

    auto model = TestModelAsVariant(10, 10);
    tableView->setModel(model);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->width(), 400);
    QCOMPARE(tableView->height(), 400);
    QCOMPARE(tableView->contentWidth(), 1000);
    QCOMPARE(tableView->contentHeight(), 1000);

    // Check that the content item is positioned according
    // to the binding in the QML file (which will set the
    // viewport to be at the center of the table).
    const qreal expectedXY = (tableView->contentWidth() - tableView->width()) / 2;
    QCOMPARE(tableView->contentX(), expectedXY);
    QCOMPARE(tableView->contentY(), expectedXY);

    // Check that we end up at the correct top-left cell:
    const qreal delegateWidth = tableViewPrivate->loadedItems.values().first()->item->width();
    const int expectedCellXY = qCeil(expectedXY / delegateWidth);
    QCOMPARE(tableViewPrivate->leftColumn(), expectedCellXY);
    QCOMPARE(tableViewPrivate->topRow(), expectedCellXY);
}

void tst_QQuickTableView::noDelegate()
{
    // Check that you can skip setting a delegate without
    // it causing any problems (like crashing or asserting).
    // And then set a delegate, and do a quick check that the
    // view gets populated as expected.
    LOAD_TABLEVIEW("plaintableview.qml");

    const int rows = 5;
    const int columns = 5;
    auto model = TestModelAsVariant(columns, rows);
    tableView->setModel(model);

    // Start with no delegate, and check
    // that we end up with no items in the table.
    tableView->setDelegate(nullptr);

    WAIT_UNTIL_POLISHED;

    auto items = tableViewPrivate->loadedItems;
    QVERIFY(items.isEmpty());

    // Set a delegate, and check that we end
    // up with the expected number of items.
    auto delegate = view->rootObject()->property("delegate").value<QQmlComponent *>();
    QVERIFY(delegate);
    tableView->setDelegate(delegate);

    WAIT_UNTIL_POLISHED;

    items = tableViewPrivate->loadedItems;
    QCOMPARE(items.size(), rows * columns);

    // And then unset the delegate again, and check
    // that we end up with no items.
    tableView->setDelegate(nullptr);

    WAIT_UNTIL_POLISHED;

    items = tableViewPrivate->loadedItems;
    QVERIFY(items.isEmpty());
}

void tst_QQuickTableView::changeDelegateDuringUpdate()
{
    // Check that you can change the delegate (set it to null)
    // while the TableView is busy loading the table.
    LOAD_TABLEVIEW("changemodelordelegateduringupdate.qml");

    auto model = TestModelAsVariant(1, 1);
    tableView->setModel(model);
    view->rootObject()->setProperty("changeDelegate", true);

    WAIT_UNTIL_POLISHED;

    // We should no longer have a delegate, and no
    // items should therefore be loaded.
    QCOMPARE(tableView->delegate(), nullptr);
    QCOMPARE(tableViewPrivate->loadedItems.size(), 0);

    // Even if the delegate is missing, we still report
    // the correct size of the model
    QCOMPARE(tableView->rows(), 1);
    QCOMPARE(tableView->columns(), 1);
};

void tst_QQuickTableView::changeModelDuringUpdate()
{
    // Check that you can change the model (set it to null)
    // while the TableView is buzy loading the table.
    LOAD_TABLEVIEW("changemodelordelegateduringupdate.qml");

    auto model = TestModelAsVariant(1, 1);
    tableView->setModel(model);
    view->rootObject()->setProperty("changeModel", true);

    WAIT_UNTIL_POLISHED;

    // We should no longer have a model, and the no
    // items should therefore be loaded.
    QVERIFY(tableView->model().isNull());
    QCOMPARE(tableViewPrivate->loadedItems.size(), 0);

    // The empty model has no rows or columns
    QCOMPARE(tableView->rows(), 0);
    QCOMPARE(tableView->columns(), 0);
};

void tst_QQuickTableView::countDelegateItems_data()
{
    QTest::addColumn<QVariant>("model");
    QTest::addColumn<int>("count");

    QTest::newRow("QAIM 1x1") << TestModelAsVariant(1, 1) << 1;
    QTest::newRow("QAIM 2x1") << TestModelAsVariant(2, 1) << 2;
    QTest::newRow("QAIM 1x2") << TestModelAsVariant(1, 2) << 2;
    QTest::newRow("QAIM 2x2") << TestModelAsVariant(2, 2) << 4;
    QTest::newRow("QAIM 4x4") << TestModelAsVariant(4, 4) << 16;

    QTest::newRow("Number model 1") << QVariant::fromValue(1) << 1;
    QTest::newRow("Number model 4") << QVariant::fromValue(4) << 4;

    QTest::newRow("QStringList 1") << QVariant::fromValue(QStringList() << "one") << 1;
    QTest::newRow("QStringList 4") << QVariant::fromValue(QStringList() << "one" << "two" << "three" << "four") << 4;
}

void tst_QQuickTableView::countDelegateItems()
{
    // Assign different models of various sizes, and check that the number of
    // delegate items in the view matches the size of the model. Note that for
    // this test to be valid, all items must be within the visible area of the view.
    QFETCH(QVariant, model);
    QFETCH(int, count);
    LOAD_TABLEVIEW("plaintableview.qml");

    tableView->setModel(model);
    WAIT_UNTIL_POLISHED;

    // Check that tableview internals contain the expected number of items
    auto const items = tableViewPrivate->loadedItems;
    QCOMPARE(items.size(), count);

    // Check that this also matches the items found in the view
    auto foundItems = findItems<QQuickItem>(tableView, kDelegateObjectName);
    QCOMPARE(foundItems.size(), count);
}

void tst_QQuickTableView::checkLayoutOfEqualSizedDelegateItems_data()
{
    QTest::addColumn<QVariant>("model");
    QTest::addColumn<QSize>("tableSize");
    QTest::addColumn<QSizeF>("spacing");
    QTest::addColumn<QMarginsF>("margins");

    // Check spacing together with different table setups
    QTest::newRow("QAIM 1x1 1,1") << TestModelAsVariant(1, 1) << QSize(1, 1) << QSizeF(1, 1) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("QAIM 5x5 0,0") << TestModelAsVariant(5, 5) << QSize(5, 5) << QSizeF(0, 0) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("QAIM 5x5 1,0") << TestModelAsVariant(5, 5) << QSize(5, 5) << QSizeF(1, 0) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("QAIM 5x5 0,1") << TestModelAsVariant(5, 5) << QSize(5, 5) << QSizeF(0, 1) << QMarginsF(0, 0, 0, 0);

    // Check spacing together with margins
    QTest::newRow("QAIM 1x1 1,1 5555") << TestModelAsVariant(1, 1) << QSize(1, 1) << QSizeF(1, 1) << QMarginsF(5, 5, 5, 5);
    QTest::newRow("QAIM 4x4 0,0 3333") << TestModelAsVariant(4, 4) << QSize(4, 4) << QSizeF(0, 0) << QMarginsF(3, 3, 3, 3);
    QTest::newRow("QAIM 4x4 2,2 1234") << TestModelAsVariant(4, 4) << QSize(4, 4) << QSizeF(2, 2) << QMarginsF(1, 2, 3, 4);

    // Check "list" models
    QTest::newRow("NumberModel 1x4, 0000") << QVariant::fromValue(4) << QSize(1, 4) << QSizeF(1, 1) << QMarginsF(0, 0, 0, 0);
    QTest::newRow("QStringList 1x4, 0,0 1111") << QVariant::fromValue(QStringList() << "one" << "two" << "three" << "four")
                                               << QSize(1, 4) << QSizeF(0, 0) << QMarginsF(1, 1, 1, 1);
}

void tst_QQuickTableView::checkLayoutOfEqualSizedDelegateItems()
{
    // Check that the geometry of the delegate items are correct
    QFETCH(QVariant, model);
    QFETCH(QSize, tableSize);
    QFETCH(QSizeF, spacing);
    QFETCH(QMarginsF, margins);
    LOAD_TABLEVIEW("plaintableview.qml");

    const qreal expectedItemWidth = 100;
    const qreal expectedItemHeight = 50;
    const int expectedItemCount = tableSize.width() * tableSize.height();

    tableView->setModel(model);
    tableView->setRowSpacing(spacing.height());
    tableView->setColumnSpacing(spacing.width());

    // Setting margins on Flickable should not affect the layout of the
    // delegate items, since the margins is "transparent" to the TableView.
    tableView->setLeftMargin(margins.left());
    tableView->setTopMargin(margins.top());
    tableView->setRightMargin(margins.right());
    tableView->setBottomMargin(margins.bottom());

    WAIT_UNTIL_POLISHED;

    auto const items = tableViewPrivate->loadedItems;
    QVERIFY(!items.isEmpty());

    for (int i = 0; i < expectedItemCount; ++i) {
        const QQuickItem *item = items[i]->item;
        QVERIFY(item);
        QCOMPARE(item->parentItem(), tableView->contentItem());

        const QPoint cell = getContextRowAndColumn(item);
        qreal expectedX = cell.x() * (expectedItemWidth + spacing.width());
        qreal expectedY = cell.y() * (expectedItemHeight + spacing.height());
        QCOMPARE(item->x(), expectedX);
        QCOMPARE(item->y(), expectedY);
        QCOMPARE(item->z(), 1);
        QCOMPARE(item->width(), expectedItemWidth);
        QCOMPARE(item->height(), expectedItemHeight);
    }
}

void tst_QQuickTableView::checkFocusRemoved_data()
{
    QTest::addColumn<QString>("focusedItemProp");

    QTest::newRow("delegate root") << QStringLiteral("delegateRoot");
    QTest::newRow("delegate child") << QStringLiteral("delegateChild");
}

void tst_QQuickTableView::checkFocusRemoved()
{
    // Check that we clear the focus of a delegate item when
    // a child of the delegate item has focus, and the cell is
    // flicked out of view.
    QFETCH(QString, focusedItemProp);
    LOAD_TABLEVIEW("tableviewfocus.qml");

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    auto const item = tableViewPrivate->loadedTableItem(QPoint(0, 0))->item;
    auto const focusedItem = qvariant_cast<QQuickItem *>(item->property(focusedItemProp.toUtf8().data()));
    QVERIFY(focusedItem);
    QCOMPARE(tableView->hasActiveFocus(), false);
    QCOMPARE(focusedItem->hasActiveFocus(), false);

    focusedItem->forceActiveFocus();
    QCOMPARE(tableView->hasActiveFocus(), true);
    QCOMPARE(focusedItem->hasActiveFocus(), true);

    // Flick the focused cell out, and check that none of the
    // items in the table has focus (which means that the reused
    // item lost focus when it was flicked out). But the tableview
    // itself will maintain active focus.
    tableView->setContentX(500);
    QCOMPARE(tableView->hasActiveFocus(), true);
    for (auto fxItem : tableViewPrivate->loadedItems) {
        auto const focusedItem2 = qvariant_cast<QQuickItem *>(fxItem->item->property(focusedItemProp.toUtf8().data()));
        QCOMPARE(focusedItem2->hasActiveFocus(), false);
    }
}

void tst_QQuickTableView::fillTableViewButNothingMore_data()
{
    QTest::addColumn<QSizeF>("spacing");

    QTest::newRow("0 0,0 0") << QSizeF(0, 0);
    QTest::newRow("0 10,10 0") << QSizeF(10, 10);
    QTest::newRow("100 10,10 0") << QSizeF(10, 10);
    QTest::newRow("0 0,0 100") << QSizeF(0, 0);
    QTest::newRow("0 10,10 100") << QSizeF(10, 10);
    QTest::newRow("100 10,10 100") << QSizeF(10, 10);
}

void tst_QQuickTableView::fillTableViewButNothingMore()
{
    // Check that we end up filling the whole visible part of
    // the tableview with cells, but nothing more.
    QFETCH(QSizeF, spacing);
    LOAD_TABLEVIEW("plaintableview.qml");

    const int rows = 100;
    const int columns = 100;
    auto model = TestModelAsVariant(rows, columns);

    tableView->setModel(model);
    tableView->setRowSpacing(spacing.height());
    tableView->setColumnSpacing(spacing.width());

    WAIT_UNTIL_POLISHED;

    auto const topLeftFxItem = tableViewPrivate->loadedTableItem(QPoint(0, 0));
    auto const topLeftItem = topLeftFxItem->item;

    auto const bottomRightLoadedCell = QPoint(tableViewPrivate->rightColumn(), tableViewPrivate->bottomRow());
    auto const bottomRightFxItem = tableViewPrivate->loadedTableItem(bottomRightLoadedCell);
    auto const bottomRightItem = bottomRightFxItem->item;
    const QPoint bottomRightCell = getContextRowAndColumn(bottomRightItem.data());

    // Check that the right-most item is overlapping the right edge of the view
    QVERIFY(bottomRightItem->x() < tableView->width());
    QVERIFY(bottomRightItem->x() + bottomRightItem->width() >= tableView->width() - spacing.width());

    // Check that the actual number of columns matches what we expect
    qreal cellWidth = bottomRightItem->width() + spacing.width();
    int expectedColumns = qCeil(tableView->width() / cellWidth);
    int actualColumns = bottomRightCell.x() + 1;
    QCOMPARE(actualColumns, expectedColumns);

    // Check that the bottom-most item is overlapping the bottom edge of the view
    QVERIFY(bottomRightItem->y() < tableView->height());
    QVERIFY(bottomRightItem->y() + bottomRightItem->height() >= tableView->height() - spacing.height());

    // Check that the actual number of rows matches what we expect
    qreal cellHeight = bottomRightItem->height() + spacing.height();
    int expectedRows = qCeil(tableView->height() / cellHeight);
    int actualRows = bottomRightCell.y() + 1;
    QCOMPARE(actualRows, expectedRows);
}

void tst_QQuickTableView::checkInitialAttachedProperties_data()
{
    QTest::addColumn<QVariant>("model");

    QTest::newRow("QAIM") << TestModelAsVariant(4, 4);
    QTest::newRow("Number model") << QVariant::fromValue(4);
    QTest::newRow("QStringList") << QVariant::fromValue(QStringList() << "0" << "1" << "2" << "3");
}

void tst_QQuickTableView::checkInitialAttachedProperties()
{
    // Check that the context and attached properties inside
    // the delegate items are what we expect at start-up.
    QFETCH(QVariant, model);
    LOAD_TABLEVIEW("plaintableview.qml");

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems) {
        const int index = fxItem->index;
        const auto item = fxItem->item;
        const auto context = qmlContext(item.data());
        const QPoint cell = tableViewPrivate->cellAtModelIndex(index);
        const int contextIndex = context->contextProperty("index").toInt();
        const QPoint contextCell = getContextRowAndColumn(item.data());
        const QString contextModelData = context->contextProperty("modelData").toString();

        QCOMPARE(contextCell.y(), cell.y());
        QCOMPARE(contextCell.x(), cell.x());
        QCOMPARE(contextIndex, index);
        QCOMPARE(contextModelData, QStringLiteral("%1").arg(cell.y()));
        QCOMPARE(getAttachedObject(item)->view(), tableView);
    }
}

void tst_QQuickTableView::checkSpacingValues()
{
    LOAD_TABLEVIEW("tableviewdefaultspacing.qml");

    int rowCount = 9;
    int columnCount = 9;
    int delegateWidth = 15;
    int delegateHeight = 10;
    auto model = TestModelAsVariant(rowCount, columnCount);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    // Default spacing : 0
    QCOMPARE(tableView->rowSpacing(), 0);
    QCOMPARE(tableView->columnSpacing(), 0);

    tableView->polish();
    WAIT_UNTIL_POLISHED;

    qreal expectedContentWidth = columnCount * (delegateWidth + tableView->columnSpacing()) - tableView->columnSpacing();
    qreal expectedContentHeight = rowCount * (delegateHeight + tableView->rowSpacing()) - tableView->rowSpacing();
    QCOMPARE(tableView->contentWidth(), expectedContentWidth);
    QCOMPARE(tableView->contentHeight(), expectedContentHeight);

    // Valid spacing assignment
    tableView->setRowSpacing(42);
    tableView->setColumnSpacing(12);
    QCOMPARE(tableView->rowSpacing(), 42);
    QCOMPARE(tableView->columnSpacing(), 12);

    tableView->polish();
    WAIT_UNTIL_POLISHED;

    expectedContentWidth = columnCount * (delegateWidth + tableView->columnSpacing()) - tableView->columnSpacing();
    expectedContentHeight = rowCount * (delegateHeight + tableView->rowSpacing()) - tableView->rowSpacing();
    QCOMPARE(tableView->contentWidth(), expectedContentWidth);
    QCOMPARE(tableView->contentHeight(), expectedContentHeight);

    // Negative spacing is allowed, and can be used to eliminate double edges
    // in the grid if the delegate is a rectangle with a border.
    tableView->setRowSpacing(-1);
    tableView->setColumnSpacing(-1);
    QCOMPARE(tableView->rowSpacing(), -1);
    QCOMPARE(tableView->columnSpacing(), -1);

    tableView->setRowSpacing(10);
    tableView->setColumnSpacing(10);
    // Invalid assignments (should ignore)
    tableView->setRowSpacing(INFINITY);
    tableView->setColumnSpacing(INFINITY);
    tableView->setRowSpacing(NAN);
    tableView->setColumnSpacing(NAN);
    QCOMPARE(tableView->rowSpacing(), 10);
    QCOMPARE(tableView->columnSpacing(), 10);
}

void tst_QQuickTableView::checkDelegateParent()
{
    // Check that TableView sets the delegate parent before
    // bindings are evaluated, so that the app can bind to it.
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    QVERIFY(view->rootObject()->property("delegateParentSetBeforeCompleted").toBool());
}

void tst_QQuickTableView::flick_data()
{
    QTest::addColumn<QSizeF>("spacing");
    QTest::addColumn<QMarginsF>("margins");
    QTest::addColumn<bool>("reuseItems");

    QTest::newRow("s:0 m:0 reuse") << QSizeF(0, 0) << QMarginsF(0, 0, 0, 0) << true;
    QTest::newRow("s:5 m:0 reuse") << QSizeF(5, 5) << QMarginsF(0, 0, 0, 0) << true;
    QTest::newRow("s:0 m:20 reuse") << QSizeF(0, 0) << QMarginsF(20, 20, 20, 20) << true;
    QTest::newRow("s:5 m:20 reuse") << QSizeF(5, 5) << QMarginsF(20, 20, 20, 20) << true;
    QTest::newRow("s:0 m:0") << QSizeF(0, 0) << QMarginsF(0, 0, 0, 0) << false;
    QTest::newRow("s:5 m:0") << QSizeF(5, 5) << QMarginsF(0, 0, 0, 0) << false;
    QTest::newRow("s:0 m:20") << QSizeF(0, 0) << QMarginsF(20, 20, 20, 20) << false;
    QTest::newRow("s:5 m:20") << QSizeF(5, 5) << QMarginsF(20, 20, 20, 20) << false;
}

void tst_QQuickTableView::flick()
{
    // Check that if we end up with the correct start and end column/row as we flick around
    // with different table configurations.
    QFETCH(QSizeF, spacing);
    QFETCH(QMarginsF, margins);
    QFETCH(bool, reuseItems);
    LOAD_TABLEVIEW("plaintableview.qml");

    const qreal delegateWidth = 100;
    const qreal delegateHeight = 50;
    const int visualColumnCount = 4;
    const int visualRowCount = 4;
    const qreal cellWidth = delegateWidth + spacing.width();
    const qreal cellHeight = delegateHeight + spacing.height();
    auto model = TestModelAsVariant(100, 100);

    tableView->setModel(model);
    tableView->setRowSpacing(spacing.height());
    tableView->setColumnSpacing(spacing.width());
    tableView->setLeftMargin(margins.left());
    tableView->setTopMargin(margins.top());
    tableView->setRightMargin(margins.right());
    tableView->setBottomMargin(margins.bottom());
    tableView->setReuseItems(reuseItems);
    tableView->setWidth(margins.left() + (visualColumnCount * cellWidth) - spacing.width());
    tableView->setHeight(margins.top() + (visualRowCount * cellHeight) - spacing.height());

    WAIT_UNTIL_POLISHED;

    // Check the "simple" case if the cells never lands egde-to-edge with the viewport. For
    // that case we only accept that visible row/columns are loaded.
    qreal flickValues[] = {0.5, 1.5, 4.5, 20.5, 10.5, 3.5, 1.5, 0.5};

    for (qreal cellsToFlick : flickValues) {
        // Flick to the beginning of the cell
        tableView->setContentX(cellsToFlick * cellWidth);
        tableView->setContentY(cellsToFlick * cellHeight);
        tableView->polish();

        WAIT_UNTIL_POLISHED;

        const int expectedTableLeft = int(cellsToFlick - int((margins.left() + spacing.width()) / cellWidth));
        const int expectedTableTop = int(cellsToFlick - int((margins.top() + spacing.height()) / cellHeight));

        QCOMPARE(tableViewPrivate->leftColumn(), expectedTableLeft);
        QCOMPARE(tableViewPrivate->rightColumn(), expectedTableLeft + visualColumnCount);
        QCOMPARE(tableViewPrivate->topRow(), expectedTableTop);
        QCOMPARE(tableViewPrivate->bottomRow(), expectedTableTop + visualRowCount);
    }
}

void tst_QQuickTableView::flickOvershoot_data()
{
    QTest::addColumn<QSizeF>("spacing");
    QTest::addColumn<QMarginsF>("margins");
    QTest::addColumn<bool>("reuseItems");

    QTest::newRow("s:0 m:0 reuse") << QSizeF(0, 0) << QMarginsF(0, 0, 0, 0) << true;
    QTest::newRow("s:5 m:0 reuse") << QSizeF(5, 5) << QMarginsF(0, 0, 0, 0) << true;
    QTest::newRow("s:0 m:20 reuse") << QSizeF(0, 0) << QMarginsF(20, 20, 20, 20) << true;
    QTest::newRow("s:5 m:20 reuse") << QSizeF(5, 5) << QMarginsF(20, 20, 20, 20) << true;
    QTest::newRow("s:0 m:0") << QSizeF(0, 0) << QMarginsF(0, 0, 0, 0) << false;
    QTest::newRow("s:5 m:0") << QSizeF(5, 5) << QMarginsF(0, 0, 0, 0) << false;
    QTest::newRow("s:0 m:20") << QSizeF(0, 0) << QMarginsF(20, 20, 20, 20) << false;
    QTest::newRow("s:5 m:20") << QSizeF(5, 5) << QMarginsF(20, 20, 20, 20) << false;
}

void tst_QQuickTableView::flickOvershoot()
{
    // Flick the table completely out and then in again, and see
    // that we still contains the expected rows/columns
    // Note that TableView always keeps top-left item loaded, even
    // when everything is flicked out of view.
    QFETCH(QSizeF, spacing);
    QFETCH(QMarginsF, margins);
    QFETCH(bool, reuseItems);
    LOAD_TABLEVIEW("plaintableview.qml");

    const int rowCount = 5;
    const int columnCount = 5;
    const qreal delegateWidth = 100;
    const qreal delegateHeight = 50;
    const qreal cellWidth = delegateWidth + spacing.width();
    const qreal cellHeight = delegateHeight + spacing.height();
    const qreal tableWidth = margins.left() + margins.right() + (cellWidth * columnCount) - spacing.width();
    const qreal tableHeight = margins.top() + margins.bottom() + (cellHeight * rowCount) - spacing.height();
    const int outsideMargin = 10;
    auto model = TestModelAsVariant(rowCount, columnCount);

    tableView->setModel(model);
    tableView->setRowSpacing(spacing.height());
    tableView->setColumnSpacing(spacing.width());
    tableView->setLeftMargin(margins.left());
    tableView->setTopMargin(margins.top());
    tableView->setRightMargin(margins.right());
    tableView->setBottomMargin(margins.bottom());
    tableView->setReuseItems(reuseItems);
    tableView->setWidth(tableWidth - margins.right() - cellWidth / 2);
    tableView->setHeight(tableHeight - margins.bottom() - cellHeight / 2);

    WAIT_UNTIL_POLISHED;

    // Flick table out of view left
    tableView->setContentX(-tableView->width() - outsideMargin);
    tableView->setContentY(0);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->leftColumn(), 0);
    QCOMPARE(tableViewPrivate->rightColumn(), 0);
    QCOMPARE(tableViewPrivate->topRow(), 0);
    QCOMPARE(tableViewPrivate->bottomRow(), rowCount - 1);

    // Flick table out of view right
    tableView->setContentX(tableWidth + outsideMargin);
    tableView->setContentY(0);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->leftColumn(), columnCount - 1);
    QCOMPARE(tableViewPrivate->rightColumn(), columnCount - 1);
    QCOMPARE(tableViewPrivate->topRow(), 0);
    QCOMPARE(tableViewPrivate->bottomRow(), rowCount - 1);

    // Flick table out of view on top
    tableView->setContentX(0);
    tableView->setContentY(-tableView->height() - outsideMargin);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->leftColumn(), 0);
    QCOMPARE(tableViewPrivate->rightColumn(), columnCount - 1);
    QCOMPARE(tableViewPrivate->topRow(), 0);
    QCOMPARE(tableViewPrivate->bottomRow(), 0);

    // Flick table out of view at the bottom
    tableView->setContentX(0);
    tableView->setContentY(tableHeight + outsideMargin);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->leftColumn(), 0);
    QCOMPARE(tableViewPrivate->rightColumn(), columnCount - 1);
    QCOMPARE(tableViewPrivate->topRow(), rowCount - 1);
    QCOMPARE(tableViewPrivate->bottomRow(), rowCount - 1);

    // Flick table out of view left and top at the same time
    tableView->setContentX(-tableView->width() - outsideMargin);
    tableView->setContentY(-tableView->height() - outsideMargin);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->leftColumn(), 0);
    QCOMPARE(tableViewPrivate->rightColumn(), 0);
    QCOMPARE(tableViewPrivate->topRow(), 0);
    QCOMPARE(tableViewPrivate->bottomRow(), 0);

    // Flick table back to origo
    tableView->setContentX(0);
    tableView->setContentY(0);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->leftColumn(), 0);
    QCOMPARE(tableViewPrivate->rightColumn(), columnCount - 1);
    QCOMPARE(tableViewPrivate->topRow(), 0);
    QCOMPARE(tableViewPrivate->bottomRow(), rowCount - 1);

    // Flick table out of view right and bottom at the same time
    tableView->setContentX(tableWidth + outsideMargin);
    tableView->setContentY(tableHeight + outsideMargin);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->leftColumn(), columnCount - 1);
    QCOMPARE(tableViewPrivate->rightColumn(), columnCount - 1);
    QCOMPARE(tableViewPrivate->topRow(), rowCount - 1);
    QCOMPARE(tableViewPrivate->bottomRow(), rowCount - 1);

    // Flick table back to origo
    tableView->setContentX(0);
    tableView->setContentY(0);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->leftColumn(), 0);
    QCOMPARE(tableViewPrivate->rightColumn(), columnCount - 1);
    QCOMPARE(tableViewPrivate->topRow(), 0);
    QCOMPARE(tableViewPrivate->bottomRow(), rowCount - 1);
}

void tst_QQuickTableView::checkRowColumnCount()
{
    // If we flick several columns (rows) at the same time, check that we don't
    // end up with loading more delegate items into memory than necessary. We
    // should free up columns as we go before loading new ones.
    LOAD_TABLEVIEW("countingtableview.qml");

    const char *maxDelegateCountProp = "maxDelegateCount";
    const qreal delegateWidth = 100;
    const qreal delegateHeight = 50;
    auto model = TestModelAsVariant(100, 100);
    const auto &loadedRows = tableViewPrivate->loadedRows;
    const auto &loadedColumns = tableViewPrivate->loadedColumns;

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    // We expect that the number of created items after start-up should match
    //the size of the visible table, pluss one extra preloaded row and column.
    const int qmlCountAfterInit = view->rootObject()->property(maxDelegateCountProp).toInt();
    const int expectedCount = (loadedColumns.count() + 1) * (loadedRows.count() + 1);
    QCOMPARE(qmlCountAfterInit, expectedCount);

    // This test will keep track of the maximum number of delegate items TableView
    // had to show at any point while flicking (in countingtableview.qml). Because
    // of the geometries chosen for TableView and the delegate, only complete columns
    // will be shown at start-up.
    QVERIFY(loadedRows.count() > loadedColumns.count());
    QCOMPARE(tableViewPrivate->loadedTableOuterRect.width(), tableView->width());
    QCOMPARE(tableViewPrivate->loadedTableOuterRect.height(), tableView->height());

    // Flick half an item to the left+up, to force one extra column and row to load before we
    // start. By doing so, we end up showing the maximum number of rows and columns that will
    // ever be shown in the view. This will make things less complicated below, when checking
    // how many items that end up visible while flicking.
    tableView->setContentX(delegateWidth / 2);
    tableView->setContentY(delegateHeight / 2);
    const int qmlCountAfterFirstFlick = view->rootObject()->property(maxDelegateCountProp).toInt();

    // Flick a long distance right
    tableView->setContentX(tableView->width() * 2);

    const int qmlCountAfterLongFlick = view->rootObject()->property(maxDelegateCountProp).toInt();
    QCOMPARE(qmlCountAfterLongFlick, qmlCountAfterFirstFlick);

    // Flick a long distance down
    tableView->setContentX(tableView->height() * 2);

    const int qmlCountAfterDownFlick = view->rootObject()->property(maxDelegateCountProp).toInt();
    QCOMPARE(qmlCountAfterDownFlick, qmlCountAfterFirstFlick);

    // Flick a long distance left
    tableView->setContentX(0);

    const int qmlCountAfterLeftFlick = view->rootObject()->property(maxDelegateCountProp).toInt();
    QCOMPARE(qmlCountAfterLeftFlick, qmlCountAfterFirstFlick);

    // Flick a long distance up
    tableView->setContentY(0);

    const int qmlCountAfterUpFlick = view->rootObject()->property(maxDelegateCountProp).toInt();
    QCOMPARE(qmlCountAfterUpFlick, qmlCountAfterFirstFlick);
}

void tst_QQuickTableView::modelSignals()
{
    LOAD_TABLEVIEW("plaintableview.qml");

    TestModel model(1, 1);
    tableView->setModel(QVariant::fromValue(&model));
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 1);
    QCOMPARE(tableView->columns(), 1);

    QVERIFY(model.insertRows(0, 1));
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 1);

    QVERIFY(model.removeRows(1, 1));
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 1);
    QCOMPARE(tableView->columns(), 1);

    model.insertColumns(1, 1);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 1);
    QCOMPARE(tableView->columns(), 2);

    model.removeColumns(1, 1);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 1);
    QCOMPARE(tableView->columns(), 1);

    model.setRowCount(10);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 10);
    QCOMPARE(tableView->columns(), 1);

    model.setColumnCount(10);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 10);
    QCOMPARE(tableView->columns(), 10);

    model.setRowCount(0);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 0);
    QCOMPARE(tableView->columns(), 10);

    model.setColumnCount(1);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 0);
    QCOMPARE(tableView->columns(), 1);

    model.setRowCount(10);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 10);
    QCOMPARE(tableView->columns(), 1);

    model.setColumnCount(10);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 10);
    QCOMPARE(tableView->columns(), 10);

    model.clear();
    model.setColumnCount(1);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rows(), 0);
    QCOMPARE(tableView->columns(), 1);
}

void tst_QQuickTableView::checkModelSignalsUpdateLayout()
{
    // Check that if the model rearranges rows and emit the
    // 'layoutChanged' signal, TableView will be updated correctly.
    LOAD_TABLEVIEW("plaintableview.qml");

    TestModel model(0, 1);
    tableView->setModel(QVariant::fromValue(&model));
    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->rows(), 0);
    QCOMPARE(tableView->columns(), 1);

    QString modelRow1Text = QStringLiteral("firstRow");
    QString modelRow2Text = QStringLiteral("secondRow");
    model.insertRow(0);
    model.insertRow(0);
    model.setModelData(QPoint(0, 0), QSize(1, 1), modelRow1Text);
    model.setModelData(QPoint(0, 1), QSize(1, 1), modelRow2Text);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->rows(), 2);
    QCOMPARE(tableView->columns(), 1);

    QString delegate1text = tableViewPrivate->loadedTableItem(QPoint(0, 0))->item->property("modelDataBinding").toString();
    QString delegate2text = tableViewPrivate->loadedTableItem(QPoint(0, 1))->item->property("modelDataBinding").toString();
    QCOMPARE(delegate1text, modelRow1Text);
    QCOMPARE(delegate2text, modelRow2Text);

    model.swapRows(0, 1);
    WAIT_UNTIL_POLISHED;

    delegate1text = tableViewPrivate->loadedTableItem(QPoint(0, 0))->item->property("modelDataBinding").toString();
    delegate2text = tableViewPrivate->loadedTableItem(QPoint(0, 1))->item->property("modelDataBinding").toString();
    QCOMPARE(delegate1text, modelRow2Text);
    QCOMPARE(delegate2text, modelRow1Text);
}

void tst_QQuickTableView::dataChangedSignal()
{
    // Check that bindings to the model inside a delegate gets updated
    // when the model item they bind to changes.
    LOAD_TABLEVIEW("plaintableview.qml");

    const QString prefix(QStringLiteral("changed"));

    TestModel model(10, 10);
    tableView->setModel(QVariant::fromValue(&model));

    WAIT_UNTIL_POLISHED;

    for (auto fxItem : tableViewPrivate->loadedItems) {
        const auto item = tableViewPrivate->loadedTableItem(fxItem->cell)->item;
        const QString modelDataBindingProperty = item->property(kModelDataBindingProp).toString();
        QString expectedModelData = QString::number(fxItem->cell.y());
        QCOMPARE(modelDataBindingProperty, expectedModelData);
    }

    // Change one cell in the model
    model.setModelData(QPoint(0, 0), QSize(1, 1), prefix);

    for (auto fxItem : tableViewPrivate->loadedItems) {
        const QPoint cell = fxItem->cell;
        const auto modelIndex = model.index(cell.y(), cell.x());
        QString expectedModelData = model.data(modelIndex, Qt::DisplayRole).toString();

        const auto item = tableViewPrivate->loadedTableItem(fxItem->cell)->item;
        const QString modelDataBindingProperty = item->property(kModelDataBindingProp).toString();

        QCOMPARE(modelDataBindingProperty, expectedModelData);
    }

    // Change four cells in one go
    model.setModelData(QPoint(1, 0), QSize(2, 2), prefix);

    for (auto fxItem : tableViewPrivate->loadedItems) {
        const QPoint cell = fxItem->cell;
        const auto modelIndex = model.index(cell.y(), cell.x());
        QString expectedModelData = model.data(modelIndex, Qt::DisplayRole).toString();

        const auto item = tableViewPrivate->loadedTableItem(fxItem->cell)->item;
        const QString modelDataBindingProperty = item->property(kModelDataBindingProp).toString();

        QCOMPARE(modelDataBindingProperty, expectedModelData);
    }
}

void tst_QQuickTableView::checkThatPoolIsDrainedWhenReuseIsFalse()
{
    // Check that the reuse pool is drained
    // immediately when setting reuseItems to false.
    LOAD_TABLEVIEW("countingtableview.qml");

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    // The pool should now contain preloaded items
    QVERIFY(tableViewPrivate->tableModel->poolSize() > 0);
    tableView->setReuseItems(false);
    // The pool should now be empty
    QCOMPARE(tableViewPrivate->tableModel->poolSize(), 0);
}

void tst_QQuickTableView::checkIfDelegatesAreReused_data()
{
    QTest::addColumn<bool>("reuseItems");

    QTest::newRow("reuse = true") << true;
    QTest::newRow("reuse = false") << false;
}

void tst_QQuickTableView::checkIfDelegatesAreReused()
{
    // Check that we end up reusing delegate items while flicking if
    // TableView has reuseItems set to true, but otherwise not.
    QFETCH(bool, reuseItems);
    LOAD_TABLEVIEW("countingtableview.qml");

    const qreal delegateWidth = 100;
    const qreal delegateHeight = 50;
    const int pageFlickCount = 4;

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);
    tableView->setReuseItems(reuseItems);

    WAIT_UNTIL_POLISHED;

    // Flick half an item to the side, to force one extra row and column to load before we start.
    // This will make things less complicated below, when checking how many times the items
    // have been reused (all items will then report the same number).
    tableView->setContentX(delegateWidth / 2);
    tableView->setContentY(delegateHeight / 2);
    QCOMPARE(tableViewPrivate->tableModel->poolSize(), 0);

    // Some items have already been pooled and reused after we moved the content view, because
    // we preload one extra row and column at start-up. So reset the count-properties back to 0
    // before we continue.
    for (auto fxItem : tableViewPrivate->loadedItems) {
        fxItem->item->setProperty("pooledCount", 0);
        fxItem->item->setProperty("reusedCount", 0);
    }

    const int visibleColumnCount = tableViewPrivate->loadedColumns.count();
    const int visibleRowCount = tableViewPrivate->loadedRows.count();
    const int delegateCountAfterInit = view->rootObject()->property(kDelegatesCreatedCountProp).toInt();

    for (int column = 1; column <= (visibleColumnCount * pageFlickCount); ++column) {
        // Flick columns to the left (and add one pixel to ensure the left column is completely out)
        tableView->setContentX((delegateWidth * column) + 1);
        // Check that the number of delegate items created so far is what we expect.
        const int delegatesCreatedCount = view->rootObject()->property(kDelegatesCreatedCountProp).toInt();
        int expectedCount = delegateCountAfterInit + (reuseItems ? 0 : visibleRowCount * column);
        QCOMPARE(delegatesCreatedCount, expectedCount);
    }

    // Check that each delegate item has been reused as many times
    // as we have flicked pages (if reuse is enabled).
    for (auto fxItem : tableViewPrivate->loadedItems) {
        int pooledCount = fxItem->item->property("pooledCount").toInt();
        int reusedCount = fxItem->item->property("reusedCount").toInt();
        if (reuseItems) {
            QCOMPARE(pooledCount, pageFlickCount);
            QCOMPARE(reusedCount, pageFlickCount);
        } else {
            QCOMPARE(pooledCount, 0);
            QCOMPARE(reusedCount, 0);
        }
    }
}

void tst_QQuickTableView::checkIfDelegatesAreReusedAsymmetricTableSize()
{
    // Check that we end up reusing all delegate items while flicking, also if the table contain
    // more columns than rows. In that case, if we flick out a whole row, we'll move a lot of
    // items into the pool. And if we then start flicking in columns, we'll only reuse a few of
    // them for each column. Still, we don't want the pool to release the superfluous items after
    // each load, since they are still in circulation and will be needed once we flick in a new
    // row at the end of the test.
    LOAD_TABLEVIEW("countingtableview.qml");

    const int columnCount = 20;
    const int rowCount = 2;
    const qreal delegateWidth = tableView->width() / columnCount;
    const qreal delegateHeight = (tableView->height() / rowCount) + 10;

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    // Let the height of each row be much bigger than the width of each column.
    view->rootObject()->setProperty("delegateWidth", delegateWidth);
    view->rootObject()->setProperty("delegateHeight", delegateHeight);

    WAIT_UNTIL_POLISHED;

    auto initialTopLeftItem = tableViewPrivate->loadedTableItem(QPoint(0, 0))->item;
    QVERIFY(initialTopLeftItem);
    int pooledCount = initialTopLeftItem->property("pooledCount").toInt();
    int reusedCount = initialTopLeftItem->property("reusedCount").toInt();
    QCOMPARE(pooledCount, 0);
    QCOMPARE(reusedCount, 0);

    // Flick half an item left+down, to force one extra row and column to load. By doing
    // so, we force the maximum number of rows and columns to show before we start the test.
    // This will make things less complicated below, when checking how many
    // times the items have been reused (all items will then report the same number).
    tableView->setContentX(delegateWidth * 0.5);
    tableView->setContentY(delegateHeight * 0.5);

    // Since we have flicked half a delegate to the left, the number of visible
    // columns is now one more than the column count were when we started the test.
    const int visibleColumnCount = tableViewPrivate->loadedColumns.count();
    QCOMPARE(visibleColumnCount, columnCount + 1);

    // We expect no items to have been pooled so far
    pooledCount = initialTopLeftItem->property("pooledCount").toInt();
    reusedCount = initialTopLeftItem->property("reusedCount").toInt();
    QCOMPARE(pooledCount, 0);
    QCOMPARE(reusedCount, 0);
    QCOMPARE(tableViewPrivate->tableModel->poolSize(), 0);

    // Flick one row out of view. This will move one whole row of items into the
    // pool without reusing them, since no new row is exposed at the bottom.
    tableView->setContentY(delegateHeight + 1);
    pooledCount = initialTopLeftItem->property("pooledCount").toInt();
    reusedCount = initialTopLeftItem->property("reusedCount").toInt();
    QCOMPARE(pooledCount, 1);
    QCOMPARE(reusedCount, 0);
    QCOMPARE(tableViewPrivate->tableModel->poolSize(), visibleColumnCount);

    const int delegateCountAfterInit = view->rootObject()->property(kDelegatesCreatedCountProp).toInt();

    // Start flicking in a lot of columns, and check that the created count stays the same
    for (int column = 1; column <= 10; ++column) {
        tableView->setContentX((delegateWidth * column) + 10);
        const int delegatesCreatedCount = view->rootObject()->property(kDelegatesCreatedCountProp).toInt();
        // Since we reuse items while flicking, the created count should stay the same
        QCOMPARE(delegatesCreatedCount, delegateCountAfterInit);
        // Since we flick out just as many columns as we flick in, the pool size should stay the same
        QCOMPARE(tableViewPrivate->tableModel->poolSize(), visibleColumnCount);
    }

    // Finally, flick one row back into view (but without flicking so far that we push the third
    // row out and into the pool). The pool should still contain the exact amount of items that
    // we had after we flicked the first row out. And this should be exactly the amount of items
    // needed to load the row back again. And this also means that the pool count should then return
    // back to 0.
    tableView->setContentY(delegateHeight - 1);
    const int delegatesCreatedCount = view->rootObject()->property(kDelegatesCreatedCountProp).toInt();
    QCOMPARE(delegatesCreatedCount, delegateCountAfterInit);
    QCOMPARE(tableViewPrivate->tableModel->poolSize(), 0);
}

void tst_QQuickTableView::checkContextProperties_data()
{
    QTest::addColumn<QVariant>("model");
    QTest::addColumn<bool>("reuseItems");

    auto stringList = QStringList();
    for (int i = 0; i < 100; ++i)
        stringList.append(QString::number(i));

    QTest::newRow("QAIM, reuse=false") << TestModelAsVariant(100, 100) << false;
    QTest::newRow("QAIM, reuse=true") << TestModelAsVariant(100, 100) << true;
    QTest::newRow("Number model, reuse=false") << QVariant::fromValue(100) << false;
    QTest::newRow("Number model, reuse=true") << QVariant::fromValue(100) << true;
    QTest::newRow("QStringList, reuse=false") << QVariant::fromValue(stringList) << false;
    QTest::newRow("QStringList, reuse=true") << QVariant::fromValue(stringList) << true;
}

void tst_QQuickTableView::checkContextProperties()
{
    // Check that the context properties of the delegate items
    // are what we expect while flicking, with or without item recycling.
    QFETCH(QVariant, model);
    QFETCH(bool, reuseItems);
    LOAD_TABLEVIEW("countingtableview.qml");

    const qreal delegateWidth = 100;
    const qreal delegateHeight = 50;
    const int rowCount = 100;
    const int pageFlickCount = 3;

    tableView->setModel(model);
    tableView->setReuseItems(reuseItems);

    WAIT_UNTIL_POLISHED;

    const int visibleRowCount = qMin(tableView->rows(), qCeil(tableView->height() / delegateHeight));
    const int visibleColumnCount = qMin(tableView->columns(), qCeil(tableView->width() / delegateWidth));

    for (int row = 1; row <= (visibleRowCount * pageFlickCount); ++row) {
        // Flick rows up
        tableView->setContentY((delegateHeight * row) + (delegateHeight / 2));
        tableView->polish();

        WAIT_UNTIL_POLISHED;

        for (int col = 0; col < visibleColumnCount; ++col) {
            const auto item = tableViewPrivate->loadedTableItem(QPoint(col, row))->item;
            const auto context = qmlContext(item.data());
            const int contextIndex = context->contextProperty("index").toInt();
            const int contextRow = context->contextProperty("row").toInt();
            const int contextColumn = context->contextProperty("column").toInt();
            const QString contextModelData = context->contextProperty("modelData").toString();

            QCOMPARE(contextIndex, row + (col * rowCount));
            QCOMPARE(contextRow, row);
            QCOMPARE(contextColumn, col);
            QCOMPARE(contextModelData, QStringLiteral("%1").arg(row));
        }
    }
}

void tst_QQuickTableView::checkContextPropertiesQQmlListProperyModel_data()
{
    QTest::addColumn<bool>("reuseItems");

    QTest::newRow("reuse=false") << false;
    QTest::newRow("reuse=true") << true;
}

void tst_QQuickTableView::checkContextPropertiesQQmlListProperyModel()
{
    // Check that the context properties of the delegate items
    // are what we expect while flicking, with or without item recycling.
    // This test hard-codes the model to be a QQmlListPropertyModel from
    // within the qml file.
    QFETCH(bool, reuseItems);
    LOAD_TABLEVIEW("qqmllistpropertymodel.qml");

    const qreal delegateWidth = 100;
    const qreal delegateHeight = 50;
    const int rowCount = 100;
    const int pageFlickCount = 3;

    tableView->setReuseItems(reuseItems);
    tableView->polish();

    WAIT_UNTIL_POLISHED;

    const int visibleRowCount = qMin(tableView->rows(), qCeil(tableView->height() / delegateHeight));
    const int visibleColumnCount = qMin(tableView->columns(), qCeil(tableView->width() / delegateWidth));

    for (int row = 1; row <= (visibleRowCount * pageFlickCount); ++row) {
        // Flick rows up
        tableView->setContentY((delegateHeight * row) + (delegateHeight / 2));
        tableView->polish();

        WAIT_UNTIL_POLISHED;

        for (int col = 0; col < visibleColumnCount; ++col) {
            const auto item = tableViewPrivate->loadedTableItem(QPoint(col, row))->item;
            const auto context = qmlContext(item.data());
            const int contextIndex = context->contextProperty("index").toInt();
            const int contextRow = context->contextProperty("row").toInt();
            const int contextColumn = context->contextProperty("column").toInt();
            const QObject *contextModelData = qvariant_cast<QObject *>(context->contextProperty("modelData"));
            const QString modelDataProperty = contextModelData->property("someCustomProperty").toString();

            QCOMPARE(contextIndex, row + (col * rowCount));
            QCOMPARE(contextRow, row);
            QCOMPARE(contextColumn, col);
            QCOMPARE(modelDataProperty, QStringLiteral("%1").arg(row));
        }
    }
}

void tst_QQuickTableView::checkRowAndColumnChangedButNotIndex()
{
    // Check that context row and column changes even if the index stays the
    // same when the item is reused. This can happen in rare cases if the item
    // is first used at e.g (row 1, col 0), but then reused at (row 0, col 1)
    // while the model has changed row count in-between.
    LOAD_TABLEVIEW("checkrowandcolumnnotchanged.qml");

    TestModel model(2, 1);
    tableView->setModel(QVariant::fromValue(&model));

    WAIT_UNTIL_POLISHED;

    model.removeRow(1);
    model.insertColumn(1);
    tableView->forceLayout();

    const auto item = tableViewPrivate->loadedTableItem(QPoint(1, 0))->item;
    const auto context = qmlContext(item.data());
    const int contextIndex = context->contextProperty("index").toInt();
    const int contextRow = context->contextProperty("row").toInt();
    const int contextColumn = context->contextProperty("column").toInt();

    QCOMPARE(contextIndex, 1);
    QCOMPARE(contextRow, 0);
    QCOMPARE(contextColumn, 1);
}

void tst_QQuickTableView::checkThatWeAlwaysEmitChangedUponItemReused()
{
    // Check that we always emit changes to index when we reuse an item, even
    // if it doesn't change. This is needed since the model can have changed
    // row or column count while the item was in the pool, which means that
    // any data referred to by the index property inside the delegate
    // will change too. So we need to refresh any bindings to index.
    // QTBUG-79209
    LOAD_TABLEVIEW("checkalwaysemit.qml");

    TestModel model(1, 1);
    tableView->setModel(QVariant::fromValue(&model));
    model.setModelData(QPoint(0, 0), QSize(1, 1), "old value");

    WAIT_UNTIL_POLISHED;

    const auto reuseItem = tableViewPrivate->loadedTableItem(QPoint(0, 0))->item;
    const auto context = qmlContext(reuseItem.data());

    // Remove the cell/row that has "old value" as model data, and
    // add a new one right after. The new cell will have the same
    // index, but with no model data assigned.
    // This change will not be detected by items in the pool. But since
    // we emit indexChanged when the item is reused, it will be updated then.
    model.removeRow(0);
    model.insertRow(0);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(context->contextProperty("index").toInt(), 0);
    QCOMPARE(context->contextProperty("row").toInt(), 0);
    QCOMPARE(context->contextProperty("column").toInt(), 0);
    QCOMPARE(context->contextProperty("modelDataFromIndex").toString(), "");
}

void tst_QQuickTableView::checkChangingModelFromDelegate()
{
    // Check that we don't restart a rebuild of the table
    // while we're in the middle of rebuilding it from before
    LOAD_TABLEVIEW("changemodelfromdelegate.qml");

    // Set addRowFromDelegate. This will trigger the QML code to add a new
    // row and call forceLayout(). When TableView instantiates the first
    // delegate in the new row, the Component.onCompleted handler will try to
    // add a new row. But since we're currently rebuilding, this should be
    // scheduled for later.
    view->rootObject()->setProperty("addRowFromDelegate", true);

    // We now expect two rows in the table, one more than initially
    QCOMPARE(tableViewPrivate->tableSize.height(), 2);
    QCOMPARE(tableViewPrivate->loadedRows.count(), 2);

    // And since the QML code tried to add another row as well, we
    // expect rebuildScheduled to be true, and a polish event to be pending.
    QVERIFY(tableViewPrivate->scheduledRebuildOptions);
    QVERIFY(tableViewPrivate->polishScheduled);
    WAIT_UNTIL_POLISHED;

    // After handling the polish event, we expect also the third row to now be added
    QCOMPARE(tableViewPrivate->tableSize.height(), 3);
    QCOMPARE(tableViewPrivate->loadedRows.count(), 3);
}

void tst_QQuickTableView::checkRebuildViewportOnly()
{
    // Check that we only rebuild from the current top-left cell
    // when you add or remove rows and columns. There should be
    // no need to do a rebuild from scratch in such cases.
    LOAD_TABLEVIEW("countingtableview.qml");

    const char *propName = "delegatesCreatedCount";
    const qreal delegateWidth = 100;
    const qreal delegateHeight = 50;

    TestModel model(100, 100);
    tableView->setModel(QVariant::fromValue(&model));

    WAIT_UNTIL_POLISHED;

    // Flick to row/column 50, 50
    tableView->setContentX(delegateWidth * 50);
    tableView->setContentY(delegateHeight * 50);

    // Set reuse items to false, just to make it easier to
    // check the number of items created during a rebuild.
    tableView->setReuseItems(false);
    const int itemCountBeforeRebuild = tableViewPrivate->loadedItems.size();

    // Since all cells have the same size, we expect that we end up creating
    // the same amount of items that were already showing before, even after
    // adding or removing rows and columns.
    view->rootObject()->setProperty(propName, 0);
    model.insertRow(51);
    WAIT_UNTIL_POLISHED;
    int countAfterRebuild = view->rootObject()->property(propName).toInt();
    QCOMPARE(countAfterRebuild, itemCountBeforeRebuild);

    view->rootObject()->setProperty(propName, 0);
    model.removeRow(51);
    WAIT_UNTIL_POLISHED;
    countAfterRebuild = view->rootObject()->property(propName).toInt();
    QCOMPARE(countAfterRebuild, itemCountBeforeRebuild);

    view->rootObject()->setProperty(propName, 0);
    model.insertColumn(51);
    WAIT_UNTIL_POLISHED;
    countAfterRebuild = view->rootObject()->property(propName).toInt();
    QCOMPARE(countAfterRebuild, itemCountBeforeRebuild);

    view->rootObject()->setProperty(propName, 0);
    model.removeColumn(51);
    WAIT_UNTIL_POLISHED;
    countAfterRebuild = view->rootObject()->property(propName).toInt();
    QCOMPARE(countAfterRebuild, itemCountBeforeRebuild);
}

void tst_QQuickTableView::useDelegateChooserWithoutDefault()
{
    // Check that the application issues a warning (but doesn't e.g
    // crash) if the delegate chooser doesn't cover all cells
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*failed"));
    LOAD_TABLEVIEW("usechooserwithoutdefault.qml");
    auto model = TestModelAsVariant(2, 1);
    tableView->setModel(model);
    WAIT_UNTIL_POLISHED;
};

void tst_QQuickTableView::checkTableviewInsideAsyncLoader()
{
    // Check that you can put a TableView inside an async Loader, and
    // that the delegate items are created before the loader is ready.
    LOAD_TABLEVIEW_ASYNC("asyncplain.qml");

    // At this point the Loader has finished
    QCOMPARE(loader->status(), QQuickLoader::Ready);

    // Check that TableView has finished building
    QVERIFY(!tableViewPrivate->scheduledRebuildOptions);
    QCOMPARE(tableViewPrivate->rebuildState, QQuickTableViewPrivate::RebuildState::Done);

    // Check that all expected delegate items have been loaded
    const qreal delegateWidth = 100;
    const qreal delegateHeight = 50;
    int expectedColumns = qCeil(tableView->width() / delegateWidth);
    int expectedRows = qCeil(tableView->height() / delegateHeight);
    QCOMPARE(tableViewPrivate->loadedColumns.count(), expectedColumns);
    QCOMPARE(tableViewPrivate->loadedRows.count(), expectedRows);

    // Check that the loader was still in a loading state while TableView was creating
    // delegate items. If we delayed creating delegate items until we got the first
    // updatePolish() callback in QQuickTableView, this would not be the case.
    auto statusWhenDelegate0_0Completed = qvariant_cast<QQuickLoader::Status>(
                loader->item()->property("statusWhenDelegate0_0Created"));
    auto statusWhenDelegate5_5Completed = qvariant_cast<QQuickLoader::Status>(
                loader->item()->property("statusWhenDelegate5_5Created"));
    QCOMPARE(statusWhenDelegate0_0Completed, QQuickLoader::Loading);
    QCOMPARE(statusWhenDelegate5_5Completed, QQuickLoader::Loading);

    // Check that TableView had a valid geometry when we started to build. If the build
    // was started too early (e.g upon QQuickTableView::componentComplete), width and
    // height would still be 0 since the bindings would not have been evaluated yet.
    qreal width = loader->item()->property("tableViewWidthWhileBuilding").toReal();
    qreal height = loader->item()->property("tableViewHeightWhileBuilding").toReal();
    QVERIFY(width > 0);
    QVERIFY(height > 0);
};

#define INT_LIST(indices) QVariant::fromValue(QList<int>() << indices)

void tst_QQuickTableView::hideRowsAndColumns_data()
{
    QTest::addColumn<QVariant>("rowsToHide");
    QTest::addColumn<QVariant>("columnsToHide");

    const auto emptyList = QVariant::fromValue(QList<int>());

    // Hide rows
    QTest::newRow("first") << INT_LIST(0) << emptyList;
    QTest::newRow("middle 1") << INT_LIST(1) << emptyList;
    QTest::newRow("middle 3") << INT_LIST(3) << emptyList;
    QTest::newRow("last") << INT_LIST(4) << emptyList;

    QTest::newRow("subsequent 0,1") << INT_LIST(0 << 1) << emptyList;
    QTest::newRow("subsequent 1,2") << INT_LIST(1 << 2) << emptyList;
    QTest::newRow("subsequent 3,4") << INT_LIST(3 << 4) << emptyList;

    QTest::newRow("all but first") << INT_LIST(1 << 2 << 3 << 4) << emptyList;
    QTest::newRow("all but last") << INT_LIST(0 << 1 << 2 << 3) << emptyList;
    QTest::newRow("all but middle") << INT_LIST(0 << 1 << 3 << 4) << emptyList;

    // Hide columns
    QTest::newRow("first") << emptyList << INT_LIST(0);
    QTest::newRow("middle 1") << emptyList << INT_LIST(1);
    QTest::newRow("middle 3") << emptyList << INT_LIST(3);
    QTest::newRow("last") << emptyList << INT_LIST(4);

    QTest::newRow("subsequent 0,1") << emptyList << INT_LIST(0 << 1);
    QTest::newRow("subsequent 1,2") << emptyList << INT_LIST(1 << 2);
    QTest::newRow("subsequent 3,4") << emptyList << INT_LIST(3 << 4);

    QTest::newRow("all but first") << emptyList << INT_LIST(1 << 2 << 3 << 4);
    QTest::newRow("all but last") << emptyList << INT_LIST(0 << 1 << 2 << 3);
    QTest::newRow("all but middle") << emptyList << INT_LIST(0 << 1 << 3 << 4);

    // Hide both rows and columns at the same time
    QTest::newRow("first") << INT_LIST(0) << INT_LIST(0);
    QTest::newRow("middle 1") << INT_LIST(1) << INT_LIST(1);
    QTest::newRow("middle 3") << INT_LIST(3) << INT_LIST(3);
    QTest::newRow("last") << INT_LIST(4) << INT_LIST(4);

    QTest::newRow("subsequent 0,1") << INT_LIST(0 << 1) << INT_LIST(0 << 1);
    QTest::newRow("subsequent 1,2") << INT_LIST(1 << 2) << INT_LIST(1 << 2);
    QTest::newRow("subsequent 3,4") << INT_LIST(3 << 4) << INT_LIST(3 << 4);

    QTest::newRow("all but first") << INT_LIST(1 << 2 << 3 << 4) << INT_LIST(1 << 2 << 3 << 4);
    QTest::newRow("all but last") << INT_LIST(0 << 1 << 2 << 3) << INT_LIST(0 << 1 << 2 << 3);
    QTest::newRow("all but middle") << INT_LIST(0 << 1 << 3 << 4) << INT_LIST(0 << 1 << 3 << 4);

    // Hide all rows and columns
    QTest::newRow("all") << INT_LIST(0 << 1 << 2 << 3 << 4) << INT_LIST(0 << 1 << 2 << 3 << 4);
}

void tst_QQuickTableView::hideRowsAndColumns()
{
    // Check that you can hide the first row (corner case)
    // and that we load the other columns as expected.
    QFETCH(QVariant, rowsToHide);
    QFETCH(QVariant, columnsToHide);
    LOAD_TABLEVIEW("hiderowsandcolumns.qml");

    const QList<int> rowsToHideList = qvariant_cast<QList<int>>(rowsToHide);
    const QList<int> columnsToHideList = qvariant_cast<QList<int>>(columnsToHide);
    const int modelSize = 5;
    auto model = TestModelAsVariant(modelSize, modelSize);
    view->rootObject()->setProperty("rowsToHide", rowsToHide);
    view->rootObject()->setProperty("columnsToHide", columnsToHide);

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    const int expectedRowCount = modelSize - rowsToHideList.size();
    const int expectedColumnCount = modelSize - columnsToHideList.size();
    QCOMPARE(tableViewPrivate->loadedRows.count(), expectedRowCount);
    QCOMPARE(tableViewPrivate->loadedColumns.count(), expectedColumnCount);

    for (const int row : tableViewPrivate->loadedRows)
        QVERIFY(!rowsToHideList.contains(row));

    for (const int column : tableViewPrivate->loadedColumns)
        QVERIFY(!columnsToHideList.contains(column));
}

void tst_QQuickTableView::hideAndShowFirstColumn()
{
    // Check that if we hide the first column, it will move
    // the second column to the origin of the viewport. Then check
    // that if we show the first column again, it will reappear at
    // the origin of the viewport, and as such, pushing the second
    // column to the right of it.
    LOAD_TABLEVIEW("hiderowsandcolumns.qml");

    const int modelSize = 5;
    auto model = TestModelAsVariant(modelSize, modelSize);
    tableView->setModel(model);

    // Start by making the first column hidden
    const auto columnsToHideList = QList<int>() << 0;
    view->rootObject()->setProperty("columnsToHide", QVariant::fromValue(columnsToHideList));

    WAIT_UNTIL_POLISHED;

    const int expectedColumnCount = modelSize - columnsToHideList.size();
    QCOMPARE(tableViewPrivate->loadedColumns.count(), expectedColumnCount);
    QCOMPARE(tableViewPrivate->leftColumn(), 1);
    QCOMPARE(tableView->contentX(), 0);
    QCOMPARE(tableViewPrivate->loadedTableOuterRect.x(), 0);

    // Make the first column in the model visible again
    const auto emptyList = QList<int>();
    view->rootObject()->setProperty("columnsToHide", QVariant::fromValue(emptyList));
    tableView->forceLayout();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedColumns.count(), modelSize);
    QCOMPARE(tableViewPrivate->leftColumn(), 0);
    QCOMPARE(tableView->contentX(), 0);
    QCOMPARE(tableViewPrivate->loadedTableOuterRect.x(), 0);
}

void tst_QQuickTableView::hideAndShowFirstRow()
{
    // Check that if we hide the first row, it will move
    // the second row to the origin of the viewport. Then check
    // that if we show the first row again, it will reappear at
    // the origin of the viewport, and as such, pushing the second
    // row below it.
    LOAD_TABLEVIEW("hiderowsandcolumns.qml");

    const int modelSize = 5;
    auto model = TestModelAsVariant(modelSize, modelSize);
    tableView->setModel(model);

    // Start by making the first row hidden
    const auto rowsToHideList = QList<int>() << 0;
    view->rootObject()->setProperty("rowsToHide", QVariant::fromValue(rowsToHideList));

    WAIT_UNTIL_POLISHED;

    const int expectedRowsCount = modelSize - rowsToHideList.size();
    QCOMPARE(tableViewPrivate->loadedRows.count(), expectedRowsCount);
    QCOMPARE(tableViewPrivate->topRow(), 1);
    QCOMPARE(tableView->contentY(), 0);
    QCOMPARE(tableViewPrivate->loadedTableOuterRect.y(), 0);

    // Make the first row in the model visible again
    const auto emptyList = QList<int>();
    view->rootObject()->setProperty("rowsToHide", QVariant::fromValue(emptyList));
    tableView->forceLayout();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewPrivate->loadedRows.count(), modelSize);
    QCOMPARE(tableViewPrivate->topRow(), 0);
    QCOMPARE(tableView->contentY(), 0);
    QCOMPARE(tableViewPrivate->loadedTableOuterRect.y(), 0);
}

void tst_QQuickTableView::checkThatRevisionedPropertiesCannotBeUsedInOldImports()
{
    // Check that if you use a QQmlAdaptorModel together with a Repeater, the
    // revisioned context properties 'row' and 'column' are not accessible.
    LOAD_TABLEVIEW("checkmodelpropertyrevision.qml");
    const int resolvedRow = view->rootObject()->property("resolvedDelegateRow").toInt();
    const int resolvedColumn = view->rootObject()->property("resolvedDelegateColumn").toInt();
    QCOMPARE(resolvedRow, 42);
    QCOMPARE(resolvedColumn, 42);
}

void tst_QQuickTableView::checkSyncView_rootView_data()
{
    QTest::addColumn<qreal>("flickToPos");
    QTest::addColumn<qreal>("rowSpacing");
    QTest::addColumn<qreal>("columnSpacing");
    QTest::addColumn<qreal>("leftMargin");
    QTest::addColumn<qreal>("rightMargin");
    QTest::addColumn<qreal>("topMargin");
    QTest::addColumn<qreal>("bottomMargin");

    QTest::newRow("pos:110") << 110. << 0. << 0. << 0. << 0. << 0. << 0.;
    QTest::newRow("pos:2010") << 2010. << 0. << 0. << 0. << 0. << 0. << 0.;

    QTest::newRow("pos:110, spacing") << 110. << 10. << 20. << 0. << 0. << 0. << 0.;
    QTest::newRow("pos:2010, spacing") << 2010. << 10. << 20. << 0. << 0. << 0. << 0.;

    QTest::newRow("pos:110, margins") << 110. << 0. << 0. << 10. << 10. << 20. << 20.;
    QTest::newRow("pos:2010, margins") << 2010. << 0. << 0. << 10. << 10. << 20. << 20.;
}

void tst_QQuickTableView::checkSyncView_rootView()
{
    // Check that if you flick on the root tableview (the view that has
    // no other view as syncView), all the other tableviews will sync
    // their content view position according to their syncDirection flag.
    QFETCH(qreal, flickToPos);
    QFETCH(qreal, rowSpacing);
    QFETCH(qreal, columnSpacing);
    QFETCH(qreal, leftMargin);
    QFETCH(qreal, rightMargin);
    QFETCH(qreal, topMargin);
    QFETCH(qreal, bottomMargin);

    LOAD_TABLEVIEW("syncviewsimple.qml");
    GET_QML_TABLEVIEW(tableViewH);
    GET_QML_TABLEVIEW(tableViewV);
    GET_QML_TABLEVIEW(tableViewHV);
    QQuickTableView *views[] = {tableViewH, tableViewV, tableViewHV};

    auto model = TestModelAsVariant(100, 100);

    tableView->setModel(model);
    for (auto view : views)
        view->setModel(model);

    tableView->setRowSpacing(rowSpacing);
    tableView->setColumnSpacing(columnSpacing);
    tableView->setLeftMargin(leftMargin);
    tableView->setRightMargin(rightMargin);
    tableView->setTopMargin(topMargin);
    tableView->setBottomMargin(bottomMargin);
    tableView->setContentX(flickToPos);
    tableView->setContentY(flickToPos);

    WAIT_UNTIL_POLISHED;

    // Check that geometry properties are mirrored accoring to sync direction
    QCOMPARE(tableViewH->columnSpacing(), tableView->columnSpacing());
    QCOMPARE(tableViewH->rowSpacing(), 0);
    QCOMPARE(tableViewH->contentWidth(), tableView->contentWidth());
    QCOMPARE(tableViewH->leftMargin(), tableView->leftMargin());
    QCOMPARE(tableViewH->rightMargin(), tableView->rightMargin());
    QCOMPARE(tableViewH->topMargin(), 0);
    QCOMPARE(tableViewH->bottomMargin(), 0);
    QCOMPARE(tableViewV->columnSpacing(), 0);
    QCOMPARE(tableViewV->rowSpacing(), tableView->rowSpacing());
    QCOMPARE(tableViewV->contentHeight(), tableView->contentHeight());
    QCOMPARE(tableViewV->topMargin(), tableView->topMargin());
    QCOMPARE(tableViewV->bottomMargin(), tableView->bottomMargin());
    QCOMPARE(tableViewV->leftMargin(), 0);
    QCOMPARE(tableViewV->rightMargin(), 0);

    // Check that viewport is in sync after the flick
    QCOMPARE(tableView->contentX(), flickToPos);
    QCOMPARE(tableView->contentY(), flickToPos);
    QCOMPARE(tableViewH->contentX(), tableView->contentX());
    QCOMPARE(tableViewH->contentY(), 0);
    QCOMPARE(tableViewV->contentX(), 0);
    QCOMPARE(tableViewV->contentY(), tableView->contentY());
    QCOMPARE(tableViewHV->contentX(), tableView->contentX());
    QCOMPARE(tableViewHV->contentY(), tableView->contentY());

    // Check that topLeft cell is in sync after the flick
    QCOMPARE(tableViewHPrivate->leftColumn(), tableViewPrivate->leftColumn());
    QCOMPARE(tableViewHPrivate->rightColumn(), tableViewPrivate->rightColumn());
    QCOMPARE(tableViewHPrivate->topRow(), 0);
    QCOMPARE(tableViewVPrivate->leftColumn(), 0);
    QCOMPARE(tableViewVPrivate->topRow(), tableViewPrivate->topRow());
    QCOMPARE(tableViewHVPrivate->leftColumn(), tableViewPrivate->leftColumn());
    QCOMPARE(tableViewHVPrivate->topRow(), tableViewPrivate->topRow());

    // Check that the geometry of the tables are in sync after the flick
    QCOMPARE(tableViewHPrivate->loadedTableOuterRect.left(), tableViewPrivate->loadedTableOuterRect.left());
    QCOMPARE(tableViewHPrivate->loadedTableOuterRect.right(), tableViewPrivate->loadedTableOuterRect.right());
    QCOMPARE(tableViewHPrivate->loadedTableOuterRect.top(), 0);

    QCOMPARE(tableViewVPrivate->loadedTableOuterRect.top(), tableViewPrivate->loadedTableOuterRect.top());
    QCOMPARE(tableViewVPrivate->loadedTableOuterRect.bottom(), tableViewPrivate->loadedTableOuterRect.bottom());
    QCOMPARE(tableViewVPrivate->loadedTableOuterRect.left(), 0);

    QCOMPARE(tableViewHVPrivate->loadedTableOuterRect, tableViewPrivate->loadedTableOuterRect);

    // Check that the column widths are in sync
    for (int column = tableView->leftColumn(); column < tableView->rightColumn(); ++column) {
        QCOMPARE(tableViewH->columnWidth(column), tableView->columnWidth(column));
        QCOMPARE(tableViewHV->columnWidth(column), tableView->columnWidth(column));
    }

    // Check that the row heights are in sync
    for (int row = tableView->topRow(); row < tableView->bottomRow(); ++row) {
        QCOMPARE(tableViewV->rowHeight(row), tableView->rowHeight(row));
        QCOMPARE(tableViewHV->rowHeight(row), tableView->rowHeight(row));
    }
}

void tst_QQuickTableView::checkSyncView_childViews_data()
{
    QTest::addColumn<int>("viewIndexToFlick");
    QTest::addColumn<qreal>("flickToPos");

    QTest::newRow("tableViewH, pos:100") << 0 << 100.;
    QTest::newRow("tableViewV, pos:100") << 1 << 100.;
    QTest::newRow("tableViewHV, pos:100") << 2 << 100.;
    QTest::newRow("tableViewH, pos:2000") << 0 << 2000.;
    QTest::newRow("tableViewV, pos:2000") << 1 << 2000.;
    QTest::newRow("tableViewHV, pos:2000") << 2 << 2000.;
}

void tst_QQuickTableView::checkSyncView_childViews()
{
    // Check that if you flick on a tableview that has a syncView, the
    // syncView will move to the new position as well, which will also
    // recursivly move all other connected child views of the syncView.
    QFETCH(int, viewIndexToFlick);
    QFETCH(qreal, flickToPos);
    LOAD_TABLEVIEW("syncviewsimple.qml");
    GET_QML_TABLEVIEW(tableViewH);
    GET_QML_TABLEVIEW(tableViewV);
    GET_QML_TABLEVIEW(tableViewHV);
    QQuickTableView *views[] = {tableViewH, tableViewV, tableViewHV};
    QQuickTableView *viewToFlick = views[viewIndexToFlick];
    QQuickTableViewPrivate *viewToFlickPrivate = QQuickTableViewPrivate::get(viewToFlick);

    auto model = TestModelAsVariant(100, 100);

    tableView->setModel(model);
    for (auto view : views)
        view->setModel(model);

    viewToFlick->setContentX(flickToPos);
    viewToFlick->setContentY(flickToPos);

    WAIT_UNTIL_POLISHED;

    // The view the user flicks on can always be flicked in both directions
    // (unless is has a flickingDirection set, which is not the case here).
    QCOMPARE(viewToFlick->contentX(), flickToPos);
    QCOMPARE(viewToFlick->contentY(), flickToPos);

    // The root view (tableView) will move in sync according
    // to the syncDirection of the view being flicked.
    if (viewToFlick->syncDirection() & Qt::Horizontal) {
        QCOMPARE(tableView->contentX(), flickToPos);
        QCOMPARE(tableViewPrivate->leftColumn(), viewToFlickPrivate->leftColumn());
        QCOMPARE(tableViewPrivate->rightColumn(), viewToFlickPrivate->rightColumn());
        QCOMPARE(tableViewPrivate->loadedTableOuterRect.left(), viewToFlickPrivate->loadedTableOuterRect.left());
        QCOMPARE(tableViewPrivate->loadedTableOuterRect.right(), viewToFlickPrivate->loadedTableOuterRect.right());
    } else {
        QCOMPARE(tableView->contentX(), 0);
        QCOMPARE(tableViewPrivate->leftColumn(), 0);
        QCOMPARE(tableViewPrivate->loadedTableOuterRect.left(), 0);
    }

    if (viewToFlick->syncDirection() & Qt::Vertical) {
        QCOMPARE(tableView->contentY(), flickToPos);
        QCOMPARE(tableViewPrivate->topRow(), viewToFlickPrivate->topRow());
        QCOMPARE(tableViewPrivate->bottomRow(), viewToFlickPrivate->bottomRow());
        QCOMPARE(tableViewPrivate->loadedTableOuterRect.top(), viewToFlickPrivate->loadedTableOuterRect.top());
        QCOMPARE(tableViewPrivate->loadedTableOuterRect.bottom(), viewToFlickPrivate->loadedTableOuterRect.bottom());
    } else {
        QCOMPARE(tableView->contentY(), 0);
        QCOMPARE(tableViewPrivate->topRow(), 0);
        QCOMPARE(tableViewPrivate->loadedTableOuterRect.top(), 0);
    }

    // The other views should continue to stay in sync with
    // the root view, unless it was the view being flicked.
    if (viewToFlick != tableViewH) {
        QCOMPARE(tableViewH->contentX(), tableView->contentX());
        QCOMPARE(tableViewH->contentY(), 0);
        QCOMPARE(tableViewHPrivate->leftColumn(), tableViewPrivate->leftColumn());
        QCOMPARE(tableViewHPrivate->rightColumn(), tableViewPrivate->rightColumn());
        QCOMPARE(tableViewHPrivate->loadedTableOuterRect.left(), tableViewPrivate->loadedTableOuterRect.left());
        QCOMPARE(tableViewHPrivate->loadedTableOuterRect.right(), tableViewPrivate->loadedTableOuterRect.right());
        QCOMPARE(tableViewHPrivate->topRow(), 0);
        QCOMPARE(tableViewHPrivate->loadedTableOuterRect.top(), 0);
    }

    if (viewToFlick != tableViewV) {
        QCOMPARE(tableViewV->contentX(), 0);
        QCOMPARE(tableViewV->contentY(), tableView->contentY());
        QCOMPARE(tableViewVPrivate->topRow(), tableViewPrivate->topRow());
        QCOMPARE(tableViewVPrivate->bottomRow(), tableViewPrivate->bottomRow());
        QCOMPARE(tableViewVPrivate->loadedTableOuterRect.top(), tableViewPrivate->loadedTableOuterRect.top());
        QCOMPARE(tableViewVPrivate->loadedTableOuterRect.bottom(), tableViewPrivate->loadedTableOuterRect.bottom());
        QCOMPARE(tableViewVPrivate->leftColumn(), 0);
        QCOMPARE(tableViewVPrivate->loadedTableOuterRect.left(), 0);
    }

    if (viewToFlick != tableViewHV) {
        QCOMPARE(tableViewHV->contentX(), tableView->contentX());
        QCOMPARE(tableViewHV->contentY(), tableView->contentY());
        QCOMPARE(tableViewHVPrivate->leftColumn(), tableViewPrivate->leftColumn());
        QCOMPARE(tableViewHVPrivate->rightColumn(), tableViewPrivate->rightColumn());
        QCOMPARE(tableViewHVPrivate->topRow(), tableViewPrivate->topRow());
        QCOMPARE(tableViewHVPrivate->bottomRow(), tableViewPrivate->bottomRow());
        QCOMPARE(tableViewHVPrivate->loadedTableOuterRect, tableViewPrivate->loadedTableOuterRect);
    }

    // Check that the column widths are in sync
    for (int column = tableView->leftColumn(); column < tableView->rightColumn(); ++column) {
        QCOMPARE(tableViewH->columnWidth(column), tableView->columnWidth(column));
        QCOMPARE(tableViewHV->columnWidth(column), tableView->columnWidth(column));
    }

    // Check that the row heights are in sync
    for (int row = tableView->topRow(); row < tableView->bottomRow(); ++row) {
        QCOMPARE(tableViewV->rowHeight(row), tableView->rowHeight(row));
        QCOMPARE(tableViewHV->rowHeight(row), tableView->rowHeight(row));
    }
}

void tst_QQuickTableView::checkSyncView_differentSizedModels()
{
    // Check that you can have two tables in a syncView relation, where
    // the sync "child" has fewer rows/columns than the syncView. In that
    // case, it will be possible to flick the syncView further out than
    // the child have rows/columns to follow. This causes some extra
    // challenges for TableView to ensure that they are still kept in
    // sync once you later flick the syncView back to a point where both
    // tables ends up visible. This test will check this sitiation.
    LOAD_TABLEVIEW("syncviewsimple.qml");
    GET_QML_TABLEVIEW(tableViewH);
    GET_QML_TABLEVIEW(tableViewV);
    GET_QML_TABLEVIEW(tableViewHV);

    auto tableViewModel = TestModelAsVariant(100, 100);
    auto tableViewHModel = TestModelAsVariant(100, 50);
    auto tableViewVModel = TestModelAsVariant(50, 100);
    auto tableViewHVModel = TestModelAsVariant(5, 5);

    tableView->setModel(tableViewModel);
    tableViewH->setModel(tableViewHModel);
    tableViewV->setModel(tableViewVModel);
    tableViewHV->setModel(tableViewHVModel);

    WAIT_UNTIL_POLISHED;

    // Flick far out, beyond the smaller tables, which will
    // also force a rebuild (and as such, cause layout properties
    // like average cell width to be temporarily out of sync).
    tableView->setContentX(5000);
    QVERIFY(tableViewPrivate->scheduledRebuildOptions);

    WAIT_UNTIL_POLISHED;

    // Check that the smaller tables are now flicked out of view
    qreal leftEdge = tableViewPrivate->loadedTableOuterRect.left();
    QVERIFY(tableViewHPrivate->loadedTableOuterRect.right() < leftEdge);
    QVERIFY(tableViewHVPrivate->loadedTableOuterRect.right() < leftEdge);

    // Flick slowly back so that we don't trigger a rebuild (since
    // we want to check that we stay in sync also when not rebuilding).
    while (tableView->contentX() > 200) {
        tableView->setContentX(tableView->contentX() - 200);
        QVERIFY(!tableViewPrivate->rebuildOptions);
        QVERIFY(!tableViewPrivate->polishScheduled);
    }

    leftEdge = tableViewPrivate->loadedTableOuterRect.left();
    const int leftColumn = tableViewPrivate->leftColumn();
    QCOMPARE(tableViewHPrivate->loadedTableOuterRect.left(), leftEdge);
    QCOMPARE(tableViewHPrivate->leftColumn(), leftColumn);

    // Because the tableView was fast flicked and then slowly flicked back, the
    // left column is now 49, which is actually far too high, since we're almost
    // at the beginning of the content view. But this "miscalculation" is expected
    // when the column widths are increasing for each column, like they do in this
    // test. In that case, the algorithm that predicts where each column should end
    // up gets slightly confused. Anyway, check that tableViewHV, that has only
    // 5 columns, is not showing any columns, since it should always stay in sync with
    // syncView regardless of the content view position.
    QVERIFY(tableViewHVPrivate->loadedColumns.isEmpty());
}

void tst_QQuickTableView::checkSyncView_differentGeometry()
{
    // Check that you can have two tables in a syncView relation, where
    // the sync "child" is larger than the sync view. This means that the
    // child will display more rows and columns than the parent.
    // In that case, the sync view will anyway need to load the same rows
    // and columns as the child, otherwise the column and row sizes
    // cannot be determined for the child.
    LOAD_TABLEVIEW("syncviewsimple.qml");
    GET_QML_TABLEVIEW(tableViewH);
    GET_QML_TABLEVIEW(tableViewV);
    GET_QML_TABLEVIEW(tableViewHV);

    tableView->setWidth(40);
    tableView->setHeight(40);

    auto tableViewModel = TestModelAsVariant(100, 100);

    tableView->setModel(tableViewModel);
    tableViewH->setModel(tableViewModel);
    tableViewV->setModel(tableViewModel);
    tableViewHV->setModel(tableViewModel);

    WAIT_UNTIL_POLISHED;

    // Check that the column widths are in sync
    for (int column = tableViewH->leftColumn(); column < tableViewH->rightColumn(); ++column) {
        QCOMPARE(tableViewH->columnWidth(column), tableView->columnWidth(column));
        QCOMPARE(tableViewHV->columnWidth(column), tableView->columnWidth(column));
    }

    // Check that the row heights are in sync
    for (int row = tableViewV->topRow(); row < tableViewV->bottomRow(); ++row) {
        QCOMPARE(tableViewV->rowHeight(row), tableView->rowHeight(row));
        QCOMPARE(tableViewHV->rowHeight(row), tableView->rowHeight(row));
    }

    // Flick a bit, and do the same test again
    tableView->setContentX(200);
    tableView->setContentY(200);
    WAIT_UNTIL_POLISHED;

    // Check that the column widths are in sync
    for (int column = tableViewH->leftColumn(); column < tableViewH->rightColumn(); ++column) {
        QCOMPARE(tableViewH->columnWidth(column), tableView->columnWidth(column));
        QCOMPARE(tableViewHV->columnWidth(column), tableView->columnWidth(column));
    }

    // Check that the row heights are in sync
    for (int row = tableViewV->topRow(); row < tableViewV->bottomRow(); ++row) {
        QCOMPARE(tableViewV->rowHeight(row), tableView->rowHeight(row));
        QCOMPARE(tableViewHV->rowHeight(row), tableView->rowHeight(row));
    }
}

void tst_QQuickTableView::checkSyncView_connect_late_data()
{
    QTest::addColumn<qreal>("flickToPos");

    QTest::newRow("pos:110") << 110.;
    QTest::newRow("pos:2010") << 2010.;
}

void tst_QQuickTableView::checkSyncView_connect_late()
{
    // Check that if you assign a syncView to a TableView late, and
    // after the views have been flicked around, they will still end up in sync.
    QFETCH(qreal, flickToPos);
    LOAD_TABLEVIEW("syncviewsimple.qml");
    GET_QML_TABLEVIEW(tableViewH);
    GET_QML_TABLEVIEW(tableViewV);
    GET_QML_TABLEVIEW(tableViewHV);
    QQuickTableView *views[] = {tableViewH, tableViewV, tableViewHV};

    auto model = TestModelAsVariant(100, 100);

    tableView->setModel(model);

    // Start with no syncView connections
    for (auto view : views) {
        view->setSyncView(nullptr);
        view->setModel(model);
    }

    WAIT_UNTIL_POLISHED;

    tableView->setContentX(flickToPos);
    tableView->setContentY(flickToPos);

    WAIT_UNTIL_POLISHED;

    // Check that viewport is not in sync after the flick
    QCOMPARE(tableView->contentX(), flickToPos);
    QCOMPARE(tableView->contentY(), flickToPos);
    QCOMPARE(tableViewH->contentX(), 0);
    QCOMPARE(tableViewH->contentY(), 0);
    QCOMPARE(tableViewV->contentX(), 0);
    QCOMPARE(tableViewV->contentY(), 0);
    QCOMPARE(tableViewHV->contentX(), 0);
    QCOMPARE(tableViewHV->contentY(), 0);

    // Assign the syncView late. This should make
    // all the views end up in sync.
    for (auto view : views) {
        view->setSyncView(tableView);
        WAIT_UNTIL_POLISHED_ARG(view);
    }

    // Check that geometry properties are mirrored
    QCOMPARE(tableViewH->columnSpacing(), tableView->columnSpacing());
    QCOMPARE(tableViewH->rowSpacing(), 0);
    QCOMPARE(tableViewH->contentWidth(), tableView->contentWidth());
    QCOMPARE(tableViewV->columnSpacing(), 0);
    QCOMPARE(tableViewV->rowSpacing(), tableView->rowSpacing());
    QCOMPARE(tableViewV->contentHeight(), tableView->contentHeight());

    // Check that viewport is in sync after the flick
    QCOMPARE(tableView->contentX(), flickToPos);
    QCOMPARE(tableView->contentY(), flickToPos);
    QCOMPARE(tableViewH->contentX(), tableView->contentX());
    QCOMPARE(tableViewH->contentY(), 0);
    QCOMPARE(tableViewV->contentX(), 0);
    QCOMPARE(tableViewV->contentY(), tableView->contentY());
    QCOMPARE(tableViewHV->contentX(), tableView->contentX());
    QCOMPARE(tableViewHV->contentY(), tableView->contentY());

    // Check that topLeft cell is in sync after the flick
    QCOMPARE(tableViewHPrivate->leftColumn(), tableViewPrivate->leftColumn());
    QCOMPARE(tableViewHPrivate->rightColumn(), tableViewPrivate->rightColumn());
    QCOMPARE(tableViewHPrivate->topRow(), 0);
    QCOMPARE(tableViewVPrivate->leftColumn(), 0);
    QCOMPARE(tableViewVPrivate->topRow(), tableViewPrivate->topRow());
    QCOMPARE(tableViewHVPrivate->leftColumn(), tableViewPrivate->leftColumn());
    QCOMPARE(tableViewHVPrivate->topRow(), tableViewPrivate->topRow());

    // Check that the geometry of the tables are in sync after the flick
    QCOMPARE(tableViewHPrivate->loadedTableOuterRect.left(), tableViewPrivate->loadedTableOuterRect.left());
    QCOMPARE(tableViewHPrivate->loadedTableOuterRect.right(), tableViewPrivate->loadedTableOuterRect.right());
    QCOMPARE(tableViewHPrivate->loadedTableOuterRect.top(), 0);

    QCOMPARE(tableViewVPrivate->loadedTableOuterRect.top(), tableViewPrivate->loadedTableOuterRect.top());
    QCOMPARE(tableViewVPrivate->loadedTableOuterRect.bottom(), tableViewPrivate->loadedTableOuterRect.bottom());
    QCOMPARE(tableViewVPrivate->loadedTableOuterRect.left(), 0);

    QCOMPARE(tableViewHVPrivate->loadedTableOuterRect, tableViewPrivate->loadedTableOuterRect);
}

void tst_QQuickTableView::checkSyncView_pageFlicking()
{
    // Check that we rebuild the syncView (instead of refilling
    // edges), if the sync child moves more than a page (the size of TableView).
    // The point is that it shouldn't matter if you fast-flick the
    // sync view itself, or a sync child. Either way, the sync view
    // needs to rebuild. This, in turn, will eventually rebuild the
    // sync children as well when they sync up later.
    LOAD_TABLEVIEW("syncviewsimple.qml");
    GET_QML_TABLEVIEW(tableViewHV);

    auto model = TestModelAsVariant(100, 100);

    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    // Move the viewport more than a "page"
    tableViewHV->setContentX(tableViewHV->width() * 2);

    QVERIFY(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::ViewportOnly);
    QVERIFY(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::CalculateNewTopLeftColumn);
    QVERIFY(!(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::CalculateNewTopLeftRow));

    WAIT_UNTIL_POLISHED;

    tableViewHV->setContentY(tableViewHV->height() * 2);

    QVERIFY(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::ViewportOnly);
    QVERIFY(!(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::CalculateNewTopLeftColumn));
    QVERIFY(tableViewPrivate->scheduledRebuildOptions & QQuickTableViewPrivate::RebuildOption::CalculateNewTopLeftRow);
}

void tst_QQuickTableView::checkSyncView_emptyModel()
{
    // When a tableview has a syncview with an empty model then it should still be
    // showing the tableview without depending on the syncview. This is particularly
    // important for headerviews for example
    LOAD_TABLEVIEW("syncviewsimple.qml");
    GET_QML_TABLEVIEW(tableViewH);
    GET_QML_TABLEVIEW(tableViewV);
    GET_QML_TABLEVIEW(tableViewHV);
    QQuickTableView *views[] = {tableViewH, tableViewV, tableViewHV};

    auto model = TestModelAsVariant(100, 100);

    for (auto view : views)
        view->setModel(model);

    WAIT_UNTIL_POLISHED_ARG(tableViewHV);

    // Check that geometry properties are mirrored
    QCOMPARE(tableViewH->columnSpacing(), tableView->columnSpacing());
    QCOMPARE(tableViewH->rowSpacing(), 0);
    QCOMPARE(tableViewH->contentWidth(), tableView->contentWidth());
    QVERIFY(tableViewH->contentHeight() > 0);
    QCOMPARE(tableViewV->columnSpacing(), 0);
    QCOMPARE(tableViewV->rowSpacing(), tableView->rowSpacing());
    QCOMPARE(tableViewV->contentHeight(), tableView->contentHeight());
    QVERIFY(tableViewV->contentWidth() > 0);

    QCOMPARE(tableViewH->contentX(), tableView->contentX());
    QCOMPARE(tableViewH->contentY(), 0);
    QCOMPARE(tableViewV->contentX(), 0);
    QCOMPARE(tableViewV->contentY(), tableView->contentY());
    QCOMPARE(tableViewHV->contentX(), tableView->contentX());
    QCOMPARE(tableViewHV->contentY(), tableView->contentY());

    QCOMPARE(tableViewHPrivate->loadedTableOuterRect.left(), tableViewPrivate->loadedTableOuterRect.left());
    QCOMPARE(tableViewHPrivate->loadedTableOuterRect.top(), 0);

    QCOMPARE(tableViewVPrivate->loadedTableOuterRect.top(), tableViewPrivate->loadedTableOuterRect.top());
    QCOMPARE(tableViewVPrivate->loadedTableOuterRect.left(), 0);
}

void tst_QQuickTableView::checkSyncView_topLeftChanged()
{
    LOAD_TABLEVIEW("syncviewsimple.qml");
    GET_QML_TABLEVIEW(tableViewH);
    GET_QML_TABLEVIEW(tableViewV);
    GET_QML_TABLEVIEW(tableViewHV);
    QQuickTableView *views[] = {tableViewH, tableViewV, tableViewHV};

    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    for (auto view : views)
        view->setModel(model);

    tableView->setColumnWidthProvider(QJSValue());
    tableView->setRowHeightProvider(QJSValue());
    view->rootObject()->setProperty("delegateWidth", 300);
    view->rootObject()->setProperty("delegateHeight", 300);
    tableView->forceLayout();

    tableViewHV->setContentX(350);
    tableViewHV->setContentY(350);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableViewH->leftColumn(), tableView->leftColumn());
    QCOMPARE(tableViewV->topRow(), tableView->topRow());

    view->rootObject()->setProperty("delegateWidth", 50);
    view->rootObject()->setProperty("delegateHeight", 50);
    tableView->forceLayout();

    QCOMPARE(tableViewH->leftColumn(), tableView->leftColumn());
    QCOMPARE(tableViewV->topRow(), tableView->topRow());
}

void tst_QQuickTableView::checkThatFetchMoreIsCalledWhenScrolledToTheEndOfTable()
{
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(5, 5, true);
    tableView->setModel(model);
    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->rows(), 5);
    QCOMPARE(tableView->columns(), 5);

    // Flick table out of view on top
    tableView->setContentX(0);
    tableView->setContentY(-tableView->height() - 10);
    tableView->polish();
    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->rows(), 6);
    QCOMPARE(tableView->columns(), 5);
}

void tst_QQuickTableView::delegateWithRequiredProperties()
{
    constexpr static int PositionRole = Qt::UserRole+1;
    struct MyTable : QAbstractTableModel {


        using QAbstractTableModel::QAbstractTableModel;

        int rowCount(const QModelIndex& = QModelIndex()) const override {
            return 3;
        }

        int columnCount(const QModelIndex& = QModelIndex()) const override {
            return 3;
        }

        QVariant data(const QModelIndex &index, int = Qt::DisplayRole) const override {
            return QVariant::fromValue(QString::asprintf("R%d:C%d", index.row(), index.column()));
        }

        QHash<int, QByteArray> roleNames() const override {
            return QHash<int, QByteArray> { {PositionRole, "position"} };
        }
    };

    auto model =  QVariant::fromValue(QSharedPointer<MyTable>(new MyTable));
    {
        QTest::ignoreMessage(QtMsgType::QtInfoMsg, "success");
        LOAD_TABLEVIEW("delegateWithRequired.qml");
        QVERIFY(tableView);
        tableView->setModel(model);
        WAIT_UNTIL_POLISHED;
        QVERIFY(view->errors().empty());
    }
    {
        QTest::ignoreMessage(QtMsgType::QtWarningMsg, QRegularExpression(R"|(TableView: failed loading index: \d)|"));
        LOAD_TABLEVIEW("delegatewithRequiredUnset.qml");
        QVERIFY(tableView);
        tableView->setModel(model);
        WAIT_UNTIL_POLISHED;
        QTRY_VERIFY(view->errors().empty());
    }
}

void tst_QQuickTableView::replaceModel()
{
    LOAD_TABLEVIEW("replaceModelTableView.qml");

    const auto objectModel = view->rootObject()->property("objectModel");
    const auto listModel = view->rootObject()->property("listModel");
    const auto delegateModel = view->rootObject()->property("delegateModel");

    tableView->setModel(listModel);
    QTRY_COMPARE(tableView->rows(), 2);
    tableView->setModel(objectModel);
    QTRY_COMPARE(tableView->rows(), 3);
    tableView->setModel(delegateModel);
    QTRY_COMPARE(tableView->rows(), 2);
    tableView->setModel(listModel);
    QTRY_COMPARE(tableView->rows(), 2);
    tableView->setModel(QVariant());
    QTRY_COMPARE(tableView->rows(), 0);
    QCOMPARE(tableView->contentWidth(), 0);
    QCOMPARE(tableView->contentHeight(), 0);
}

void tst_QQuickTableView::cellAtPos_data()
{
    QTest::addColumn<QPointF>("contentStartPos");
    QTest::addColumn<QPointF>("localPos");
    QTest::addColumn<bool>("includeSpacing");
    QTest::addColumn<QPoint>("expectedCell");
    QTest::addColumn<QSizeF>("margins");

    const int spacing = 10;
    const QPointF cellSize(100, 50);
    const QPointF halfCell = cellSize / 2;
    const QPointF quadSpace(spacing / 4, spacing / 4);

    auto cellStart = [&](int column, int row){
        const qreal x = (column * (cellSize.x() + spacing));
        const qreal y = (row * (cellSize.y() + spacing));
        return QPointF(x, y);
    };

    QTest::newRow("1") << QPointF(0, 0) << cellStart(0, 0) << false << QPoint(0, 0) << QSizeF(0, 0);
    QTest::newRow("2") << QPointF(0, 0) << cellStart(1, 0) << false << QPoint(1, 0) << QSizeF(0, 0);
    QTest::newRow("3") << QPointF(0, 0) << cellStart(0, 1) << false << QPoint(0, 1) << QSizeF(0, 0);
    QTest::newRow("4") << QPointF(0, 0) << cellStart(1, 1) << false << QPoint(1, 1) << QSizeF(0, 0);

    QTest::newRow("5") << QPointF(0, 0) << cellStart(1, 1) - quadSpace << false << QPoint(-1, -1) << QSizeF(0, 0);
    QTest::newRow("6") << QPointF(0, 0) << cellStart(0, 0) + cellSize + quadSpace << false << QPoint(-1, -1) << QSizeF(0, 0);
    QTest::newRow("7") << QPointF(0, 0) << cellStart(0, 1) + cellSize + quadSpace << false << QPoint(-1, -1) << QSizeF(0, 0);

    QTest::newRow("8") << QPointF(0, 0) << cellStart(1, 1) - quadSpace << true << QPoint(1, 1) << QSizeF(0, 0);
    QTest::newRow("9") << QPointF(0, 0) << cellStart(0, 0) + cellSize + quadSpace << true << QPoint(0, 0) << QSizeF(0, 0);
    QTest::newRow("10") << QPointF(0, 0) << cellStart(0, 1) + cellSize + quadSpace << true << QPoint(0, 1) << QSizeF(0, 0);

    QTest::newRow("11") << cellStart(50, 50) << cellStart(50, 50) << false << QPoint(50, 50) << QSizeF(0, 0);
    QTest::newRow("12") << cellStart(50, 50) << cellStart(54, 54) << false << QPoint(54, 54) << QSizeF(0, 0);
    QTest::newRow("13") << cellStart(50, 50) << cellStart(54, 54) - quadSpace << false << QPoint(-1, -1) << QSizeF(0, 0);
    QTest::newRow("14") << cellStart(50, 50) << cellStart(54, 54) + cellSize + quadSpace << false << QPoint(-1, -1) << QSizeF(0, 0);
    QTest::newRow("15") << cellStart(50, 50) << cellStart(54, 54) - quadSpace << true << QPoint(54, 54) << QSizeF(0, 0);
    QTest::newRow("16") << cellStart(50, 50) << cellStart(54, 54) + cellSize + quadSpace << true << QPoint(54, 54) << QSizeF(0, 0);

    QTest::newRow("17") << cellStart(50, 50) + halfCell << cellStart(50, 50) << false << QPoint(50, 50) << QSizeF(0, 0);
    QTest::newRow("18") << cellStart(50, 50) + halfCell << cellStart(51, 51) << false << QPoint(51, 51) << QSizeF(0, 0);
    QTest::newRow("19") << cellStart(50, 50) + halfCell << cellStart(54, 54) << false << QPoint(54, 54) << QSizeF(0, 0);

    QTest::newRow("20") << QPointF(0, 0) << cellStart(0, 0) << false << QPoint(0, 0) << QSizeF(150, 150);
    QTest::newRow("20") << QPointF(0, 0) << cellStart(5, 5) << false << QPoint(5, 5) << QSizeF(150, 150);

    QTest::newRow("20") << QPointF(-150, -150) << cellStart(0, 0) << false << QPoint(0, 0) << QSizeF(150, 150);
    QTest::newRow("21") << QPointF(-150, -150) << cellStart(4, 0) + halfCell << false << QPoint(4, 0) << QSizeF(150, 150);
    QTest::newRow("22") << QPointF(-150, -150) << cellStart(0, 4) + halfCell << false << QPoint(0, 4) << QSizeF(150, 150);
    QTest::newRow("23") << QPointF(-150, -150) << cellStart(4, 4) + halfCell << false << QPoint(4, 4) << QSizeF(150, 150);
}

void tst_QQuickTableView::cellAtPos()
{
    QFETCH(QPointF, contentStartPos);
    QFETCH(QPointF, localPos);
    QFETCH(bool, includeSpacing);
    QFETCH(QPoint, expectedCell);
    QFETCH(QSizeF, margins);

    LOAD_TABLEVIEW("plaintableview.qml");
    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);
    tableView->setRowSpacing(10);
    tableView->setColumnSpacing(10);
    tableView->setLeftMargin(margins.width());
    tableView->setLeftMargin(margins.height());
    tableView->setTopMargin(margins.height());
    tableView->setContentX(contentStartPos.x());
    tableView->setContentY(contentStartPos.y());

    WAIT_UNTIL_POLISHED;

    QPoint cell = tableView->cellAtPosition(localPos, includeSpacing);
    QCOMPARE(cell, expectedCell);
}

void tst_QQuickTableView::positionViewAtRow_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<QQuickTableView::PositionModeFlag>("alignment");
    QTest::addColumn<qreal>("offset");
    QTest::addColumn<QRectF>("subRect");
    QTest::addColumn<qreal>("contentYStartPos");

    QRectF subRects[] = { QRectF(), QRectF(11, 12, 13, 14) };

    for (auto subRect : subRects) {
        QTest::newRow("AlignTop 0") << 0 << QQuickTableView::AlignTop << 0. << subRect << 0.;
        QTest::newRow("AlignTop 1") << 1 << QQuickTableView::AlignTop << 0. << subRect << 0.;
        QTest::newRow("AlignTop 1") << 1 << QQuickTableView::AlignTop << 0. << subRect << 50.;
        QTest::newRow("AlignTop 50") << 50 << QQuickTableView::AlignTop << 0. << subRect << -1.;
        QTest::newRow("AlignTop 0") << 0 << QQuickTableView::AlignTop << 0. << subRect << -1.;
        QTest::newRow("AlignTop 1") << 1 << QQuickTableView::AlignTop << -10. << subRect << 0.;
        QTest::newRow("AlignTop 1") << 1 << QQuickTableView::AlignTop << -10. << subRect << 50.;
        QTest::newRow("AlignTop 50") << 50 << QQuickTableView::AlignTop << -10. << subRect << -1.;

        QTest::newRow("AlignBottom 50") << 50 << QQuickTableView::AlignBottom << 0. << subRect << -1.;
        QTest::newRow("AlignBottom 98") << 98 << QQuickTableView::AlignBottom << 0. << subRect << -1.;
        QTest::newRow("AlignBottom 99") << 99 << QQuickTableView::AlignBottom << 0. << subRect << -1.;
        QTest::newRow("AlignBottom 50") << 40 << QQuickTableView::AlignBottom << 10. << subRect << -1.;
        QTest::newRow("AlignBottom 40") << 50 << QQuickTableView::AlignBottom << -10. << subRect << -1.;
        QTest::newRow("AlignBottom 98") << 98 << QQuickTableView::AlignBottom << 10. << subRect << -1.;
        QTest::newRow("AlignBottom 99") << 99 << QQuickTableView::AlignBottom << -10. << subRect << -1.;

        QTest::newRow("AlignCenter 40") << 40 << QQuickTableView::AlignCenter << 0. << subRect << -1.;
        QTest::newRow("AlignCenter 50") << 50 << QQuickTableView::AlignCenter << 0. << subRect << -1.;
        QTest::newRow("AlignCenter 40") << 40 << QQuickTableView::AlignCenter << 10. << subRect << -1.;
        QTest::newRow("AlignCenter 50") << 50 << QQuickTableView::AlignCenter << -10. << subRect << -1.;
    }
}

void tst_QQuickTableView::positionViewAtRow()
{
    // Check that positionViewAtRow actually flicks the view
    // to the right position so that the row becomes visible.
    // For this test, we only check cells that can be placed exactly
    // according to the given alignment.
    QFETCH(int, row);
    QFETCH(QQuickTableView::PositionModeFlag, alignment);
    QFETCH(qreal, offset);
    QFETCH(QRectF, subRect);
    QFETCH(qreal, contentYStartPos);

    LOAD_TABLEVIEW("plaintableview.qml");
    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);
    if (contentYStartPos >= 0)
        tableView->setContentY(contentYStartPos);

    WAIT_UNTIL_POLISHED;

    tableView->positionViewAtRow(row, alignment, offset, subRect);

    if (!tableView->isRowLoaded(row))
        WAIT_UNTIL_POLISHED;

    const QPoint cell(0, row);
    const int modelIndex = tableViewPrivate->modelIndexAtCell(cell);
    QVERIFY(tableViewPrivate->loadedItems.contains(modelIndex));
    const QRectF cellRect = tableViewPrivate->loadedTableItem(cell)->geometry();
    const QRectF alignmentRect = subRect.isValid() ? subRect.translated(cellRect.topLeft()) : cellRect;

    switch (alignment) {
    case QQuickTableView::AlignTop:
        QCOMPARE(alignmentRect.y(), tableView->contentY() - offset);
        break;
    case QQuickTableView::AlignBottom:
        QCOMPARE(alignmentRect.bottom(), tableView->contentY() + tableView->height() - offset);
        break;
    case QQuickTableView::AlignCenter:
        QCOMPARE(alignmentRect.y(), tableView->contentY() + (tableView->height() / 2) - (alignmentRect.height() / 2) - offset);
        break;
    default:
        Q_UNREACHABLE();
    }
}

void tst_QQuickTableView::positionViewAtColumn_data()
{
    QTest::addColumn<int>("column");
    QTest::addColumn<QQuickTableView::PositionModeFlag>("alignment");
    QTest::addColumn<qreal>("offset");
    QTest::addColumn<QRectF>("subRect");
    QTest::addColumn<qreal>("contentXStartPos");

    QRectF subRects[] = { QRectF(), QRectF(11, 12, 13, 14) };

    for (auto subRect : subRects) {
        QTest::newRow("AlignLeft 0") << 0 << QQuickTableView::AlignLeft << 0. << subRect << 0.;
        QTest::newRow("AlignLeft 1") << 1 << QQuickTableView::AlignLeft << 0. << subRect << 0.;
        QTest::newRow("AlignLeft 1") << 1 << QQuickTableView::AlignLeft << 0. << subRect << 50.;
        QTest::newRow("AlignLeft 50") << 50 << QQuickTableView::AlignLeft << 0. << subRect << -1.;
        QTest::newRow("AlignLeft 0") << 0 << QQuickTableView::AlignLeft << 0. << subRect << -1.;
        QTest::newRow("AlignLeft 1") << 1 << QQuickTableView::AlignLeft << -10. << subRect << 0.;
        QTest::newRow("AlignLeft 1") << 1 << QQuickTableView::AlignLeft << -10. << subRect << 50.;
        QTest::newRow("AlignLeft 50") << 50 << QQuickTableView::AlignLeft << -10. << subRect << -1.;

        QTest::newRow("AlignRight 50") << 50 << QQuickTableView::AlignRight << 0. << subRect << -1.;
        QTest::newRow("AlignRight 99") << 99 << QQuickTableView::AlignRight << 0. << subRect << -1.;
        QTest::newRow("AlignRight 50") << 50 << QQuickTableView::AlignRight << 10. << subRect << -1.;
        QTest::newRow("AlignRight 99") << 99 << QQuickTableView::AlignRight << -10. << subRect << -1.;

        QTest::newRow("AlignCenter 40") << 50 << QQuickTableView::AlignCenter << 0. << subRect << -1.;
        QTest::newRow("AlignCenter 50") << 50 << QQuickTableView::AlignCenter << 0. << subRect << -1.;
        QTest::newRow("AlignCenter 40") << 50 << QQuickTableView::AlignCenter << 10. << subRect << -1.;
        QTest::newRow("AlignCenter 50") << 50 << QQuickTableView::AlignCenter << -10. << subRect << -1.;
    }
}

void tst_QQuickTableView::positionViewAtColumn()
{
    // Check that positionViewAtColumn actually flicks the view
    // to the right position so that the row becomes visible.
    // For this test, we only check cells that can be placed exactly
    // according to the given alignment.
    QFETCH(int, column);
    QFETCH(QQuickTableView::PositionModeFlag, alignment);
    QFETCH(qreal, offset);
    QFETCH(QRectF, subRect);
    QFETCH(qreal, contentXStartPos);

    LOAD_TABLEVIEW("plaintableview.qml");
    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);
    if (contentXStartPos >= 0)
        tableView->setContentX(contentXStartPos);

    WAIT_UNTIL_POLISHED;

    tableView->positionViewAtColumn(column, alignment, offset, subRect);

    if (!tableView->isColumnLoaded(column))
        WAIT_UNTIL_POLISHED;

    const QPoint cell(column, 0);
    const int modelIndex = tableViewPrivate->modelIndexAtCell(cell);
    QVERIFY(tableViewPrivate->loadedItems.contains(modelIndex));
    const QRectF cellRect = tableViewPrivate->loadedTableItem(cell)->geometry();
    const QRectF alignmentRect = subRect.isValid() ? subRect.translated(cellRect.topLeft()) : cellRect;

    switch (alignment) {
    case QQuickTableView::AlignLeft:
        QCOMPARE(alignmentRect.x(), tableView->contentX() - offset);
        break;
    case QQuickTableView::AlignRight:
        QCOMPARE(alignmentRect.right(), tableView->contentX() + tableView->width() - offset);
        break;
    case QQuickTableView::AlignCenter:
        QCOMPARE(alignmentRect.x(), tableView->contentX() + (tableView->width() / 2) - (alignmentRect.width() / 2) - offset);
        break;
    default:
        Q_UNREACHABLE();
    }
}

void tst_QQuickTableView::positionViewAtRowClamped_data()
{
    QTest::addColumn<int>("row");
    QTest::addColumn<QQuickTableView::PositionModeFlag>("alignment");
    QTest::addColumn<qreal>("offset");
    QTest::addColumn<QRectF>("subRect");
    QTest::addColumn<qreal>("contentYStartPos");

    QRectF subRects[] = { QRectF(), QRectF(1, 2, 3, 4) };

    for (auto subRect : subRects) {
        QTest::newRow("AlignTop 0") << 0 << QQuickTableView::AlignTop << -10. << subRect << 0.;
        QTest::newRow("AlignTop 0") << 0 << QQuickTableView::AlignTop << -10. << subRect << -1.;
        QTest::newRow("AlignTop 99") << 99 << QQuickTableView::AlignTop << 0. << subRect << -1.;
        QTest::newRow("AlignTop 99") << 99 << QQuickTableView::AlignTop << -10. << subRect << -1.;

        QTest::newRow("AlignBottom 0") << 0 << QQuickTableView::AlignBottom << 0. << subRect << 0.;
        QTest::newRow("AlignBottom 1") << 1 << QQuickTableView::AlignBottom << 0. << subRect << 0.;
        QTest::newRow("AlignBottom 1") << 1 << QQuickTableView::AlignBottom << 0. << subRect << 50.;
        QTest::newRow("AlignBottom 0") << 0 << QQuickTableView::AlignBottom << 0. << subRect << -1.;

        QTest::newRow("AlignBottom 0") << 0 << QQuickTableView::AlignBottom << 10. << subRect << 0.;
        QTest::newRow("AlignBottom 1") << 1 << QQuickTableView::AlignBottom << 10. << subRect << 0.;
        QTest::newRow("AlignBottom 1") << 1 << QQuickTableView::AlignBottom << 10. << subRect << 50.;
        QTest::newRow("AlignBottom 0") << 0 << QQuickTableView::AlignBottom << 10. << subRect << -1.;
        QTest::newRow("AlignBottom 99") << 99 << QQuickTableView::AlignBottom << 50. << subRect << -1.;

        QTest::newRow("AlignCenter 0") << 0 << QQuickTableView::AlignCenter << 0. << subRect << 0.;
        QTest::newRow("AlignCenter 1") << 1 << QQuickTableView::AlignCenter << 0. << subRect << 0.;
        QTest::newRow("AlignCenter 1") << 1 << QQuickTableView::AlignCenter << 0. << subRect << 50.;
        QTest::newRow("AlignCenter 0") << 0 << QQuickTableView::AlignCenter << 0. << subRect << -1.;
        QTest::newRow("AlignCenter 99") << 99 << QQuickTableView::AlignCenter << 0. << subRect << -1.;

        QTest::newRow("AlignCenter 0") << 0 << QQuickTableView::AlignCenter << -10. << subRect << 0.;
        QTest::newRow("AlignCenter 1") << 1 << QQuickTableView::AlignCenter << -10. << subRect << 0.;
        QTest::newRow("AlignCenter 1") << 1 << QQuickTableView::AlignCenter << -10. << subRect << 50.;
        QTest::newRow("AlignCenter 0") << 0 << QQuickTableView::AlignCenter << -10. << subRect << -1.;
        QTest::newRow("AlignCenter 99") << 99 << QQuickTableView::AlignCenter << -10. << subRect << -1.;
    }
}

void tst_QQuickTableView::positionViewAtRowClamped()
{
    // Check that positionViewAtRow actually flicks the table to the
    // right position so that the row becomes visible. For this test, we
    // only test cells that cannot be placed exactly at the given alignment,
    // because it would cause the table to overshoot. Instead the
    // table should be flicked to the edge of the viewport, close to the
    // requested alignment.
    QFETCH(int, row);
    QFETCH(QQuickTableView::PositionModeFlag, alignment);
    QFETCH(qreal, offset);
    QFETCH(QRectF, subRect);
    QFETCH(qreal, contentYStartPos);

    LOAD_TABLEVIEW("plaintableview.qml");
    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);
    if (contentYStartPos >= 0)
        tableView->setContentY(contentYStartPos);

    if (!tableView->isRowLoaded(row))
        WAIT_UNTIL_POLISHED;

    tableView->positionViewAtRow(row, alignment, offset, subRect);

    if (!tableView->isRowLoaded(row))
        WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->contentY(), row < 50 ? 0 : tableView->contentHeight() - tableView->height());
}

void tst_QQuickTableView::positionViewAtColumnClamped_data()
{
    QTest::addColumn<int>("column");
    QTest::addColumn<QQuickTableView::PositionModeFlag>("alignment");
    QTest::addColumn<qreal>("offset");
    QTest::addColumn<QRectF>("subRect");
    QTest::addColumn<qreal>("contentXStartPos");

    QRectF subRects[] = { QRectF(), QRectF(1, 2, 3, 4) };

    for (auto subRect : subRects) {
        QTest::newRow("AlignLeft 0") << 0 << QQuickTableView::AlignLeft << -10. << subRect << 0.;
        QTest::newRow("AlignLeft 0") << 0 << QQuickTableView::AlignLeft << -10. << subRect << -1.;
        QTest::newRow("AlignLeft 99") << 99 << QQuickTableView::AlignLeft << 0. << subRect << -1.;
        QTest::newRow("AlignLeft 99") << 99 << QQuickTableView::AlignLeft << -10. << subRect << -1.;

        QTest::newRow("AlignRight 0") << 0 << QQuickTableView::AlignRight << 0. << subRect << 0.;
        QTest::newRow("AlignRight 1") << 1 << QQuickTableView::AlignRight << 0. << subRect << 0.;
        QTest::newRow("AlignRight 1") << 1 << QQuickTableView::AlignRight << 0. << subRect << 50.;
        QTest::newRow("AlignRight 0") << 0 << QQuickTableView::AlignRight << 0. << subRect << -1.;

        QTest::newRow("AlignRight 0") << 0 << QQuickTableView::AlignRight << 10. << subRect << 0.;
        QTest::newRow("AlignRight 1") << 1 << QQuickTableView::AlignRight << 10. << subRect << 0.;
        QTest::newRow("AlignRight 1") << 1 << QQuickTableView::AlignRight << 10. << subRect << 50.;
        QTest::newRow("AlignRight 0") << 0 << QQuickTableView::AlignRight << 10. << subRect << -1.;
        QTest::newRow("AlignRight 99") << 99 << QQuickTableView::AlignRight << 100. << subRect << -1.;

        QTest::newRow("AlignCenter 0") << 0 << QQuickTableView::AlignCenter << 0. << subRect << 0.;
        QTest::newRow("AlignCenter 1") << 1 << QQuickTableView::AlignCenter << 0. << subRect << 0.;
        QTest::newRow("AlignCenter 1") << 1 << QQuickTableView::AlignCenter << 0. << subRect << 50.;
        QTest::newRow("AlignCenter 0") << 0 << QQuickTableView::AlignCenter << 0. << subRect << -1.;
        QTest::newRow("AlignCenter 99") << 99 << QQuickTableView::AlignCenter << 0. << subRect << -1.;

        QTest::newRow("AlignCenter 0") << 0 << QQuickTableView::AlignCenter << -10. << subRect << 0.;
        QTest::newRow("AlignCenter 1") << 1 << QQuickTableView::AlignCenter << -10. << subRect << 0.;
        QTest::newRow("AlignCenter 1") << 1 << QQuickTableView::AlignCenter << -10. << subRect << 50.;
        QTest::newRow("AlignCenter 0") << 0 << QQuickTableView::AlignCenter << -10. << subRect << -1.;
        QTest::newRow("AlignCenter 99") << 99 << QQuickTableView::AlignCenter << -10. << subRect << -1.;
    }
}

void tst_QQuickTableView::positionViewAtColumnClamped()
{
    // Check that positionViewAtColumn actually flicks the table to the
    // right position so that the column becomes visible. For this test, we
    // only test cells that cannot be placed exactly at the given alignment,
    // because it would cause the table to overshoot. Instead the
    // table should be flicked to the edge of the viewport, close to the
    // requested alignment.
    QFETCH(int, column);
    QFETCH(QQuickTableView::PositionModeFlag, alignment);
    QFETCH(qreal, offset);
    QFETCH(QRectF, subRect);
    QFETCH(qreal, contentXStartPos);

    LOAD_TABLEVIEW("plaintableview.qml");
    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);
    if (contentXStartPos >= 0)
        tableView->setContentX(contentXStartPos);

    WAIT_UNTIL_POLISHED;

    tableView->positionViewAtColumn(column, alignment, offset, subRect);

    if (!tableView->isColumnLoaded(column))
        WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->contentX(), column < 50 ? 0 : tableView->contentWidth() - tableView->width());
}

void tst_QQuickTableView::positionViewAtCellWithAnimation()
{
    // Check that when we flick to already loaded cell in the
    // table, this will start the animation, and the view will
    // be position correctly after the expected duration.

    LOAD_TABLEVIEW("plaintableview.qml");
    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);
    tableView->setAnimate(true);

    WAIT_UNTIL_POLISHED;

    QPoint cell(tableView->rightColumn(), tableView->bottomRow());
    const QRectF cellGeometry = tableViewPrivate->loadedTableItem(cell)->geometry();
    const int serializedIndex = tableViewPrivate->modelIndexAtCell(cell);
    const QModelIndex index = tableView->index(cell.y(), cell.x());

    QVERIFY(tableViewPrivate->loadedItems.contains(serializedIndex));
    QVERIFY(!tableViewPrivate->positionXAnimation.isRunning());
    QVERIFY(!tableViewPrivate->positionYAnimation.isRunning());

    // Animate the cell to the top left location in the view
    tableView->positionViewAtIndex(index, QQuickTableView::AlignTop | QQuickTableView::AlignLeft);

    // Wait for animation to finish
    QVERIFY(tableViewPrivate->positionXAnimation.isRunning());
    QVERIFY(tableViewPrivate->positionYAnimation.isRunning());
    QTRY_COMPARE(tableViewPrivate->positionXAnimation.isRunning(), false);
    QTRY_COMPARE(tableViewPrivate->positionYAnimation.isRunning(), false);

    // Check that the cell is now placed in the top left corner
    QVERIFY(tableViewPrivate->loadedItems.contains(serializedIndex));
    QPointF expectedPos = tableView->mapToItem(tableView->contentItem(), QPointF(0, 0));
    QCOMPARE(cellGeometry.x(), expectedPos.x());
    QCOMPARE(cellGeometry.y(), expectedPos.y());

    // Animate the cell to the top right location in the view
    tableView->positionViewAtIndex(index, QQuickTableView::AlignTop | QQuickTableView::AlignRight);

    // Wait for animation to finish
    QVERIFY(tableViewPrivate->positionXAnimation.isRunning());
    QVERIFY(!tableViewPrivate->positionYAnimation.isRunning());
    QTRY_COMPARE(tableViewPrivate->positionXAnimation.isRunning(), false);

    // Check that the cell is now placed in the top right corner
    QVERIFY(tableViewPrivate->loadedItems.contains(serializedIndex));
    expectedPos = tableView->mapToItem(tableView->contentItem(), QPointF(tableView->width(), 0));
    QCOMPARE(cellGeometry.right(), expectedPos.x());
    QCOMPARE(cellGeometry.y(), expectedPos.y());

    // Animate the cell to the bottom left location in the view
    tableView->positionViewAtIndex(index, QQuickTableView::AlignBottom | QQuickTableView::AlignLeft);

    // Wait for animation to finish
    QVERIFY(tableViewPrivate->positionXAnimation.isRunning());
    QVERIFY(tableViewPrivate->positionYAnimation.isRunning());
    QTRY_COMPARE(tableViewPrivate->positionXAnimation.isRunning(), false);
    QTRY_COMPARE(tableViewPrivate->positionYAnimation.isRunning(), false);

    // Check that the cell is now placed in the bottom left corner
    QVERIFY(tableViewPrivate->loadedItems.contains(serializedIndex));
    expectedPos = tableView->mapToItem(tableView->contentItem(), QPointF(0, tableView->height()));
    QCOMPARE(cellGeometry.x(), expectedPos.x());
    QCOMPARE(cellGeometry.bottom(), expectedPos.y());

    // Animate the cell to the bottom right location in the view
    tableView->positionViewAtCell(cell, QQuickTableView::AlignBottom | QQuickTableView::AlignRight);

    // Wait for animation to finish
    QVERIFY(tableViewPrivate->positionXAnimation.isRunning());
    QVERIFY(!tableViewPrivate->positionYAnimation.isRunning());
    QTRY_COMPARE(tableViewPrivate->positionXAnimation.isRunning(), false);

    // Check that the cell is now placed in the bottom right corner
    QVERIFY(tableViewPrivate->loadedItems.contains(serializedIndex));
    expectedPos = tableView->mapToItem(tableView->contentItem(), QPointF(tableView->width(), tableView->height()));
    QCOMPARE(cellGeometry.right(), expectedPos.x());
    QCOMPARE(cellGeometry.bottom(), expectedPos.y());
}

void tst_QQuickTableView::positionViewAtCell_VisibleAndContain_data()
{
    QTest::addColumn<QPoint>("cell");
    QTest::addColumn<QQuickTableView::PositionModeFlag>("mode");
    QTest::addColumn<QPointF>("offset");

    QTest::newRow("99, 99, Contain") << QPoint{99, 99} << QQuickTableView::Contain << QPointF{0, 0};
    QTest::newRow("0, 0, Contain") << QPoint{0, 0} << QQuickTableView::Contain << QPointF{0, 0};
    QTest::newRow("5, 0, Contain") << QPoint{5, 0} << QQuickTableView::Contain << QPointF{0, 0};
    QTest::newRow("0, 7, Contain") << QPoint{0, 7} << QQuickTableView::Contain << QPointF{0, 0};
    QTest::newRow("1, 1, Contain") << QPoint{1, 1} << QQuickTableView::Contain << QPointF{0, 0};
    QTest::newRow("10, 10, Contain") << QPoint{10, 10} << QQuickTableView::Contain << QPointF{0, 0};

    QTest::newRow("99, 99, Visible") << QPoint{99, 99} << QQuickTableView::Visible << QPointF{0, 0};
    QTest::newRow("0, 0, Visible") << QPoint{0, 0} << QQuickTableView::Visible << QPointF{0, 0};
    QTest::newRow("5, 1, Visible") << QPoint{5, 1} << QQuickTableView::Visible << QPointF{0, 0};
    QTest::newRow("1, 7, Visible") << QPoint{1, 7} << QQuickTableView::Visible << QPointF{0, 0};
    QTest::newRow("1, 1, Visible") << QPoint{1, 1} << QQuickTableView::Visible << QPointF{0, 0};
    QTest::newRow("10, 10, Visible") << QPoint{10, 10} << QQuickTableView::Visible << QPointF{0, 0};

    QTest::newRow("99, 99, Contain, margins") << QPoint{99, 99} << QQuickTableView::Contain << QPointF{10, 10};
    QTest::newRow("0, 0, Contain, margins") << QPoint{0, 0} << QQuickTableView::Contain << QPointF{10, 10};
    QTest::newRow("5, 0, Contain, margins") << QPoint{5, 1} << QQuickTableView::Contain << QPointF{10, 10};
    QTest::newRow("1, 7, Contain, margins") << QPoint{1, 7} << QQuickTableView::Contain << QPointF{10, 10};
    QTest::newRow("1, 1, Contain, margins") << QPoint{1, 1} << QQuickTableView::Contain << QPointF{10, 10};
    QTest::newRow("10, 10, Contain, margins") << QPoint{10, 10} << QQuickTableView::Contain << QPointF{10, 10};
}

void tst_QQuickTableView::positionViewAtCell_VisibleAndContain()
{
    // Check that the PositionModes "Visible" and "Contain" works according
    // to the documentation.
    QFETCH(QPoint, cell);
    QFETCH(QQuickTableView::PositionModeFlag, mode);
    QFETCH(QPointF, offset);

    LOAD_TABLEVIEW("plaintableview.qml");
    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);
    tableView->setAnimate(true);

    WAIT_UNTIL_POLISHED;

    const bool cellIsVisible = tableView->itemAtCell(cell) != nullptr;
    bool cellIsCompletelyVisible = false;
    if (cellIsVisible) {
        const QRectF cellRect = tableViewPrivate->loadedTableItem(cell)->geometry();
        QRectF viewportRect = tableViewPrivate->viewportRect;
        viewportRect.adjust(offset.x(), offset.y(), -offset.x(), -offset.y());
        cellIsCompletelyVisible = viewportRect.contains(cellRect);
    }

    tableView->positionViewAtCell(cell, mode, offset);

    if (cellIsCompletelyVisible || (cellIsVisible && mode == QQuickTableView::Visible)) {
        // Nothing to do!
        QVERIFY(!tableViewPrivate->positionXAnimation.isRunning());
        QVERIFY(!tableViewPrivate->positionYAnimation.isRunning());
        QVERIFY(!QQuickTest::qIsPolishScheduled(tableView));
    } else if (cellIsVisible) {
        // TableView will scroll towards the cell, unless it'a already at the correct place
        QTRY_COMPARE(tableViewPrivate->positionXAnimation.isRunning(), false);
        QTRY_COMPARE(tableViewPrivate->positionYAnimation.isRunning(), false);
    } else {
        // TableView will rebuild on top of the cell
        QVERIFY(!tableViewPrivate->positionXAnimation.isRunning());
        QVERIFY(!tableViewPrivate->positionYAnimation.isRunning());
        WAIT_UNTIL_POLISHED;
    }

    QVERIFY(tableView->itemAtCell(cell));
}

void tst_QQuickTableView::positionViewAtCell_VisibleAndContain_SubRect_data()
{
    QTest::addColumn<QPoint>("cell");
    QTest::addColumn<QQuickTableView::PositionModeFlag>("mode");
    QTest::addColumn<QRectF>("subRect");
    QTest::addColumn<QPointF>("contentStartPos");

    QRectF subRects[] = { QRectF(0, 0, 10, 10),
                          QRectF(10, 10, 10, 10),
                          QRectF(80, 30, 10, 10),
                          QRectF(90, 40, 10, 10),
                          QRectF(0, 0, 100, 50) };

    for (auto subRect : subRects) {
        QTest::newRow("99, 99, Contain") << QPoint{99, 99} << QQuickTableView::Contain << subRect << QPointF(0, 0);
        QTest::newRow("0, 0, Contain") << QPoint{0, 0} << QQuickTableView::Contain << subRect << QPointF(0, 0);
        QTest::newRow("0, 0, Contain, start: 50, 25") << QPoint{0, 0} << QQuickTableView::Contain << subRect << QPointF(50, 25);
        QTest::newRow("5, 0, Contain") << QPoint{5, 0} << QQuickTableView::Contain << subRect << QPointF(0, 0);
        QTest::newRow("0, 7, Contain") << QPoint{0, 7} << QQuickTableView::Contain << subRect << QPointF(0, 0);
        QTest::newRow("5, 7, Contain, start: -50, -25") << QPoint{5, 7} << QQuickTableView::Contain << subRect << QPointF(-50, -25);

        QTest::newRow("99, 99, Visible") << QPoint{99, 99} << QQuickTableView::Visible << subRect << QPointF(0, 0);
        QTest::newRow("0, 0, Visible") << QPoint{0, 0} << QQuickTableView::Visible << subRect << QPointF(0, 0);
        QTest::newRow("0, 0, Visible, start: 50, 25") << QPoint{0, 0} << QQuickTableView::Visible << subRect << QPointF(50, 25);
        QTest::newRow("5, 1, Visible") << QPoint{5, 1} << QQuickTableView::Visible << subRect << QPointF(0, 0);
        QTest::newRow("1, 7, Visible") << QPoint{1, 7} << QQuickTableView::Visible << subRect << QPointF(0, 0);
        QTest::newRow("5, 7, Visible, start: -50, -25") << QPoint{5, 7} << QQuickTableView::Visible << subRect << QPointF(-50, -25);
    }
}

void tst_QQuickTableView::positionViewAtCell_VisibleAndContain_SubRect()
{
    // Check that the PositionModes "Visible" and "Contain" works when using subrects
    QFETCH(QPoint, cell);
    QFETCH(QQuickTableView::PositionModeFlag, mode);
    QFETCH(QRectF, subRect);
    QFETCH(QPointF, contentStartPos);

    LOAD_TABLEVIEW("plaintableview.qml");
    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);
    tableView->setAnimate(true);
    tableView->setContentX(contentStartPos.x());
    tableView->setContentY(contentStartPos.y());

    WAIT_UNTIL_POLISHED;

    const bool cellIsVisible = tableView->itemAtCell(cell) != nullptr;
    bool subRectIsVisible = false;
    bool subRectIsContained = false;

    if (cellIsVisible) {
        const QRectF cellRect = tableViewPrivate->loadedTableItem(cell)->geometry();
        const QRectF alignmentRect = subRect.translated(cellRect.topLeft());
        const QRectF viewportRect = tableViewPrivate->viewportRect;
        subRectIsVisible = viewportRect.intersects(alignmentRect);
        subRectIsContained = viewportRect.contains(alignmentRect);
    }

    tableView->positionViewAtCell(cell, mode, QPointF(), subRect);

    if (cellIsVisible) {
        if ((mode == QQuickTableView::Visible && subRectIsVisible) ||
                (mode == QQuickTableView::Contain && subRectIsContained)) {
            // The mode is already fulfilled, so verify that no animation (or rebuild) runs
            QVERIFY(!tableViewPrivate->positionXAnimation.isRunning());
            QVERIFY(!tableViewPrivate->positionYAnimation.isRunning());
            QVERIFY(!QQuickTest::qIsPolishScheduled(tableView));
        } else {
            // TableView will scroll towards the cell
            QVERIFY(tableViewPrivate->positionXAnimation.isRunning()
                    || tableViewPrivate->positionYAnimation.isRunning());
            QTRY_VERIFY(!tableViewPrivate->positionXAnimation.isRunning()
                        && !tableViewPrivate->positionYAnimation.isRunning());
        }
    } else {
        // The cell is not loaded, so TableView will rebuild
        WAIT_UNTIL_POLISHED;
    }

    // Check that the subRect is now visible inside the viewport, according to the mode
    QVERIFY(tableView->itemAtCell(cell));
    const QRectF cellRectAfterPositioning = tableViewPrivate->loadedTableItem(cell)->geometry();
    const QRectF alignmentRectAfterPositioning = subRect.translated(cellRectAfterPositioning.topLeft());
    const QRectF viewportRect = tableViewPrivate->viewportRect;

    if (mode == QQuickTableView::Visible)
        QVERIFY(viewportRect.intersects(alignmentRectAfterPositioning));
    else // QQuickTableView::Contain
        QVERIFY(viewportRect.contains(alignmentRectAfterPositioning));
}

void tst_QQuickTableView::positionViewAtCellForLargeCells_data()
{
    QTest::addColumn<qreal>("cellSize");

    QTest::newRow("200") << 200.;
    QTest::newRow("800") << 800.;
}

void tst_QQuickTableView::positionViewAtCellForLargeCells()
{
    // Position the view on a cell outside the viewport. When the cells are larger
    // than the viewport, check that TableView.Contain will place the cell top-left.
    // When the cells are smaller than the viewport, it should be placed bottom-right.
    QFETCH(qreal, cellSize);

    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(10, 10);
    tableView->setModel(model);

    view->rootObject()->setProperty("delegateWidth", cellSize);
    view->rootObject()->setProperty("delegateHeight", cellSize);

    WAIT_UNTIL_POLISHED;

    const QPoint cell(5, 5);
    tableView->positionViewAtCell(cell, QQuickTableView::Contain);

    WAIT_UNTIL_POLISHED;

    const QQuickItem *item = tableView->itemAtCell(cell);
    QVERIFY(item);

    QPointF expectedPos;
    if (cellSize > tableView->width()) {
        expectedPos = tableView->mapToItem(tableView->contentItem(), QPointF(0, 0));
    } else {
        expectedPos = tableView->mapToItem(tableView->contentItem(), QPointF(tableView->width(), tableView->height()));
        expectedPos -= QPointF(item->width(), item->height());
    }

    QCOMPARE(item->x(), expectedPos.x());
    QCOMPARE(item->y(), expectedPos.y());
}

void tst_QQuickTableView::positionViewAtCellForLargeCellsUsingSubrect()
{
    // Position the view on a cell outside the viewport. When a cell is larger
    // than the viewport, TableView.Contain will normally place it top-left.
    // But if we specify a subRect that is close to the bottom-right edge (and
    // the subRect is smaller than the viewport), TableView should align
    // bottom-right of the subRect instead.
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(10, 10);
    tableView->setModel(model);
    tableView->setAnimate(false);

    const qreal cellSize = 800;
    view->rootObject()->setProperty("delegateWidth", cellSize);
    view->rootObject()->setProperty("delegateHeight", cellSize);

    WAIT_UNTIL_POLISHED;

    const QRectF subRect(cellSize - 100, cellSize - 100, 10, 10);
    tableView->positionViewAtCell(QPoint(0, 0), QQuickTableView::Contain, QPointF(), subRect);
    QCOMPARE(tableView->contentX(), -(tableView->width() - subRect.right()));
    QCOMPARE(tableView->contentY(), -(tableView->height() - subRect.bottom()));
}

void tst_QQuickTableView::positionViewAtLastRow_data()
{
    QTest::addColumn<QString>("signalToTest");
    QTest::addColumn<bool>("animate");

    QTest::newRow("positionOnRowsChanged, animate=false") << "positionOnRowsChanged" << false;
    QTest::newRow("positionOnRowsChanged, animate=true") << "positionOnRowsChanged" << true;
    QTest::newRow("positionOnContentHeightChanged, animate=false") << "positionOnContentHeightChanged" << false;
    QTest::newRow("positionOnContentHeightChanged, animate=true") << "positionOnContentHeightChanged" << true;
}

void tst_QQuickTableView::positionViewAtLastRow()
{
    // Check that we can make TableView always scroll to the
    // last row in the model by positioning the view upon
    // a rowsChanged callback
    QFETCH(QString, signalToTest);
    QFETCH(bool, animate);

    LOAD_TABLEVIEW("positionlast.qml");

    // Use a very large model to indirectly test that we "fast-flick" to
    // the end at start-up (instead of loading and unloading rows, which
    // would take forever).
    TestModel model(2000000, 2000000);
    tableView->setModel(QVariant::fromValue(&model));
    tableView->setAnimate(animate);

    view->rootObject()->setProperty(signalToTest.toUtf8().constData(), true);

    WAIT_UNTIL_POLISHED;

    const qreal delegateSize = 100.;
    const qreal viewportRowCount = tableView->height() / delegateSize;

    // Check that the viewport is positioned at the last row at start-up
    QCOMPARE(tableView->rows(), model.rowCount());
    QCOMPARE(tableView->bottomRow(), model.rowCount() - 1);
    QCOMPARE(tableView->contentY(), (model.rowCount() - viewportRowCount) * delegateSize);

    // Check that the viewport is positioned at the last
    // row after more rows are added.
    for (int row = 0; row < 2; ++row) {
        model.addRow(model.rowCount() - 1);

        WAIT_UNTIL_POLISHED;

        if (tableView->animate()) {
            QVERIFY(tableViewPrivate->positionYAnimation.isRunning());
            QTRY_VERIFY(!tableViewPrivate->positionYAnimation.isRunning());
        }

        QCOMPARE(tableView->rows(), model.rowCount());
        QCOMPARE(tableView->bottomRow(), model.rowCount() - 1);
        QCOMPARE(tableView->contentY(), (model.rowCount() - viewportRowCount) * delegateSize);
    }
}

void tst_QQuickTableView::positionViewAtLastColumn_data()
{
    QTest::addColumn<QString>("signalToTest");
    QTest::addColumn<bool>("animate");

    QTest::newRow("positionOnColumnsChanged, animate=false") << "positionOnColumnsChanged" << false;
    QTest::newRow("positionOnColumnsChanged, animate=true") << "positionOnColumnsChanged" << true;
    QTest::newRow("positionOnContentWidthChanged, animate=false") << "positionOnContentWidthChanged" << false;
    QTest::newRow("positionOnContentWidthChanged, animate=true") << "positionOnContentWidthChanged" << true;
}

void tst_QQuickTableView::positionViewAtLastColumn()
{
    // Check that we can make TableView always scroll to the
    // last column in the model by positioning the view upon
    // a columnsChanged callback
    QFETCH(QString, signalToTest);
    QFETCH(bool, animate);

    LOAD_TABLEVIEW("positionlast.qml");

    // Use a very large model to indirectly test that we "fast-flick" to
    // the end at start-up (instead of loading and unloading columns, which
    // would take forever).
    TestModel model(2000000, 2000000);
    tableView->setModel(QVariant::fromValue(&model));
    tableView->setAnimate(animate);

    view->rootObject()->setProperty(signalToTest.toUtf8().constData(), true);

    WAIT_UNTIL_POLISHED;

    const qreal delegateSize = 100.;
    const qreal viewportColumnCount = tableView->width() / delegateSize;

    // Check that the viewport is positioned at the last column at start-up
    QCOMPARE(tableView->columns(), model.columnCount());
    QCOMPARE(tableView->rightColumn(), model.columnCount() - 1);
    QCOMPARE(tableView->contentX(), (model.columnCount() - viewportColumnCount) * delegateSize);

    // Check that the viewport is positioned at the last
    // column after more columns are added.
    for (int column = 0; column < 2; ++column) {
        model.addColumn(model.columnCount() - 1);

        WAIT_UNTIL_POLISHED;

        if (tableView->animate()) {
            QVERIFY(tableViewPrivate->positionXAnimation.isRunning());
            QTRY_VERIFY(!tableViewPrivate->positionXAnimation.isRunning());
        }

        QCOMPARE(tableView->columns(), model.columnCount());
        QCOMPARE(tableView->rightColumn(), model.columnCount() - 1);
        QCOMPARE(tableView->contentX(), (model.columnCount() - viewportColumnCount) * delegateSize);
    }
}

void tst_QQuickTableView::itemAtCell_data()
{
    QTest::addColumn<QPoint>("cell");
    QTest::addColumn<bool>("shouldExist");

    QTest::newRow("0, 0") << QPoint(0, 0) << true;
    QTest::newRow("0, 4") << QPoint(0, 4) << true;
    QTest::newRow("4, 0") << QPoint(4, 0) << true;
    QTest::newRow("4, 4") << QPoint(4, 4) << true;
    QTest::newRow("30, 30") << QPoint(30, 30) << false;
    QTest::newRow("-1, -1") << QPoint(-1, -1) << false;
}

void tst_QQuickTableView::itemAtCell()
{
    QFETCH(QPoint, cell);
    QFETCH(bool, shouldExist);

    LOAD_TABLEVIEW("plaintableview.qml");
    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    const auto item = tableView->itemAtCell(cell);
    if (shouldExist) {
        const auto context = qmlContext(item);
        const int contextRow = context->contextProperty("row").toInt();
        const int contextColumn = context->contextProperty("column").toInt();
        QCOMPARE(contextColumn, cell.x());
        QCOMPARE(contextRow, cell.y());
    } else {
        QVERIFY(!item);
    }
}

void tst_QQuickTableView::leftRightTopBottomProperties_data()
{
    QTest::addColumn<QPointF>("contentStartPos");
    QTest::addColumn<QMargins>("expectedTable");
    QTest::addColumn<QMargins>("expectedSignalCount");

    QTest::newRow("1") << QPointF(0, 0) << QMargins(0, 0, 5, 7) << QMargins(0, 0, 1, 1);
    QTest::newRow("2") << QPointF(100, 50) << QMargins(1, 1, 6, 8) << QMargins(1, 1, 2, 2);
    QTest::newRow("3") << QPointF(220, 120) << QMargins(2, 2, 8, 10) << QMargins(2, 2, 4, 4);
    QTest::newRow("4") << QPointF(1000, 1000) << QMargins(9, 19, 15, 27) << QMargins(1, 1, 2, 2);
}

void tst_QQuickTableView::leftRightTopBottomProperties()
{
    QFETCH(QPointF, contentStartPos);
    QFETCH(QMargins, expectedTable);
    QFETCH(QMargins, expectedSignalCount);

    LOAD_TABLEVIEW("plaintableview.qml");
    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    QSignalSpy leftSpy(tableView, &QQuickTableView::leftColumnChanged);
    QSignalSpy rightSpy(tableView, &QQuickTableView::rightColumnChanged);
    QSignalSpy topSpy(tableView, &QQuickTableView::topRowChanged);
    QSignalSpy bottomSpy(tableView, &QQuickTableView::bottomRowChanged);

    WAIT_UNTIL_POLISHED;

    tableView->setContentX(contentStartPos.x());
    tableView->setContentY(contentStartPos.y());

    tableView->polish();
    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->leftColumn(), expectedTable.left());
    QCOMPARE(tableView->topRow(), expectedTable.top());
    QCOMPARE(tableView->rightColumn(), expectedTable.right());
    QCOMPARE(tableView->bottomRow(), expectedTable.bottom());

    QCOMPARE(leftSpy.size(), expectedSignalCount.left());
    QCOMPARE(rightSpy.size(), expectedSignalCount.right());
    QCOMPARE(topSpy.size(), expectedSignalCount.top());
    QCOMPARE(bottomSpy.size(), expectedSignalCount.bottom());
}

void tst_QQuickTableView::leftRightTopBottomUpdatedBeforeSignalEmission()
{
    // Check that leftColumn, rightColumn, topRow and bottomRow are
    // actually updated before the changed signals are emitted.
    LOAD_TABLEVIEW("plaintableview.qml");
    auto model = TestModelAsVariant(100, 100);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    connect(tableView, &QQuickTableView::leftColumnChanged, [=]{
        QCOMPARE(tableView->leftColumn(), 1);
    });
    connect(tableView, &QQuickTableView::rightColumnChanged, [=]{
        QCOMPARE(tableView->rightColumn(), 6);
    });
    connect(tableView, &QQuickTableView::topRowChanged, [=]{
        QCOMPARE(tableView->topRow(), 1);
    });
    connect(tableView, &QQuickTableView::bottomRowChanged, [=]{
        QCOMPARE(tableView->bottomRow(), 8);
    });

    tableView->setContentX(100);
    tableView->setContentY(50);
}

void tst_QQuickTableView::checkContentSize_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("colCount");

    QTest::newRow("4x4") << 4 << 4;
    QTest::newRow("100x100") << 100 << 100;
    QTest::newRow("0x0") << 0 << 0;
}

void tst_QQuickTableView::checkContentSize()
{
    QFETCH(int, rowCount);
    QFETCH(int, colCount);

    // Check that the content size is initially correct, and that
    // it updates when we change e.g the model or spacing (QTBUG-87680)
    LOAD_TABLEVIEW("plaintableview.qml");

    TestModel model(rowCount, colCount);
    tableView->setModel(QVariant::fromValue(&model));
    tableView->setRowSpacing(1);
    tableView->setColumnSpacing(2);

    WAIT_UNTIL_POLISHED;

    const qreal delegateWidth = 100;
    const qreal delegateHeight = 50;
    qreal colSpacing = tableView->columnSpacing();
    qreal rowSpacing = tableView->rowSpacing();

    // Check that content size has the exepected initial values
    QCOMPARE(tableView->contentWidth(), colCount == 0 ? 0 : (colCount * (delegateWidth + colSpacing)) - colSpacing);
    QCOMPARE(tableView->contentHeight(), rowCount == 0 ? 0 : (rowCount * (delegateHeight + rowSpacing)) - rowSpacing);

    // Set no spacing
    rowSpacing = 0;
    colSpacing = 0;
    tableView->setRowSpacing(rowSpacing);
    tableView->setColumnSpacing(colSpacing);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->contentWidth(), colCount * delegateWidth);
    QCOMPARE(tableView->contentHeight(), rowCount * delegateHeight);

    // Set typical spacing values
    rowSpacing = 2;
    colSpacing = 3;
    tableView->setRowSpacing(rowSpacing);
    tableView->setColumnSpacing(colSpacing);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->contentWidth(), colCount == 0 ? 0 : (colCount * (delegateWidth + colSpacing)) - colSpacing);
    QCOMPARE(tableView->contentHeight(), rowCount == 0 ? 0 : (rowCount * (delegateHeight + rowSpacing)) - rowSpacing);

    // Add a row and a column
    model.insertRow(0);
    model.insertColumn(0);
    rowCount = model.rowCount();
    colCount = model.columnCount();
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->contentWidth(), (colCount * (delegateWidth + colSpacing)) - colSpacing);
    QCOMPARE(tableView->contentHeight(), (rowCount * (delegateHeight + rowSpacing)) - rowSpacing);

    // Remove a row (this should also make affect contentWidth if rowCount becomes 0)
    model.removeRow(0);
    rowCount = model.rowCount();
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->contentWidth(), rowCount == 0 ? 0 : (colCount * (delegateWidth + colSpacing)) - colSpacing);
    QCOMPARE(tableView->contentHeight(), rowCount == 0 ? 0 : (rowCount * (delegateHeight + rowSpacing)) - rowSpacing);
}

void tst_QQuickTableView::checkSelectionModelWithRequiredSelectedProperty_data()
{
    QTest::addColumn<QVector<QPoint>>("selected");
    QTest::addColumn<QPoint>("toggle");

    QTest::newRow("nothing selected") << QVector<QPoint>() << QPoint(0,0);
    QTest::newRow("one item selected") << (QVector<QPoint>() << QPoint(0, 0)) << QPoint(1, 1);
    QTest::newRow("two items selected") << (QVector<QPoint>() << QPoint(1, 1) << QPoint(2, 2)) << QPoint(1, 1);
}

void tst_QQuickTableView::checkSelectionModelWithRequiredSelectedProperty()
{
    // Check that if you add a "required property selected" to the delegate,
    // TableView will give it a value upon creation that matches the state
    // in the selection model.
    QFETCH(QVector<QPoint>, selected);
    QFETCH(QPoint, toggle);

    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(10, 10);
    QItemSelectionModel selectionModel(&model);

    // Set initially selected cells
    for (auto it = selected.constBegin(); it != selected.constEnd(); ++it) {
        const QPoint &cell = *it;
        selectionModel.select(model.index(cell.y(), cell.x()), QItemSelectionModel::Select);
    }

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);

    WAIT_UNTIL_POLISHED;

    // Check that all delegates have "selected" set with the initial value
    for (auto fxItem : tableViewPrivate->loadedItems) {
        const auto context = qmlContext(fxItem->item.data());
        const int row = context->contextProperty("row").toInt();
        const int column = context->contextProperty("column").toInt();
        const bool selected = fxItem->item->property("selected").toBool();
        const auto modelIndex = model.index(row, column);
        QCOMPARE(selected, selectionModel.isSelected(modelIndex));
    }

    // Toggle selected on one of the model indices, and check
    // that the "selected" property got updated as well
    const QModelIndex toggleIndex = model.index(toggle.y(), toggle.x());
    const bool wasSelected = selectionModel.isSelected(toggleIndex);
    selectionModel.select(toggleIndex, QItemSelectionModel::Toggle);
    const auto fxItem = tableViewPrivate->loadedTableItem(toggle);
    const bool isSelected = fxItem->item->property("selected").toBool();
    QCOMPARE(isSelected, !wasSelected);
}

void tst_QQuickTableView::checkSelectionModelWithUnrequiredSelectedProperty()
{
    // Check that if there is a property "selected" in the delegate, but it's
    // not required, then TableView will not touch it. This is for legacy reasons, to
    // not break applications written before Qt 6.2 that has such a property
    // added for application logic.
    LOAD_TABLEVIEW("tableviewwithselected2.qml");

    TestModel model(10, 10);
    tableView->setModel(QVariant::fromValue(&model));
    QItemSelectionModel *selectionModel = tableView->selectionModel();
    QVERIFY(selectionModel);

    // Select a cell
    selectionModel->select(model.index(1, 1), QItemSelectionModel::Select);

    WAIT_UNTIL_POLISHED;

    const auto fxItem = tableViewPrivate->loadedTableItem(QPoint(1, 1));
    const bool selected = fxItem->item->property("selected").toBool();
    QCOMPARE(selected, false);
}

void tst_QQuickTableView::removeAndAddSelectionModel()
{
    // Check that if we remove the selection model from TableView, all delegates
    // will be unselected. And opposite, if we add the selection model back, the
    // delegates will be updated.
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(10, 10);
    QItemSelectionModel selectionModel(&model);

    // Select a cell in the selection model
    selectionModel.select(model.index(1, 1), QItemSelectionModel::Select);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);

    WAIT_UNTIL_POLISHED;

    // Check that the delegate item is selected
    const auto fxItem = tableViewPrivate->loadedTableItem(QPoint(1, 1));
    bool selected = fxItem->item->property("selected").toBool();
    QCOMPARE(selected, true);

    // Remove the selection model, and check that the delegate item is now unselected
    tableView->setSelectionModel(nullptr);
    selected = fxItem->item->property("selected").toBool();
    QCOMPARE(selected, false);

    // Add the selection model back, and check that the delegate item is selected again
    tableView->setSelectionModel(&selectionModel);
    selected = fxItem->item->property("selected").toBool();
    QCOMPARE(selected, true);
}

void tst_QQuickTableView::warnOnWrongModelInSelectionModel()
{
    // The model set on the SelectionModel should always match the model
    // set on TableView. This is normally handled automatically, but it's
    // possible to circumvent. This test will check that we warn if that happens.
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model1(10, 10);
    TestModel model2(10, 10);

    tableView->setModel(QVariant::fromValue(&model1));
    QItemSelectionModel selectionModel;
    tableView->setSelectionModel(&selectionModel);

    // Set a different model
    selectionModel.setModel(&model2);

    // And change currentIndex. This will produce a warning.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*TableView.selectionModel.model.*"));
    selectionModel.setCurrentIndex(model2.index(0, 0), QItemSelectionModel::NoUpdate);
}

void tst_QQuickTableView::selectionBehaviorCells_data()
{
    QTest::addColumn<QPoint>("endCellDist");

    QTest::newRow("single cell") << QPoint(0, 0);

    QTest::newRow("left to right") << QPoint(1, 0);
    QTest::newRow("left to right") << QPoint(2, 0);
    QTest::newRow("right to left") << QPoint(-1, 0);
    QTest::newRow("right to left") << QPoint(-2, 0);

    QTest::newRow("top to bottom") << QPoint(0, 1);
    QTest::newRow("top to bottom") << QPoint(0, 2);
    QTest::newRow("bottom to top") << QPoint(0, -1);
    QTest::newRow("bottom to top") << QPoint(0, -2);

    QTest::newRow("diagonal top left to bottom right") << QPoint(1, 1);
    QTest::newRow("diagonal top left to bottom right") << QPoint(2, 2);
    QTest::newRow("diagonal bottom left to top right") << QPoint(-1, -1);
    QTest::newRow("diagonal bottom left to top right") << QPoint(-2, -2);
    QTest::newRow("diagonal top right to bottom left") << QPoint(-1, 1);
    QTest::newRow("diagonal top right to bottom left") << QPoint(-2, 2);
    QTest::newRow("diagonal bottom right to top left") << QPoint(1, -1);
    QTest::newRow("diagonal bottom right to top left") << QPoint(2, -2);
}

void tst_QQuickTableView::selectionBehaviorCells()
{
    // Check that the TableView implement QQuickSelectableInterface setSelectionStartPos, setSelectionEndPos
    // and clearSelection correctly. Do this by calling setSelectionStartPos/setSelectionEndPos on top of
    // different cells, and see that we end up with the expected selections.
    QFETCH(QPoint, endCellDist);
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(10, 10);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(selectionModel.hasSelection(), false);
    QCOMPARE(tableView->selectionBehavior(), QQuickTableView::SelectCells);

    const QPoint startCell(5, 5);
    const QPoint endCell = startCell + endCellDist;
    const QPoint endCellWrapped = startCell - endCellDist;

    const QQuickItem *startItem = tableView->itemAtCell(startCell);
    const QQuickItem *endItem = tableView->itemAtCell(endCell);
    const QQuickItem *endItemWrapped = tableView->itemAtCell(endCellWrapped);
    QVERIFY(startItem);
    QVERIFY(endItem);
    QVERIFY(endItemWrapped);

    const QPointF startPos(startItem->x(), startItem->y());
    const QPointF endPos(endItem->x(), endItem->y());
    const QPointF endPosWrapped(endItemWrapped->x(), endItemWrapped->y());

    tableViewPrivate->setSelectionStartPos(startPos);
    tableViewPrivate->setSelectionEndPos(endPos);

    QCOMPARE(selectionModel.hasSelection(), true);

    const int x1 = qMin(startCell.x(), endCell.x());
    const int x2 = qMax(startCell.x(), endCell.x());
    const int y1 = qMin(startCell.y(), endCell.y());
    const int y2 = qMax(startCell.y(), endCell.y());

    for (int x = x1; x < x2; ++x) {
        for (int y = y1; y < y2; ++y) {
            const auto index = model.index(y, x);
            QVERIFY(selectionModel.isSelected(index));
        }
    }

    const int expectedCount = (x2 - x1 + 1) * (y2 - y1 + 1);
    const int actualCount = selectionModel.selectedIndexes().size();
    QCOMPARE(actualCount, expectedCount);

    // Wrap the selection
    tableViewPrivate->setSelectionEndPos(endPosWrapped);

    for (int x = x2; x < x1; ++x) {
        for (int y = y2; y < y1; ++y) {
            const auto index = model.index(y, x);
            QVERIFY(selectionModel.isSelected(index));
        }
    }

    const int actualCountAfterWrap = selectionModel.selectedIndexes().size();
    QCOMPARE(actualCountAfterWrap, expectedCount);

    tableViewPrivate->clearSelection();
    QCOMPARE(selectionModel.hasSelection(), false);
}

void tst_QQuickTableView::selectionBehaviorRows()
{
    // Check that the TableView implement QQuickSelectableInterface setSelectionStartPos, setSelectionEndPos
    // and clearSelection correctly for QQuickTableView::SelectRows.
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(10, 10);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setSelectionBehavior(QQuickTableView::SelectRows);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(selectionModel.hasSelection(), false);

    // Drag from row 0 to row 3
    tableViewPrivate->setSelectionStartPos(QPointF(0, 0));
    tableViewPrivate->setSelectionEndPos(QPointF(60, 60));

    QCOMPARE(selectionModel.hasSelection(), true);

    const int expectedCount = 10 * 3; // all columns * three rows
    int actualCount = selectionModel.selectedIndexes().size();
    QCOMPARE(actualCount, expectedCount);

    for (int x = 0; x < tableView->columns(); ++x) {
        for (int y = 0; y < 3; ++y) {
            const auto index = model.index(y, x);
            QVERIFY(selectionModel.isSelected(index));
        }
    }

    selectionModel.clear();
    QCOMPARE(selectionModel.hasSelection(), false);

    // Drag from row 3 to row 0 (and overshoot mouse)
    tableViewPrivate->setSelectionStartPos(QPointF(60, 60));
    tableViewPrivate->setSelectionEndPos(QPointF(-10, -10));

    QCOMPARE(selectionModel.hasSelection(), true);

    actualCount = selectionModel.selectedIndexes().size();
    QCOMPARE(actualCount, expectedCount);

    for (int x = 0; x < tableView->columns(); ++x) {
        for (int y = 0; y < 3; ++y) {
            const auto index = model.index(y, x);
            QVERIFY(selectionModel.isSelected(index));
        }
    }
}

void tst_QQuickTableView::selectionBehaviorColumns()
{
    // Check that the TableView implement QQuickSelectableInterface setSelectionStartPos, setSelectionEndPos
    // and clearSelection correctly for QQuickTableView::SelectColumns.
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(10, 10);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setSelectionBehavior(QQuickTableView::SelectColumns);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(selectionModel.hasSelection(), false);

    // Drag from column 0 to column 3
    tableViewPrivate->setSelectionStartPos(QPointF(0, 0));
    tableViewPrivate->setSelectionEndPos(QPointF(60, 60));

    QCOMPARE(selectionModel.hasSelection(), true);

    const int expectedCount = 10 * 3; // all rows * three columns
    int actualCount = selectionModel.selectedIndexes().size();
    QCOMPARE(actualCount, expectedCount);

    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < tableView->rows(); ++y) {
            const auto index = model.index(y, x);
            QVERIFY(selectionModel.isSelected(index));
        }
    }

    selectionModel.clear();
    QCOMPARE(selectionModel.hasSelection(), false);

    // Drag from column 3 to column 0 (and overshoot mouse)
    tableViewPrivate->setSelectionStartPos(QPointF(60, 60));
    tableViewPrivate->setSelectionEndPos(QPointF(-10, -10));

    QCOMPARE(selectionModel.hasSelection(), true);

    actualCount = selectionModel.selectedIndexes().size();
    QCOMPARE(actualCount, expectedCount);

    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < tableView->rows(); ++y) {
            const auto index = model.index(y, x);
            QVERIFY(selectionModel.isSelected(index));
        }
    }
}

void tst_QQuickTableView::selectionBehaviorDisabled()
{
    // Check that the TableView implement QQuickSelectableInterface setSelectionStartPos, setSelectionEndPos
    // and clearSelection correctly for QQuickTableView::SelectionDisabled.
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(10, 10);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setSelectionBehavior(QQuickTableView::SelectionDisabled);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(selectionModel.hasSelection(), false);

    // Drag from column 0 to column 3
    tableViewPrivate->setSelectionStartPos(QPointF(0, 0));
    tableViewPrivate->setSelectionEndPos(QPointF(60, 60));

    QCOMPARE(selectionModel.hasSelection(), false);
}

void tst_QQuickTableView::testSelectableStartPosEndPosOutsideView()
{
    // Call setSelectionStartPos and setSelectionEndPos with positions outside the view.
    // This should first of all not crash, but instead just clamp the selection to the
    // cells that are visible inside the view.
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(10, 10);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);

    WAIT_UNTIL_POLISHED;

    const QPoint centerCell(5, 5);
    const QQuickItem *centerItem = tableView->itemAtCell(centerCell);
    QVERIFY(centerItem);

    const QPointF centerPos(centerItem->x(), centerItem->y());
    const QPointF outsideLeft(-100, centerPos.y());
    const QPointF outsideRight(tableView->width() + 100, centerPos.y());
    const QPointF outsideTop(centerPos.x(), -100);
    const QPointF outsideBottom(centerPos.x(), tableView->height() + 100);

    tableViewPrivate->setSelectionStartPos(centerPos);

    tableViewPrivate->setSelectionEndPos(outsideLeft);
    for (int x = 0; x <= centerCell.x(); ++x) {
        const auto index = model.index(centerCell.y(), x);
        QVERIFY(selectionModel.isSelected(index));
    }

    tableViewPrivate->setSelectionEndPos(outsideRight);
    for (int x = centerCell.x(); x < model.columnCount(); ++x) {
        const auto index = model.index(centerCell.y(), x);
        QVERIFY(selectionModel.isSelected(index));
    }

    tableViewPrivate->setSelectionEndPos(outsideTop);
    for (int y = 0; y <= centerCell.y(); ++y) {
        const auto index = model.index(y, centerCell.x());
        QVERIFY(selectionModel.isSelected(index));
    }

    tableViewPrivate->setSelectionEndPos(outsideBottom);
    for (int y = centerCell.y(); y < model.rowCount(); ++y) {
        const auto index = model.index(y, centerCell.x());
        QVERIFY(selectionModel.isSelected(index));
    }
}

void tst_QQuickTableView::testSelectableScrollTowardsPos()
{
    // Check that TableView will implement the scrollTowardsSelectionPoint function
    // correctly, and move the content item towards the given position
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(200, 200);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->contentX(), 0);
    QCOMPARE(tableView->contentY(), 0);

    const QSizeF step(1, 1);
    const QPointF topLeft(-100, -100);
    const QPointF topRight(tableView->width() + 100, -100);
    const QPointF bottomLeft(-100, tableView->height() + 100);
    const QPointF bottomRight(tableView->width() + 100, tableView->height() + 100);

    tableViewPrivate->scrollTowardsSelectionPoint(topRight, step);
    QCOMPARE(tableView->contentX(), step.width());
    QCOMPARE(tableView->contentY(), 0);

    tableViewPrivate->scrollTowardsSelectionPoint(bottomRight, step);
    QCOMPARE(tableView->contentX(), step.width() * 2);
    QCOMPARE(tableView->contentY(), step.height());

    tableViewPrivate->scrollTowardsSelectionPoint(bottomLeft, step);
    QCOMPARE(tableView->contentX(), step.width());
    QCOMPARE(tableView->contentY(), step.height() * 2);

    tableViewPrivate->scrollTowardsSelectionPoint(topLeft, step);
    QCOMPARE(tableView->contentX(), 0);
    QCOMPARE(tableView->contentY(), step.height());

    tableViewPrivate->scrollTowardsSelectionPoint(topLeft, step);
    QCOMPARE(tableView->contentX(), 0);
    QCOMPARE(tableView->contentY(), 0);
}

void tst_QQuickTableView::setCurrentIndexFromSelectionModel()
{
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(40, 40);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setFocus(true);
    const char kCurrent[] = "current";

    WAIT_UNTIL_POLISHED;

    // Check that all delegates have current set to false upon start
    for (auto fxItem : tableViewPrivate->loadedItems)
        QVERIFY(!fxItem->item->property(kCurrent).toBool());

    // Start by making cell 0, 0 current
    const QPoint cell0_0(0, 0);
    selectionModel.setCurrentIndex(tableView->modelIndex(cell0_0), QItemSelectionModel::NoUpdate);
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());

    // Move currentIndex to a cell outside the viewport by accessing the selection
    // model directly, scroll to it, and check current status.
    const QPoint cellAtEnd(tableView->columns() - 1, tableView->rows() - 1);
    selectionModel.setCurrentIndex(tableView->modelIndex(cellAtEnd), QItemSelectionModel::NoUpdate);
    QVERIFY(!tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());

    tableView->positionViewAtCell(cellAtEnd, QQuickTableView::AlignBottom | QQuickTableView::AlignRight);
    WAIT_UNTIL_POLISHED;
    QVERIFY(tableView->itemAtCell(cellAtEnd));
    QVERIFY(tableView->itemAtCell(cellAtEnd)->property(kCurrent).toBool());
}

void tst_QQuickTableView::clearSelectionOnTap_data()
{
    QTest::addColumn<bool>("selectionEnabled");
    QTest::newRow("selections enabled") << true;
    QTest::newRow("selections disabled") << false;
}

void tst_QQuickTableView::clearSelectionOnTap()
{
    // Check that we clear the current selection when tapping
    // inside TableView. But only if TableView has selections
    // enabled. Otherwise, TableView should not touch the selection model.
    QFETCH(bool, selectionEnabled);
    LOAD_TABLEVIEW("tableviewwithselected2.qml");

    TestModel model(40, 40);
    tableView->setModel(QVariant::fromValue(&model));
    if (!selectionEnabled)
        tableView->setSelectionBehavior(QQuickTableView::SelectionDisabled);

    WAIT_UNTIL_POLISHED;

    // Select root item
    const auto index = tableView->selectionModel()->model()->index(0, 0);
    tableView->selectionModel()->select(index, QItemSelectionModel::Select);
    QCOMPARE(tableView->selectionModel()->selectedIndexes().size(), 1);

    // Click on a cell
    const auto item = tableView->itemAtIndex(tableView->index(0, 0));
    QVERIFY(item);
    QPoint localPos = QPoint(item->width() / 2, item->height() / 2);
    QPoint pos = item->window()->contentItem()->mapFromItem(item, localPos).toPoint();
    QTest::mouseClick(item->window(), Qt::LeftButton, Qt::NoModifier, pos);

    QCOMPARE(tableView->selectionModel()->hasSelection(), !selectionEnabled);
}

void tst_QQuickTableView::moveCurrentIndexUsingArrowKeys()
{
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(40, 40);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setFocus(true);
    QQuickWindow *window = tableView->window();
    const char kCurrent[] = "current";

    WAIT_UNTIL_POLISHED;

    // Check that all delegates have current set to false upon start
    for (auto fxItem : tableViewPrivate->loadedItems)
        QVERIFY(!fxItem->item->property(kCurrent).toBool());

    QCOMPARE(tableView->currentColumn(), -1);
    QCOMPARE(tableView->currentRow(), -1);

    // Start by making cell 0, 0 current
    const QPoint cell0_0(0, 0);
    selectionModel.setCurrentIndex(tableView->modelIndex(cell0_0), QItemSelectionModel::NoUpdate);
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());

    // Trying to move the index out of the table with the keys should be a no-op:
    QTest::keyPress(window, Qt::Key_Left);
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QTest::keyPress(window, Qt::Key_Up);
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Move currentIndex right
    const QPoint cell1_0(1, 0);
    QTest::keyPress(window, Qt::Key_Right);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell1_0));
    QVERIFY(!tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(tableView->itemAtCell(cell1_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell1_0.x());
    QCOMPARE(tableView->currentRow(), cell1_0.y());

    // Move currentIndex left
    QTest::keyPress(window, Qt::Key_Left);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(!tableView->itemAtCell(cell1_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Move currentIndex down
    const QPoint cell0_1(0, 1);
    QTest::keyPress(window, Qt::Key_Down);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_1));
    QVERIFY(!tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(tableView->itemAtCell(cell0_1)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_1.x());
    QCOMPARE(tableView->currentRow(), cell0_1.y());

    // Move currentIndex up
    QTest::keyPress(window, Qt::Key_Up);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(!tableView->itemAtCell(cell0_1)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());
}

void tst_QQuickTableView::moveCurrentIndexUsingHomeAndEndKeys()
{
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(40, 40);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setFocus(true);
    QQuickWindow *window = tableView->window();
    const char kCurrent[] = "current";

    WAIT_UNTIL_POLISHED;

    // Check that all delegates have current set to false upon start
    for (auto fxItem : tableViewPrivate->loadedItems)
        QVERIFY(!fxItem->item->property(kCurrent).toBool());

    QCOMPARE(tableView->currentColumn(), -1);
    QCOMPARE(tableView->currentRow(), -1);

    const QPoint cell0_0(0, 0);
    const QPoint cellHorEnd(tableView->columns() - 1, 0);

    // Start by making cell 0, 0 current
    selectionModel.setCurrentIndex(tableView->modelIndex(cell0_0), QItemSelectionModel::NoUpdate);
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Move currentIndex to end
    QTest::keyPress(window, Qt::Key_End);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cellHorEnd));
    QTRY_VERIFY(tableView->itemAtCell(cellHorEnd));
    QVERIFY(tableView->itemAtCell(cellHorEnd)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cellHorEnd.x());
    QCOMPARE(tableView->currentRow(), cellHorEnd.y());

    // Move currentIndex to end once more is a no-op
    QTest::keyPress(window, Qt::Key_End);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cellHorEnd));
    QVERIFY(tableView->itemAtCell(cellHorEnd)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cellHorEnd.x());
    QCOMPARE(tableView->currentRow(), cellHorEnd.y());

    // Move currentIndex to home
    QTest::keyPress(window, Qt::Key_Home);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QTRY_VERIFY(tableView->itemAtCell(cell0_0));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Move currentIndex to home once more is a no-op
    QTest::keyPress(window, Qt::Key_Home);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());
}

void tst_QQuickTableView::moveCurrentIndexUsingPageUpDownKeys()
{
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(40, 40);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setFocus(true);
    QQuickWindow *window = tableView->window();
    const char kCurrent[] = "current";

    WAIT_UNTIL_POLISHED;

    // Check that all delegates have current set to false upon start
    for (auto fxItem : tableViewPrivate->loadedItems)
        QVERIFY(!fxItem->item->property(kCurrent).toBool());

    QCOMPARE(tableView->currentColumn(), -1);
    QCOMPARE(tableView->currentRow(), -1);

    // Start by making cell 0, 0 current
    const QPoint cell0_0(0, 0);
    selectionModel.setCurrentIndex(tableView->modelIndex(cell0_0), QItemSelectionModel::NoUpdate);
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Move currentIndex page down
    const QPoint bottomCell(0, tableView->bottomRow());
    QTest::keyPress(window, Qt::Key_PageDown);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(bottomCell));
    QVERIFY(tableView->itemAtCell(cell0_0));
    QVERIFY(tableView->itemAtCell(bottomCell));
    QVERIFY(!tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(tableView->itemAtCell(bottomCell)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), bottomCell.x());
    QCOMPARE(tableView->currentRow(), bottomCell.y());

    // Move currentIndex page up
    QTest::keyPress(window, Qt::Key_PageUp);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QVERIFY(tableView->itemAtCell(cell0_0));
    QVERIFY(tableView->itemAtCell(bottomCell));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(!tableView->itemAtCell(bottomCell)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Move currentIndex page down a second. The second time will cause a fast-flick.
    const QPoint bottomCellPageTwo(0, 38);
    QTest::keyPress(window, Qt::Key_PageDown);
    QTest::keyPress(window, Qt::Key_PageDown);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(bottomCellPageTwo));
    QTRY_VERIFY(tableView->itemAtCell(bottomCellPageTwo));
    QVERIFY(tableView->itemAtCell(bottomCellPageTwo)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), bottomCellPageTwo.x());
    QCOMPARE(tableView->currentRow(), bottomCellPageTwo.y());

    // Move currentIndex page down a third time. This will hit the end of the table
    // before a whole page can be reached.
    const QPoint cellVerEnd(0, tableView->rows() - 1);
    QTest::keyPress(window, Qt::Key_PageDown);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cellVerEnd));
    QTRY_VERIFY(tableView->itemAtCell(cellVerEnd));
    QVERIFY(tableView->itemAtCell(cellVerEnd)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cellVerEnd.x());
    QCOMPARE(tableView->currentRow(), cellVerEnd.y());

    // Move currentIndex page down once more is a no-op
    QTest::keyPress(window, Qt::Key_PageDown);
    QVERIFY(tableView->itemAtCell(cellVerEnd)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cellVerEnd.x());
    QCOMPARE(tableView->currentRow(), cellVerEnd.y());

    // Move currentIndex page up
    const QPoint cellTop1(0, tableView->topRow());
    QTest::keyPress(window, Qt::Key_PageUp);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cellTop1));
    QVERIFY(tableView->itemAtCell(cellTop1));
    QVERIFY(tableView->itemAtCell(cellTop1)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cellTop1.x());
    QCOMPARE(tableView->currentRow(), cellTop1.y());

    // Move currentIndex page up a second time. This will cause a fast-flick, which
    // happens to end up on row 1.
    const QPoint cell0_1(0, 1);
    QTest::keyPress(window, Qt::Key_PageUp);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_1));
    QTRY_VERIFY(tableView->itemAtCell(cell0_1));
    QVERIFY(tableView->itemAtCell(cell0_1)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_1.x());
    QCOMPARE(tableView->currentRow(), cell0_1.y());

    // Move currentIndex page up a third time. This will bring the table
    // all the way to the top.
    QTest::keyPress(window, Qt::Key_PageUp);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QTRY_VERIFY(tableView->itemAtCell(cell0_0));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Move currentIndex page up once more. This will be a no-op.
    QTest::keyPress(window, Qt::Key_PageUp);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Move currentIndex to a cell outside the viewport by accessing the selection
    // model directly, scroll to it, and check current status.
    const QPoint cellAtEnd(tableView->columns() - 1, tableView->rows() - 1);
    selectionModel.setCurrentIndex(tableView->modelIndex(cellAtEnd), QItemSelectionModel::NoUpdate);
    tableView->positionViewAtCell(cellAtEnd, QQuickTableView::AlignBottom | QQuickTableView::AlignRight);
    QTRY_VERIFY(tableView->itemAtCell(cellAtEnd));
    QVERIFY(tableView->itemAtCell(cellAtEnd)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cellAtEnd.x());
    QCOMPARE(tableView->currentRow(), cellAtEnd.y());
}

void tst_QQuickTableView::moveCurrentIndexUsingTabKey_data()
{
    QTest::addColumn<bool>("hide");

    QTest::newRow("all visible") << false;
    QTest::newRow("some hidden") << true;
}

void tst_QQuickTableView::moveCurrentIndexUsingTabKey()
{
    QFETCH(bool, hide);
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(5, 6);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setFocus(true);
    QQuickWindow *window = tableView->window();
    const char kCurrent[] = "current";

    int lastRow = 4;
    int lastColumn = 5;

    if (hide) {
        // Hide last row and column. Those sections should
        // no longer be taking into account when tabbing.
        tableView->setRowHeight(lastRow, 0);
        tableView->setColumnWidth(lastColumn, 0);
        lastRow--;
        lastColumn--;
    }

    WAIT_UNTIL_POLISHED;

    QVERIFY(tableView->activeFocusOnTab());

    QCOMPARE(tableView->currentColumn(), -1);
    QCOMPARE(tableView->currentRow(), -1);

    // Start by making cell 0, 0 current
    const QPoint cell0_0(0, 0);
    selectionModel.setCurrentIndex(tableView->modelIndex(cell0_0), QItemSelectionModel::NoUpdate);
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Press Tab
    const QPoint cell1_0(1, 0);
    QTest::keyPress(window, Qt::Key_Tab);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell1_0));
    QVERIFY(!tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(tableView->itemAtCell(cell1_0)->property(kCurrent).toBool());

    // Press Backtab
    QTest::keyPress(window, Qt::Key_Backtab);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(!tableView->itemAtCell(cell1_0)->property(kCurrent).toBool());
    QVERIFY(!selectionModel.hasSelection());

    // Press Backtab again. This wraps current index to the
    // bottom right of the table
    const QPoint cell_bottomRight(lastColumn, lastRow);
    QTest::keyPress(window, Qt::Key_Backtab);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell_bottomRight));
    QVERIFY(!tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(tableView->itemAtCell(cell_bottomRight)->property(kCurrent).toBool());
    QVERIFY(!selectionModel.hasSelection());

    // Press Tab. This wraps current index back to the 0_0
    QTest::keyPress(window, Qt::Key_Tab);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(!tableView->itemAtCell(cell_bottomRight)->property(kCurrent).toBool());
    QVERIFY(!selectionModel.hasSelection());

    // Make 0_1 current, and press Backtab. This should
    // wrap current index to the last column, but on the row above.
    const QPoint cell0_1(0, 1);
    const QPoint cellRightAbove(lastColumn, 0);
    selectionModel.setCurrentIndex(tableView->modelIndex(cell0_1), QItemSelectionModel::NoUpdate);
    QVERIFY(!tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(tableView->itemAtCell(cell0_1)->property(kCurrent).toBool());
    QTest::keyPress(window, Qt::Key_Backtab);
    QVERIFY(tableView->itemAtCell(cellRightAbove)->property(kCurrent).toBool());
    QVERIFY(!tableView->itemAtCell(cell0_1)->property(kCurrent).toBool());
    QVERIFY(!selectionModel.hasSelection());

    // Press Tab. This wraps current index back to 0_1
    QTest::keyPress(window, Qt::Key_Tab);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_1));
    QVERIFY(!tableView->itemAtCell(cellRightAbove)->property(kCurrent).toBool());
    QVERIFY(tableView->itemAtCell(cell0_1)->property(kCurrent).toBool());
    QVERIFY(!selectionModel.hasSelection());
}

void tst_QQuickTableView::respectActiveFocusOnTabDisabled()
{
    // Ensure that we don't move focus for tab or backtab
    // when TableView.setActiveFocusOnTab is false.
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(3, 3);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setActiveFocusOnTab(false);
    tableView->setFocus(true);

    QQuickWindow *window = tableView->window();
    const char kCurrent[] = "current";

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->currentColumn(), -1);
    QCOMPARE(tableView->currentRow(), -1);
    QVERIFY(!tableView->activeFocusOnTab());

    // Start by making cell 0, 0 current
    const QPoint cell0_0(0, 0);
    selectionModel.setCurrentIndex(tableView->modelIndex(cell0_0), QItemSelectionModel::NoUpdate);
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Press Tab
    const QPoint cell1_0(1, 0);
    QTest::keyPress(window, Qt::Key_Tab);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
    QVERIFY(!tableView->itemAtCell(cell1_0)->property(kCurrent).toBool());

    // Press Backtab
    QTest::keyPress(window, Qt::Key_Backtab);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kCurrent).toBool());
}

void tst_QQuickTableView::setCurrentIndexOnFirstKeyPress_data()
{
    QTest::addColumn<Qt::Key>("arrowKey");

    QTest::newRow("left") << Qt::Key_Left;
    QTest::newRow("right") << Qt::Key_Right;
    QTest::newRow("up") << Qt::Key_Up;
    QTest::newRow("down") << Qt::Key_Down;
}

void tst_QQuickTableView::setCurrentIndexOnFirstKeyPress()
{
    // Check that TableView has focus, but no cell is current, the
    // first key press on any of the arrow keys will assign the
    // top left cell to be current.
    QFETCH(Qt::Key, arrowKey);
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(2, 2);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setFocus(true);
    QQuickWindow *window = tableView->window();
    const char kCurrent[] = "current";

    WAIT_UNTIL_POLISHED;

    // Check that all delegates have current set to false upon start
    for (auto fxItem : tableViewPrivate->loadedItems)
        QVERIFY(!fxItem->item->property(kCurrent).toBool());

    QCOMPARE(tableView->currentColumn(), -1);
    QCOMPARE(tableView->currentRow(), -1);

    // Pressing a random key, e.g 'a', should not change current index
    QTest::keyPress(window, Qt::Key_A);
    QVERIFY(!selectionModel.currentIndex().isValid());
    QCOMPARE(tableView->currentColumn(), -1);
    QCOMPARE(tableView->currentRow(), -1);

    // Press the given arrow key
    const QPoint topLeftCell(0, 0);
    QTest::keyPress(window, arrowKey);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(topLeftCell));
    QVERIFY(tableView->itemAtCell(topLeftCell)->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), topLeftCell.x());
    QCOMPARE(tableView->currentRow(), topLeftCell.y());
}

void tst_QQuickTableView::setCurrentIndexFromMouse()
{
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(40, 40);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setFocus(true);
    QQuickWindow *window = tableView->window();
    QQuickItem *contentItem = window->contentItem();
    const char kCurrent[] = "current";

    WAIT_UNTIL_POLISHED;

    // Check that all delegates have current set to false upon start
    for (auto fxItem : tableViewPrivate->loadedItems)
        QVERIFY(!fxItem->item->property(kCurrent).toBool());

    QCOMPARE(tableView->currentColumn(), -1);
    QCOMPARE(tableView->currentRow(), -1);

    // Click on cell 0, 0
    const QPoint cell0_0(0, 0);
    const auto item0_0 = tableView->itemAtCell(cell0_0);
    QVERIFY(item0_0);
    QPoint pos = contentItem->mapFromItem(item0_0, QPointF(5, 5)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell0_0));
    QVERIFY(item0_0->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Click on cell 1, 2
    const QPoint cell1_2(1, 2);
    auto item1_2 = tableView->itemAtCell(cell1_2);
    QVERIFY(item1_2);
    pos = contentItem->mapFromItem(item1_2, QPointF(5, 5)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell1_2));
    QVERIFY(!item0_0->property(kCurrent).toBool());
    QVERIFY(item1_2->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cell1_2.x());
    QCOMPARE(tableView->currentRow(), cell1_2.y());

    // Position the view at the end of the table, and click on the bottom-right cell
    const QPoint cellAtEnd(tableView->columns() - 1, tableView->rows() - 1);
    tableView->positionViewAtCell(cellAtEnd, QQuickTableView::AlignBottom | QQuickTableView::AlignRight);
    WAIT_UNTIL_POLISHED;
    auto itemAtEnd = tableView->itemAtCell(cellAtEnd);
    QVERIFY(itemAtEnd);
    QVERIFY(!itemAtEnd->property(kCurrent).toBool());
    pos = contentItem->mapFromItem(itemAtEnd, QPointF(5, 5)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cellAtEnd));
    QVERIFY(itemAtEnd->property(kCurrent).toBool());
    QCOMPARE(tableView->currentColumn(), cellAtEnd.x());
    QCOMPARE(tableView->currentRow(), cellAtEnd.y());
}

void tst_QQuickTableView::showMarginsWhenNavigatingToEnd()
{
    LOAD_TABLEVIEW("plaintableview.qml");

    TestModel model(40, 40);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setAnimate(false);
    tableView->setFocus(true);

    QQuickWindow *window = tableView->window();

    WAIT_UNTIL_POLISHED;

    const qreal margin = 10;
    tableView->setLeftMargin(margin);
    tableView->setRightMargin(margin);
    tableView->setTopMargin(margin);
    tableView->setBottomMargin(margin);

    selectionModel.setCurrentIndex(tableView->modelIndex(QPoint(1, 1)), QItemSelectionModel::NoUpdate);

    // move to cell 0, 1
    QCOMPARE(tableView->contentX(), 0);
    QTest::keyPress(window, Qt::Key_Left);
    QCOMPARE(tableView->contentX(), -margin);

    // move to cell 0, 0
    QCOMPARE(tableView->contentY(), 0);
    QTest::keyPress(window, Qt::Key_Up);
    QCOMPARE(tableView->contentY(), -margin);

    selectionModel.setCurrentIndex(tableView->modelIndex(QPoint(38, 38)), QItemSelectionModel::NoUpdate);
    tableView->positionViewAtCell(tableView->cellAtIndex(selectionModel.currentIndex()), QQuickTableView::Contain);

    WAIT_UNTIL_POLISHED;

    // move to cell 39, 38
    QTest::keyPress(window, Qt::Key_Right);
    const qreal cellRightEdge = tableViewPrivate->loadedTableOuterRect.right();
    QCOMPARE(tableView->contentX(), cellRightEdge + margin - tableView->width());

    // move to cell 39, 39
    QTest::keyPress(window, Qt::Key_Down);
    const qreal cellBottomEdge = tableViewPrivate->loadedTableOuterRect.bottom();
    QCOMPARE(tableView->contentY(), cellBottomEdge + margin - tableView->height());
}

void tst_QQuickTableView::disablePointerNavigation()
{
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(40, 40);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setFocus(true);
    tableView->setPointerNavigationEnabled(false);
    QQuickWindow *window = tableView->window();
    QQuickItem *contentItem = window->contentItem();

    WAIT_UNTIL_POLISHED;

    QVERIFY(!selectionModel.currentIndex().isValid());

    // Click on cell 0, 0, nothing should happen
    const QPoint cell0_0(0, 0);
    const auto item0_0 = tableView->itemAtCell(cell0_0);
    QVERIFY(item0_0);
    QPoint pos = contentItem->mapFromItem(item0_0, QPointF(5, 5)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
    QVERIFY(!selectionModel.currentIndex().isValid());
    QVERIFY(!item0_0->property("current").toBool());
    QCOMPARE(tableView->currentColumn(), -1);
    QCOMPARE(tableView->currentRow(), -1);

    // Enable navigation, and try again
    tableView->setPointerNavigationEnabled(true);
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, pos);
    QCOMPARE(selectionModel.currentIndex(), tableView->index(0, 0));
    QVERIFY(item0_0->property("current").toBool());
    QCOMPARE(tableView->currentColumn(), cell0_0.x());
    QCOMPARE(tableView->currentRow(), cell0_0.y());

    // Set an invalid current index in the selection model
    selectionModel.setCurrentIndex(QModelIndex(), QItemSelectionModel::NoUpdate);
    QVERIFY(!item0_0->property("current").toBool());
    QCOMPARE(tableView->currentColumn(), -1);
    QCOMPARE(tableView->currentRow(), -1);
}

void tst_QQuickTableView::disableKeyNavigation()
{
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(40, 40);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setKeyNavigationEnabled(false);
    tableView->setFocus(true);
    QQuickWindow *window = tableView->window();
    const char kCurrent[] = "current";

    WAIT_UNTIL_POLISHED;

    // Start by making cell 1, 1 current
    const QPoint cell1_1(1, 1);
    selectionModel.setCurrentIndex(tableView->modelIndex(cell1_1), QItemSelectionModel::NoUpdate);
    QCOMPARE(tableView->itemAtCell(cell1_1)->property(kCurrent).toBool(), true);

    // Try to move currentIndex right by pressing Key_Right. Nothing should happen.
    const QPoint cell2_1(2, 1);
    QTest::keyPress(window, Qt::Key_Right);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell1_1));
    QVERIFY(tableView->itemAtCell(cell1_1)->property(kCurrent).toBool());
    QVERIFY(!tableView->itemAtCell(cell2_1)->property(kCurrent).toBool());

    // Enable navigation, and try again
    tableView->setKeyNavigationEnabled(true);
    QTest::keyPress(window, Qt::Key_Right);
    QCOMPARE(selectionModel.currentIndex(), tableView->modelIndex(cell2_1));
    QVERIFY(!tableView->itemAtCell(cell1_1)->property(kCurrent).toBool());
    QVERIFY(tableView->itemAtCell(cell2_1)->property(kCurrent).toBool());
}

void tst_QQuickTableView::selectUsingArrowKeys()
{
    // Select cells in the view using the keyboard
    // by going in a square around cell 1, 1
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(40, 40);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setFocus(true);
    QQuickWindow *window = tableView->window();
    const char kSelected[] = "selected";

    WAIT_UNTIL_POLISHED;

    // Check that all delegates have selected set to false upon start
    for (auto fxItem : tableViewPrivate->loadedItems)
        QCOMPARE(fxItem->item->property(kSelected).toBool(), false);

    // Start by making cell 1, 1 current
    const QPoint cell1_1(1, 1);
    selectionModel.setCurrentIndex(tableView->modelIndex(cell1_1), QItemSelectionModel::NoUpdate);
    QCOMPARE(tableView->itemAtCell(cell1_1)->property(kSelected).toBool(), false);

    // Move currentIndex right while holding down shift to select
    const QPoint cell2_1(2, 1);
    QTest::keyPress(window, Qt::Key_Right, Qt::ShiftModifier);
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_1)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell2_1)));
    QVERIFY(tableView->itemAtCell(cell1_1)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell2_1)->property(kSelected).toBool());

    // Move currentIndex down while holding down shift to select
    const QPoint cell2_2(2, 2);
    const QPoint cell1_2(1, 2);
    QTest::keyPress(window, Qt::Key_Down, Qt::ShiftModifier);
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_1)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell2_1)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell2_2)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_2)));
    QVERIFY(tableView->itemAtCell(cell1_1)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell2_1)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell2_2)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell1_2)->property(kSelected).toBool());

    // Move currentIndex left while holding down shift to select
    QTest::keyPress(window, Qt::Key_Left, Qt::ShiftModifier);
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_1)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_2)));
    QVERIFY(!selectionModel.isSelected(tableView->modelIndex(cell2_1)));
    QVERIFY(!selectionModel.isSelected(tableView->modelIndex(cell2_2)));
    QVERIFY(tableView->itemAtCell(cell1_1)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell1_2)->property(kSelected).toBool());
    QVERIFY(!tableView->itemAtCell(cell2_1)->property(kSelected).toBool());
    QVERIFY(!tableView->itemAtCell(cell2_2)->property(kSelected).toBool());

    // Move currentIndex left while holding down shift to select
    const QPoint cell0_1(0, 1);
    const QPoint cell0_2(0, 2);
    QTest::keyPress(window, Qt::Key_Left, Qt::ShiftModifier);
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell0_1)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell0_2)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_1)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_2)));
    QVERIFY(tableView->itemAtCell(cell0_1)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell0_2)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell1_1)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell1_2)->property(kSelected).toBool());

    // Move currentIndex up while holding down shift to select
    QTest::keyPress(window, Qt::Key_Up, Qt::ShiftModifier);
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell0_1)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_1)));
    QVERIFY(!selectionModel.isSelected(tableView->modelIndex(cell0_2)));
    QVERIFY(!selectionModel.isSelected(tableView->modelIndex(cell1_2)));
    QVERIFY(tableView->itemAtCell(cell0_1)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell1_1)->property(kSelected).toBool());
    QVERIFY(!tableView->itemAtCell(cell0_2)->property(kSelected).toBool());
    QVERIFY(!tableView->itemAtCell(cell1_2)->property(kSelected).toBool());

    // Move currentIndex up while holding down shift to select
    const QPoint cell0_0(0, 0);
    const QPoint cell1_0(1, 0);
    QTest::keyPress(window, Qt::Key_Up, Qt::ShiftModifier);
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell0_0)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell0_1)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_0)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_1)));
    QVERIFY(tableView->itemAtCell(cell0_0)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell0_1)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell1_0)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell1_1)->property(kSelected).toBool());

    // Move currentIndex right while holding down shift to select
    QTest::keyPress(window, Qt::Key_Right, Qt::ShiftModifier);
    QVERIFY(!selectionModel.isSelected(tableView->modelIndex(cell0_0)));
    QVERIFY(!selectionModel.isSelected(tableView->modelIndex(cell0_1)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_0)));
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell1_1)));
    QVERIFY(!tableView->itemAtCell(cell0_0)->property(kSelected).toBool());
    QVERIFY(!tableView->itemAtCell(cell0_1)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell1_0)->property(kSelected).toBool());
    QVERIFY(tableView->itemAtCell(cell1_1)->property(kSelected).toBool());

    // Finally, move currentIndex _without_ shift, which should clear the selection
    QTest::keyPress(window, Qt::Key_Right);
    QVERIFY(!selectionModel.isSelected(tableView->modelIndex(cell1_0)));
    QVERIFY(!selectionModel.isSelected(tableView->modelIndex(cell1_1)));
    QVERIFY(!tableView->itemAtCell(cell1_0)->property(kSelected).toBool());
    QVERIFY(!tableView->itemAtCell(cell1_1)->property(kSelected).toBool());
}

void tst_QQuickTableView::selectUsingHomeAndEndKeys()
{
    // Select cells in the view by using the home and end keys
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(4, 40);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setFocus(true);
    QQuickWindow *window = tableView->window();
    const char kSelected[] = "selected";

    WAIT_UNTIL_POLISHED;

    // Check that all delegates have selected set to false upon start
    for (auto fxItem : tableViewPrivate->loadedItems)
        QVERIFY(!fxItem->item->property(kSelected).toBool());

    // Start by making cell 0, 0 current
    const QPoint cell0_0(0, 0);
    selectionModel.setCurrentIndex(tableView->modelIndex(cell0_0), QItemSelectionModel::NoUpdate);
    QVERIFY(!tableView->itemAtCell(cell0_0)->property(kSelected).toBool());

    // Move currentIndex to the end while holding down shift to select
    const QPoint cellAtHorEnd(tableView->columns() - 1, 0);
    QTest::keyPress(window, Qt::Key_End, Qt::ShiftModifier);
    QTRY_VERIFY(tableView->itemAtCell(cellAtHorEnd));
    for (int c = 0; c <= cellAtHorEnd.x(); ++c) {
        const QPoint cell(c, cellAtHorEnd.y());
        QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell)));
        const QQuickItem *item = tableView->itemAtCell(cell);
        if (item)
            QVERIFY(item->property(kSelected).toBool());
    }

    // Move currentIndex to home while holding down shift to select.
    // This should result in only the first cell being selected.
    const QPoint cellAtHome(0, 0);
    QTest::keyPress(window, Qt::Key_Home, Qt::ShiftModifier);
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cellAtHome)));
    QTRY_VERIFY(tableView->itemAtCell(cellAtHome));
    QVERIFY(tableView->itemAtCell(cellAtHome)->property(kSelected).toBool());
    for (int c = 1; c <= cellAtHorEnd.x(); ++c) {
        const QPoint cell(c, cellAtHorEnd.y());
        QVERIFY(!selectionModel.isSelected(tableView->modelIndex(cell)));
        const QQuickItem *item = tableView->itemAtCell(cell);
        if (item)
            QVERIFY(!item->property(kSelected).toBool());
    }

    // Reverse the test, by starting from cellAtHorEnd
    selectionModel.setCurrentIndex(tableView->modelIndex(cellAtHorEnd), QItemSelectionModel::Clear);
    tableView->positionViewAtCell(cellAtHorEnd, QQuickTableView::AlignTop | QQuickTableView::AlignRight);
    WAIT_UNTIL_POLISHED;
    QQuickItem *itemAtHorEnd = tableView->itemAtCell(cellAtHorEnd);
    QVERIFY(itemAtHorEnd);
    QCOMPARE(itemAtHorEnd->property(kSelected).toBool(), false);

    // Move currentIndex home while holding down shift to select
    QTest::keyPress(window, Qt::Key_Home, Qt::ShiftModifier);
    QTRY_VERIFY(tableView->itemAtCell(cellAtHome));
    for (int c = 0; c <= cellAtHorEnd.x(); ++c) {
        const QPoint cell(c, cellAtHorEnd.y());
        QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell)));
        const QQuickItem *item = tableView->itemAtCell(cell);
        if (item)
            QVERIFY(item->property(kSelected).toBool());
    }

    // Move currentIndex to end while holding down shift to select.
    // This should result in only cellAtHorEnd being selected.
    QTest::keyPress(window, Qt::Key_End, Qt::ShiftModifier);
    QVERIFY(selectionModel.isSelected(tableView->modelIndex(cellAtHorEnd)));
    QTRY_VERIFY(tableView->itemAtCell(cellAtHorEnd));
    QVERIFY(tableView->itemAtCell(cellAtHorEnd)->property(kSelected).toBool());
    for (int c = 0; c < cellAtHorEnd.x(); ++c) {
        const QPoint cell(c, cellAtHorEnd.y());
        QVERIFY(!selectionModel.isSelected(tableView->modelIndex(cell)));
        const QQuickItem *item = tableView->itemAtCell(cell);
        if (item)
            QVERIFY(!item->property(kSelected).toBool());
    }
}

void tst_QQuickTableView::selectUsingPageUpDownKeys()
{
    // Select cells in the view by using the page up and down keys
    LOAD_TABLEVIEW("tableviewwithselected1.qml");

    TestModel model(30, 3);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);
    tableView->setFocus(true);
    QQuickWindow *window = tableView->window();
    const char kSelected[] = "selected";

    WAIT_UNTIL_POLISHED;

    // Check that all delegates have selected set to false upon start
    for (auto fxItem : tableViewPrivate->loadedItems)
        QVERIFY(!fxItem->item->property(kSelected).toBool());

    // Start by making cell 0, 0 current
    const QPoint cell0_0(0, 0);
    selectionModel.setCurrentIndex(tableView->modelIndex(cell0_0), QItemSelectionModel::NoUpdate);
    QVERIFY(!tableView->itemAtCell(cell0_0)->property(kSelected).toBool());

    // Move currentIndex page down while holding down shift to select
    const QPoint cellAtBottom(0, tableView->bottomRow());
    QTest::keyPress(window, Qt::Key_PageDown, Qt::ShiftModifier);
    QVERIFY(tableView->itemAtCell(cellAtBottom));
    for (int r = 0; r <= cellAtBottom.y(); ++r) {
        const QPoint cell(cellAtBottom.x(), r);
        QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell)));
        const QQuickItem *item = tableView->itemAtCell(cell);
        if (item)
            QVERIFY(item->property(kSelected).toBool());
    }

    // Move currentIndex page up while holding down shift to select
    const QPoint cellAtTop(0, 0);
    QTest::keyPress(window, Qt::Key_PageUp, Qt::ShiftModifier);
    QVERIFY(tableView->itemAtCell(cellAtTop));
    QVERIFY(tableView->itemAtCell(cellAtTop)->property(kSelected).toBool());
    for (int r = 1; r <= cellAtBottom.y(); ++r) {
        const QPoint cell(cellAtBottom.x(), r);
        QVERIFY(!selectionModel.isSelected(tableView->modelIndex(cell)));
        const QQuickItem *item = tableView->itemAtCell(cell);
        if (item)
            QVERIFY(!item->property(kSelected).toBool());
    }

    // Move currentIndex page down twice while holding down shift to select.
    // This will select all cells in the first column, even the ones that are initially hidden.
    const QPoint cellAtVerEnd(0, tableView->rows() - 1);
    QTest::keyPress(window, Qt::Key_PageDown, Qt::ShiftModifier);
    QTest::keyPress(window, Qt::Key_PageDown, Qt::ShiftModifier);
    QTRY_VERIFY(tableView->itemAtCell(cellAtVerEnd));
    QCOMPARE(tableView->bottomRow(), cellAtVerEnd.y());
    for (int r = 0; r <= cellAtBottom.y(); ++r) {
        const QPoint cell(cellAtBottom.x(), r);
        QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell)));
        const QQuickItem *item = tableView->itemAtCell(cell);
        if (item)
            QVERIFY(item->property(kSelected).toBool());
    }

    // Reverse the test, by starting from cellAtVerEnd
    selectionModel.clearSelection();
    QVERIFY(!tableView->itemAtCell(cellAtVerEnd)->property(kSelected).toBool());

    // Move currentIndex page up while holding down shift to select
    const QPoint cellAtTopRow(0, tableView->topRow());
    QTest::keyPress(window, Qt::Key_PageUp, Qt::ShiftModifier);
    QTRY_VERIFY(tableView->itemAtCell(cellAtTopRow));
    for (int r = cellAtTopRow.y(); r <= cellAtVerEnd.y(); ++r) {
        const QPoint cell(cellAtBottom.x(), r);
        QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell)));
        const QQuickItem *item = tableView->itemAtCell(cell);
        if (item)
            QVERIFY(item->property(kSelected).toBool());
    }

    // Move currentIndex page up once more while holding down shift to select.
    // This will bring currentIndex to the top.
    QTest::keyPress(window, Qt::Key_PageUp, Qt::ShiftModifier);
    QTRY_VERIFY(tableView->itemAtCell(cellAtTop));
    for (int r = cellAtTop.y(); r <= cellAtVerEnd.y(); ++r) {
        const QPoint cell(cellAtBottom.x(), r);
        QVERIFY(selectionModel.isSelected(tableView->modelIndex(cell)));
        const QQuickItem *item = tableView->itemAtCell(cell);
        if (item)
            QVERIFY(item->property(kSelected).toBool());
    }
}

void tst_QQuickTableView::testDeprecatedApi()
{
    // Check that you can still use Qt.Alignment as second argument
    // to positionViewAtCell() (for backwards compatibility before Qt 6.4)
    LOAD_TABLEVIEW("deprecatedapi.qml");

    TestModel model(200, 200);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);

    WAIT_UNTIL_POLISHED;

    QMetaObject::invokeMethod(tableView, "positionUsingDeprecatedEnum");

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->rightColumn(), model.columnCount() - 1);
    QCOMPARE(tableView->bottomRow(), model.rowCount() - 1);
}

void tst_QQuickTableView::alternatingRows()
{
    // Check that you can set 'alternate'
    LOAD_TABLEVIEW("plaintableview.qml");

    QVERIFY(tableView->alternatingRows());
    tableView->setAlternatingRows(false);
    QVERIFY(!tableView->alternatingRows());
    tableView->setAlternatingRows(true);
    QVERIFY(tableView->alternatingRows());
}

void tst_QQuickTableView::boundDelegateComponent()
{
    QQmlEngine engine;
    const QUrl url(testFileUrl("boundDelegateComponent.qml"));
    QQmlComponent c(&engine, url);
    QVERIFY2(c.isReady(), qPrintable(c.errorString()));

    QTest::ignoreMessage(
            QtWarningMsg, qPrintable(QLatin1String("%1:16: ReferenceError: index is not defined")
                                             .arg(url.toString())));

    QScopedPointer<QObject> o(c.create());
    QVERIFY(!o.isNull());

    QQmlContext *context = qmlContext(o.data());

    QObject *inner = context->objectForName(QLatin1String("tableView"));
    QVERIFY(inner != nullptr);
    QQuickTableView *tableView = qobject_cast<QQuickTableView *>(inner);
    QVERIFY(tableView != nullptr);
    QObject *item = tableView->itemAtCell({0, 0});
    QVERIFY(item);
    QCOMPARE(item->objectName(), QLatin1String("fooouterundefined"));

    QObject *inner2 = context->objectForName(QLatin1String("tableView2"));
    QVERIFY(inner2 != nullptr);
    QQuickTableView *tableView2 = qobject_cast<QQuickTableView *>(inner2);
    QVERIFY(tableView2 != nullptr);
    QObject *item2 = tableView2->itemAtCell({0, 0});
    QVERIFY(item2);
    QCOMPARE(item2->objectName(), QLatin1String("fooouter0"));

    QQmlComponent *comp = qobject_cast<QQmlComponent *>(
            context->objectForName(QLatin1String("outerComponent")));
    QVERIFY(comp != nullptr);

    for (int i = 0; i < 3 * 2; ++i) {
        QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(QLatin1String("%1:54:21: ReferenceError: model is not defined")
                                   .arg(url.toString())));
    }

    QScopedPointer<QObject> outerItem(comp->create(context));
    QVERIFY(!outerItem.isNull());
    QQuickTableView *innerTableView = qobject_cast<QQuickTableView *>(
            qmlContext(outerItem.data())->objectForName(QLatin1String("innerTableView")));
    QVERIFY(innerTableView != nullptr);
    QCOMPARE(innerTableView->rows(), 3);
    for (int i = 0; i < 3; ++i)
        QVERIFY(innerTableView->itemAtIndex(innerTableView->index(i, 0))->objectName().isEmpty());
}

void tst_QQuickTableView::setColumnWidth_data()
{
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<int>("column");
    QTest::addColumn<qreal>("size");

    QTest::newRow("first column") << 5 << 0 << 10.;
    QTest::newRow("second column") << 5 << 2 << 10.;
    QTest::newRow("a hidden column") << 20 << 19 << 10.;
    QTest::newRow("a column outside model") << 1 << 5 << 10.;
}

void tst_QQuickTableView::setColumnWidth()
{
    // Test that you can set the width of a column explicitly
    QFETCH(int, columnCount);
    QFETCH(int, column);
    QFETCH(qreal, size);
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(2, columnCount);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    tableView->setColumnWidth(column, size);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitColumnWidth(column), size);
    if (tableView->isColumnLoaded(column))
        QCOMPARE(tableView->columnWidth(column), size);
    else
        QCOMPARE(tableView->columnWidth(column), -1);
}


void tst_QQuickTableView::setColumnWidthWhenProviderIsSet()
{
    // Test that explicitly set column widths will be
    // ignored if a columnWidthProvider is set
    LOAD_TABLEVIEW("userowcolumnprovider.qml");

    auto model = TestModelAsVariant(5, 5);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    tableView->setColumnWidth(1, 100);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitColumnWidth(1), 100);
    QCOMPARE(tableView->columnWidth(1), 11);
}

void tst_QQuickTableView::setColumnWidthForInvalidColumn()
{
    // Check that you cannot set a column width for
    // a negative column index.
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(5, 5);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*column must be greather than, or equal to, zero"));
    tableView->setColumnWidth(-1, 10);

    QCOMPARE(tableView->explicitColumnWidth(-1), -1);
    QCOMPARE(tableView->columnWidth(-1), -1);
}

void tst_QQuickTableView::setColumnWidthWhenUsingSyncView()
{
    // Test that if you set an explicit column width on a TableView
    // that has a sync view, then we set the column width on the
    // sync view instead.
    LOAD_TABLEVIEW("syncviewsimple.qml");
    GET_QML_TABLEVIEW(tableViewH);
    GET_QML_TABLEVIEW(tableViewHV);

    const auto model = TestModelAsVariant(3, 3);
    QQuickTableView *views[] = {tableView, tableViewH, tableViewHV};
    for (auto view : views)
        view->setModel(model);

    const int column = 1;
    const qreal size = 200;

    tableView->setColumnWidthProvider(QJSValue());
    tableViewH->setColumnWidth(column, size);

    WAIT_UNTIL_POLISHED;

    for (auto view : views) {
        QCOMPARE(view->explicitColumnWidth(column), size);
        QCOMPARE(view->columnWidth(column), size);
    }
}

void tst_QQuickTableView::resetColumnWidth()
{
    // Check that you can reset a column width
    // by setting its width to -1
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(5, 5);
    tableView->setModel(model);

    const int column = 1;
    const qreal size = 10.;
    const qreal defaultSize = 100.;

    WAIT_UNTIL_POLISHED;

    tableView->setColumnWidth(column, size);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitColumnWidth(column), size);
    QCOMPARE(tableView->columnWidth(column), size);

    tableView->setColumnWidth(column, -1);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitColumnWidth(column), -1);
    QCOMPARE(tableView->columnWidth(column), defaultSize);
}

void tst_QQuickTableView::clearColumnWidths()
{
    // Check that clearColumnWidths() works as documented
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(5, 5);
    tableView->setModel(model);

    const qreal defaultSize = 100.;

    WAIT_UNTIL_POLISHED;

    tableView->setColumnWidth(0, 10);
    tableView->setColumnWidth(1, 20);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitColumnWidth(0), 10);
    QCOMPARE(tableView->columnWidth(0), 10);
    QCOMPARE(tableView->explicitColumnWidth(1), 20);
    QCOMPARE(tableView->columnWidth(1), 20);

    tableView->clearColumnWidths();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitColumnWidth(0), -1);
    QCOMPARE(tableView->columnWidth(0), defaultSize);
    QCOMPARE(tableView->explicitColumnWidth(1), -1);
    QCOMPARE(tableView->columnWidth(1), defaultSize);
}

void tst_QQuickTableView::setRowHeight_data()
{
    QTest::addColumn<int>("rowCount");
    QTest::addColumn<int>("row");
    QTest::addColumn<qreal>("size");

    QTest::newRow("first row") << 5 << 0 << 10.;
    QTest::newRow("second row") << 5 << 2 << 10.;
    QTest::newRow("a hidden row") << 20 << 19 << 10.;
    QTest::newRow("a row outside model") << 1 << 5 << 10.;
}

void tst_QQuickTableView::setRowHeight()
{
    // Test that you can set the height of a row explicitly
    QFETCH(int, rowCount);
    QFETCH(int, row);
    QFETCH(qreal, size);
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(rowCount, 2);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    tableView->setRowHeight(row, size);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitRowHeight(row), size);
    if (tableView->isRowLoaded(row))
        QCOMPARE(tableView->rowHeight(row), size);
    else
        QCOMPARE(tableView->rowHeight(row), -1);
}

void tst_QQuickTableView::setRowHeightWhenProviderIsSet()
{
    // Test that explicitly set row heights will be
    // ignored if a rowHeightProvider is set
    LOAD_TABLEVIEW("userowcolumnprovider.qml");

    auto model = TestModelAsVariant(5, 5);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    tableView->setRowHeight(1, 100);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitRowHeight(1), 100);
    QCOMPARE(tableView->rowHeight(1), 11);
}

void tst_QQuickTableView::setRowHeightForInvalidRow()
{
    // Check that you cannot set a row height for
    // a negative row index.
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(5, 5);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*row must be greather than, or equal to, zero"));
    tableView->setRowHeight(-1, 10);

    QCOMPARE(tableView->explicitRowHeight(-1), -1);
    QCOMPARE(tableView->rowHeight(-1), -1);
}

void tst_QQuickTableView::setRowHeightWhenUsingSyncView()
{
    // Test that if you set an explicit row height on a TableView
    // that has a sync view, then we set the column width on the
    // sync view instead.
    LOAD_TABLEVIEW("syncviewsimple.qml");
    GET_QML_TABLEVIEW(tableViewV);
    GET_QML_TABLEVIEW(tableViewHV);

    const auto model = TestModelAsVariant(3, 3);
    QQuickTableView *views[] = {tableView, tableViewV, tableViewHV};
    for (auto view : views)
        view->setModel(model);

    const int row = 1;
    const qreal size = 200;

    tableView->setRowHeightProvider(QJSValue());
    tableViewV->setRowHeight(row, size);

    WAIT_UNTIL_POLISHED;

    for (auto view : views) {
        QCOMPARE(view->explicitRowHeight(row), size);
        QCOMPARE(view->rowHeight(row), size);
    }
}

void tst_QQuickTableView::resetRowHeight()
{
    // Check that you can reset a row height
    // by setting its width to -1
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(5, 5);
    tableView->setModel(model);

    const int row = 1;
    const qreal size = 10.;
    const qreal defaultSize = 50.;

    WAIT_UNTIL_POLISHED;

    tableView->setRowHeight(row, size);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitRowHeight(row), size);
    QCOMPARE(tableView->rowHeight(row), size);

    tableView->setRowHeight(row, -1);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitRowHeight(row), -1);
    QCOMPARE(tableView->rowHeight(row), defaultSize);
}

void tst_QQuickTableView::clearRowHeights()
{
    // Check that clearRowHeights() works as documented
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(5, 5);
    tableView->setModel(model);

    const qreal defaultSize = 50.;

    WAIT_UNTIL_POLISHED;

    tableView->setRowHeight(0, 10);
    tableView->setRowHeight(1, 20);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitRowHeight(0), 10);
    QCOMPARE(tableView->rowHeight(0), 10);
    QCOMPARE(tableView->explicitRowHeight(1), 20);
    QCOMPARE(tableView->rowHeight(1), 20);

    tableView->clearRowHeights();

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitRowHeight(0), -1);
    QCOMPARE(tableView->rowHeight(0), defaultSize);
    QCOMPARE(tableView->explicitRowHeight(1), -1);
    QCOMPARE(tableView->rowHeight(1), defaultSize);
}

void tst_QQuickTableView::deletedDelegate()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("deletedDelegate.qml"));
    std::unique_ptr<QObject> root(component.create());
    QVERIFY(root);
    auto tv = root->findChild<QQuickTableView *>("tableview");
    QVERIFY(tv);
    // we need one event loop iteration for the deferred delete to trigger
    // thus the QTRY_VERIFY
    QTRY_COMPARE(tv->delegate(), nullptr);
}

void tst_QQuickTableView::tableViewInteractive()
{
    LOAD_TABLEVIEW("tableviewinteractive.qml");

    auto *root = view->rootObject();
    QVERIFY(root);
    auto *window = root->window();
    QVERIFY(window);

    int eventCount = root->property("eventCount").toInt();
    QCOMPARE(eventCount, 0);

    // Event though we make 'interactive' as false, the TableView has
    // pointerNacigationEnabled set as true by default, which allows it to consume
    // mouse events and thus, eventCount still be zero
    tableView->setInteractive(false);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    eventCount = root->property("eventCount").toInt();
    QCOMPARE(eventCount, 0);

    // Making both 'interactive' and 'pointerNavigationEnabled' as false, doesn't
    // allow TableView (and its parent Flickable)  to consume mouse event and it
    // passes to the below visual item
    tableView->setInteractive(false);
    tableView->setPointerNavigationEnabled(false);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    eventCount = root->property("eventCount").toInt();
    QCOMPARE(eventCount, 1);

    // Making 'interactive' as true and 'pointerNavigationEnabled' as false,
    // allows parent of TableView (i.e. Flickable) to consume mouse events
    tableView->setInteractive(true);
    tableView->setPointerNavigationEnabled(false);
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    eventCount = root->property("eventCount").toInt();
    QCOMPARE(eventCount, 1);
}

void tst_QQuickTableView::columnResizing_data()
{
    QTest::addColumn<int>("column");
    QTest::addColumn<bool>("pointerNavigationEnabled");

    QTest::newRow("first") << 0 << true;
    QTest::newRow("middle") << 1 << true;
    QTest::newRow("middle") << 1 << false;
    QTest::newRow("last") << 2 << true;
}

void tst_QQuickTableView::columnResizing()
{
    // Check that the user can drag on the horizontal
    // end of a cell to resize the whole column.
    QFETCH(int, column);
    QFETCH(bool, pointerNavigationEnabled);
    LOAD_TABLEVIEW("tableviewwithselected2.qml");

    auto model = TestModelAsVariant(3, 3);
    tableView->setModel(model);
    tableView->setResizableColumns(true);
    // Resizing column should not be affected by pointerNavigationEnabled
    // (since it is controller by its own property).
    tableView->setPointerNavigationEnabled(pointerNavigationEnabled);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitColumnWidth(column), -1);

    // A resize shouldn't also change the current index or start a selection.
    QSignalSpy currentIndexSpy(tableView->selectionModel(), &QItemSelectionModel::currentChanged);
    QSignalSpy selectionSpy(tableView->selectionModel(), &QItemSelectionModel::selectionChanged);

    const auto item = tableView->itemAtIndex(tableView->index(0, column));
    QQuickWindow *window = item->window();

    const qreal columnStartWidth = tableView->columnWidth(column);
    const QPoint localPos = QPoint(item->width(), item->height() / 2);
    const QPoint startPos = window->contentItem()->mapFromItem(item, localPos).toPoint();
    const QPoint startDragDist = QPoint(qApp->styleHints()->startDragDistance() + 1, 0);
    const QPoint dragLength(100, 0);

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, startPos);
    QTest::mouseMove(window, startPos + startDragDist);
    QTest::mouseMove(window, startPos + dragLength);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, startPos + dragLength);

    const qreal newColumnWidth = columnStartWidth + dragLength.x() - startDragDist.x();
    QCOMPARE(tableView->explicitColumnWidth(column), newColumnWidth);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->columnWidth(column), newColumnWidth);

    QCOMPARE(currentIndexSpy.count(), 0);
    QCOMPARE(selectionSpy.count(), 0);
}

void tst_QQuickTableView::rowResizing_data()
{
    QTest::addColumn<int>("row");

    QTest::newRow("first") << 0;
    QTest::newRow("middle") << 1;
    QTest::newRow("last") << 2;
}

void tst_QQuickTableView::rowResizing()
{
    // Check that the user can drag on the vertical
    // end of a cell to resize the whole row.
    QFETCH(int, row);
    LOAD_TABLEVIEW("tableviewwithselected2.qml");

    auto model = TestModelAsVariant(3, 3);
    tableView->setModel(model);
    tableView->setResizableRows(true);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitRowHeight(row), -1);

    // A resize shouldn't also change the current index or start a selection.
    QSignalSpy currentIndexSpy(tableView->selectionModel(), &QItemSelectionModel::currentChanged);
    QSignalSpy selectionSpy(tableView->selectionModel(), &QItemSelectionModel::selectionChanged);

    const auto item = tableView->itemAtIndex(tableView->index(row, 0));
    QQuickWindow *window = item->window();

    const qreal rowStartHeight = tableView->rowHeight(row);
    const QPoint localPos = QPoint(item->width() / 2, item->height());
    const QPoint startPos = window->contentItem()->mapFromItem(item, localPos).toPoint();
    const QPoint startDragDist = QPoint(0, qApp->styleHints()->startDragDistance() + 1);
    const QPoint dragLength(0, 100);

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, startPos);
    QTest::mouseMove(window, startPos + startDragDist);
    QTest::mouseMove(window, startPos + dragLength);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, startPos + dragLength);

    const qreal newRowHeight = rowStartHeight + dragLength.y() - startDragDist.y();
    QCOMPARE(tableView->explicitRowHeight(row), newRowHeight);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->rowHeight(row), newRowHeight);

    QCOMPARE(currentIndexSpy.count(), 0);
    QCOMPARE(selectionSpy.count(), 0);
}

void tst_QQuickTableView::rowAndColumnResizing_data()
{
    QTest::addColumn<int>("rowAndColumn");
    QTest::addColumn<bool>("addDelegateDragHandler");

    QTest::newRow("first") << 0 << false;
    QTest::newRow("middle") << 1 << false;
    QTest::newRow("last") << 2 << false;

    QTest::newRow("first, addDelegateDragHandler") << 0 << true;
}

void tst_QQuickTableView::rowAndColumnResizing()
{
    // Check that the user can drag in the corner of a cell
    // to resize both the row and the column at the same time.
    QFETCH(int, rowAndColumn);
    QFETCH(bool, addDelegateDragHandler);
    LOAD_TABLEVIEW("tableviewwithselected2.qml");

    auto model = TestModelAsVariant(3, 3);
    tableView->setModel(model);
    tableView->setResizableColumns(true);
    tableView->setResizableRows(true);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->explicitColumnWidth(rowAndColumn), -1);
    QCOMPARE(tableView->explicitRowHeight(rowAndColumn), -1);

    const auto item = tableView->itemAtIndex(tableView->index(rowAndColumn, rowAndColumn));
    QVERIFY(item);

    if (addDelegateDragHandler) {
        // Check that the grab permissions set on the resize handler
        // allows you to add an ordinary drag handler to a delegate
        // without blocking the resize handler.
        new QQuickDragHandler(item);
    }

    QQuickWindow *window = item->window();

    const qreal columnStartWidth = tableView->columnWidth(rowAndColumn);
    const qreal rowStartHeight = tableView->rowHeight(rowAndColumn);

    const QPoint localPos = QPoint(item->width(), item->height());
    const QPoint startPos = window->contentItem()->mapFromItem(item, localPos).toPoint();
    const qreal startDist = qApp->styleHints()->startDragDistance();
    const QPoint startDragDist = QPoint(startDist + 1, startDist + 1);
    const QPoint dragLength(100, 100);

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, startPos);
    QTest::mouseMove(window, startPos + startDragDist);
    QTest::mouseMove(window, startPos + dragLength);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, startPos + dragLength);

    const qreal newColumnWidth = columnStartWidth + dragLength.x() - startDragDist.x();
    const qreal newRowHeight = rowStartHeight + dragLength.y() - startDragDist.y();
    QCOMPARE(tableView->explicitColumnWidth(rowAndColumn), newColumnWidth);
    QCOMPARE(tableView->explicitRowHeight(rowAndColumn), newRowHeight);
    WAIT_UNTIL_POLISHED;
    QCOMPARE(tableView->columnWidth(rowAndColumn), newColumnWidth);
    QCOMPARE(tableView->rowHeight(rowAndColumn), newRowHeight);

    // A resize shouldn't also change the current index
    QVERIFY(!tableView->selectionModel()->currentIndex().isValid());
}

void tst_QQuickTableView::columnResizingDisabled()
{
    // Check that the user cannot drag on the horizontal end of a cell
    // to resize a column if not resizableColumns is enabled.
    // In that case, a drag should drag the flickable instead.
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(3, 3);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->contentY(), 0);

    const int row = 1;
    QCOMPARE(tableView->explicitRowHeight(row), -1);

    const auto item = tableView->itemAtIndex(tableView->index(row, 0));
    QQuickWindow *window = item->window();

    const QPoint localPos = QPoint(item->width() / 2, item->height());
    const QPoint startPos = window->contentItem()->mapFromItem(item, localPos).toPoint();
    const QPoint startDragDist = QPoint(0, qApp->styleHints()->startDragDistance() + 1);
    const QPoint dragLength(0, 100);

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, startPos);
    QTest::mouseMove(window, startPos + startDragDist);
    QTest::mouseMove(window, startPos + dragLength);
    QTest::mouseMove(window, startPos + (dragLength * 2));
    QVERIFY(tableView->contentY() < 0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, startPos + dragLength);

    QVERIFY(!QQuickTest::qIsPolishScheduled(item));
    QCOMPARE(tableView->explicitRowHeight(row), -1);
    QCOMPARE(tableView->rowHeight(row), 50);
}

void tst_QQuickTableView::rowResizingDisabled()
{
    // Check that the user cannot drag on the vertical end of a cell to
    // resize a row if not resizableRows is enabled.
    // In that case, a drag should drag the flickable instead.
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(3, 3);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->contentY(), 0);

    const int row = 1;
    QCOMPARE(tableView->explicitRowHeight(row), -1);

    const auto item = tableView->itemAtIndex(tableView->index(row, 0));
    QQuickWindow *window = item->window();

    const QPoint localPos = QPoint(item->width() / 2, item->height());
    const QPoint startPos = window->contentItem()->mapFromItem(item, localPos).toPoint();
    const QPoint startDragDist = QPoint(0, qApp->styleHints()->startDragDistance() + 1);
    const QPoint dragLength(0, 100);

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, startPos);
    QTest::mouseMove(window, startPos + startDragDist);
    QTest::mouseMove(window, startPos + dragLength);
    QTest::mouseMove(window, startPos + (dragLength * 2));
    QVERIFY(tableView->contentY() < 0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, startPos + dragLength);

    QVERIFY(!QQuickTest::qIsPolishScheduled(item));
    QCOMPARE(tableView->explicitRowHeight(row), -1);
    QCOMPARE(tableView->rowHeight(row), 50);
}

void tst_QQuickTableView::dragFromCellCenter()
{
    // Check that the user cannot resize a row (or column) by dragging
    // from the center of a cell. In that case, a drag should
    // drag the flickable instead.
    LOAD_TABLEVIEW("plaintableview.qml");

    auto model = TestModelAsVariant(3, 3);
    tableView->setModel(model);
    tableView->setResizableRows(true);

    WAIT_UNTIL_POLISHED;

    QCOMPARE(tableView->contentY(), 0);

    const int row = 1;
    QCOMPARE(tableView->explicitRowHeight(row), -1);

    const auto item = tableView->itemAtIndex(tableView->index(row, 0));
    QQuickWindow *window = item->window();

    const QPoint localPos = QPoint(item->width() / 2, item->height() / 2);
    const QPoint startPos = window->contentItem()->mapFromItem(item, localPos).toPoint();
    const QPoint startDragDist = QPoint(0, qApp->styleHints()->startDragDistance() + 1);
    const QPoint dragLength(0, 100);

    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, startPos);
    QTest::mouseMove(window, startPos + startDragDist);
    QTest::mouseMove(window, startPos + dragLength);
    QTest::mouseMove(window, startPos + (dragLength * 2));
    QVERIFY(tableView->contentY() < 0);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, startPos + dragLength);

    QVERIFY(!QQuickTest::qIsPolishScheduled(item));
    QCOMPARE(tableView->explicitRowHeight(row), -1);
    QCOMPARE(tableView->rowHeight(row), 50);
}

void tst_QQuickTableView::tapOnResizeArea_data()
{
    QTest::addColumn<bool>("resizableRows");
    QTest::addColumn<bool>("resizableColumns");
    QTest::addColumn<bool>("interactive");

    for (bool interactive : {true, false}) {
        QTest::newRow("resize disabled") << false << false << interactive;
        QTest::newRow("resizableRows") << true << false << interactive;
        QTest::newRow("resizableColumns") << false << true << interactive;
        QTest::newRow("resizableRows && resizableColumns") << true << true << interactive;
    }
}

void tst_QQuickTableView::tapOnResizeArea()
{
    // Check that if a tap or a press happens on the resize area between the
    // cells, we only change the current index if the resizing is disabled.
    QFETCH(bool, resizableRows);
    QFETCH(bool, resizableColumns);
    QFETCH(bool, interactive);
    LOAD_TABLEVIEW("tableviewwithselected2.qml");

    auto model = TestModel(3, 3);
    tableView->setModel(QVariant::fromValue(&model));
    tableView->setResizableRows(resizableRows);
    tableView->setResizableColumns(resizableColumns);
    tableView->setInteractive(interactive);
    tableView->setPointerNavigationEnabled(true);

    WAIT_UNTIL_POLISHED;

    const QPoint cell(1, 1);
    const auto item = tableView->itemAtCell(cell);
    QQuickWindow *window = item->window();

    const QPoint localPos = QPoint(item->width() - 1, item->height() - 1);
    const QPoint tapPos = window->contentItem()->mapFromItem(item, localPos).toPoint();

    // Start by moving the mouse out of the way
    QTest::mouseMove(window, tapPos + QPoint(200, 200));
    // Then do a tap on the resize area
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, tapPos);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, tapPos);

    if (resizableRows || resizableColumns)
        QVERIFY(!tableView->selectionModel()->currentIndex().isValid());
    else
        QCOMPARE(tableView->selectionModel()->currentIndex(), model.index(1, 1));
}

void tst_QQuickTableView::editUsingEditTriggers_data()
{
    QTest::addColumn<QQuickTableView::EditTriggers>("editTriggers");
    QTest::addColumn<bool>("interactive");

    // We need to test both with and without interactive, since SingleTapped
    // actions will happen already on press in a TableView that is not interactive!
    for (bool interactive : {true, false}) {
        QTest::newRow("NoEditTriggers") << QQuickTableView::EditTriggers(QQuickTableView::NoEditTriggers) << interactive;
        QTest::newRow("SingleTapped") << QQuickTableView::EditTriggers(QQuickTableView::SingleTapped) << interactive;
        QTest::newRow("DoubleTapped") << QQuickTableView::EditTriggers(QQuickTableView::DoubleTapped) << interactive;
        QTest::newRow("SelectedTapped") << QQuickTableView::EditTriggers(QQuickTableView::SelectedTapped) << interactive;
        QTest::newRow("EditKeyPressed") << QQuickTableView::EditTriggers(QQuickTableView::EditKeyPressed) << interactive;
        QTest::newRow("AnyKeyPressed") << QQuickTableView::EditTriggers(QQuickTableView::EditKeyPressed) << interactive;
        QTest::newRow("DoubleTapped | EditKeyPressed")
                << QQuickTableView::EditTriggers(QQuickTableView::DoubleTapped | QQuickTableView::EditKeyPressed) << interactive;
        QTest::newRow("SingleTapped | AnyKeyPressed")
                << QQuickTableView::EditTriggers(QQuickTableView::SingleTapped | QQuickTableView::AnyKeyPressed) << interactive;
    }
}

void tst_QQuickTableView::editUsingEditTriggers()
{
    // Check that you can start to edit in TableView
    // using the available edit triggers.
    QFETCH(QQuickTableView::EditTriggers, editTriggers);
    QFETCH(bool, interactive);
    LOAD_TABLEVIEW("editdelegate.qml");

    auto model = TestModel(4, 4);
    tableView->setModel(QVariant::fromValue(&model));
    tableView->setInteractive(interactive);
    tableView->forceActiveFocus();

    QCOMPARE(tableView->editTriggers(), QQuickTableView::DoubleTapped | QQuickTableView::EditKeyPressed);
    tableView->setEditTriggers(editTriggers);

    const char kEditItem[] = "editItem";
    const char kEditIndex[] = "editIndex";

    WAIT_UNTIL_POLISHED;

    QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
    QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());

    const QPoint cell1(1, 1);
    const QPoint cell2(2, 1);
    const QModelIndex index1 = tableView->modelIndex(cell1);
    const QModelIndex index2 = tableView->modelIndex(cell2);
    const auto item1 = tableView->itemAtCell(cell1);
    const auto item2 = tableView->itemAtCell(cell2);
    QVERIFY(item1);
    QVERIFY(item2);

    QQuickWindow *window = tableView->window();

    const QPoint localPos = QPoint(item1->width() - 1, item1->height() - 1);
    const QPoint localPosOutside = QPoint(tableView->contentWidth() + 10, tableView->contentHeight() + 10);
    const QPoint tapPos1 = window->contentItem()->mapFromItem(item1, localPos).toPoint();
    const QPoint tapPos2 = window->contentItem()->mapFromItem(item2, localPos).toPoint();
    const QPoint tapOutsideContentItem = window->contentItem()->mapFromItem(item2, localPosOutside).toPoint();

    if (editTriggers & QQuickTableView::SingleTapped) {
        // edit cell 1
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QCOMPARE(tableView->selectionModel()->currentIndex(), index1);
        const auto editItem1 = tableView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem1);
        QVERIFY(editItem1->hasActiveFocus());
        QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index1);

        // edit cell 2 (without closing the previous edit session first)
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
        QCOMPARE(tableView->selectionModel()->currentIndex(), index2);
        const auto editItem2 = tableView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem2);
        QVERIFY(editItem2->hasActiveFocus());
        QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index2);

        // single tap outside content item should close the editor
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapOutsideContentItem);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QCOMPARE(tableView->selectionModel()->currentIndex(), index2);
    }

    if (editTriggers & QQuickTableView::DoubleTapped) {
        // edit cell 1
        QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QCOMPARE(tableView->selectionModel()->currentIndex(), index1);
        const auto editItem1 = tableView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem1);
        QVERIFY(editItem1->hasActiveFocus());
        QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index1);

        // edit cell 2 (without closing the previous edit session first)
        QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
        QCOMPARE(tableView->selectionModel()->currentIndex(), index2);
        const auto editItem2 = tableView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem2);
        QVERIFY(editItem2->hasActiveFocus());
        QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index2);

        // single tap outside the edit item should close the editor
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QCOMPARE(tableView->selectionModel()->currentIndex(), index1);

        if (!(editTriggers & QQuickTableView::SingleTapped)) {
            // single tap on a cell should not open the editor
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
            QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
            QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        }

        // single tap outside content item should make sure editing ends
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapOutsideContentItem);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTableView::SelectedTapped) {
        // select cell first, then tap on it
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::Select);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QCOMPARE(tableView->selectionModel()->currentIndex(), index1);
        const auto editItem1 = tableView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem1);
        QVERIFY(editItem1->hasActiveFocus());
        QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index1);

        // tap on a non-selected cell. This should close the editor, and move
        // the current index, but not begin to edit the cell.
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
        QCOMPARE(tableView->selectionModel()->currentIndex(), index2);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());

        // tap on a non-selected cell while no editor is active
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QCOMPARE(tableView->selectionModel()->currentIndex(), index2);

        // tap on the current cell. This alone should not start an edit (unless it's also selected)
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTableView::EditKeyPressed) {
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_Return);
        const auto editItem1 = tableView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem1);
        QVERIFY(editItem1->hasActiveFocus());
        QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index1);

        // Pressing escape should close the editor
        QTest::keyClick(window, Qt::Key_Escape);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QCOMPARE(tableView->selectionModel()->currentIndex(), index1);

        // Pressing Enter to open the editor again
        QTest::keyClick(window, Qt::Key_Enter);
        const auto editItem2 = tableView->property(kEditItem).value<QQuickItem *>();
        QVERIFY(editItem2);
        QVERIFY(editItem2->hasActiveFocus());
        QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index1);

        // single tap outside the edit item should close the editor
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTableView::AnyKeyPressed) {
        // Pressing key x should start to edit. And in case of AnyKeyPressed, we
        // also replay the key event to the focus object.
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_X);
        QCOMPARE(tableView->selectionModel()->currentIndex(), index1);
        QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index1);
        auto textInput1 = tableView->property(kEditItem).value<QQuickTextInput *>();
        QVERIFY(textInput1);
        QVERIFY(textInput1->hasActiveFocus());
        QCOMPARE(textInput1->text(), "x");

        // Pressing escape should close the editor
        QTest::keyClick(window, Qt::Key_Escape);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QCOMPARE(tableView->selectionModel()->currentIndex(), index1);

        // Pressing a modifier key alone should not open the editor
        QTest::keyClick(window, Qt::Key_Shift);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QTest::keyClick(window, Qt::Key_Control);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QTest::keyClick(window, Qt::Key_Alt);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QTest::keyClick(window, Qt::Key_Meta);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());

        // Pressing enter should also start to edit. But this is a
        // special case, we don't replay enter into the focus object.
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_Enter);
        QCOMPARE(tableView->selectionModel()->currentIndex(), index1);
        QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index1);
        auto textInput2 = tableView->property(kEditItem).value<QQuickTextInput *>();
        QVERIFY(textInput2);
        QVERIFY(textInput2->hasActiveFocus());
        QCOMPARE(textInput2->text(), "1");

        if (!(editTriggers & QQuickTableView::SingleTapped)) {
            // single tap outside the edit item should close the editor
            QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos2);
            QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
            QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        }

        // single tap outside content item should make sure editing ends
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapOutsideContentItem);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers == QQuickTableView::NoEditTriggers) {
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, tapPos1);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_Return);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_Enter);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_X);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }
}

void tst_QQuickTableView::editUsingTab()
{
    // Check that the you can commit and start to edit
    // the next cell by pressing tab and backtab.
    LOAD_TABLEVIEW("editdelegate.qml");

    auto model = TestModel(4, 4);
    tableView->setModel(QVariant::fromValue(&model));
    tableView->forceActiveFocus();

    const char kEditItem[] = "editItem";
    const char kEditIndex[] = "editIndex";

    WAIT_UNTIL_POLISHED;

    const QPoint cell1(1, 1);
    const QPoint cell2(2, 1);
    const QModelIndex index1 = tableView->modelIndex(cell1);
    const QModelIndex index2 = tableView->modelIndex(cell2);

    QQuickWindow *window = tableView->window();

    // Edit cell 1
    tableView->edit(index1);
    QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index1);
    const QQuickItem *editItem1 = tableView->property(kEditItem).value<QQuickItem *>();
    QVERIFY(editItem1);

    // Press Tab to edit cell 2
    QTest::keyClick(window, Qt::Key_Tab);
    QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index2);
    const QQuickItem *editItem2 = tableView->property(kEditItem).value<QQuickItem *>();
    QVERIFY(editItem2);

    // Press Backtab to edit cell 1
    QTest::keyClick(window, Qt::Key_Backtab);
    QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index1);
    const QQuickItem *editItem3 = tableView->property(kEditItem).value<QQuickItem *>();
    QVERIFY(editItem3);
}

void tst_QQuickTableView::editDelegateComboBox()
{
    // Using a ComboBox as an edit delegate should be a quite common
    // use case. So test that it works.
    LOAD_TABLEVIEW("editdelegate_combobox.qml");

    auto model = TestModel(4, 4);
    tableView->setModel(QVariant::fromValue(&model));
    tableView->forceActiveFocus();

    const char kEditItem[] = "editItem";
    const char kEditIndex[] = "editIndex";
    const char kCommitCount[] = "commitCount";
    const char kComboFocusCount[] = "comboFocusCount";

    WAIT_UNTIL_POLISHED;

    const QPoint cell1(1, 1);
    const QPoint cell2(2, 1);
    const QModelIndex index1 = tableView->modelIndex(cell1);
    const QModelIndex index2 = tableView->modelIndex(cell2);

    QQuickWindow *window = tableView->window();

    // Edit cell 1
    tableView->edit(index1);
    QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index1);
    const QQuickItem *editItem1 = tableView->property(kEditItem).value<QQuickItem *>();
    QVERIFY(editItem1);
    QCOMPARE(tableView->property(kComboFocusCount).value<int>(), 1);

    // Press Tab to edit cell 2
    QTest::keyClick(window, Qt::Key_Tab);
    QCOMPARE(tableView->property(kCommitCount).value<int>(), 1);
    QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index2);
    const QQuickItem *editItem2 = tableView->property(kEditItem).value<QQuickItem *>();
    QVERIFY(editItem2);
    QCOMPARE(tableView->property(kComboFocusCount).value<int>(), 2);

    // Press Enter to commit
    QTest::keyClick(window, Qt::Key_Enter);
    QCOMPARE(tableView->property(kCommitCount).value<int>(), 2);
    QCOMPARE(tableView->property(kComboFocusCount).value<int>(), 2);
    QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());

    // Edit cell 1
    tableView->edit(index1);
    // Press escape to close editor
    QTest::keyClick(window, Qt::Key_Escape);
    QCOMPARE(tableView->property(kCommitCount).value<int>(), 2);
    QCOMPARE(tableView->property(kComboFocusCount).value<int>(), 3);
    QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());

    // Edit cell 2
    tableView->edit(index2);
    // Press space to open combo menu
    QTest::keyClick(window, Qt::Key_Space);
    // Press Enter to commit and close the editor
    QTest::keyClick(window, Qt::Key_Enter);
    QCOMPARE(tableView->property(kCommitCount).value<int>(), 3);
    QCOMPARE(tableView->property(kComboFocusCount).value<int>(), 4);
    QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
}

void tst_QQuickTableView::editOnNonEditableCell_data()
{
    QTest::addColumn<QQuickTableView::EditTriggers>("editTriggers");

    QTest::newRow("SingleTapped") << QQuickTableView::EditTriggers(QQuickTableView::SingleTapped);
    QTest::newRow("DoubleTapped") << QQuickTableView::EditTriggers(QQuickTableView::DoubleTapped);
    QTest::newRow("SelectedTapped") << QQuickTableView::EditTriggers(QQuickTableView::SelectedTapped);
    QTest::newRow("EditKeyPressed") << QQuickTableView::EditTriggers(QQuickTableView::EditKeyPressed);
    QTest::newRow("AnyKeyPressed") << QQuickTableView::EditTriggers(QQuickTableView::EditKeyPressed);
}

void tst_QQuickTableView::editOnNonEditableCell()
{
    // Check that the user cannot edit a non-editable cell from the edit triggers.
    // Note: we don't want TableView to print out warnings in this case, since
    // the user is not doing anything wrong. We only want to print out warnings if
    // the application is calling edit() explicitly on a cell that cannot be edited
    // (separate test below).
    QFETCH(QQuickTableView::EditTriggers, editTriggers);
    LOAD_TABLEVIEW("editdelegate.qml");

    auto model = TestModel(4, 4);
    // set flags that exclude Qt::ItemIsEditable
    model.setFlags(Qt::ItemIsEnabled);
    tableView->setModel(QVariant::fromValue(&model));
    tableView->setEditTriggers(editTriggers);
    tableView->forceActiveFocus();

    const char kEditItem[] = "editItem";
    const char kEditIndex[] = "editIndex";

    WAIT_UNTIL_POLISHED;

    const QPoint cell(1, 1);
    const QModelIndex index1 = tableView->modelIndex(cell);
    const auto item = tableView->itemAtCell(cell);
    QVERIFY(item);

    QQuickWindow *window = tableView->window();

    const QPoint localPos = QPoint(item->width() - 1, item->height() - 1);
    const QPoint tapPos = window->contentItem()->mapFromItem(item, localPos).toPoint();

    if (editTriggers & QQuickTableView::SingleTapped) {
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTableView::DoubleTapped) {
        QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTableView::SelectedTapped) {
        // select cell first, then tap on it
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTableView::EditKeyPressed) {
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_Enter);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_Return);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTableView::AnyKeyPressed) {
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_X);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_Enter);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }
}

void tst_QQuickTableView::noEditDelegate_data()
{
    QTest::addColumn<QQuickTableView::EditTriggers>("editTriggers");

    QTest::newRow("NoEditTriggers") << QQuickTableView::EditTriggers(QQuickTableView::NoEditTriggers);
    QTest::newRow("SingleTapped") << QQuickTableView::EditTriggers(QQuickTableView::SingleTapped);
    QTest::newRow("DoubleTapped") << QQuickTableView::EditTriggers(QQuickTableView::DoubleTapped);
    QTest::newRow("SelectedTapped") << QQuickTableView::EditTriggers(QQuickTableView::SelectedTapped);
    QTest::newRow("EditKeyPressed") << QQuickTableView::EditTriggers(QQuickTableView::EditKeyPressed);
    QTest::newRow("AnyKeyPressed") << QQuickTableView::EditTriggers(QQuickTableView::EditKeyPressed);
}

void tst_QQuickTableView::noEditDelegate()
{
    // Check that you cannot start to edit if
    // no edit delegate has been set.
    QFETCH(QQuickTableView::EditTriggers, editTriggers);
    LOAD_TABLEVIEW("tableviewwithselected2.qml");

    auto model = TestModel(4, 4);
    tableView->setModel(QVariant::fromValue(&model));
    tableView->setEditTriggers(editTriggers);
    tableView->forceActiveFocus();

    const char kEditItem[] = "editItem";
    const char kEditIndex[] = "editIndex";

    WAIT_UNTIL_POLISHED;

    QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
    QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());

    const QPoint cell(1, 1);
    const QModelIndex index1 = tableView->modelIndex(cell);
    const auto item = tableView->itemAtCell(cell);
    QVERIFY(item);

    QQuickWindow *window = tableView->window();

    const QPoint localPos = QPoint(item->width() - 1, item->height() - 1);
    const QPoint tapPos = window->contentItem()->mapFromItem(item, localPos).toPoint();

    if (editTriggers & QQuickTableView::SingleTapped) {
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTableView::DoubleTapped) {
        QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTableView::SelectedTapped) {
        // select cell first, then tap on it
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTableView::EditKeyPressed) {
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_Enter);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_Return);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers & QQuickTableView::AnyKeyPressed) {
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_X);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_Enter);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }

    if (editTriggers == QQuickTableView::NoEditTriggers) {
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::mouseDClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        tableView->selectionModel()->setCurrentIndex(index1, QItemSelectionModel::NoUpdate);
        QTest::keyClick(window, Qt::Key_Return);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_Enter);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
        QTest::keyClick(window, Qt::Key_X);
        QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
        QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    }
}

void tst_QQuickTableView::editAndCloseEditor()
{
    // Check that the application can call edit() and closeEditor()
    LOAD_TABLEVIEW("editdelegate.qml");

    auto model = TestModel(4, 4);
    tableView->setModel(QVariant::fromValue(&model));
    tableView->forceActiveFocus();

    const char kEditItem[] = "editItem";
    const char kEditIndex[] = "editIndex";

    WAIT_UNTIL_POLISHED;

    const QPoint cell1(1, 1);
    const QPoint cell2(2, 2);
    const QModelIndex index1 = tableView->modelIndex(cell1);
    const QModelIndex index2 = tableView->modelIndex(cell2);

    const auto cellItem1 = tableView->itemAtCell(tableView->cellAtIndex(index1));
    const auto cellItem2 = tableView->itemAtCell(tableView->cellAtIndex(index2));
    QVERIFY(cellItem1);
    QVERIFY(cellItem2);
    QCOMPARE(cellItem1->property("editing").toBool(), false);
    QCOMPARE(cellItem2->property("editing").toBool(), false);

    // Edit cell 1
    tableView->edit(index1);
    QCOMPARE(tableView->selectionModel()->currentIndex(), index1);
    const QQuickItem *editItem1 = tableView->property(kEditItem).value<QQuickItem *>();
    QVERIFY(editItem1);
    QVERIFY(editItem1->hasActiveFocus());
    QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index1);
    QCOMPARE(editItem1->parentItem(), cellItem1);
    QCOMPARE(editItem1->property("editing").toBool(), true);
    QCOMPARE(cellItem1->property("editing").toBool(), true);

    // Edit cell 2
    tableView->edit(index2);
    QCOMPARE(tableView->selectionModel()->currentIndex(), index2);
    const QQuickItem *editItem2 = tableView->property(kEditItem).value<QQuickItem *>();
    QVERIFY(editItem2);
    QVERIFY(editItem2->hasActiveFocus());
    QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index2);
    QCOMPARE(editItem2->parentItem(), cellItem2);
    QCOMPARE(editItem2->property("editing").toBool(), true);
    QCOMPARE(cellItem2->property("editing").toBool(), true);
    QCOMPARE(cellItem1->property("editing").toBool(), false);

    // Close the editor
    tableView->closeEditor();
    QCOMPARE(tableView->selectionModel()->currentIndex(), index2);
    QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
    QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    QCOMPARE(cellItem2->property("editing").toBool(), false);
}

void tst_QQuickTableView::editWarning_noEditDelegate()
{
    // Check that the TableView will print out a warning if the
    // application calls edit() on a cell that has no editDelegate.
    LOAD_TABLEVIEW("tableviewwithselected2.qml");

    auto model = TestModel(4, 4);
    tableView->setModel(QVariant::fromValue(&model));

    WAIT_UNTIL_POLISHED;

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*cannot edit: no TableView.editDelegate set!"));
    tableView->edit(tableView->index(1, 1));
}

void tst_QQuickTableView::editWarning_invalidIndex()
{
    // Check that the TableView will print out a warning if the
    // application calls edit() on an invalid index.
    LOAD_TABLEVIEW("editdelegate.qml");

    auto model = TestModel(4, 4);
    tableView->setModel(QVariant::fromValue(&model));

    WAIT_UNTIL_POLISHED;

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*cannot edit: index is not valid!"));
    tableView->edit(tableView->index(-1, -1));
}

void tst_QQuickTableView::editWarning_nonEditableModelItem()
{
    // Check that the TableView will print out a warning if the
    // application calls edit() on cell that cannot, according
    // to the model flags, be edited.
    LOAD_TABLEVIEW("editdelegate.qml");

    auto model = TestModel(4, 4);
    tableView->setModel(QVariant::fromValue(&model));
    // set flags that exclude Qt::ItemIsEditable
    model.setFlags(Qt::ItemIsEnabled);

    WAIT_UNTIL_POLISHED;

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(".*cannot edit:.*flags.*Qt::ItemIsEditable"));
    tableView->edit(tableView->index(1, 1));
}

void tst_QQuickTableView::attachedPropertiesOnEditDelegate()
{
    // Check that the TableView.commit signal is emitted when
    // the user presses enter or return, but not when e.g pressing escape.
    // Also check that TableView.view is correct.
    LOAD_TABLEVIEW("editdelegate.qml");

    auto model = TestModel(4, 4);
    tableView->setModel(QVariant::fromValue(&model));
    tableView->forceActiveFocus();

    const char kEditItem[] = "editItem";
    const char kEditIndex[] = "editIndex";

    WAIT_UNTIL_POLISHED;

    const QPoint cell(1, 1);
    const QModelIndex index = tableView->modelIndex(cell);
    QQuickWindow *window = tableView->window();

    // Open the edit
    tableView->edit(index);
    QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index);
    QQuickItem *editItem1 = tableView->property(kEditItem).value<QQuickItem *>();
    QVERIFY(editItem1);
    const auto attached1 = getAttachedObject(editItem1);
    QVERIFY(attached1);
    QSignalSpy commitSpy1(attached1, &QQuickTableViewAttached::commit);

    // Check that TableView has been assigned to TableView.view
    QCOMPARE(attached1->view(), tableView);

    // Accept and close the edit, check commit signal
    QTest::keyClick(window, Qt::Key_Enter);
    QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
    QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    QCOMPARE(commitSpy1.count(), 1);

    // Repeat once more, but use Key_Return to accept instead
    tableView->edit(index);
    QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index);
    QQuickItem *editItem2 = tableView->property(kEditItem).value<QQuickItem *>();
    QVERIFY(editItem2);
    const auto attached2 = getAttachedObject(editItem2);
    QVERIFY(attached2);
    QSignalSpy commitSpy2(attached2, &QQuickTableViewAttached::commit);

    QTest::keyClick(window, Qt::Key_Return);
    QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
    QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    QCOMPARE(commitSpy1.count(), 1);
    QCOMPARE(commitSpy2.count(), 1);

    // Repeat once more, but use Key_Escape instead.
    // This should close the edit, but without an accepted signal.
    tableView->edit(index);
    QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index);
    QQuickItem *editItem3 = tableView->property(kEditItem).value<QQuickItem *>();
    QVERIFY(editItem3);
    const auto attached3 = getAttachedObject(editItem3);
    QVERIFY(editItem3);
    QSignalSpy commitSpy3(attached3, &QQuickTableViewAttached::commit);

    QTest::keyClick(window, Qt::Key_Escape);
    QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
    QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    QCOMPARE(commitSpy3.count(), 0);

    // Repeat once more, but tap outside the edit item.
    // This should close the edit, but without an accepted signal.
    tableView->edit(index);
    QCOMPARE(tableView->property(kEditIndex).value<QModelIndex>(), index);
    QQuickItem *editItem4 = tableView->property(kEditItem).value<QQuickItem *>();
    QVERIFY(editItem4);
    const auto attached4 = getAttachedObject(editItem4);
    QVERIFY(editItem4);
    QSignalSpy commitSpy4(attached4, &QQuickTableViewAttached::commit);

    const QPoint tapPos = window->contentItem()->mapFromItem(editItem4, QPointF(-10, -10)).toPoint();
    QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, tapPos);
    QVERIFY(!tableView->property(kEditItem).value<QQuickItem *>());
    QVERIFY(!tableView->property(kEditIndex).value<QModelIndex>().isValid());
    QCOMPARE(commitSpy4.count(), 0);
}

void tst_QQuickTableView::requiredPropertiesOnEditDelegate()
{
    // Check that all expected required properties on the edit
    // delegate (like row, column, current) has correct values.
    LOAD_TABLEVIEW("editdelegate.qml");

    TestModel model(4, 4);
    QItemSelectionModel selectionModel(&model);

    tableView->setModel(QVariant::fromValue(&model));
    tableView->setSelectionModel(&selectionModel);

    const char kEditItem[] = "editItem";

    WAIT_UNTIL_POLISHED;

    const QPoint cell(1, 1);
    const QModelIndex index1 = tableView->modelIndex(cell);
    const QModelIndex index2 = tableView->index(2, 2);

    tableView->edit(index1);

    auto textInput = tableView->property(kEditItem).value<QQuickTextInput *>();
    QVERIFY(textInput);
    // Check that "text: display" in the edit delegate works
    QCOMPARE(textInput->text(), "1");

    QCOMPARE(textInput->property("current").toBool(), true);
    QCOMPARE(textInput->property("selected").toBool(), false);
    QCOMPARE(textInput->property("editing").toBool(), true);
    selectionModel.select(index1, QItemSelectionModel::Select);
    QCOMPARE(textInput->property("selected").toBool(), true);
    selectionModel.setCurrentIndex(index2, QItemSelectionModel::Select);
    QCOMPARE(textInput->property("current").toBool(), false);
}

void tst_QQuickTableView::resettingRolesRespected()
{
    LOAD_TABLEVIEW("resetModelData.qml");

    TestModel model(1, 1);
    tableView->setModel(QVariant::fromValue(&model));

    WAIT_UNTIL_POLISHED;

    QVERIFY(!tableView->property("success").toBool());
    model.useCustomRoleNames(true);
    QTRY_VERIFY(tableView->property("success").toBool());
}

void tst_QQuickTableView::checkScroll_data()
{
    QTest::addColumn<bool>("resizableColumns");
    QTest::addColumn<bool>("resizableRows");
    QTest::newRow("T, T") << true << true;
    QTest::newRow("T, F") << true << false;
    QTest::newRow("F, T") << false << true;
    QTest::newRow("F, F") << false << false;
}

/*!
    Make sure that the TableView is scrollable regardless
    of the values of resizableColumns and resizableRows.
*/
void tst_QQuickTableView::checkScroll() // QTBUG-116566
{
    QFETCH(bool, resizableColumns);
    QFETCH(bool, resizableRows);
    LOAD_TABLEVIEW("plaintableview.qml"); // gives us 'tableView' variable

    tableView->setResizableColumns(resizableColumns);
    tableView->setResizableRows(resizableRows);

    auto model = TestModelAsVariant(20, 10);
    tableView->setModel(model);

    WAIT_UNTIL_POLISHED;

    // Scroll with the mouse wheel
    sendWheelEvent(view, {10, 10}, {0, -120}, {0, -120});

    // Check that scrolling succeeded
    QTRY_COMPARE_GT(tableView->contentY(), 20);
}

void tst_QQuickTableView::checkRebuildJsModel()
{
    LOAD_TABLEVIEW("resetJsModelData.qml"); // gives us 'tableView' variable

    // Generate javascript model
    const int size = 5;
    const char* modelUpdated = "modelUpdated";

    QJSEngine jsEngine;
    QJSValue jsArray;
    jsArray = jsEngine.newArray(size);
    for (int i = 0; i < size; ++i)
        jsArray.setProperty(i, QRandomGenerator::global()->generate());

    QVariant jsModel = QVariant::fromValue(jsArray);
    tableView->setModel(jsModel);
    WAIT_UNTIL_POLISHED;

    // Model change would be triggered for the first time
    QCOMPARE(tableView->property(modelUpdated).toInt(), 1);

    // Set the same model once again and check if model changes
    tableView->setModel(jsModel);
    QCOMPARE(tableView->property(modelUpdated).toInt(), 1);
}

QTEST_MAIN(tst_QQuickTableView)

#include "tst_qquicktableview.moc"
