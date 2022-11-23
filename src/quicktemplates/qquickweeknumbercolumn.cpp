// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickweeknumbercolumn_p.h"
#include "qquickweeknumbermodel_p.h"

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>
#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype WeekNumberColumn
    \inherits Control
//!     \instantiates QQuickWeekNumberColumn
    \inqmlmodule QtQuick.Controls
    \brief A column of week numbers.

    WeekNumberColumn presents week numbers in a column. The week numbers
    are calculated for a given \l month and \l year, using the specified
    \l {Control::locale}{locale}.

    \image qtquickcontrols-weeknumbercolumn.png
    \snippet qtquickcontrols-weeknumbercolumn.qml 1

    WeekNumberColumn can be used as a standalone control, but it is most
    often used in conjunction with MonthGrid. Regardless of the use case,
    positioning of the column is left to the user.

    \image qtquickcontrols-weeknumbercolumn-layout.png
    \snippet qtquickcontrols-weeknumbercolumn-layout.qml 1

    The visual appearance of WeekNumberColumn can be changed by
    implementing a \l {delegate}{custom delegate}.

    \sa MonthGrid, DayOfWeekRow
*/

class QQuickWeekNumberColumnPrivate : public QQuickControlPrivate
{
public:
    QQuickWeekNumberColumnPrivate() : delegate(nullptr), model(nullptr) { }

    void resizeItems();

    QVariant source;
    QQmlComponent *delegate;
    QQuickWeekNumberModel *model;
};

void QQuickWeekNumberColumnPrivate::resizeItems()
{
    if (!contentItem)
        return;

    QSizeF itemSize;
    itemSize.setWidth(contentItem->width());
    itemSize.setHeight((contentItem->height() - 5 * spacing) / 6);

    const auto childItems = contentItem->childItems();
    for (QQuickItem *item : childItems)
        item->setSize(itemSize);
}

QQuickWeekNumberColumn::QQuickWeekNumberColumn(QQuickItem *parent) :
    QQuickControl(*(new QQuickWeekNumberColumnPrivate), parent)
{
    Q_D(QQuickWeekNumberColumn);
    d->model = new QQuickWeekNumberModel(this);
    d->source = QVariant::fromValue(d->model);
    connect(d->model, &QQuickWeekNumberModel::monthChanged, this, &QQuickWeekNumberColumn::monthChanged);
    connect(d->model, &QQuickWeekNumberModel::yearChanged, this, &QQuickWeekNumberColumn::yearChanged);
}

/*!
    \qmlproperty int QtQuick.Controls::WeekNumberColumn::month

    This property holds the number of the month that the week numbers are
    calculated for. The default value is the current month.

    \include zero-based-months.qdocinc

    \sa Calendar
*/
int QQuickWeekNumberColumn::month() const
{
    Q_D(const QQuickWeekNumberColumn);
    return d->model->month() - 1;
}

void QQuickWeekNumberColumn::setMonth(int month)
{
    Q_D(QQuickWeekNumberColumn);
    if (month < 0 || month > 11) {
        qmlWarning(this) << "month " << month << " is out of range [0...11]";
        return;
    }
    d->model->setMonth(month + 1);
}

/*!
    \qmlproperty int QtQuick.Controls::WeekNumberColumn::year

    This property holds the number of the year that the week numbers are calculated for.

    The value must be in the range from \c -271820 to \c 275759. The default
    value is the current year.
*/
int QQuickWeekNumberColumn::year() const
{
    Q_D(const QQuickWeekNumberColumn);
    return d->model->year();
}

void QQuickWeekNumberColumn::setYear(int year)
{
    Q_D(QQuickWeekNumberColumn);
    if (year < -271820 || year > 275759) {
        qmlWarning(this) << "year " << year << " is out of range [-271820...275759]";
        return;
    }
    d->model->setYear(year);
}

/*!
    \internal
    \qmlproperty model QtQuick.Controls::WeekNumberColumn::source

    This property holds the source model that is used as a data model
    for the internal content column.
*/
QVariant QQuickWeekNumberColumn::source() const
{
    Q_D(const QQuickWeekNumberColumn);
    return d->source;
}

void QQuickWeekNumberColumn::setSource(const QVariant &source)
{
    Q_D(QQuickWeekNumberColumn);
    if (d->source != source) {
        d->source = source;
        emit sourceChanged();
    }
}

/*!
    \qmlproperty Component QtQuick.Controls::WeekNumberColumn::delegate

    This property holds the item delegate that visualizes each week number.

    In addition to the \c index property, a list of model data roles
    are available in the context of each delegate:
    \table
        \row \li \b model.weekNumber : int \li The week number
    \endtable

    The following snippet presents the default implementation of the item
    delegate. It can be used as a starting point for implementing custom
    delegates.

    \snippet basic/WeekNumberColumn.qml delegate
*/
QQmlComponent *QQuickWeekNumberColumn::delegate() const
{
    Q_D(const QQuickWeekNumberColumn);
    return d->delegate;
}

void QQuickWeekNumberColumn::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickWeekNumberColumn);
    if (d->delegate != delegate) {
        d->delegate = delegate;
        emit delegateChanged();
    }
}

void QQuickWeekNumberColumn::componentComplete()
{
    Q_D(QQuickWeekNumberColumn);
    QQuickControl::componentComplete();
    d->resizeItems();
}

void QQuickWeekNumberColumn::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickWeekNumberColumn);
    QQuickControl::geometryChange(newGeometry, oldGeometry);
    if (isComponentComplete())
        d->resizeItems();
}

void QQuickWeekNumberColumn::localeChange(const QLocale &newLocale, const QLocale &oldLocale)
{
    Q_D(QQuickWeekNumberColumn);
    QQuickControl::localeChange(newLocale, oldLocale);
    d->model->setLocale(newLocale);
}

void QQuickWeekNumberColumn::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_D(QQuickWeekNumberColumn);
    QQuickControl::paddingChange(newPadding, oldPadding);
    if (isComponentComplete())
        d->resizeItems();
}

QT_END_NAMESPACE

#include "moc_qquickweeknumbercolumn_p.cpp"
