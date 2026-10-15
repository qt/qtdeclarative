// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljslinterrenamedcomponents_p.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {

using namespace Qt::StringLiterals;

static QString qmlNameFromFilePath(QStringView path)
{
    if (!path.endsWith(".qml"_L1))
        return {};

    QStringView result(path);
    result.chop(".qml"_L1.size());
    if (result.endsWith(".ui"_L1))
        result.chop(".ui"_L1.size());
    const qsizetype start = path.lastIndexOf("/"_L1);
    result.slice(start == -1 ? 0 : start + 1);
    return result.toString();
}

static void removeModuleImportQualifiers(QStringList *names)
{
    for (QString &name : *names) {
        if (qsizetype idx = name.lastIndexOf(u'.'); idx != -1)
            name.slice(idx + 1);
    }
}

void LinterRenamedComponents::handleRenamedType(const QQmlJSScope::ConstPtr &scope,
                                                const QStringView name,
                                                const QQmlJS::SourceLocation &location,
                                                QQmlJSLogger *logger) const
{
    if (scope.isNull())
        return;

    const auto factory = scope.factory();
    const QString oldName = qmlNameFromFilePath(factory ? factory->filePath() : scope->filePath());
    if (oldName != name)
        return;

    QStringList names = m_scopeToName->values(scope);
    removeModuleImportQualifiers(&names);
    auto begin = names.begin();
    auto end = std::remove(begin, names.end(), oldName);
    std::sort(begin, end);
    names.erase(std::unique(begin, end), names.end());

    if (names.size() == 0)
        return;

    logger->log(
            "\"%2\" is explicitly renamed to \"%1\" via a qmldir entry or QT_QML_SOURCE_TYPENAME CMake property, use \"%1\" instead."_L1
                    .arg(qJoin(names.begin(), names.end(), QString(), "\", \""_L1), name),
            qmlRenamedType, location);
}
} // namespace QQmlJS

QT_END_NAMESPACE
