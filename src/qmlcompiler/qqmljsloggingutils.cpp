// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsloggingutils_p.h"

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtCore/qcommandlineparser.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \class QQmlSA::LoggerWarningId
    \inmodule QtQmlCompiler

    \brief A wrapper around a string literal to uniquely identify
    warning categories in the \c{QQmlSA} framework.
*/

/*!
    \fn QQmlSA::LoggerWarningId::LoggerWarningId(QAnyStringView name)
    Constructs a LoggerWarningId object with logging catergory name \a name.
*/

/*!
    \fn QAnyStringView QQmlSA::LoggerWarningId::name() const
    Returns the name of the wrapped warning category.
*/

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

namespace LoggingUtils {

QString levelToString(const QQmlJS::LoggerCategory &category)
{
    Q_ASSERT(category.isIgnored() || category.level() != QtCriticalMsg);
    if (category.isIgnored())
        return QStringLiteral("disable");

    switch (category.level()) {
    case QtInfoMsg:
        return QStringLiteral("info");
    case QtWarningMsg:
        return QStringLiteral("warning");
    default:
        Q_UNREACHABLE();
        break;
    }
};

/*!
\internal
Sets the category levels from a settings file and an optional parser.
Calls \c {parser->showHelp(-1)} for invalid logging levels.
*/
void updateLogLevels(QList<LoggerCategory> &categories,
                     const QQmlToolingSettings &settings,
                     QCommandLineParser *parser)
{
    bool success = true;
    for (auto &category : categories) {
        if (category.isDefault())
            continue;

        const QString value = [&] () {
            const QString key = category.id().name().toString();
            if (parser && parser->isSet(key))
                return parser->value(key);

            // Do not try to set the levels if it's due to a default config option.
            // This way we can tell which options have actually been overwritten by the user.
            const QString settingsName = QStringLiteral("Warnings/") + category.settingsName();
            const QString value = settings.value(settingsName).toString();
            if (levelToString(category) == value)
                return QString();

            return value;
        }();
        if (value.isEmpty())
            continue;

        if (value == "disable"_L1) {
            category.setLevel(QtCriticalMsg);
            category.setIgnored(true);
        } else if (value == "info"_L1) {
            category.setLevel(QtInfoMsg);
            category.setIgnored(false);
        } else if (value == "warning"_L1) {
            category.setLevel(QtWarningMsg);
            category.setIgnored(false);
        } else {
            qWarning() << "Invalid logging level" << value << "provided for"
                       << category.id().name().toString()
                       << "(allowed are: disable, info, warning)";
            success = false;

        }
    }
    if (!success && parser)
        parser->showHelp(-1);
}
} // namespace LoggingUtils

} // namespace QQmlJS

QT_END_NAMESPACE
