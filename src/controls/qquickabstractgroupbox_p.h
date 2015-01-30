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

#ifndef QQUICKABSTRACTGROUPBOX_P_H
#define QQUICKABSTRACTGROUPBOX_P_H

#include <QtQuickControls/private/qquickabstractcontainer_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractGroupBoxPrivate;

class Q_QUICKCONTROLS_EXPORT QQuickAbstractGroupBox : public QQuickAbstractContainer
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)
    Q_PROPERTY(QQuickItem *label READ label WRITE setLabel NOTIFY labelChanged FINAL)
    Q_PROPERTY(QQuickItem *frame READ frame WRITE setFrame NOTIFY frameChanged FINAL)

public:
    explicit QQuickAbstractGroupBox(QQuickItem *parent = Q_NULLPTR);

    QString title() const;
    void setTitle(const QString &title);

    QQuickItem *label() const;
    void setLabel(QQuickItem *label);

    QQuickItem *frame() const;
    void setFrame(QQuickItem *frame);

Q_SIGNALS:
    void titleChanged();
    void labelChanged();
    void frameChanged();

private:
    Q_DISABLE_COPY(QQuickAbstractGroupBox)
    Q_DECLARE_PRIVATE(QQuickAbstractGroupBox)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTGROUPBOX_P_H
