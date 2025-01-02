// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDTESTUTILS_P_H
#define ANDROIDTESTUTILS_P_H

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

#include <QtCore/qfile.h>
#include <QtCore/qstring.h>
#include <QtCore/qstandardpaths.h>

QT_BEGIN_NAMESPACE

static QFile androidExitCodeFile()
{
    const QString testHome = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return QFile(testHome + "/qtest_last_exit_code");
}

static void writeAndroidExitCode(int code)
{
    QFile exitCodeFile = androidExitCodeFile();
    if (exitCodeFile.open(QIODevice::WriteOnly)) {
        exitCodeFile.write(qPrintable(QString::number(code)));
    } else {
        qWarning("Failed to open %s for writing test exit code: %s",
                 qPrintable(exitCodeFile.fileName()), qPrintable(exitCodeFile.errorString()));
    }
}

static void clearAndroidExitCode()
{
    androidExitCodeFile().remove();
}

QT_END_NAMESPACE

#endif
