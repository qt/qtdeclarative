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

#ifndef QQUICKABSTRACTCONTAINER_P_H
#define QQUICKABSTRACTCONTAINER_P_H

#include <QtQuickControls/private/qquickcontrol_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractContainerPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickAbstractContainer : public QQuickControl
{
    Q_OBJECT
    Q_PROPERTY(qreal contentWidth READ contentWidth WRITE setContentWidth NOTIFY contentWidthChanged FINAL)
    Q_PROPERTY(qreal contentHeight READ contentHeight WRITE setContentHeight NOTIFY contentHeightChanged FINAL)
    Q_PROPERTY(QQuickItem *contentItem READ contentItem WRITE setContentItem NOTIFY contentItemChanged FINAL)

public:
    explicit QQuickAbstractContainer(QQuickItem *parent = Q_NULLPTR);

    qreal contentWidth() const;
    void setContentWidth(qreal width);

    qreal contentHeight() const;
    void setContentHeight(qreal height);

    QQuickItem *contentItem() const;
    void setContentItem(QQuickItem *item);

Q_SIGNALS:
    void contentWidthChanged();
    void contentHeightChanged();
    void contentItemChanged();

protected:
    QQuickAbstractContainer(QQuickAbstractContainerPrivate &dd, QQuickItem *parent);

private:
    Q_DISABLE_COPY(QQuickAbstractContainer)
    Q_DECLARE_PRIVATE(QQuickAbstractContainer)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTCONTAINER_P_H
