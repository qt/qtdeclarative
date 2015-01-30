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

#include <QtQml/qqmlextensionplugin.h>

#include <QtQuickCalendar/private/qquickabstractcalendarview_p.h>
#include <QtQuickCalendar/private/qquickabstractdayofweekrow_p.h>
#include <QtQuickCalendar/private/qquickabstractweeknumbercolumn_p.h>
#include <QtQuickCalendar/private/qquickcalendarmodel_p.h>
#include <QtQuickCalendar/private/qquickdayofweekmodel_p.h>
#include <QtQuickCalendar/private/qquickmonthmodel_p.h>
#include <QtQuickCalendar/private/qquickweeknumbermodel_p.h>

QT_BEGIN_NAMESPACE

class QtQuickCalendar2Plugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface/1.0")

public:
    void registerTypes(const char *uri);
};

void QtQuickCalendar2Plugin::registerTypes(const char *uri)
{
    qmlRegisterType<QQuickAbstractCalendarView>(uri, 2, 0, "AbstractCalendarView");
    qmlRegisterType<QQuickAbstractDayOfWeekRow>(uri, 2, 0, "AbstractDayOfWeekRow");
    qmlRegisterType<QQuickAbstractWeekNumberColumn>(uri, 2, 0, "AbstractWeekNumberColumn");
    qmlRegisterType<QQuickCalendarModel>(uri, 2, 0, "CalendarModel");
    qmlRegisterType<QQuickDayOfWeekModel>(uri, 2, 0, "DayOfWeekModel");
    qmlRegisterType<QQuickMonthModel>(uri, 2, 0, "MonthModel");
    qmlRegisterType<QQuickWeekNumberModel>(uri, 2, 0, "WeekNumberModel");
}

QT_END_NAMESPACE

#include "qtquickcalendar2plugin.moc"
