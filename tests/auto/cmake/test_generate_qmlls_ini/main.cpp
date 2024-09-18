// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qlibraryinfo.h>
#include <QtQml/qqml.h>
#include <QtTest/qtest.h>

class tst_generate_qmlls_ini : public QObject
{
    Q_OBJECT
private slots:
    void qmllsIniAreCorrect();
};

using namespace Qt::StringLiterals;

#ifndef SOURCE_DIRECTORY
#  define SOURCE_DIRECTORY u"invalid_source_directory"_s
#endif
#ifndef BUILD_DIRECTORY
#  define BUILD_DIRECTORY u"invalid_build_directory"_s
#endif

void tst_generate_qmlls_ini::qmllsIniAreCorrect()
{
    const QString qmllsIniName = u".qmlls.ini"_s;
    QDir source(SOURCE_DIRECTORY);
    QDir build(BUILD_DIRECTORY);
    if (!source.exists())
        QSKIP(u"Cannot find source directory '%1', skipping test..."_s.arg(SOURCE_DIRECTORY)
                      .toLatin1());

    const QString qmllsIniTemplate = uR"([General]
buildDir=%1
no-cmake-calls=false
docDir=%3
importPaths=%4
)"_s;

    const QString &docPath = QLibraryInfo::path(QLibraryInfo::DocumentationPath);
    const QString emptyImportPath;
    {
        auto file = QFile(source.absoluteFilePath(qmllsIniName));
        QVERIFY(file.exists());
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        const auto fileContent = QString::fromUtf8(file.readAll());
        auto secondFolder = QDir(build.absolutePath().append(u"/qml/hello/subfolders"_s));
        QVERIFY(secondFolder.exists());
        QCOMPARE(fileContent,
                 qmllsIniTemplate.arg(build.absolutePath()
                                              .append(QDir::listSeparator())
                                              .append(secondFolder.absolutePath()),
                                      docPath, emptyImportPath));
    }

    {
    QDir sourceSubfolder = source;
    QVERIFY(sourceSubfolder.cd(u"SomeSubfolder"_s));
    QDir buildSubfolder(build.absolutePath().append(u"/SomeSubfolder/qml/Some/Sub/Folder"_s));
    {
        auto file = QFile(sourceSubfolder.absoluteFilePath(qmllsIniName));
        QVERIFY(file.exists());
        QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
        const auto fileContent = QString::fromUtf8(file.readAll());
        QCOMPARE(fileContent,
                 qmllsIniTemplate.arg(buildSubfolder.absolutePath(), docPath, emptyImportPath));
    }
    }

    {
        QDir dottedUriSubfolder = source;
        QVERIFY(dottedUriSubfolder.cd(u"Dotted"_s));
        QVERIFY(dottedUriSubfolder.cd(u"Uri"_s));
        {
            auto file = QFile(dottedUriSubfolder.absoluteFilePath(qmllsIniName));
            QVERIFY(file.exists());
            QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
            const auto fileContent = QString::fromUtf8(file.readAll());
            QCOMPARE(fileContent,
                     qmllsIniTemplate.arg(build.absolutePath(), docPath, emptyImportPath));
        }
    }
    {
        QDir dottedUriSubfolder = source;
        QVERIFY(dottedUriSubfolder.cd(u"Dotted"_s));
        QVERIFY(dottedUriSubfolder.cd(u"Uri"_s));
        QVERIFY(dottedUriSubfolder.cd(u"Hello"_s));
        QVERIFY(dottedUriSubfolder.cd(u"World"_s));
        {
            auto file = QFile(dottedUriSubfolder.absoluteFilePath(qmllsIniName));
            QVERIFY(file.exists());
            QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
            const auto fileContent = QString::fromUtf8(file.readAll());
            QCOMPARE(fileContent,
                     qmllsIniTemplate.arg(build.absolutePath(), docPath, emptyImportPath));
        }
    }
    {
        QDir dottedUriSubfolder = source;
        QVERIFY(dottedUriSubfolder.cd(u"ModuleWithDependency"_s));
        QVERIFY(dottedUriSubfolder.cd(u"MyModule"_s));
        {
            auto file = QFile(dottedUriSubfolder.absoluteFilePath(qmllsIniName));
            QVERIFY(file.exists());
            QVERIFY(file.open(QFile::ReadOnly | QFile::Text));
            const auto fileContent = QString::fromUtf8(file.readAll());
            QCOMPARE(fileContent,
                     qmllsIniTemplate.arg(build.absoluteFilePath(u"ModuleWithDependency"_s),
                                          docPath, build.absoluteFilePath(u"Dependency"_s)));
        }
    }
}

QTEST_MAIN(tst_generate_qmlls_ini)

#include "main.moc"
