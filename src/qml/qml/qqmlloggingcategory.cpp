// Copyright (C) 2016 Pelagicore AG
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlloggingcategory_p.h"

#include <QtQml/qqmlinfo.h>

#include <memory>

/*!
    \qmltype LoggingCategory
    \ingroup qml-utility-elements
    \inqmlmodule QtQml
    \brief Defines a logging category in QML.
    \since 5.8

    A logging category can be passed to console.log() and friends as the first argument.
    If supplied to the logger the LoggingCategory's name will be used as logging category.
    Otherwise the default logging category will be used.

    \qml
    import QtQuick

    Item {
        LoggingCategory {
            id: category
            name: "com.qt.category"
            defaultLogLevel: LoggingCategory.Warning
        }

        Component.onCompleted: {
            console.log(category, "log message");
            console.warn(category, "warning message");
        }
    }
    \endqml

    By default this outputs only \c{com.qt.category: warning message}. The
    \c{log message} is suppressed due to the \l{defaultLogLevel}. You can,
    however, configure log levels for QML logging categories the same way
    you can configure them for
    \l{QLoggingCategory#configuring-categories}{QLoggingCategory}.

    \note As the creation of objects is expensive, it is encouraged to put the needed
    LoggingCategory definitions into a singleton and import this where needed.

    \sa QLoggingCategory
*/

/*!
    \qmlproperty string QtQml::LoggingCategory::name

    Holds the name of the logging category.

    \note This property needs to be set when declaring the LoggingCategory
    and cannot be changed later.

    \sa QLoggingCategory::categoryName()
*/

/*!
    \qmlproperty enumeration QtQml::LoggingCategory::defaultLogLevel
    \since 5.12

    Holds the default log level of the logging category. By default it is
    created with the LoggingCategory.Debug log level.

    The following enumeration values are available:
    \list
    \li LoggingCategory.Debug
    \li LoggingCategory.Info
    \li LoggingCategory.Warning
    \li LoggingCategory.Critical
    \li LoggingCategory.Fatal
    \endlist

    They mirror the values of the \l{QtMsgType} enumeration.

    \note This property needs to be set when declaring the LoggingCategory
    and cannot be changed later.

    \sa QtMsgType
*/

QQmlLoggingCategory::QQmlLoggingCategory(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
{
}

QQmlLoggingCategory::~QQmlLoggingCategory()
{
}

QString QQmlLoggingCategory::name() const
{
    return QString::fromUtf8(m_name);
}

QQmlLoggingCategory::DefaultLogLevel QQmlLoggingCategory::defaultLogLevel() const
{
    return m_defaultLogLevel;
}

QLoggingCategory *QQmlLoggingCategory::category() const
{
    return m_category.get();
}

void QQmlLoggingCategory::classBegin()
{
}

void QQmlLoggingCategory::componentComplete()
{
    m_initialized = true;
    if (m_name.isNull()) {
        qmlWarning(this) << QLatin1String("Declaring the name of a LoggingCategory is mandatory and cannot be changed later");
    } else {
        auto category = std::make_unique<QLoggingCategory>(m_name.constData(), QtMsgType(m_defaultLogLevel));
        m_category.swap(category);
    }
}

void QQmlLoggingCategory::setDefaultLogLevel(DefaultLogLevel defaultLogLevel)
{
    if (m_defaultLogLevel == defaultLogLevel)
        return;

    if (m_initialized) {
        qmlWarning(this) << QLatin1String("The defaultLogLevel of a LoggingCategory cannot be changed after the component is completed");
        return;
    }

    m_defaultLogLevel = defaultLogLevel;
}

void QQmlLoggingCategory::setName(const QString &name)
{
    const QByteArray newName = name.toUtf8();

    if (m_name == newName)
        return;

    if (m_initialized) {
        qmlWarning(this) << QLatin1String("The name of a LoggingCategory cannot be changed after the component is completed");
        return;
    }

    m_name = newName;
}

#include "moc_qqmlloggingcategory_p.cpp"
