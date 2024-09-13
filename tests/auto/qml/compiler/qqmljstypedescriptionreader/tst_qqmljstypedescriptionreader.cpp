// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qqmljstypedescriptionreader.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

void tst_qqmljstypedescriptionreader::warningFreeQmltypes_data()
{
    QTest::addColumn<QString>("path");

    QTest::addRow("qmltypesFromQtQuick65") << u"qmltypesFromQtQuick65.qmltypes"_s;
}

void tst_qqmljstypedescriptionreader::warningFreeQmltypes()
{
    QTest::failOnWarning();

    QFETCH(QString, path);

    QFile file(testFile(path));
    QVERIFY(file.open(QFile::ReadOnly));

    QQmlJSTypeDescriptionReader reader(path, file.readAll());
    QList<QQmlJSExportedScope> objects;
    QStringList dependencies;
    QVERIFY(reader(&objects, &dependencies));
    QCOMPARE(reader.warningMessage(), QString());
    QCOMPARE(reader.errorMessage(), QString());
}

QT_END_NAMESPACE

QTEST_MAIN(tst_qqmljstypedescriptionreader)
