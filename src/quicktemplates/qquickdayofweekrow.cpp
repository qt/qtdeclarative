// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdayofweekrow_p.h"
#include "qquickdayofweekmodel_p.h"

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype DayOfWeekRow
    \inherits Control
//!     \nativetype QQuickDayOfWeekRow
    \inqmlmodule QtQuick.Controls
    \brief A row of names for the days in a week.

    DayOfWeekRow presents day of week names in a row. The names of the days
    are ordered and formatted using the specified \l {Control::locale}{locale}.

    \image qtquickcontrols-dayofweekrow.webp
    \snippet qtquickcontrols-dayofweekrow.qml 1

    DayOfWeekRow can be used as a standalone control, but it is most
    often used in conjunction with MonthGrid. Regardless of the use case,
    positioning of the row is left to the user.

    \image qtquickcontrols-dayofweekrow-layout.webp
    \snippet qtquickcontrols-dayofweekrow-layout.qml 1

    The visual appearance of DayOfWeekRow can be changed by
    implementing a \l {delegate}{custom delegate}.

    \sa MonthGrid, WeekNumberColumn
*/

class QQuickDayOfWeekRowPrivate : public QQuickControlPrivate
{
public:
    QQuickDayOfWeekRowPrivate() : delegate(nullptr), model(nullptr) { }

    void resizeItems();

    QVariant source;
    QQmlComponent *delegate;
    QQuickDayOfWeekModel *model;
};

void QQuickDayOfWeekRowPrivate::resizeItems()
{
    if (!contentItem)
        return;

    QSizeF itemSize;
    itemSize.setWidth((contentItem->width() - 6 * spacing) / 7);
    itemSize.setHeight(contentItem->height());

    const auto childItems = contentItem->childItems();
    for (QQuickItem *item : childItems)
        item->setSize(itemSize);
}

QQuickDayOfWeekRow::QQuickDayOfWeekRow(QQuickItem *parent) :
    QQuickControl(*(new QQuickDayOfWeekRowPrivate), parent)
{
    Q_D(QQuickDayOfWeekRow);
    d->model = new QQuickDayOfWeekModel(this);
    d->source = QVariant::fromValue(d->model);
}

/*!
    \internal
    \qmlproperty model QtQuick.Controls::DayOfWeekRow::source

    This property holds the source model that is used as a data model
    for the internal content row.
*/
QVariant QQuickDayOfWeekRow::source() const
{
    Q_D(const QQuickDayOfWeekRow);
    return d->source;
}

void QQuickDayOfWeekRow::setSource(const QVariant &source)
{
    Q_D(QQuickDayOfWeekRow);
    if (d->source != source) {
        d->source = source;
        emit sourceChanged();
    }
}

/*!
    \qmlproperty Component QtQuick.Controls::DayOfWeekRow::delegate

    This property holds the item delegate that visualizes each day of the week.

    In addition to the \c index property, a list of model data roles
    are available in the context of each delegate:
    \table
        \row
        \li \b model.day : int
        \li The day of week (\l Qt::DayOfWeek)
        \row
        \li \b model.longName : string
        \li The long version of the day name; for example,
   "Monday" (\l QLocale::LongFormat)
        \row
        \li \b model.shortName : string
        \li The short version of the day name; for example, "Mon"
        (\l QLocale::ShortFormat)
        \row
        \li \b model.narrowName : string
        \li A special version of the day name for use when space is limited.
        For example, "M" (\l QLocale::NarrowFormat)
    \endtable

    The following snippet presents the default implementation of the item
    delegate. It can be used as a starting point for implementing custom
    delegates.

    \snippet basic/DayOfWeekRow.qml delegate
*/
QQmlComponent *QQuickDayOfWeekRow::delegate() const
{
    Q_D(const QQuickDayOfWeekRow);
    return d->delegate;
}

void QQuickDayOfWeekRow::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickDayOfWeekRow);
    if (d->delegate != delegate) {
        d->delegate = delegate;
        emit delegateChanged();
    }
}

void QQuickDayOfWeekRow::componentComplete()
{
    Q_D(QQuickDayOfWeekRow);
    QQuickControl::componentComplete();
    d->resizeItems();
}

void QQuickDayOfWeekRow::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickDayOfWeekRow);
    QQuickControl::geometryChange(newGeometry, oldGeometry);
    if (isComponentComplete())
        d->resizeItems();
}

void QQuickDayOfWeekRow::localeChange(const QLocale &newLocale, const QLocale &oldLocale)
{
    Q_D(QQuickDayOfWeekRow);
    QQuickControl::localeChange(newLocale, oldLocale);
    d->model->setLocale(newLocale);
}

void QQuickDayOfWeekRow::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_D(QQuickDayOfWeekRow);
    QQuickControl::paddingChange(newPadding, oldPadding);
    if (isComponentComplete())
        d->resizeItems();
}

QT_END_NAMESPACE

#include "moc_qquickdayofweekrow_p.cpp"
