/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Calendar module of the Qt Toolkit.
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

#include "qquickcalendarview_p.h"
#include "qquickmonthmodel_p.h"

#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>
#include <QtLabsTemplates/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype CalendarView
    \inherits Control
    \instantiates QQuickCalendarView
    \inqmlmodule Qt.labs.calendar
    \brief A calendar view.

    CalendarView presents a calendar month in a grid. The contents
    are calculated for a given \l month and \l year, using the specified
    \l locale.

    \image qtlabscalendar-calendarview.png
    \snippet calendarview/qtlabscalendar-calendarview.qml 1

    CalendarView can be used as a standalone control, but it is most often
    used in conjunction with DayOfWeekRow and WeekNumberColumn. Regardless
    of the use case, positioning of the grid is left to the user.

    \image qtlabscalendar-calendarview-layout.png
    \snippet calendarview/qtlabscalendar-calendarview-layout.qml 1

    The visual appearance of CalendarView can be changed by
    implementing a \l {delegate}{custom delegate}.

    \sa DayOfWeekRow, WeekNumberColumn, CalendarModel
*/

class QQuickCalendarViewPrivate : public QQuickControlPrivate
{
    Q_DECLARE_PUBLIC(QQuickCalendarView)

public:
    QQuickCalendarViewPrivate() : pressTimer(0), pressedItem(Q_NULLPTR), model(Q_NULLPTR), delegate(Q_NULLPTR) { }

    void resizeItems();

    QQuickItem *cellAt(const QPoint &pos) const;
    QDate dateOf(QQuickItem *cell) const;

    void updatePress(const QPoint &pos);
    void clearPress(bool clicked);

    static void setContextProperty(QQuickItem *item, const QString &name, const QVariant &value);

    QString title;
    QVariant source;
    QDate pressedDate;
    int pressTimer;
    QQuickItem *pressedItem;
    QQuickMonthModel *model;
    QQmlComponent *delegate;
};

void QQuickCalendarViewPrivate::resizeItems()
{
    if (!contentItem)
        return;

    QSizeF itemSize;
    itemSize.setWidth((contentItem->width() - 6 * spacing) / 7);
    itemSize.setHeight((contentItem->height() - 5 * spacing) / 6);

    foreach (QQuickItem *item, contentItem->childItems())
        item->setSize(itemSize);
}

QQuickItem *QQuickCalendarViewPrivate::cellAt(const QPoint &pos) const
{
    Q_Q(const QQuickCalendarView);
    if (contentItem) {
        QPointF mapped = q->mapToItem(contentItem, pos);
        return contentItem->childAt(mapped.x(), mapped.y());
    }
    return Q_NULLPTR;
}

QDate QQuickCalendarViewPrivate::dateOf(QQuickItem *cell) const
{
    if (contentItem)
        return model->dateAt(contentItem->childItems().indexOf(cell));
    return QDate();
}

void QQuickCalendarViewPrivate::updatePress(const QPoint &pos)
{
    Q_Q(QQuickCalendarView);
    clearPress(false);
    pressedItem = cellAt(pos);
    setContextProperty(pressedItem, QStringLiteral("pressed"), true);
    pressedDate = dateOf(pressedItem);
    if (pressedDate.isValid())
        emit q->pressed(pressedDate);
}

void QQuickCalendarViewPrivate::clearPress(bool clicked)
{
    Q_Q(QQuickCalendarView);
    setContextProperty(pressedItem, QStringLiteral("pressed"), false);
    if (pressedDate.isValid()) {
        emit q->released(pressedDate);
        if (clicked)
            emit q->clicked(pressedDate);
    }
    pressedDate = QDate();
    pressedItem = Q_NULLPTR;
}

void QQuickCalendarViewPrivate::setContextProperty(QQuickItem *item, const QString &name, const QVariant &value)
{
    QQmlContext *context = qmlContext(item);
    if (context && context->isValid()) {
        context = context->parentContext();
        if (context && context->isValid())
            context->setContextProperty(name, value);
    }
}

QQuickCalendarView::QQuickCalendarView(QQuickItem *parent) :
    QQuickControl(*(new QQuickCalendarViewPrivate), parent)
{
    Q_D(QQuickCalendarView);
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton);

    d->model = new QQuickMonthModel(this);
    d->source = QVariant::fromValue(d->model);
    connect(d->model, &QQuickMonthModel::monthChanged, this, &QQuickCalendarView::monthChanged);
    connect(d->model, &QQuickMonthModel::yearChanged, this, &QQuickCalendarView::yearChanged);
    connect(d->model, &QQuickMonthModel::localeChanged, this, &QQuickCalendarView::localeChanged);
    connect(d->model, &QQuickMonthModel::titleChanged, this, &QQuickCalendarView::titleChanged);
}

/*!
    \qmlproperty int Qt.labs.calendar::CalendarView::month

    This property holds the number of the month.
*/
int QQuickCalendarView::month() const
{
    Q_D(const QQuickCalendarView);
    return d->model->month();
}

void QQuickCalendarView::setMonth(int month)
{
    Q_D(QQuickCalendarView);
    d->model->setMonth(month);
}

/*!
    \qmlproperty int Qt.labs.calendar::CalendarView::year

    This property holds the number of the year.
*/
int QQuickCalendarView::year() const
{
    Q_D(const QQuickCalendarView);
    return d->model->year();
}

void QQuickCalendarView::setYear(int year)
{
    Q_D(QQuickCalendarView);
    d->model->setYear(year);
}

/*!
    \qmlproperty Locale Qt.labs.calendar::CalendarView::locale

    This property holds the locale that is used to calculate the contents.
*/
QLocale QQuickCalendarView::locale() const
{
    Q_D(const QQuickCalendarView);
    return d->model->locale();
}

void QQuickCalendarView::setLocale(const QLocale &locale)
{
    Q_D(QQuickCalendarView);
    d->model->setLocale(locale);
}

/*!
    \internal
    \qmlproperty model Qt.labs.calendar::CalendarView::source

    This property holds the source model that is used as a data model
    for the internal content column.
*/
QVariant QQuickCalendarView::source() const
{
    Q_D(const QQuickCalendarView);
    return d->source;
}

void QQuickCalendarView::setSource(const QVariant &source)
{
    Q_D(QQuickCalendarView);
    if (d->source != source) {
        d->source = source;
        emit sourceChanged();
    }
}

/*!
    \qmlproperty string Qt.labs.calendar::CalendarView::title

    This property holds a title for the calendar.

    This property is provided for convenience. CalendarView itself does
    not visualize the title. The default value consists of the month name,
    formatted using \l locale, and the year number.
*/
QString QQuickCalendarView::title() const
{
    Q_D(const QQuickCalendarView);
    if (d->title.isNull())
        return d->model->title();
    return d->title;
}

void QQuickCalendarView::setTitle(const QString &title)
{
    Q_D(QQuickCalendarView);
    if (d->title != title) {
        d->title = title;
        emit titleChanged();
    }
}

/*!
    \qmlproperty Component Qt.labs.calendar::CalendarView::delegate

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

    \snippet CalendarView.qml delegate
*/
QQmlComponent *QQuickCalendarView::delegate() const
{
    Q_D(const QQuickCalendarView);
    return d->delegate;
}

void QQuickCalendarView::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickCalendarView);
    if (d->delegate != delegate) {
        d->delegate = delegate;
        emit delegateChanged();
    }
}

void QQuickCalendarView::componentComplete()
{
    Q_D(QQuickCalendarView);
    QQuickControl::componentComplete();
    if (d->contentItem) {
        foreach (QQuickItem *child, d->contentItem->childItems()) {
            if (!QQuickItemPrivate::get(child)->isTransparentForPositioner())
                d->setContextProperty(child, QStringLiteral("pressed"), false);
        }
    }
    d->resizeItems();
}

void QQuickCalendarView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickCalendarView);
    QQuickControl::geometryChanged(newGeometry, oldGeometry);
    if (isComponentComplete())
        d->resizeItems();
}

void QQuickCalendarView::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_D(QQuickCalendarView);
    QQuickControl::paddingChange(newPadding, oldPadding);
    if (isComponentComplete())
        d->resizeItems();
}

void QQuickCalendarView::updatePolish()
{
    Q_D(QQuickCalendarView);
    QQuickControl::updatePolish();
    d->resizeItems();
}

void QQuickCalendarView::mousePressEvent(QMouseEvent *event)
{
    Q_D(QQuickCalendarView);
    d->updatePress(event->pos());
    if (d->pressedDate.isValid())
        d->pressTimer = startTimer(qGuiApp->styleHints()->mousePressAndHoldInterval());
    event->accept();
}

void QQuickCalendarView::mouseMoveEvent(QMouseEvent *event)
{
    Q_D(QQuickCalendarView);
    d->updatePress(event->pos());
    event->accept();
}

void QQuickCalendarView::mouseReleaseEvent(QMouseEvent *event)
{
    Q_D(QQuickCalendarView);
    d->clearPress(true);
    event->accept();
}

void QQuickCalendarView::mouseUngrabEvent()
{
    Q_D(QQuickCalendarView);
    d->clearPress(false);
}

void QQuickCalendarView::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickCalendarView);
    if (event->timerId() == d->pressTimer) {
        if (d->pressedDate.isValid())
            emit pressAndHold(d->pressedDate);
        killTimer(d->pressTimer);
    }
}

QT_END_NAMESPACE
