// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljsloggingutils_p.h"

#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtCore/qcommandlineparser.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

/*!
    \enum QQmlSA::WarningSeverity
    \inmodule QtQmlCompiler
    \since 6.12

    \brief Holds the possible severity levels of diagnostics messages for a logging category.
    \value Error    Messages emitted by this logging category will cause an error.
    \value Warning  Messages emitted by this logging category are warnings. No error is produced unless the \l{qmllint#Settings}{MaxWarnings} setting is set and exceeded.
    \value Info     Messages emitted by this logging category are purely informative and do not count towards \l{qmllint#Settings}{MaxWarnings}.
    \value Disable  This logging category will not emit any messages.
*/

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

namespace QQmlSA {

static_assert(int(WarningSeverity::Disable) != int(QtMsgType::QtInfoMsg));
static_assert(int(WarningSeverity::Disable) != int(QtMsgType::QtWarningMsg));
static_assert(int(WarningSeverity::Disable) != int(QtMsgType::QtCriticalMsg));

} // namespace QQmlSA

namespace QQmlJS {

LoggerCategory::LoggerCategory() : d_ptr{ new LoggerCategoryPrivate } { }

LoggerCategory::LoggerCategory(
        const QString &name, const QString &settingsName, const QString &description,
        WarningSeverity severity, Essentiality essentiality)
    : d_ptr{ new LoggerCategoryPrivate(name, settingsName, description, severity, essentiality) }
{
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
    return d->name();
}

QString LoggerCategory::settingsName() const
{
    Q_D(const LoggerCategory);
    return d->settingsName();
}

QString LoggerCategory::description() const
{
    Q_D(const LoggerCategory);
    return d->description();
}

WarningSeverity LoggerCategory::severity() const
{
    Q_D(const LoggerCategory);
    return d->severity();
}

bool LoggerCategory::isEssential() const
{
    Q_D(const LoggerCategory);
    return d->isEssential();
}

LoggerWarningId LoggerCategory::id() const
{
    Q_D(const LoggerCategory);
    return d->id();
}

void LoggerCategory::setSeverity(WarningSeverity severity)
{
    Q_D(LoggerCategory);
    d->setSeverity(severity);
}

LoggerCategoryPrivate::LoggerCategoryPrivate(const QString &name, const QString &settingsName,
                                             const QString &description, WarningSeverity severity,
                                             LoggerCategory::Essentiality essentiality)
    : m_name(name), m_settingsName(settingsName), m_description(description), m_severity(severity)
    , m_isEssential(essentiality == LoggerCategory::Essential)
{
}

void LoggerCategoryPrivate::setSeverity(WarningSeverity severity)
{
    if (m_severity == severity)
        return;

    m_severity = severity;
    m_changed = true;
}

LoggerCategoryPrivate *LoggerCategoryPrivate::get(LoggerCategory *loggerCategory)
{
    Q_ASSERT(loggerCategory);
    return loggerCategory->d_func();
}

namespace LoggingUtils {

QString severityToString(QQmlJS::WarningSeverity severity)
{
    switch (severity) {
    case QQmlJS::WarningSeverity::Disable:
        return QStringLiteral("disable");
    case QQmlJS::WarningSeverity::Info:
        return QStringLiteral("info");
    case QQmlJS::WarningSeverity::Warning:
        return QStringLiteral("warning");
    case QQmlJS::WarningSeverity::Error:
        return QStringLiteral("error");
    default:
        Q_UNREACHABLE();
        break;
    }
};

static QStringList settingsNamesForCategory(const LoggerCategory &category)
{
    const QString name = category.settingsName();
    QStringList result{ QStringLiteral("Warnings/") + name };
    const QString sliced = "Warnings/"_L1 + name.sliced(name.indexOf(u'.') + 1);
    if (sliced != result.last())
        result.append(sliced);
    return result;
}

static QString lookInSettings(const LoggerCategory &category, const QQmlToolingSettings &settings,
                              const QString &settingsName)
{
    if (settings.isSet(settingsName))
        return settings.value(settingsName).toString();
    static constexpr QLatin1String propertyAliasCyclesKey = "Warnings/PropertyAliasCycles"_L1;

    // keep compatibility to deprecated settings
    if (category.name() == qmlAliasCycle.name() || category.name() == qmlUnresolvedAlias.name()) {
        if (settings.isSet(propertyAliasCyclesKey)) {
            qWarning()
                    << "Detected deprecated setting name \"PropertyAliasCycles\". Use %1 or %2 instead."_L1
                               .arg(qmlAliasCycle.name(), qmlUnresolvedAlias.name());
            return settings.value(propertyAliasCyclesKey).toString();
        }
    }
    return {};
}

static QString severityValueForCategory(const LoggerCategory &category,
                                     const QQmlToolingSettings &settings,
                                     QCommandLineParser *parser)
{
    const QString key = category.id().name().toString();
    if (parser && parser->isSet(key))
        return parser->value(key);

    const QStringList settingsName = settingsNamesForCategory(category);
    for (const QString &settingsName : settingsName) {
        const QString value = lookInSettings(category, settings, settingsName);
        if (value.isEmpty())
            continue;

        // Do not try to set the severity if it's due to a default config option.
        // This way we can tell which options have actually been overwritten by the user.
        if (severityToString(category.severity()) == value)
            return QString();

        return value;
    }
    return QString();
}

std::optional<QQmlJS::WarningSeverity> severityFromString(const QString &s)
{
    if (s == "disable"_L1)
        return QQmlJS::WarningSeverity::Disable;
    if (s == "info"_L1)
        return QQmlJS::WarningSeverity::Info;
    if (s == "warning"_L1)
        return QQmlJS::WarningSeverity::Warning;
    if (s == "error"_L1)
        return QQmlJS::WarningSeverity::Error;
    return {};
}

static constexpr bool less(WarningSeverity lhs, WarningSeverity rhs)
{
    const auto orderedValue = [](WarningSeverity s) {
        switch (s) {
        case WarningSeverity::Error:
            return 3;
        case WarningSeverity::Warning:
            return 2;
        case WarningSeverity::Info:
            return 1;
        case WarningSeverity::Disable:
            return 0;
        }
        Q_UNREACHABLE();
    };
    return orderedValue(lhs) < orderedValue(rhs);
}

/*!
\internal
Sets the category severity from a settings file and an optional parser.
Calls \c {parser->showHelp(-1)} for an invalid logging severity.
*/
void updateLogSeverities(QList<LoggerCategory> &categories,
                         const QQmlToolingSettings &settings,
                         QCommandLineParser *parser,
                         CategorySelection categorySelection)
{
    bool success = true;
    for (auto &category : categories) {
        // Disable all non-essential categories, they may still be overriden by settings or command line options
        if (categorySelection == CategorySelection::Explicit && !category.isEssential())
            category.setSeverity(QQmlSA::WarningSeverity::Disable);

        const QString value = severityValueForCategory(category, settings, parser);
        if (value.isEmpty())
            continue;

        const QString &name = category.id().name().toString();
        const std::optional<QQmlJS::WarningSeverity> severity = severityFromString(value);
        if (!severity.has_value()) {
            qWarning() << "Invalid logging severity" << value << "provided for" << name
                       << "(allowed are: disable, info, warning, error).";
            success = false;
            continue;
        }

        if (category.isEssential() && less(severity.value(), category.severity())) {
            qWarning() << "In order to ensure the proper function of qmllint, the severity of the "
                          "essential category %1 cannot be lowered."_L1.arg(name);
            continue;
        }

        category.setSeverity(*severity);
    }

    if (!success && parser)
        parser->showHelp(-1);
}
} // namespace LoggingUtils

} // namespace QQmlJS

QT_END_NAMESPACE
