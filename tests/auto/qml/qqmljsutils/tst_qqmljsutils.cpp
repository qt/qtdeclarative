// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qqmljsutils.h"
#include <QtQmlCompiler/private/qqmljsutils_p.h>

using namespace Qt::StringLiterals;

void tst_qqmljsutils::findResourceFilesFromBuildFolders()
{
    const QString buildFolder = testFile("buildfolder");
    const QStringList resourceFiles = QQmlJSUtils::resourceFilesFromBuildFolders({ buildFolder});
    QCOMPARE(resourceFiles,
             QStringList{ testFile(u"buildfolder/.qt/rcc/appuntitled72_raw_qml_0.qrc"_s) });
};

QTEST_MAIN(tst_qqmljsutils)
