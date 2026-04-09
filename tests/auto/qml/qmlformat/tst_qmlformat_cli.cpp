// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/private/qemulationdetector_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomlinewriter_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtQmlFormat/private/qqmlformatoptions_p.h>

#include "tst_qmlformat_base.h"

using namespace QQmlJS::Dom;

// TODO refactor extension helpers
const QString QML_EXT = ".qml";
const QString JS_EXT = ".js";
const QString MJS_EXT = ".mjs";
static QStringView fileExt(QStringView filename)
{
    if (filename.endsWith(QML_EXT)) {
        return QML_EXT;
    }
    if (filename.endsWith(JS_EXT)) {
        return JS_EXT;
    }
    if (filename.endsWith(MJS_EXT)) {
        return MJS_EXT;
    }
    Q_UNREACHABLE();
};

class TestQmlformatCli : public TestQmlformatBase
{
    Q_OBJECT

public:
    enum class RunOption { OnCopy, OrigToCopy };
    TestQmlformatCli();

private Q_SLOTS:
    void initTestCase() override;

    // actually testFormat tests CLI of qmlformat
    void testFormat();
    void testFormat_data();

    void testLineEndings();

    void testBackupFileLimit();

    void testFilesOption_data();
    void testFilesOption();

    void commandLineOptions_data();
    void commandLineOptions();

    void writeDefaults();

    void outputOptions();

    void settingsKeysStayStable();

    void settingsFromFileOrCommandLine_data();
    void settingsFromFileOrCommandLine();

    void multipleSettingsFiles();

private:
    // TODO(QTBUG-117849) refactor this helper function
    QString runQmlformat(const QString &fileToFormat, QStringList args, bool shouldSucceed = true,
                         RunOption rOption = RunOption::OnCopy, QStringView ext = QML_EXT);

    QString m_qmlformatPath;
};

// Don't fail on warnings because we read a lot of QML files that might intentionally be malformed.
TestQmlformatCli::TestQmlformatCli()
    : TestQmlformatBase(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::DoNotFailOnWarnings)
{
}

void TestQmlformatCli::initTestCase()
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
}

QString TestQmlformatCli::runQmlformat(const QString &fileToFormat, QStringList args,
                                    bool shouldSucceed, RunOption rOptions, QStringView ext)
{
    // Copy test file to temporary location
    QTemporaryDir tempDir;
    const QString tempFile = (tempDir.path() + QDir::separator() + "to_format") % ext;

    if (rOptions == RunOption::OnCopy) {
        QFile::copy(fileToFormat, tempFile);
        QFile::copy(testFile(".qmlformat.ini"), tempDir.filePath(".qmlformat.ini"));
        args << QLatin1String("-i");
        args << tempFile;
    } else {
        args << fileToFormat;
    }

    auto verify = [&]() {
        QProcess process;
        if (rOptions == RunOption::OrigToCopy)
            process.setStandardOutputFile(tempFile);
        process.start(m_qmlformatPath, args);
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        if (shouldSucceed)
            QCOMPARE(process.exitCode(), 0);
    };
    verify();

    QFile temp(tempFile);

    if (!temp.open(QIODevice::ReadOnly))
        qFatal("Could not open %s", qPrintable(tempFile));
    QString formatted = QString::fromUtf8(temp.readAll());

    return formatted;
}


void TestQmlformatCli::testLineEndings()
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

void TestQmlformatCli::testFormat_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("fileFormatted");
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<RunOption>("runOption");

    QTest::newRow("example1 (tabs)")
            << "Example1.qml"
            << "Example1.formatted.tabs.qml" << QStringList { "-t" } << RunOption::OnCopy;
    QTest::newRow("example1 (two spaces)")
            << "Example1.qml"
            << "Example1.formatted.2spaces.qml" << QStringList { "-w", "2" } << RunOption::OnCopy;
    QTest::newRow("settings") << "settings/Example1.qml"
                              << "settings/Example1.formatted_mac_cr.qml" << QStringList {}
                              << RunOption::OrigToCopy;
    QTest::newRow("objects spacing (no changes)")
            << "objectsSpacing.qml"
            << "objectsSpacing.formatted.qml" << QStringList { "--objects-spacing" } << RunOption::OnCopy;

    QTest::newRow("normalize + objects spacing")
            << "normalizedObjectsSpacing.qml"
            << "normalizedObjectsSpacing.formatted.qml" << QStringList { "-n", "--objects-spacing" } << RunOption::OnCopy;

    QTest::newRow("sorting imports")
        << "sortingImports.qml"
        << "sortingImports.formatted.qml" << QStringList { "-S" } << RunOption::OnCopy;

    QTest::newRow("ids new lines")
            << "checkIdsNewline.qml"
            << "checkIdsNewline.formatted.qml" << QStringList { "-n" } << RunOption::OnCopy;

    QTest::newRow("functions spacing (no changes)")
            << "functionsSpacing.qml"
            << "functionsSpacing.formatted.qml" << QStringList { "--functions-spacing" } << RunOption::OnCopy;

    QTest::newRow("normalize + functions spacing")
            << "normalizedFunctionsSpacing.qml"
            << "normalizedFunctionsSpacing.formatted.qml" << QStringList { "-n", "--functions-spacing" } << RunOption::OnCopy;

    QTest::newRow("normalize + keep attributes order")
            << "normalizedGroupAttributesTogether.qml"
            << "normalizedGroupAttributesTogether.formatted.qml"
            << QStringList{ "--group-attributes-together" } << RunOption::OnCopy;

    QTest::newRow("indentEquals2")
            << "threeFunctionsOneLine.js"
            << "threeFunctions.formattedW2.js" << QStringList{"-w=2"} << RunOption::OnCopy;

    QTest::newRow("tabIndents")
            << "threeFunctionsOneLine.js"
            << "threeFunctions.formattedTabs.js" << QStringList{"-t"} << RunOption::OnCopy;

    QTest::newRow("normalizedFunctionSpacing")
            << "threeFunctionsOneLine.js"
            << "threeFunctions.formattedFuncSpacing.js"
            << QStringList{ "-n", "--functions-spacing" } << RunOption::OnCopy;

    QTest::newRow("esm_tabIndents")
            << "mini_esm.mjs"
            << "mini_esm.formattedTabs.mjs" << QStringList{ "-t" } << RunOption::OnCopy;

    QTest::newRow("singeLineEmptyObjects")
            << "singleLineEmptyObjects.qml"
            << "singleLineEmptyObjects.formatted.qml"
            << QStringList{ "--single-line-empty-objects" } << RunOption::OnCopy;
}

void TestQmlformatCli::testFormat()
{
    QFETCH(QString, file);
    QFETCH(QString, fileFormatted);
    QFETCH(QStringList, args);
    QFETCH(RunOption, runOption);

    auto formatted = runQmlformat(testFile(file), args, true, runOption, fileExt(file));
    QEXPECT_FAIL("normalizedFunctionSpacing",
                 "Normalize && function spacing are not yet supported for JS", Abort);
    auto exp = readTestFile(fileFormatted);
    QCOMPARE(formatted, exp);
}

void TestQmlformatCli::testBackupFileLimit()
{
    // Create a temporary directory
    QTemporaryDir tempDir;

    // Unformatted file to format
    const QString fileToFormat{ testFile("Annotations.qml") };

    {
        const QString tempFile = tempDir.path() + QDir::separator() + "test_0.qml";
        const QString backupFile = tempFile + QStringLiteral("~");
        QFile::copy(fileToFormat, tempFile);

        QProcess process;
        process.start(m_qmlformatPath, QStringList{ "--verbose", "--inplace", tempFile });
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
        QVERIFY(QFileInfo::exists(tempFile));
        QVERIFY(!QFileInfo::exists(backupFile));
    };
}

void TestQmlformatCli::testFilesOption_data()
{
    QTest::addColumn<QString>("containerFile");
    QTest::addColumn<QStringList>("individualFiles");

    QTest::newRow("initial") << "fileListToFormat" << QStringList{ "valid1.qml", "valid2.qml" };
}

void TestQmlformatCli::testFilesOption()
{
    QFETCH(QString, containerFile);
    QFETCH(QStringList, individualFiles);

    // Create a temporary directory
    QTemporaryDir tempDir;
    QStringList actualFormattedFilesPath;

    // Iterate through files in the source directory and copy them to the temporary directory
    const auto sourceDir = dataDirectory() + QDir::separator() + "filesOption";

    // Create a file that contains the list of files to be formatted
    const QString tempFilePath = tempDir.path() + QDir::separator() + containerFile;
    QFile container(tempFilePath);
    QVERIFY2(container.open(QIODevice::Text | QIODevice::WriteOnly),
             "Cannot create temp test file");

    QFile::copy(testFile(".qmlformat.ini"), tempDir.filePath(".qmlformat.ini"));

    QTextStream out(&container);
    for (const auto &file : individualFiles) {
        QString destinationFilePath = tempDir.path() + QDir::separator() + file;
        if (QFile::copy(sourceDir + QDir::separator() + file, destinationFilePath))
            actualFormattedFilesPath << destinationFilePath;
        out << destinationFilePath << "\n";
    }
    container.close();

    {
        QProcess process;
        process.start(m_qmlformatPath, QStringList{"-F", tempFilePath});
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
    }

    const auto readFile = [](const QString &filePath){
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "Error on opening the file " << filePath;
            return QByteArray{};
        }

        return file.readAll();
    };

    for (const auto &filePath : std::as_const(actualFormattedFilesPath)) {
        auto expectedFormattedFile = QFileInfo(filePath).fileName();
        const auto expectedFormattedFilePath = sourceDir + QDir::separator() +
            expectedFormattedFile.replace(".qml", ".formatted.qml");

        QCOMPARE(readFile(filePath), readFile(expectedFormattedFilePath));
    }
}

void TestQmlformatCli::commandLineOptions_data()
{
    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QString>("expectedErrorMessage");

    const QString dummy = testFile("dummy.qml");
    const QString empty = testFile("empty");
    QTest::newRow("columnWidthError")
            << QStringList{ dummy, "-W", "-11111" }
            << "Error: Invalid value passed to -W. Must be an integer >= -1\n";
    QTest::newRow("columnWidthNoError")
            << QStringList{ dummy, "-W", "80" } << "";
    QTest::newRow("indentWidthError")
            << QStringList{ dummy, "--indent-width", "expect integer" }
            << "Error: Invalid value passed to -w\n";
    QTest::newRow("indentWidthNoError")
            << QStringList{ dummy, "--indent-width", "4" } << "";
    QTest::newRow("noInputFiles.qml")
            << QStringList{} << "Error: Expected at least one input file.\n";
    QTest::newRow("fOptionFileDoesNotExist")
            << QStringList{ "-F", "nope" }
            << "Error: Could not open file \"nope\" for option -F.\n";
    QTest::newRow("fOptionFileIsEmpty")
            << QStringList{ "-F", empty }
            << "Error: File \"" + empty + "\" for option -F is empty.\n";
    QTest::newRow("fOptionFileContainsNope")
            << QStringList{ "-F", testFile("filesToFormatNope") }
            << "Error: Entry \"nope\" of file \"" + testFile("filesToFormatNope")
                    + "\" passed to option -F could not be found.\n";
    QTest::newRow("positionalArgumentDoesNotExist")
            << QStringList{ "nope" }
            << "Error: Could not find file \"nope\".\n";
}

void TestQmlformatCli::commandLineOptions()
{
    QFETCH(QStringList, args);
    QFETCH(QString, expectedErrorMessage);

    auto verify = [&]() {
        QTemporaryDir tempDir;
        const QString tempFile = tempDir.path() + QDir::separator() + "test_0.qml";

        QProcess process;
        process.setStandardOutputFile(tempFile);
        process.start(m_qmlformatPath, args);
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        // normalized error message
        auto rawError = process.readAllStandardError();
        QTextStream stream(&rawError, QIODeviceBase::ReadOnly | QIODeviceBase::Text);
        QCOMPARE(stream.readAll(), expectedErrorMessage.toUtf8());
        if (expectedErrorMessage.isEmpty())
            QCOMPARE(process.exitCode(), 0);
        else
            QCOMPARE_NE(process.exitCode(), 0);
    };

    verify();
}

void TestQmlformatCli::writeDefaults()
{
    QTemporaryDir tempDir;
    const QString qmlformatIni = tempDir.path() + QDir::separator() + ".qmlformat.ini";

    QProcess process;
    process.setWorkingDirectory(tempDir.path());
    process.start(m_qmlformatPath, QStringList{ "--write-defaults" });
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    QQmlToolingSettings settings("qmlformat");
    QVERIFY(settings.search(qmlformatIni).isValid());

    QVERIFY(settings.isSet(QQmlFormatSettings::s_useTabsSetting));
    QCOMPARE(settings.value(QQmlFormatSettings::s_useTabsSetting).toBool(), false);

    QVERIFY(settings.isSet(QQmlFormatSettings::s_indentWidthSetting));
    QCOMPARE(settings.value(QQmlFormatSettings::s_indentWidthSetting).toInt(), 4);

    QVERIFY(settings.isSet(QQmlFormatSettings::s_maxColumnWidthSetting));
    QCOMPARE(settings.value(QQmlFormatSettings::s_maxColumnWidthSetting).toInt(), -1);

    QVERIFY(settings.isSet(QQmlFormatSettings::s_normalizeSetting));
    QCOMPARE(settings.value(QQmlFormatSettings::s_normalizeSetting).toBool(), false);

    QVERIFY(settings.isSet(QQmlFormatSettings::s_newlineSetting));
    QCOMPARE(settings.value(QQmlFormatSettings::s_newlineSetting).toString(), "native");

    QVERIFY(settings.isSet(QQmlFormatSettings::s_objectsSpacingSetting));
    QCOMPARE(settings.value(QQmlFormatSettings::s_objectsSpacingSetting).toBool(), false);

    QVERIFY(settings.isSet(QQmlFormatSettings::s_functionsSpacingSetting));
    QCOMPARE(settings.value(QQmlFormatSettings::s_functionsSpacingSetting).toBool(), false);

    QVERIFY(settings.isSet(QQmlFormatSettings::s_groupAttributesTogetherSetting));
    QCOMPARE(settings.value(QQmlFormatSettings::s_groupAttributesTogetherSetting).toBool(), false);

    QVERIFY(settings.isSet(QQmlFormatSettings::s_sortImportsSetting));
    QCOMPARE(settings.value(QQmlFormatSettings::s_sortImportsSetting).toBool(), false);

    QVERIFY(settings.isSet(QQmlFormatSettings::s_singleLineEmptyObjectsSetting));
    QCOMPARE(settings.value(QQmlFormatSettings::s_singleLineEmptyObjectsSetting).toBool(), false);

    QVERIFY(settings.isSet(QQmlFormatSettings::s_semiColonRuleSetting));
    QCOMPARE(settings.value(QQmlFormatSettings::s_semiColonRuleSetting).toString(), "always"_L1);
}

void TestQmlformatCli::outputOptions()
{
    QProcess process;
    process.start(m_qmlformatPath, QStringList{ "--output-options" });
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);

    QJsonDocument doc = QJsonDocument::fromJson(process.readAllStandardOutput());

    auto findJsonObject = [doc](const QString &name){
        QJsonObject rootObj = doc.object();
        QJsonArray optionsArray = rootObj["options"].toArray();

        for (const QJsonValue &optionValue : optionsArray) {
            if (!optionValue.isObject())
                continue;

            QJsonObject optionObj = optionValue.toObject();
            if (optionObj["name"].toString() == name)
                return optionObj;
        }

        return QJsonObject();
    };

    {
        QJsonObject obj = findJsonObject(QQmlFormatSettings::s_useTabsSetting);
        QVERIFY(!obj.isEmpty());
        QCOMPARE(obj["value"], false);
        QCOMPARE(obj["hint"], QMetaType::fromType<bool>().name());
    }

    {
        QJsonObject obj = findJsonObject(QQmlFormatSettings::s_indentWidthSetting);
        QVERIFY(!obj.isEmpty());
        QCOMPARE(obj["value"], 4);
        QCOMPARE(obj["hint"], QMetaType::fromType<int>().name());
    }

    {
        QJsonObject obj = findJsonObject(QQmlFormatSettings::s_maxColumnWidthSetting);
        QVERIFY(!obj.isEmpty());
        QCOMPARE(obj["value"], -1);
        QCOMPARE(obj["hint"], QMetaType::fromType<int>().name());
    }

    {
        QJsonObject obj = findJsonObject(QQmlFormatSettings::s_normalizeSetting);
        QVERIFY(!obj.isEmpty());
        QCOMPARE(obj["value"], false);
        QCOMPARE(obj["hint"], QMetaType::fromType<bool>().name());
    }

    {
        QJsonObject obj = findJsonObject(QQmlFormatSettings::s_newlineSetting);
        QVERIFY(!obj.isEmpty());
        QCOMPARE(obj["value"], "native"_L1);
        QCOMPARE(obj["hint"], QStringList({ "unix", "windows", "macos", "native" }).join(','));
    }

    {
        QJsonObject obj = findJsonObject(QQmlFormatSettings::s_objectsSpacingSetting);
        QVERIFY(!obj.isEmpty());
        QCOMPARE(obj["value"], false);
        QCOMPARE(obj["hint"], QMetaType::fromType<bool>().name());
    }

    {
        QJsonObject obj = findJsonObject(QQmlFormatSettings::s_functionsSpacingSetting);
        QVERIFY(!obj.isEmpty());
        QCOMPARE(obj["value"], false);
        QCOMPARE(obj["hint"], QMetaType::fromType<bool>().name());
    }

    {
        QJsonObject obj = findJsonObject(QQmlFormatSettings::s_groupAttributesTogetherSetting);
        QVERIFY(!obj.isEmpty());
        QCOMPARE(obj["value"], false);
        QCOMPARE(obj["hint"], QMetaType::fromType<bool>().name());
    }

    {
        QJsonObject obj = findJsonObject(QQmlFormatSettings::s_sortImportsSetting);
        QVERIFY(!obj.isEmpty());
        QCOMPARE(obj["value"], false);
        QCOMPARE(obj["hint"], QMetaType::fromType<bool>().name());
    }

    {
        QJsonObject obj = findJsonObject(QQmlFormatSettings::s_singleLineEmptyObjectsSetting);
        QVERIFY(!obj.isEmpty());
        QCOMPARE(obj["value"], false);
        QCOMPARE(obj["hint"], QMetaType::fromType<bool>().name());
    }

    {
        QJsonObject obj = findJsonObject(QQmlFormatSettings::s_semiColonRuleSetting);
        QVERIFY(!obj.isEmpty());
        QCOMPARE(obj["value"], "always"_L1);
        QCOMPARE(obj["hint"], QStringList({ "always", "essential" }).join(','));
    }
}

void TestQmlformatCli::settingsKeysStayStable()
{
    QCOMPARE(QQmlFormatSettings::s_useTabsSetting, "UseTabs"_L1);
    QCOMPARE(QQmlFormatSettings::s_indentWidthSetting, "IndentWidth"_L1);
    QCOMPARE(QQmlFormatSettings::s_maxColumnWidthSetting, "MaxColumnWidth"_L1);
    QCOMPARE(QQmlFormatSettings::s_normalizeSetting, "NormalizeOrder"_L1);
    QCOMPARE(QQmlFormatSettings::s_newlineSetting, "NewlineType"_L1);
    QCOMPARE(QQmlFormatSettings::s_objectsSpacingSetting, "ObjectsSpacing"_L1);
    QCOMPARE(QQmlFormatSettings::s_functionsSpacingSetting, "FunctionsSpacing"_L1);
    QCOMPARE(QQmlFormatSettings::s_groupAttributesTogetherSetting, "GroupAttributesTogether"_L1);
    QCOMPARE(QQmlFormatSettings::s_sortImportsSetting, "SortImports"_L1);
    QCOMPARE(QQmlFormatSettings::s_semiColonRuleSetting, "SemicolonRule"_L1);
}

void TestQmlformatCli::settingsFromFileOrCommandLine_data()
{
    QTest::addColumn<QString>("qmlformatIniPath");
    QTest::addColumn<QStringList>("qmlformatInitOptions");
    QTest::addColumn<QQmlFormatOptions>("expectedOptions");

    {
        QQmlFormatOptions options;
        options.setIndentWidth(20);
        // In settings file, indentwidth is set to 4000, while cli overrides it to 20
        // 20 should be the final value
        QTest::newRow("clOverridesIndentWidth")
                << testFile("iniFiles/dummySettingsFile.ini")
                << QStringList{ m_qmlformatPath, "--indent-width", "20" } << options;
        options.setIndentWidth(4000);
        // In settings file, indentwidth is set to 4000, and nothing overrides it.
        // 4000 should be the final value
        QTest::newRow("iniFileIndentWidth") << testFile("iniFiles/dummySettingsFile.ini")
                                            << QStringList{ m_qmlformatPath } << options;
        options.setMaxColumnWidth(100);
        // In settings file, maxcolumnwidth is set to -1, but cli overrides it 100.
        // 100 should be the final value
        QTest::newRow("clOverridesColumnWidth")
                << testFile("iniFiles/dummySettingsFile.ini")
                << QStringList{ m_qmlformatPath, "-W", "100" } << options;
    }
    {
        QQmlFormatOptions options;
        // settings file sets all bools excepts Tabs to true.
        options.setTabsEnabled(false);
        options.setNormalizeEnabled(true);
        options.setObjectsSpacing(true);
        options.setFunctionsSpacing(true);
        QTest::newRow("iniFileSetsBools") << testFile("iniFiles/toggledBools.ini")
                                          << QStringList{ m_qmlformatPath } << options;

        // cli overrides the Tabs option to true
        options.setTabsEnabled(true);
        QTest::newRow("cliOverridesTabs") << testFile("iniFiles/toggledBools.ini")
                                          << QStringList{ m_qmlformatPath, "--tabs" } << options;
    }
    {
        // settings should apply when -F is passed
        QQmlFormatOptions options;
        options.setIndentWidth(4000);
        QTest::newRow("settingOnFilesOption")
                << testFile("iniFiles/dummySettingsFile.ini")
                << QStringList{ m_qmlformatPath, "-F", "dummyFilesPath" } << options;
    }
    {
        // In settings file, semicolonRule is set to Essential and cli does not override it.
        // Essential should be the final value
        QQmlFormatOptions expectedOptions;
        expectedOptions.setSemicolonRule(QQmlJS::Dom::LineWriterOptions::SemicolonRule::Essential);
        QTest::newRow("semiColonRuleFromIniFile")
                << testFile("iniFiles/semicolonRule.ini")
                << QStringList{ m_qmlformatPath} << expectedOptions;
    }
}

void TestQmlformatCli::settingsFromFileOrCommandLine()
{
    QFETCH(QString, qmlformatIniPath);
    QFETCH(QStringList, qmlformatInitOptions);
    QFETCH(QQmlFormatOptions, expectedOptions);

    auto verify = [&]() {
        QTemporaryDir tempDir;
        const QString qmlformatIni = tempDir.path() + QDir::separator() + ".qmlformat.ini";
        const QString dummyQmlFile = tempDir.path() + QDir::separator() + "test.qml";

        QFile::copy(qmlformatIniPath, qmlformatIni);
        QQmlFormatSettings settings("qmlformat");
        QStringList cmdlineOptions;
        if ((qstrcmp(QTest::currentDataTag(), "settingOnFilesOption") == 0))
            cmdlineOptions = qmlformatInitOptions << "-F" << dummyQmlFile;
        else
            cmdlineOptions = QStringList(dummyQmlFile) << qmlformatInitOptions;

        QQmlFormatOptions options = QQmlFormatOptions::buildCommandLineOptions(cmdlineOptions);
        auto overridenOptions = options.optionsForFile(dummyQmlFile, &settings);

        QCOMPARE(overridenOptions.tabsEnabled(), expectedOptions.tabsEnabled());
        QCOMPARE(overridenOptions.indentWidth(), expectedOptions.indentWidth());
        QCOMPARE(overridenOptions.maxColumnWidth(), expectedOptions.maxColumnWidth());
        QCOMPARE(overridenOptions.normalizeEnabled(), expectedOptions.normalizeEnabled());
        QCOMPARE(overridenOptions.newline(), expectedOptions.newline());
        QCOMPARE(overridenOptions.objectsSpacing(), expectedOptions.objectsSpacing());
        QCOMPARE(overridenOptions.functionsSpacing(), expectedOptions.functionsSpacing());
        QCOMPARE(overridenOptions.sortImports(), expectedOptions.sortImports());
        QCOMPARE(overridenOptions.semicolonRule(), expectedOptions.semicolonRule());
    };

    verify();
}

/*
* Create a temporary directory with the following structure
|--dir1
|  |--.qmlformat.ini
|  |-- test1.qml
|--dir2
|  |-- test2.qml

* test2.qml should differ from the test2.qml options on indentwidth, because test1 gets it from
* its settings file.
*/
void TestQmlformatCli::multipleSettingsFiles()
{
    QTemporaryDir tempDir;
    QTemporaryDir dir1(tempDir.path() + "/dir1");
    QTemporaryDir dir2(tempDir.path() + "/dir2");
    const QString qmlformat1Ini = dir1.path() + "/.qmlformat.ini";
    const QString test1Qml = dir1.path() + "/test.qml";
    const QString test2Qml = dir2.path() + "/test.qml";

    QFile::copy(testFile("iniFiles/dummySettingsFile.ini"), qmlformat1Ini);
    QQmlFormatSettings settings("qmlformat");
    QQmlFormatOptions options =
            QQmlFormatOptions::buildCommandLineOptions(QStringList{ m_qmlformatPath });
    auto test1Options = options.optionsForFile(test1Qml, &settings);
    auto test2Options = options.optionsForFile(test2Qml, &settings);

    QCOMPARE(test1Options.tabsEnabled(), test2Options.tabsEnabled());
    QCOMPARE_NE(test1Options.indentWidth(), test2Options.indentWidth());
    QCOMPARE(test1Options.maxColumnWidth(), test2Options.maxColumnWidth());
    QCOMPARE(test1Options.normalizeEnabled(), test2Options.normalizeEnabled());
    QCOMPARE(test1Options.newline(), test2Options.newline());
    QCOMPARE(test1Options.objectsSpacing(), test2Options.objectsSpacing());
    QCOMPARE(test1Options.functionsSpacing(), test2Options.functionsSpacing());
    QCOMPARE(test1Options.sortImports(), test2Options.sortImports());
    QCOMPARE(test1Options.semicolonRule(), test2Options.semicolonRule());
}
QTEST_MAIN(TestQmlformatCli)
#include "tst_qmlformat_cli.moc"
