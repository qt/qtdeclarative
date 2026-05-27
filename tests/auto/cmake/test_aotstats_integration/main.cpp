// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QObject>
#include <QtTest/qtest.h>

using namespace Qt::StringLiterals;

class tst_aotstats_integration : public QObject
{
    Q_OBJECT
private slots:
    void check_all_aotstats_output();
};

void tst_aotstats_integration::check_all_aotstats_output()
{
    const QString source = SOURCE_DIRECTORY;
    const QString build = BUILD_DIRECTORY;

    QDir buildDir(build);
    QVERIFY(buildDir.exists());
    QFile f(buildDir.filePath(".rcc/qmlcache/all_aotstats.txt"));
    QVERIFY(f.exists());
    QVERIFY(f.open(QFile::ReadOnly | QFile::Text));

    QString expected = uR"(############ AOT COMPILATION STATS ############
Module NoBindings(nobindings_module):
--File %1/no_bindings/NoBindings.qml
  No attempts at compiling a binding or function
Module Normal(normal_module):
--File %1/normal/Normal.qml
  4 of 5 (80%) bindings or functions compiled to Cpp successfully
    f: [Normal.qml:4:5]
      result: Success
      duration: 100us
    s: [Normal.qml:5:24]
      result: Success
      duration: 100us
    g: [Normal.qml:6:5]
      result: Error: Functions without type annotations won't be compiled
      duration: 100us
    関数: [Normal.qml:8:5]
      result: Success
      duration: 100us
    財産: [Normal.qml:9:22]
      result: Success
      duration: 100us

############ AOT COMPILATION STATS SUMMARY ############
Module NoBindings(nobindings_module): No attempted compilations
Module Normal(normal_module): 4 of 5 (80%) bindings or functions compiled to Cpp successfully
Module Empty(empty_module): No .qml files to compile.
Module OnlyBytecode(onlybytecode_module): No .qml files compiled (--only-bytecode).
Total results: 4 of 5 (80%) bindings or functions compiled to Cpp successfully
Successful codegens took an average of 100us
)"_s.arg(source);

    QString actual = f.readAll();
    actual.replace(QRegularExpression(uR"(duration: \d+us)"_s), u"duration: 100us"_s);
    actual.replace(QRegularExpression(uR"(average of \d+us)"_s), u"average of 100us"_s);

    QCOMPARE(actual, expected);
}

QTEST_MAIN(tst_aotstats_integration)

#include "main.moc"
