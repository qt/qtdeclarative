// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsloggingutils.h"
#include "qqmljsloggingutils_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {

LoggerCategory::LoggerCategory() : d_ptr{ new LoggerCategoryPrivate } { }

LoggerCategory::LoggerCategory(QString name, QString settingsName, QString description,
                               QtMsgType level, bool ignored, bool isDefault)
    : d_ptr{ new LoggerCategoryPrivate }
{
    Q_D(LoggerCategory);
    d->m_name = name;
    d->m_settingsName = settingsName;
    d->m_description = description;
    d->m_level = level;
    d->m_ignored = ignored;
    d->m_isDefault = isDefault;
}

LoggerCategory::LoggerCategory(const LoggerCategory &other)
    : d_ptr{ new LoggerCategoryPrivate{ *other.d_func() } }
{
}

LoggerCategory::LoggerCategory(LoggerCategory &&) noexcept = default;

LoggerCategory &LoggerCategory::operator=(const LoggerCategory &other)
{
    *d_func() = *other.d_func();
    return *this;
}

LoggerCategory &LoggerCategory::operator=(LoggerCategory &&) noexcept = default;

LoggerCategory::~LoggerCategory() = default;

QString LoggerCategory::name() const
{
    Q_D(const LoggerCategory);
    return d->m_name;
}

QString LoggerCategory::settingsName() const
{
    Q_D(const LoggerCategory);
    return d->m_settingsName;
}

QString LoggerCategory::description() const
{
    Q_D(const LoggerCategory);
    return d->m_description;
}

QtMsgType LoggerCategory::level() const
{
    Q_D(const LoggerCategory);
    return d->m_level;
}

bool LoggerCategory::isIgnored() const
{
    Q_D(const LoggerCategory);
    return d->m_ignored;
}

bool LoggerCategory::isDefault() const
{
    Q_D(const LoggerCategory);
    return d->m_isDefault;
}

LoggerWarningId LoggerCategory::id() const
{
    Q_D(const LoggerCategory);
    return d->id();
}

void LoggerCategory::setLevel(QtMsgType type)
{
    Q_D(LoggerCategory);
    d->setLevel(type);
}

void LoggerCategoryPrivate::setLevel(QtMsgType type)
{
    if (m_level == type)
        return;

    m_level = type;
    m_changed = true;
}

void LoggerCategory::setIgnored(bool isIgnored)
{
    Q_D(LoggerCategory);
    d->setIgnored(isIgnored);
}

void LoggerCategoryPrivate::setIgnored(bool isIgnored)
{
    if (m_ignored == isIgnored)
        return;

    m_ignored = isIgnored;
    m_changed = true;
}

bool LoggerCategoryPrivate::hasChanged() const
{
    return m_changed;
}

LoggerCategoryPrivate *LoggerCategoryPrivate::get(LoggerCategory *loggerCategory)
{
    Q_ASSERT(loggerCategory);
    return loggerCategory->d_func();
}

/*!
    \class QQmlSA::LoggerWarningId
    \inmodule QtQmlCompiler

    \brief A wrapper around a string literal to uniquely identify
    warning categories in the \c{QQmlSA} framework.
*/

} // namespace QQmlJS

QT_END_NAMESPACE
