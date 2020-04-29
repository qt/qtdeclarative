/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Calendar module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickdayofweekrow_p.h"
#include "qquickdayofweekmodel_p.h"

#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype DayOfWeekRow
    \inherits Control
//!     \instantiates QQuickDayOfWeekRow
    \inqmlmodule Qt.labs.calendar
    \brief A row of names for the days in a week.

    DayOfWeekRow presents day of week names in a row. The names of the days
    are ordered and formatted using the specified \l {Control::locale}{locale}.

    \image qtlabscalendar-dayofweekrow.png
    \snippet qtlabscalendar-dayofweekrow.qml 1

    DayOfWeekRow can be used as a standalone control, but it is most
    often used in conjunction with MonthGrid. Regardless of the use case,
    positioning of the row is left to the user.

    \image qtlabscalendar-dayofweekrow-layout.png
    \snippet qtlabscalendar-dayofweekrow-layout.qml 1

    The visual appearance of DayOfWeekRow can be changed by
    implementing a \l {delegate}{custom delegate}.

    \labs

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
    \qmlproperty model Qt.labs.calendar::DayOfWeekRow::source

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
    \qmlproperty Component Qt.labs.calendar::DayOfWeekRow::delegate

    This property holds the item delegate that visualizes each day of the week.

    In addition to the \c index property, a list of model data roles
    are available in the context of each delegate:
    \table
        \row \li \b model.day : int \li The day of week (\l Qt::DayOfWeek)
        \row \li \b model.longName : string \li The long version of the day name; for example, "Monday" (\l QLocale::LongFormat)
        \row \li \b model.shortName : string \li The short version of the day name; for example, "Mon" (\l QLocale::ShortFormat)
        \row \li \b model.narrowName : string \li A special version of the day name for use when space is limited; for example, "M" (\l QLocale::NarrowFormat)
    \endtable

    The following snippet presents the default implementation of the item
    delegate. It can be used as a starting point for implementing custom
    delegates.

    \snippet DayOfWeekRow.qml delegate
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
