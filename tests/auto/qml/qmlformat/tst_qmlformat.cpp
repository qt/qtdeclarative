/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/private/qemulationdetector_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class TestQmlformat: public QQmlDataTest
{
    Q_OBJECT

public:
    TestQmlformat();

private Q_SLOTS:
    void initTestCase() override;

    void testFormat();
    void testFormat_data();

    void testLineEndings();
#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
    void testExample();
    void testExample_data();
#endif

private:
    QString readTestFile(const QString &path);
    QString runQmlformat(const QString &fileToFormat, QStringList args, bool shouldSucceed = true);

    QString m_qmlformatPath;
    QStringList m_excludedDirs;
    QStringList m_invalidFiles;

    QStringList findFiles(const QDir &);
    bool isInvalidFile(const QFileInfo &fileName) const;
};

TestQmlformat::TestQmlformat()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void TestQmlformat::initTestCase()
{
    QQmlDataTest::initTestCase();
    m_qmlformatPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmlformat");
#ifdef Q_OS_WIN
    m_qmlformatPath += QLatin1String(".exe");
#endif
    if (!QFileInfo(m_qmlformatPath).exists()) {
        QString message = QStringLiteral("qmlformat executable not found (looked for %0)").arg(m_qmlformatPath);
        QFAIL(qPrintable(message));
    }

    // Add directories you want excluded here

    // These snippets are not expected to run on their own.
    m_excludedDirs << "doc/src/snippets/qml/visualdatamodel_rootindex";
    m_excludedDirs << "doc/src/snippets/qml/qtbinding";
    m_excludedDirs << "doc/src/snippets/qml/imports";
    m_excludedDirs << "doc/src/snippets/qtquick1/visualdatamodel_rootindex";
    m_excludedDirs << "doc/src/snippets/qtquick1/qtbinding";
    m_excludedDirs << "doc/src/snippets/qtquick1/imports";
    m_excludedDirs << "tests/manual/v4";
    m_excludedDirs << "tests/auto/qml/ecmascripttests";
    m_excludedDirs << "tests/auto/qml/qmllint";

    // Add invalid files (i.e. files with syntax errors)
    m_invalidFiles << "tests/auto/quick/qquickloader/data/InvalidSourceComponent.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/signal.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/signal.3.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/signal.5.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/property.4.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/empty.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/missingObject.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/insertedSemicolon.1.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nonexistantProperty.5.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidRoot.1.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidQmlEnumValue.1.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidQmlEnumValue.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/questionDotEOF.qml";
    m_invalidFiles << "tests/auto/qml/qquickfolderlistmodel/data/dummy.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.1.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.3.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.4.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.5.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.6.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/numberParsing_error.1.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/numberParsing_error.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon_error1.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon_error1.qml";
    m_invalidFiles << "tests/auto/qml/debugger/qqmlpreview/data/broken.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/fuzzed.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/fuzzed.3.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/requiredProperties.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_LHS_And.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_LHS_And.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_LHS_Or.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_RHS_And.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_RHS_Or.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/typeAnnotations.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmlparser/data/disallowedtypeannotations/qmlnestedfunction.qml";

    // These files rely on exact formatting
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon1.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon_error1.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon2.qml";
}

QStringList TestQmlformat::findFiles(const QDir &d)
{
    for (int ii = 0; ii < m_excludedDirs.count(); ++ii) {
        QString s = m_excludedDirs.at(ii);
        if (d.absolutePath().endsWith(s))
            return QStringList();
    }

    QStringList rv;

    QStringList files = d.entryList(QStringList() << QLatin1String("*.qml"),
                                    QDir::Files);
    foreach (const QString &file, files) {
        rv << d.absoluteFilePath(file);
    }

    QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                   QDir::NoSymLinks);
    foreach (const QString &dir, dirs) {
        QDir sub = d;
        sub.cd(dir);
        rv << findFiles(sub);
    }

    return rv;
}

bool TestQmlformat::isInvalidFile(const QFileInfo &fileName) const
{
    for (const QString &invalidFile : m_invalidFiles) {
        if (fileName.absoluteFilePath().endsWith(invalidFile))
            return true;
    }
    return false;
}

QString TestQmlformat::readTestFile(const QString &path)
{
    QFile file(testFile(path));

    if (!file.open(QIODevice::ReadOnly))
        return "";

    return QString::fromUtf8(file.readAll());
}

void TestQmlformat::testLineEndings()
{
    // macos
    const QString macosContents =
            runQmlformat(testFile("Example1.formatted.qml"), { "-l", "macos" });
    QVERIFY(!macosContents.contains("\n"));
    QVERIFY(macosContents.contains("\r"));

    // windows
    const QString windowsContents =
            runQmlformat(testFile("Example1.formatted.qml"), { "-l", "windows" });
    QVERIFY(windowsContents.contains("\r\n"));

    // unix
    const QString unixContents = runQmlformat(testFile("Example1.formatted.qml"), { "-l", "unix" });
    QVERIFY(unixContents.contains("\n"));
    QVERIFY(!unixContents.contains("\r"));
}

void TestQmlformat::testFormat_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("fileFormatted");
    QTest::addColumn<QStringList>("args");

    QTest::newRow("example1")
            << "Example1.qml"
            << "Example1.formatted.qml" << QStringList {};
    QTest::newRow("example1 (tabs)") << "Example1.qml"
                                     << "Example1.formatted.tabs.qml" << QStringList { "-t" };
    QTest::newRow("example1 (two spaces)")
            << "Example1.qml"
            << "Example1.formatted.2spaces.qml" << QStringList { "-w", "2" };
    QTest::newRow("annotation")
            << "Annotations.qml"
            << "Annotations.formatted.qml" << QStringList {};
    QTest::newRow("front inline") << "FrontInline.qml"
                                  << "FrontInline.formatted.qml" << QStringList {};
    QTest::newRow("if blocks") << "IfBlocks.qml"
                               << "IfBlocks.formatted.qml" << QStringList {};
    QTest::newRow("read-only properties") << "readOnlyProps.qml"
                                          << "readOnlyProps.formatted.qml" << QStringList {};
    QTest::newRow("states and transitions")
            << "statesAndTransitions.qml"
            << "statesAndTransitions.formatted.qml" << QStringList {};
    QTest::newRow("large bindings") << "largeBindings.qml"
                                    << "largeBindings.formatted.qml" << QStringList {};
    QTest::newRow("verbatim strings") << "verbatimString.qml"
                                      << "verbatimString.formatted.qml" << QStringList {};
    QTest::newRow("inline components") << "inlineComponents.qml"
                                       << "inlineComponents.formatted.qml" << QStringList {};
    QTest::newRow("nested ifs") << "nestedIf.qml"
                                << "nestedIf.formatted.qml" << QStringList {};
    QTest::newRow("QTBUG-85003") << "QtBug85003.qml"
                                 << "QtBug85003.formatted.qml" << QStringList {};
    QTest::newRow("nested functions") << "nestedFunctions.qml"
                                      << "nestedFunctions.formatted.qml" << QStringList {};
    QTest::newRow("multiline comments") << "multilineComment.qml"
                                        << "multilineComment.formatted.qml" << QStringList {};
    QTest::newRow("for of") << "forOf.qml"
                            << "forOf.formatted.qml" << QStringList {};
    QTest::newRow("property names") << "propertyNames.qml"
                                    << "propertyNames.formatted.qml" << QStringList {};
    QTest::newRow("empty object") << "emptyObject.qml"
                                  << "emptyObject.formatted.qml" << QStringList {};
    QTest::newRow("arrow functions") << "arrowFunctions.qml"
                                     << "arrowFunctions.formatted.qml" << QStringList {};
}

void TestQmlformat::testFormat()
{
    QFETCH(QString, file);
    QFETCH(QString, fileFormatted);
    QFETCH(QStringList, args);

    QCOMPARE(runQmlformat(testFile(file), args), readTestFile(fileFormatted));
}

#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
void TestQmlformat::testExample_data()
{
    if (QTestPrivate::isRunningArmOnX86())
        QSKIP("Crashes in QEMU. (timeout)");
    QTest::addColumn<QString>("file");

    QString examples = QLatin1String(SRCDIR) + "/../../../../examples/";
    QString tests = QLatin1String(SRCDIR) + "/../../../../tests/";

    QStringList files;
    files << findFiles(QDir(examples));
    files << findFiles(QDir(tests));

    for (const QString &file : files)
        QTest::newRow(qPrintable(file)) << file;
}
#endif

#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
void TestQmlformat::testExample()
{
    QFETCH(QString, file);
    const bool isInvalid = isInvalidFile(QFileInfo(file));
    QString output = runQmlformat(file, {}, !isInvalid);

    if (!isInvalid)
        QVERIFY(!output.isEmpty());
}
#endif

QString TestQmlformat::runQmlformat(const QString &fileToFormat, QStringList args,
                                    bool shouldSucceed)
{
    // Copy test file to temporary location
    QTemporaryDir tempDir;
    const QString tempFile = tempDir.path() + QDir::separator() + "to_format.qml";
    QFile::copy(fileToFormat, tempFile);

    args << QLatin1String("-i");
    args << tempFile;

    auto verify = [&]() {
        QProcess process;
        process.start(m_qmlformatPath, args);
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        if (shouldSucceed)
            QCOMPARE(process.exitCode(), 0);
    };
    verify();

    QFile temp(tempFile);

    temp.open(QIODevice::ReadOnly);
    QString formatted = QString::fromUtf8(temp.readAll());

    return formatted;
}

QTEST_MAIN(TestQmlformat)
#include "tst_qmlformat.moc"
