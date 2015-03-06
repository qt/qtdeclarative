/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

#include "qquickabstractapplicationwindow_p.h"

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype ApplicationWindow
    \inherits Window
    \instantiates QQuickAbstractApplicationWindow
    \inqmlmodule QtQuick.Controls
    \ingroup application
    \brief An application window.

    TODO
*/

class QQuickAbstractApplicationWindowPrivate : public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickAbstractApplicationWindow)

public:
    QQuickAbstractApplicationWindowPrivate() : contentWidth(0), contentHeight(0), header(Q_NULLPTR), footer(Q_NULLPTR) { }

    void relayout();

    void itemImplicitWidthChanged(QQuickItem *item) Q_DECL_OVERRIDE;
    void itemImplicitHeightChanged(QQuickItem *item) Q_DECL_OVERRIDE;

    qreal contentWidth;
    qreal contentHeight;
    QQuickItem *header;
    QQuickItem *footer;
    QQuickAbstractApplicationWindow *q_ptr;
};

void QQuickAbstractApplicationWindowPrivate::relayout()
{
    Q_Q(QQuickAbstractApplicationWindow);
    QQuickItem *content = q->contentItem();
    qreal hh = header ? header->height() : 0;
    qreal fh = footer ? footer->height() : 0;

    content->setY(hh);
    content->setHeight(q->height() - hh - fh);

    if (header)
        header->setY(-hh);
    if (footer)
        footer->setY(content->height());
}

void QQuickAbstractApplicationWindowPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

void QQuickAbstractApplicationWindowPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    Q_UNUSED(item);
    relayout();
}

QQuickAbstractApplicationWindow::QQuickAbstractApplicationWindow(QWindow *parent) :
    QQuickWindowQmlImpl(parent), d_ptr(new QQuickAbstractApplicationWindowPrivate)
{
    d_ptr->q_ptr = this;
}

QQuickAbstractApplicationWindow::~QQuickAbstractApplicationWindow()
{
    Q_D(QQuickAbstractApplicationWindow);
    if (d->header)
        QQuickItemPrivate::get(d->header)->removeItemChangeListener(d, QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
    if (d->footer)
        QQuickItemPrivate::get(d->footer)->removeItemChangeListener(d, QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
}

/*!
    \qmlproperty Item QtQuickControls2::ApplicationWindow::header

    TODO
*/
QQuickItem *QQuickAbstractApplicationWindow::header() const
{
    Q_D(const QQuickAbstractApplicationWindow);
    return d->header;
}

void QQuickAbstractApplicationWindow::setHeader(QQuickItem *header)
{
    Q_D(QQuickAbstractApplicationWindow);
    if (d->header != header) {
        delete d->header;
        d->header = header;
        if (header) {
            header->setParentItem(contentItem());
            QQuickItemPrivate *p = QQuickItemPrivate::get(header);
            p->addItemChangeListener(d, QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
            if (qFuzzyIsNull(header->z()))
                header->setZ(1);
        }
        emit headerChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::ApplicationWindow::footer

    TODO
*/
QQuickItem *QQuickAbstractApplicationWindow::footer() const
{
    Q_D(const QQuickAbstractApplicationWindow);
    return d->footer;
}

void QQuickAbstractApplicationWindow::setFooter(QQuickItem *footer)
{
    Q_D(QQuickAbstractApplicationWindow);
    if (d->footer != footer) {
        delete d->footer;
        d->footer = footer;
        if (footer) {
            footer->setParentItem(contentItem());
            QQuickItemPrivate *p = QQuickItemPrivate::get(footer);
            p->addItemChangeListener(d, QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight);
            if (qFuzzyIsNull(footer->z()))
                footer->setZ(1);
        }
        emit footerChanged();
    }
}

/*!
    \qmlproperty real QtQuickControls2::ApplicationWindow::contentWidth

    TODO
*/
qreal QQuickAbstractApplicationWindow::contentWidth() const
{
    Q_D(const QQuickAbstractApplicationWindow);
    return d->contentWidth;
}

void QQuickAbstractApplicationWindow::setContentWidth(qreal width)
{
    Q_D(QQuickAbstractApplicationWindow);
    if (d->contentWidth != width) {
        d->contentWidth = width;
        emit contentWidthChanged();
    }
}

/*!
    \qmlproperty real QtQuickControls2::ApplicationWindow::contentHeight

    TODO
*/
qreal QQuickAbstractApplicationWindow::contentHeight() const
{
    Q_D(const QQuickAbstractApplicationWindow);
    return d->contentHeight;
}

void QQuickAbstractApplicationWindow::setContentHeight(qreal height)
{
    Q_D(QQuickAbstractApplicationWindow);
    if (d->contentHeight != height) {
        d->contentHeight = height;
        emit contentHeightChanged();
    }
}

void QQuickAbstractApplicationWindow::resizeEvent(QResizeEvent *event)
{
    QQuickWindowQmlImpl::resizeEvent(event);

    Q_D(QQuickAbstractApplicationWindow);
    if (d->header) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->header);
        if (!p->widthValid || qFuzzyCompare(d->header->width(), event->oldSize().width()))
            d->header->setWidth(width());
    }
    if (d->footer) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->footer);
        if (!p->widthValid || qFuzzyCompare(d->footer->width(), event->oldSize().width()))
            d->footer->setWidth(width());
    }
    d->relayout();
}

QT_END_NAMESPACE
