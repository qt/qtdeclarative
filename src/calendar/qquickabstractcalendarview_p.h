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

#ifndef QQUICKABSTRACTCALENDARVIEW_P_H
#define QQUICKABSTRACTCALENDARVIEW_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQuickCalendar/private/qtquickcalendarglobal_p.h>
#include <QtQuickControls/private/qquickabstractcontainer_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractCalendarViewPrivate;

class Q_QUICKCALENDAR_EXPORT QQuickAbstractCalendarView : public QQuickAbstractContainer
{
    Q_OBJECT
    Q_PROPERTY(int month READ month WRITE setMonth NOTIFY monthChanged FINAL)
    Q_PROPERTY(int year READ year WRITE setYear NOTIFY yearChanged FINAL)
    Q_PROPERTY(QLocale locale READ locale WRITE setLocale NOTIFY localeChanged FINAL)
    Q_PROPERTY(QVariant source READ source WRITE setSource NOTIFY sourceChanged FINAL)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged FINAL)

public:
    explicit QQuickAbstractCalendarView(QQuickItem *parent = Q_NULLPTR);

    int month() const;
    void setMonth(int month);

    int year() const;
    void setYear(int year);

    QLocale locale() const;
    void setLocale(const QLocale &locale);

    QVariant source() const;
    void setSource(const QVariant &source);

    QString title() const;
    void setTitle(const QString &title);

Q_SIGNALS:
    void monthChanged();
    void yearChanged();
    void localeChanged();
    void sourceChanged();
    void titleChanged();

    void pressed(const QDate &date);
    void released(const QDate &date);
    void clicked(const QDate &date);
    void pressAndHold(const QDate &date);

protected:
    void componentComplete() Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseUngrabEvent() Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QQuickAbstractCalendarView)
    Q_DECLARE_PRIVATE(QQuickAbstractCalendarView)
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTCALENDARVIEW_P_H
