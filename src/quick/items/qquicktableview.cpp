// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktableview_p.h"
#include "qquicktableview_p_p.h"

#include <QtCore/qtimer.h>
#include <QtCore/qdir.h>
#include <QtQmlModels/private/qqmldelegatemodel_p.h>
#include <QtQmlModels/private/qqmldelegatemodel_p_p.h>
#include <QtQml/private/qqmlincubator_p.h>
#include <QtQmlModels/private/qqmlchangeset_p.h>
#include <QtQml/qqmlinfo.h>

#include <QtQuick/private/qquickflickable_p_p.h>
#include <QtQuick/private/qquickitemviewfxitem_p_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>

/*!
    \qmltype TableView
    \inqmlmodule QtQuick
    \since 5.12
    \ingroup qtquick-views
    \inherits Flickable
    \brief Provides a table view of items to display data from a model.

    A TableView has a \l model that defines the data to be displayed, and a
    \l delegate that defines how the data should be displayed.

    TableView inherits \l Flickable. This means that while the model can have
    any number of rows and columns, only a subsection of the table is usually
    visible inside the viewport. As soon as you flick, new rows and columns
    enter the viewport, while old ones exit and are removed from the viewport.
    The rows and columns that move out are reused for building the rows and columns
    that move into the viewport. As such, the TableView support models of any
    size without affecting performance.

    A TableView displays data from models created from built-in QML types
    such as ListModel and XmlListModel, which populates the first column only
    in a TableView. To create models with multiple columns, either use
    \l TableModel or a C++ model that inherits QAbstractItemModel.

    A TableView does not include headers by default. You can add headers
    using the \l HorizontalHeaderView and \l VerticalHeaderView from
    Qt Quick Controls.

    \note TableView will only \l {isRowLoaded()}{load} as many delegate items as
    needed to fill up the view. There is no guarantee that items outside the view
    will be loaded, although TableView will sometimes pre-load items for
    optimization reasons. Hence, a TableView with zero width or height might not
    load any delegate items at all.

    \section1 Example Usage

    \section2 C++ Models

    The following example shows how to create a model from C++ with multiple
    columns:

    \snippet qml/tableview/cpp-tablemodel.h 0

    And then how to use it from QML:

    \snippet qml/tableview/cpp-tablemodel.qml 0

    \section2 QML Models

    For prototyping and displaying very simple data (from a web API, for
    example), \l TableModel can be used:

    \snippet qml/tableview/qml-tablemodel.qml 0

    \section1 Reusing items

    TableView recycles delegate items by default, instead of instantiating from
    the \l delegate whenever new rows and columns are flicked into view. This
    approach gives a huge performance boost, depending on the complexity of the
    delegate.

    When an item is flicked out, it moves to the \e{reuse pool}, which is an
    internal cache of unused items. When this happens, the \l TableView::pooled
    signal is emitted to inform the item about it. Likewise, when the item is
    moved back from the pool, the \l TableView::reused signal is emitted.

    Any item properties that come from the model are updated when the
    item is reused. This includes \c index, \c row, and \c column, but also
    any model roles.

    \note Avoid storing any state inside a delegate. If you do, reset it
    manually on receiving the \l TableView::reused signal.

    If an item has timers or animations, consider pausing them on receiving
    the \l TableView::pooled signal. That way you avoid using the CPU resources
    for items that are not visible. Likewise, if an item has resources that
    cannot be reused, they could be freed up.

    If you don't want to reuse items or if the \l delegate cannot support it,
    you can set the \l reuseItems property to \c false.

    \note While an item is in the pool, it might still be alive and respond
    to connected signals and bindings.

    The following example shows a delegate that animates a spinning rectangle. When
    it is pooled, the animation is temporarily paused:

    \snippet qml/tableview/reusabledelegate.qml 0

    \section1 Row heights and column widths

    When a new column is flicked into view, TableView will determine its width
    by calling the \l columnWidthProvider. If set, this function will alone decide
    the width of the column. Otherwise, it will check if an explicit width has
    been set with \l setColumnWidth(). If not, \l implicitColumnWidth() will be used.
    The implicit width of a column is the same as the largest
    \l {Item::implicitWidth}{implicit width} found among the currently loaded
    delegate items in that column. Trying to set an explicit \c width directly on
    a delegate has no effect, and will be ignored and overwritten. The same logic also
    applies to row heights.

    An implementation of a columnWidthProvider that is equivalent to the default
    logic would be:

    \code
    columnWidthProvider: function(column) {
        let w = explicitColumnWidth(column)
        if (w >= 0)
            return w;
        return implicitColumnWidth(column)
    }
    \endcode

    Once the column width is resolved, all other items in the same column are resized
    to this width, including any items that are flicked into the view at a later point.

    \note The resolved width of a column is discarded when the whole column is flicked out
    of the view, and is recalculated again if it's flicked back in. This means that if the
    width depends on the \l implicitColumnWidth(), the calculation can be different each time,
    depending on which row you're at when the column enters (since \l implicitColumnWidth()
    only considers the delegate items that are currently \l {isColumnLoaded()}{loaded}).
    To avoid this, you should use a \l columnWidthProvider, or ensure that all the delegate
    items in the same column have the same \c implicitWidth.

    If you change the values that a \l rowHeightProvider or a
    \l columnWidthProvider return for rows and columns inside the viewport, you
    must call \l forceLayout. This informs TableView that it needs to use the
    provider functions again to recalculate and update the layout.

    Since Qt 5.13, if you want to hide a specific column, you can return \c 0
    from the \l columnWidthProvider for that column. Likewise, you can return 0
    from the \l rowHeightProvider to hide a row. If you return a negative
    number, TableView will fall back to calculate the size based on the delegate
    items.

    \note The size of a row or column should be a whole number to avoid
    sub-pixel alignment of items.

    The following example shows how to set a simple \c columnWidthProvider
    together with a timer that modifies the values the function returns. When
    the array is modified, \l forceLayout is called to let the changes
    take effect:

    \snippet qml/tableview/tableviewwithprovider.qml 0

    \section1 Editing cells

    You can let the user edit table cells by providing an edit delegate. The
    edit delegate will be instantiated according to the \l editTriggers, which
    by default is when the user double taps on a cell, or presses e.g
    \l Qt::Key_Enter or \l Qt::Key_Return. The edit delegate is set using
    \l {TableView::editDelegate}, which is an attached property that you set
    on the \l delegate. The following snippet shows how to do that:

    \snippet qml/tableview/editdelegate.qml 0

    If the user presses Qt::Key_Enter or Qt::Key_Return while the edit delegate
    is active, TableView will emit the \l TableView::commit signal to the edit
    delegate, so that it can write back the changed data to the model.

    \note In order for a cell to be editable, the model needs to override
    \l QAbstractItemModel::flags(), and return \c Qt::ItemIsEditable.
    This flag is not enabled in QAbstractItemModel by default.
    The override could for example look like this:

    \code
    Qt::ItemFlags QAbstractItemModelSubClass::flags(const QModelIndex &index) const override
    {
        Q_UNUSED(index)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }
    \endcode

    If the \l {TableView::delegate}{TableView delegate} has a property
    \c {required property bool editing} defined, it will be set to \c true
    for the delegate being edited. See the documentation for
    \l editDelegate for an example on how to use it.

    \sa TableView::editDelegate, TableView::commit, editTriggers, edit(), closeEditor()

    \section1 Overlays and underlays

    All new items that are instantiated from the delegate are parented to the
    \l{Flickable::}{contentItem} with the \c z value, \c 1. You can add your
    own items inside the Tableview, as child items of the Flickable. By
    controlling their \c z value, you can make them be on top of or
    underneath the table items.

    Here is an example that shows how to add some text on top of the table, that
    moves together with the table as you flick:

    \snippet qml/tableview/tableviewwithheader.qml 0

    Here is another example that shows how to create an overlay item that
    stays on top of a particular cell. This requires a bit more code, since
    the location of a cell will \l {layoutChanged}{change} if the user, for
    example, is resizing a column in front of it.

    \snippet qml/tableview/overlay.qml 0

    You could also parent the overlay directly to the cell instead of the
    \l {Flickable::}{contentItem}. But doing so will be fragile since the cell is unloaded
    or reused whenever it's flicked out of the viewport.

    \sa layoutChanged()

    \section1 Selecting items

    You can add selection support to TableView by assigning an \l ItemSelectionModel to
    the \l selectionModel property. It will then use this model to control which
    delegate items should be shown as selected, and which item should be shown as
    current. You can set \l selectionBehavior to control if the user should
    be allowed to select individual cells, rows, or columns.

    To find out whether a delegate is selected or current, declare the
    following properties:

    \code
    delegate: Item {
        required property bool selected
        required property bool current
        // ...
    }
    \endcode

    \note the \c selected and \c current properties must be defined as \c required.
    This will inform TableView that it should take responsibility for updating their
    values. If not, they will simply be ignored. See also \l {Required Properties}.

    The following snippet shows how an application can render the delegate differently
    depending on the \c selected property:

    \snippet qml/tableview/selectionmodel.qml 0

    The \l currentRow and \l currentColumn properties can also be useful if you need
    to render a delegate differently depending on if it lies on the same row or column
    as the current item.

    \note \l{Qt Quick Controls} offers a SelectionRectangle that can be used
    to let the user select cells.

    \note By default, a cell will become
    \l {ItemSelectionModel::currentIndex}{current}, and any selections will
    be removed, when the user taps on it. If such default tap behavior is not wanted
    (e.g if you use custom pointer handlers inside your delegate), you can set
    \l pointerNavigationEnabled to \c false.

    \section1 Keyboard navigation

    In order to support keyboard navigation, you need to assign an \l ItemSelectionModel
    to the \l selectionModel property. TableView will then use this model to manipulate
    the model's \l {ItemSelectionModel::currentIndex}{currentIndex}.

    It's the responsibility of the delegate to render itself as
    \l {ItemSelectionModel::currentIndex}{current}. You can do this by adding a
    property \c {required property bool current} to it, and let the appearance
    depend on its state. The \c current property's value is set by the TableView.
    You can also disable keyboard navigation fully (in case you want to implement your
    own key handlers) by setting \l keyNavigationEnabled to \c false.

    The following example demonstrates how you can use keyboard navigation together
    with \c current and \c selected properties:

    \snippet qml/tableview/keyboard-navigation.qml 0

    \section1 Copy and paste

    Implementing copy and paste operations for a TableView usually also includes using
    a QUndoStack (or some other undo/redo framework). The QUndoStack can be used to
    store the different operations done on the model, like adding or removing rows, or
    pasting data from the clipboard, with a way to undo it again later. However, an
    accompanying QUndoStack that describes the possible operations, and how to undo them,
    should be designed according to the needs of the model and the application.
    As such, TableView doesn't offer a built-in API for handling copy and paste.

    The following snippet can be used as a reference for how to add copy and paste support
    to your model and TableView. It uses the existing mime data API in QAbstractItemModel,
    together with QClipboard. The snippet will work as it is, but can also be extended to
    use a QUndoStack.

    \code
    // Inside your C++ QAbstractTableModel subclass:

    Q_INVOKABLE void copyToClipboard(const QModelIndexList &indexes) const
    {
        QGuiApplication::clipboard()->setMimeData(mimeData(indexes));
    }

    Q_INVOKABLE bool pasteFromClipboard(const QModelIndex &targetIndex)
    {
        const QMimeData *mimeData = QGuiApplication::clipboard()->mimeData();
        // Consider using a QUndoCommand for the following call. It should store
        // the (mime) data for the model items that are about to be overwritten, so
        // that a later call to undo can revert it.
        return dropMimeData(mimeData, Qt::CopyAction, -1, -1, targetIndex);
    }
    \endcode

    The two functions can, for example, be used from QML like this:

    \code
    TableView {
        id: tableView
        model: tableModel
        selectionModel: ItemSelectionModel {}

        Shortcut {
           sequence: StandardKey.Copy
           onActivated: {
               let indexes = tableView.selectionModel.selectedIndexes
               tableView.model.copyToClipboard(indexes)
           }
        }

        Shortcut {
           sequence: StandardKey.Paste
           onActivated: {
               let targetIndex = tableView.selectionModel.currentIndex
               tableView.model.pasteFromClipboard(targetIndex)
           }
        }
    }
    \endcode

    \sa QAbstractItemModel::mimeData(), QAbstractItemModel::dropMimeData(), QUndoStack, QUndoCommand, QClipboard
*/

/*!
    \qmlproperty int QtQuick::TableView::rows
    \readonly

    This property holds the number of rows in the table.

    \note \a rows is usually equal to the number of rows in the model, but can
    temporarily differ until all pending model changes have been processed.

    This property is read only.
*/

/*!
    \qmlproperty int QtQuick::TableView::columns
    \readonly

    This property holds the number of columns in the table.

    \note \a columns is usually equal to the number of columns in the model, but
    can temporarily differ until all pending model changes have been processed.

    If the model is a list, columns will be \c 1.

    This property is read only.
*/

/*!
    \qmlproperty real QtQuick::TableView::rowSpacing

    This property holds the spacing between the rows.

    The default value is \c 0.
*/

/*!
    \qmlproperty real QtQuick::TableView::columnSpacing

    This property holds the spacing between the columns.

    The default value is \c 0.
*/

/*!
    \qmlproperty var QtQuick::TableView::rowHeightProvider

    This property can hold a function that returns the row height for each row
    in the model. It is called whenever TableView needs to know the height of
    a specific row. The function takes one argument, \c row, for which the
    TableView needs to know the height.

    Since Qt 5.13, if you want to hide a specific row, you can return \c 0
    height for that row. If you return a negative number, TableView calculates
    the height based on the delegate items.

    \note The rowHeightProvider will usually be called two times when
    a row is about to load (or when doing layout). First, to know if
    the row is visible and should be loaded. And second, to determine
    the height of the row after all items have been loaded.
    If you need to calculate the row height based on the size of the delegate
    items, you need to wait for the second call, when all the items have been loaded.
    You can check for this by calling \l {isRowLoaded()}{isRowLoaded(row)},
    and simply return -1 if that is not yet the case.

    \sa rowHeightProvider, isRowLoaded(), {Row heights and column widths}
*/

/*!
    \qmlproperty var QtQuick::TableView::columnWidthProvider

    This property can hold a function that returns the column width for each
    column in the model. It is called whenever TableView needs to know the
    width of a specific column. The function takes one argument, \c column,
    for which the TableView needs to know the width.

    Since Qt 5.13, if you want to hide a specific column, you can return \c 0
    width for that column. If you return a negative number, TableView
    calculates the width based on the delegate items.

    \note The columnWidthProvider will usually be called two times when
    a column is about to load (or when doing layout). First, to know if
    the column is visible and should be loaded. And second, to determine
    the width of the column after all items have been loaded.
    If you need to calculate the column width based on the size of the delegate
    items, you need to wait for the second call, when all the items have been loaded.
    You can check for this by calling \l {isColumnLoaded}{isColumnLoaded(column)},
    and simply return -1 if that is not yet the case.

    \sa rowHeightProvider, isColumnLoaded(), {Row heights and column widths}
*/

/*!
    \qmlproperty model QtQuick::TableView::model
    This property holds the model that provides data for the table.

    The model provides the set of data that is used to create the items
    in the view. Models can be created directly in QML using \l TableModel,
    \l ListModel, \l ObjectModel, or provided by a custom
    C++ model class. The C++ model must be a subclass of \l QAbstractItemModel
    or a simple list.

    \sa {qml-data-models}{Data Models}
*/

/*!
    \qmlproperty Component QtQuick::TableView::delegate

    The delegate provides a template defining each cell item instantiated by the
    view. The model index is exposed as an accessible \c index property. The same
    applies to \c row and \c column. Properties of the model are also available
    depending upon the type of \l {qml-data-models}{Data Model}.

    A delegate should specify its size using \l{Item::}{implicitWidth} and
    \l {Item::}{implicitHeight}. The TableView lays out the items based on that
    information. Explicit width or height settings are ignored and overwritten.

    Inside the delegate, you can optionally add one or more of the following
    properties. TableView modifies the values of these properties to inform the
    delegate which state it's in. This can be used by the delegate to render
    itself differently according on its own state.

    \list
    \li required property bool current - \c true if the delegate is \l {Keyboard navigation}{current.}
    \li required property bool selected - \c true if the delegate is \l {Selecting items}{selected.}
    \li required property bool editing - \c true if the delegate is being \l {Editing cells}{edited.}
    \endlist

    The following example shows how to use these properties:
    \code
    delegate: Rectangle {
        required property bool current
        required property bool selected
        border.width: current ? 1 : 0
        color: selected ? palette.highlight : palette.base
    }
    \endcode

    \note Delegates are instantiated as needed and may be destroyed at any time.
    They are also reused if the \l reuseItems property is set to \c true. You
    should therefore avoid storing state information in the delegates.

    \sa {Row heights and column widths}, {Reusing items}, {Required Properties}
*/

/*!
    \qmlproperty bool QtQuick::TableView::reuseItems

    This property holds whether or not items instantiated from the \l delegate
    should be reused. If set to \c false, any currently pooled items
    are destroyed.

    \sa {Reusing items}, TableView::pooled, TableView::reused
*/

/*!
    \qmlproperty real QtQuick::TableView::contentWidth

    This property holds the table width required to accommodate the number of
    columns in the model. This is usually not the same as the \c width of the
    \l view, which means that the table's width could be larger or smaller than
    the viewport width. As a TableView cannot always know the exact width of
    the table without loading all columns in the model, the \c contentWidth is
    usually an estimate based on the initially loaded table.

    If you know what the width of the table will be, assign a value to
    \c contentWidth, to avoid unnecessary calculations and updates to the
    TableView.

    \sa contentHeight, columnWidthProvider
*/

/*!
    \qmlproperty real QtQuick::TableView::contentHeight

    This property holds the table height required to accommodate the number of
    rows in the data model. This is usually not the same as the \c height of the
    \c view, which means that the table's height could be larger or smaller than the
    viewport height. As a TableView cannot always know the exact height of the
    table without loading all rows in the model, the \c contentHeight is
    usually an estimate based on the initially loaded table.

    If you know what the height of the table will be, assign a
    value to \c contentHeight, to avoid unnecessary calculations and updates to
    the TableView.

    \sa contentWidth, rowHeightProvider
*/

/*!
    \qmlmethod QtQuick::TableView::forceLayout

    Responding to changes in the model are batched so that they are handled
    only once per frame. This means the TableView delays showing any changes
    while a script is being run. The same is also true when changing
    properties, such as \l rowSpacing or \l{Item::anchors.leftMargin}{leftMargin}.

    This method forces the TableView to immediately update the layout so
    that any recent changes take effect.

    Calling this function re-evaluates the size and position of each visible
    row and column. This is needed if the functions assigned to
    \l rowHeightProvider or \l columnWidthProvider return different values than
    what is already assigned.
*/

/*!
    \qmlproperty bool QtQuick::TableView::alternatingRows

    This property controls whether the background color of the rows should alternate.
    The default value is style dependent.

    \note This property is only a hint, and might therefore not be
    respected by custom delegates. It's recommended that a delegate alternates
    between \c palette.base and \c palette.alternateBase when this hint is
    \c true, so that the colors can be set from outside of the delegate.
    For example:

    \code
    background: Rectangle {
        color: control.row === control.tableView.currentRow
               ? control.palette.highlight
               : (control.tableView.alternatingRows && control.row % 2 !== 0
               ? control.palette.alternateBase
               : control.palette.base)
    }
    \endcode
*/

/*!
    \qmlproperty int QtQuick::TableView::leftColumn

    This property holds the leftmost column that is currently visible inside the view.

    \sa rightColumn, topRow, bottomRow
*/

/*!
    \qmlproperty int QtQuick::TableView::rightColumn

    This property holds the rightmost column that is currently visible inside the view.

    \sa leftColumn, topRow, bottomRow
*/

/*!
    \qmlproperty int QtQuick::TableView::topRow

    This property holds the topmost row that is currently visible inside the view.

    \sa leftColumn, rightColumn, bottomRow
*/

/*!
    \qmlproperty int QtQuick::TableView::bottomRow

    This property holds the bottom-most row that is currently visible inside the view.

    \sa leftColumn, rightColumn, topRow
*/

/*!
    \qmlproperty int QtQuick::TableView::currentColumn
    \readonly

    This read-only property holds the column in the view that contains the
    item that is \l {Keyboard navigation}{current.} If no item is current, it will be \c -1.

    \note In order for TableView to report what the current column is, you
    need to assign an \l ItemSelectionModel to \l selectionModel.

    \sa currentRow, selectionModel, {Selecting items}
*/

/*!
    \qmlproperty int QtQuick::TableView::currentRow
    \readonly

    This read-only property holds the row in the view that contains the item
    that is \l {Keyboard navigation}{current.} If no item is current, it will be \c -1.

    \note In order for TableView to report what the current row is, you
    need to assign an \l ItemSelectionModel to \l selectionModel.

    \sa currentColumn, selectionModel, {Selecting items}
*/

/*!
    \qmlproperty ItemSelectionModel QtQuick::TableView::selectionModel
    \since 6.2

    This property can be set to control which delegate items should be shown as
    selected, and which item should be shown as current. If the delegate has a
    \c {required property bool selected} defined, TableView will keep it in sync
    with the selection state of the corresponding model item in the selection model.
    If the delegate has a \c {required property bool current} defined, TableView will
    keep it in sync with selectionModel.currentIndex.

    \sa {Selecting items}, SelectionRectangle, keyNavigationEnabled, pointerNavigationEnabled
*/

/*!
    \qmlproperty bool QtQuick::TableView::animate
    \since 6.4

    This property can be set to control if TableView should animate the
    \l {Flickable::}{contentItem} (\l {Flickable::}{contentX} and
    \l {Flickable::}{contentY}). It is used by
    \l positionViewAtCell(), and when navigating
    \l {QItemSelectionModel::currentIndex}{the current index}
    with the keyboard. The default value is \c true.

    If set to \c false, any ongoing animation will immediately stop.

    \note This property is only a hint. TableView might choose to position
    the content item without an animation if, for example, the target cell is not
    \l {isRowLoaded()}{loaded}. However, if set to \c false, animations will
    always be off.

    \sa positionViewAtCell()
*/

/*!
    \qmlproperty bool QtQuick::TableView::keyNavigationEnabled
    \since 6.4

    This property can be set to control if the user should be able
    to change \l {QItemSelectionModel::currentIndex()}{the current index}
    using the keyboard. The default value is \c true.

    \note In order for TableView to support keyboard navigation, you
    need to assign an \l ItemSelectionModel to \l selectionModel.

    \sa {Keyboard navigation}, selectionModel, selectionBehavior
    \sa pointerNavigationEnabled, {Flickable::}{interactive}
*/

/*!
    \qmlproperty bool QtQuick::TableView::pointerNavigationEnabled
    \since 6.4

    This property can be set to control if the user should be able
    to change \l {QItemSelectionModel::currentIndex()}{the current index}
    using mouse or touch. The default value is \c true.

    \sa selectionModel, keyNavigationEnabled, {Flickable::}{interactive}
*/

/*!
    \qmlproperty enumeration QtQuick::TableView::selectionBehavior
    \since 6.4

    This property holds whether the user can select cells, rows or columns.

    \value TableView.SelectionDisabled
           The user cannot perform selections
    \value TableView.SelectCells
           (Default value) The user can select individual cells
    \value TableView.SelectRows
           The user can only select rows
    \value TableView.SelectColumns
           The user can only select columns

    \sa {Selecting items}, selectionMode, selectionModel, keyNavigationEnabled
*/

/*!
    \qmlproperty enumeration QtQuick::TableView::selectionMode
    \since 6.6

    If \l selectionBehavior is set to \c {TableView.SelectCells}, this property holds
    whether the user can select one cell at a time, or multiple cells.
    If \l selectionBehavior is set to \c {TableView.SelectRows}, this property holds
    whether the user can select one row at a time, or multiple rows.
    If \l selectionBehavior is set to \c {TableView.SelectColumns}, this property holds
    whether the user can select one column at a time, or multiple columns.

    The following modes are available:

    \value TableView.SingleSelection
           The user can select a single cell, row or column.
    \value TableView.ContiguousSelection
           The user can select a single contiguous block of cells.
           An existing selection can be made bigger or smaller by holding down
           the \c Shift modifier while selecting.
    \value TableView.ExtendedSelection
           (Default value) The user can select multiple individual blocks of
           cells. An existing selection can be made bigger or smaller by
           holding down the \c Shift modifier while selecting. A new selection
           block can be started without clearing the current selection by
           holding down the \c Control modifier while selecting.

    \sa {Selecting items}, selectionBehavior, selectionModel, keyNavigationEnabled
*/

/*!
    \qmlproperty bool QtQuick::TableView::resizableColumns
    \since 6.5

    This property holds whether the user is allowed to resize columns
    by dragging between the cells. The default value is \c false.
*/

/*!
    \qmlproperty bool QtQuick::TableView::resizableRows
    \since 6.5

    This property holds whether the user is allowed to resize rows
    by dragging between the cells. The default value is \c false.
*/

/*!
    \qmlproperty enumeration QtQuick::TableView::editTriggers
    \since 6.5

    This property holds the different ways the user can start to edit a cell.
    It can be a combination of the following values:

    \default TableView.DoubleTapped | TableView.EditKeyPressed.
    \value TableView.NoEditTriggers - the user cannot trigger editing of cells.
        When this value is set, TableView will neither \e {open or close}
        the edit delegate as a response to any user interaction.
        But the application can call \l edit() and \l closeEditor() manually.
    \value TableView.SingleTapped - the user can edit a cell by single tapping it.
    \value TableView.DoubleTapped - the user can edit a cell by double tapping it.
    \value TableView.SelectedTapped - the user can edit a
        \l {QItemSelectionModel::selectedIndexes()}{selected cell} by tapping it.
    \value TableView.EditKeyPressed - the user can edit the
        \l {ItemSelectionModel::currentIndex}{current cell} by pressing one
        of the edit keys. The edit keys are decided by the OS, but are normally
        \c Qt::Key_Enter and \c Qt::Key_Return.
    \value TableView.AnyKeyPressed - the user can edit the
        \l {ItemSelectionModel::currentIndex}{current cell} by pressing any key, other
        than the cell navigation keys. The pressed key is also sent to the
        focus object inside the \l {TableView::editDelegate}{edit delegate}.

    For \c TableView.SelectedTapped, \c TableView.EditKeyPressed, and
    \c TableView.AnyKeyPressed to have any effect, TableView needs to have a
    \l {selectionModel}{selection model} assigned, since they depend on a
    \l {ItemSelectionModel::currentIndex}{current index} being set. To be
    able to receive any key events at all, TableView will also need to have
    \l QQuickItem::activeFocus.

    When editing a cell, the user can press \c Qt::Key_Tab or \c Qt::Key_Backtab
    to \l {TableView::commit}{commit} the data, and move editing to the next
    cell. This behavior can be disabled by setting
    \l QQuickItem::activeFocusOnTab on TableView to \c false.

    \note In order for a cell to be editable, the \l delegate needs an
    \l {TableView::editDelegate}{edit delegate} attached, and the model
    needs to return \c Qt::ItemIsEditable from \l QAbstractItemModel::flags()
    (exemplified underneath).
    If you still cannot edit a cell after activating one of the specified
    triggers, you can, as a help, try to call \l edit() explicitly (e.g
    from a Button/TapHandler). Doing so will print out a warning explaining
    why the cell cannot be edited.

    \code
    Qt::ItemFlags QAbstractItemModelSubClass::flags(const QModelIndex &index) const override
    {
        Q_UNUSED(index)
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
    }
    \endcode

    \sa TableView::editDelegate, TableView::commit, {Editing cells}
*/

/*!
    \qmlmethod QtQuick::TableView::positionViewAtCell(point cell, PositionMode mode, point offset, rect subRect)

    Positions \l {Flickable::}{contentX} and \l {Flickable::}{contentY} such
    that \a cell is at the position specified by \a mode. \a mode
    can be an or-ed combination of the following:

    \value TableView.AlignLeft Position the cell at the left of the view.
    \value TableView.AlignHCenter Position the cell at the horizontal center of the view.
    \value TableView.AlignRight Position the cell at the right of the view.
    \value TableView.AlignTop Position the cell at the top of the view.
    \value TableView.AlignVCenter Position the cell at the vertical center of the view.
    \value TableView.AlignBottom Position the cell at the bottom of the view.
    \value TableView.AlignCenter The same as (TableView.AlignHCenter | TableView.AlignVCenter)
    \value TableView.Visible If any part of the cell is visible then take no action. Otherwise
           move the content item so that the entire cell becomes visible.
    \value TableView.Contain If the entire cell is visible then take no action. Otherwise
           move the content item so that the entire cell becomes visible. If the cell is
           bigger than the view, the top-left part of the cell will be preferred.

    If no vertical alignment is specified, vertical positioning will be ignored.
    The same is true for horizontal alignment.

    Optionally, you can specify \a offset to move \e contentX and \e contentY an extra number of
    pixels beyond the target alignment. E.g if you want to position the view so
    that cell [10, 10] ends up at the top-left corner with a 5px margin, you could do:

    \code
    positionViewAtCell(Qt.point(10, 10), TableView.AlignLeft | TableView.AlignTop, Qt.point(-5, -5))
    \endcode

    As of Qt 6.4, you can specify a \a subRect to position on a rectangle inside
    the \a cell, rather than on the bounding rectangle of the whole cell. This can
    be useful if the cell is e.g larger than the view, and you want to ensure that a
    specific part of it is visible. The \a subRect needs to be
    \l {QRectF::isValid()}{valid} to be taken into consideration.

    \note It is not recommended to use \e contentX or \e contentY
    to position the view at a particular cell. This is unreliable since removing items from
    the start of the table does not cause all other items to be repositioned.
    TableView can also sometimes place rows and columns at approximate positions to
    optimize for speed. The only exception is if the cell is already visible in
    the view, which can be checked upfront by calling \l itemAtCell().

    Methods should only be called after the Component has completed. To position
    the view at startup, this method should be called by Component.onCompleted. For
    example, to position the view at the end:

    \code
    Component.onCompleted: positionViewAtCell(Qt.point(columns - 1, rows - 1), TableView.AlignRight | TableView.AlignBottom)
    \endcode

    \note The second argument to this function used to be Qt.Alignment. For backwards
    compatibility, that enum can still be used. The change to use PositionMode was done
    in Qt 6.4.

    \sa animate
*/

/*!
    \qmlmethod QtQuick::TableView::positionViewAtIndex(QModelIndex index, PositionMode mode, point offset, rect subRect)
    \since 6.5

    Positions the view such that \a index is at the position specified
    by \a mode, \a offset and \a subRect.

    Convenience method for calling
    \code
    positionViewAtRow(rowAtIndex(index), mode & Qt.AlignVertical_Mask, offset.y, subRect)
    positionViewAtColumn(columnAtIndex(index), mode & Qt.AlignVertical_Mask, offset.x, subRect)
    \endcode
*/

/*!
    \qmlmethod bool QtQuick::TableView::isColumnLoaded(int column)
    \since 6.2

    Returns \c true if the given \a column is loaded.

    A column is loaded when TableView has loaded the delegate items
    needed to show the column inside the view. This also usually means
    that the column is visible for the user, but not always.

    This function can be used whenever you need to iterate over the
    delegate items for a column, e.g from a \l columnWidthProvider, to
    be sure that the delegate items are available for iteration.
*/

/*!
    \qmlmethod bool QtQuick::TableView::isRowLoaded(int row)
    \since 6.2

    Returns \c true if the given \a row is loaded.

    A row is loaded when TableView has loaded the delegate items
    needed to show the row inside the view. This also usually means
    that the row is visible for the user, but not always.

    This function can be used whenever you need to iterate over the
    delegate items for a row, e.g from a \l rowHeightProvider, to
    be sure that the delegate items are available for iteration.
*/

/*!
    \qmlmethod QtQuick::TableView::positionViewAtCell(int column, int row, PositionMode mode, point offset, rect subRect)
    \deprecated

    Use \l {positionViewAtIndex()}{positionViewAtIndex(index(row, column), ...)} instead.
*/

/*!
    \qmlmethod QtQuick::TableView::positionViewAtRow(int row, PositionMode mode, real offset, rect subRect)

    Positions {Flickable::}{contentY} such that \a row is at the position specified
    by \a mode, \a offset and \a subRect.

    Convenience method for calling
    \code
    positionViewAtCell(Qt.point(0, row), mode & Qt.AlignVertical_Mask, offset, subRect)
    \endcode
*/

/*!
    \qmlmethod QtQuick::TableView::positionViewAtColumn(int column, PositionMode mode, real offset, rect subRect)

    Positions {Flickable::}{contentX} such that \a column is at the position specified
    by \a mode, \a offset and \a subRect.

    Convenience method for calling
    \code
    positionViewAtCell(Qt.point(column, 0), mode & Qt.AlignHorizontal_Mask, offset, subRect)
    \endcode
*/

/*!
    \qmlmethod Item QtQuick::TableView::itemAtCell(point cell)

    Returns the delegate item at \a cell if loaded, otherwise \c null.

    \note only the items that are visible in the view are normally loaded.
    As soon as a cell is flicked out of the view, the item inside will
    either be unloaded or placed in the recycle pool. As such, the return
    value should never be stored.
*/

/*!
    \qmlmethod Item QtQuick::TableView::itemAtCell(int column, int row)
    \deprecated

    Use \l {itemAtIndex()}{itemAtIndex(index(row, column))} instead.
*/

/*!
    \qmlmethod Item QtQuick::TableView::itemAtIndex(QModelIndex index)
    \since 6.5

    Returns the instantiated delegate item for the cell that represents
    \a index. If the item is not \l {isRowLoaded()}{loaded}, the value
    will be \c null.

    \note only the items that are visible in the view are normally loaded.
    As soon as a cell is flicked out of the view, the item inside will
    either be unloaded or placed in the recycle pool. As such, the return
    value should never be stored.

    \note If the \l model is not a QAbstractItemModel, you can also use
    \l {itemAtCell()}{itemAtCell(Qt.point(column, row))}. But be aware
    that \c {point.x} maps to columns and \c {point.y} maps to rows.
*/

/*!
    \qmlmethod Point QtQuick::TableView::cellAtPos(point position, bool includeSpacing)
    \obsolete

    Use cellAtPosition(point position) instead.
*/

/*!
    \qmlmethod Point QtQuick::TableView::cellAtPos(real x, real y, bool includeSpacing)
    \obsolete

    Use cellAtPosition(real x, real y) instead.
*/

/*!
    \qmlmethod Point QtQuick::TableView::cellAtPosition(point position, bool includeSpacing)

    Returns the cell at the given \a position in the table. \a position should be relative
    to the \l {Flickable::}{contentItem}. If no \l {isRowLoaded()}{loaded} cell intersects
    with \a position, the return value will be \c point(-1, -1).

    If \a includeSpacing is set to \c true, a cell's bounding box will be considered
    to include half the adjacent \l rowSpacing and \l columnSpacing on each side. The
    default value is \c false.

    \note A \l {Qt Quick Input Handlers}{Input Handler} attached to a TableView installs
    itself on the \l {Flickable::}{contentItem} rather than the view. So the position
    reported by the handler can be used directly in a call to this function without any
    \l {QQuickItem::mapFromItem()}{mapping}.

    \sa columnSpacing, rowSpacing
*/

/*!
    \qmlmethod Point QtQuick::TableView::cellAtPosition(real x, real y, bool includeSpacing)

    Convenience for calling \c{cellAtPosition(Qt.point(x, y), includeSpacing)}.
*/

/*!
    \qmlmethod real QtQuick::TableView::columnWidth(int column)
    \since 6.2

    Returns the width of the given \a column. If the column is not
    loaded (and therefore not visible), the return value will be \c -1.

    \sa columnWidthProvider, implicitColumnWidth(), isColumnLoaded(), {Row heights and column widths}
*/

/*!
    \qmlmethod real QtQuick::TableView::rowHeight(int row)
    \since 6.2

    Returns the height of the given \a row. If the row is not
    loaded (and therefore not visible), the return value will be \c -1.

    \sa rowHeightProvider, implicitRowHeight(), isRowLoaded(), {Row heights and column widths}
*/

/*!
    \qmlmethod real QtQuick::TableView::implicitColumnWidth(int column)
    \since 6.2

    Returns the implicit width of the given \a column. If the
    column is not loaded (and therefore not visible), the return value
    will be \c -1.

    The implicit width of a column is the largest implicitWidth
    found among the currently loaded delegate items inside that column.
    Widths returned by the \l columnWidthProvider will not be taken
    into account.

    \sa columnWidthProvider, columnWidth(), isColumnLoaded(), {Row heights and column widths}
*/

/*!
    \qmlmethod real QtQuick::TableView::implicitRowHeight(int row)
    \since 6.2

    Returns the implicit height of the given \a row. If the
    row is not loaded (and therefore not visible), the return value
    will be \c -1.

    The implicit height of a row is the largest implicitHeight
    found among the currently loaded delegate items inside that row.
    Heights returned by the \l rowHeightProvider will not be taken
    into account.

    \sa rowHeightProvider, rowHeight(), isRowLoaded(), {Row heights and column widths}
*/

/*!
    \qmlmethod QtQuick::TableView::setColumnWidth(int column, real size)

    Sets the explicit column width of column \a column to \a size.

    If you want to read back the values you set with this function, you
    should use \l explicitColumnWidth(). \l columnWidth() will return
    the actual size of the column, which can be different if a
    \l columnWidthProvider is set.

    When TableView needs to resolve the width of \a column, it will first try
    to call the \l columnWidthProvider. Only if a provider is not set, will
    the widths set with this function be used by default. You can, however, call
    \l explicitColumnWidth() from within the provider, and if needed, moderate
    the values to e.g always be within a certain interval.
    The following snippet shows an example on how to do that:

    \code
    columnWidthProvider: function(column) {
        let w = explicitColumnWidth(column)
        if (w >= 0)
            return Math.max(100, w);
        return implicitColumnWidth(column)
    }
    \endcode

    If \a size is equal to \c 0, the column will be hidden. If \a size is
    equal to \c -1, the column will be reset back to use \l implicitColumnWidth().
    You are allowed to specify column sizes for columns that are outside the
    size of the model.

    \note The sizes you set will not be cleared if you change the \l model.
    To clear the sizes, you need to call \l clearColumnWidths() explicitly.

    \include tableview.qdocinc explicit-column-size-and-syncview

    \note For models with \e lots of columns, using \l setColumnWidth() to set the widths for
    all the columns at start-up, can be suboptimal. This will consume start-up time and
    memory (for storing all the widths). A more scalable approach is to use a
    \l columnWidthProvider instead, or rely on the implicit width of the delegate.
    A \c columnWidthProvider will only be called on an as-needed basis, and will not
    be affected by the size of the model.

    \sa explicitColumnWidth(), setRowHeight(), clearColumnWidths(), {Row heights and column widths}
*/

/*!
    \qmlmethod QtQuick::TableView::clearColumnWidths()

    Clears all the column widths set with \l setColumnWidth().

    \include tableview.qdocinc explicit-column-size-and-syncview

    \sa setColumnWidth(), clearRowHeights(), {Row heights and column widths}
*/

/*!
    \qmlmethod qreal QtQuick::TableView::explicitColumnWidth(int column)

    Returns the width of the \a column set with \l setColumnWidth(). This width might
    differ from the actual width of the column, if a \l columnWidthProvider
    is in use. To get the actual width of a column, use \l columnWidth().

    A return value equal to \c 0 means that the column has been told to hide.
    A return value equal to \c -1 means that no explicit width has been set
    for the column.

    \include tableview.qdocinc explicit-column-size-and-syncview

    \sa setColumnWidth(), columnWidth(), {Row heights and column widths}
*/

/*!
    \qmlmethod QtQuick::TableView::setRowHeight(int row, real size)

    Sets the explicit row height of row \a row to \a size.

    If you want to read back the values you set with this function, you
    should use \l explicitRowHeight(). \l rowHeight() will return
    the actual height of the row, which can be different if a
    \l rowHeightProvider is set.

    When TableView needs to resolve the height of \a row, it will first try
    to call the \l rowHeightProvider. Only if a provider is not set, will
    the heights set with this function be used by default. You can, however, call
    \l explicitRowHeight() from within the provider, and if needed, moderate
    the values to e.g always be within a certain interval.
    The following snippet shows an example on how to do that:

    \code
    rowHeightProvider: function(row) {
        let h = explicitRowHeight(row)
        if (h >= 0)
            return Math.max(100, h);
        return implicitRowHeight(row)
    }
    \endcode

    If \a size is equal to \c 0, the row will be hidden. If \a size is
    equal to \c -1, the row will be reset back to use \l implicitRowHeight().
    You are allowed to specify row sizes for rows that are outside the
    size of the model.

    \note The sizes you set will not be cleared if you change the \l model.
    To clear the sizes, you need to call \l clearRowHeights() explicitly.

    \include tableview.qdocinc explicit-row-size-and-syncview

    \note For models with \e lots of rows, using \l setRowHeight() to set the heights for
    all the rows at start-up, can be suboptimal. This will consume start-up time and
    memory (for storing all the heights). A more scalable approach is to use a
    \l rowHeightProvider instead, or rely on the implicit height of the delegate.
    A \c rowHeightProvider will only be called on an as-needed basis, and will not
    be affected by the size of the model.

    \sa explicitRowHeight(), setColumnWidth(), {Row heights and column widths}
*/

/*!
    \qmlmethod QtQuick::TableView::clearRowHeights()

    Clears all the row heights set with \l setRowHeight().

    \include tableview.qdocinc explicit-row-size-and-syncview

    \sa setRowHeight(), clearColumnWidths(), {Row heights and column widths}
*/

/*!
    \qmlmethod qreal QtQuick::TableView::explicitRowHeight(int row)

    Returns the height of the \a row set with \l setRowHeight(). This height might
    differ from the actual height of the column, if a \l rowHeightProvider
    is in use. To get the actual height of a row, use \l rowHeight().

    A return value equal to \c 0 means that the row has been told to hide.
    A return value equal to \c -1 means that no explicit height has been set
    for the row.

    \include tableview.qdocinc explicit-row-size-and-syncview

    \sa setRowHeight(), rowHeight(), {Row heights and column widths}
*/

/*!
    \qmlmethod QModelIndex QtQuick::TableView::modelIndex(int row, int column)
    \since 6.4
    \deprecated

    Use \l {QtQuick::TableView::}{index(int row, int column)} instead.

    \note Because of an API incompatible change between Qt 6.4.0 and Qt 6.4.2, the
    order of \c row and \c column was specified in the opposite order. If you
    rely on the order to be \c {modelIndex(column, row)}, you can set the
    environment variable \c QT_QUICK_TABLEVIEW_COMPAT_VERSION to \c 6.4
*/

/*!
    \qmlmethod QModelIndex QtQuick::TableView::modelIndex(point cell)
    \since 6.4

    Convenience function for doing:
    \code
    modelIndex(cell.y, cell.x)
    \endcode

    A cell is simply a \l point that combines row and column into
    a single type.

    \note \c {point.x} will map to the column, and \c {point.y} will map to the row.
*/

/*!
    \qmlmethod QModelIndex QtQuick::TableView::index(int row, int column)
    \since 6.4.3

    Returns the \l QModelIndex that maps to \a row and \a column in the view.

    \a row and \a column should be the row and column in the view (table row and
    table column), and not a row and column in the model. For a plain
    TableView, this is equivalent of calling \c {model.index(row, column).}
    But for a subclass of TableView, like TreeView, where the data model is
    wrapped inside an internal proxy model that flattens the tree structure
    into a table, you need to use this function to resolve the model index.

    \sa rowAtIndex(), columnAtIndex()
*/

/*!
    \qmlmethod int QtQuick::TableView::rowAtIndex(QModelIndex modelIndex)
    \since 6.4

    Returns the row in the view that maps to \a modelIndex in the model.

    \sa columnAtIndex(), index()
*/

/*!
    \qmlmethod int QtQuick::TableView::columnAtIndex(QModelIndex modelIndex)
    \since 6.4

    Returns the column in the view that maps to \a modelIndex in the model.

    \sa rowAtIndex(), index()
*/

/*!
    \qmlmethod point QtQuick::TableView::cellAtIndex(QModelIndex modelIndex)
    \since 6.4

    Returns the cell in the view that maps to \a modelIndex in the model.
    Convenience function for doing:

    \code
    Qt.point(columnAtIndex(modelIndex), rowAtIndex(modelIndex))
    \endcode

    A cell is simply a \l point that combines row and column into
    a single type.

    \note that \c {point.x} will map to the column, and
    \c {point.y} will map to the row.
*/

/*!
    \qmlmethod QtQuick::TableView::edit(QModelIndex modelIndex)
    \since 6.5

    This function starts an editing session for the cell that represents
    \a modelIndex. If the user is already editing another cell, that session ends.

    Normally you can specify the different ways of starting an edit session by
    using \l editTriggers instead. If that isn't sufficient, you can use this
    function. To take full control over cell editing and keep TableView from
    interfering, set editTriggers to \c TableView.NoEditTriggers.

    \note The \l {ItemSelectionModel::currentIndex}{current index} in the
    \l {selectionModel}{selection model} will also change to \a modelIndex.

    \sa closeEditor(), editTriggers, TableView::editDelegate, {Editing cells}
*/

/*!
    \qmlmethod QtQuick::TableView::closeEditor()
    \since 6.5

    If the user is editing a cell, calling this function will
    stop the editing, and destroy the edit delegate instance.

    \sa edit(), TableView::editDelegate, {Editing cells}
*/

/*!
    \qmlsignal QtQuick::TableView::layoutChanged()
    \since 6.5

    This signal is emitted whenever the layout of the
    \l {isColumnLoaded()}{loaded} rows and columns has potentially
    changed. This will especially be the case when \l forceLayout()
    is called, but also when e.g resizing a row or a column, or
    when a row or column have entered or left the viewport.

    This signal can be used to for example update the geometry
    of overlays.

    \sa forceLayout(), {Overlays and underlays}
*/

/*!
    \qmlattachedproperty TableView QtQuick::TableView::view

    This attached property holds the view that manages the delegate instance.
    It is attached to each instance of the delegate.
*/

/*!
    \qmlattachedsignal QtQuick::TableView::pooled

    This signal is emitted after an item has been added to the reuse
    pool. You can use it to pause ongoing timers or animations inside
    the item, or free up resources that cannot be reused.

    This signal is emitted only if the \l reuseItems property is \c true.

    \sa {Reusing items}, reuseItems, reused
*/

/*!
    \qmlattachedsignal QtQuick::TableView::reused

    This signal is emitted after an item has been reused. At this point, the
    item has been taken out of the pool and placed inside the content view,
    and the model properties such as index, row, and column have been updated.

    Other properties that are not provided by the model does not change when an
    item is reused. You should avoid storing any state inside a delegate, but if
    you do, manually reset that state on receiving this signal.

    This signal is emitted when the item is reused, and not the first time the
    item is created.

    This signal is emitted only if the \l reuseItems property is \c true.

    \sa {Reusing items}, reuseItems, pooled
*/

/*!
    \qmlattachedsignal QtQuick::TableView::commit
    This signal is emitted by the \l {TableView::editDelegate}{edit delegate}

    This attached signal is emitted when the \l {TableView::editDelegate}{edit delegate}
    is active, and the user presses \l Qt::Key_Enter or \l Qt::Key_Return. It will also
    be emitted if TableView has \l QQuickItem::activeFocusOnTab set, and the user
    presses Qt::Key_Tab or Qt::Key_Backtab.

    This signal will \e not be emitted if editing ends because of reasons other
    than the ones mentioned. This includes e.g if the user presses
    Qt::Key_Escape, taps outside the delegate, the row or column being
    edited is deleted, or if the application calls \l closeEditor().

    Upon receiving the signal, the edit delegate should write any modified data
    back to the model.

    \note This property should be attached to the
    \l {TableView::editDelegate}{edit delegate}, and not to the \l delegate.

    \sa TableView::editDelegate, editTriggers, {Editing cells}
*/

/*!
    \qmlattachedproperty Component QtQuick::TableView::editDelegate

    This attached property holds the edit delegate. It's instantiated
    when editing begins, and parented to the delegate it edits. It
    supports the same required properties as the
    \l {TableView::delegate}{TableView delegate}, including \c index, \c row and \c column.
    Properties of the model, like \c display and \c edit, are also available
    (depending on the \l {QAbstractItemModel::roleNames()}{role names} exposed
    by the model).

    Editing starts when the actions specified by \l editTriggers are met, and
    the current cell is editable.

    \note In order for a cell to be editable, the model needs to override
    \l QAbstractItemModel::flags(), and return \c Qt::ItemIsEditable.

    You can also open and close the edit delegate manually by calling \l edit()
    and \l closeEditor(), respectively. The \c Qt::ItemIsEditable flag will
    then be ignored.

    Editing ends when the user presses \c Qt::Key_Enter or \c Qt::Key_Return
    (and also \c Qt::Key_Tab or \c Qt::Key_Backtab, if TableView has
    \l QQuickItem::activeFocusOnTab set). In that case, the \l TableView::commit
    signal will be emitted, so that the edit delegate can respond by writing any
    modified data back to the model. If editing ends because of other reasons
    (e.g if the user presses Qt::Key_Escape), the signal will not be emitted.
    In any case will \l {Component::destruction}{destruction()} be emitted in the end.

    While the edit delegate is showing, the cell underneath will still be visible, and
    therefore shine through if the edit delegate is translucent, or otherwise doesn't
    cover the whole cell. If this is not wanted, you can either let the root item
    of the edit delegate be a solid \l Rectangle, or hide some of the items
    inside the \l {TableView::delegate}{TableView delegate.}. The latter can be done
    by defining a property \c {required property bool editing} inside it, that you
    bind to the \l {QQuickItem::}{visible} property of some of the child items.
    The following snippet shows how to do that:

    \snippet qml/tableview/editdelegate.qml 1

    When the edit delegate is instantiated, TableView will call \l QQuickItem::forceActiveFocus()
    on it. If you want active focus to be set on a child of the edit delegate instead, let
    the edit delegate be a \l FocusScope.

    \sa editTriggers, TableView::commit, edit(), closeEditor(), {Editing cells}
*/

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcTableViewDelegateLifecycle, "qt.quick.tableview.lifecycle")

#define Q_TABLEVIEW_UNREACHABLE(output) { dumpTable(); qWarning() << "output:" << output; Q_UNREACHABLE(); }
#define Q_TABLEVIEW_ASSERT(cond, output) Q_ASSERT((cond) || [&](){ dumpTable(); qWarning() << "output:" << output; return false;}())

static const Qt::Edge allTableEdges[] = { Qt::LeftEdge, Qt::RightEdge, Qt::TopEdge, Qt::BottomEdge };

static const char* kRequiredProperties = "_qt_tableview_requiredpropertymask";
static const char* kRequiredProperty_selected = "selected";
static const char* kRequiredProperty_current = "current";
static const char* kRequiredProperty_editing = "editing";

QQuickTableViewPrivate::EdgeRange::EdgeRange()
    : startIndex(kEdgeIndexNotSet)
    , endIndex(kEdgeIndexNotSet)
    , size(0)
{}

bool QQuickTableViewPrivate::EdgeRange::containsIndex(Qt::Edge edge, int index)
{
    if (startIndex == kEdgeIndexNotSet)
        return false;

    if (endIndex == kEdgeIndexAtEnd) {
        switch (edge) {
        case Qt::LeftEdge:
        case Qt::TopEdge:
            return index <= startIndex;
        case Qt::RightEdge:
        case Qt::BottomEdge:
            return index >= startIndex;
        }
    }

    const int s = std::min(startIndex, endIndex);
    const int e = std::max(startIndex, endIndex);
    return index >= s && index <= e;
}

QQuickTableViewPrivate::QQuickTableViewPrivate()
    : QQuickFlickablePrivate()
{
}

QQuickTableViewPrivate::~QQuickTableViewPrivate()
{
    if (editItem) {
        QQuickItem *cellItem = editItem->parentItem();
        Q_ASSERT(cellItem);
        editModel->dispose(editItem);
        tableModel->release(cellItem, QQmlInstanceModel::NotReusable);
    }

    if (editModel)
        delete editModel;

    for (auto *fxTableItem : loadedItems) {
        if (auto item = fxTableItem->item) {
            if (fxTableItem->ownItem)
                delete item;
            else if (tableModel)
                tableModel->dispose(item);
        }
        delete fxTableItem;
    }

    if (tableModel)
        delete tableModel;
}

QString QQuickTableViewPrivate::tableLayoutToString() const
{
    if (loadedItems.isEmpty())
        return QLatin1String("table is empty!");
    return QString(QLatin1String("table cells: (%1,%2) -> (%3,%4), item count: %5, table rect: %6,%7 x %8,%9"))
            .arg(leftColumn()).arg(topRow())
            .arg(rightColumn()).arg(bottomRow())
            .arg(loadedItems.size())
            .arg(loadedTableOuterRect.x())
            .arg(loadedTableOuterRect.y())
            .arg(loadedTableOuterRect.width())
            .arg(loadedTableOuterRect.height());
}

void QQuickTableViewPrivate::dumpTable() const
{
    auto listCopy = loadedItems.values();
    std::stable_sort(listCopy.begin(), listCopy.end(),
        [](const FxTableItem *lhs, const FxTableItem *rhs)
        { return lhs->index < rhs->index; });

    qWarning() << QStringLiteral("******* TABLE DUMP *******");
    for (int i = 0; i < listCopy.size(); ++i)
        qWarning() << static_cast<FxTableItem *>(listCopy.at(i))->cell;
    qWarning() << tableLayoutToString();

    const QString filename = QStringLiteral("QQuickTableView_dumptable_capture.png");
    const QString path = QDir::current().absoluteFilePath(filename);
    if (q_func()->window() && q_func()->window()->grabWindow().save(path))
        qWarning() << "Window capture saved to:" << path;
}

void QQuickTableViewPrivate::setRequiredProperty(const char *property,
    const QVariant &value, int serializedModelIndex, QObject *object, bool init)
{
    Q_Q(QQuickTableView);

    QQmlTableInstanceModel *tableInstanceModel = qobject_cast<QQmlTableInstanceModel *>(model);
    if (!tableInstanceModel) {
        // TableView only supports using required properties when backed by
        // a QQmlTableInstanceModel. This is almost always the case, except
        // if you assign it an ObjectModel or a DelegateModel (which are really
        // not supported by TableView, it expects a QAIM).
        return;
    }

    // Attaching a property list to the delegate item is just a
    // work-around until QMetaProperty::isRequired() works (QTBUG-98846).
    const QString propertyName = QString::fromUtf8(property);

    if (init) {
        bool wasRequired = false;
        if (object == editItem) {
            // Special case: the item that we should write to belongs to the edit
            // model rather than 'model' (which is used for normal delegate items).
            wasRequired = editModel->setRequiredProperty(serializedModelIndex, propertyName, value);
        } else {
            wasRequired = tableInstanceModel->setRequiredProperty(serializedModelIndex, propertyName, value);
        }
        if (wasRequired) {
            QStringList propertyList = object->property(kRequiredProperties).toStringList();
            object->setProperty(kRequiredProperties, propertyList << propertyName);
        }
    } else {
        {
            const QStringList propertyList = object->property(kRequiredProperties).toStringList();
            if (propertyList.contains(propertyName)) {
                const auto metaObject = object->metaObject();
                const int propertyIndex = metaObject->indexOfProperty(property);
                const auto metaProperty = metaObject->property(propertyIndex);
                metaProperty.write(object, value);
            }
        }

        if (editItem) {
            // Whenever we're told to update a required property for a table item that has the
            // same model index as the edit item, we also mirror that update to the edit item.
            // As such, this function is never called for the edit item directly (except the
            // first time when it needs to be initialized).
            Q_TABLEVIEW_ASSERT(object != editItem, "");
            const QModelIndex modelIndex = q->modelIndex(cellAtModelIndex(serializedModelIndex));
            if (modelIndex == editIndex) {
                const QStringList propertyList = editItem->property(kRequiredProperties).toStringList();
                if (propertyList.contains(propertyName)) {
                    const auto metaObject = editItem->metaObject();
                    const int propertyIndex = metaObject->indexOfProperty(property);
                    const auto metaProperty = metaObject->property(propertyIndex);
                    metaProperty.write(editItem, value);
                }
            }
        }

    }
}

QQuickItem *QQuickTableViewPrivate::selectionPointerHandlerTarget() const
{
    return const_cast<QQuickTableView *>(q_func())->contentItem();
}

bool QQuickTableViewPrivate::startSelection(const QPointF &pos)
{
    Q_Q(QQuickTableView);
    Q_UNUSED(pos);

    if (selectionBehavior == QQuickTableView::SelectionDisabled) {
        qmlWarning(q) << "Cannot start selection: TableView.selectionBehavior == TableView.SelectionDisabled";
        return false;
    }

    // Only allow a selection if it doesn't conflict with resizing
    if (resizeHandler->state() != QQuickTableViewResizeHandler::Listening)
        return false;

    // For SingleSelection and ContiguousSelection, we should only allow one selection at a time
    if (selectionMode == QQuickTableView::SingleSelection
            || selectionMode == QQuickTableView::ContiguousSelection)
        clearSelection();

    selectionStartCell = QPoint(-1, -1);
    selectionEndCell = QPoint(-1, -1);
    q->closeEditor();
    return true;
}

void QQuickTableViewPrivate::setSelectionStartPos(const QPointF &pos)
{
    if (loadedItems.isEmpty())
        return;
    if (!selectionModel) {
        if (warnNoSelectionModel)
            qmlWarning(q_func()) << "Cannot set selection: no SelectionModel assigned!";
        warnNoSelectionModel = false;
        return;
    }
    const QAbstractItemModel *qaim = selectionModel->model();
    if (!qaim)
        return;

    if (selectionMode == QQuickTableView::SingleSelection
            && cellIsValid(selectionStartCell)) {
        return;
    }

    const QRect prevSelection = selection();
    const QPoint clampedCell = clampedCellAtPos(pos);
    if (!cellIsValid(clampedCell))
        return;

    setCurrentIndex(clampedCell);

    switch (selectionBehavior) {
    case QQuickTableView::SelectCells:
        selectionStartCell = clampedCell;
        break;
    case QQuickTableView::SelectRows:
        selectionStartCell = QPoint(0, clampedCell.y());
        break;
    case QQuickTableView::SelectColumns:
        selectionStartCell = QPoint(clampedCell.x(), 0);
        break;
    case QQuickTableView::SelectionDisabled:
        return;
    }

    if (!cellIsValid(selectionEndCell))
        return;

    // Update selection model
    updateSelection(prevSelection, selection());
}

void QQuickTableViewPrivate::setSelectionEndPos(const QPointF &pos)
{
    if (loadedItems.isEmpty())
        return;
    if (!selectionModel) {
        if (warnNoSelectionModel)
            qmlWarning(q_func()) << "Cannot set selection: no SelectionModel assigned!";
        warnNoSelectionModel = false;
        return;
    }
    const QAbstractItemModel *qaim = selectionModel->model();
    if (!qaim)
        return;

    const QRect prevSelection = selection();

    QPoint clampedCell;
    if (selectionMode == QQuickTableView::SingleSelection) {
        clampedCell = selectionStartCell;
    } else {
        clampedCell = clampedCellAtPos(pos);
        if (!cellIsValid(clampedCell))
            return;
    }

    setCurrentIndex(clampedCell);

    switch (selectionBehavior) {
    case QQuickTableView::SelectCells:
        selectionEndCell = clampedCell;
        break;
    case QQuickTableView::SelectRows:
        selectionEndCell = QPoint(tableSize.width() - 1, clampedCell.y());
        break;
    case QQuickTableView::SelectColumns:
        selectionEndCell = QPoint(clampedCell.x(), tableSize.height() - 1);
        break;
    case QQuickTableView::SelectionDisabled:
        return;
    }

    if (!cellIsValid(selectionStartCell))
        return;

    // Update selection model
    updateSelection(prevSelection, selection());
}

QPoint QQuickTableViewPrivate::clampedCellAtPos(const QPointF &pos) const
{
    Q_Q(const QQuickTableView);

    // Note: pos should be relative to selectionPointerHandlerTarget()
    QPoint cell = q->cellAtPosition(pos, true);
    if (cellIsValid(cell))
        return cell;

    // Clamp the cell to the loaded table and the viewport, whichever is the smallest
    QPointF clampedPos(
                qBound(loadedTableOuterRect.x(), pos.x(), loadedTableOuterRect.right() - 1),
                qBound(loadedTableOuterRect.y(), pos.y(), loadedTableOuterRect.bottom() - 1));
    QPointF clampedPosInView = q->mapFromItem(selectionPointerHandlerTarget(), clampedPos);
    clampedPosInView.rx() = qBound(0., clampedPosInView.x(), viewportRect.width());
    clampedPosInView.ry() = qBound(0., clampedPosInView.y(), viewportRect.height());
    clampedPos = q->mapToItem(selectionPointerHandlerTarget(), clampedPosInView);

    return q->cellAtPosition(clampedPos, true);
}

void QQuickTableViewPrivate::updateSelection(const QRect &oldSelection, const QRect &newSelection)
{
    const QAbstractItemModel *qaim = selectionModel->model();
    const QRect oldRect = oldSelection.normalized();
    const QRect newRect = newSelection.normalized();

    // Select cells inside the new selection rect
    {
        const QModelIndex startIndex = qaim->index(newRect.y(), newRect.x());
        const QModelIndex endIndex = qaim->index(newRect.y() + newRect.height(), newRect.x() + newRect.width());
        selectionModel->select(QItemSelection(startIndex, endIndex), QItemSelectionModel::Select);
    }

    // Unselect cells in the new minus old rects
    if (oldRect.x() < newRect.x()) {
        const QModelIndex startIndex = qaim->index(oldRect.y(), oldRect.x());
        const QModelIndex endIndex = qaim->index(oldRect.y() + oldRect.height(), newRect.x() - 1);
        selectionModel->select(QItemSelection(startIndex, endIndex), QItemSelectionModel::Deselect);
    } else if (oldRect.x() + oldRect.width() > newRect.x() + newRect.width()) {
        const QModelIndex startIndex = qaim->index(oldRect.y(), newRect.x() + newRect.width() + 1);
        const QModelIndex endIndex = qaim->index(oldRect.y() + oldRect.height(), oldRect.x() + oldRect.width());
        selectionModel->select(QItemSelection(startIndex, endIndex), QItemSelectionModel::Deselect);
    }

    if (oldRect.y() < newRect.y()) {
        const QModelIndex startIndex = qaim->index(oldRect.y(), oldRect.x());
        const QModelIndex endIndex = qaim->index(newRect.y() - 1, oldRect.x() + oldRect.width());
        selectionModel->select(QItemSelection(startIndex, endIndex), QItemSelectionModel::Deselect);
    } else if (oldRect.y() + oldRect.height() > newRect.y() + newRect.height()) {
        const QModelIndex startIndex = qaim->index(newRect.y() + newRect.height() + 1, oldRect.x());
        const QModelIndex endIndex = qaim->index(oldRect.y() + oldRect.height(), oldRect.x() + oldRect.width());
        selectionModel->select(QItemSelection(startIndex, endIndex), QItemSelectionModel::Deselect);
    }
}

void QQuickTableViewPrivate::clearSelection()
{
    selectionStartCell = QPoint(-1, -1);
    selectionEndCell = QPoint(-1, -1);

    if (selectionModel)
        selectionModel->clearSelection();
}

void QQuickTableViewPrivate::normalizeSelection()
{
    // Normalize the selection if necessary, so that the start cell is to the left
    // and above the end cell. This is typically done after a selection drag has
    // finished so that the start and end positions up in sync with the handles.
    // This will not cause any changes to the selection itself.
    if (selectionEndCell.x() < selectionStartCell.x())
        std::swap(selectionStartCell.rx(), selectionEndCell.rx());
    if (selectionEndCell.y() < selectionStartCell.y())
        std::swap(selectionStartCell.ry(), selectionEndCell.ry());
}

QRectF QQuickTableViewPrivate::selectionRectangle() const
{
    Q_Q(const QQuickTableView);

    QPoint topLeftCell = selectionStartCell;
    QPoint bottomRightCell = selectionEndCell;
    if (bottomRightCell.x() < topLeftCell.x())
        std::swap(topLeftCell.rx(), bottomRightCell.rx());
    if (selectionEndCell.y() < topLeftCell.y())
        std::swap(topLeftCell.ry(), bottomRightCell.ry());

    const QPoint leftCell(topLeftCell.x(), topRow());
    const QPoint topCell(leftColumn(), topLeftCell.y());
    const QPoint rightCell(bottomRightCell.x(), topRow());
    const QPoint bottomCell(leftColumn(), bottomRightCell.y());

    // If the corner cells of the selection are loaded, we can position the
    // selection rectangle at its exact location. Otherwise we extend it out
    // to the edges of the content item. This is not ideal, but the best we
    // can do while the location of the corner cells are unknown.
    // This will at least move the selection handles (and other overlay) out
    // of the viewport until the affected cells are eventually loaded.
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    if (loadedItems.contains(modelIndexAtCell(leftCell)))
        left = loadedTableItem(leftCell)->geometry().left();
    else if (leftCell.x() > rightColumn())
        left = q->contentWidth();

    if (loadedItems.contains(modelIndexAtCell(topCell)))
        top = loadedTableItem(topCell)->geometry().top();
    else if (topCell.y() > bottomRow())
        top = q->contentHeight();

    if (loadedItems.contains(modelIndexAtCell(rightCell)))
        right = loadedTableItem(rightCell)->geometry().right();
    else if (rightCell.x() > rightColumn())
        right = q->contentWidth();

    if (loadedItems.contains(modelIndexAtCell(bottomCell)))
        bottom = loadedTableItem(bottomCell)->geometry().bottom();
    else if (bottomCell.y() > bottomRow())
        bottom = q->contentHeight();

    return QRectF(left, top, right - left, bottom - top);
}

QRect QQuickTableViewPrivate::selection() const
{
    const qreal w = selectionEndCell.x() - selectionStartCell.x();
    const qreal h = selectionEndCell.y() - selectionStartCell.y();
    return QRect(selectionStartCell.x(), selectionStartCell.y(), w, h);
}

QSizeF QQuickTableViewPrivate::scrollTowardsSelectionPoint(const QPointF &pos, const QSizeF &step)
{
    Q_Q(QQuickTableView);

    if (loadedItems.isEmpty())
        return QSizeF();

    // Scroll the content item towards pos.
    // Return the distance in pixels from the edge of the viewport to pos.
    // The caller will typically use this information to throttle the scrolling speed.
    // If pos is already inside the viewport, or the viewport is scrolled all the way
    // to the end, we return 0.
    QSizeF dist(0, 0);

    const bool outsideLeft = pos.x() < viewportRect.x();
    const bool outsideRight = pos.x() >= viewportRect.right() - 1;
    const bool outsideTop = pos.y() < viewportRect.y();
    const bool outsideBottom = pos.y() >= viewportRect.bottom() - 1;

    if (outsideLeft) {
        const bool firstColumnLoaded = atTableEnd(Qt::LeftEdge);
        const qreal remainingDist = viewportRect.left() - loadedTableOuterRect.left();
        if (remainingDist > 0 || !firstColumnLoaded) {
            qreal stepX = step.width();
            if (firstColumnLoaded)
                stepX = qMin(stepX, remainingDist);
            q->setContentX(q->contentX() - stepX);
            dist.setWidth(pos.x() - viewportRect.left() - 1);
        }
    } else if (outsideRight) {
        const bool lastColumnLoaded = atTableEnd(Qt::RightEdge);
        const qreal remainingDist = loadedTableOuterRect.right() - viewportRect.right();
        if (remainingDist > 0 || !lastColumnLoaded) {
            qreal stepX = step.width();
            if (lastColumnLoaded)
                stepX = qMin(stepX, remainingDist);
            q->setContentX(q->contentX() + stepX);
            dist.setWidth(pos.x() - viewportRect.right() - 1);
        }
    }

    if (outsideTop) {
        const bool firstRowLoaded = atTableEnd(Qt::TopEdge);
        const qreal remainingDist = viewportRect.top() - loadedTableOuterRect.top();
        if (remainingDist > 0 || !firstRowLoaded) {
            qreal stepY = step.height();
            if (firstRowLoaded)
                stepY = qMin(stepY, remainingDist);
            q->setContentY(q->contentY() - stepY);
            dist.setHeight(pos.y() - viewportRect.top() - 1);
        }
    } else if (outsideBottom) {
        const bool lastRowLoaded = atTableEnd(Qt::BottomEdge);
        const qreal remainingDist = loadedTableOuterRect.bottom() - viewportRect.bottom();
        if (remainingDist > 0 || !lastRowLoaded) {
            qreal stepY = step.height();
            if (lastRowLoaded)
                stepY = qMin(stepY, remainingDist);
            q->setContentY(q->contentY() + stepY);
            dist.setHeight(pos.y() - viewportRect.bottom() - 1);
        }
    }

    return dist;
}

QQuickTableViewAttached *QQuickTableViewPrivate::getAttachedObject(const QObject *object) const
{
    QObject *attachedObject = qmlAttachedPropertiesObject<QQuickTableView>(object);
    return static_cast<QQuickTableViewAttached *>(attachedObject);
}

int QQuickTableViewPrivate::modelIndexAtCell(const QPoint &cell) const
{
    // QQmlTableInstanceModel expects index to be in column-major
    // order. This means that if the view is transposed (with a flipped
    // width and height), we need to calculate it in row-major instead.
    if (isTransposed) {
        int availableColumns = tableSize.width();
        return (cell.y() * availableColumns) + cell.x();
    } else {
        int availableRows = tableSize.height();
        return (cell.x() * availableRows) + cell.y();
    }
}

QPoint QQuickTableViewPrivate::cellAtModelIndex(int modelIndex) const
{
    // QQmlTableInstanceModel expects index to be in column-major
    // order. This means that if the view is transposed (with a flipped
    // width and height), we need to calculate it in row-major instead.
    if (isTransposed) {
        int availableColumns = tableSize.width();
        int row = int(modelIndex / availableColumns);
        int column = modelIndex % availableColumns;
        return QPoint(column, row);
    } else {
        int availableRows = tableSize.height();
        int column = int(modelIndex / availableRows);
        int row = modelIndex % availableRows;
        return QPoint(column, row);
    }
}

int QQuickTableViewPrivate::modelIndexToCellIndex(const QModelIndex &modelIndex) const
{
    // Convert QModelIndex to cell index. A cell index is just an
    // integer representation of a cell instead of using a QPoint.
    const QPoint cell = q_func()->cellAtIndex(modelIndex);
    if (!cellIsValid(cell))
        return -1;
    return modelIndexAtCell(cell);
}

int QQuickTableViewPrivate::edgeToArrayIndex(Qt::Edge edge) const
{
    return int(log2(float(edge)));
}

void QQuickTableViewPrivate::clearEdgeSizeCache()
{
    cachedColumnWidth.startIndex = kEdgeIndexNotSet;
    cachedRowHeight.startIndex = kEdgeIndexNotSet;

    for (Qt::Edge edge : allTableEdges)
        cachedNextVisibleEdgeIndex[edgeToArrayIndex(edge)].startIndex = kEdgeIndexNotSet;
}

int QQuickTableViewPrivate::nextVisibleEdgeIndexAroundLoadedTable(Qt::Edge edge) const
{
    // Find the next column (or row) around the loaded table that is
    // visible, and should be loaded next if the content item moves.
    int startIndex = -1;
    switch (edge) {
    case Qt::LeftEdge: startIndex = leftColumn() - 1; break;
    case Qt::RightEdge: startIndex = rightColumn() + 1; break;
    case Qt::TopEdge: startIndex = topRow() - 1; break;
    case Qt::BottomEdge: startIndex = bottomRow() + 1; break;
    }

    return nextVisibleEdgeIndex(edge, startIndex);
}

int QQuickTableViewPrivate::nextVisibleEdgeIndex(Qt::Edge edge, int startIndex) const
{
    // First check if we have already searched for the first visible index
    // after the given startIndex recently, and if so, return the cached result.
    // The cached result is valid if startIndex is inside the range between the
    // startIndex and the first visible index found after it.
    auto &cachedResult = cachedNextVisibleEdgeIndex[edgeToArrayIndex(edge)];
    if (cachedResult.containsIndex(edge, startIndex))
        return cachedResult.endIndex;

    // Search for the first column (or row) in the direction of edge that is
    // visible, starting from the given column (startIndex).
    int foundIndex = kEdgeIndexNotSet;
    int testIndex = startIndex;

    switch (edge) {
    case Qt::LeftEdge: {
        forever {
            if (testIndex < 0) {
                foundIndex = kEdgeIndexAtEnd;
                break;
            }

            if (!isColumnHidden(testIndex)) {
                foundIndex = testIndex;
                break;
            }

            --testIndex;
        }
        break; }
    case Qt::RightEdge: {
        forever {
            if (testIndex > tableSize.width() - 1) {
                foundIndex = kEdgeIndexAtEnd;
                break;
            }

            if (!isColumnHidden(testIndex)) {
                foundIndex = testIndex;
                break;
            }

            ++testIndex;
        }
        break; }
    case Qt::TopEdge: {
        forever {
            if (testIndex < 0) {
                foundIndex = kEdgeIndexAtEnd;
                break;
            }

            if (!isRowHidden(testIndex)) {
                foundIndex = testIndex;
                break;
            }

            --testIndex;
        }
        break; }
    case Qt::BottomEdge: {
        forever {
            if (testIndex > tableSize.height() - 1) {
                foundIndex = kEdgeIndexAtEnd;
                break;
            }

            if (!isRowHidden(testIndex)) {
                foundIndex = testIndex;
                break;
            }

            ++testIndex;
        }
        break; }
    }

    cachedResult.startIndex = startIndex;
    cachedResult.endIndex = foundIndex;
    return foundIndex;
}

void QQuickTableViewPrivate::updateContentWidth()
{
    // Note that we actually never really know what the content size / size of the full table will
    // be. Even if e.g spacing changes, and we normally would assume that the size of the table
    // would increase accordingly, the model might also at some point have removed/hidden/resized
    // rows/columns outside the viewport. This would also affect the size, but since we don't load
    // rows or columns outside the viewport, this information is ignored. And even if we did, we
    // might also have been fast-flicked to a new location at some point, and started a new rebuild
    // there based on a new guesstimated top-left cell. So the calculated content size should always
    // be understood as a guesstimate, which sometimes can be really off (as a tradeoff for performance).
    // When this is not acceptable, the user can always set a custom content size explicitly.
    Q_Q(QQuickTableView);

    if (syncHorizontally) {
        QBoolBlocker fixupGuard(inUpdateContentSize, true);
        q->QQuickFlickable::setContentWidth(syncView->contentWidth());
        return;
    }

    if (explicitContentWidth.isValid()) {
        // Don't calculate contentWidth when it
        // was set explicitly by the application.
        return;
    }

    if (loadedItems.isEmpty()) {
        QBoolBlocker fixupGuard(inUpdateContentSize, true);
        if (model && model->count() > 0 && tableModel && tableModel->delegate())
            q->QQuickFlickable::setContentWidth(kDefaultColumnWidth);
        else
            q->QQuickFlickable::setContentWidth(0);
        return;
    }

    const int nextColumn = nextVisibleEdgeIndexAroundLoadedTable(Qt::RightEdge);
    const int columnsRemaining = nextColumn == kEdgeIndexAtEnd ? 0 : tableSize.width() - nextColumn;
    const qreal remainingColumnWidths = columnsRemaining * averageEdgeSize.width();
    const qreal remainingSpacing = columnsRemaining * cellSpacing.width();
    const qreal estimatedRemainingWidth = remainingColumnWidths + remainingSpacing;
    const qreal estimatedWidth = loadedTableOuterRect.right() + estimatedRemainingWidth;

    QBoolBlocker fixupGuard(inUpdateContentSize, true);
    q->QQuickFlickable::setContentWidth(estimatedWidth);
}

void QQuickTableViewPrivate::updateContentHeight()
{
    Q_Q(QQuickTableView);

    if (syncVertically) {
        QBoolBlocker fixupGuard(inUpdateContentSize, true);
        q->QQuickFlickable::setContentHeight(syncView->contentHeight());
        return;
    }

    if (explicitContentHeight.isValid()) {
        // Don't calculate contentHeight when it
        // was set explicitly by the application.
        return;
    }

    if (loadedItems.isEmpty()) {
        QBoolBlocker fixupGuard(inUpdateContentSize, true);
        if (model && model->count() > 0 && tableModel && tableModel->delegate())
            q->QQuickFlickable::setContentHeight(kDefaultRowHeight);
        else
            q->QQuickFlickable::setContentHeight(0);
        return;
    }

    const int nextRow = nextVisibleEdgeIndexAroundLoadedTable(Qt::BottomEdge);
    const int rowsRemaining = nextRow == kEdgeIndexAtEnd ? 0 : tableSize.height() - nextRow;
    const qreal remainingRowHeights = rowsRemaining * averageEdgeSize.height();
    const qreal remainingSpacing = rowsRemaining * cellSpacing.height();
    const qreal estimatedRemainingHeight = remainingRowHeights + remainingSpacing;
    const qreal estimatedHeight = loadedTableOuterRect.bottom() + estimatedRemainingHeight;

    QBoolBlocker fixupGuard(inUpdateContentSize, true);
    q->QQuickFlickable::setContentHeight(estimatedHeight);
}

void QQuickTableViewPrivate::updateExtents()
{
    // When rows or columns outside the viewport are removed or added, or a rebuild
    // forces us to guesstimate a new top-left, the edges of the table might end up
    // out of sync with the edges of the content view. We detect this situation here, and
    // move the origin to ensure that there will never be gaps at the end of the table.
    // Normally we detect that the size of the whole table is not going to be equal to the
    // size of the content view already when we load the last row/column, and especially
    // before it's flicked completely inside the viewport. For those cases we simply adjust
    // the origin/endExtent, to give a smooth flicking experience.
    // But if flicking fast (e.g with a scrollbar), it can happen that the viewport ends up
    // outside the end of the table in just one viewport update. To avoid a "blink" in the
    // viewport when that happens, we "move" the loaded table into the viewport to cover it.
    Q_Q(QQuickTableView);

    bool tableMovedHorizontally = false;
    bool tableMovedVertically = false;

    const int nextLeftColumn = nextVisibleEdgeIndexAroundLoadedTable(Qt::LeftEdge);
    const int nextRightColumn = nextVisibleEdgeIndexAroundLoadedTable(Qt::RightEdge);
    const int nextTopRow = nextVisibleEdgeIndexAroundLoadedTable(Qt::TopEdge);
    const int nextBottomRow = nextVisibleEdgeIndexAroundLoadedTable(Qt::BottomEdge);

    if (syncHorizontally) {
        const auto syncView_d = syncView->d_func();
        origin.rx() = syncView_d->origin.x();
        endExtent.rwidth() = syncView_d->endExtent.width();
        hData.markExtentsDirty();
    } else if (nextLeftColumn == kEdgeIndexAtEnd) {
        // There are no more columns to load on the left side of the table.
        // In that case, we ensure that the origin match the beginning of the table.
        if (loadedTableOuterRect.left() > viewportRect.left()) {
            // We have a blank area at the left end of the viewport. In that case we don't have time to
            // wait for the viewport to move (after changing origin), since that will take an extra
            // update cycle, which will be visible as a blink. Instead, unless the blank spot is just
            // us overshooting, we brute force the loaded table inside the already existing viewport.
            if (loadedTableOuterRect.left() > origin.x()) {
                const qreal diff = loadedTableOuterRect.left() - origin.x();
                loadedTableOuterRect.moveLeft(loadedTableOuterRect.left() - diff);
                loadedTableInnerRect.moveLeft(loadedTableInnerRect.left() - diff);
                tableMovedHorizontally = true;
            }
        }
        origin.rx() = loadedTableOuterRect.left();
        hData.markExtentsDirty();
    } else if (loadedTableOuterRect.left() <= origin.x() + cellSpacing.width()) {
        // The table rect is at the origin, or outside, but we still have more
        // visible columns to the left. So we try to guesstimate how much space
        // the rest of the columns will occupy, and move the origin accordingly.
        const int columnsRemaining = nextLeftColumn + 1;
        const qreal remainingColumnWidths = columnsRemaining * averageEdgeSize.width();
        const qreal remainingSpacing = columnsRemaining * cellSpacing.width();
        const qreal estimatedRemainingWidth = remainingColumnWidths + remainingSpacing;
        origin.rx() = loadedTableOuterRect.left() - estimatedRemainingWidth;
        hData.markExtentsDirty();
    } else if (nextRightColumn == kEdgeIndexAtEnd) {
        // There are no more columns to load on the right side of the table.
        // In that case, we ensure that the end of the content view match the end of the table.
        if (loadedTableOuterRect.right() < viewportRect.right()) {
            // We have a blank area at the right end of the viewport. In that case we don't have time to
            // wait for the viewport to move (after changing endExtent), since that will take an extra
            // update cycle, which will be visible as a blink. Instead, unless the blank spot is just
            // us overshooting, we brute force the loaded table inside the already existing viewport.
            const qreal w = qMin(viewportRect.right(), q->contentWidth() + endExtent.width());
            if (loadedTableOuterRect.right() < w) {
                const qreal diff = loadedTableOuterRect.right() - w;
                loadedTableOuterRect.moveRight(loadedTableOuterRect.right() - diff);
                loadedTableInnerRect.moveRight(loadedTableInnerRect.right() - diff);
                tableMovedHorizontally = true;
            }
        }
        endExtent.rwidth() = loadedTableOuterRect.right() - q->contentWidth();
        hData.markExtentsDirty();
    } else if (loadedTableOuterRect.right() >= q->contentWidth() + endExtent.width() - cellSpacing.width()) {
        // The right-most column is outside the end of the content view, and we
        // still have more visible columns in the model. This can happen if the application
        // has set a fixed content width.
        const int columnsRemaining = tableSize.width() - nextRightColumn;
        const qreal remainingColumnWidths = columnsRemaining * averageEdgeSize.width();
        const qreal remainingSpacing = columnsRemaining * cellSpacing.width();
        const qreal estimatedRemainingWidth = remainingColumnWidths + remainingSpacing;
        const qreal pixelsOutsideContentWidth = loadedTableOuterRect.right() - q->contentWidth();
        endExtent.rwidth() = pixelsOutsideContentWidth + estimatedRemainingWidth;
        hData.markExtentsDirty();
    }

    if (syncVertically) {
        const auto syncView_d = syncView->d_func();
        origin.ry() = syncView_d->origin.y();
        endExtent.rheight() = syncView_d->endExtent.height();
        vData.markExtentsDirty();
    } else if (nextTopRow == kEdgeIndexAtEnd) {
        // There are no more rows to load on the top side of the table.
        // In that case, we ensure that the origin match the beginning of the table.
        if (loadedTableOuterRect.top() > viewportRect.top()) {
            // We have a blank area at the top of the viewport. In that case we don't have time to
            // wait for the viewport to move (after changing origin), since that will take an extra
            // update cycle, which will be visible as a blink. Instead, unless the blank spot is just
            // us overshooting, we brute force the loaded table inside the already existing viewport.
            if (loadedTableOuterRect.top() > origin.y()) {
                const qreal diff = loadedTableOuterRect.top() - origin.y();
                loadedTableOuterRect.moveTop(loadedTableOuterRect.top() - diff);
                loadedTableInnerRect.moveTop(loadedTableInnerRect.top() - diff);
                tableMovedVertically = true;
            }
        }
        origin.ry() = loadedTableOuterRect.top();
        vData.markExtentsDirty();
    } else if (loadedTableOuterRect.top() <= origin.y() + cellSpacing.height()) {
        // The table rect is at the origin, or outside, but we still have more
        // visible rows at the top. So we try to guesstimate how much space
        // the rest of the rows will occupy, and move the origin accordingly.
        const int rowsRemaining = nextTopRow + 1;
        const qreal remainingRowHeights = rowsRemaining * averageEdgeSize.height();
        const qreal remainingSpacing = rowsRemaining * cellSpacing.height();
        const qreal estimatedRemainingHeight = remainingRowHeights + remainingSpacing;
        origin.ry() = loadedTableOuterRect.top() - estimatedRemainingHeight;
        vData.markExtentsDirty();
    } else if (nextBottomRow == kEdgeIndexAtEnd) {
        // There are no more rows to load on the bottom side of the table.
        // In that case, we ensure that the end of the content view match the end of the table.
        if (loadedTableOuterRect.bottom() < viewportRect.bottom()) {
            // We have a blank area at the bottom of the viewport. In that case we don't have time to
            // wait for the viewport to move (after changing endExtent), since that will take an extra
            // update cycle, which will be visible as a blink. Instead, unless the blank spot is just
            // us overshooting, we brute force the loaded table inside the already existing viewport.
            const qreal h = qMin(viewportRect.bottom(), q->contentHeight() + endExtent.height());
            if (loadedTableOuterRect.bottom() < h) {
                const qreal diff = loadedTableOuterRect.bottom() - h;
                loadedTableOuterRect.moveBottom(loadedTableOuterRect.bottom() - diff);
                loadedTableInnerRect.moveBottom(loadedTableInnerRect.bottom() - diff);
                tableMovedVertically = true;
            }
        }
        endExtent.rheight() = loadedTableOuterRect.bottom() - q->contentHeight();
        vData.markExtentsDirty();
    } else if (loadedTableOuterRect.bottom() >= q->contentHeight() + endExtent.height() - cellSpacing.height()) {
        // The bottom-most row is outside the end of the content view, and we
        // still have more visible rows in the model. This can happen if the application
        // has set a fixed content height.
        const int rowsRemaining = tableSize.height() - nextBottomRow;
        const qreal remainingRowHeigts = rowsRemaining * averageEdgeSize.height();
        const qreal remainingSpacing = rowsRemaining * cellSpacing.height();
        const qreal estimatedRemainingHeight = remainingRowHeigts + remainingSpacing;
        const qreal pixelsOutsideContentHeight = loadedTableOuterRect.bottom() - q->contentHeight();
        endExtent.rheight() = pixelsOutsideContentHeight + estimatedRemainingHeight;
        vData.markExtentsDirty();
    }

    if (tableMovedHorizontally || tableMovedVertically) {
        qCDebug(lcTableViewDelegateLifecycle) << "move table to" << loadedTableOuterRect;

        // relayoutTableItems() will take care of moving the existing
        // delegate items into the new loadedTableOuterRect.
        relayoutTableItems();

        // Inform the sync children that they need to rebuild to stay in sync
        for (auto syncChild : std::as_const(syncChildren)) {
            auto syncChild_d = syncChild->d_func();
            syncChild_d->scheduledRebuildOptions |= RebuildOption::ViewportOnly;
            if (tableMovedHorizontally)
                syncChild_d->scheduledRebuildOptions |= RebuildOption::CalculateNewTopLeftColumn;
            if (tableMovedVertically)
                syncChild_d->scheduledRebuildOptions |= RebuildOption::CalculateNewTopLeftRow;
        }
    }

    if (hData.minExtentDirty || vData.minExtentDirty) {
        qCDebug(lcTableViewDelegateLifecycle) << "move origin and endExtent to:" << origin << endExtent;
        // updateBeginningEnd() will let the new extents take effect. This will also change the
        // visualArea of the flickable, which again will cause any attached scrollbars to adjust
        // the position of the handle. Note the latter will cause the viewport to move once more.
        updateBeginningEnd();
    }
}

void QQuickTableViewPrivate::updateAverageColumnWidth()
{
    if (explicitContentWidth.isValid()) {
        const qreal accColumnSpacing = (tableSize.width() - 1) * cellSpacing.width();
        averageEdgeSize.setWidth((explicitContentWidth - accColumnSpacing) / tableSize.width());
    } else {
        const qreal accColumnSpacing = (loadedColumns.count() - 1) * cellSpacing.width();
        averageEdgeSize.setWidth((loadedTableOuterRect.width() - accColumnSpacing) / loadedColumns.count());
    }
}

void QQuickTableViewPrivate::updateAverageRowHeight()
{
    if (explicitContentHeight.isValid()) {
        const qreal accRowSpacing = (tableSize.height() - 1) * cellSpacing.height();
        averageEdgeSize.setHeight((explicitContentHeight - accRowSpacing) / tableSize.height());
    } else {
        const qreal accRowSpacing = (loadedRows.count() - 1) * cellSpacing.height();
        averageEdgeSize.setHeight((loadedTableOuterRect.height() - accRowSpacing) / loadedRows.count());
    }
}

void QQuickTableViewPrivate::syncLoadedTableRectFromLoadedTable()
{
    const QPoint topLeft = QPoint(leftColumn(), topRow());
    const QPoint bottomRight = QPoint(rightColumn(), bottomRow());
    QRectF topLeftRect = loadedTableItem(topLeft)->geometry();
    QRectF bottomRightRect = loadedTableItem(bottomRight)->geometry();
    loadedTableOuterRect = QRectF(topLeftRect.topLeft(), bottomRightRect.bottomRight());
    loadedTableInnerRect = QRectF(topLeftRect.bottomRight(), bottomRightRect.topLeft());
}

void QQuickTableViewPrivate::shiftLoadedTableRect(const QPointF newPosition)
{
    // Move the tracked table rects to the new position. For this to
    // take visual effect (move the delegate items to be inside the table
    // rect), it needs to be followed by a relayoutTableItems().
    // Also note that the position of the viewport needs to be adjusted
    // separately for it to overlap the loaded table.
    const QPointF innerDiff = loadedTableOuterRect.topLeft() - loadedTableInnerRect.topLeft();
    loadedTableOuterRect.moveTopLeft(newPosition);
    loadedTableInnerRect.moveTopLeft(newPosition + innerDiff);
}

QQuickTableViewPrivate::RebuildOptions QQuickTableViewPrivate::checkForVisibilityChanges()
{
    // This function will check if there are any visibility changes among
    // the _already loaded_ rows and columns. Note that there can be rows
    // and columns to the bottom or right that was not loaded, but should
    // now become visible (in case there is free space around the table).
    if (loadedItems.isEmpty()) {
        // Report no changes
        return RebuildOption::None;
    }

    RebuildOptions rebuildOptions = RebuildOption::None;

    if (loadedTableOuterRect.x() == origin.x() && leftColumn() != 0) {
        // Since the left column is at the origin of the viewport, but still not the first
        // column in the model, we need to calculate a new left column since there might be
        // columns in front of it that used to be hidden, but should now be visible (QTBUG-93264).
        rebuildOptions.setFlag(RebuildOption::ViewportOnly);
        rebuildOptions.setFlag(RebuildOption::CalculateNewTopLeftColumn);
    } else {
        // Go through all loaded columns from first to last, find the columns that used
        // to be hidden and not loaded, and check if they should become visible
        // (and vice versa). If there is a change, we need to rebuild.
        for (int column = leftColumn(); column <= rightColumn(); ++column) {
            const bool wasVisibleFromBefore = loadedColumns.contains(column);
            const bool isVisibleNow = !qFuzzyIsNull(getColumnWidth(column));
            if (wasVisibleFromBefore == isVisibleNow)
                continue;

            // A column changed visibility. This means that it should
            // either be loaded or unloaded. So we need a rebuild.
            qCDebug(lcTableViewDelegateLifecycle) << "Column" << column << "changed visibility to" << isVisibleNow;
            rebuildOptions.setFlag(RebuildOption::ViewportOnly);
            if (column == leftColumn()) {
                // The first loaded column should now be hidden. This means that we
                // need to calculate which column should now be first instead.
                rebuildOptions.setFlag(RebuildOption::CalculateNewTopLeftColumn);
            }
            break;
        }
    }

    if (loadedTableOuterRect.y() == origin.y() && topRow() != 0) {
        // Since the top row is at the origin of the viewport, but still not the first
        // row in the model, we need to calculate a new top row since there might be
        // rows in front of it that used to be hidden, but should now be visible (QTBUG-93264).
        rebuildOptions.setFlag(RebuildOption::ViewportOnly);
        rebuildOptions.setFlag(RebuildOption::CalculateNewTopLeftRow);
    } else {
        // Go through all loaded rows from first to last, find the rows that used
        // to be hidden and not loaded, and check if they should become visible
        // (and vice versa). If there is a change, we need to rebuild.
        for (int row = topRow(); row <= bottomRow(); ++row) {
            const bool wasVisibleFromBefore = loadedRows.contains(row);
            const bool isVisibleNow = !qFuzzyIsNull(getRowHeight(row));
            if (wasVisibleFromBefore == isVisibleNow)
                continue;

            // A row changed visibility. This means that it should
            // either be loaded or unloaded. So we need a rebuild.
            qCDebug(lcTableViewDelegateLifecycle) << "Row" << row << "changed visibility to" << isVisibleNow;
            rebuildOptions.setFlag(RebuildOption::ViewportOnly);
            if (row == topRow())
                rebuildOptions.setFlag(RebuildOption::CalculateNewTopLeftRow);
            break;
        }
    }

    return rebuildOptions;
}

void QQuickTableViewPrivate::forceLayout(bool immediate)
{
    clearEdgeSizeCache();
    RebuildOptions rebuildOptions = RebuildOption::None;

    const QSize actualTableSize = calculateTableSize();
    if (tableSize != actualTableSize) {
        // The table size will have changed if forceLayout is called after
        // the row count in the model has changed, but before we received
        // a rowsInsertedCallback about it (and vice versa for columns).
        rebuildOptions |= RebuildOption::ViewportOnly;
    }

    // Resizing a column (or row) can result in the table going from being
    // e.g completely inside the viewport to go outside. And in the latter
    // case, the user needs to be able to scroll the viewport, also if
    // flags such as Flickable.StopAtBounds is in use. So we need to
    // update contentWidth/Height to support that case.
    rebuildOptions |= RebuildOption::LayoutOnly
            | RebuildOption::CalculateNewContentWidth
            | RebuildOption::CalculateNewContentHeight
            | checkForVisibilityChanges();

    scheduleRebuildTable(rebuildOptions);

    if (immediate) {
        auto rootView = rootSyncView();
        const bool updated = rootView->d_func()->updateTableRecursive();
        if (!updated) {
            qWarning() << "TableView::forceLayout(): Cannot do an immediate re-layout during an ongoing layout!";
            rootView->polish();
        }
    }
}

void QQuickTableViewPrivate::syncLoadedTableFromLoadRequest()
{
    if (loadRequest.edge() == Qt::Edge(0)) {
        // No edge means we're loading the top-left item
        loadedColumns.insert(loadRequest.column());
        loadedRows.insert(loadRequest.row());
        return;
    }

    switch (loadRequest.edge()) {
    case Qt::LeftEdge:
    case Qt::RightEdge:
        loadedColumns.insert(loadRequest.column());
        break;
    case Qt::TopEdge:
    case Qt::BottomEdge:
        loadedRows.insert(loadRequest.row());
        break;
    }
}

FxTableItem *QQuickTableViewPrivate::loadedTableItem(const QPoint &cell) const
{
    const int modelIndex = modelIndexAtCell(cell);
    Q_TABLEVIEW_ASSERT(loadedItems.contains(modelIndex), modelIndex << cell);
    return loadedItems.value(modelIndex);
}

FxTableItem *QQuickTableViewPrivate::createFxTableItem(const QPoint &cell, QQmlIncubator::IncubationMode incubationMode)
{
    Q_Q(QQuickTableView);

    bool ownItem = false;
    int modelIndex = modelIndexAtCell(cell);

    QObject* object = model->object(modelIndex, incubationMode);
    if (!object) {
        if (model->incubationStatus(modelIndex) == QQmlIncubator::Loading) {
            // Item is incubating. Return nullptr for now, and let the table call this
            // function again once we get a callback to itemCreatedCallback().
            return nullptr;
        }

        qWarning() << "TableView: failed loading index:" << modelIndex;
        object = new QQuickItem();
        ownItem = true;
    }

    QQuickItem *item = qmlobject_cast<QQuickItem*>(object);
    if (!item) {
        // The model could not provide an QQuickItem for the
        // given index, so we create a placeholder instead.
        qWarning() << "TableView: delegate is not an item:" << modelIndex;
        model->release(object);
        item = new QQuickItem();
        ownItem = true;
    } else {
        QQuickAnchors *anchors = QQuickItemPrivate::get(item)->_anchors;
        if (anchors && anchors->activeDirections())
            qmlWarning(item) << "TableView: detected anchors on delegate with index: " << modelIndex
                             << ". Use implicitWidth and implicitHeight instead.";
    }

    if (ownItem) {
        // Parent item is normally set early on from initItemCallback (to
        // allow bindings to the parent property). But if we created the item
        // within this function, we need to set it explicit.
        item->setImplicitWidth(kDefaultColumnWidth);
        item->setImplicitHeight(kDefaultRowHeight);
        item->setParentItem(q->contentItem());
    }
    Q_TABLEVIEW_ASSERT(item->parentItem() == q->contentItem(), item->parentItem());

    FxTableItem *fxTableItem = new FxTableItem(item, q, ownItem);
    fxTableItem->setVisible(false);
    fxTableItem->cell = cell;
    fxTableItem->index = modelIndex;
    return fxTableItem;
}

FxTableItem *QQuickTableViewPrivate::loadFxTableItem(const QPoint &cell, QQmlIncubator::IncubationMode incubationMode)
{
#ifdef QT_DEBUG
    // Since TableView needs to work flawlessly when e.g incubating inside an async
    // loader, being able to override all loading to async while debugging can be helpful.
    static const bool forcedAsync = forcedIncubationMode == QLatin1String("async");
    if (forcedAsync)
        incubationMode = QQmlIncubator::Asynchronous;
#endif

    // Note that even if incubation mode is asynchronous, the item might
    // be ready immediately since the model has a cache of items.
    QBoolBlocker guard(blockItemCreatedCallback);
    auto item = createFxTableItem(cell, incubationMode);
    qCDebug(lcTableViewDelegateLifecycle) << cell << "ready?" << bool(item);
    return item;
}

void QQuickTableViewPrivate::releaseLoadedItems(QQmlTableInstanceModel::ReusableFlag reusableFlag) {
    // Make a copy and clear the list of items first to avoid destroyed
    // items being accessed during the loop (QTBUG-61294)
    auto const tmpList = loadedItems;
    loadedItems.clear();
    for (FxTableItem *item : tmpList)
        releaseItem(item, reusableFlag);
}

void QQuickTableViewPrivate::releaseItem(FxTableItem *fxTableItem, QQmlTableInstanceModel::ReusableFlag reusableFlag)
{
    Q_Q(QQuickTableView);
    // Note that fxTableItem->item might already have been destroyed, in case
    // the item is owned by the QML context rather than the model (e.g ObjectModel etc).
    auto item = fxTableItem->item;

    if (fxTableItem->ownItem) {
        Q_TABLEVIEW_ASSERT(item, fxTableItem->index);
        delete item;
    } else if (item) {
        auto releaseFlag = model->release(item, reusableFlag);
        if (releaseFlag == QQmlInstanceModel::Pooled) {
            fxTableItem->setVisible(false);

            // If the item (or a descendant) has focus, remove it, so
            // that the item doesn't enter with focus when it's reused.
            if (QQuickWindow *window = item->window()) {
                const auto focusItem = qobject_cast<QQuickItem *>(window->focusObject());
                if (focusItem) {
                    const bool hasFocus = item == focusItem || item->isAncestorOf(focusItem);
                    if (hasFocus) {
                        const auto focusChild = QQuickItemPrivate::get(q)->subFocusItem;
                        deliveryAgentPrivate()->clearFocusInScope(q, focusChild, Qt::OtherFocusReason);
                    }
                }
            }
        }
    }

    delete fxTableItem;
}

void QQuickTableViewPrivate::unloadItem(const QPoint &cell)
{
    const int modelIndex = modelIndexAtCell(cell);
    Q_TABLEVIEW_ASSERT(loadedItems.contains(modelIndex), modelIndex << cell);
    releaseItem(loadedItems.take(modelIndex), reusableFlag);
}

bool QQuickTableViewPrivate::canLoadTableEdge(Qt::Edge tableEdge, const QRectF fillRect) const
{
    switch (tableEdge) {
    case Qt::LeftEdge:
        return loadedTableOuterRect.left() > fillRect.left() + cellSpacing.width();
    case Qt::RightEdge:
        return loadedTableOuterRect.right() < fillRect.right() - cellSpacing.width();
    case Qt::TopEdge:
        return loadedTableOuterRect.top() > fillRect.top() + cellSpacing.height();
    case Qt::BottomEdge:
        return loadedTableOuterRect.bottom() < fillRect.bottom() - cellSpacing.height();
    }

    return false;
}

bool QQuickTableViewPrivate::canUnloadTableEdge(Qt::Edge tableEdge, const QRectF fillRect) const
{
    // Note: if there is only one row or column left, we cannot unload, since
    // they are needed as anchor point for further layouting. We also skip
    // unloading in the direction we're currently scrolling.

    switch (tableEdge) {
    case Qt::LeftEdge:
        if (loadedColumns.count() <= 1)
            return false;
        if (positionXAnimation.isRunning()) {
            const qreal to = positionXAnimation.to().toFloat();
            if (to < viewportRect.x())
                return false;
        }
        return loadedTableInnerRect.left() <= fillRect.left();
    case Qt::RightEdge:
        if (loadedColumns.count() <= 1)
            return false;
        if (positionXAnimation.isRunning()) {
            const qreal to = positionXAnimation.to().toFloat();
            if (to > viewportRect.x())
                return false;
        }
        return loadedTableInnerRect.right() >= fillRect.right();
    case Qt::TopEdge:
        if (loadedRows.count() <= 1)
            return false;
        if (positionYAnimation.isRunning()) {
            const qreal to = positionYAnimation.to().toFloat();
            if (to < viewportRect.y())
                return false;
        }
        return loadedTableInnerRect.top() <= fillRect.top();
    case Qt::BottomEdge:
        if (loadedRows.count() <= 1)
            return false;
        if (positionYAnimation.isRunning()) {
            const qreal to = positionYAnimation.to().toFloat();
            if (to > viewportRect.y())
                return false;
        }
        return loadedTableInnerRect.bottom() >= fillRect.bottom();
    }
    Q_TABLEVIEW_UNREACHABLE(tableEdge);
    return false;
}

Qt::Edge QQuickTableViewPrivate::nextEdgeToLoad(const QRectF rect)
{
    for (Qt::Edge edge : allTableEdges) {
        if (!canLoadTableEdge(edge, rect))
            continue;
        const int nextIndex = nextVisibleEdgeIndexAroundLoadedTable(edge);
        if (nextIndex == kEdgeIndexAtEnd)
            continue;
        return edge;
    }

    return Qt::Edge(0);
}

Qt::Edge QQuickTableViewPrivate::nextEdgeToUnload(const QRectF rect)
{
    for (Qt::Edge edge : allTableEdges) {
        if (canUnloadTableEdge(edge, rect))
            return edge;
    }
    return Qt::Edge(0);
}

qreal QQuickTableViewPrivate::cellWidth(const QPoint& cell) const
{
    // Using an items width directly is not an option, since we change
    // it during layout (which would also cause problems when recycling items).
    auto const cellItem = loadedTableItem(cell)->item;
    return cellItem->implicitWidth();
}

qreal QQuickTableViewPrivate::cellHeight(const QPoint& cell) const
{
    // Using an items height directly is not an option, since we change
    // it during layout (which would also cause problems when recycling items).
    auto const cellItem = loadedTableItem(cell)->item;
    return cellItem->implicitHeight();
}

qreal QQuickTableViewPrivate::sizeHintForColumn(int column) const
{
    // Find the widest cell in the column, and return its width
    qreal columnWidth = 0;
    for (const int row : loadedRows)
        columnWidth = qMax(columnWidth, cellWidth(QPoint(column, row)));

    return columnWidth;
}

qreal QQuickTableViewPrivate::sizeHintForRow(int row) const
{
    // Find the highest cell in the row, and return its height
    qreal rowHeight = 0;
    for (const int column : loadedColumns)
        rowHeight = qMax(rowHeight, cellHeight(QPoint(column, row)));
    return rowHeight;
}

QSize QQuickTableViewPrivate::calculateTableSize()
{
    QSize size(0, 0);
    if (tableModel)
        size = QSize(tableModel->columns(), tableModel->rows());
    else if (model)
        size = QSize(1, model->count());

    return isTransposed ? size.transposed() : size;
}

qreal QQuickTableViewPrivate::getColumnLayoutWidth(int column)
{
    // Return the column width specified by the application, or go
    // through the loaded items and calculate it as a fallback. For
    // layouting, the width can never be zero (or negative), as this
    // can lead us to be stuck in an infinite loop trying to load and
    // fill out the empty viewport space with empty columns.
    const qreal explicitColumnWidth = getColumnWidth(column);
    if (explicitColumnWidth >= 0)
        return explicitColumnWidth;

    if (syncHorizontally) {
        if (syncView->d_func()->loadedColumns.contains(column))
            return syncView->d_func()->getColumnLayoutWidth(column);
    }

    // Iterate over the currently visible items in the column. The downside
    // of doing that, is that the column width will then only be based on the implicit
    // width of the currently loaded items (which can be different depending on which
    // row you're at when the column is flicked in). The upshot is that you don't have to
    // bother setting columnWidthProvider for small tables, or if the implicit width doesn't vary.
    qreal columnWidth = sizeHintForColumn(column);

    if (qIsNaN(columnWidth) || columnWidth <= 0) {
        if (!layoutWarningIssued) {
            layoutWarningIssued = true;
            qmlWarning(q_func()) << "the delegate's implicitWidth needs to be greater than zero";
        }
        columnWidth = kDefaultColumnWidth;
    }

    return columnWidth;
}

qreal QQuickTableViewPrivate::getEffectiveRowY(int row) const
{
    // Return y pos of row after layout
    Q_TABLEVIEW_ASSERT(loadedRows.contains(row), row);
    return loadedTableItem(QPoint(leftColumn(), row))->geometry().y();
}

qreal QQuickTableViewPrivate::getEffectiveRowHeight(int row) const
{
    // Return row height after layout
    Q_TABLEVIEW_ASSERT(loadedRows.contains(row), row);
    return loadedTableItem(QPoint(leftColumn(), row))->geometry().height();
}

qreal QQuickTableViewPrivate::getEffectiveColumnX(int column) const
{
    // Return x pos of column after layout
    Q_TABLEVIEW_ASSERT(loadedColumns.contains(column), column);
    return loadedTableItem(QPoint(column, topRow()))->geometry().x();
}

qreal QQuickTableViewPrivate::getEffectiveColumnWidth(int column) const
{
    // Return column width after layout
    Q_TABLEVIEW_ASSERT(loadedColumns.contains(column), column);
    return loadedTableItem(QPoint(column, topRow()))->geometry().width();
}

qreal QQuickTableViewPrivate::getRowLayoutHeight(int row)
{
    // Return the row height specified by the application, or go
    // through the loaded items and calculate it as a fallback. For
    // layouting, the height can never be zero (or negative), as this
    // can lead us to be stuck in an infinite loop trying to load and
    // fill out the empty viewport space with empty rows.
    const qreal explicitRowHeight = getRowHeight(row);
    if (explicitRowHeight >= 0)
        return explicitRowHeight;

    if (syncVertically) {
        if (syncView->d_func()->loadedRows.contains(row))
            return syncView->d_func()->getRowLayoutHeight(row);
    }

    // Iterate over the currently visible items in the row. The downside
    // of doing that, is that the row height will then only be based on the implicit
    // height of the currently loaded items (which can be different depending on which
    // column you're at when the row is flicked in). The upshot is that you don't have to
    // bother setting rowHeightProvider for small tables, or if the implicit height doesn't vary.
    qreal rowHeight = sizeHintForRow(row);

    if (qIsNaN(rowHeight) || rowHeight <= 0) {
        if (!layoutWarningIssued) {
            layoutWarningIssued = true;
            qmlWarning(q_func()) << "the delegate's implicitHeight needs to be greater than zero";
        }
        rowHeight = kDefaultRowHeight;
    }

    return rowHeight;
}

qreal QQuickTableViewPrivate::getColumnWidth(int column) const
{
    // Return the width of the given column, if explicitly set. Return 0 if the column
    // is hidden, and -1 if the width is not set (which means that the width should
    // instead be calculated from the implicit size of the delegate items. This function
    // can be overridden by e.g HeaderView to provide the column widths by other means.
    Q_Q(const QQuickTableView);

    const int noExplicitColumnWidth = -1;

    if (cachedColumnWidth.startIndex == column)
        return cachedColumnWidth.size;

    if (syncHorizontally)
        return syncView->d_func()->getColumnWidth(column);

    if (columnWidthProvider.isUndefined()) {
        // We only respect explicit column widths when no columnWidthProvider
        // is set. Otherwise it's the responsibility of the provider to e.g
        // call explicitColumnWidth() (and implicitColumnWidth()), if needed.
        qreal explicitColumnWidth = q->explicitColumnWidth(column);
        if (explicitColumnWidth >= 0)
            return explicitColumnWidth;
        return noExplicitColumnWidth;
    }

    qreal columnWidth = noExplicitColumnWidth;

    if (columnWidthProvider.isCallable()) {
        auto const columnAsArgument = QJSValueList() << QJSValue(column);
        columnWidth = columnWidthProvider.call(columnAsArgument).toNumber();
        if (qIsNaN(columnWidth) || columnWidth < 0)
            columnWidth = noExplicitColumnWidth;
    } else {
        if (!layoutWarningIssued) {
            layoutWarningIssued = true;
            qmlWarning(q_func()) << "columnWidthProvider doesn't contain a function";
        }
        columnWidth = noExplicitColumnWidth;
    }

    cachedColumnWidth.startIndex = column;
    cachedColumnWidth.size = columnWidth;
    return columnWidth;
}

qreal QQuickTableViewPrivate::getRowHeight(int row) const
{
    // Return the height of the given row, if explicitly set. Return 0 if the row
    // is hidden, and -1 if the height is not set (which means that the height should
    // instead be calculated from the implicit size of the delegate items. This function
    // can be overridden by e.g HeaderView to provide the row heights by other means.
    Q_Q(const QQuickTableView);

    const int noExplicitRowHeight = -1;

    if (cachedRowHeight.startIndex == row)
        return cachedRowHeight.size;

    if (syncVertically)
        return syncView->d_func()->getRowHeight(row);

    if (rowHeightProvider.isUndefined()) {
        // We only resepect explicit row heights when no rowHeightProvider
        // is set. Otherwise it's the responsibility of the provider to e.g
        // call explicitRowHeight() (and implicitRowHeight()), if needed.
        qreal explicitRowHeight = q->explicitRowHeight(row);
        if (explicitRowHeight >= 0)
            return explicitRowHeight;
        return noExplicitRowHeight;
    }

    qreal rowHeight = noExplicitRowHeight;

    if (rowHeightProvider.isCallable()) {
        auto const rowAsArgument = QJSValueList() << QJSValue(row);
        rowHeight = rowHeightProvider.call(rowAsArgument).toNumber();
        if (qIsNaN(rowHeight) || rowHeight < 0)
            rowHeight = noExplicitRowHeight;
    } else {
        if (!layoutWarningIssued) {
            layoutWarningIssued = true;
            qmlWarning(q_func()) << "rowHeightProvider doesn't contain a function";
        }
        rowHeight = noExplicitRowHeight;
    }

    cachedRowHeight.startIndex = row;
    cachedRowHeight.size = rowHeight;
    return rowHeight;
}

qreal QQuickTableViewPrivate::getAlignmentContentX(int column, Qt::Alignment alignment, const qreal offset, const QRectF &subRect)
{
    Q_Q(QQuickTableView);

    qreal contentX = 0;
    const int columnX = getEffectiveColumnX(column);

    if (subRect.isValid()) {
        if (alignment == (Qt::AlignLeft | Qt::AlignRight)) {
            // Special case: Align to the right as long as the left
            // edge of the cell remains visible. Otherwise align to the left.
            alignment = subRect.width() > q->width() ? Qt::AlignLeft : Qt::AlignRight;
        }

        if (alignment & Qt::AlignLeft) {
            contentX = columnX + subRect.x() + offset;
        } else if (alignment & Qt::AlignRight) {
            contentX = columnX + subRect.right() - viewportRect.width() + offset;
        } else if (alignment & Qt::AlignHCenter) {
            const qreal centerDistance = (viewportRect.width() - subRect.width()) / 2;
            contentX = columnX + subRect.x() - centerDistance + offset;
        }
    } else {
        const int columnWidth = getEffectiveColumnWidth(column);
        if (alignment == (Qt::AlignLeft | Qt::AlignRight))
            alignment = columnWidth > q->width() ? Qt::AlignLeft : Qt::AlignRight;

        if (alignment & Qt::AlignLeft) {
            contentX = columnX + offset;
        } else if (alignment & Qt::AlignRight) {
            contentX = columnX + columnWidth - viewportRect.width() + offset;
        } else if (alignment & Qt::AlignHCenter) {
            const qreal centerDistance = (viewportRect.width() - columnWidth) / 2;
            contentX = columnX - centerDistance + offset;
        }
    }

    // Don't overshoot
    contentX = qBound(-q->minXExtent(), contentX, -q->maxXExtent());

    return contentX;
}

qreal QQuickTableViewPrivate::getAlignmentContentY(int row, Qt::Alignment alignment, const qreal offset, const QRectF &subRect)
{
    Q_Q(QQuickTableView);

    qreal contentY = 0;
    const int rowY = getEffectiveRowY(row);

    if (subRect.isValid()) {
        if (alignment == (Qt::AlignTop | Qt::AlignBottom)) {
            // Special case: Align to the bottom as long as the top
            // edge of the cell remains visible. Otherwise align to the top.
            alignment = subRect.height() > q->height() ? Qt::AlignTop : Qt::AlignBottom;
        }

        if (alignment & Qt::AlignTop) {
            contentY = rowY + subRect.y() + offset;
        } else if (alignment & Qt::AlignBottom) {
            contentY = rowY + subRect.bottom() - viewportRect.height() + offset;
        } else if (alignment & Qt::AlignVCenter) {
            const qreal centerDistance = (viewportRect.height() - subRect.height()) / 2;
            contentY = rowY + subRect.y() - centerDistance + offset;
        }
    } else {
        const int rowHeight = getEffectiveRowHeight(row);
        if (alignment == (Qt::AlignTop | Qt::AlignBottom))
            alignment = rowHeight > q->height() ? Qt::AlignTop : Qt::AlignBottom;

        if (alignment & Qt::AlignTop) {
            contentY = rowY + offset;
        } else if (alignment & Qt::AlignBottom) {
            contentY = rowY + rowHeight - viewportRect.height() + offset;
        } else if (alignment & Qt::AlignVCenter) {
            const qreal centerDistance = (viewportRect.height() - rowHeight) / 2;
            contentY = rowY - centerDistance + offset;
        }
    }

    // Don't overshoot
    contentY = qBound(-q->minYExtent(), contentY, -q->maxYExtent());

    return contentY;
}

bool QQuickTableViewPrivate::isColumnHidden(int column) const
{
    // A column is hidden if the width is explicit set to zero (either by
    // using a columnWidthProvider, or by overriding getColumnWidth()).
    return qFuzzyIsNull(getColumnWidth(column));
}

bool QQuickTableViewPrivate::isRowHidden(int row) const
{
    // A row is hidden if the height is explicit set to zero (either by
    // using a rowHeightProvider, or by overriding getRowHeight()).
    return qFuzzyIsNull(getRowHeight(row));
}

void QQuickTableViewPrivate::relayoutTableItems()
{
    qCDebug(lcTableViewDelegateLifecycle);

    if (viewportRect.isEmpty()) {
        // This can happen if TableView was resized down to have a zero size
        qCDebug(lcTableViewDelegateLifecycle()) << "Skipping relayout, viewport has zero size";
        return;
    }

    qreal nextColumnX = loadedTableOuterRect.x();
    qreal nextRowY = loadedTableOuterRect.y();

    for (const int column : loadedColumns) {
        // Adjust the geometry of all cells in the current column
        const qreal width = getColumnLayoutWidth(column);

        for (const int row : loadedRows) {
            auto item = loadedTableItem(QPoint(column, row));
            QRectF geometry = item->geometry();
            geometry.moveLeft(nextColumnX);
            geometry.setWidth(width);
            item->setGeometry(geometry);
        }

        if (width > 0)
            nextColumnX += width + cellSpacing.width();
    }

    for (const int row : loadedRows) {
        // Adjust the geometry of all cells in the current row
        const qreal height = getRowLayoutHeight(row);

        for (const int column : loadedColumns) {
            auto item = loadedTableItem(QPoint(column, row));
            QRectF geometry = item->geometry();
            geometry.moveTop(nextRowY);
            geometry.setHeight(height);
            item->setGeometry(geometry);
        }

        if (height > 0)
            nextRowY += height + cellSpacing.height();
    }

    if (Q_UNLIKELY(lcTableViewDelegateLifecycle().isDebugEnabled())) {
        for (const int column : loadedColumns) {
            for (const int row : loadedRows) {
                QPoint cell = QPoint(column, row);
                qCDebug(lcTableViewDelegateLifecycle()) << "relayout item:" << cell << loadedTableItem(cell)->geometry();
            }
        }
    }
}

void QQuickTableViewPrivate::layoutVerticalEdge(Qt::Edge tableEdge)
{
    int columnThatNeedsLayout;
    int neighbourColumn;
    qreal columnX;
    qreal columnWidth;

    if (tableEdge == Qt::LeftEdge) {
        columnThatNeedsLayout = leftColumn();
        neighbourColumn = loadedColumns.values().at(1);
        columnWidth = getColumnLayoutWidth(columnThatNeedsLayout);
        const auto neighbourItem = loadedTableItem(QPoint(neighbourColumn, topRow()));
        columnX = neighbourItem->geometry().left() - cellSpacing.width() - columnWidth;
    } else {
        columnThatNeedsLayout = rightColumn();
        neighbourColumn = loadedColumns.values().at(loadedColumns.count() - 2);
        columnWidth = getColumnLayoutWidth(columnThatNeedsLayout);
        const auto neighbourItem = loadedTableItem(QPoint(neighbourColumn, topRow()));
        columnX = neighbourItem->geometry().right() + cellSpacing.width();
    }

    for (const int row : loadedRows) {
        auto fxTableItem = loadedTableItem(QPoint(columnThatNeedsLayout, row));
        auto const neighbourItem = loadedTableItem(QPoint(neighbourColumn, row));
        const qreal rowY = neighbourItem->geometry().y();
        const qreal rowHeight = neighbourItem->geometry().height();

        fxTableItem->setGeometry(QRectF(columnX, rowY, columnWidth, rowHeight));
        fxTableItem->setVisible(true);

        qCDebug(lcTableViewDelegateLifecycle()) << "layout item:" << QPoint(columnThatNeedsLayout, row) << fxTableItem->geometry();
    }
}

void QQuickTableViewPrivate::layoutHorizontalEdge(Qt::Edge tableEdge)
{
    int rowThatNeedsLayout;
    int neighbourRow;

    if (tableEdge == Qt::TopEdge) {
        rowThatNeedsLayout = topRow();
        neighbourRow = loadedRows.values().at(1);
    } else {
        rowThatNeedsLayout = bottomRow();
        neighbourRow = loadedRows.values().at(loadedRows.count() - 2);
    }

    // Set the width first, since text items in QtQuick will calculate
    // implicitHeight based on the text items width.
    for (const int column : loadedColumns) {
        auto fxTableItem = loadedTableItem(QPoint(column, rowThatNeedsLayout));
        auto const neighbourItem = loadedTableItem(QPoint(column, neighbourRow));
        const qreal columnX = neighbourItem->geometry().x();
        const qreal columnWidth = neighbourItem->geometry().width();
        fxTableItem->item->setX(columnX);
        fxTableItem->item->setWidth(columnWidth);
    }

    qreal rowY;
    qreal rowHeight;
    if (tableEdge == Qt::TopEdge) {
        rowHeight = getRowLayoutHeight(rowThatNeedsLayout);
        const auto neighbourItem = loadedTableItem(QPoint(leftColumn(), neighbourRow));
        rowY = neighbourItem->geometry().top() - cellSpacing.height() - rowHeight;
    } else {
        rowHeight = getRowLayoutHeight(rowThatNeedsLayout);
        const auto neighbourItem = loadedTableItem(QPoint(leftColumn(), neighbourRow));
        rowY = neighbourItem->geometry().bottom() + cellSpacing.height();
    }

    for (const int column : loadedColumns) {
        auto fxTableItem = loadedTableItem(QPoint(column, rowThatNeedsLayout));
        fxTableItem->item->setY(rowY);
        fxTableItem->item->setHeight(rowHeight);
        fxTableItem->setVisible(true);

        qCDebug(lcTableViewDelegateLifecycle()) << "layout item:" << QPoint(column, rowThatNeedsLayout) << fxTableItem->geometry();
    }
}

void QQuickTableViewPrivate::layoutTopLeftItem()
{
    const QPoint cell(loadRequest.column(), loadRequest.row());
    auto topLeftItem = loadedTableItem(cell);
    auto item = topLeftItem->item;

    item->setPosition(loadRequest.startPosition());
    item->setSize(QSizeF(getColumnLayoutWidth(cell.x()), getRowLayoutHeight(cell.y())));
    topLeftItem->setVisible(true);
    qCDebug(lcTableViewDelegateLifecycle) << "geometry:" << topLeftItem->geometry();
}

void QQuickTableViewPrivate::layoutTableEdgeFromLoadRequest()
{
    if (loadRequest.edge() == Qt::Edge(0)) {
        // No edge means we're loading the top-left item
        layoutTopLeftItem();
        return;
    }

    switch (loadRequest.edge()) {
    case Qt::LeftEdge:
    case Qt::RightEdge:
        layoutVerticalEdge(loadRequest.edge());
        break;
    case Qt::TopEdge:
    case Qt::BottomEdge:
        layoutHorizontalEdge(loadRequest.edge());
        break;
    }
}

void QQuickTableViewPrivate::processLoadRequest()
{
    Q_Q(QQuickTableView);
    Q_TABLEVIEW_ASSERT(loadRequest.isActive(), "");

    while (loadRequest.hasCurrentCell()) {
        QPoint cell = loadRequest.currentCell();
        FxTableItem *fxTableItem = loadFxTableItem(cell, loadRequest.incubationMode());

        if (!fxTableItem) {
            // Requested item is not yet ready. Just leave, and wait for this
            // function to be called again when the item is ready.
            return;
        }

        loadedItems.insert(modelIndexAtCell(cell), fxTableItem);
        loadRequest.moveToNextCell();
    }

    qCDebug(lcTableViewDelegateLifecycle()) << "all items loaded!";

    syncLoadedTableFromLoadRequest();
    layoutTableEdgeFromLoadRequest();
    syncLoadedTableRectFromLoadedTable();

    if (rebuildState == RebuildState::Done) {
        // Loading of this edge was not done as a part of a rebuild, but
        // instead as an incremental build after e.g a flick.
        updateExtents();
        drainReusePoolAfterLoadRequest();

        switch (loadRequest.edge()) {
        case Qt::LeftEdge:
            emit q->leftColumnChanged();
            break;
        case Qt::RightEdge:
            emit q->rightColumnChanged();
            break;
        case Qt::TopEdge:
            emit q->topRowChanged();
            break;
        case Qt::BottomEdge:
            emit q->bottomRowChanged();
            break;
        }

        if (editIndex.isValid())
            updateEditItem();

        emit q->layoutChanged();
    }

    loadRequest.markAsDone();

    qCDebug(lcTableViewDelegateLifecycle()) << "current table:" << tableLayoutToString();
    qCDebug(lcTableViewDelegateLifecycle()) << "Load request completed!";
    qCDebug(lcTableViewDelegateLifecycle()) << "****************************************";
}

void QQuickTableViewPrivate::processRebuildTable()
{
    Q_Q(QQuickTableView);

    if (rebuildState == RebuildState::Begin) {
        if (Q_UNLIKELY(lcTableViewDelegateLifecycle().isDebugEnabled())) {
            qCDebug(lcTableViewDelegateLifecycle()) << "begin rebuild:" << q;
            if (rebuildOptions & RebuildOption::All)
                qCDebug(lcTableViewDelegateLifecycle()) << "RebuildOption::All, options:" << rebuildOptions;
            else if (rebuildOptions & RebuildOption::ViewportOnly)
                qCDebug(lcTableViewDelegateLifecycle()) << "RebuildOption::ViewportOnly, options:" << rebuildOptions;
            else if (rebuildOptions & RebuildOption::LayoutOnly)
                qCDebug(lcTableViewDelegateLifecycle()) << "RebuildOption::LayoutOnly, options:" << rebuildOptions;
            else
                Q_TABLEVIEW_UNREACHABLE(rebuildOptions);
        }

        tableSizeBeforeRebuild = tableSize;
        edgesBeforeRebuild = loadedItems.isEmpty() ? QMargins()
            : QMargins(q->leftColumn(), q->topRow(), q->rightColumn(), q->bottomRow());
    }

    moveToNextRebuildState();

    if (rebuildState == RebuildState::LoadInitalTable) {
        loadInitialTable();
        if (!moveToNextRebuildState())
            return;
    }

    if (rebuildState == RebuildState::VerifyTable) {
        if (loadedItems.isEmpty()) {
            qCDebug(lcTableViewDelegateLifecycle()) << "no items loaded!";
            updateContentWidth();
            updateContentHeight();
            rebuildState = RebuildState::Done;
        } else if (!moveToNextRebuildState()) {
            return;
        }
    }

    if (rebuildState == RebuildState::LayoutTable) {
        layoutAfterLoadingInitialTable();
        loadAndUnloadVisibleEdges();
        if (!moveToNextRebuildState())
            return;
    }

    if (rebuildState == RebuildState::CancelOvershoot) {
        cancelOvershootAfterLayout();
        loadAndUnloadVisibleEdges();
        if (!moveToNextRebuildState())
            return;
    }

    if (rebuildState == RebuildState::UpdateContentSize) {
        updateContentSize();
        if (!moveToNextRebuildState())
            return;
    }

    const bool preload = (rebuildOptions & RebuildOption::All
                          && reusableFlag == QQmlTableInstanceModel::Reusable);

    if (rebuildState == RebuildState::PreloadColumns) {
        if (preload && !atTableEnd(Qt::RightEdge))
            loadEdge(Qt::RightEdge, QQmlIncubator::AsynchronousIfNested);
        if (!moveToNextRebuildState())
            return;
    }

    if (rebuildState == RebuildState::PreloadRows) {
        if (preload && !atTableEnd(Qt::BottomEdge))
            loadEdge(Qt::BottomEdge, QQmlIncubator::AsynchronousIfNested);
        if (!moveToNextRebuildState())
            return;
    }

    if (rebuildState == RebuildState::MovePreloadedItemsToPool) {
        while (Qt::Edge edge = nextEdgeToUnload(viewportRect))
            unloadEdge(edge);
        if (!moveToNextRebuildState())
            return;
    }

    if (rebuildState == RebuildState::Done) {
        if (tableSizeBeforeRebuild.width() != tableSize.width())
            emit q->columnsChanged();
        if (tableSizeBeforeRebuild.height() != tableSize.height())
            emit q->rowsChanged();
        if (edgesBeforeRebuild.left() != q->leftColumn())
            emit q->leftColumnChanged();
        if (edgesBeforeRebuild.right() != q->rightColumn())
            emit q->rightColumnChanged();
        if (edgesBeforeRebuild.top() != q->topRow())
            emit q->topRowChanged();
        if (edgesBeforeRebuild.bottom() != q->bottomRow())
            emit q->bottomRowChanged();

        if (editIndex.isValid())
            updateEditItem();
        updateCurrentRowAndColumn();

        emit q->layoutChanged();

        qCDebug(lcTableViewDelegateLifecycle()) << "current table:" << tableLayoutToString();
        qCDebug(lcTableViewDelegateLifecycle()) << "rebuild completed!";
        qCDebug(lcTableViewDelegateLifecycle()) << "################################################";
        qCDebug(lcTableViewDelegateLifecycle());
    }

    Q_TABLEVIEW_ASSERT(rebuildState == RebuildState::Done, int(rebuildState));
}

bool QQuickTableViewPrivate::moveToNextRebuildState()
{
    if (loadRequest.isActive()) {
        // Items are still loading async, which means
        // that the current state is not yet done.
        return false;
    }

    if (rebuildState == RebuildState::Begin
            && rebuildOptions.testFlag(RebuildOption::LayoutOnly))
        rebuildState = RebuildState::LayoutTable;
    else
        rebuildState = RebuildState(int(rebuildState) + 1);

    qCDebug(lcTableViewDelegateLifecycle()) << int(rebuildState);
    return true;
}

void QQuickTableViewPrivate::calculateTopLeft(QPoint &topLeftCell, QPointF &topLeftPos)
{
    if (tableSize.isEmpty()) {
        // There is no cell that can be top left
        topLeftCell.rx() = kEdgeIndexAtEnd;
        topLeftCell.ry() = kEdgeIndexAtEnd;
        return;
    }

    if (syncHorizontally || syncVertically) {
        const auto syncView_d = syncView->d_func();

        if (syncView_d->loadedItems.isEmpty()) {
            topLeftCell.rx() = 0;
            topLeftCell.ry() = 0;
            return;
        }

        // Get sync view top left, and use that as our own top left (if possible)
        const QPoint syncViewTopLeftCell(syncView_d->leftColumn(), syncView_d->topRow());
        const auto syncViewTopLeftFxItem = syncView_d->loadedTableItem(syncViewTopLeftCell);
        const QPointF syncViewTopLeftPos = syncViewTopLeftFxItem->geometry().topLeft();

        if (syncHorizontally) {
            topLeftCell.rx() = syncViewTopLeftCell.x();
            topLeftPos.rx() = syncViewTopLeftPos.x();

            if (topLeftCell.x() >= tableSize.width()) {
                // Top left is outside our own model.
                topLeftCell.rx() = kEdgeIndexAtEnd;
                topLeftPos.rx() = kEdgeIndexAtEnd;
            }
        }

        if (syncVertically) {
            topLeftCell.ry() = syncViewTopLeftCell.y();
            topLeftPos.ry() = syncViewTopLeftPos.y();

            if (topLeftCell.y() >= tableSize.height()) {
                // Top left is outside our own model.
                topLeftCell.ry() = kEdgeIndexAtEnd;
                topLeftPos.ry() = kEdgeIndexAtEnd;
            }
        }

        if (syncHorizontally && syncVertically) {
            // We have a valid top left, so we're done
            return;
        }
    }

    // Since we're not sync-ing both horizontal and vertical, calculate the missing
    // dimention(s) ourself. If we rebuild all, we find the first visible top-left
    // item starting from cell(0, 0). Otherwise, guesstimate which row or column that
    // should be the new top-left given the geometry of the viewport.

    if (!syncHorizontally) {
        if (rebuildOptions & RebuildOption::All) {
            // Find the first visible column from the beginning
            topLeftCell.rx() = nextVisibleEdgeIndex(Qt::RightEdge, 0);
            if (topLeftCell.x() == kEdgeIndexAtEnd) {
                // No visible column found
                return;
            }
        } else if (rebuildOptions & RebuildOption::CalculateNewTopLeftColumn) {
            // Guesstimate new top left
            const int newColumn = int(viewportRect.x() / (averageEdgeSize.width() + cellSpacing.width()));
            topLeftCell.rx() = qBound(0, newColumn, tableSize.width() - 1);
            topLeftPos.rx() = topLeftCell.x() * (averageEdgeSize.width() + cellSpacing.width());
        } else if (rebuildOptions & RebuildOption::PositionViewAtColumn) {
            topLeftCell.rx() = qBound(0, positionViewAtColumnAfterRebuild, tableSize.width() - 1);
            topLeftPos.rx() = qFloor(topLeftCell.x()) * (averageEdgeSize.width() + cellSpacing.width());
        } else {
            // Keep the current top left, unless it's outside model
            topLeftCell.rx() = qBound(0, leftColumn(), tableSize.width() - 1);
            // We begin by loading the columns where the viewport is at
            // now. But will move the whole table and the viewport
            // later, when we do a layoutAfterLoadingInitialTable().
            topLeftPos.rx() = loadedTableOuterRect.x();
        }
    }

    if (!syncVertically) {
        if (rebuildOptions & RebuildOption::All) {
            // Find the first visible row from the beginning
            topLeftCell.ry() = nextVisibleEdgeIndex(Qt::BottomEdge, 0);
            if (topLeftCell.y() == kEdgeIndexAtEnd) {
                // No visible row found
                return;
            }
        } else if (rebuildOptions & RebuildOption::CalculateNewTopLeftRow) {
            // Guesstimate new top left
            const int newRow = int(viewportRect.y() / (averageEdgeSize.height() + cellSpacing.height()));
            topLeftCell.ry() = qBound(0, newRow, tableSize.height() - 1);
            topLeftPos.ry() = topLeftCell.y() * (averageEdgeSize.height() + cellSpacing.height());
        } else if (rebuildOptions & RebuildOption::PositionViewAtRow) {
            topLeftCell.ry() = qBound(0, positionViewAtRowAfterRebuild, tableSize.height() - 1);
            topLeftPos.ry() = qFloor(topLeftCell.y()) * (averageEdgeSize.height() + cellSpacing.height());
        } else {
            topLeftCell.ry() = qBound(0, topRow(), tableSize.height() - 1);
            topLeftPos.ry() = loadedTableOuterRect.y();
        }
    }
}

void QQuickTableViewPrivate::loadInitialTable()
{
    tableSize = calculateTableSize();

    if (positionXAnimation.isRunning()) {
        positionXAnimation.stop();
        setLocalViewportX(positionXAnimation.to().toReal());
        syncViewportRect();
    }

    if (positionYAnimation.isRunning()) {
        positionYAnimation.stop();
        setLocalViewportY(positionYAnimation.to().toReal());
        syncViewportRect();
    }

    QPoint topLeft;
    QPointF topLeftPos;
    calculateTopLeft(topLeft, topLeftPos);
    qCDebug(lcTableViewDelegateLifecycle()) << "initial viewport rect:" << viewportRect;
    qCDebug(lcTableViewDelegateLifecycle()) << "initial top left cell:" << topLeft << ", pos:" << topLeftPos;

    if (!loadedItems.isEmpty()) {
        if (rebuildOptions & RebuildOption::All)
            releaseLoadedItems(QQmlTableInstanceModel::NotReusable);
        else if (rebuildOptions & RebuildOption::ViewportOnly)
            releaseLoadedItems(reusableFlag);
    }

    if (rebuildOptions & RebuildOption::All) {
        origin = QPointF(0, 0);
        endExtent = QSizeF(0, 0);
        hData.markExtentsDirty();
        vData.markExtentsDirty();
        updateBeginningEnd();
    }

    loadedColumns.clear();
    loadedRows.clear();
    loadedTableOuterRect = QRect();
    loadedTableInnerRect = QRect();
    clearEdgeSizeCache();

    if (syncHorizontally)
        setLocalViewportX(syncView->contentX());

    if (syncVertically)
        setLocalViewportY(syncView->contentY());

    if (!syncHorizontally && rebuildOptions & RebuildOption::PositionViewAtColumn)
        setLocalViewportX(topLeftPos.x());

    if (!syncVertically && rebuildOptions & RebuildOption::PositionViewAtRow)
        setLocalViewportY(topLeftPos.y());

    syncViewportRect();

    if (!model) {
        qCDebug(lcTableViewDelegateLifecycle()) << "no model found, leaving table empty";
        return;
    }

    if (model->count() == 0) {
        qCDebug(lcTableViewDelegateLifecycle()) << "empty model found, leaving table empty";
        return;
    }

    if (tableModel && !tableModel->delegate()) {
        qCDebug(lcTableViewDelegateLifecycle()) << "no delegate found, leaving table empty";
        return;
    }

    if (topLeft.x() == kEdgeIndexAtEnd || topLeft.y() == kEdgeIndexAtEnd) {
        qCDebug(lcTableViewDelegateLifecycle()) << "no visible row or column found, leaving table empty";
        return;
    }

    if (topLeft.x() == kEdgeIndexNotSet || topLeft.y() == kEdgeIndexNotSet) {
        qCDebug(lcTableViewDelegateLifecycle()) << "could not resolve top-left item, leaving table empty";
        return;
    }

    if (viewportRect.isEmpty()) {
        qCDebug(lcTableViewDelegateLifecycle()) << "viewport has zero size, leaving table empty";
        return;
    }

    // Load top-left item. After loaded, loadItemsInsideRect() will take
    // care of filling out the rest of the table.
    loadRequest.begin(topLeft, topLeftPos, QQmlIncubator::AsynchronousIfNested);
    processLoadRequest();
    loadAndUnloadVisibleEdges();
}

void QQuickTableViewPrivate::updateContentSize()
{
    const bool allColumnsLoaded = atTableEnd(Qt::LeftEdge) && atTableEnd(Qt::RightEdge);
    if (rebuildOptions.testFlag(RebuildOption::CalculateNewContentWidth) || allColumnsLoaded) {
        updateAverageColumnWidth();
        updateContentWidth();
    }

    const bool allRowsLoaded = atTableEnd(Qt::TopEdge) && atTableEnd(Qt::BottomEdge);
    if (rebuildOptions.testFlag(RebuildOption::CalculateNewContentHeight) || allRowsLoaded) {
        updateAverageRowHeight();
        updateContentHeight();
    }

    updateExtents();
}

void QQuickTableViewPrivate::layoutAfterLoadingInitialTable()
{
    clearEdgeSizeCache();
    relayoutTableItems();
    syncLoadedTableRectFromLoadedTable();

    updateContentSize();

    adjustViewportXAccordingToAlignment();
    adjustViewportYAccordingToAlignment();
}

void QQuickTableViewPrivate::adjustViewportXAccordingToAlignment()
{
    // Check if we are supposed to position the viewport at a certain column
    if (!rebuildOptions.testFlag(RebuildOption::PositionViewAtColumn))
        return;
    // The requested column might have been hidden or is outside model bounds
    if (positionViewAtColumnAfterRebuild != leftColumn())
        return;

    const qreal newContentX = getAlignmentContentX(
                positionViewAtColumnAfterRebuild,
                positionViewAtColumnAlignment,
                positionViewAtColumnOffset,
                positionViewAtColumnSubRect);

    setLocalViewportX(newContentX);
    syncViewportRect();
}

void QQuickTableViewPrivate::adjustViewportYAccordingToAlignment()
{
    // Check if we are supposed to position the viewport at a certain row
    if (!rebuildOptions.testFlag(RebuildOption::PositionViewAtRow))
        return;
    // The requested row might have been hidden or is outside model bounds
    if (positionViewAtRowAfterRebuild != topRow())
        return;

    const qreal newContentY = getAlignmentContentY(
                positionViewAtRowAfterRebuild,
                positionViewAtRowAlignment,
                positionViewAtRowOffset,
                positionViewAtRowSubRect);

    setLocalViewportY(newContentY);
    syncViewportRect();
}

void QQuickTableViewPrivate::cancelOvershootAfterLayout()
{
    Q_Q(QQuickTableView);

    // Note: we only want to cancel overshoot from a rebuild if we're supposed to position
    // the view on a specific cell. The app is allowed to overshoot by setting contentX and
    // contentY manually. Also, if this view is a sync child, we should always stay in sync
    // with the syncView, so then we don't do anything.
    const bool positionVertically = rebuildOptions.testFlag(RebuildOption::PositionViewAtRow);
    const bool positionHorizontally = rebuildOptions.testFlag(RebuildOption::PositionViewAtColumn);
    const bool cancelVertically = positionVertically && !syncVertically;
    const bool cancelHorizontally = positionHorizontally && !syncHorizontally;

    if (cancelHorizontally && !qFuzzyIsNull(q->horizontalOvershoot())) {
        qCDebug(lcTableViewDelegateLifecycle()) << "cancelling overshoot horizontally:" << q->horizontalOvershoot();
        setLocalViewportX(q->horizontalOvershoot() < 0 ? -q->minXExtent() : -q->maxXExtent());
        syncViewportRect();
    }

    if (cancelVertically && !qFuzzyIsNull(q->verticalOvershoot())) {
        qCDebug(lcTableViewDelegateLifecycle()) << "cancelling overshoot vertically:" << q->verticalOvershoot();
        setLocalViewportY(q->verticalOvershoot() < 0 ? -q->minYExtent() : -q->maxYExtent());
        syncViewportRect();
    }
}

void QQuickTableViewPrivate::unloadEdge(Qt::Edge edge)
{
    Q_Q(QQuickTableView);
    qCDebug(lcTableViewDelegateLifecycle) << edge;

    switch (edge) {
    case Qt::LeftEdge: {
        const int column = leftColumn();
        for (int row : loadedRows)
            unloadItem(QPoint(column, row));
        loadedColumns.remove(column);
        syncLoadedTableRectFromLoadedTable();
        if (rebuildState == RebuildState::Done)
            emit q->leftColumnChanged();
        break; }
    case Qt::RightEdge: {
        const int column = rightColumn();
        for (int row : loadedRows)
            unloadItem(QPoint(column, row));
        loadedColumns.remove(column);
        syncLoadedTableRectFromLoadedTable();
        if (rebuildState == RebuildState::Done)
            emit q->rightColumnChanged();
        break; }
    case Qt::TopEdge: {
        const int row = topRow();
        for (int col : loadedColumns)
            unloadItem(QPoint(col, row));
        loadedRows.remove(row);
        syncLoadedTableRectFromLoadedTable();
        if (rebuildState == RebuildState::Done)
            emit q->topRowChanged();
        break; }
    case Qt::BottomEdge: {
        const int row = bottomRow();
        for (int col : loadedColumns)
            unloadItem(QPoint(col, row));
        loadedRows.remove(row);
        syncLoadedTableRectFromLoadedTable();
        if (rebuildState == RebuildState::Done)
            emit q->bottomRowChanged();
        break; }
    }

    if (rebuildState == RebuildState::Done)
        emit q->layoutChanged();

    qCDebug(lcTableViewDelegateLifecycle) << tableLayoutToString();
}

void QQuickTableViewPrivate::loadEdge(Qt::Edge edge, QQmlIncubator::IncubationMode incubationMode)
{
    const int edgeIndex = nextVisibleEdgeIndexAroundLoadedTable(edge);
    qCDebug(lcTableViewDelegateLifecycle) << edge << edgeIndex <<  q_func();

    const auto &visibleCells = edge & (Qt::LeftEdge | Qt::RightEdge)
            ? loadedRows.values() : loadedColumns.values();
    loadRequest.begin(edge, edgeIndex, visibleCells, incubationMode);
    processLoadRequest();
}

void QQuickTableViewPrivate::loadAndUnloadVisibleEdges(QQmlIncubator::IncubationMode incubationMode)
{
    // Unload table edges that have been moved outside the visible part of the
    // table (including buffer area), and load new edges that has been moved inside.
    // Note: an important point is that we always keep the table rectangular
    // and without holes to reduce complexity (we never leave the table in
    // a half-loaded state, or keep track of multiple patches).
    // We load only one edge (row or column) at a time. This is especially
    // important when loading into the buffer, since we need to be able to
    // cancel the buffering quickly if the user starts to flick, and then
    // focus all further loading on the edges that are flicked into view.

    if (loadRequest.isActive()) {
        // Don't start loading more edges while we're
        // already waiting for another one to load.
        return;
    }

    if (loadedItems.isEmpty()) {
        // We need at least the top-left item to be loaded before we can
        // start loading edges around it. Not having a top-left item at
        // this point means that the model is empty (or no delegate).
        return;
    }

    bool tableModified;

    do {
        tableModified = false;

        if (Qt::Edge edge = nextEdgeToUnload(viewportRect)) {
            tableModified = true;
            unloadEdge(edge);
        }

        if (Qt::Edge edge = nextEdgeToLoad(viewportRect)) {
            tableModified = true;
            loadEdge(edge, incubationMode);
            if (loadRequest.isActive())
                return;
        }
    } while (tableModified);

}

void QQuickTableViewPrivate::drainReusePoolAfterLoadRequest()
{
    Q_Q(QQuickTableView);

    if (reusableFlag == QQmlTableInstanceModel::NotReusable || !tableModel)
        return;

    if (!qFuzzyIsNull(q->verticalOvershoot()) || !qFuzzyIsNull(q->horizontalOvershoot())) {
        // Don't drain while we're overshooting, since this will fill up the
        // pool, but we expect to reuse them all once the content item moves back.
        return;
    }

    // When loading edges, we don't want to drain the reuse pool too aggressively. Normally,
    // all the items in the pool are reused rapidly as the content view is flicked around
    // anyway. Even if the table is temporarily flicked to a section that contains fewer
    // cells than what used to be (e.g if the flicked-in rows are taller than average), it
    // still makes sense to keep all the items in circulation; Chances are, that soon enough,
    // thinner rows are flicked back in again (meaning that we can fit more items into the
    // view). But at the same time, if a delegate chooser is in use, the pool might contain
    // items created from different delegates. And some of those delegates might be used only
    // occasionally. So to avoid situations where an item ends up in the pool for too long, we
    // call drain after each load request, but with a sufficiently large pool time. (If an item
    // in the pool has a large pool time, it means that it hasn't been reused for an equal
    // amount of load cycles, and should be released).
    //
    // We calculate an appropriate pool time by figuring out what the minimum time must be to
    // not disturb frequently reused items. Since the number of items in a row might be higher
    // than in a column (or vice versa), the minimum pool time should take into account that
    // you might be flicking out a single row (filling up the pool), before you continue
    // flicking in several new columns (taking them out again, but now in smaller chunks). This
    // will increase the number of load cycles items are kept in the pool (poolTime), but still,
    // we shouldn't release them, as they are still being reused frequently.
    // To get a flexible maxValue (that e.g tolerates rows and columns being flicked
    // in with varying sizes, causing some items not to be resued immediately), we multiply the
    // value by 2. Note that we also add an extra +1 to the column count, because the number of
    // visible columns will fluctuate between +1/-1 while flicking.
    const int w = loadedColumns.count();
    const int h = loadedRows.count();
    const int minTime = int(std::ceil(w > h ? qreal(w + 1) / h : qreal(h + 1) / w));
    const int maxTime = minTime * 2;
    tableModel->drainReusableItemsPool(maxTime);
}

void QQuickTableViewPrivate::scheduleRebuildTable(RebuildOptions options) {
    if (!q_func()->isComponentComplete()) {
        // We'll rebuild the table once complete anyway
        return;
    }

    scheduledRebuildOptions |= options;
    q_func()->polish();
}

QQuickTableView *QQuickTableViewPrivate::rootSyncView() const
{
    QQuickTableView *root = const_cast<QQuickTableView *>(q_func());
    while (QQuickTableView *view = root->d_func()->syncView)
        root = view;
    return root;
}

void QQuickTableViewPrivate::updatePolish()
{
    // We always start updating from the top of the syncView tree, since
    // the layout of a syncView child will depend on the layout of the syncView.
    //  E.g when a new column is flicked in, the syncView should load and layout
    // the column first, before any syncChildren gets a chance to do the same.
    Q_TABLEVIEW_ASSERT(!polishing, "recursive updatePolish() calls are not allowed!");
    rootSyncView()->d_func()->updateTableRecursive();
}

bool QQuickTableViewPrivate::updateTableRecursive()
{
    if (polishing) {
        // We're already updating the Table in this view, so
        // we cannot continue. Signal this back by returning false.
        // The caller can then choose to call "polish()" instead, to
        // do the update later.
        return false;
    }

    const bool updateComplete = updateTable();
    if (!updateComplete)
        return false;

    const auto children = syncChildren;
    for (auto syncChild : children) {
        auto syncChild_d = syncChild->d_func();
        const int mask =
                RebuildOption::PositionViewAtRow |
                RebuildOption::PositionViewAtColumn |
                RebuildOption::CalculateNewTopLeftRow |
                RebuildOption::CalculateNewTopLeftColumn;
        syncChild_d->scheduledRebuildOptions |= rebuildOptions & ~mask;

        const bool descendantUpdateComplete = syncChild_d->updateTableRecursive();
        if (!descendantUpdateComplete)
            return false;
    }

    rebuildOptions = RebuildOption::None;

    return true;
}

bool QQuickTableViewPrivate::updateTable()
{
    // Whenever something changes, e.g viewport moves, spacing is set to a
    // new value, model changes etc, this function will end up being called. Here
    // we check what needs to be done, and load/unload cells accordingly.
    // If we cannot complete the update (because we need to wait for an item
    // to load async), we return false.

    Q_TABLEVIEW_ASSERT(!polishing, "recursive updatePolish() calls are not allowed!");
    QBoolBlocker polishGuard(polishing, true);

    if (loadRequest.isActive()) {
        // We're currently loading items async to build a new edge in the table. We see the loading
        // as an atomic operation, which means that we don't continue doing anything else until all
        // items have been received and laid out. Note that updatePolish is then called once more
        // after the loadRequest has completed to handle anything that might have occurred in-between.
        return false;
    }

    if (rebuildState != RebuildState::Done) {
        processRebuildTable();
        return rebuildState == RebuildState::Done;
    }

    syncWithPendingChanges();

    if (rebuildState == RebuildState::Begin) {
        processRebuildTable();
        return rebuildState == RebuildState::Done;
    }

    if (loadedItems.isEmpty())
        return !loadRequest.isActive();

    loadAndUnloadVisibleEdges();
    updateEditItem();

    return !loadRequest.isActive();
}

void QQuickTableViewPrivate::fixup(QQuickFlickablePrivate::AxisData &data, qreal minExtent, qreal maxExtent)
{
    if (inUpdateContentSize) {
        // We update the content size dynamically as we load and unload edges.
        // Unfortunately, this also triggers a call to this function. The base
        // implementation will do things like start a momentum animation or move
        // the content view somewhere else, which causes glitches. This can
        // especially happen if flicking on one of the syncView children, which triggers
        // an update to our content size. In that case, the base implementation don't know
        // that the view is being indirectly dragged, and will therefore do strange things as
        // it tries to 'fixup' the geometry. So we use a guard to prevent this from happening.
        return;
    }

    QQuickFlickablePrivate::fixup(data, minExtent, maxExtent);
}

QTypeRevision QQuickTableViewPrivate::resolveImportVersion()
{
    const auto data = QQmlData::get(q_func());
    if (!data || !data->propertyCache)
        return QTypeRevision::zero();

    const auto cppMetaObject = data->propertyCache->firstCppMetaObject();
    const auto qmlTypeView = QQmlMetaType::qmlType(cppMetaObject);

    // TODO: did we rather want qmlTypeView.revision() here?
    return qmlTypeView.metaObjectRevision();
}

void QQuickTableViewPrivate::createWrapperModel()
{
    Q_Q(QQuickTableView);
    // When the assigned model is not an instance model, we create a wrapper
    // model (QQmlTableInstanceModel) that keeps a pointer to both the
    // assigned model and the assigned delegate. This model will give us a
    // common interface to any kind of model (js arrays, QAIM, number etc), and
    // help us create delegate instances.
    tableModel = new QQmlTableInstanceModel(qmlContext(q));
    tableModel->useImportVersion(resolveImportVersion());
    model = tableModel;
}

bool QQuickTableViewPrivate::selectedInSelectionModel(const QPoint &cell) const
{
    if (!selectionModel)
        return false;

    QAbstractItemModel *model = selectionModel->model();
    if (!model)
        return false;

    return selectionModel->isSelected(q_func()->modelIndex(cell));
}

bool QQuickTableViewPrivate::currentInSelectionModel(const QPoint &cell) const
{
    if (!selectionModel)
        return false;

    QAbstractItemModel *model = selectionModel->model();
    if (!model)
        return false;

    return selectionModel->currentIndex() == q_func()->modelIndex(cell);
}

void QQuickTableViewPrivate::selectionChangedInSelectionModel(const QItemSelection &selected, const QItemSelection &deselected)
{
    if (!selectionModel->hasSelection()) {
        // Ensure that we cancel any ongoing key/mouse-based selections
        // if selectionModel.clearSelection() is called.
        clearSelection();
    }

    const auto &selectedIndexes = selected.indexes();
    const auto &deselectedIndexes = deselected.indexes();
    for (int i = 0; i < selectedIndexes.size(); ++i)
        setSelectedOnDelegateItem(selectedIndexes.at(i), true);
    for (int i = 0; i < deselectedIndexes.size(); ++i)
        setSelectedOnDelegateItem(deselectedIndexes.at(i), false);
}

void QQuickTableViewPrivate::setSelectedOnDelegateItem(const QModelIndex &modelIndex, bool select)
{
    if (modelIndex.isValid() && modelIndex.model() != selectionSourceModel()) {
        qmlWarning(q_func())
                << "Cannot select cells: TableView.selectionModel.model is not "
                << "compatible with the model displayed in the view";
        return;
    }

    const int cellIndex = modelIndexToCellIndex(modelIndex);
    if (!loadedItems.contains(cellIndex))
        return;
    const QPoint cell = cellAtModelIndex(cellIndex);
    QQuickItem *item = loadedTableItem(cell)->item;
    setRequiredProperty(kRequiredProperty_selected, QVariant::fromValue(select), cellIndex, item, false);
}

QAbstractItemModel *QQuickTableViewPrivate::selectionSourceModel()
{
    // TableView.selectionModel.model should always be the same as TableView.model.
    // After all, when the user selects an index in the view, the same index should
    // be selected in the selection model. We therefore set the model in
    // selectionModel.model automatically.
    // But it's not always the case that the model shown in the view is the same
    // as TableView.model. Subclasses with a proxy model will instead show the
    // proxy model (e.g TreeView and HeaderView). And then it's no longer clear if
    // we should use the proxy model or the TableView.model as source model in
    // TableView.selectionModel. It's up to the subclass. But in short, if the proxy
    // model shares the same model items as TableView.model (just with e.g a filter
    // applied, or sorted etc), then TableView.model should be used. If the proxy
    // model is a completely different model that shares no model items with
    // TableView.model, then the proxy model should be used (e.g HeaderView).
    return qaim(modelImpl());
}

QAbstractItemModel *QQuickTableViewPrivate::qaim(QVariant modelAsVariant) const
{
    // If modelAsVariant wraps a qaim, return it
    if (modelAsVariant.userType() == qMetaTypeId<QJSValue>())
        modelAsVariant = modelAsVariant.value<QJSValue>().toVariant();
    return qvariant_cast<QAbstractItemModel *>(modelAsVariant);
}

void QQuickTableViewPrivate::updateSelectedOnAllDelegateItems()
{
    updateCurrentRowAndColumn();

    for (auto it = loadedItems.keyBegin(), end = loadedItems.keyEnd(); it != end; ++it) {
        const int cellIndex = *it;
        const QPoint cell = cellAtModelIndex(cellIndex);
        const bool selected = selectedInSelectionModel(cell);
        const bool current = currentInSelectionModel(cell);
        QQuickItem *item = loadedTableItem(cell)->item;
        const bool editing = editIndex == q_func()->modelIndex(cell);
        setRequiredProperty(kRequiredProperty_selected, QVariant::fromValue(selected), cellIndex, item, false);
        setRequiredProperty(kRequiredProperty_current, QVariant::fromValue(current), cellIndex, item, false);
        setRequiredProperty(kRequiredProperty_editing, QVariant::fromValue(editing), cellIndex, item, false);
    }
}

void QQuickTableViewPrivate::currentChangedInSelectionModel(const QModelIndex &current, const QModelIndex &previous)
{
    if (current.isValid() && current.model() != selectionSourceModel()) {
        qmlWarning(q_func())
                << "Cannot change current index: TableView.selectionModel.model is not "
                << "compatible with the model displayed in the view";
        return;
    }

    updateCurrentRowAndColumn();
    setCurrentOnDelegateItem(previous, false);
    setCurrentOnDelegateItem(current, true);
}

void QQuickTableViewPrivate::updateCurrentRowAndColumn()
{
    Q_Q(QQuickTableView);

    const QModelIndex currentIndex = selectionModel ? selectionModel->currentIndex() : QModelIndex();
    const QPoint currentCell = q->cellAtIndex(currentIndex);
    if (currentCell.x() != currentColumn) {
        currentColumn = currentCell.x();
        emit q->currentColumnChanged();
    }

    if (currentCell.y() != currentRow) {
        currentRow = currentCell.y();
        emit q->currentRowChanged();
    }
}

void QQuickTableViewPrivate::setCurrentOnDelegateItem(const QModelIndex &index, bool isCurrent)
{
    const int cellIndex = modelIndexToCellIndex(index);
    if (!loadedItems.contains(cellIndex))
        return;

    const QPoint cell = cellAtModelIndex(cellIndex);
    QQuickItem *item = loadedTableItem(cell)->item;
    setRequiredProperty(kRequiredProperty_current, QVariant::fromValue(isCurrent), cellIndex, item, false);
}

void QQuickTableViewPrivate::itemCreatedCallback(int modelIndex, QObject*)
{
    if (blockItemCreatedCallback)
        return;

    qCDebug(lcTableViewDelegateLifecycle) << "item done loading:"
        << cellAtModelIndex(modelIndex);

    // Since the item we waited for has finished incubating, we can
    // continue with the load request. processLoadRequest will
    // ask the model for the requested item once more, which will be
    // quick since the model has cached it.
    processLoadRequest();
    loadAndUnloadVisibleEdges();
    updatePolish();
}

void QQuickTableViewPrivate::initItemCallback(int modelIndex, QObject *object)
{
    Q_Q(QQuickTableView);

    auto item = qobject_cast<QQuickItem*>(object);
    if (!item)
        return;

    item->setParentItem(q->contentItem());
    item->setZ(1);

    const QPoint cell = cellAtModelIndex(modelIndex);
    const bool current = currentInSelectionModel(cell);
    const bool selected = selectedInSelectionModel(cell);
    setRequiredProperty(kRequiredProperty_current, QVariant::fromValue(current), modelIndex, object, true);
    setRequiredProperty(kRequiredProperty_selected, QVariant::fromValue(selected), modelIndex, object, true);
    setRequiredProperty(kRequiredProperty_editing, QVariant::fromValue(false), modelIndex, item, true);

    if (auto attached = getAttachedObject(object))
        attached->setView(q);
}

void QQuickTableViewPrivate::itemPooledCallback(int modelIndex, QObject *object)
{
    Q_UNUSED(modelIndex);

    if (auto attached = getAttachedObject(object))
        emit attached->pooled();
}

void QQuickTableViewPrivate::itemReusedCallback(int modelIndex, QObject *object)
{
    const QPoint cell = cellAtModelIndex(modelIndex);
    const bool current = currentInSelectionModel(cell);
    const bool selected = selectedInSelectionModel(cell);
    setRequiredProperty(kRequiredProperty_current, QVariant::fromValue(current), modelIndex, object, false);
    setRequiredProperty(kRequiredProperty_selected, QVariant::fromValue(selected), modelIndex, object, false);
    // Note: the edit item will never be reused, so no reason to set kRequiredProperty_editing

    if (auto item = qobject_cast<QQuickItem*>(object))
        QQuickItemPrivate::get(item)->setCulled(false);

    if (auto attached = getAttachedObject(object))
        emit attached->reused();
}

void QQuickTableViewPrivate::syncWithPendingChanges()
{
    // The application can change properties like the model or the delegate while
    // we're e.g in the middle of e.g loading a new row. Since this will lead to
    // unpredicted behavior, and possibly a crash, we need to postpone taking
    // such assignments into effect until we're in a state that allows it.

    syncViewportRect();
    syncModel();
    syncDelegate();
    syncSyncView();
    syncPositionView();

    syncRebuildOptions();
}

void QQuickTableViewPrivate::syncRebuildOptions()
{
    if (!scheduledRebuildOptions)
        return;

    rebuildState = RebuildState::Begin;
    rebuildOptions = scheduledRebuildOptions;
    scheduledRebuildOptions = RebuildOption::None;

    if (loadedItems.isEmpty())
        rebuildOptions.setFlag(RebuildOption::All);

    // Some options are exclusive:
    if (rebuildOptions.testFlag(RebuildOption::All)) {
        rebuildOptions.setFlag(RebuildOption::ViewportOnly, false);
        rebuildOptions.setFlag(RebuildOption::LayoutOnly, false);
        rebuildOptions.setFlag(RebuildOption::CalculateNewContentWidth);
        rebuildOptions.setFlag(RebuildOption::CalculateNewContentHeight);
    } else if (rebuildOptions.testFlag(RebuildOption::ViewportOnly)) {
        rebuildOptions.setFlag(RebuildOption::LayoutOnly, false);
    }

    if (rebuildOptions.testFlag(RebuildOption::PositionViewAtRow))
        rebuildOptions.setFlag(RebuildOption::CalculateNewTopLeftRow, false);

    if (rebuildOptions.testFlag(RebuildOption::PositionViewAtColumn))
        rebuildOptions.setFlag(RebuildOption::CalculateNewTopLeftColumn, false);
}

void QQuickTableViewPrivate::syncDelegate()
{
    if (!tableModel) {
        // Only the tableModel uses the delegate assigned to a
        // TableView. DelegateModel has it's own delegate, and
        // ObjectModel etc. doesn't use one.
        return;
    }

    if (assignedDelegate != tableModel->delegate())
        tableModel->setDelegate(assignedDelegate);
}

QVariant QQuickTableViewPrivate::modelImpl() const
{
    return assignedModel;
}

void QQuickTableViewPrivate::setModelImpl(const QVariant &newModel)
{
    assignedModel = newModel;
    scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::All);
    emit q_func()->modelChanged();
}

void QQuickTableViewPrivate::syncModel()
{
    if (compareModel(modelVariant, assignedModel))
        return;

    if (model) {
        disconnectFromModel();
        releaseLoadedItems(QQmlTableInstanceModel::NotReusable);
    }

    modelVariant = assignedModel;
    QVariant effectiveModelVariant = modelVariant;
    if (effectiveModelVariant.userType() == qMetaTypeId<QJSValue>())
        effectiveModelVariant = effectiveModelVariant.value<QJSValue>().toVariant();

    const auto instanceModel = qobject_cast<QQmlInstanceModel *>(qvariant_cast<QObject*>(effectiveModelVariant));

    if (instanceModel) {
        if (tableModel) {
            delete tableModel;
            tableModel = nullptr;
        }
        model = instanceModel;
    } else {
        if (!tableModel)
            createWrapperModel();
        tableModel->setModel(effectiveModelVariant);
    }

    connectToModel();
}

void QQuickTableViewPrivate::syncSyncView()
{
    Q_Q(QQuickTableView);

    if (assignedSyncView != syncView) {
        if (syncView)
            syncView->d_func()->syncChildren.removeOne(q);

        if (assignedSyncView) {
            QQuickTableView *view = assignedSyncView;

            while (view) {
                if (view == q) {
                    if (!layoutWarningIssued) {
                        layoutWarningIssued = true;
                        qmlWarning(q) << "TableView: recursive syncView connection detected!";
                    }
                    syncView = nullptr;
                    return;
                }
                view = view->d_func()->syncView;
            }

            assignedSyncView->d_func()->syncChildren.append(q);
            scheduledRebuildOptions |= RebuildOption::ViewportOnly;
        }

        syncView = assignedSyncView;
    }

    syncHorizontally = syncView && assignedSyncDirection & Qt::Horizontal;
    syncVertically = syncView && assignedSyncDirection & Qt::Vertical;

    if (syncHorizontally) {
        QBoolBlocker fixupGuard(inUpdateContentSize, true);
        q->setColumnSpacing(syncView->columnSpacing());
        q->setLeftMargin(syncView->leftMargin());
        q->setRightMargin(syncView->rightMargin());
        updateContentWidth();

        if (syncView->leftColumn() != q->leftColumn()) {
            // The left column is no longer the same as the left
            // column in syncView. This requires a rebuild.
            scheduledRebuildOptions |= QQuickTableViewPrivate::RebuildOption::CalculateNewTopLeftColumn;
            scheduledRebuildOptions.setFlag(RebuildOption::ViewportOnly);
        }
    }

    if (syncVertically) {
        QBoolBlocker fixupGuard(inUpdateContentSize, true);
        q->setRowSpacing(syncView->rowSpacing());
        q->setTopMargin(syncView->topMargin());
        q->setBottomMargin(syncView->bottomMargin());
        updateContentHeight();

        if (syncView->topRow() != q->topRow()) {
            // The top row is no longer the same as the top
            // row in syncView. This requires a rebuild.
            scheduledRebuildOptions |= QQuickTableViewPrivate::RebuildOption::CalculateNewTopLeftRow;
            scheduledRebuildOptions.setFlag(RebuildOption::ViewportOnly);
        }
    }

    if (syncView && loadedItems.isEmpty() && !tableSize.isEmpty()) {
        // When we have a syncView, we can sometimes temporarily end up with no loaded items.
        // This can happen if the syncView has a model with more rows or columns than us, in
        // which case the viewport can end up in a place where we have no rows or columns to
        // show. In that case, check now if the viewport has been flicked back again, and
        // that we can rebuild the table with a visible top-left cell.
        const auto syncView_d = syncView->d_func();
        if (!syncView_d->loadedItems.isEmpty()) {
            if (syncHorizontally && syncView_d->leftColumn() <= tableSize.width() - 1)
                scheduledRebuildOptions |= QQuickTableViewPrivate::RebuildOption::ViewportOnly;
            else if (syncVertically && syncView_d->topRow() <= tableSize.height() - 1)
                scheduledRebuildOptions |= QQuickTableViewPrivate::RebuildOption::ViewportOnly;
        }
    }
}

void QQuickTableViewPrivate::syncPositionView()
{
    // Only positionViewAtRowAfterRebuild/positionViewAtColumnAfterRebuild are critical
    // to sync before a rebuild to avoid them being overwritten
    // by the setters while building. The other position properties
    // can change without it causing trouble.
    positionViewAtRowAfterRebuild = assignedPositionViewAtRowAfterRebuild;
    positionViewAtColumnAfterRebuild = assignedPositionViewAtColumnAfterRebuild;
}

void QQuickTableViewPrivate::connectToModel()
{
    Q_Q(QQuickTableView);
    Q_TABLEVIEW_ASSERT(model, "");

    QObjectPrivate::connect(model, &QQmlInstanceModel::createdItem, this, &QQuickTableViewPrivate::itemCreatedCallback);
    QObjectPrivate::connect(model, &QQmlInstanceModel::initItem, this, &QQuickTableViewPrivate::initItemCallback);
    QObjectPrivate::connect(model, &QQmlTableInstanceModel::itemPooled, this, &QQuickTableViewPrivate::itemPooledCallback);
    QObjectPrivate::connect(model, &QQmlTableInstanceModel::itemReused, this, &QQuickTableViewPrivate::itemReusedCallback);

    // Connect atYEndChanged to a function that fetches data if more is available
    QObjectPrivate::connect(q, &QQuickTableView::atYEndChanged, this, &QQuickTableViewPrivate::fetchMoreData);

    if (auto const aim = model->abstractItemModel()) {
        // When the model exposes a QAIM, we connect to it directly. This means that if the current model is
        // a QQmlDelegateModel, we just ignore all the change sets it emits. In most cases, the model will instead
        // be our own QQmlTableInstanceModel, which doesn't bother creating change sets at all. For models that are
        // not based on QAIM (like QQmlObjectModel, QQmlListModel, javascript arrays etc), there is currently no way
        // to modify the model at runtime without also re-setting the model on the view.
        connect(aim, &QAbstractItemModel::rowsMoved, this, &QQuickTableViewPrivate::rowsMovedCallback);
        connect(aim, &QAbstractItemModel::columnsMoved, this, &QQuickTableViewPrivate::columnsMovedCallback);
        connect(aim, &QAbstractItemModel::rowsInserted, this, &QQuickTableViewPrivate::rowsInsertedCallback);
        connect(aim, &QAbstractItemModel::rowsRemoved, this, &QQuickTableViewPrivate::rowsRemovedCallback);
        connect(aim, &QAbstractItemModel::columnsInserted, this, &QQuickTableViewPrivate::columnsInsertedCallback);
        connect(aim, &QAbstractItemModel::columnsRemoved, this, &QQuickTableViewPrivate::columnsRemovedCallback);
        connect(aim, &QAbstractItemModel::modelReset, this, &QQuickTableViewPrivate::modelResetCallback);
        connect(aim, &QAbstractItemModel::layoutChanged, this, &QQuickTableViewPrivate::layoutChangedCallback);
    } else {
        QObjectPrivate::connect(model, &QQmlInstanceModel::modelUpdated, this, &QQuickTableViewPrivate::modelUpdated);
    }
}

void QQuickTableViewPrivate::disconnectFromModel()
{
    Q_Q(QQuickTableView);
    Q_TABLEVIEW_ASSERT(model, "");

    QObjectPrivate::disconnect(model, &QQmlInstanceModel::createdItem, this, &QQuickTableViewPrivate::itemCreatedCallback);
    QObjectPrivate::disconnect(model, &QQmlInstanceModel::initItem, this, &QQuickTableViewPrivate::initItemCallback);
    QObjectPrivate::disconnect(model, &QQmlTableInstanceModel::itemPooled, this, &QQuickTableViewPrivate::itemPooledCallback);
    QObjectPrivate::disconnect(model, &QQmlTableInstanceModel::itemReused, this, &QQuickTableViewPrivate::itemReusedCallback);

    QObjectPrivate::disconnect(q, &QQuickTableView::atYEndChanged, this, &QQuickTableViewPrivate::fetchMoreData);

    if (auto const aim = model->abstractItemModel()) {
        disconnect(aim, &QAbstractItemModel::rowsMoved, this, &QQuickTableViewPrivate::rowsMovedCallback);
        disconnect(aim, &QAbstractItemModel::columnsMoved, this, &QQuickTableViewPrivate::columnsMovedCallback);
        disconnect(aim, &QAbstractItemModel::rowsInserted, this, &QQuickTableViewPrivate::rowsInsertedCallback);
        disconnect(aim, &QAbstractItemModel::rowsRemoved, this, &QQuickTableViewPrivate::rowsRemovedCallback);
        disconnect(aim, &QAbstractItemModel::columnsInserted, this, &QQuickTableViewPrivate::columnsInsertedCallback);
        disconnect(aim, &QAbstractItemModel::columnsRemoved, this, &QQuickTableViewPrivate::columnsRemovedCallback);
        disconnect(aim, &QAbstractItemModel::modelReset, this, &QQuickTableViewPrivate::modelResetCallback);
        disconnect(aim, &QAbstractItemModel::layoutChanged, this, &QQuickTableViewPrivate::layoutChangedCallback);
    } else {
        QObjectPrivate::disconnect(model, &QQmlInstanceModel::modelUpdated, this, &QQuickTableViewPrivate::modelUpdated);
    }
}

void QQuickTableViewPrivate::modelUpdated(const QQmlChangeSet &changeSet, bool reset)
{
    Q_UNUSED(changeSet);
    Q_UNUSED(reset);

    Q_TABLEVIEW_ASSERT(!model->abstractItemModel(), "");
    scheduleRebuildTable(RebuildOption::ViewportOnly
                         | RebuildOption::CalculateNewContentWidth
                         | RebuildOption::CalculateNewContentHeight);
}

void QQuickTableViewPrivate::rowsMovedCallback(const QModelIndex &parent, int, int, const QModelIndex &, int )
{
    if (parent != QModelIndex())
        return;

    scheduleRebuildTable(RebuildOption::ViewportOnly);
}

void QQuickTableViewPrivate::columnsMovedCallback(const QModelIndex &parent, int, int, const QModelIndex &, int)
{
    if (parent != QModelIndex())
        return;

    scheduleRebuildTable(RebuildOption::ViewportOnly);
}

void QQuickTableViewPrivate::rowsInsertedCallback(const QModelIndex &parent, int, int)
{
    if (parent != QModelIndex())
        return;

    scheduleRebuildTable(RebuildOption::ViewportOnly | RebuildOption::CalculateNewContentHeight);
}

void QQuickTableViewPrivate::rowsRemovedCallback(const QModelIndex &parent, int, int)
{
    Q_Q(QQuickTableView);

    if (parent != QModelIndex())
        return;

    // If editIndex was a part of the removed rows, it will now be invalid.
    if (!editIndex.isValid() && editItem)
        q->closeEditor();

    scheduleRebuildTable(RebuildOption::ViewportOnly | RebuildOption::CalculateNewContentHeight);
}

void QQuickTableViewPrivate::columnsInsertedCallback(const QModelIndex &parent, int, int)
{
    if (parent != QModelIndex())
        return;

    // Adding a column (or row) can result in the table going from being
    // e.g completely inside the viewport to go outside. And in the latter
    // case, the user needs to be able to scroll the viewport, also if
    // flags such as Flickable.StopAtBounds is in use. So we need to
    // update contentWidth to support that case.
    scheduleRebuildTable(RebuildOption::ViewportOnly | RebuildOption::CalculateNewContentWidth);
}

void QQuickTableViewPrivate::columnsRemovedCallback(const QModelIndex &parent, int, int)
{
    Q_Q(QQuickTableView);

    if (parent != QModelIndex())
        return;

    // If editIndex was a part of the removed columns, it will now be invalid.
    if (!editIndex.isValid() && editItem)
        q->closeEditor();

    scheduleRebuildTable(RebuildOption::ViewportOnly | RebuildOption::CalculateNewContentWidth);
}

void QQuickTableViewPrivate::layoutChangedCallback(const QList<QPersistentModelIndex> &parents, QAbstractItemModel::LayoutChangeHint hint)
{
    Q_UNUSED(parents);
    Q_UNUSED(hint);

    scheduleRebuildTable(RebuildOption::ViewportOnly);
}

void QQuickTableViewPrivate::fetchMoreData()
{
    if (tableModel && tableModel->canFetchMore()) {
        tableModel->fetchMore();
        scheduleRebuildTable(RebuildOption::ViewportOnly);
    }
}

void QQuickTableViewPrivate::modelResetCallback()
{
    Q_Q(QQuickTableView);
    q->closeEditor();
    scheduleRebuildTable(RebuildOption::All);
}

bool QQuickTableViewPrivate::compareModel(const QVariant& model1, const QVariant& model2) const
{
    return (model1 == model2 ||
            (model1.userType() == qMetaTypeId<QJSValue>() && model2.userType() == qMetaTypeId<QJSValue>() &&
                                 model1.value<QJSValue>().strictlyEquals(model2.value<QJSValue>())));
}

void QQuickTableViewPrivate::positionViewAtRow(int row, Qt::Alignment alignment, qreal offset, const QRectF subRect)
{
    Qt::Alignment verticalAlignment = alignment & (Qt::AlignTop | Qt::AlignVCenter | Qt::AlignBottom);
    Q_TABLEVIEW_ASSERT(verticalAlignment, alignment);

    if (syncHorizontally) {
        syncView->d_func()->positionViewAtRow(row, verticalAlignment, offset, subRect);
    } else {
        if (!scrollToRow(row, verticalAlignment, offset, subRect)) {
            // Could not scroll, so rebuild instead
            assignedPositionViewAtRowAfterRebuild = row;
            positionViewAtRowAlignment = verticalAlignment;
            positionViewAtRowOffset = offset;
            positionViewAtRowSubRect = subRect;
            scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::ViewportOnly |
                                 QQuickTableViewPrivate::RebuildOption::PositionViewAtRow);
        }
    }
}

void QQuickTableViewPrivate::positionViewAtColumn(int column, Qt::Alignment alignment, qreal offset, const QRectF subRect)
{
    Qt::Alignment horizontalAlignment = alignment & (Qt::AlignLeft | Qt::AlignHCenter | Qt::AlignRight);
    Q_TABLEVIEW_ASSERT(horizontalAlignment, alignment);

    if (syncVertically) {
        syncView->d_func()->positionViewAtColumn(column, horizontalAlignment, offset, subRect);
    } else {
        if (!scrollToColumn(column, horizontalAlignment, offset, subRect)) {
            // Could not scroll, so rebuild instead
            assignedPositionViewAtColumnAfterRebuild = column;
            positionViewAtColumnAlignment = horizontalAlignment;
            positionViewAtColumnOffset = offset;
            positionViewAtColumnSubRect = subRect;
            scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::ViewportOnly |
                                 QQuickTableViewPrivate::RebuildOption::PositionViewAtColumn);
        }
    }
}

bool QQuickTableViewPrivate::scrollToRow(int row, Qt::Alignment alignment, qreal offset, const QRectF subRect)
{
    Q_Q(QQuickTableView);

    // This function will only scroll to rows that are loaded (since we
    // don't know the location of unloaded rows). But as an exception, to
    // allow moving currentIndex out of the viewport, we support scrolling
    // to a row that is adjacent to the loaded table. So start by checking
    // if we should load en extra row.
    if (row < topRow()) {
        if (row != nextVisibleEdgeIndex(Qt::TopEdge, topRow() - 1))
            return false;
        loadEdge(Qt::TopEdge, QQmlIncubator::Synchronous);
    } else if (row > bottomRow()) {
        if (row != nextVisibleEdgeIndex(Qt::BottomEdge, bottomRow() + 1))
            return false;
        loadEdge(Qt::BottomEdge, QQmlIncubator::Synchronous);
    } else if (row < topRow() || row > bottomRow()) {
        return false;
    }

    if (!loadedRows.contains(row))
        return false;

    const qreal newContentY = getAlignmentContentY(row, alignment, offset, subRect);
    if (qFuzzyCompare(newContentY, q->contentY()))
        return true;

    if (animate) {
        const qreal diffY = qAbs(newContentY - q->contentY());
        const qreal duration = qBound(700., diffY * 5, 1500.);
        positionYAnimation.setTo(newContentY);
        positionYAnimation.setDuration(duration);
        positionYAnimation.restart();
    } else {
        positionYAnimation.stop();
        q->setContentY(newContentY);
    }

    return true;
}

bool QQuickTableViewPrivate::scrollToColumn(int column, Qt::Alignment alignment, qreal offset, const QRectF subRect)
{
    Q_Q(QQuickTableView);

    // This function will only scroll to columns that are loaded (since we
    // don't know the location of unloaded columns). But as an exception, to
    // allow moving currentIndex out of the viewport, we support scrolling
    // to a column that is adjacent to the loaded table. So start by checking
    // if we should load en extra column.
    if (column < leftColumn()) {
        if (column != nextVisibleEdgeIndex(Qt::LeftEdge, leftColumn() - 1))
            return false;
        loadEdge(Qt::LeftEdge, QQmlIncubator::Synchronous);
    } else if (column > rightColumn()) {
        if (column != nextVisibleEdgeIndex(Qt::RightEdge, rightColumn() + 1))
            return false;
        loadEdge(Qt::RightEdge, QQmlIncubator::Synchronous);
    } else if (column < leftColumn() || column > rightColumn()) {
        return false;
    }

    if (!loadedColumns.contains(column))
        return false;

    const qreal newContentX = getAlignmentContentX(column, alignment, offset, subRect);
    if (qFuzzyCompare(newContentX, q->contentX()))
        return true;

    if (animate) {
        const qreal diffX = qAbs(newContentX - q->contentX());
        const qreal duration = qBound(700., diffX * 5, 1500.);
        positionXAnimation.setTo(newContentX);
        positionXAnimation.setDuration(duration);
        positionXAnimation.restart();
    } else {
        positionXAnimation.stop();
        q->setContentX(newContentX);
    }

    return true;
}

void QQuickTableViewPrivate::scheduleRebuildIfFastFlick()
{
    Q_Q(QQuickTableView);
    // If the viewport has moved more than one page vertically or horizontally, we switch
    // strategy from refilling edges around the current table to instead rebuild the table
    // from scratch inside the new viewport. This will greatly improve performance when flicking
    // a long distance in one go, which can easily happen when dragging on scrollbars.
    // Note that we don't want to update the content size in this case, since first of all, the
    // content size should logically not change as a result of flicking. But more importantly, updating
    // the content size in combination with fast-flicking has a tendency to cause flicker in the viewport.

    // Check the viewport moved more than one page vertically
    if (!viewportRect.intersects(QRectF(viewportRect.x(), q->contentY(), 1, q->height()))) {
        scheduledRebuildOptions |= RebuildOption::CalculateNewTopLeftRow;
        scheduledRebuildOptions |= RebuildOption::ViewportOnly;
    }

    // Check the viewport moved more than one page horizontally
    if (!viewportRect.intersects(QRectF(q->contentX(), viewportRect.y(), q->width(), 1))) {
        scheduledRebuildOptions |= RebuildOption::CalculateNewTopLeftColumn;
        scheduledRebuildOptions |= RebuildOption::ViewportOnly;
    }
}

void QQuickTableViewPrivate::setLocalViewportX(qreal contentX)
{
    // Set the new viewport position if changed, but don't trigger any
    // rebuilds or updates. We use this function internally to distinguish
    // external flicking from internal sync-ing of the content view.
    Q_Q(QQuickTableView);
    QBoolBlocker blocker(inSetLocalViewportPos, true);

    if (qFuzzyCompare(contentX, q->contentX()))
        return;

    q->setContentX(contentX);
}

void QQuickTableViewPrivate::setLocalViewportY(qreal contentY)
{
    // Set the new viewport position if changed, but don't trigger any
    // rebuilds or updates. We use this function internally to distinguish
    // external flicking from internal sync-ing of the content view.
    Q_Q(QQuickTableView);
    QBoolBlocker blocker(inSetLocalViewportPos, true);

    if (qFuzzyCompare(contentY, q->contentY()))
        return;

    q->setContentY(contentY);
}

void QQuickTableViewPrivate::syncViewportRect()
{
    // Sync viewportRect so that it contains the actual geometry of the viewport.
    // Since the column (and row) size of a sync child is decided by the column size
    // of its sync view, the viewport width of a sync view needs to be the maximum of
    // the sync views width, and its sync childrens width. This to ensure that no sync
    // child loads a column which is not yet loaded by the sync view, since then the
    // implicit column size cannot be resolved.
    Q_Q(QQuickTableView);

    qreal w = q->width();
    qreal h = q->height();

    for (auto syncChild : std::as_const(syncChildren)) {
        auto syncChild_d = syncChild->d_func();
        if (syncChild_d->syncHorizontally)
            w = qMax(w, syncChild->width());
        if (syncChild_d->syncHorizontally)
            h = qMax(h, syncChild->height());
    }

    viewportRect = QRectF(q->contentX(), q->contentY(), w, h);
}

void QQuickTableViewPrivate::init()
{
    Q_Q(QQuickTableView);

    q->setFlag(QQuickItem::ItemIsFocusScope);
    q->setActiveFocusOnTab(true);

    positionXAnimation.setTargetObject(q);
    positionXAnimation.setProperty(QStringLiteral("contentX"));
    positionXAnimation.setEasing(QEasingCurve::OutQuart);

    positionYAnimation.setTargetObject(q);
    positionYAnimation.setProperty(QStringLiteral("contentY"));
    positionYAnimation.setEasing(QEasingCurve::OutQuart);

    auto tapHandler = new QQuickTableViewTapHandler(q);

    hoverHandler = new QQuickTableViewHoverHandler(q);
    resizeHandler = new QQuickTableViewResizeHandler(q);
    hoverHandler->setEnabled(resizableRows || resizableColumns);
    resizeHandler->setEnabled(resizableRows || resizableColumns);

    // To allow for a more snappy UX, we try to change the current index already upon
    // receiving a pointer press. But we should only do that if the view is not interactive
    // (so that it doesn't interfere with flicking), and if the resizeHandler is not
    // being hovered/dragged. For those cases, we fall back to setting the current index
    // on tap instead. A double tap on a resize area should also revert the section size
    // back to its implicit size.
    QObject::connect(tapHandler, &QQuickTapHandler::pressedChanged, [this, q, tapHandler] {
        if (!tapHandler->isPressed())
            return;

        positionXAnimation.stop();
        positionYAnimation.stop();

        if (!q->isInteractive())
            handleTap(tapHandler->point());
    });

    QObject::connect(tapHandler, &QQuickTapHandler::singleTapped, [this, q, tapHandler] {
        if (q->isInteractive())
            handleTap(tapHandler->point());
    });

    QObject::connect(tapHandler, &QQuickTapHandler::doubleTapped, [this, q, tapHandler] {
        const bool resizeRow = resizableRows && hoverHandler->m_row != -1;
        const bool resizeColumn = resizableColumns && hoverHandler->m_column != -1;

        if (resizeRow || resizeColumn) {
            if (resizeRow)
                q->setRowHeight(hoverHandler->m_row, -1);
            if (resizeColumn)
                q->setColumnWidth(hoverHandler->m_column, -1);
        } else if (editTriggers & QQuickTableView::DoubleTapped) {
            const QPointF pos = tapHandler->point().pressPosition();
            const QPoint cell = q->cellAtPosition(pos);
            const QModelIndex index = q->modelIndex(cell);
            if (canEdit(index, false))
                q->edit(index);
        }
    });
}

void QQuickTableViewPrivate::handleTap(const QQuickHandlerPoint &point)
{
    Q_Q(QQuickTableView);

    if (keyNavigationEnabled)
        q->forceActiveFocus(Qt::MouseFocusReason);

    if (point.modifiers() != Qt::NoModifier)
        return;
    if (resizableRows && hoverHandler->m_row != -1)
        return;
    if (resizableColumns && hoverHandler->m_column != -1)
        return;
    if (resizeHandler->state() != QQuickTableViewResizeHandler::Listening)
        return;

    const QModelIndex tappedIndex = q->modelIndex(q->cellAtPosition(point.position()));
    bool tappedCellIsSelected = false;

    if (selectionModel)
        tappedCellIsSelected = selectionModel->isSelected(tappedIndex);

    if (canEdit(tappedIndex, false)) {
        if (editTriggers & QQuickTableView::SingleTapped) {
            if (selectionBehavior != QQuickTableView::SelectionDisabled)
                clearSelection();
            q->edit(tappedIndex);
            return;
        } else if (editTriggers & QQuickTableView::SelectedTapped && tappedCellIsSelected) {
            q->edit(tappedIndex);
            return;
        }
    }

    // Since the tap didn't result in selecting or editing cells, we clear
    // the current selection and move the current index instead.
    if (pointerNavigationEnabled) {
        q->closeEditor();
        if (selectionBehavior != QQuickTableView::SelectionDisabled)
            clearSelection();
        setCurrentIndexFromTap(point.position());
    }
}

bool QQuickTableViewPrivate::canEdit(const QModelIndex tappedIndex, bool warn)
{
    // Check that a call to edit(tappedIndex) would not
    // result in warnings being printed.
    Q_Q(QQuickTableView);

    if (!tappedIndex.isValid()) {
        if (warn)
            qmlWarning(q) << "cannot edit: index is not valid!";
        return false;
    }

    if (auto const qaim = model->abstractItemModel()) {
        if (!(qaim->flags(tappedIndex) & Qt::ItemIsEditable)) {
            if (warn)
                qmlWarning(q) << "cannot edit: QAbstractItemModel::flags(index) doesn't contain Qt::ItemIsEditable";
            return false;
        }
    }

    const QPoint cell = q->cellAtIndex(tappedIndex);
    const QQuickItem *cellItem = q->itemAtCell(cell);
    if (!cellItem) {
        if (warn)
            qmlWarning(q) << "cannot edit: the cell to edit is not inside the viewport!";
        return false;
    }

    auto attached = getAttachedObject(cellItem);
    if (!attached || !attached->editDelegate()) {
        if (warn)
            qmlWarning(q) << "cannot edit: no TableView.editDelegate set!";
        return false;
    }

    return true;
}

void QQuickTableViewPrivate::syncViewportPosRecursive()
{
    Q_Q(QQuickTableView);
    QBoolBlocker recursionGuard(inSyncViewportPosRecursive, true);

    if (syncView) {
        auto syncView_d = syncView->d_func();
        if (!syncView_d->inSyncViewportPosRecursive) {
            if (syncHorizontally)
                syncView_d->setLocalViewportX(q->contentX());
            if (syncVertically)
                syncView_d->setLocalViewportY(q->contentY());
            syncView_d->syncViewportPosRecursive();
        }
    }

    for (auto syncChild : std::as_const(syncChildren)) {
        auto syncChild_d = syncChild->d_func();
        if (!syncChild_d->inSyncViewportPosRecursive) {
            if (syncChild_d->syncHorizontally)
                syncChild_d->setLocalViewportX(q->contentX());
            if (syncChild_d->syncVertically)
                syncChild_d->setLocalViewportY(q->contentY());
            syncChild_d->syncViewportPosRecursive();
        }
    }
}

void QQuickTableViewPrivate::setCurrentIndexFromTap(const QPointF &pos)
{
    Q_Q(QQuickTableView);

    const QPoint cell = q->cellAtPosition(pos);
    if (!cellIsValid(cell))
        return;

    setCurrentIndex(cell);
}

void QQuickTableViewPrivate::setCurrentIndex(const QPoint &cell)
{
    if (!selectionModel)
        return;

    const auto index = q_func()->modelIndex(cell);
    selectionModel->setCurrentIndex(index, QItemSelectionModel::NoUpdate);
}

bool QQuickTableViewPrivate::setCurrentIndexFromKeyEvent(QKeyEvent *e)
{
    Q_Q(QQuickTableView);

    if (!selectionModel || !selectionModel->model())
        return false;

    const QModelIndex currentIndex = selectionModel->currentIndex();
    const QPoint currentCell = q->cellAtIndex(currentIndex);
    const bool select = (e->modifiers() & Qt::ShiftModifier) && (e->key() != Qt::Key_Backtab);

    if (!q->activeFocusOnTab()) {
        switch (e->key()) {
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            return false;
        }
    }

    if (!cellIsValid(currentCell)) {
        switch (e->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_Home:
        case Qt::Key_End:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            // Special case: the current index doesn't map to a cell in the view (perhaps
            // because it isn't set yet). In that case, we set it to be the top-left cell.
            const QModelIndex topLeftIndex = q->index(topRow(), leftColumn());
            selectionModel->setCurrentIndex(topLeftIndex, QItemSelectionModel::NoUpdate);
            return true;
        }
        return false;
    }

    auto beginMoveCurrentIndex = [&](){
        if (!select) {
            clearSelection();
        } else if (selectionRectangle().isEmpty()) {
            const int serializedStartIndex = modelIndexToCellIndex(selectionModel->currentIndex());
            if (loadedItems.contains(serializedStartIndex)) {
                const QRectF startGeometry = loadedItems.value(serializedStartIndex)->geometry();
                setSelectionStartPos(startGeometry.center());
            }
        }
    };

    auto endMoveCurrentIndex = [&](const QPoint &cell){
        if (select) {
            if (polishScheduled)
                forceLayout(true);
            const int serializedEndIndex = modelIndexAtCell(cell);
            if (loadedItems.contains(serializedEndIndex)) {
                const QRectF endGeometry = loadedItems.value(serializedEndIndex)->geometry();
                setSelectionEndPos(endGeometry.center());
            }
        }
        selectionModel->setCurrentIndex(q->modelIndex(cell), QItemSelectionModel::NoUpdate);
    };

    switch (e->key()) {
    case Qt::Key_Up: {
        beginMoveCurrentIndex();
        const int nextRow = nextVisibleEdgeIndex(Qt::TopEdge, currentCell.y() - 1);
        if (nextRow == kEdgeIndexAtEnd)
            break;
        const qreal marginY = atTableEnd(Qt::TopEdge, nextRow - 1) ? -q->topMargin() : 0;
        q->positionViewAtRow(nextRow, QQuickTableView::Contain, marginY);
        endMoveCurrentIndex({currentCell.x(), nextRow});
        break; }
    case Qt::Key_Down: {
        beginMoveCurrentIndex();
        const int nextRow = nextVisibleEdgeIndex(Qt::BottomEdge, currentCell.y() + 1);
        if (nextRow == kEdgeIndexAtEnd)
            break;
        const qreal marginY = atTableEnd(Qt::BottomEdge, nextRow + 1) ? q->bottomMargin() : 0;
        q->positionViewAtRow(nextRow, QQuickTableView::Contain, marginY);
        endMoveCurrentIndex({currentCell.x(), nextRow});
        break; }
    case Qt::Key_Left: {
        beginMoveCurrentIndex();
        const int nextColumn = nextVisibleEdgeIndex(Qt::LeftEdge, currentCell.x() - 1);
        if (nextColumn == kEdgeIndexAtEnd)
            break;
        const qreal marginX = atTableEnd(Qt::LeftEdge, nextColumn - 1) ? -q->leftMargin() : 0;
        q->positionViewAtColumn(nextColumn, QQuickTableView::Contain, marginX);
        endMoveCurrentIndex({nextColumn, currentCell.y()});
        break; }
    case Qt::Key_Right: {
        beginMoveCurrentIndex();
        const int nextColumn = nextVisibleEdgeIndex(Qt::RightEdge, currentCell.x() + 1);
        if (nextColumn == kEdgeIndexAtEnd)
            break;
        const qreal marginX = atTableEnd(Qt::RightEdge, nextColumn + 1) ? q->rightMargin() : 0;
        q->positionViewAtColumn(nextColumn, QQuickTableView::Contain, marginX);
        endMoveCurrentIndex({nextColumn, currentCell.y()});
        break; }
    case Qt::Key_PageDown: {
        int newBottomRow = -1;
        beginMoveCurrentIndex();
        if (currentCell.y() < bottomRow()) {
            // The first PageDown should just move currentIndex to the bottom
            newBottomRow = bottomRow();
            q->positionViewAtRow(newBottomRow, QQuickTableView::AlignBottom, 0);
        } else {
            q->positionViewAtRow(bottomRow(), QQuickTableView::AlignTop, 0);
            positionYAnimation.complete();
            newBottomRow = topRow() != bottomRow() ? bottomRow() : bottomRow() + 1;
            const qreal marginY = atTableEnd(Qt::BottomEdge, newBottomRow + 1) ? q->bottomMargin() : 0;
            q->positionViewAtRow(newBottomRow, QQuickTableView::AlignTop | QQuickTableView::AlignBottom, marginY);
            positionYAnimation.complete();
        }
        endMoveCurrentIndex(QPoint(currentCell.x(), newBottomRow));
        break; }
    case Qt::Key_PageUp: {
        int newTopRow = -1;
        beginMoveCurrentIndex();
        if (currentCell.y() > topRow()) {
            // The first PageUp should just move currentIndex to the top
            newTopRow = topRow();
            q->positionViewAtRow(newTopRow, QQuickTableView::AlignTop, 0);
        } else {
            q->positionViewAtRow(topRow(), QQuickTableView::AlignBottom, 0);
            positionYAnimation.complete();
            newTopRow = topRow() != bottomRow() ? topRow() : topRow() - 1;
            const qreal marginY = atTableEnd(Qt::TopEdge, newTopRow - 1) ? -q->topMargin() : 0;
            q->positionViewAtRow(newTopRow, QQuickTableView::AlignTop, marginY);
            positionYAnimation.complete();
        }
        endMoveCurrentIndex(QPoint(currentCell.x(), newTopRow));
        break; }
    case Qt::Key_Home: {
        beginMoveCurrentIndex();
        const int firstColumn = nextVisibleEdgeIndex(Qt::RightEdge, 0);
        q->positionViewAtColumn(firstColumn, QQuickTableView::AlignLeft, -q->leftMargin());
        endMoveCurrentIndex(QPoint(firstColumn, currentCell.y()));
        break; }
    case Qt::Key_End: {
        beginMoveCurrentIndex();
        const int lastColumn = nextVisibleEdgeIndex(Qt::LeftEdge, tableSize.width() - 1);
        q->positionViewAtColumn(lastColumn, QQuickTableView::AlignRight, q->rightMargin());
        endMoveCurrentIndex(QPoint(lastColumn, currentCell.y()));
        break; }
    case Qt::Key_Tab: {
        beginMoveCurrentIndex();
        int nextRow = currentCell.y();
        int nextColumn = nextVisibleEdgeIndex(Qt::RightEdge, currentCell.x() + 1);
        if (nextColumn == kEdgeIndexAtEnd) {
            nextRow = nextVisibleEdgeIndex(Qt::BottomEdge, currentCell.y() + 1);
            if (nextRow == kEdgeIndexAtEnd)
                nextRow = nextVisibleEdgeIndex(Qt::BottomEdge, 0);
            nextColumn = nextVisibleEdgeIndex(Qt::RightEdge, 0);
            const qreal marginY = atTableEnd(Qt::BottomEdge, nextRow + 1) ? q->bottomMargin() : 0;
            q->positionViewAtRow(nextRow, QQuickTableView::Contain, marginY);
        }

        qreal marginX = 0;
        if (atTableEnd(Qt::RightEdge, nextColumn + 1))
            marginX = q->leftMargin();
        else if (atTableEnd(Qt::LeftEdge, nextColumn - 1))
            marginX = -q->leftMargin();

        q->positionViewAtColumn(nextColumn, QQuickTableView::Contain, marginX);
        endMoveCurrentIndex({nextColumn, nextRow});
        break; }
    case Qt::Key_Backtab: {
        beginMoveCurrentIndex();
        int nextRow = currentCell.y();
        int nextColumn = nextVisibleEdgeIndex(Qt::LeftEdge, currentCell.x() - 1);
        if (nextColumn == kEdgeIndexAtEnd) {
            nextRow = nextVisibleEdgeIndex(Qt::TopEdge, currentCell.y() - 1);
            if (nextRow == kEdgeIndexAtEnd)
                nextRow = nextVisibleEdgeIndex(Qt::TopEdge, tableSize.height() - 1);
            nextColumn = nextVisibleEdgeIndex(Qt::LeftEdge, tableSize.width() - 1);
            const qreal marginY = atTableEnd(Qt::TopEdge, nextRow - 1) ? -q->topMargin() : 0;
            q->positionViewAtRow(nextRow, QQuickTableView::Contain, marginY);
        }

        qreal marginX = 0;
        if (atTableEnd(Qt::RightEdge, nextColumn + 1))
            marginX = q->leftMargin();
        else if (atTableEnd(Qt::LeftEdge, nextColumn - 1))
            marginX = -q->leftMargin();

        q->positionViewAtColumn(nextColumn, QQuickTableView::Contain, marginX);
        endMoveCurrentIndex({nextColumn, nextRow});
        break; }
    default:
        return false;
    }

    return true;
}

bool QQuickTableViewPrivate::editFromKeyEvent(QKeyEvent *e)
{
    Q_Q(QQuickTableView);

    if (editTriggers == QQuickTableView::NoEditTriggers)
        return false;
    if (!selectionModel || !selectionModel->model())
        return false;

    const QModelIndex index = selectionModel->currentIndex();
    const QPoint cell = q->cellAtIndex(index);
    const QQuickItem *cellItem = q->itemAtCell(cell);
    if (!cellItem)
        return false;

    auto attached = getAttachedObject(cellItem);
    if (!attached || !attached->editDelegate())
        return false;

    bool anyKeyPressed = false;
    bool editKeyPressed = false;

    switch (e->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
#ifndef Q_OS_MACOS
    case Qt::Key_F2:
#endif
        anyKeyPressed = true;
        editKeyPressed = true;
        break;
    case Qt::Key_Shift:
    case Qt::Key_Alt:
    case Qt::Key_Control:
    case Qt::Key_Meta:
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
        break;
    default:
        anyKeyPressed = true;
    }

    const bool anyKeyAccepted = anyKeyPressed && (editTriggers & QQuickTableView::AnyKeyPressed);
    const bool editKeyAccepted = editKeyPressed && (editTriggers & QQuickTableView::EditKeyPressed);

    if (!(editKeyAccepted || anyKeyAccepted))
        return false;

    if (!canEdit(index, false)) {
        // If canEdit() returns false at this point (e.g because currentIndex is not
        // editable), we still want to eat the key event, to keep a consistent behavior
        // when some cells are editable, but others not.
        return true;
    }

    q->edit(index);

    if (editIndex.isValid() && anyKeyAccepted && !editKeyPressed) {
        // Replay the key event to the focus object (which should at this point
        // be the edit item, or an item inside the edit item).
        QGuiApplication::sendEvent(QGuiApplication::focusObject(), e);
    }

    return true;
}

#if QT_CONFIG(cursor)
void QQuickTableViewPrivate::updateCursor()
{
    int row = resizableRows ? hoverHandler->m_row : -1;
    int column = resizableColumns ? hoverHandler->m_column : -1;

    const auto resizeState = resizeHandler->state();
    if (resizeState == QQuickTableViewResizeHandler::DraggingStarted
            || resizeState == QQuickTableViewResizeHandler::Dragging) {
        // Don't change the cursor while resizing, even if
        // the pointer is not actually hovering the grid.
        row = resizeHandler->m_row;
        column = resizeHandler->m_column;
    }

    if (row != -1 || column != -1) {
        Qt::CursorShape shape;
        if (row != -1 && column != -1)
            shape = Qt::SizeFDiagCursor;
        else if (row != -1)
            shape = Qt::SplitVCursor;
        else
            shape = Qt::SplitHCursor;

        if (m_cursorSet)
            qApp->changeOverrideCursor(shape);
        else
            qApp->setOverrideCursor(shape);

        m_cursorSet = true;
    } else if (m_cursorSet) {
        qApp->restoreOverrideCursor();
        m_cursorSet = false;
    }
}
#endif

void QQuickTableViewPrivate::updateEditItem()
{
    Q_Q(QQuickTableView);

    if (!editItem)
        return;

    const QPoint cell = q->cellAtIndex(editIndex);
    auto cellItem = q->itemAtCell(cell);
    if (!cellItem) {
        // The delegate item that is being edited has left the viewport. But since we
        // added an extra reference to it when editing began, the delegate item has
        // not been unloaded! It's therefore still on the content item (outside the
        // viewport), but its position will no longer be updated until the row and column
        // it's a part of enters the viewport again. To avoid glitches related to the
        // item showing up on wrong places (e.g after resizing a column in front of it),
        // we move it far out of the viewport. This way it will be "hidden", but continue
        // to have edit focus. When the row and column that it's a part of are eventually
        // flicked back in again, a relayout will move it back to the correct place.
        editItem->parentItem()->setX(-editItem->width() - 10000);
    }
}

QQuickTableView::QQuickTableView(QQuickItem *parent)
    : QQuickFlickable(*(new QQuickTableViewPrivate), parent)
{
    d_func()->init();
}

QQuickTableView::QQuickTableView(QQuickTableViewPrivate &dd, QQuickItem *parent)
    : QQuickFlickable(dd, parent)
{
    d_func()->init();
}

QQuickTableView::~QQuickTableView()
{
}

void QQuickTableView::componentFinalized()
{
    // componentComplete() is called on us after all static values have been assigned, but
    // before bindings to any anchestors has been evaluated. Especially this means that
    // if our size is bound to the parents size, it will still be empty at that point.
    // And we cannot build the table without knowing our own size. We could wait until we
    // got the first updatePolish() callback, but at that time, any asynchronous loaders that we
    // might be inside have already finished loading, which means that we would load all
    // the delegate items synchronously instead of asynchronously. We therefore use componentFinalized
    // which gets called after all the bindings we rely on has been evaluated.
    // When receiving this call, we load the delegate items (and build the table).

    // Now that all bindings are evaluated, and we know
    // our final geometery, we can build the table.
    Q_D(QQuickTableView);
    qCDebug(lcTableViewDelegateLifecycle);
    d->updatePolish();
}

qreal QQuickTableView::minXExtent() const
{
    return QQuickFlickable::minXExtent() - d_func()->origin.x();
}

qreal QQuickTableView::maxXExtent() const
{
    return QQuickFlickable::maxXExtent() - d_func()->endExtent.width();
}

qreal QQuickTableView::minYExtent() const
{
    return QQuickFlickable::minYExtent() - d_func()->origin.y();
}

qreal QQuickTableView::maxYExtent() const
{
    return QQuickFlickable::maxYExtent() - d_func()->endExtent.height();
}

int QQuickTableView::rows() const
{
    return d_func()->tableSize.height();
}

int QQuickTableView::columns() const
{
    return d_func()->tableSize.width();
}

qreal QQuickTableView::rowSpacing() const
{
    return d_func()->cellSpacing.height();
}

void QQuickTableView::setRowSpacing(qreal spacing)
{
    Q_D(QQuickTableView);
    if (qt_is_nan(spacing) || !qt_is_finite(spacing))
        return;
    if (qFuzzyCompare(d->cellSpacing.height(), spacing))
        return;

    d->cellSpacing.setHeight(spacing);
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::LayoutOnly
                            | QQuickTableViewPrivate::RebuildOption::CalculateNewContentHeight);
    emit rowSpacingChanged();
}

qreal QQuickTableView::columnSpacing() const
{
    return d_func()->cellSpacing.width();
}

void QQuickTableView::setColumnSpacing(qreal spacing)
{
    Q_D(QQuickTableView);
    if (qt_is_nan(spacing) || !qt_is_finite(spacing))
        return;
    if (qFuzzyCompare(d->cellSpacing.width(), spacing))
        return;

    d->cellSpacing.setWidth(spacing);
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::LayoutOnly
                            | QQuickTableViewPrivate::RebuildOption::CalculateNewContentWidth);
    emit columnSpacingChanged();
}

QJSValue QQuickTableView::rowHeightProvider() const
{
    return d_func()->rowHeightProvider;
}

void QQuickTableView::setRowHeightProvider(const QJSValue &provider)
{
    Q_D(QQuickTableView);
    if (provider.strictlyEquals(d->rowHeightProvider))
        return;

    d->rowHeightProvider = provider;
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::ViewportOnly
                            | QQuickTableViewPrivate::RebuildOption::CalculateNewContentHeight);
    emit rowHeightProviderChanged();
}

QJSValue QQuickTableView::columnWidthProvider() const
{
    return d_func()->columnWidthProvider;
}

void QQuickTableView::setColumnWidthProvider(const QJSValue &provider)
{
    Q_D(QQuickTableView);
    if (provider.strictlyEquals(d->columnWidthProvider))
        return;

    d->columnWidthProvider = provider;
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::ViewportOnly
                            | QQuickTableViewPrivate::RebuildOption::CalculateNewContentWidth);
    emit columnWidthProviderChanged();
}

QVariant QQuickTableView::model() const
{
    return d_func()->modelImpl();
}

void QQuickTableView::setModel(const QVariant &newModel)
{
    Q_D(QQuickTableView);
    if (d->compareModel(newModel, d->assignedModel))
        return;

    closeEditor();
    d->setModelImpl(newModel);
    if (d->selectionModel)
        d->selectionModel->setModel(d->selectionSourceModel());
}

QQmlComponent *QQuickTableView::delegate() const
{
    return d_func()->assignedDelegate;
}

void QQuickTableView::setDelegate(QQmlComponent *newDelegate)
{
    Q_D(QQuickTableView);
    if (newDelegate == d->assignedDelegate)
        return;

    d->assignedDelegate = newDelegate;
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::All);

    emit delegateChanged();
}

QQuickTableView::EditTriggers QQuickTableView::editTriggers() const
{
    return d_func()->editTriggers;
}

void QQuickTableView::setEditTriggers(QQuickTableView::EditTriggers editTriggers)
{
    Q_D(QQuickTableView);
    if (editTriggers == d->editTriggers)
        return;

    d->editTriggers = editTriggers;

    emit editTriggersChanged();
}

bool QQuickTableView::reuseItems() const
{
    return bool(d_func()->reusableFlag == QQmlTableInstanceModel::Reusable);
}

void QQuickTableView::setReuseItems(bool reuse)
{
    Q_D(QQuickTableView);
    if (reuseItems() == reuse)
        return;

    d->reusableFlag = reuse ? QQmlTableInstanceModel::Reusable : QQmlTableInstanceModel::NotReusable;

    if (!reuse && d->tableModel) {
        // When we're told to not reuse items, we
        // immediately, as documented, drain the pool.
        d->tableModel->drainReusableItemsPool(0);
    }

    emit reuseItemsChanged();
}

void QQuickTableView::setContentWidth(qreal width)
{
    Q_D(QQuickTableView);
    d->explicitContentWidth = width;
    QQuickFlickable::setContentWidth(width);
}

void QQuickTableView::setContentHeight(qreal height)
{
    Q_D(QQuickTableView);
    d->explicitContentHeight = height;
    QQuickFlickable::setContentHeight(height);
}

/*!
    \qmlproperty TableView QtQuick::TableView::syncView

    If this property of a TableView is set to another TableView, both the
    tables will synchronize with regard to flicking, column widths/row heights,
    and spacing according to \l syncDirection.

    If \l syncDirection contains \l {Qt::Horizontal}{Qt.Horizontal}, current
    tableView's column widths, column spacing, and horizontal flicking movement
    synchronizes with syncView's.

    If \l syncDirection contains \l {Qt::Vertical}{Qt.Vertical}, current
    tableView's row heights, row spacing, and vertical flicking movement
    synchronizes with syncView's.

    \sa syncDirection
*/
QQuickTableView *QQuickTableView::syncView() const
{
   return d_func()->assignedSyncView;
}

void QQuickTableView::setSyncView(QQuickTableView *view)
{
    Q_D(QQuickTableView);
    if (d->assignedSyncView == view)
        return;

    d->assignedSyncView = view;
    d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::ViewportOnly);

    emit syncViewChanged();
}

/*!
    \qmlproperty Qt::Orientations QtQuick::TableView::syncDirection

    If the \l syncView is set on a TableView, this property controls
    synchronization of flicking direction(s) for both tables. The default is \c
    {Qt.Horizontal | Qt.Vertical}, which means that if you flick either table
    in either direction, the other table is flicked the same amount in the
    same direction.

    This property and \l syncView can be used to make two tableViews
    synchronize with each other smoothly in flicking regardless of the different
    overshoot/undershoot, velocity, acceleration/deceleration or rebound
    animation, and so on.

    A typical use case is to make several headers flick along with the table.

    \sa syncView
*/
Qt::Orientations QQuickTableView::syncDirection() const
{
   return d_func()->assignedSyncDirection;
}

void QQuickTableView::setSyncDirection(Qt::Orientations direction)
{
    Q_D(QQuickTableView);
    if (d->assignedSyncDirection == direction)
        return;

    d->assignedSyncDirection = direction;
    if (d->assignedSyncView)
        d->scheduleRebuildTable(QQuickTableViewPrivate::RebuildOption::ViewportOnly);

    emit syncDirectionChanged();
}

QItemSelectionModel *QQuickTableView::selectionModel() const
{
    return d_func()->selectionModel;
}

void QQuickTableView::setSelectionModel(QItemSelectionModel *selectionModel)
{
    Q_D(QQuickTableView);
    if (d->selectionModel == selectionModel)
        return;

    // Note: There is no need to rebuild the table when the selection model
    // changes, since selections only affect the internals of the delegate
    // items, and not the layout of the TableView.

    if (d->selectionModel) {
        QQuickTableViewPrivate::disconnect(d->selectionModel, &QItemSelectionModel::selectionChanged,
                                        d, &QQuickTableViewPrivate::selectionChangedInSelectionModel);
        QQuickTableViewPrivate::disconnect(d->selectionModel, &QItemSelectionModel::currentChanged,
                                        d, &QQuickTableViewPrivate::currentChangedInSelectionModel);
    }

    d->selectionModel = selectionModel;

    if (d->selectionModel) {
        d->selectionModel->setModel(d->selectionSourceModel());
        QQuickTableViewPrivate::connect(d->selectionModel, &QItemSelectionModel::selectionChanged,
                                        d, &QQuickTableViewPrivate::selectionChangedInSelectionModel);
        QQuickTableViewPrivate::connect(d->selectionModel, &QItemSelectionModel::currentChanged,
                                        d, &QQuickTableViewPrivate::currentChangedInSelectionModel);
    }

    d->updateSelectedOnAllDelegateItems();

    emit selectionModelChanged();
}

bool QQuickTableView::animate() const
{
   return d_func()->animate;
}

void QQuickTableView::setAnimate(bool animate)
{
    Q_D(QQuickTableView);
    if (d->animate == animate)
        return;

    d->animate = animate;
    if (!animate) {
        d->positionXAnimation.stop();
        d->positionYAnimation.stop();
    }

    emit animateChanged();
}

bool QQuickTableView::keyNavigationEnabled() const
{
    return d_func()->keyNavigationEnabled;
}

void QQuickTableView::setKeyNavigationEnabled(bool enabled)
{
    Q_D(QQuickTableView);
    if (d->keyNavigationEnabled == enabled)
        return;

    d->keyNavigationEnabled = enabled;

    emit keyNavigationEnabledChanged();
}

bool QQuickTableView::pointerNavigationEnabled() const
{
    return d_func()->pointerNavigationEnabled;
}

void QQuickTableView::setPointerNavigationEnabled(bool enabled)
{
    Q_D(QQuickTableView);
    if (d->pointerNavigationEnabled == enabled)
        return;

    d->pointerNavigationEnabled = enabled;

    emit pointerNavigationEnabledChanged();
}

int QQuickTableView::leftColumn() const
{
    Q_D(const QQuickTableView);
    return d->loadedItems.isEmpty() ? -1 : d_func()->leftColumn();
}

int QQuickTableView::rightColumn() const
{
    Q_D(const QQuickTableView);
    return d->loadedItems.isEmpty() ? -1 : d_func()->rightColumn();
}

int QQuickTableView::topRow() const
{
    Q_D(const QQuickTableView);
    return d->loadedItems.isEmpty() ? -1 : d_func()->topRow();
}

int QQuickTableView::bottomRow() const
{
    Q_D(const QQuickTableView);
    return d->loadedItems.isEmpty() ? -1 : d_func()->bottomRow();
}

int QQuickTableView::currentRow() const
{
    return d_func()->currentRow;
}

int QQuickTableView::currentColumn() const
{
    return d_func()->currentColumn;
}

void QQuickTableView::positionViewAtRow(int row, PositionMode mode, qreal offset, const QRectF &subRect)
{
    Q_D(QQuickTableView);
    if (row < 0 || row >= rows() || d->loadedRows.isEmpty())
        return;

    // Note: PositionMode::Contain is from here on translated to (Qt::AlignTop | Qt::AlignBottom).
    // This is an internal (unsupported) combination which means "align bottom if the whole cell
    // fits inside the viewport, otherwise align top".

    if (mode & (AlignTop | AlignBottom | AlignVCenter)) {
        mode &= AlignTop | AlignBottom | AlignVCenter;
        d->positionViewAtRow(row, Qt::Alignment(int(mode)), offset, subRect);
    } else if (mode == Contain) {
        if (row < topRow()) {
            d->positionViewAtRow(row, Qt::AlignTop, offset, subRect);
        } else if (row > bottomRow()) {
            d->positionViewAtRow(row, Qt::AlignTop | Qt::AlignBottom, offset, subRect);
        } else if (row == topRow()) {
            if (!subRect.isValid()) {
                d->positionViewAtRow(row, Qt::AlignTop, offset, subRect);
            } else {
                const qreal subRectTop = d->loadedTableOuterRect.top() + subRect.top();
                const qreal subRectBottom = d->loadedTableOuterRect.top() + subRect.bottom();
                if (subRectTop < d->viewportRect.y())
                    d->positionViewAtRow(row, Qt::AlignTop, offset, subRect);
                else if (subRectBottom > d->viewportRect.bottom())
                    d->positionViewAtRow(row, Qt::AlignTop | Qt::AlignBottom, offset, subRect);
            }
        } else if (row == bottomRow()) {
            if (!subRect.isValid()) {
                d->positionViewAtRow(row, Qt::AlignTop | Qt::AlignBottom, offset, subRect);
            } else {
                // Note: entering here means that topRow() != bottomRow(). So at least two rows are
                // visible in the viewport, which means that the top side of the subRect is visible.
                const qreal subRectBottom = d->loadedTableInnerRect.bottom() + subRect.bottom();
                if (subRectBottom > d->viewportRect.bottom())
                    d->positionViewAtRow(row, Qt::AlignTop | Qt::AlignBottom, offset, subRect);
            }
        }
    } else if (mode == Visible) {
        if (row < topRow()) {
            d->positionViewAtRow(row, Qt::AlignTop, -offset, subRect);
        } else if (row > bottomRow()) {
            d->positionViewAtRow(row, Qt::AlignTop | Qt::AlignBottom, offset, subRect);
        } else if (subRect.isValid()) {
            if (row == topRow()) {
                const qreal subRectTop = d->loadedTableOuterRect.top() + subRect.top();
                const qreal subRectBottom = d->loadedTableOuterRect.top() + subRect.bottom();
                if (subRectBottom < d->viewportRect.top())
                    d->positionViewAtRow(row, Qt::AlignTop, offset, subRect);
                else if (subRectTop > d->viewportRect.bottom())
                    d->positionViewAtRow(row, Qt::AlignTop | Qt::AlignBottom, offset, subRect);
            } else if (row == bottomRow()) {
                // Note: entering here means that topRow() != bottomRow(). So at least two rows are
                // visible in the viewport, which means that the top side of the subRect is visible.
                const qreal subRectTop = d->loadedTableInnerRect.bottom() + subRect.top();
                if (subRectTop > d->viewportRect.bottom())
                    d->positionViewAtRow(row, Qt::AlignTop | Qt::AlignBottom, offset, subRect);
            }
        }
    } else {
        qmlWarning(this) << "Unsupported mode:" << int(mode);
    }
}

void QQuickTableView::positionViewAtColumn(int column, PositionMode mode, qreal offset, const QRectF &subRect)
{
    Q_D(QQuickTableView);
    if (column < 0 || column >= columns() || d->loadedColumns.isEmpty())
        return;

    // Note: PositionMode::Contain is from here on translated to (Qt::AlignLeft | Qt::AlignRight).
    // This is an internal (unsupported) combination which means "align right if the whole cell
    // fits inside the viewport, otherwise align left".

    if (mode & (AlignLeft | AlignRight | AlignHCenter)) {
        mode &= AlignLeft | AlignRight | AlignHCenter;
        d->positionViewAtColumn(column, Qt::Alignment(int(mode)), offset, subRect);
    } else if (mode == Contain) {
        if (column < leftColumn()) {
            d->positionViewAtColumn(column, Qt::AlignLeft, offset, subRect);
        } else if (column > rightColumn()) {
            d->positionViewAtColumn(column, Qt::AlignLeft | Qt::AlignRight, offset, subRect);
        } else if (column == leftColumn()) {
            if (!subRect.isValid()) {
                d->positionViewAtColumn(column, Qt::AlignLeft, offset, subRect);
            } else {
                const qreal subRectLeft = d->loadedTableOuterRect.left() + subRect.left();
                const qreal subRectRight = d->loadedTableOuterRect.left() + subRect.right();
                if (subRectLeft < d->viewportRect.left())
                    d->positionViewAtColumn(column, Qt::AlignLeft, offset, subRect);
                else if (subRectRight > d->viewportRect.right())
                    d->positionViewAtColumn(column, Qt::AlignLeft | Qt::AlignRight, offset, subRect);
            }
        } else if (column == rightColumn()) {
            if (!subRect.isValid()) {
                d->positionViewAtColumn(column, Qt::AlignLeft | Qt::AlignRight, offset, subRect);
            } else {
                // Note: entering here means that leftColumn() != rightColumn(). So at least two columns
                // are visible in the viewport, which means that the left side of the subRect is visible.
                const qreal subRectRight = d->loadedTableInnerRect.right() + subRect.right();
                if (subRectRight > d->viewportRect.right())
                    d->positionViewAtColumn(column, Qt::AlignLeft | Qt::AlignRight, offset, subRect);
            }
        }
    } else if (mode == Visible) {
        if (column < leftColumn()) {
            d->positionViewAtColumn(column, Qt::AlignLeft, -offset, subRect);
        } else if (column > rightColumn()) {
            d->positionViewAtColumn(column, Qt::AlignLeft | Qt::AlignRight, offset, subRect);
        } else if (subRect.isValid()) {
            if (column == leftColumn()) {
                const qreal subRectLeft = d->loadedTableOuterRect.left() + subRect.left();
                const qreal subRectRight = d->loadedTableOuterRect.left() + subRect.right();
                if (subRectRight < d->viewportRect.left())
                    d->positionViewAtColumn(column, Qt::AlignLeft, offset, subRect);
                else if (subRectLeft > d->viewportRect.right())
                    d->positionViewAtColumn(column, Qt::AlignLeft | Qt::AlignRight, offset, subRect);
            } else if (column == rightColumn()) {
                // Note: entering here means that leftColumn() != rightColumn(). So at least two columns
                // are visible in the viewport, which means that the left side of the subRect is visible.
                const qreal subRectLeft = d->loadedTableInnerRect.right() + subRect.left();
                if (subRectLeft > d->viewportRect.right())
                    d->positionViewAtColumn(column, Qt::AlignLeft | Qt::AlignRight, offset, subRect);
            }
        }
    } else {
        qmlWarning(this) << "Unsupported mode:" << int(mode);
    }
}

void QQuickTableView::positionViewAtCell(const QPoint &cell, PositionMode mode, const QPointF &offset, const QRectF &subRect)
{
    PositionMode horizontalMode = mode & ~(AlignTop | AlignBottom | AlignVCenter);
    PositionMode verticalMode = mode & ~(AlignLeft | AlignRight | AlignHCenter);
    if (!horizontalMode && !verticalMode) {
        qmlWarning(this) << "Unsupported mode:" << int(mode);
        return;
    }

    if (horizontalMode)
        positionViewAtColumn(cell.x(), horizontalMode, offset.x(), subRect);
    if (verticalMode)
        positionViewAtRow(cell.y(), verticalMode, offset.y(), subRect);
}

void QQuickTableView::positionViewAtIndex(const QModelIndex &index, PositionMode mode, const QPointF &offset, const QRectF &subRect)
{
    PositionMode horizontalMode = mode & ~(AlignTop | AlignBottom | AlignVCenter);
    PositionMode verticalMode = mode & ~(AlignLeft | AlignRight | AlignHCenter);
    if (!horizontalMode && !verticalMode) {
        qmlWarning(this) << "Unsupported mode:" << int(mode);
        return;
    }

    if (horizontalMode)
        positionViewAtColumn(columnAtIndex(index), horizontalMode, offset.x(), subRect);
    if (verticalMode)
        positionViewAtRow(rowAtIndex(index), verticalMode, offset.y(), subRect);
}

#if QT_DEPRECATED_SINCE(6, 5)
void QQuickTableView::positionViewAtCell(int column, int row, PositionMode mode, const QPointF &offset, const QRectF &subRect)
{
    PositionMode horizontalMode = mode & ~(AlignTop | AlignBottom | AlignVCenter);
    PositionMode verticalMode = mode & ~(AlignLeft | AlignRight | AlignHCenter);
    if (!horizontalMode && !verticalMode) {
        qmlWarning(this) << "Unsupported mode:" << int(mode);
        return;
    }

    if (horizontalMode)
        positionViewAtColumn(column, horizontalMode, offset.x(), subRect);
    if (verticalMode)
        positionViewAtRow(row, verticalMode, offset.y(), subRect);
}
#endif

QQuickItem *QQuickTableView::itemAtCell(const QPoint &cell) const
{
    Q_D(const QQuickTableView);
    const int modelIndex = d->modelIndexAtCell(cell);
    if (!d->loadedItems.contains(modelIndex))
        return nullptr;
    return d->loadedItems.value(modelIndex)->item;
}

#if QT_DEPRECATED_SINCE(6, 5)
QQuickItem *QQuickTableView::itemAtCell(int column, int row) const
{
    return itemAtCell(QPoint(column, row));
}
#endif

QQuickItem *QQuickTableView::itemAtIndex(const QModelIndex &index) const
{
    Q_D(const QQuickTableView);
    const int serializedIndex = d->modelIndexToCellIndex(index);
    if (!d->loadedItems.contains(serializedIndex))
        return nullptr;
    return d->loadedItems.value(serializedIndex)->item;
}

#if QT_DEPRECATED_SINCE(6, 4)
QPoint QQuickTableView::cellAtPos(qreal x, qreal y, bool includeSpacing) const
{
    return cellAtPosition(mapToItem(contentItem(), {x, y}), includeSpacing);
}

QPoint QQuickTableView::cellAtPos(const QPointF &position, bool includeSpacing) const
{
    return cellAtPosition(mapToItem(contentItem(), position), includeSpacing);
}
#endif

QPoint QQuickTableView::cellAtPosition(qreal x, qreal y, bool includeSpacing) const
{
    return cellAtPosition(QPoint(x, y), includeSpacing);
}

QPoint QQuickTableView::cellAtPosition(const QPointF &position, bool includeSpacing) const
{
    Q_D(const QQuickTableView);

    if (!d->loadedTableOuterRect.contains(position))
        return QPoint(-1, -1);

    const qreal hSpace = d->cellSpacing.width();
    const qreal vSpace = d->cellSpacing.height();
    qreal currentColumnEnd = d->loadedTableOuterRect.x();
    qreal currentRowEnd = d->loadedTableOuterRect.y();

    int foundColumn = -1;
    int foundRow = -1;

    for (const int column : d->loadedColumns) {
        currentColumnEnd += d->getEffectiveColumnWidth(column);
        if (position.x() < currentColumnEnd) {
            foundColumn = column;
            break;
        }
        currentColumnEnd += hSpace;
        if (!includeSpacing && position.x() < currentColumnEnd) {
            // Hit spacing
            return QPoint(-1, -1);
        } else if (includeSpacing && position.x() < currentColumnEnd - (hSpace / 2)) {
            foundColumn = column;
            break;
        }
    }

    for (const int row : d->loadedRows) {
        currentRowEnd += d->getEffectiveRowHeight(row);
        if (position.y() < currentRowEnd) {
            foundRow = row;
            break;
        }
        currentRowEnd += vSpace;
        if (!includeSpacing && position.y() < currentRowEnd) {
            // Hit spacing
            return QPoint(-1, -1);
        }
        if (includeSpacing && position.y() < currentRowEnd - (vSpace / 2)) {
            foundRow = row;
            break;
        }
    }

    return QPoint(foundColumn, foundRow);
}

bool QQuickTableView::isColumnLoaded(int column) const
{
    Q_D(const QQuickTableView);
    if (!d->loadedColumns.contains(column))
        return false;

    if (d->rebuildState != QQuickTableViewPrivate::RebuildState::Done) {
        // TableView is rebuilding, and none of the rows and columns
        // are completely loaded until we reach the layout phase.
        if (d->rebuildState < QQuickTableViewPrivate::RebuildState::LayoutTable)
            return false;
    }

    return true;
}

bool QQuickTableView::isRowLoaded(int row) const
{
    Q_D(const QQuickTableView);
    if (!d->loadedRows.contains(row))
        return false;

    if (d->rebuildState != QQuickTableViewPrivate::RebuildState::Done) {
        // TableView is rebuilding, and none of the rows and columns
        // are completely loaded until we reach the layout phase.
        if (d->rebuildState < QQuickTableViewPrivate::RebuildState::LayoutTable)
            return false;
    }

    return true;
}

qreal QQuickTableView::columnWidth(int column) const
{
    Q_D(const QQuickTableView);
    if (!isColumnLoaded(column))
        return -1;

    return d->getEffectiveColumnWidth(column);
}

qreal QQuickTableView::rowHeight(int row) const
{
    Q_D(const QQuickTableView);
    if (!isRowLoaded(row))
        return -1;

    return d->getEffectiveRowHeight(row);
}

qreal QQuickTableView::implicitColumnWidth(int column) const
{
    Q_D(const QQuickTableView);
    if (!isColumnLoaded(column))
        return -1;

    return d->sizeHintForColumn(column);
}

qreal QQuickTableView::implicitRowHeight(int row) const
{
    Q_D(const QQuickTableView);
    if (!isRowLoaded(row))
        return -1;

    return d->sizeHintForRow(row);
}

void QQuickTableView::setColumnWidth(int column, qreal size)
{
    Q_D(QQuickTableView);
    if (column < 0) {
        qmlWarning(this) << "column must be greather than, or equal to, zero";
        return;
    }

    if (d->syncHorizontally) {
        d->syncView->setColumnWidth(column, size);
        return;
    }

    if (qFuzzyCompare(explicitColumnWidth(column), size))
        return;

    if (size < 0)
        d->explicitColumnWidths.remove(column);
    else
        d->explicitColumnWidths.insert(column, size);

    if (d->loadedItems.isEmpty())
        return;

    const bool allColumnsLoaded = d->atTableEnd(Qt::LeftEdge) && d->atTableEnd(Qt::RightEdge);
    if (column >= leftColumn() || column <= rightColumn() || allColumnsLoaded)
        d->forceLayout(false);
}

void QQuickTableView::clearColumnWidths()
{
    Q_D(QQuickTableView);

    if (d->syncHorizontally) {
        d->syncView->clearColumnWidths();
        return;
    }

    if (d->explicitColumnWidths.isEmpty())
        return;

    d->explicitColumnWidths.clear();
    d->forceLayout(false);
}

qreal QQuickTableView::explicitColumnWidth(int column) const
{
    Q_D(const QQuickTableView);

    if (d->syncHorizontally)
        return d->syncView->explicitColumnWidth(column);

    const auto it = d->explicitColumnWidths.constFind(column);
    if (it != d->explicitColumnWidths.constEnd())
        return *it;
    return -1;
}

void QQuickTableView::setRowHeight(int row, qreal size)
{
    Q_D(QQuickTableView);
    if (row < 0) {
        qmlWarning(this) << "row must be greather than, or equal to, zero";
        return;
    }

    if (d->syncVertically) {
        d->syncView->setRowHeight(row, size);
        return;
    }

    if (qFuzzyCompare(explicitRowHeight(row), size))
        return;

    if (size < 0)
        d->explicitRowHeights.remove(row);
    else
        d->explicitRowHeights.insert(row, size);

    if (d->loadedItems.isEmpty())
        return;

    const bool allRowsLoaded = d->atTableEnd(Qt::TopEdge) && d->atTableEnd(Qt::BottomEdge);
    if (row >= topRow() || row <= bottomRow() || allRowsLoaded)
        d->forceLayout(false);
}

void QQuickTableView::clearRowHeights()
{
    Q_D(QQuickTableView);

    if (d->syncVertically) {
        d->syncView->clearRowHeights();
        return;
    }

    if (d->explicitRowHeights.isEmpty())
        return;

    d->explicitRowHeights.clear();
    d->forceLayout(false);
}

qreal QQuickTableView::explicitRowHeight(int row) const
{
    Q_D(const QQuickTableView);

    if (d->syncVertically)
        return d->syncView->explicitRowHeight(row);

    const auto it = d->explicitRowHeights.constFind(row);
    if (it != d->explicitRowHeights.constEnd())
        return *it;
    return -1;
}

QModelIndex QQuickTableView::modelIndex(const QPoint &cell) const
{
    Q_D(const QQuickTableView);
    if (cell.x() < 0 || cell.x() >= columns() || cell.y() < 0 || cell.y() >= rows())
        return {};

    auto const qaim = d->model->abstractItemModel();
    if (!qaim)
        return {};

    return qaim->index(cell.y(), cell.x());
}

QPoint QQuickTableView::cellAtIndex(const QModelIndex &index) const
{
    if (!index.isValid() || index.parent().isValid())
        return {-1, -1};
    return {index.column(), index.row()};
}

#if QT_DEPRECATED_SINCE(6, 4)
QModelIndex QQuickTableView::modelIndex(int row, int column) const
{
    static bool compat6_4 = qEnvironmentVariable("QT_QUICK_TABLEVIEW_COMPAT_VERSION") == QStringLiteral("6.4");
    if (compat6_4) {
        // In Qt 6.4.0 and 6.4.1, a source incompatible change led to row and column
        // being documented to be specified in the opposite order.
        // QT_QUICK_TABLEVIEW_COMPAT_VERSION can therefore be set to force tableview
        // to continue accepting calls to modelIndex(column, row).
        return modelIndex({row, column});
    } else {
        qmlWarning(this) << "modelIndex(row, column) is deprecated. "
                            "Use index(row, column) instead. For more information, see "
                            "https://doc.qt.io/qt-6/qml-qtquick-tableview-obsolete.html";
        return modelIndex({column, row});
    }
}
#endif

QModelIndex QQuickTableView::index(int row, int column) const
{
    return modelIndex({column, row});
}

int QQuickTableView::rowAtIndex(const QModelIndex &index) const
{
    return cellAtIndex(index).y();
}

int QQuickTableView::columnAtIndex(const QModelIndex &index) const
{
    return cellAtIndex(index).x();
}

void QQuickTableView::forceLayout()
{
    d_func()->forceLayout(true);
}

void QQuickTableView::edit(const QModelIndex &index)
{
    Q_D(QQuickTableView);

    if (!d->canEdit(index, true))
        return;

    if (d->editIndex == index)
        return;

    if (!d->tableModel)
        return;

    if (!d->editModel) {
        d->editModel = new QQmlTableInstanceModel(qmlContext(this));
        d->editModel->useImportVersion(d->resolveImportVersion());
        QObject::connect(d->editModel, &QQmlInstanceModel::initItem,
                         [this, d] (int serializedModelIndex, QObject *object) {
            // initItemCallback will call setRequiredProperty for each required property in the
            // delegate, both for this class, but also also for any subclasses. setRequiredProperty
            // is currently dependent of the QQmlTableInstanceModel that was used to create the object
            // in order to initialize required properties, so we need to set the editItem variable
            // early on, so that we can use it in setRequiredProperty.
            d->editIndex = modelIndex(d->cellAtModelIndex(serializedModelIndex));
            d->editItem = qmlobject_cast<QQuickItem*>(object);
            if (!d->editItem)
                return;
            // Initialize required properties
            d->initItemCallback(serializedModelIndex, object);
            const auto cellItem = itemAtCell(cellAtIndex(d->editIndex));
            Q_ASSERT(cellItem);
            d->editItem->setParentItem(cellItem);
            // Move the cell item to the top of the other items, to ensure
            // that e.g a focus frame ends up on top of all the cells
            cellItem->setZ(2);
        });
    }

    if (d->selectionModel)
        d->selectionModel->setCurrentIndex(index, QItemSelectionModel::NoUpdate);

    if (d->editIndex.isValid())
        closeEditor();

    const auto cellItem = itemAtCell(cellAtIndex(index));
    Q_ASSERT(cellItem);
    const auto attached = d->getAttachedObject(cellItem);
    Q_ASSERT(attached);

    d->editModel->setModel(d->tableModel->model());
    d->editModel->setDelegate(attached->editDelegate());

    const int cellIndex = d->modelIndexToCellIndex(index);
    QObject* object = d->editModel->object(cellIndex, QQmlIncubator::Synchronous);
    if (!object) {
        d->editIndex = QModelIndex();
        d->editItem = nullptr;
        qmlWarning(this) << "cannot edit: TableView.editDelegate could not be instantiated!";
        return;
    }

    // Note: at this point, editIndex and editItem has been set from initItem!

    if (!d->editItem) {
        qmlWarning(this) << "cannot edit: TableView.editDelegate is not an Item!";
        d->editItem = nullptr;
        d->editIndex = QModelIndex();
        d->editModel->release(object, QQmlInstanceModel::NotReusable);
        return;
    }

    // Reference the cell item once more, so that it doesn't
    // get reused or deleted if it leaves the viewport.
    d->model->object(cellIndex, QQmlIncubator::Synchronous);

    // Inform the delegate, and the edit delegate, that they're being edited
    d->setRequiredProperty(kRequiredProperty_editing, QVariant::fromValue(true), cellIndex, cellItem, false);

    // Transfer focus to the edit item
    d->editItem->forceActiveFocus(Qt::MouseFocusReason);

    // Install an event filter on the focus object to handle Enter and Tab.
    // Note that the focusObject doesn't need to be the editItem itself, in
    // case the editItem is a FocusScope.
    if (QObject *focusObject = d->editItem->window()->focusObject()) {
        QQuickItem *focusItem = qobject_cast<QQuickItem *>(focusObject);
        if (focusItem == d->editItem || d->editItem->isAncestorOf(focusItem))
            focusItem->installEventFilter(this);
    }
}

void QQuickTableView::closeEditor()
{
    Q_D(QQuickTableView);

    if (!d->editItem)
        return;

    QQuickItem *cellItem = d->editItem->parentItem();
    d->editModel->release(d->editItem, QQmlInstanceModel::NotReusable);
    d->editItem = nullptr;

    cellItem->setZ(1);
    const int cellIndex = d->modelIndexToCellIndex(d->editIndex);
    d->setRequiredProperty(kRequiredProperty_editing, QVariant::fromValue(false), cellIndex, cellItem, false);
    // Remove the extra reference we sat on the cell item from edit()
    d->model->release(cellItem, QQmlInstanceModel::NotReusable);

    if (d->editIndex.isValid()) {
        // Note: we can have an invalid editIndex, even when we
        // have an editItem, if the model has changed (e.g been reset)!
        d->editIndex = QModelIndex();
    }
}

QQuickTableViewAttached *QQuickTableView::qmlAttachedProperties(QObject *obj)
{
    return new QQuickTableViewAttached(obj);
}

void QQuickTableView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTableView);
    QQuickFlickable::geometryChange(newGeometry, oldGeometry);

    if (d->tableModel) {
        // When the view changes size, we force the pool to
        // shrink by releasing all pooled items.
        d->tableModel->drainReusableItemsPool(0);
    }

    d->forceLayout(false);
}

void QQuickTableView::viewportMoved(Qt::Orientations orientation)
{
    Q_D(QQuickTableView);

    // If the new viewport position was set from the setLocalViewportXY()
    // functions, we just update the position silently and return. Otherwise, if
    // the viewport was flicked by the user, or some other control, we
    // recursively sync all the views in the hierarchy to the same position.
    QQuickFlickable::viewportMoved(orientation);
    if (d->inSetLocalViewportPos)
        return;

    // Move all views in the syncView hierarchy to the same contentX/Y.
    // We need to start from this view (and not the root syncView) to
    // ensure that we respect all the individual syncDirection flags
    // between the individual views in the hierarchy.
    d->syncViewportPosRecursive();

    auto rootView = d->rootSyncView();
    auto rootView_d = rootView->d_func();

    rootView_d->scheduleRebuildIfFastFlick();

    if (!rootView_d->polishScheduled) {
        if (rootView_d->scheduledRebuildOptions) {
            // When we need to rebuild, collecting several viewport
            // moves and do a single polish gives a quicker UI.
            rootView->polish();
        } else {
            // Updating the table right away when flicking
            // slowly gives a smoother experience.
            const bool updated = rootView->d_func()->updateTableRecursive();
            if (!updated) {
                // One, or more, of the views are already in an
                // update, so we need to wait a cycle.
                rootView->polish();
            }
        }
    }
}

void QQuickTableView::keyPressEvent(QKeyEvent *e)
{
    Q_D(QQuickTableView);

    if (!d->keyNavigationEnabled) {
        QQuickFlickable::keyPressEvent(e);
        return;
    }

    if (d->tableSize.isEmpty())
        return;

    if (d->editIndex.isValid()) {
        // While editing, we limit the keys that we
        // handle to not interfere with editing.
        return;
    }

    if (d->setCurrentIndexFromKeyEvent(e))
        return;

    if (d->editFromKeyEvent(e))
        return;

    QQuickFlickable::keyPressEvent(e);
}

bool QQuickTableView::eventFilter(QObject *obj, QEvent *event)
{
    Q_D(QQuickTableView);

    if (event->type() == QEvent::KeyPress) {
        Q_ASSERT(d->editItem);
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (auto attached = d->getAttachedObject(d->editItem))
                emit attached->commit();
            closeEditor();
            return true;
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            if (activeFocusOnTab()) {
                if (auto attached = d->getAttachedObject(d->editItem))
                    emit attached->commit();
                closeEditor();
                if (d->setCurrentIndexFromKeyEvent(keyEvent)) {
                    const QModelIndex currentIndex = d->selectionModel->currentIndex();
                    if (d->canEdit(currentIndex, false))
                        edit(currentIndex);
                }
                return true;
            }
            break;
        case Qt::Key_Escape:
            closeEditor();
            return true;
        }
    }

    return QQuickFlickable::eventFilter(obj, event);
}

bool QQuickTableView::alternatingRows() const
{
    return d_func()->alternatingRows;
}

void QQuickTableView::setAlternatingRows(bool alternatingRows)
{
    Q_D(QQuickTableView);
    if (d->alternatingRows == alternatingRows)
        return;

    d->alternatingRows = alternatingRows;
    emit alternatingRowsChanged();
}

QQuickTableView::SelectionBehavior QQuickTableView::selectionBehavior() const
{
    return d_func()->selectionBehavior;
}

void QQuickTableView::setSelectionBehavior(SelectionBehavior selectionBehavior)
{
    Q_D(QQuickTableView);
    if (d->selectionBehavior == selectionBehavior)
        return;

    d->selectionBehavior = selectionBehavior;
    emit selectionBehaviorChanged();
}

QQuickTableView::SelectionMode QQuickTableView::selectionMode() const
{
    return d_func()->selectionMode;
}

void QQuickTableView::setSelectionMode(SelectionMode selectionMode)
{
    Q_D(QQuickTableView);
    if (d->selectionMode == selectionMode)
        return;

    d->selectionMode = selectionMode;
    emit selectionModeChanged();
}

bool QQuickTableView::resizableColumns() const
{
    return d_func()->resizableColumns;
}

void QQuickTableView::setResizableColumns(bool enabled)
{
    Q_D(QQuickTableView);
    if (d->resizableColumns == enabled)
        return;

    d->resizableColumns = enabled;
    d->resizeHandler->setEnabled(d->resizableRows || d->resizableColumns);
    d->hoverHandler->setEnabled(d->resizableRows || d->resizableColumns);

    emit resizableColumnsChanged();
}

bool QQuickTableView::resizableRows() const
{
    return d_func()->resizableRows;
}

void QQuickTableView::setResizableRows(bool enabled)
{
    Q_D(QQuickTableView);
    if (d->resizableRows == enabled)
        return;

    d->resizableRows = enabled;
    d->resizeHandler->setEnabled(d->resizableRows || d->resizableColumns);
    d->hoverHandler->setEnabled(d->resizableRows || d->resizableColumns);

    emit resizableRowsChanged();
}

// ----------------------------------------------

QQuickTableViewHoverHandler::QQuickTableViewHoverHandler(QQuickTableView *view)
    : QQuickHoverHandler(view->contentItem())
{
    setMargin(5);

    connect(this, &QQuickHoverHandler::hoveredChanged, [this] {
        if (!isHoveringGrid())
            return;
        m_row = -1;
        m_column = -1;
#if QT_CONFIG(cursor)
        auto tableView = static_cast<QQuickTableView *>(parentItem()->parent());
        auto tableViewPrivate = QQuickTableViewPrivate::get(tableView);
        tableViewPrivate->updateCursor();
#endif
    });
}

void QQuickTableViewHoverHandler::handleEventPoint(QPointerEvent *event, QEventPoint &point)
{
    QQuickHoverHandler::handleEventPoint(event, point);

    auto tableView = static_cast<QQuickTableView *>(parentItem()->parent());
#if QT_CONFIG(cursor)
    auto tableViewPrivate = QQuickTableViewPrivate::get(tableView);
#endif

    const QPoint cell = tableView->cellAtPosition(point.position(), true);
    const auto item = tableView->itemAtCell(cell);
    if (!item) {
        m_row = -1;
        m_column = -1;
#if QT_CONFIG(cursor)
        tableViewPrivate->updateCursor();
#endif
        return;
    }

    const QPointF itemPos = item->mapFromItem(tableView->contentItem(), point.position());
    const bool hoveringRow = (itemPos.y() < margin() || itemPos.y() > item->height() - margin());
    const bool hoveringColumn = (itemPos.x() < margin() || itemPos.x() > item->width() - margin());
    m_row = hoveringRow ? itemPos.y() < margin() ? cell.y() - 1 : cell.y() : -1;
    m_column = hoveringColumn ? itemPos.x() < margin() ? cell.x() - 1 : cell.x() : -1;
#if QT_CONFIG(cursor)
    tableViewPrivate->updateCursor();
#endif
}

// ----------------------------------------------

QQuickTableViewResizeHandler::QQuickTableViewResizeHandler(QQuickTableView *view)
    : QQuickSinglePointHandler(view->contentItem())
{
    setMargin(5);
    // Set a grab permission that stops the flickable, as well as
    // any drag handler inside the delegate, from stealing the drag.
    setGrabPermissions(QQuickPointerHandler::CanTakeOverFromAnything);
    setObjectName("tableViewResizeHandler");
}

void QQuickTableViewResizeHandler::onGrabChanged(QQuickPointerHandler *grabber
                                     , QPointingDevice::GrabTransition transition
                                     , QPointerEvent *ev
                                     , QEventPoint &point)
{
    QQuickSinglePointHandler::onGrabChanged(grabber, transition, ev, point);

    switch (transition) {
    case QPointingDevice::GrabPassive:
    case QPointingDevice::GrabExclusive:
        break;
    case QPointingDevice::UngrabPassive:
    case QPointingDevice::UngrabExclusive:
    case QPointingDevice::CancelGrabPassive:
    case QPointingDevice::CancelGrabExclusive:
    case QPointingDevice::OverrideGrabPassive:
        if (m_state == DraggingStarted || m_state == Dragging) {
            m_state = DraggingFinished;
            updateDrag(ev, point);
        }
        break;
    }
}

bool QQuickTableViewResizeHandler::wantsEventPoint(const QPointerEvent *event, const QEventPoint &point)
{
    if (!QQuickSinglePointHandler::wantsEventPoint(event, point))
        return false;

    // If we have a mouse wheel event then we do not want to do anything related to resizing.
    if (event->type() == QEvent::Type::Wheel)
        return false;

    // When the user is flicking, we disable resizing, so that
    // he doesn't start to resize by accident.
    auto tableView = static_cast<QQuickTableView *>(parentItem()->parent());
    return !tableView->isMoving();
}

void QQuickTableViewResizeHandler::handleEventPoint(QPointerEvent *event, QEventPoint &point)
{
    // Resolve which state we're in first...
    updateState(point);
    // ...and act on it next
    updateDrag(event, point);
}

void QQuickTableViewResizeHandler::updateState(QEventPoint &point)
{
    auto tableView = static_cast<QQuickTableView *>(parentItem()->parent());
    auto tableViewPrivate = QQuickTableViewPrivate::get(tableView);

    if (m_state == DraggingFinished)
        m_state = Listening;

    if (point.state() == QEventPoint::Pressed) {
        m_row = tableViewPrivate->resizableRows ? tableViewPrivate->hoverHandler->m_row : -1;
        m_column = tableViewPrivate->resizableColumns ? tableViewPrivate->hoverHandler->m_column : -1;
        if (m_row != -1 || m_column != -1)
            m_state = Tracking;
    } else if (point.state() == QEventPoint::Released) {
        if (m_state == DraggingStarted || m_state == Dragging)
            m_state = DraggingFinished;
        else
            m_state = Listening;
    } else if (point.state() == QEventPoint::Updated) {
        switch (m_state) {
        case Listening:
            break;
        case Tracking: {
            const qreal distX = m_column != -1 ? point.position().x() - point.pressPosition().x() : 0;
            const qreal distY = m_row != -1 ? point.position().y() - point.pressPosition().y() : 0;
            const qreal dragDist = qSqrt(distX * distX + distY * distY);
            if (dragDist > qApp->styleHints()->startDragDistance())
                m_state = DraggingStarted;
            break;}
        case DraggingStarted:
            m_state = Dragging;
            break;
        case Dragging:
            break;
        case DraggingFinished:
            // Handled at the top of the function
            Q_UNREACHABLE();
            break;
        }
    }
}

void QQuickTableViewResizeHandler::updateDrag(QPointerEvent *event, QEventPoint &point)
{
    auto tableView = static_cast<QQuickTableView *>(parentItem()->parent());
#if QT_CONFIG(cursor)
    auto tableViewPrivate = QQuickTableViewPrivate::get(tableView);
#endif

    switch (m_state) {
    case Listening:
        break;
    case Tracking:
        setPassiveGrab(event, point, true);
        // Disable flicking while dragging. TableView uses filtering instead of
        // pointer handlers to do flicking, so setting an exclusive grab (together
        // with grab permissions) doens't work ATM.
        tableView->setFiltersChildMouseEvents(false);
        break;
    case DraggingStarted:
        setExclusiveGrab(event, point, true);
        m_columnStartX = point.position().x();
        m_columnStartWidth = tableView->columnWidth(m_column);
        m_rowStartY = point.position().y();
        m_rowStartHeight = tableView->rowHeight(m_row);
#if QT_CONFIG(cursor)
        tableViewPrivate->updateCursor();
#endif
        Q_FALLTHROUGH();
    case Dragging: {
        const qreal distX = point.position().x() - m_columnStartX;
        const qreal distY = point.position().y() - m_rowStartY;
        if (m_column != -1)
            tableView->setColumnWidth(m_column, qMax(0.001, m_columnStartWidth + distX));
        if (m_row != -1)
            tableView->setRowHeight(m_row, qMax(0.001, m_rowStartHeight + distY));
        break; }
    case DraggingFinished: {
        tableView->setFiltersChildMouseEvents(true);
#if QT_CONFIG(cursor)
        tableViewPrivate->updateCursor();
#endif
        break; }
    }
}

// ----------------------------------------------

QQuickTableViewTapHandler::QQuickTableViewTapHandler(QQuickTableView *view)
    : QQuickTapHandler(view->contentItem())
{
    setObjectName("tableViewTapHandler");
}

bool QQuickTableViewTapHandler::wantsEventPoint(const QPointerEvent *event, const QEventPoint &point)
{
    auto tableView = static_cast<QQuickTableView *>(parentItem()->parent());
    auto tableViewPrivate = QQuickTableViewPrivate::get(tableView);
    return tableViewPrivate->pointerNavigationEnabled && QQuickTapHandler::wantsEventPoint(event, point);
}

QT_END_NAMESPACE

#include "moc_qquicktableview_p.cpp"
#include "moc_qquicktableview_p_p.cpp"
