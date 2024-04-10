// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmltoolingutils_p.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>

using namespace Qt::StringLiterals;

/*!
\internal

Helper utils to help QQmlTooling retrieve certain values from the environment or command line
options.
It helps to keep the warning messages consistent between tools like qmlls and qmllint when
they use environment variables, for examples.
*/

void QQmlToolingUtils::warnForInvalidDirs(const QStringList &dirs, const QString &origin)
{
    for (const QString &path : dirs) {
        QFileInfo info(path);
        if (!info.exists()) {
            qWarning().noquote().nospace()
                    << u"Argument \"%1\" %2 does not exist."_s.arg(path, origin);
            continue;
        }
        if (!info.isDir()) {
            qWarning().noquote().nospace()
                    << "Argument \"" << path << "\" " << origin << " is not a directory.";
            continue;
        }
    }
}

QStringList
QQmlToolingUtils::getAndWarnForInvalidDirsFromEnv(const QString &environmentVariableName)
{
    const QStringList envPaths = qEnvironmentVariable(environmentVariableName.toUtf8())
                                         .split(QDir::listSeparator(), Qt::SkipEmptyParts);
    warnForInvalidDirs(envPaths,
                       u"from environment variable \"%1\""_s.arg(environmentVariableName));
    return envPaths;
}

QStringList QQmlToolingUtils::getAndWarnForInvalidDirsFromOption(const QCommandLineParser &parser,
                                                                 const QCommandLineOption &option)
{
    if (!parser.isSet(option))
        return {};

    const QStringList dirs = parser.values(option);
    const QString optionName = option.names().constFirst();
    warnForInvalidDirs(dirs, u"passed to -%1"_s.arg(optionName));
    return dirs;
}
