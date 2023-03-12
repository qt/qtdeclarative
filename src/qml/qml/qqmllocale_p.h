// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLLOCALE_H
#define QQMLLOCALE_H

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

#include <qqml.h>

#include <QtCore/qlocale.h>
#include <QtCore/qobject.h>
#include <private/qtqmlglobal_p.h>
#include <private/qv4object_p.h>

QT_REQUIRE_CONFIG(qml_locale);

QT_BEGIN_NAMESPACE


class QQmlDateExtension
{
public:
    static void registerExtension(QV4::ExecutionEngine *engine);

private:
    static QV4::ReturnedValue method_toLocaleString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_toLocaleTimeString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_toLocaleDateString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_fromLocaleString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_fromLocaleTimeString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_fromLocaleDateString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_timeZoneUpdated(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
};


class QQmlNumberExtension
{
public:
    static void registerExtension(QV4::ExecutionEngine *engine);

private:
    static QV4::ReturnedValue method_toLocaleString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_fromLocaleString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_toLocaleCurrencyString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
};


namespace QQmlLocale
{
    Q_NAMESPACE_EXPORT(Q_QML_PRIVATE_EXPORT)
    QML_NAMED_ELEMENT(Locale)
    QML_ADDED_IN_VERSION(2, 2)
    QML_NAMESPACE_EXTENDED(QLocale)

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
    Q_ENUM_NS(DayOfWeek)

    Q_QML_PRIVATE_EXPORT QV4::ReturnedValue locale(QV4::ExecutionEngine *engine, const QString &localeName);
    Q_QML_PRIVATE_EXPORT QV4::ReturnedValue wrap(QV4::ExecutionEngine *engine, const QLocale &locale);
    Q_QML_PRIVATE_EXPORT void registerStringLocaleCompare(QV4::ExecutionEngine *engine);
    Q_QML_PRIVATE_EXPORT QV4::ReturnedValue method_localeCompare(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
};

namespace QV4 {

namespace Heap {

struct QQmlLocaleData : Object {
    inline void init() { locale = new QLocale; }
    void destroy() {
        delete locale;
        Object::destroy();
    }
    QLocale *locale;
};

}

struct QQmlLocaleData : public QV4::Object
{
    V4_OBJECT2(QQmlLocaleData, Object)
    V4_NEEDS_DESTROY

    static QLocale *getThisLocale(QV4::Scope &scope, const QV4::Value *thisObject) {
        const QV4::Object *o = thisObject->as<Object>();
        const QQmlLocaleData *data = o ? o->as<QQmlLocaleData>() : nullptr;
        if (!data) {
            scope.engine->throwTypeError();
            return nullptr;
        }
        return data->d()->locale;
    }

    static QV4::ReturnedValue method_currencySymbol(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_dateTimeFormat(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_timeFormat(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_dateFormat(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_monthName(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_standaloneMonthName(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_dayName(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_standaloneDayName(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_toString(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);

    static QV4::ReturnedValue method_get_firstDayOfWeek(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_measurementSystem(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_textDirection(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_weekDays(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_uiLanguages(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);

    static QV4::ReturnedValue method_get_name(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_nativeLanguageName(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
#if QT_DEPRECATED_SINCE(6, 6)
    static QV4::ReturnedValue method_get_nativeCountryName(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
#endif
    static QV4::ReturnedValue method_get_nativeTerritoryName(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_decimalPoint(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_groupSeparator(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_percent(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_zeroDigit(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_negativeSign(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_positiveSign(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_exponential(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_amText(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_get_pmText(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);

    static QV4::ReturnedValue method_get_numberOptions(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    static QV4::ReturnedValue method_set_numberOptions(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);

    static QV4::ReturnedValue method_get_formattedDataSize(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif
