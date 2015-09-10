/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Calendar module of the Qt Toolkit.
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

#include "qquickweeknumbercolumn_p.h"
#include "qquickweeknumbermodel_p.h"

#include <QtQuickTemplates/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickWeekNumberColumnPrivate : public QQuickControlPrivate
{
public:
    QQuickWeekNumberColumnPrivate() : delegate(Q_NULLPTR), model(Q_NULLPTR) { }

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

    foreach (QQuickItem *item, contentItem->childItems())
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
    connect(d->model, &QQuickWeekNumberModel::localeChanged, this, &QQuickWeekNumberColumn::localeChanged);
}

int QQuickWeekNumberColumn::month() const
{
    Q_D(const QQuickWeekNumberColumn);
    return d->model->month();
}

void QQuickWeekNumberColumn::setMonth(int month)
{
    Q_D(QQuickWeekNumberColumn);
    d->model->setMonth(month);
}

int QQuickWeekNumberColumn::year() const
{
    Q_D(const QQuickWeekNumberColumn);
    return d->model->year();
}

void QQuickWeekNumberColumn::setYear(int year)
{
    Q_D(QQuickWeekNumberColumn);
    d->model->setYear(year);
}

QLocale QQuickWeekNumberColumn::locale() const
{
    Q_D(const QQuickWeekNumberColumn);
    return d->model->locale();
}

void QQuickWeekNumberColumn::setLocale(const QLocale &locale)
{
    Q_D(QQuickWeekNumberColumn);
    d->model->setLocale(locale);
}

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

void QQuickWeekNumberColumn::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickWeekNumberColumn);
    QQuickControl::geometryChanged(newGeometry, oldGeometry);
    if (isComponentComplete())
        d->resizeItems();
}

void QQuickWeekNumberColumn::paddingChange(const QMarginsF &newPadding, const QMarginsF &oldPadding)
{
    Q_D(QQuickWeekNumberColumn);
    QQuickControl::paddingChange(newPadding, oldPadding);
    if (isComponentComplete())
        d->resizeItems();
}

QT_END_NAMESPACE
