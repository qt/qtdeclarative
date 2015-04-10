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

#include "qquickcalendarview_p.h"
#include "qquickmonthmodel_p.h"

#include <QtGui/qstylehints.h>
#include <QtGui/qguiapplication.h>
#include <QtQuickControls/private/qquickcontainer_p_p.h>

QT_BEGIN_NAMESPACE

class QQuickCalendarViewPrivate : public QQuickContainerPrivate
{
    Q_DECLARE_PUBLIC(QQuickCalendarView)

public:
    QQuickCalendarViewPrivate() : pressTimer(0), pressedItem(Q_NULLPTR), model(Q_NULLPTR) { }

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
};

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
    QQuickContainer(*(new QQuickCalendarViewPrivate), parent)
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

void QQuickCalendarView::componentComplete()
{
    Q_D(QQuickCalendarView);
    QQuickContainer::componentComplete();
    if (d->contentItem) {
        foreach (QQuickItem *child, d->contentItem->childItems()) {
            if (!child->inherits("QQuickRepeater"))
                d->setContextProperty(child, QStringLiteral("pressed"), false);
        }
    }
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
