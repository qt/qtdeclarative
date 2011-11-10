/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVELOCALE_H
#define QDECLARATIVELOCALE_H

#include <qdeclarative.h>

#include <QtCore/qlocale.h>
#include <QtCore/qobject.h>
#include <private/qv8engine_p.h>


QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeDateExtension
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
};


class QDeclarativeNumberExtension
{
public:
    static void registerExtension(QV8Engine *engine);

private:
    static v8::Handle<v8::Value> toLocaleString(const v8::Arguments& args);
    static v8::Handle<v8::Value> fromLocaleString(const v8::Arguments& args);
    static v8::Handle<v8::Value> toLocaleCurrencyString(const v8::Arguments&  args);
};


class Q_AUTOTEST_EXPORT QDeclarativeLocale
{
    Q_GADGET
    Q_ENUMS(MeasurementSystem)
    Q_ENUMS(FormatType)
    Q_ENUMS(CurrencySymbolFormat)
    Q_ENUMS(DayOfWeek)

public:
    ~QDeclarativeLocale();

    enum MeasurementSystem {
        MetricSystem = QLocale::MetricSystem,
        ImperialSystem = QLocale::ImperialSystem
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

private:
    QDeclarativeLocale();
};


QT_END_NAMESPACE

QT_END_HEADER

#endif
