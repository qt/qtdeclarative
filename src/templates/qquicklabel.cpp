/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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

#include "qquicklabel_p.h"
#include "qquicklabel_p_p.h"
#include "qquickcontrol_p.h"
#include "qquickcontrol_p_p.h"

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickclipnode_p.h>

#ifndef QT_NO_ACCESSIBILITY
#include <QtQuick/private/qquickaccessibleattached_p.h>
#endif

QT_BEGIN_NAMESPACE

QQuickLabel::QQuickLabel(QQuickItem *parent) :
    QQuickText(*(new QQuickLabelPrivate), parent)
{
    Q_D(const QQuickLabel);
    QObjectPrivate::connect(this, &QQuickText::textChanged,
                            d, &QQuickLabelPrivate::_q_textChanged);
}

QQuickLabel::~QQuickLabel()
{
}

/*!
    \internal

    Determine which font is implicitly imposed on this control by its ancestors
    and QGuiApplication::font, resolve this against its own font (attributes from
    the implicit font are copied over). Then propagate this font to this
    control's children.
*/
void QQuickLabelPrivate::resolveFont()
{
    Q_Q(QQuickLabel);
    QFont naturalFont = QQuickControlPrivate::naturalControlFont(q);
    QFont resolvedFont = sourceFont.resolve(naturalFont);
    if (sourceFont.resolve() == resolvedFont.resolve() && sourceFont == resolvedFont)
        return;

    q->QQuickText::setFont(resolvedFont);

    emit q->fontChanged();
}

void QQuickLabelPrivate::_q_textChanged(const QString &text)
{
#ifndef QT_NO_ACCESSIBILITY
    if (accessibleAttached)
        accessibleAttached->setName(text);
#else
    Q_UNUSED(text)
#endif
}

QFont QQuickLabel::font() const
{
    return QQuickText::font();
}

void QQuickLabel::setFont(const QFont &font)
{
    Q_D(QQuickLabel);
    if (d->sourceFont == font)
        return;

    // Determine which font is inherited from this control's ancestors and
    // QGuiApplication::font, resolve this against \a font (attributes from the
    // inherited font are copied over). Then propagate this font to this
    // control's children.
    QFont naturalFont = QQuickControlPrivate::naturalControlFont(this);
    QFont resolvedFont = font.resolve(naturalFont);
    if (d->sourceFont.resolve() == resolvedFont.resolve() && d->sourceFont == resolvedFont)
        return;

    QQuickText::setFont(font);

    emit fontChanged();
}

void QQuickLabel::classBegin()
{
    QQuickText::classBegin();
#ifndef QT_NO_ACCESSIBILITY
    Q_D(QQuickLabel);
    d->accessibleAttached = qobject_cast<QQuickAccessibleAttached *>(qmlAttachedPropertiesObject<QQuickAccessibleAttached>(this, true));
    if (d->accessibleAttached)
        d->accessibleAttached->setRole((QAccessible::Role)0x00000029); // Accessible.StaticText
    else
        qWarning() << "QQuickLabel: QQuickAccessibleAttached object creation failed!";
#endif
}

QQuickItem *QQuickLabel::background() const
{
    Q_D(const QQuickLabel);
    return d->background;
}

void QQuickLabel::setBackground(QQuickItem *background)
{
    Q_D(QQuickLabel);
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

void QQuickLabel::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickLabel);
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
