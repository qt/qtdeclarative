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

#include "qquickabstractlabel_p.h"

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickclipnode_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractLabelPrivate
{
    Q_DECLARE_PUBLIC(QQuickAbstractLabel)

public:
    QQuickAbstractLabelPrivate() : background(Q_NULLPTR) { }

    QQuickItem *background;
    QQuickAbstractLabel *q_ptr;
};

QQuickAbstractLabel::QQuickAbstractLabel(QQuickItem *parent) :
    QQuickText(parent), d_ptr(new QQuickAbstractLabelPrivate)
{
    Q_D(QQuickAbstractLabel);
    d->q_ptr = this;
}

QQuickAbstractLabel::~QQuickAbstractLabel()
{
}

QQuickItem *QQuickAbstractLabel::background() const
{
    Q_D(const QQuickAbstractLabel);
    return d->background;
}

void QQuickAbstractLabel::setBackground(QQuickItem *background)
{
    Q_D(QQuickAbstractLabel);
    if (d->background != background) {
        delete d->background;
        d->background = background;
        if (background) {
            background->setParentItem(this);
            if (qFuzzyIsNull(background->z()))
                background->setZ(-1);
        }
        emit backgroundChanged();
    }
}

void QQuickAbstractLabel::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickAbstractLabel);
    QQuickText::geometryChanged(newGeometry, oldGeometry);
    if (d->background) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(d->background);
        if (!p->widthValid) {
            d->background->setWidth(newGeometry.width());
            p->widthValid = false;
        }
        if (!p->heightValid) {
            d->background->setHeight(newGeometry.height());
            p->heightValid = false;
        }
    }
}

QT_END_NAMESPACE
