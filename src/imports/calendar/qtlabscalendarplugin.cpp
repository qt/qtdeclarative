/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Calendar module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQml/qqmlextensionplugin.h>

#include "qquickdayofweekrow_p.h"
#include "qquickmonthgrid_p.h"
#include "qquickweeknumbercolumn_p.h"
#include "qquickcalendarmodel_p.h"
#include "qquickcalendar_p.h"

QT_BEGIN_NAMESPACE

class QtLabsCalendarPlugin: public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    QtLabsCalendarPlugin(QObject *parent = nullptr);
    void registerTypes(const char *uri) override;
};

QtLabsCalendarPlugin::QtLabsCalendarPlugin(QObject *parent) : QQmlExtensionPlugin(parent)
{
}

static QObject *calendarSingleton(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine);
    Q_UNUSED(scriptEngine);
    return new QQuickCalendar;
}

void QtLabsCalendarPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<QQuickDayOfWeekRow>(uri, 1, 0, "AbstractDayOfWeekRow");
    qmlRegisterType<QQuickMonthGrid>(uri, 1, 0, "AbstractMonthGrid");
    qmlRegisterType<QQuickWeekNumberColumn>(uri, 1, 0, "AbstractWeekNumberColumn");
    qmlRegisterType<QQuickCalendarModel>(uri, 1, 0, "CalendarModel");
    qmlRegisterSingletonType<QQuickCalendar>(uri, 1, 0, "Calendar", calendarSingleton);
}

QT_END_NAMESPACE

#include "qtlabscalendarplugin.moc"
