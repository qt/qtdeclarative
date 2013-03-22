/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLLOCALE_H
#define QQMLLOCALE_H

#include <qqml.h>

#include <QtCore/qlocale.h>
#include <QtCore/qobject.h>
#include <private/qv8engine_p.h>


QT_BEGIN_NAMESPACE


class QQmlDateExtension
{
public:
    static void registerExtension(QV8Engine *engine);

private:
    static v8::Handle<v8::Value> toLocaleString(const v8::Arguments& args);
    static v8::Handle<v8::Value> toLocaleTimeString(const v8::Arguments& args);
    static v8::Handle<v8::Value> toLocaleDateString(const v8::Arguments& args);
    static v8::Handle<v8::Value> fromLocaleString(const v8::Arguments& args);
    static v8::Handle<v8::Value> fromLocaleTimeString(const v8::Arguments& args);
    static v8::Handle<v8::Value> fromLocaleDateString(const v8::Arguments& args);
    static v8::Handle<v8::Value> timeZoneUpdated(const v8::Arguments& args);
};


class QQmlNumberExtension
{
public:
    static void registerExtension(QV8Engine *engine);

private:
    static v8::Handle<v8::Value> toLocaleString(const v8::Arguments& args);
    static v8::Handle<v8::Value> fromLocaleString(const v8::Arguments& args);
    static v8::Handle<v8::Value> toLocaleCurrencyString(const v8::Arguments&  args);
};


class Q_AUTOTEST_EXPORT QQmlLocale
{
    Q_GADGET
    Q_ENUMS(MeasurementSystem)
    Q_ENUMS(FormatType)
    Q_ENUMS(CurrencySymbolFormat)
    Q_ENUMS(DayOfWeek)

public:
    ~QQmlLocale();

    enum MeasurementSystem {
        MetricSystem = QLocale::MetricSystem,
        ImperialSystem = QLocale::ImperialSystem,
        ImperialUSSystem = QLocale::ImperialUSSystem,
        ImperialUKSystem = QLocale::ImperialUKSystem
    };
    enum FormatType {
        LongFormat = QLocale::LongFormat,
        ShortFormat = QLocale::ShortFormat,
        NarrowFormat = QLocale::NarrowFormat
    };
    enum CurrencySymbolFormat {
        CurrencyIsoCode = QLocale::CurrencyIsoCode,
        CurrencySymbol = QLocale::CurrencySymbol,
        CurrencyDisplayName = QLocale::CurrencyDisplayName
    };
    // Qt defines Sunday as 7, but JS Date assigns Sunday 0
    enum DayOfWeek {
        Sunday = 0,
        Monday = Qt::Monday,
        Tuesday = Qt::Tuesday,
        Wednesday = Qt::Wednesday,
        Thursday = Qt::Thursday,
        Friday = Qt::Friday,
        Saturday = Qt::Saturday
    };

    static v8::Handle<v8::Value> locale(QV8Engine *v8engine, const QString &lang);

    static void registerStringLocaleCompare(QV8Engine *engine);

private:
    QQmlLocale();

    static v8::Handle<v8::Value> localeCompare(const v8::Arguments &args);
};


QT_END_NAMESPACE

#endif
