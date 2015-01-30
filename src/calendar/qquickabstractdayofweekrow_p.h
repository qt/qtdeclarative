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

#ifndef QQUICKABSTRACTDAYOFWEEKROW_P_H
#define QQUICKABSTRACTDAYOFWEEKROW_P_H

#include <QtQuickCalendar/private/qtquickcalendarglobal_p.h>
#include <QtQuickControls/private/qquickabstractcontainer_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractDayOfWeekRowPrivate;

class Q_QUICKCALENDAR_EXPORT QQuickAbstractDayOfWeekRow : public QQuickAbstractContainer
{
    Q_OBJECT
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(QVariant source READ source WRITE setSource NOTIFY sourceChanged FINAL)

public:
    explicit QQuickAbstractDayOfWeekRow(QQuickItem *parent = Q_NULLPTR);

    QLocale locale() const;
    void setLocale(const QLocale &locale);

    QVariant source() const;
    void setSource(const QVariant &source);

Q_SIGNALS:
    void localeChanged();
    void sourceChanged();

private:
    Q_DISABLE_COPY(QQuickAbstractDayOfWeekRow)
    Q_DECLARE_PRIVATE(QQuickAbstractDayOfWeekRow)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTDAYOFWEEKROW_P_H
