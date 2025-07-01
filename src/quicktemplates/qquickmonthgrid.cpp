// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qquickmonthgrid_p.h"
#include "qquickmonthmodel_p.h"

#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype MonthGrid
    \inherits Control
//!     \nativetype QQuickMonthGrid
    \inqmlmodule QtQuick.Controls
    \brief A grid of days for a calendar month.

    MonthGrid presents a calendar month in a grid. The contents are
    calculated for a given \l month and \l year, using the specified
    \l {Control::locale}{locale}.

    \image qtquickcontrols-monthgrid.webp
    \snippet qtquickcontrols-monthgrid.qml 1

    MonthGrid can be used as a standalone control, but it is most often
    used in conjunction with DayOfWeekRow and WeekNumberColumn. Regardless
    of the use case, positioning of the grid is left to the user.

    \image qtquickcontrols-monthgrid-layout.webp
    \snippet qtquickcontrols-monthgrid-layout.qml 1

    The visual appearance of MonthGrid can be changed by
    implementing a \l {delegate}{custom delegate}.

    When viewing any given month, MonthGrid shows days from the previous and
    next month. This means it always shows six rows, even when first or last
    row is entirely within an adjacent month.

    \section1 Localizing days

    To localize days, use \l {Locale::toString()}{Locale.toString()}.
    For example, to display day numbers in an Arabic locale:

    \snippet qtquickcontrols-monthgrid-localization.qml 1

    \sa DayOfWeekRow, WeekNumberColumn, CalendarModel,
        {Qt Quick Controls - Event Calendar}
*/

/*!
    \qmlsignal QtQuick.Controls::MonthGrid::pressed(date date)

    This signal is emitted when \a date is pressed.
*/

/*!
    \qmlsignal QtQuick.Controls::MonthGrid::released(date date)

    This signal is emitted when \a date is released.
*/

/*!
    \qmlsignal QtQuick.Controls::MonthGrid::clicked(date date)

    This signal is emitted when \a date is clicked.
*/

/*!
    \qmlsignal QtQuick.Controls::MonthGrid::pressAndHold(date date)

    This signal is emitted when \a date is pressed and held down.
*/

class QQuickMonthGridPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickMonthGrid)

public:
    QQuickMonthGridPrivate() : pressTimer(0), pressedItem(nullptr), model(nullptr), delegate(nullptr) { }

    void resizeItems();

    QQuickItem *cellAt(const QPointF &pos) const;
    QDateTime dateOf(QQuickItem *cell) const;

    void updatePress(const QPointF &pos);
    void clearPress(bool clicked);

    bool handlePress(const QPointF &point, ulong timestamp) override;
    bool handleMove(const QPointF &point, ulong timestamp) override;
    bool handleRelease(const QPointF &point, ulong timestamp) override;
    void handleUngrab() override;

    QString title;
    QVariant source;

    // Only the date matters, but we have to store it as QDateTime for compatibility
    // with Date: QTBUG-72208. See QQuickMonthModelPrivate::populate for more info.
    QDateTime pressedDate;
    int pressTimer;
    QQuickItem *pressedItem;
    QQuickMonthModel *model;
    QQmlComponent *delegate;
};

void QQuickMonthGridPrivate::resizeItems()
{
    if (!contentItem)
        return;

    QSizeF itemSize;
    itemSize.setWidth((contentItem->width() - 6 * spacing) / 7);
    itemSize.setHeight((contentItem->height() - 5 * spacing) / 6);

    const auto childItems = contentItem->childItems();
    for (QQuickItem *item : childItems) {
        if (!QQuickItemPrivate::get(item)->isTransparentForPositioner())
            item->setSize(itemSize);
    }
}

QQuickItem *QQuickMonthGridPrivate::cellAt(const QPointF &pos) const
{
    Q_Q(const QQuickMonthGrid);
    if (contentItem) {
        QPointF mapped = q->mapToItem(contentItem, pos);
        return contentItem->childAt(mapped.x(), mapped.y());
    }
    return nullptr;
}

QDateTime QQuickMonthGridPrivate::dateOf(QQuickItem *cell) const
{
    if (contentItem)
        return model->dateAt(contentItem->childItems().indexOf(cell));
    return {};
}

void QQuickMonthGridPrivate::updatePress(const QPointF &pos)
{
    Q_Q(QQuickMonthGrid);
    clearPress(false);
    pressedItem = cellAt(pos);
    pressedDate = dateOf(pressedItem);
    if (pressedDate.isValid())
        emit q->pressed(pressedDate);
}

void QQuickMonthGridPrivate::clearPress(bool clicked)
{
    Q_Q(QQuickMonthGrid);
    if (pressedDate.isValid()) {
        emit q->released(pressedDate);
        if (clicked)
            emit q->clicked(pressedDate);
    }
    pressedDate = {};
    pressedItem = nullptr;
}

bool QQuickMonthGridPrivate::handlePress(const QPointF &point, ulong timestamp)
{
    Q_Q(QQuickMonthGrid);
    QQuickControlPrivate::handlePress(point, timestamp);
    updatePress(point);
    if (pressedDate.isValid())
        pressTimer = q->startTimer(qGuiApp->styleHints()->mousePressAndHoldInterval());
    return true;
}

bool QQuickMonthGridPrivate::handleMove(const QPointF &point, ulong timestamp)
{
    QQuickControlPrivate::handleMove(point, timestamp);
    updatePress(point);
    return true;
}

bool QQuickMonthGridPrivate::handleRelease(const QPointF &point, ulong timestamp)
{
    QQuickControlPrivate::handleRelease(point, timestamp);
    clearPress(true);
    return true;
}

void QQuickMonthGridPrivate::handleUngrab()
{
    QQuickControlPrivate::handleUngrab();
    clearPress(false);
}

QQuickMonthGrid::QQuickMonthGrid(QQuickItem *parent) :
    QQuickControl(*(new QQuickMonthGridPrivate), parent)
{
    Q_D(QQuickMonthGrid);
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);
#if QT_CONFIG(cursor)
    setCursor(Qt::ArrowCursor);
#endif

    d->model = new QQuickMonthModel(this);
    d->source = QVariant::fromValue(d->model);
    connect(d->model, &QQuickMonthModel::monthChanged, this, &QQuickMonthGrid::monthChanged);
    connect(d->model, &QQuickMonthModel::yearChanged, this, &QQuickMonthGrid::yearChanged);
    connect(d->model, &QQuickMonthModel::titleChanged, this, &QQuickMonthGrid::titleChanged);
}

/*!
    \qmlproperty int QtQuick.Controls::MonthGrid::month

    This property holds the number of the month. The default value is the
    current month.

    \include zero-based-months.qdocinc

    \sa Calendar
*/
int QQuickMonthGrid::month() const
{
    Q_D(const QQuickMonthGrid);
    return d->model->month() - 1;
}

void QQuickMonthGrid::setMonth(int month)
{
    Q_D(QQuickMonthGrid);
    if (month < 0 || month > 11) {
        qmlWarning(this) << "month " << month << " is out of range [0...11]";
        return;
    }
    d->model->setMonth(month + 1);
}

/*!
    \qmlproperty int QtQuick.Controls::MonthGrid::year

    This property holds the number of the year.

    The value must be in the range from \c -271820 to \c 275759. The default
    value is the current year.
*/
int QQuickMonthGrid::year() const
{
    Q_D(const QQuickMonthGrid);
    return d->model->year();
}

void QQuickMonthGrid::setYear(int year)
{
    Q_D(QQuickMonthGrid);
    if (year < -271820 || year > 275759) {
        qmlWarning(this) << "year " << year << " is out of range [-271820...275759]";
        return;
    }
    d->model->setYear(year);
}

/*!
    \internal
    \qmlproperty model QtQuick.Controls::MonthGrid::source

    This property holds the source model that is used as a data model
    for the internal content column.
*/
QVariant QQuickMonthGrid::source() const
{
    Q_D(const QQuickMonthGrid);
    return d->source;
}

void QQuickMonthGrid::setSource(const QVariant &source)
{
    Q_D(QQuickMonthGrid);
    if (d->source != source) {
        d->source = source;
        emit sourceChanged();
    }
}

/*!
    \qmlproperty string QtQuick.Controls::MonthGrid::title

    This property holds a title for the calendar.

    This property is provided for convenience. MonthGrid itself does
    not visualize the title. The default value consists of the month name,
    formatted using \l {Control::locale}{locale}, and the year number.
*/
QString QQuickMonthGrid::title() const
{
    Q_D(const QQuickMonthGrid);
    if (d->title.isNull())
        return d->model->title();
    return d->title;
}

void QQuickMonthGrid::setTitle(const QString &title)
{
    Q_D(QQuickMonthGrid);
    if (d->title != title) {
        d->title = title;
        emit titleChanged();
    }
}

/*!
    \qmlproperty Component QtQuick.Controls::MonthGrid::delegate

    This property holds the item delegate that visualizes each day.

    In addition to the \c index property, a list of model data roles
    are available in the context of each delegate:
    \table
        \row \li \b model.date : date \li The date of the cell
        \row \li \b model.day : int \li The number of the day
        \row \li \b model.today : bool \li Whether the delegate represents today
        \row \li \b model.weekNumber : int \li The week number
        \row \li \b model.month : int \li The number of the month
        \row \li \b model.year : int \li The number of the year
    \endtable

    The following snippet presents the default implementation of the item
    delegate. It can be used as a starting point for implementing custom
    delegates.

    \snippet basic/MonthGrid.qml delegate
*/
QQmlComponent *QQuickMonthGrid::delegate() const
{
    Q_D(const QQuickMonthGrid);
    return d->delegate;
}

void QQuickMonthGrid::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickMonthGrid);
    if (d->delegate != delegate) {
        d->delegate = delegate;
        emit delegateChanged();
    }
}

void QQuickMonthGrid::componentComplete()
{
    Q_D(QQuickMonthGrid);
    QQuickControl::componentComplete();
    d->resizeItems();
}

void QQuickMonthGrid::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickMonthGrid);
    QQuickControl::geometryChange(newGeometry, oldGeometry);
    if (isComponentComplete())
        d->resizeItems();
}

void QQuickMonthGrid::localeChange(const QLocale &newLocale, const QLocale &oldLocale)
{
    Q_D(QQuickMonthGrid);
    QQuickControl::localeChange(newLocale, oldLocale);
    d->model->setLocale(newLocale);
}

void QQuickMonthGrid::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_D(QQuickMonthGrid);
    QQuickControl::paddingChange(newPadding, oldPadding);
    if (isComponentComplete())
        d->resizeItems();
}

void QQuickMonthGrid::updatePolish()
{
    Q_D(QQuickMonthGrid);
    QQuickControl::updatePolish();
    d->resizeItems();
}

void QQuickMonthGrid::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickMonthGrid);
    if (event->timerId() == d->pressTimer) {
        if (d->pressedDate.isValid())
            emit pressAndHold(d->pressedDate);
        killTimer(d->pressTimer);
    }
}

QT_END_NAMESPACE

#include "moc_qquickmonthgrid_p.cpp"
