// Copyright (C) 2016 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sergio Martins <sergio.martins@kdab.com>
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qmlutils_p.h>
#include <private/qqmljscontextproperties_p.h>
#include <private/qqmljslinter_p.h>
#include <private/qqmlsa_p.h>
#include <private/qqmltoolingsettings_p.h>
#include <private/qtqmlglobal_p.h>

#include <QtCore/qcomparehelpers.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qplugin.h>
#include <QtCore/qprocess.h>
#include <QtCore/qstring.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/qxmlstream.h>
#include <QtTest/qtest.h>

#if QT_CONFIG(qmlcontextpropertydump)
#  include <QtCore/qsettings.h>
#endif

Q_IMPORT_PLUGIN(LintPlugin)

using namespace Qt::StringLiterals;

class TestQmllint: public QQmlDataTest
{
    Q_OBJECT

public:
    TestQmllint();

    struct Message
    {
        QString text = QString();
        quint32 line = 0, column = 0;
        QtMsgType severity = QtWarningMsg;
    };

    struct Edit
    {
        QString replacement;
        quint32 line = 0;
        quint32 column = 0;
    };
    using Edits = QList<Edit>;

    struct Fix
    {
        QString text;
        QList<Edit> edits;
        quint32 line = 0, column = 0;

        Fix(const QString &text, const QList<Edit> &&edits, quint32 line = 0, quint32 column = 0)
            : text(text), edits(edits), line(line), column(column)
        {}

        Fix(const QString &text, Edit &&edit, quint32 line = 0, quint32 column = 0)
            : text(text), edits(QList<Edit>() << edit), line(line), column(column)
        {}
    };

    struct ResultBuilder;
    struct Result
    {
        enum Flag {
            ExitsNormally = 0x1,
            NoMessages = 0x2,
            AutoFixable = 0x4,
            UseSettings = 0x8
        };
        Q_DECLARE_FLAGS(Flags, Flag)

    private:
        friend struct ResultBuilder;
        Result() = default;
        Result(const QList<Message> &expectedMessages, const QList<Message> &unexpectedMessages,
               const QList<Fix> &fixes, Flags flags)
            : expectedMessages(expectedMessages), unexpectedMessages(unexpectedMessages),
              expectedFixes(fixes), flags(flags) { }

    public:
        QList<Message> expectedMessages = {};
        QList<Message> unexpectedMessages = {};
        QList<Fix> expectedFixes = {};
        Flags flags = {};
    };

    struct ResultBuilder
    {
        ResultBuilder &addExpected(const QString &text, quint32 line = 0, quint32 column = 0,
                                   QtMsgType severity = QtWarningMsg)
        {
            result.expectedMessages.append({ text, line, column, severity });
            return *this;
        }

        ResultBuilder &addUnexpected(const QString &text, quint32 line = 0, quint32 column = 0,
                                     QtMsgType severity = QtWarningMsg)
        {
            result.unexpectedMessages.append({ text, line, column, severity });
            return *this;
        }

        ResultBuilder &addFix(const QString &text, const QList<Edit> &&edits = {}, quint32 line = 0,
                              quint32 column = 0)
        {
            result.expectedFixes.append(Fix(text, std::move(edits), line, column));
            return *this;
        }
        ResultBuilder &addFix(const QString &text, Edit &&edit, quint32 line = 0,
                              quint32 column = 0)
        {
            result.expectedFixes.append(Fix(text, std::move(edit), line, column));
            return *this;
        }

        ResultBuilder &setFlag(Result::Flag f, bool v = true)
        {
            result.flags.setFlag(f, v);
            return *this;
        }

        Result build() { return result; }

        static Result singleExpected(const QString &text, quint32 line = 0, quint32 column = 0,
                                     QtMsgType severity = QtWarningMsg)
        {
            Result result;
            result.expectedMessages.append({ text, line, column, severity });
            return result;
        }

        static Result cleanResult()
        {
            Result result;
            result.flags |= Result::Flags(Result::NoMessages | Result::ExitsNormally);
            return result;
        }

        static Result cleanResultWithSettings()
        {
            Result result;
            result.flags |= Result::Flags(Result::NoMessages | Result::ExitsNormally
                                          | Result::UseSettings);
            return result;
        }

        static Result ignoredResult() { return Result{}; }

    private:
        Result result;
    };

    struct BatchElement
    {
        QString file;
        Result result;
    };
    struct Batch
    {
        QList<BatchElement> elements;
        Batch &addCleanFile(const QString &file)
        {
            elements.push_back({ file, ResultBuilder::cleanResult() });
            return *this;
        }
        Batch &addDirtyFile(const QString &file, const Result &result)
        {
            elements.push_back({ file, result });
            return *this;
        }
    };

    struct Environment : public QList<std::pair<QString, QString>>
    {
        using QList<std::pair<QString, QString>>::QList;
    };

private Q_SLOTS:
    void initTestCase() override;

    void testUnqualified();
    void testUnqualified_data();

    void batches_data();
    void batches();

    void cleanQmlCode_data();
    void cleanQmlCode();

    void dirtyQmlCode_data();
    void dirtyQmlCode();

    void dirtyQmlSnippet_data();
    void dirtyQmlSnippet();

    void cleanQmlSnippet_data();
    void cleanQmlSnippet();

    void dirtyJsSnippet_data();
    void dirtyJsSnippet();

    void cleanJsSnippet_data();
    void cleanJsSnippet();

    void contextPropertiesFromRootUrls_data();
    void contextPropertiesFromRootUrls();
    void contextPropertiesFromUser();
#if QT_CONFIG(qmlcontextpropertydump)
    void contextPropertiesFromHeuristicWrite();
    void contextPropertiesFromHeuristicRead();
    void contextPropertiesFromHeuristicLint();
#endif

    void compilerWarnings_data();
    void compilerWarnings();

    void cycles_data();
    void cycles();

    void jsFileInBatch();

    void testUnknownCausesFail();

    void directoryPassedAsQmlTypesFile();
    void oldQmltypes();

    void qmltypes_data();
    void qmltypes();

    void autoqmltypes();
    void resources();

    void multiDirectory();

    void typeInstantiatedRecursively_data();
    void typeInstantiatedRecursively();
    void typeInstantiatedRecursivelyInBuildFolder();

    void requiredProperty();

    void settingsFile();

    void additionalImplicitImport();

    void qrcUrlImport();

    void incorrectImportFromHost_data();
    void incorrectImportFromHost();

    void attachedPropertyReuse();

    void missingBuiltinsNoCrash();
    void stubQtQuickNoCrash();

    void missingRegistration_data();
    void missingRegistration();

    void absolutePath();

    void importMultipartUri();

    void lintModule_data();
    void lintModule();

    void testLineEndings();
    void valueTypesFromString();

    void ignoreSettingsNotCommandLineOptions();
    void backslashedQmldirPath();

    void environment_data();
    void environment();

    void maxWarnings();

    void unrecognizedIniSection();

    void shadow_data();
    void shadow();

    void uselessExpressionStatements_data();
    void uselessExpressionStatements();

    void crashes();

    void useProperFunction_data();
    void useProperFunction();

    void weakPointers();

    void registerMergeTreeRecursion();

#if QT_CONFIG(library)
    void hasTestPlugin();
    void testPlugin_data();
    void testPlugin();
    void testPluginHelpCommandLine();
    void testPluginCommandLine();
    void quickPlugin();
    void hasQdsPlugin();
    void qdsPlugin_data();
    void qdsPlugin();
#endif

#if QT_CONFIG(process)
    void importRelScript();
#endif

    void replayImportWarnings();
    void errorCategory();
    void noSettingsPollution_data();
    void noSettingsPollution();
    void syntaxIsEssential();
    void essentialCantBeLowered();
    void essentialCanBeRaised();

    void onlyExplicitCategories();
    void onlyExplicitCategoriesIni();

private:
    enum DefaultImportOption { NoDefaultImports, UseDefaultImports };
    enum ContainOption { StringNotContained, StringContained };

    enum LintType { LintFile, LintModule };

    static QStringList warningsShouldFailArgs() {
        static QStringList args {"-W", "0"};
        return args;
    }

    QString runQmllint(const QString &fileToLint, std::function<void(QProcess &)> handleResult,
                       const QStringList &extraArgs = QStringList(), bool ignoreSettings = true,
                       bool addImportDirs = true, bool absolutePath = true,
                       const Environment &env = {});
    QString runQmllint(const QString &fileToLint, bool shouldSucceed,
                       const QStringList &extraArgs = QStringList(), bool ignoreSettings = true,
                       bool addImportDirs = true, bool absolutePath = true,
                       const Environment &env = {});

    enum CallQmllintCheck {
        ShouldFail = 0,
        ShouldSucceed = 1,
        HasAutoFix = 2,
        HasAutoFixAndSucceeds = HasAutoFix | ShouldSucceed,
    };
    Q_DECLARE_FLAGS(CallQmllintChecks, CallQmllintCheck);

    static CallQmllintChecks fromResultFlags(Result::Flags flags)
    {
        CallQmllintChecks result;
        result.setFlag(ShouldSucceed, flags.testFlags(Result::ExitsNormally));
        result.setFlag(HasAutoFix, flags.testFlags(Result::AutoFixable));
        return result;
    }

    struct CallQmllintOptions
    {
        QStringList importPaths = {};
        QStringList qmldirFiles = {};
        QStringList resources = {};
        DefaultImportOption defaultImports = UseDefaultImports;
        QList<QQmlJS::LoggerCategory> *categories = nullptr;
        LintType type = LintFile;
        bool readSettings = false;
        QHash<QString, QQmlJS::WarningSeverity> categorySeverityOverrides = {};
        QStringList rootUrls = {};
        QHash<QString, QString> qrcToFilePaths = {};
        QStringList plugins;
    };

    QJsonArray callQmllintImpl(const QString &fileToLint, const QString &fileCpntent,
                               const CallQmllintOptions &options,
                               CallQmllintChecks check = CallQmllintCheck::ShouldSucceed);
    QJsonArray callQmllint(const QString &fileToLint, const CallQmllintOptions &options,
                           CallQmllintChecks check = CallQmllintCheck::ShouldSucceed);
    QJsonArray callQmllintOnSnippet(const QString &snippet, const CallQmllintOptions &options,
                                    CallQmllintChecks checks);

    void testFixes(QQmlJSLogger *logger, bool shouldSucceed, QStringList importPaths,
                   QStringList qmldirFiles, QStringList resources,
                   DefaultImportOption defaultImports, QList<QQmlJS::LoggerCategory> *categories,
                   bool autoFixable, bool readSettings, const QString &fixedPath);

    void searchWarnings(const QJsonArray &warnings, const QString &string,
                        QtMsgType type = QtWarningMsg, quint32 line = 0, quint32 column = 0,
                        ContainOption shouldContain = StringContained);
    void searchFixes(const QJsonArray &warnings, const QString &substring, const Edits &edits,
                     quint32 line, quint32 column);

    template<typename ExpectedMessageFailureHandler, typename BadMessageFailureHandler,
             typename ReplacementFailureHandler>
    void checkResult(const QJsonArray &warnings, const Result &result,
                     ExpectedMessageFailureHandler onExpectedMessageFailures,
                     BadMessageFailureHandler onBadMessageFailures,
                     ReplacementFailureHandler onReplacementFailures);

    void checkResult(const QJsonArray &warnings, const Result &result)
    {
        checkResult(
                warnings, result, [] {}, [] {}, [] {});
    }

    void runTest(const QString &testFile, const Result &result, QStringList importDirs = {},
                 QStringList qmltypesFiles = {}, QStringList resources = {},
                 DefaultImportOption defaultImports = UseDefaultImports,
                 QList<QQmlJS::LoggerCategory> *categories = nullptr);

    void runTest(const QString &testFile, const Result &result, CallQmllintOptions options);

    QString m_qmllintPath;

    QStringList m_defaultImportPaths;
    class TestLinter : public QQmlJSLinter
    {
    public:
        using QQmlJSLinter::QQmlJSLinter;

        void prepareScopeForReuse(const QString &filePath)
        {
            QQmlJSScope::Ptr scope = m_importer.importFile(filePath);
            QQmlJSScope::resetForReparse(scope);
            resetFactory(
                    scope, &m_importer,
                    [](QQmlJSImporter *, const QString &, const QSharedPointer<QQmlJSScope> &) { });
        }

        QQmlJSLinter::Result lintFile(const QString &filename, const QString *fileContents,
                                      LintOptions options, const QStringList &qmlImportPaths,
                                      const QStringList &qmldirFiles,
                                      const QStringList &resourceFiles,
                                      const QList<QQmlJS::LoggerCategory> &categories)
        {
            if (!prepareFileForBatchLinting(filename, fileContents, options, qmlImportPaths,
                                            qmldirFiles, resourceFiles, categories)) {
                // Calling clearCache for each linted file/snippet slows down the tests by a factor of 2.5.
                // Therefore, only call it when scope re-using happens (prepareFileForBatchLinting returns false).
                clearCache();
                const bool result =
                        prepareFileForBatchLinting(filename, fileContents, options, qmlImportPaths,
                                                   qmldirFiles, resourceFiles, categories);
                Q_ASSERT(result);
            }
            return lintFileInBatch(filename);
        }
    };
    TestLinter m_linter;
    QList<QQmlJS::LoggerCategory> m_categories = QQmlJSLogger::builtinCategories();
};

Q_DECLARE_METATYPE(TestQmllint::Result)

TestQmllint::TestQmllint()
    : QQmlDataTest(QT_QMLTEST_DATADIR),
      m_defaultImportPaths({ QLibraryInfo::path(QLibraryInfo::QmlImportsPath), dataDirectory() }),
      m_linter(m_defaultImportPaths)
{
    for (const QQmlJSLinter::Plugin &plugin : m_linter.plugins())
        m_categories.append(plugin.categories());
}

void TestQmllint::initTestCase()
{
    QQmlDataTest::initTestCase();
    m_qmllintPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmllint");

#ifdef Q_OS_WIN
    m_qmllintPath += QLatin1String(".exe");
#endif
    if (!QFileInfo(m_qmllintPath).exists()) {
        QString message = QStringLiteral("qmllint executable not found (looked for %0)").arg(m_qmllintPath);
        QFAIL(qPrintable(message));
    }
}

void TestQmllint::testUnqualified()
{
    QFETCH(QString, filename);
    QFETCH(Result, result);

    runTest(filename, result);
}

void TestQmllint::testUnqualified_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");

    // id from nowhere (as with setContextProperty)
    QTest::newRow("IdFromOuterSpace")
            << QStringLiteral("IdFromOuterSpace.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1, 4, 8)
               .addExpected("Unqualified access"_L1, 7, 21)
               .build();
    // access property of root object
    QTest::newRow("FromRootDirect")
            << QStringLiteral("FromRoot.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1, 9, 16) // new property
               .addExpected("Unqualified access"_L1, 13, 33) // builtin property
               .addFix("unqualified is a member of a parent element.\n      "_L1
                       "You can qualify the access with its id to avoid this warning."_L1,
                       Edit{ u"root."_s, 9, 16 })
               .addFix("x is a member of a parent element.\n      "_L1
                       "You can qualify the access with its id to avoid this warning."_L1,
                       Edit{ u"root."_s, 13, 33 })
               .build();
    // access injected name from signal
    QTest::newRow("SignalHandler")
            << QStringLiteral("SignalHandler.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1, 5, 21)
               .addExpected("Unqualified access"_L1, 10, 21)
               .addExpected("Unqualified access"_L1, 8, 29)
               .addExpected("Unqualified access"_L1, 12, 34)
               .addFix("\"mouse\" is ambiguous. Use a function instead: function(mouse) { ... }"_L1,
                       Edit{ "function(mouse) "_L1, 4, 22 })
               .addFix("\"mouse\" is ambiguous. Use a function instead: function(mouse) { ... }"_L1,
                       Edit{ "function(mouse) "_L1, 9, 24 })
               .addFix("\"mouse\" is ambiguous. Use a function instead: (mouse) => ..."_L1,
                       Edit{ "(mouse) => "_L1, 8, 16 })
               .addFix("\"mouse\" is ambiguous. Use a function instead: (mouse) => ..."_L1,
                       Edit{ "(mouse) => "_L1, 12, 21 })
               .build();
    // access catch identifier outside catch block
    QTest::newRow("CatchStatement")
            << QStringLiteral("CatchStatement.qml")
            << ResultBuilder().addExpected("Unqualified access"_L1, 6, 21).build();
    QTest::newRow("NonSpuriousParent")
            << QStringLiteral("nonSpuriousParentWarning.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1, 6, 25)
               .addFix("You can qualify the access with its id to avoid this warning "_L1
                       "(You first have to give the element an id)."_L1, Edit{ u"<id>."_s, 6, 25 })
               .build();

    QTest::newRow("crashConnections")
            << QStringLiteral("crashConnections.qml")
            << ResultBuilder().addExpected("FirstRunDialog was not found"_L1, 4, 13).build();

    QTest::newRow("delegateContextProperties")
            << QStringLiteral("delegateContextProperties.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1, 6, 14)
               .addExpected("Unqualified access"_L1, 7, 15)
               .addExpected("'model' is implicitly injected into this delegate. Add a required "
                            "property 'model' to the delegate instead.")
               .addExpected("'index' is implicitly injected into this delegate. Add a required "
                            "property 'index' to the delegate instead.")
               .build();
    QTest::newRow("storeSloppy")
            << QStringLiteral("UnqualifiedInStoreSloppy.qml")
            << ResultBuilder().addExpected("Unqualified access"_L1, 9, 26).build();
    QTest::newRow("storeStrict")
            << QStringLiteral("UnqualifiedInStoreStrict.qml")
            << ResultBuilder().addExpected("Unqualified access", 9, 52).build();
}

void TestQmllint::testUnknownCausesFail()
{
    runTest("unknownElement.qml",
            ResultBuilder()
            .addExpected("Unknown was not found. Did you add all imports and dependencies?"_L1,
                         4, 5, QtWarningMsg)
            .build());
    runTest("TypeWithUnknownPropertyType.qml",
            ResultBuilder()
            .addExpected("Something was not found. Did you add all imports and dependencies?",
                         4, 5, QtWarningMsg)
            .build());
}

void TestQmllint::directoryPassedAsQmlTypesFile()
{
    runTest("unknownElement.qml",
            ResultBuilder()
            .addExpected("QML types file cannot be a directory: "_L1 + dataDirectory())
            .build(),
            {}, { dataDirectory() });
}

void TestQmllint::oldQmltypes()
{
    runTest("oldQmltypes.qml",
            ResultBuilder()
            .addExpected("typeinfo not declared in qmldir file"_L1)
            .addExpected("Found deprecated dependency specifications"_L1)
            .addExpected("Meta object revision and export version differ."_L1)
            .addExpected("Revision 0 corresponds to version 0.0; it should be 1.0."_L1)
            .addUnexpected("QQuickItem was not found. Did you add all imports and dependencies?"_L1)
            .build());

    runTest("oldUnusedQmlTypes.qml",
            ResultBuilder()
            .addExpected("typeinfo not declared in qmldir file"_L1)
            .addExpected("Found deprecated dependency specifications"_L1)
            .addExpected("Meta object revision and export version differ."_L1)
            .addExpected("Revision 0 corresponds to version 0.0; it should be 1.0.")
            .addUnexpected("Unused import"_L1, 1, 1, QtInfoMsg)
            .build());
}

void TestQmllint::qmltypes_data()
{
    QTest::addColumn<QString>("file");

    const QString importsPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    QDirIterator it(importsPath, { "*.qmltypes" },
                    QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        QTest::addRow("%s", qPrintable(it.next().mid(importsPath.size()))) << it.filePath();
}

void TestQmllint::qmltypes()
{
    QFETCH(QString, file);
    callQmllint(file, {});
}

void TestQmllint::autoqmltypes()
{
    QProcess process;
    process.setWorkingDirectory(testFile("autoqmltypes"));
    process.start(m_qmllintPath, warningsShouldFailArgs() << QStringLiteral("test.qml") );

    process.waitForFinished();

    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QVERIFY(process.exitCode() != 0);

    QVERIFY(process.readAllStandardError()
                .contains("is not a qmldir file. Assuming qmltypes"));
    QVERIFY(process.readAllStandardOutput().isEmpty());

    {
        QProcess bare;
        bare.setWorkingDirectory(testFile("autoqmltypes"));
        bare.start(m_qmllintPath, warningsShouldFailArgs() << QStringLiteral("--bare") << QStringLiteral("test.qml") );
        bare.waitForFinished();

        const QByteArray errors = bare.readAllStandardError();
        QVERIFY(!errors.contains("is not a qmldir file. Assuming qmltypes"));
        QVERIFY(errors.contains("Failed to import TestTest."));
        QVERIFY(bare.readAllStandardOutput().isEmpty());

        QCOMPARE(bare.exitStatus(), QProcess::NormalExit);
        QVERIFY(bare.exitCode() != 0);
    }
}

void TestQmllint::resources()
{
    {
        // We need to clear the import cache before we add a qrc file with different
        // contents for the same paths.
        const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });

        CallQmllintOptions options;
        options.resources.append(testFile("resource.qrc"));

        callQmllint(testFile("resource.qml"), options);
        callQmllint(testFile("badResource.qml"), options, CallQmllintCheck::ShouldFail);
    }

    callQmllint(testFile("resource.qml"), CallQmllintOptions{}, CallQmllintCheck::ShouldFail);
    callQmllint(testFile("badResource.qml"), CallQmllintOptions{});

    {
        const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });
        CallQmllintOptions options;
        options.resources.append(testFile("T/a.qrc"));

        callQmllint(testFile("T/b.qml"), options);
    }

    {
        const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });
        CallQmllintOptions options;
        options.resources.append(testFile("relPathQrc/resources.qrc"));

        callQmllint(testFile("relPathQrc/Foo/Thing.qml"), options);
    }
}

void TestQmllint::multiDirectory()
{
    CallQmllintOptions options;
    options.resources.append(testFile("MultiDirectory/multi.qrc"));

    callQmllint(testFile("MultiDirectory/qml/Inner.qml"), options);
    callQmllint(testFile("MultiDirectory/qml/pages/Page.qml"), options);
}

void TestQmllint::typeInstantiatedRecursively_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;

    QTest::newRow("withQmldirAlias")
            << u"qmldirs/renameSnippetToCustomName/Snippet.qml"_s
            << ResultBuilder::singleExpected("Type \"Snippet\" can't be instantiated recursively"_L1, 4, 5)
            << defaultOptions;

    constexpr QLatin1String warningForRecursiveMain =
            "Type \"%1\" can't be instantiated recursively"_L1;
    QTest::newRow("withMultipleQmldirAliases")
            << u"qmldirs/renameFileToMultipleNames/Main.qml"_s
            << ResultBuilder()
               .addExpected(warningForRecursiveMain.arg("Main"_L1), 4, 5)
               .addExpected(warningForRecursiveMain.arg("Name1"_L1), 5, 5)
               .addExpected(warningForRecursiveMain.arg("Name2"_L1), 6, 5)
               .addExpected(warningForRecursiveMain.arg("Name3"_L1), 7, 5)
               .addExpected(warningForRecursiveMain.arg("Name4"_L1), 8, 5)
               .build()
            << defaultOptions;
}

void TestQmllint::typeInstantiatedRecursively()
{
    m_linter.clearCache(); // note: clearing the cache in dirtyQmlCode() slows down the test suite considerably.
    dirtyQmlCode();
}

void TestQmllint::typeInstantiatedRecursivelyInBuildFolder()
{
    CallQmllintOptions options;
    options.resources.append(testFile("mymodule-build/.qt/rcc/qmake_app.qrc"));
    options.resources.append(testFile("mymodule-build/.qt/rcc/app_raw_qml_0.qrc"));

    checkResult(
            callQmllint(testFile("mymodule-source/MyModule/Main.qml"), options,
                        CallQmllintCheck::ShouldFail),
            ResultBuilder()
                .addExpected("Type \"Main\" can't be instantiated recursively"_L1, 4, 5)
                .addExpected("\"Main\" is explicitly renamed to \"NewName\" via a qmldir entry or "
                             "QT_QML_SOURCE_TYPENAME CMake property, use \"NewName\" instead."_L1, 4, 5)
                .addExpected("Type \"NewName\" can't be instantiated recursively"_L1, 5, 5)
                .build());
}

void TestQmllint::dirtyQmlCode_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;

    QTest::newRow("2Interceptors")
            << QStringLiteral("2interceptors.qml")
            << ResultBuilder::singleExpected("Duplicate interceptor on property \"x\""_L1)
            << defaultOptions;
    QTest::newRow("2ValueSources")
            << QStringLiteral("2valueSources.qml")
            << ResultBuilder::singleExpected("Duplicate value source on property \"x\""_L1)
            << defaultOptions;
    QTest::newRow("AssignToReadOnlyProperty")
            << QStringLiteral("assignToReadOnlyProperty.qml")
            << ResultBuilder::singleExpected("Cannot assign to read-only property activeFocus"_L1)
            << defaultOptions;
    QTest::newRow("AssignToReadOnlyProperty2")
            << QStringLiteral("assignToReadOnlyProperty2.qml")
            << ResultBuilder::singleExpected("Cannot assign to read-only property activeFocus"_L1)
            << defaultOptions;
    QTest::newRow("AutomatchedSignalHandler")
            << QStringLiteral("AutomatchedSignalHandler.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1, 12, 36)
               .addExpected("Implicitly defining \"onClicked\" as signal handler in Connections "
                            "is deprecated. Create a function instead: \"function onClicked() "
                            "{ ... }\""_L1)
               .build()
            << defaultOptions;
    QTest::newRow("BadAttached")
            << QStringLiteral("badAttached.qml")
            << ResultBuilder::singleExpected("unknown attached property scope WrongAttached."_L1)
            << defaultOptions;
    QTest::newRow("BadBinding")
            << QStringLiteral("badBinding.qml")
            << ResultBuilder::singleExpected("Could not find property \"doesNotExist\"."_L1)
            << defaultOptions;
    QTest::newRow("BadLiteralBinding")
            << QStringLiteral("badLiteralBinding.qml")
            << ResultBuilder::singleExpected("Cannot assign literal of type string to int"_L1)
            << defaultOptions;
    QTest::newRow("BadLiteralBindingDate")
            << QStringLiteral("badLiteralBindingDate.qml")
            << ResultBuilder::singleExpected("Cannot assign binding of type QString to QDateTime"_L1)
            << defaultOptions;
    QTest::newRow("BadModulePrefix")
            << QStringLiteral("badModulePrefix.qml")
            << ResultBuilder::singleExpected("Cannot access singleton as a property of an object"_L1)
            << defaultOptions;
    QTest::newRow("BadModulePrefix2")
            << QStringLiteral("badModulePrefix2.qml")
            << ResultBuilder()
               .addExpected("Cannot use non-QObject type QRectF to access prefixed import"_L1)
               .addUnexpected("Type not found in namespace"_L1)
               .addUnexpected("Member \"BirthdayParty\" not found on type \"QRectF\""_L1)
               .build()
            << defaultOptions;
    QTest::newRow("BadPropertyType")
            << QStringLiteral("badPropertyType.qml")
            << ResultBuilder::singleExpected("No type found for property \"bad\". This may be due "
                                             "to a missing import statement or incomplete qmltypes "
                                             "files."_L1)
            << defaultOptions;
    QTest::newRow("BadScriptBindingOnAttachedSignalHandler")
            << QStringLiteral("badScriptBinding.attachedSignalHandler.qml")
            << ResultBuilder::singleExpected("no matching signal found for handler \"onBogusSignal\""_L1, 3, 10)
            << defaultOptions;
    QTest::newRow("BadScriptBindingOnAttachedType")
            << QStringLiteral("badScriptBinding.attached.qml")
            << ResultBuilder::singleExpected("Could not find property \"bogusProperty\"."_L1, 5, 12)
            << defaultOptions;
    QTest::newRow("BadScriptBindingOnGroup")
            << QStringLiteral("badScriptBinding.group.qml")
            << ResultBuilder::singleExpected("Could not find property \"bogusProperty\"."_L1, 3, 10)
            << defaultOptions;
    QTest::newRow("CoerceToVoid")
            << QStringLiteral("coercetovoid.qml")
            << ResultBuilder::singleExpected("Function without return type annotation returns double"_L1)
            << defaultOptions;
    QTest::newRow("DefaultPropertyLookupInUnknownType")
            << QStringLiteral("unknownParentDefaultPropertyCheck.qml")
            << ResultBuilder::singleExpected("Alien was not found. Did you add all imports and dependencies?"_L1)
            << defaultOptions;
    QTest::newRow("DefaultPropertyWithWrongType(string)")
            << QStringLiteral("defaultPropertyWithWrongType.qml")
            << ResultBuilder()
               .addExpected("Cannot assign to default property of incompatible type"_L1)
               .addUnexpected("Cannot assign to non-existent default property"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("Deprecation (Property binding, no reason)")
            << QStringLiteral("deprecatedPropertyBinding.qml")
            << ResultBuilder::singleExpected("Binding on deprecated property \"deprecated\""_L1)
            << defaultOptions;
    QTest::newRow("Deprecation (Property binding, with reason)")
            << QStringLiteral("deprecatedPropertyBindingReason.qml")
            << ResultBuilder::singleExpected("Binding on deprecated property \"deprecatedReason\" (Reason: Test)"_L1)
            << defaultOptions;
    QTest::newRow("Deprecation (Property, no reason)")
            << QStringLiteral("deprecatedProperty.qml")
            << ResultBuilder::singleExpected("Property \"deprecated\" is deprecated"_L1)
            << defaultOptions;
    QTest::newRow("Deprecation (Property, with reason)")
            << QStringLiteral("deprecatedPropertyReason.qml")
            << ResultBuilder::singleExpected("Property \"deprecated\" is deprecated (Reason: Test)"_L1)
            << defaultOptions;
    QTest::newRow("Deprecation (Type, no reason)")
            << QStringLiteral("deprecatedType.qml")
            << ResultBuilder::singleExpected("Type \"TypeDeprecated\" is deprecated"_L1)
            << defaultOptions;
    QTest::newRow("Deprecation (Type, with reason)")
            << QStringLiteral("deprecatedTypeReason.qml")
            << ResultBuilder::singleExpected("Type \"TypeDeprecatedReason\" is deprecated (Reason: Test)"_L1)
            << defaultOptions;
    QTest::newRow("DoubleAssignToDefaultProperty")
            << QStringLiteral("defaultPropertyWithDoubleAssignment.qml")
            << ResultBuilder::singleExpected("Cannot assign multiple objects to a default non-list property"_L1)
            << defaultOptions;
    CallQmllintOptions withFileSelectorResourceFile = defaultOptions;
    withFileSelectorResourceFile.resources.append(testFile("FileSelector3/resources.qrc"));
    QTest::newRow("fileSelectorIncompatibleType")
            << QStringLiteral("FileSelector3/Bad.qml")
            << ResultBuilder()
               .addExpected("Type Bad has a potentially incompatible file-selected variant %1."_L1
                            .arg(testFile("FileSelector3/+Material/Bad.qml")), 3, 1)
               .addUnexpected("Ambiguous"_L1)
               .setFlag(Result::UseSettings)
               .build()
            << withFileSelectorResourceFile;
    QTest::newRow("fileSelectorIncompatibleType2")
            << QStringLiteral("FileSelector3/SubCircle.qml")
            << ResultBuilder()
               .addExpected("Type SubCircle is ambiguous due to file selector usage, ignoring %1."_L1
                            .arg(testFile("FileSelector3/+Material/SubCircle.qml")), 3, 1)
               .addUnexpected("potentially incompatible"_L1)
               .setFlag(Result::UseSettings)
               .build()
            << withFileSelectorResourceFile;
    QTest::newRow("fileSelectorIncompatibleFileSelectedType")
            << QStringLiteral("FileSelector3/+Material/Bad.qml")
            << ResultBuilder()
               .addExpected("File-selected type Bad is potentially incompatible with %1."_L1
                              .arg(testFile("FileSelector3/Bad.qml")), 3, 1)
               .build()
            << withFileSelectorResourceFile;
    QTest::newRow("fileSelectorIncompatibleFileSelectedType2")
            << QStringLiteral("FileSelector3/+Material/SubCircle.qml")
            << ResultBuilder()
               .addExpected("File-selected type SubCircle is ambiguous due to file selector usage, "
                            "this file will be ignored in favour of %1."_L1
                            .arg(testFile("FileSelector3/SubCircle.qml")), 3, 1)
               .addUnexpected("potentially incompatible"_L1)
               .setFlag(Result::UseSettings)
               .build()
            << withFileSelectorResourceFile;
    // This might be debatable: Maybe someone alwyas has 2 selectors at the same time
    // But for now, we assume that warning about it leads to a better trade-off.
    QTest::newRow("fileSelectorAccessingFileFromOtherSelector")
            << QStringLiteral("FileSelector5/+bar/Type.qml")
            << ResultBuilder::singleExpected("FooInternal was not found"_L1, 3, 1)
            << defaultOptions;
    QTest::newRow(("ImportModuleWithFileSelector"))
            << QStringLiteral("FileSelector/main.qml") << ResultBuilder::cleanResultWithSettings()
            << defaultOptions;

    QTest::newRow(("ImportFileSelector"))
            << QStringLiteral("FileSelector/ToolBar.qml")
            << ResultBuilder()
               .addExpected("Type ToolBar is ambiguous due to file selector usage, ignoring %1"_L1
                            .arg(testFile("FileSelector/+Material/ToolBar.qml")), 3, 1, QtInfoMsg)
               .setFlag(Result::UseSettings)
               .setFlag(Result::ExitsNormally)
               .build()
            << defaultOptions;
    QTest::newRow(("ImportFileSelector2ToolBar"))
            << QStringLiteral("FileSelector2/ToolBar.qml")
            << ResultBuilder()
               .addExpected("Type ToolBar is ambiguous due to file selector usage, ignoring %1"_L1
                            .arg(testFile("FileSelector2/+Material/ToolBar.qml")),
                            3, 1, QtMsgType::QtInfoMsg)
               .addUnexpected("Type ToolBar is ambiguous due to file selector usage, ignoring %1"_L1
                              .arg(testFile("FileSelector2/+Material/ToolBar.qml")),
                              3, 1, QtMsgType::QtWarningMsg)
               .addUnexpected("Item was not found."_L1)
               .addUnexpected("Broken"_L1)
               .setFlag(Result::UseSettings)
               .setFlag(Result::ExitsNormally)
               .build()
            << defaultOptions;
    QTest::newRow(("ImportFileSelector2Broken"))
            << QStringLiteral("FileSelector2/ToolBar.qml")
            << ResultBuilder()
               .addExpected("Type ToolBar is ambiguous due to file selector usage, ignoring %1"_L1
                            .arg(testFile("FileSelector2/+Material/ToolBar.qml")),
                            3, 1, QtMsgType::QtInfoMsg)
               .addUnexpected("Type ToolBar is ambiguous due to file selector usage, ignoring %1"_L1
                              .arg(testFile("FileSelector2/+Material/ToolBar.qml")),
                              3, 1, QtMsgType::QtWarningMsg)
               .addUnexpected("ToolBar"_L1)
               .setFlag(Result::UseSettings)
               .setFlag(Result::ExitsNormally)
               .build()
            << defaultOptions;
    QTest::newRow("InvalidImport")
            << QStringLiteral("invalidImport.qml")
            << ResultBuilder::singleExpected("Failed to import FooBar. Are your import paths set "
                                             "up properly?"_L1, 2, 1)
            << defaultOptions;
    QTest::newRow("Invalid_id_blockstatement")
            << QStringLiteral("invalidId2.qml")
            << ResultBuilder::singleExpected("id must be followed by an identifier"_L1)
            << defaultOptions;
    QTest::newRow("Invalid_id_expression")
            << QStringLiteral("invalidId1.qml")
            << ResultBuilder::singleExpected("Failed to parse id"_L1)
            << defaultOptions;
    QTest::newRow("Invalid_syntax_JS")
            << QStringLiteral("failure1.js")
            << ResultBuilder::singleExpected("Expected token `;'"_L1, 4, 12)
            << defaultOptions;
    QTest::newRow("Invalid_syntax_QML")
            << QStringLiteral("failure1.qml")
            << ResultBuilder::singleExpected("Expected token `:'"_L1, 4, 8)
            << defaultOptions;
    QTest::newRow("IsNotAnEntryOfEnum")
            << QStringLiteral("IsNotAnEntryOfEnum.qml")
            << ResultBuilder()
               .addExpected("Member \"Mode\" not found on type \"IsNotAnEntryOfEnum\""_L1, 12, 29)
               .addExpected("\"Hour\" is not an entry of enum \"Mode\"."_L1, 13, 62)
               .addFix("Did you mean \"mode\"?"_L1, Edit{ "mode"_L1, 12, 29 })
               .addFix("Did you mean \"Hours\"?"_L1, Edit{ "Hours"_L1, 13, 62 })
               .build()
            << defaultOptions;
    QTest::newRow("MemberNotFound")
            << QStringLiteral("memberNotFound.qml")
            << ResultBuilder::singleExpected("Member \"foo\" not found on type \"memberNotFound\""_L1, 6, 31)
            << defaultOptions;
    QTest::newRow("MissingDefaultProperty")
            << QStringLiteral("defaultPropertyWithoutKeyword.qml")
            << ResultBuilder::singleExpected("Cannot assign to non-existent default property"_L1)
            << defaultOptions;
    QTest::newRow("MissingDefaultPropertyDefinedInTheSameType")
            << QStringLiteral("defaultPropertyWithinTheSameType.qml")
            << ResultBuilder::singleExpected("Cannot assign to non-existent default property"_L1)
            << defaultOptions;
    QTest::newRow("MultiDefaultPropertyWithWrongType")
            << QStringLiteral("multiDefaultPropertyWithWrongType.qml")
            << ResultBuilder()
               .addExpected("Cannot assign to default property of incompatible type"_L1)
               .addUnexpected("Cannot assign to non-existent default property"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("NonExistentListProperty")
            << QStringLiteral("nonExistentListProperty.qml")
            << ResultBuilder::singleExpected("Could not find property \"objs\"."_L1)
            << defaultOptions;
    QTest::newRow("OnAssignment")
            << QStringLiteral("onAssignment.qml")
            << ResultBuilder::singleExpected("Member \"loops\" not found on type \"bool\""_L1)
            << defaultOptions;
    QTest::newRow("PropertyAliasCycles") << QStringLiteral("settings/propertyAliasCycle/file.qml")
                                         << ResultBuilder::cleanResultWithSettings() << defaultOptions;
    // make sure that warnings are triggered without settings:
    QTest::newRow("PropertyAliasCycles2")
            << QStringLiteral("settings/propertyAliasCycle/file.qml")
            << ResultBuilder()
               .addExpected("\"cycle1\" is part of an alias cycle"_L1)
               .addExpected("\"cycle1\" is part of an alias cycle"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("QtQuick.Window 2.0")
            << QStringLiteral("qtquickWindow20.qml")
            << ResultBuilder::singleExpected("Member \"window\" not found on type \"QQuickWindow\""_L1)
            << defaultOptions;
    QTest::newRow("SignalParameterMismatch")
            << QStringLiteral("namedSignalParameters.qml")
            << ResultBuilder()
               .addExpected("Parameter 1 to signal handler for \"onSig\" is called \"argarg\". "
                            "The signal has a parameter of the same name in position 2."_L1)
               .addUnexpected("onSig2"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("StoreNameMethod")
            << QStringLiteral("storeNameMethod.qml")
            << ResultBuilder::singleExpected("Cannot assign to method foo"_L1)
            << defaultOptions;
    QTest::newRow("TooManySignalParameters")
            << QStringLiteral("tooManySignalParameters.qml")
            << ResultBuilder::singleExpected("Signal handler for \"onSig\" has more formal "
                                             "parameters than the signal it handles."_L1)
            << defaultOptions;
    QTest::newRow("TypePropertAccess")
            << QStringLiteral("typePropertyAccess.qml")
            << ResultBuilder::ignoredResult() << defaultOptions;
    QTest::newRow("UnknownJavascriptMethd")
            << QStringLiteral("unknownJavascriptMethod.qml")
            << ResultBuilder::singleExpected("Member \"foo2\" not found on type \"Methods\""_L1, 5, 25)
            << defaultOptions;
    QTest::newRow("Unused Import (prefix)")
            << QStringLiteral("unused_prefix.qml")
            << ResultBuilder()
               .addExpected("Unused import"_L1, 1, 1, QtInfoMsg)
               .setFlag(Result::ExitsNormally)
               .build()
            << defaultOptions;
    QTest::newRow("Unused Import (simple)")
            << QStringLiteral("unused_simple.qml")
            << ResultBuilder()
               .addExpected("Unused import"_L1, 1, 1, QtInfoMsg)
               .setFlag(Result::ExitsNormally)
               .build()
            << defaultOptions;
    QTest::newRow("ValueSource+2Interceptors")
            << QStringLiteral("valueSourceBetween2interceptors.qml")
            << ResultBuilder::singleExpected("Duplicate interceptor on property \"x\""_L1)
            << defaultOptions;
    QTest::newRow("ValueSource+ListValue")
            << QStringLiteral("valueSource_listValue.qml")
            << ResultBuilder::singleExpected("Cannot combine value source and binding on property \"objs\""_L1)
            << defaultOptions;
    QTest::newRow("ValueSource+Value")
            << QStringLiteral("valueSource_Value.qml")
            << ResultBuilder::singleExpected("Cannot combine value source and binding on property \"obj\""_L1)
            << defaultOptions;
    QTest::newRow("VariableUsedBeforeDeclaration")
            << QStringLiteral("useBeforeDeclaration.qml")
            << ResultBuilder()
               .addExpected("Identifier 'argq' is used here before its declaration"_L1, 5, 9)
               .addExpected("Note: declaration of 'argq' here"_L1, 6, 13)
               .build()
            << defaultOptions;
    QTest::newRow("WithStatement")
            << QStringLiteral("WithStatement.qml")
            << ResultBuilder::singleExpected("with statements are strongly discouraged"_L1)
            << defaultOptions;
    QTest::newRow("aliasCycle1")
            << QStringLiteral("aliasCycle.qml")
            << ResultBuilder()
               .addExpected("Alias \"b\" is part of an alias cycle"_L1, 6, 5)
               .addExpected("Alias \"a\" is part of an alias cycle"_L1, 5, 5)
               .build()
            << defaultOptions;
    QTest::newRow("anchors3")
            << QStringLiteral("anchors3.qml")
            << ResultBuilder::singleExpected("Cannot assign binding of type QQuickItem to QQuickAnchorLine"_L1)
            << defaultOptions;
    QTest::newRow("annotatedDefaultParameter")
            << QStringLiteral("annotatedDefaultParameter.qml")
            << ResultBuilder::singleExpected("Type annotations on default parameters are not supported"_L1)
            << defaultOptions;
    QTest::newRow("assignNonExistingTypeToVarProp")
            << QStringLiteral("assignNonExistingTypeToVarProp.qml")
            << ResultBuilder::singleExpected("NonExistingType was not found. Did you add all "
                                             "imports and dependencies?"_L1)
            << defaultOptions;
    // should succeed, but it does not:
    QTest::newRow("attachedPropertyAccess") << QStringLiteral("goodAttachedPropertyAccess.qml")
                                            << ResultBuilder::cleanResult() << defaultOptions;
    // should succeed, but it does not:
    QTest::newRow("attachedPropertyNested") << QStringLiteral("goodAttachedPropertyNested.qml")
                                            << ResultBuilder::cleanResult() << defaultOptions;
    QTest::newRow("autoFixConnectionsBinding")
            << QStringLiteral("autofix/ConnectionsHandler.qml")
            << ResultBuilder()
               .addExpected("Implicitly defining \"onWidthChanged\" as signal handler in "
                            "Connections is deprecated. Create a function instead: \"function "
                            "onWidthChanged() { ... }\"."_L1)
               .addExpected("Implicitly defining \"onColorChanged\" as signal handler in "
                            "Connections is deprecated. Create a function instead: \"function "
                            "onColorChanged(collie) { ... }\"."_L1)
               .build()
            << defaultOptions;
    QTest::newRow("bad constant number to string")
            << QStringLiteral("numberToStringProperty.qml")
            << ResultBuilder::singleExpected("Cannot assign literal of type double to QString"_L1)
            << defaultOptions;
    QTest::newRow("bad string binding (QT_TR_NOOP)")
            << QStringLiteral("bad_QT_TR_NOOP.qml")
            << ResultBuilder::singleExpected("Cannot assign literal of type string to int"_L1)
            << defaultOptions;
    QTest::newRow("bad template literal (simple)")
            << QStringLiteral("badTemplateStringSimple.qml")
            << ResultBuilder::singleExpected("Cannot assign literal of type string to int"_L1)
            << defaultOptions;
    QTest::newRow("bad tranlsation binding (qsTr)")
            << QStringLiteral("bad_qsTr.qml") << ResultBuilder::ignoredResult() << defaultOptions;
    QTest::newRow("bad unary minus to string")
            << QStringLiteral("unaryMinusToStringProperty.qml")
            << ResultBuilder::singleExpected("Cannot assign literal of type double to QString"_L1)
            << defaultOptions;
    QTest::newRow("badAlias")
            << QStringLiteral("badAlias.qml")
            << ResultBuilder::singleExpected("Cannot resolve alias \"wrong\""_L1, 4, 5)
            << defaultOptions;
    QTest::newRow("badAliasExpression")
            << QStringLiteral("badAliasExpression.qml")
            << ResultBuilder::singleExpected("Invalid alias expression. Only IDs and field member "
                                             "expressions can be aliased"_L1, 5, 26)
            << defaultOptions;
    QTest::newRow("badAliasNotAnExpression")
            << QStringLiteral("badAliasNotAnExpression.qml")
            << ResultBuilder::singleExpected("Invalid alias expression. Only IDs and field member "
                                             "expressions can be aliased"_L1, 4, 30)
            << defaultOptions;
    QTest::newRow("badAliasObject")
            << QStringLiteral("badAliasObject.qml")
            << ResultBuilder::singleExpected("Member \"wrongwrongwrong\" not found on type "
                                             "\"QtObject\""_L1, 8, 40)
            << defaultOptions;
    QTest::newRow("badAliasProperty1")
            << QStringLiteral("badAliasProperty.qml")
            << ResultBuilder::singleExpected("Cannot resolve alias \"wrong\""_L1, 5, 5)
            << defaultOptions;
    QTest::newRow("badAttachedProperty")
            << QStringLiteral("badAttachedProperty.qml")
            << ResultBuilder::singleExpected("Member \"progress\" not found on type \"TestTypeAttached\""_L1)
            << defaultOptions;
    QTest::newRow("badAttachedPropertyNested")
            << QStringLiteral("badAttachedPropertyNested.qml")
            << ResultBuilder()
               .addExpected("Member \"progress\" not found on type \"QObject\""_L1, 12, 41)
               .addUnexpected("Member \"progress\" not found on type \"QObject\""_L1, 6, 37)
               .build()
            << defaultOptions;
    QTest::newRow("badAttachedPropertyTypeQtObject")
            << QStringLiteral("badAttachedPropertyTypeQtObject.qml")
            << ResultBuilder::singleExpected("Cannot assign object of type QtObject to int"_L1)
            << defaultOptions;
    QTest::newRow("badAttachedPropertyTypeString")
            << QStringLiteral("badAttachedPropertyTypeString.qml")
            << ResultBuilder::singleExpected("Cannot assign literal of type string to int"_L1)
            << defaultOptions;
    QTest::newRow("badEnumFromQtQml")
            << QStringLiteral("badEnumFromQtQml.qml")
            << ResultBuilder::singleExpected("Member \"Linear123\" not found on type \"QQmlEasing\""_L1, 4, 30)
            << defaultOptions;
    QTest::newRow("badGeneralizedGroup1")
            << QStringLiteral("badGeneralizedGroup1.qml")
            << ResultBuilder::singleExpected("Could not find property \"aaaa\"."_L1)
            << defaultOptions;
    QTest::newRow("badGeneralizedGroup2")
            << QStringLiteral("badGeneralizedGroup2.qml")
            << ResultBuilder::singleExpected("unknown grouped property scope aself"_L1)
            << defaultOptions;
    QTest::newRow("badParent")
            << QStringLiteral("badParent.qml")
            << ResultBuilder::singleExpected("Member \"rrr\" not found on type \"badParent\""_L1, 5, 34)
            << defaultOptions;
    QTest::newRow("badQmldirImportAndDepend")
            << QStringLiteral("qmldirImportAndDepend/bad.qml")
            << ResultBuilder::singleExpected("Item was not found. Did you add all imports and "
                                             "dependencies?"_L1, 3, 1)
            << defaultOptions;
    QTest::newRow("badScript")
            << QStringLiteral("badScript.qml")
            << ResultBuilder::singleExpected("Member \"stuff\" not found on type \"Empty\""_L1, 5, 21)
            << defaultOptions;
    QTest::newRow("badScriptOnAttachedProperty")
            << QStringLiteral("badScript.attached.qml")
            << ResultBuilder::singleExpected("Unqualified access"_L1, 3, 26)
            << defaultOptions;
    QTest::newRow("badTypeAssertion")
            << QStringLiteral("badTypeAssertion.qml")
            << ResultBuilder::singleExpected("Member \"rrr\" not found on type \"QQuickItem\""_L1, 5, 39)
            << defaultOptions;
    QTest::newRow("badlyBoundComponents")
            << QStringLiteral("badlyBoundComponents.qml")
            << ResultBuilder::singleExpected("Unqualified access"_L1, 18, 36)
            << defaultOptions;
    QTest::newRow("brokenNamespace")
            << QStringLiteral("brokenNamespace.qml")
            << ResultBuilder::singleExpected("Type not found in namespace"_L1, 4, 19)
            << defaultOptions;
    QTest::newRow("cachedDependency")
            << QStringLiteral("cachedDependency.qml")
            << ResultBuilder()
               .addExpected("Unused import"_L1, 1, 1, QtInfoMsg)
               .addUnexpected("Cannot assign binding of type QQuickItem to QObject"_L1)
               .setFlag(Result::ExitsNormally)
               .build()
            << defaultOptions;
    QTest::newRow("componentDefinitionInnerRequiredProperty")
            << u"componentDefinitionInnerRequiredProperty.qml"_s
            << ResultBuilder::singleExpected("Component is missing required property bar from "
                                             "Rectangle"_L1, 11, 13)
            << defaultOptions;
    QTest::newRow("cppPropertyChangeHandlers-no-property")
            << QStringLiteral("badCppPropertyChangeHandlers3.qml")
            << ResultBuilder::singleExpected("no matching signal found for handler \"onXChanged\""_L1)
            << defaultOptions;
    QTest::newRow("cppPropertyChangeHandlers-not-a-signal")
            << QStringLiteral("badCppPropertyChangeHandlers4.qml")
            << ResultBuilder::singleExpected("no matching signal found for handler \"onWannabeSignal\""_L1)
            << defaultOptions;
    QTest::newRow("cppPropertyChangeHandlers-wrong-parameters-size-bindable")
            << QStringLiteral("badCppPropertyChangeHandlers1.qml")
            << ResultBuilder::singleExpected("Signal handler for \"onAChanged\" has more formal "
                                             "parameters than the signal it handles"_L1)
            << defaultOptions;
    QTest::newRow("cppPropertyChangeHandlers-wrong-parameters-size-notify")
            << QStringLiteral("badCppPropertyChangeHandlers2.qml")
            << ResultBuilder::singleExpected("Signal handler for \"onBChanged\" has more formal "
                                             "parameters than the signal it handles"_L1)
            << defaultOptions;
    QTest::newRow("cycle in import")
            << QStringLiteral("cycleHead.qml")
            << ResultBuilder::singleExpected("MenuItem is part of an inheritance cycle: "
                                             "MenuItem -> MenuItem"_L1)
            << defaultOptions;
    QTest::newRow("deprecatedFunction")
            << QStringLiteral("deprecatedFunction.qml")
            << ResultBuilder::singleExpected("Method \"deprecated(foobar)\" is deprecated "
                                             "(Reason: No particular reason.)"_L1)
            << defaultOptions;
    QTest::newRow("deprecatedFunctionInherited")
            << QStringLiteral("deprecatedFunctionInherited.qml")
            << ResultBuilder::singleExpected("Method \"deprecatedInherited(c, d)\" is deprecated "
                                             "(Reason: This deprecation should be visible!)"_L1)
            << defaultOptions;
    QTest::newRow("didYouMean(binding)")
            << QStringLiteral("didYouMeanBinding.qml")
            << ResultBuilder()
               .addExpected("Could not find property \"witdh\"."_L1)
               .addFix("Did you mean \"width\"?"_L1, Edit{ "width"_L1 })
               .build()
            << defaultOptions;
    QTest::newRow("didYouMean(component)")
            << QStringLiteral("didYouMeanComponent.qml")
            << ResultBuilder()
               .addExpected("Itym was not found. Did you add all imports and dependencies?"_L1)
               .addFix("Item"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("didYouMean(enum)")
            << QStringLiteral("didYouMeanEnum.qml")
            << ResultBuilder()
               .addExpected("Member \"Readx\" not found on type \"QQuickImage\""_L1)
               .addFix("Ready"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("didYouMean(property)")
            << QStringLiteral("didYouMeanProperty.qml")
            << ResultBuilder()
               .addExpected("Member \"hoight\" not found on type \"Rectangle\""_L1)
               .addFix("height"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("didYouMean(propertyCall)")
            << QStringLiteral("didYouMeanPropertyCall.qml")
            << ResultBuilder()
               .addExpected("Member \"lgg\" not found on type \"Console\""_L1)
               .addFix("log"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("didYouMean(unqualified)")
            << QStringLiteral("didYouMeanUnqualified.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1)
               .addFix("Did you mean \"height\"?"_L1, Edit{ "height"_L1 })
               .build()
            << defaultOptions;
    QTest::newRow("didYouMean(unqualifiedCall)")
            << QStringLiteral("didYouMeanUnqualifiedCall.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1)
               .addFix("Did you mean \"func\""_L1, Edit{ "func"_L1 })
               .build()
            << defaultOptions;
    QTest::newRow("duplicateImportsDirty")
            << QStringLiteral("duplicateImportsDirty.qml")
            << ResultBuilder()
               .addExpected("Duplicate import 'QtQml'"_L1, 2, 8)
               .addExpected("Note: previous import 'QtQml' here"_L1, 1, 8)
               .addExpected("Duplicate import 'Truc'"_L1, 6, 8)
               .addExpected("Note: previous import 'Truc' here", 5, 8)
               .addExpected("Duplicate import 'QtQuick.Controls'"_L1, 9, 8)
               .addExpected("Note: previous import 'QtQuick.Controls' here"_L1, 8, 8)
               .addExpected("Duplicate import '..'"_L1, 12, 8)
               .addExpected("Note: previous import '..' here"_L1, 11, 8)
               .addUnexpected("Duplicate import 'QtQml'"_L1, 3, 8)
               .build()
            << defaultOptions;
    QTest::newRow("duplicated id")
            << QStringLiteral("duplicateId.qml")
            << ResultBuilder::singleExpected("Found a duplicated id. id root was first declared "_L1,
                                             0, 0, QtCriticalMsg)
            << defaultOptions;
    QTest::newRow("duplicatedPropertyName")
            << QStringLiteral("duplicatedPropertyName.qml")
            << ResultBuilder()
               .addExpected("Duplicated property name \"cat\", \"cat\" is already a property."_L1, 5, 21)
               .addExpected("Duplicated signal name \"clicked\", \"clicked\" is already a signal"_L1, 8, 12)
               .build()
            << defaultOptions;
    QTest::newRow("enumInvalid")
            << QStringLiteral("enumInvalid.qml")
            << ResultBuilder()
               .addExpected("Member \"red\" not found on type \"QtObject\""_L1, 5, 25)
               .addExpected("Member \"red\" not found on type \"QtObject\""_L1, 6, 25)
               .addExpected("Member \"S2\" not found on type \"EnumTesterScoped\""_L1, 8, 38)
               .addFix("Did you mean \"U2\"?"_L1, Edit{ "U2"_L1, 8, 38 })
               .build()
            << defaultOptions;
    QTest::newRow("enumsAreNotTypes_functionAnnotations")
            << QStringLiteral("EnumsAreNotTypes_functionAnnotations.qml")
            << ResultBuilder()
               .addExpected("QML enumerations are not types. Use int, or use double if the enum's "
                            "underlying type does not fit into int."_L1, 5, 17)
               .addExpected("QML enumerations are not types. Use int, or use double if the enum's "
                            "underlying type does not fit into int."_L1, 6, 9)
               .build()
            << defaultOptions;
    QTest::newRow("id_in_value_type")
            << QStringLiteral("idInValueType.qml")
            << ResultBuilder::singleExpected("id declarations are only allowed in objects"_L1)
            << defaultOptions;
    QTest::newRow("inaccessibleId")
            << QStringLiteral("inaccessibleId.qml")
            << ResultBuilder::singleExpected("Member \"objectName\" not found on type \"int\""_L1)
            << defaultOptions;
    QTest::newRow("inaccessibleId2")
            << QStringLiteral("inaccessibleId2.qml")
            << ResultBuilder::singleExpected("Member \"objectName\" not found on type \"int\""_L1)
            << defaultOptions;
    QTest::newRow("incompleteQmltypes")
            << QStringLiteral("incompleteQmltypes.qml")
            << ResultBuilder::singleExpected("Type \"QPalette\" of property \"palette\" not found"_L1, 5, 26)
            << defaultOptions;
    QTest::newRow("incompleteQmltypes2")
            << QStringLiteral("incompleteQmltypes2.qml")
            << ResultBuilder::singleExpected("Type CustomPalette is used but it is not resolved"_L1, 5, 35)
            << defaultOptions;
    QTest::newRow("incompleteQmltypes3")
            << QStringLiteral("incompleteQmltypes3.qml")
            << ResultBuilder::singleExpected("Type \"QPalette\" of property \"palette\" not found"_L1, 5, 21)
            << defaultOptions;
    QTest::newRow("inheritanceCycle")
            << QStringLiteral("Cycle1.qml")
            << ResultBuilder::singleExpected("Cycle1 is part of an inheritance cycle: Cycle1 -> "
                                             "Cycle2 -> Cycle3 -> Cycle1"_L1, 2, 1)
            << defaultOptions;
    QTest::newRow("inlineComponentNoComponent")
            << QStringLiteral("inlineComponentNoComponent.qml")
            << ResultBuilder::singleExpected("Inline component declaration must be followed by a "
                                             "typename"_L1, 3, 2)
            << defaultOptions;
    QTest::newRow("inlineComponentSearchInfiniteLoop")
            << QStringLiteral("InlineComponentSearchInfiniteLoop_Main.qml")
            << ResultBuilder::singleExpected("InlineComponentSearchInfiniteLoop_Other.a was not "
                                             "found. Did you add all imports and dependencies?"_L1,
                                             5, 5)
            << defaultOptions;
    QTest::newRow("invalidAliasTarget1")
            << QStringLiteral("invalidAliasTarget.qml")
            << ResultBuilder()
               .addExpected("Invalid alias expression - an initializer is needed."_L1, 6, 18)
               .addExpected("Invalid alias expression. Only IDs and field member expressions can "
                            "be aliased"_L1, 7, 30)
               .addExpected("Invalid alias expression. Only IDs and field member expressions can be "
                            "aliased"_L1, 9, 34)
               .build()
            << defaultOptions;
    QTest::newRow("invalidInterceptor")
            << QStringLiteral("invalidInterceptor.qml")
            << ResultBuilder::singleExpected("On-binding for property \"angle\" has wrong "
                                             "type \"Item\""_L1)
            << defaultOptions;
    QTest::newRow("javascriptMethodsInModule")
            << QStringLiteral("javascriptMethodsInModuleBad.qml")
            << ResultBuilder::singleExpected("Member \"unknownFunc\" not found on type \"Foo\""_L1, 5, 21)
            << defaultOptions;
    QTest::newRow("jsVarDeclarationsWriteConst")
            << QStringLiteral("jsVarDeclarationsWriteConst.qml")
            << ResultBuilder::singleExpected("Cannot assign to read-only property constProp"_L1)
            << defaultOptions;
    QTest::newRow("lintInnerFunctionsToo")
            << QStringLiteral("lintInnerFunctionsToo.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1, 5, 17)
               .addExpected("Unqualified access"_L1, 6, 23)
               .addExpected("Unqualified access"_L1, 7, 25)
               .addExpected("Unqualified access"_L1, 8, 26)
               .addExpected("Unqualified access"_L1, 11, 17)
               .addExpected("Unqualified access"_L1, 16, 30)
               .addExpected("Unqualified access"_L1, 17, 38)
               .addExpected("Unqualified access"_L1, 18, 35)
               .addExpected("Unqualified access"_L1, 19, 51)
               .build()
            << defaultOptions;
    QTest::newRow("lowerCaseQualifiedImport")
            << QStringLiteral("lowerCaseQualifiedImport.qml")
            << ResultBuilder()
               .addExpected("Import qualifier 'test' must start with a capital letter."_L1)
               .addExpected("Namespace 'test' of 'test.Rectangle' must start with an upper case "
                            "letter."_L1)
               .build()
            << defaultOptions;
    QTest::newRow("lowerCaseQualifiedImport2")
            << QStringLiteral("lowerCaseQualifiedImport2.qml")
            << ResultBuilder()
               .addExpected("Import qualifier 'test' must start with a capital letter."_L1)
               .addExpected("Namespace 'test' of 'test.Item' must start with an upper case letter."_L1)
               .addExpected("Namespace 'test' of 'test.Rectangle' must start with an upper case letter."_L1)
               .addExpected("Namespace 'test' of 'test.color' must start with an upper case letter."_L1)
               .addExpected("Namespace 'test' of 'test.Grid' must start with an upper case letter."_L1)
               .build()
            << defaultOptions;
    QTest::newRow("missingComponentBehaviorBound")
            << QStringLiteral("missingComponentBehaviorBound.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1, 8, 31)
               .addFix("Set \"pragma ComponentBehavior: Bound\" in order to use IDs from "
                       "outer components in nested components."_L1,
                       Edit{ "pragma ComponentBehavior: Bound\n"_L1, 0, 0 })
               .setFlag(Result::AutoFixable)
               .build()
            << defaultOptions;
    QTest::newRow("missingQmltypes")
            << QStringLiteral("missingQmltypes.qml")
            << ResultBuilder::singleExpected("QML types file does not exist"_L1)
            << defaultOptions;
    QTest::newRow("missingRequiredAlias")
            << QStringLiteral("missingRequiredAlias.qml")
            << ResultBuilder::singleExpected("Component is missing required property foo from Item"_L1)
            << defaultOptions;
    QTest::newRow("missingRequiredOnObjectDefinitionBinding")
            << QStringLiteral("missingRequiredPropertyOnObjectDefinitionBinding.qml")
            << ResultBuilder::singleExpected(
                   uR"(Component is missing required property i from QtObject)"_s, 4, 26)
            << defaultOptions;
    QTest::newRow("missingSingletonPragma")
            << QStringLiteral("Singletons/MissingPragma.qml")
            << ResultBuilder::singleExpected("Type MissingPragma declared as singleton in qmldir "
                                             "but missing pragma Singleton"_L1)
            << defaultOptions;
    QTest::newRow("missingSingletonQmldir")
            << QStringLiteral("Singletons/MissingQmldirSingleton.qml")
            << ResultBuilder::singleExpected("Type MissingQmldirSingleton not declared as "
                                             "singleton in qmldir but using pragma Singleton"_L1)
            << defaultOptions;
    QTest::addRow("multifix")
            << QStringLiteral("multifix.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1, 7,  19, QtWarningMsg)
               .addExpected("Unqualified access"_L1, 11,  19, QtWarningMsg)
               .addFix("Set \"pragma ComponentBehavior: Bound\" in order to use IDs from "
                       "outer components in nested components."_L1,
                       Edit{ "pragma ComponentBehavior: Bound\n"_L1, 1, 1 })
               .setFlag(Result::AutoFixable)
               .build()
            << defaultOptions;
    QTest::newRow("multilineString")
            << QStringLiteral("multilineString.qml")
            << ResultBuilder()
               .addExpected("String contains unescaped line terminator which is deprecated."_L1,
                            0, 0, QtInfoMsg)
               .addFix("Use a template literal instead."_L1,
                       Edit{ "`Foo\nmultiline\\`\nstring`"_L1, 4, 32 })
               .addFix("Use a template literal instead."_L1,
                       Edit{ "`another\\`\npart\nof it`"_L1, 6, 11 })
               .addFix("Use a template literal instead."_L1,
                       Edit{ R"(`
quote: " \\" \\\\"
ticks: \` \` \\\` \\\`
singleTicks: ' \' \\' \\\'
expression: \${expr} \${expr} \\\${expr} \\\${expr}`)"_L1,
                             10, 28 })
               .addFix("Use a template literal instead."_L1,
                       Edit{ R"(`
quote: " \" \\" \\\"
ticks: \` \` \\\` \\\`
singleTicks: ' \\' \\\\'
expression: \${expr} \${expr} \\\${expr} \\\${expr}`)"_L1,
                             16, 27 })
               .setFlag(Result::ExitsNormally)
               .setFlag(Result::AutoFixable)
               .build()
            << defaultOptions;

    // The warning should show up only once even though
    // we have to run the type propagator multiple times.
    QTest::newRow("multiplePasses")
            << testFile("multiplePasses.qml")
            << ResultBuilder::singleExpected("Unqualified access"_L1)
            << defaultOptions;
    QTest::newRow("nanchors1")
            << QStringLiteral("nanchors1.qml")
            << ResultBuilder::singleExpected("unknown grouped property scope nanchors."_L1)
            << defaultOptions;
    QTest::newRow("nanchors2")
            << QStringLiteral("nanchors2.qml")
            << ResultBuilder::singleExpected("unknown grouped property scope nanchors."_L1)
            << defaultOptions;
    QTest::newRow("nanchors3")
            << QStringLiteral("nanchors3.qml")
            << ResultBuilder::singleExpected("unknown grouped property scope nanchors."_L1)
            << defaultOptions;
    QTest::newRow("nestedInlineComponents")
            << QStringLiteral("nestedInlineComponents.qml")
            << ResultBuilder::singleExpected("Nested inline components are not supported"_L1)
            << defaultOptions;
    QTest::newRow("nonNullStored")
            << QStringLiteral("nonNullStored.qml")
            << ResultBuilder()
               .addExpected("Type Foozle is used but it is not resolved"_L1)
               .addUnexpected("Unqualified access"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("notQmlRootMethods")
            << QStringLiteral("notQmlRootMethods.qml")
            << ResultBuilder()
               .addExpected("Member \"deleteLater\" not found on type \"notQmlRootMethods\""_L1)
               .addExpected("Member \"destroyed\" not found on type \"notQmlRootMethods\""_L1)
               .build()
            << defaultOptions;
    QTest::newRow("nullBinding")
            << QStringLiteral("nullBinding.qml")
            << ResultBuilder::singleExpected("Cannot assign literal of type null to double"_L1)
            << defaultOptions;
    QTest::newRow("parentIsComponent")
            << QStringLiteral("parentIsComponent.qml")
            << ResultBuilder::singleExpected(
                   "Member \"progress\" not found on type \"QQuickItem\""_L1, 7, 39)
            << defaultOptions;
    QTest::newRow("redundantOptionalChainingEnums")
            << QStringLiteral("RedundantOptionalChainingEnums.qml")
            << ResultBuilder()
               .addExpected("Redundant optional chaining for enum lookup"_L1, 5, 54)
               .addExpected("Redundant optional chaining for enum lookup"_L1, 6, 26)
               .build()
            << defaultOptions;
    QTest::newRow("renamedType")
            << u"qmldirs/renameSnippetToCustomName/UseOldName.qml"_s
            << ResultBuilder()
               .addExpected("\"UseOldName\" is explicitly renamed to \"NewName\" via a qmldir "
                            "entry or QT_QML_SOURCE_TYPENAME CMake property, use \"NewName\" "
                            "instead."_L1, 4, 14)
               .addExpected("\"UseOldName\" is explicitly renamed to \"NewName\" via a qmldir "
                            "entry or QT_QML_SOURCE_TYPENAME CMake property, use \"NewName\" "
                            "instead."_L1, 5, 19)
               .addExpected("\"UseOldName\" is explicitly renamed to \"NewName\" via a qmldir "
                            "entry or QT_QML_SOURCE_TYPENAME CMake property, use \"NewName\" "
                            "instead."_L1, 5, 32)
               .build()
            << defaultOptions;
    constexpr QLatin1String renameTypeWarning =
            "\"Main\" is explicitly renamed to \"Name1\", \"Name2\", \"Name3\", \"Name4\" via a qmldir entry or QT_QML_SOURCE_TYPENAME CMake property, use \"Name1\", \"Name2\", \"Name3\", \"Name4\" instead."_L1;
    QTest::newRow("renamedTypeUsage")
            << u"qmldirs/renameFileToMultipleNames/AnotherFile.qml"_s
            << ResultBuilder()
               .addExpected(renameTypeWarning, 4, 5)
               .addExpected(renameTypeWarning, 10, 26)
               .addExpected(renameTypeWarning, 16, 19)
               .addExpected(renameTypeWarning, 16, 38)
               .build()
            << defaultOptions;
    QTest::newRow("segFault (bad)")
            << QStringLiteral("SegFault.bad.qml")
            << ResultBuilder::singleExpected(
                   "Member \"foobar\" not found on type \"QQuickScreenAttached\""_L1)
            << defaultOptions;
    {
       constexpr QLatin1String msg = "Reading non-constant and non-notifiable property %1. Binding "
                                     "might not update when the property changes."_L1;
        QTest::newRow("stalePropertyRead")
                << QStringLiteral("StalePropertyRead.qml")
                << ResultBuilder()
                   .addExpected(msg.arg("cppStale"_L1), 10, 24)
                   .addExpected(msg.arg("cppReadonly"_L1), 11, 24)
                   .addUnexpected(msg.arg("cppConstant"_L1), 14, 24)
                   .addUnexpected(msg.arg("cppNotifiable"_L1), 15, 24)
                   .addUnexpected(msg.arg("cppConstantNotifiable"_L1), 16, 24)
                   .addUnexpected(msg.arg("i"_L1), 17, 24)
                   .addUnexpected(msg.arg("ro"_L1), 18, 24)
                   .build()
                << defaultOptions;
    }
    QTest::newRow("string as id")
            << QStringLiteral("stringAsId.qml")
            << ResultBuilder::singleExpected("ids do not need quotation marks"_L1)
            << defaultOptions;
    QTest::newRow("stringIdUsedInWarning")
            << QStringLiteral("stringIdUsedInWarning.qml")
            << ResultBuilder()
               .addExpected("i is a member of a parent element"_L1)
               .addFix("i is a member of a parent element.\n      "
                       "You can qualify the access with its id to avoid this warning."_L1,
                       Edit{ "stringy."_L1 })
               .build()
            << defaultOptions;
    QTest::newRow("unboundComponents")
            << QStringLiteral("unboundComponents.qml")
            << ResultBuilder()
               .addExpected("Unqualified access"_L1, 10, 25)
               .addExpected("Unqualified access"_L1, 14, 33)
               .build()
            << defaultOptions;
    QTest::newRow("unknownTypeCustomParser")
            << QStringLiteral("unknownTypeCustomParser.qml")
            << ResultBuilder::singleExpected("TypeDoesNotExist was not found."_L1)
            << defaultOptions;
    QTest::newRow("unresolvedArrayBinding")
            << QStringLiteral("unresolvedArrayBinding.qml")
            << ResultBuilder::singleExpected(
                   "Declaring an object which is not a Qml object as a list member."_L1)
            << defaultOptions;
    QTest::newRow("unresolvedAttachedType")
            << QStringLiteral("unresolvedAttachedType.qml")
            << ResultBuilder()
               .addExpected("unknown attached property scope UnresolvedAttachedType."_L1)
               .addUnexpected("Could not find property \"property\"."_L1)
               .build()
            << defaultOptions;
    QTest::newRow("unresolvedType")
            << QStringLiteral("unresolvedType.qml")
            << ResultBuilder()
               .addExpected("UnresolvedType was not found. Did you add all imports and dependencies"_L1)
               .addUnexpected("incompatible type"_L1)
               .build()
            << defaultOptions;
    // We want to see the warning about the missing type only once.
    QTest::newRow("unresolvedType2")
            << QStringLiteral("unresolvedType2.qml")
            << ResultBuilder()
               .addExpected("QQC2.Label was not found. Did you add all imports and dependencies?"_L1)
               .addUnexpected("'QQC2.Label' is used but it is not resolved"_L1)
               .addUnexpected("Type QQC2.Label is used but it is not resolved"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("unresolvedTypeAnnotation")
            << QStringLiteral("unresolvedTypeAnnotations.qml")
            << ResultBuilder()
               .addExpected(R"("A" was not found for the type of parameter "a" in method "f".)"_L1, 4, 17)
               .addExpected(R"("B" was not found for the type of parameter "b" in method "f".)"_L1, 4, 23)
               .addExpected(R"("R" was not found for the return type of method "g".)"_L1, 5, 18)
               .addExpected(R"("C" was not found for the type of parameter "c" in method "h".)"_L1, 6, 17)
               .addExpected(R"("R" was not found for the return type of method "h".)"_L1, 6, 22)
               .addExpected(R"("D" was not found for the type of parameter "d" in method "i".)"_L1, 7, 17)
               .addExpected(R"("G" was not found for the type of parameter "g" in method "i".)"_L1, 7, 26)
               .build()
            << defaultOptions;

    QTest::newRow("jsdeclInQmlScope")
            << QStringLiteral("jsdeclInQmlScope.qml")
            << ResultBuilder::singleExpected(
                   "JavaScript declarations are not allowed in QML elements"_L1, 4, 13)
            << defaultOptions;

    QTest::newRow("contextPropertiesFromUser")
            << u"ContextProperties/qml/MyUserContextProperties.qml"_s
            << ResultBuilder()
               .addExpected("Potential context property access detected. Context properties are "
                            "discouraged in QML: use normal, required, or singleton properties "
                            "instead.\nNote: 'myUserCP1' assumed to be a potential context "
                            "property because it is not declared as required property."_L1,
                            7, 22)
               .addExpected("Potential context property access detected. Context properties are "
                            "discouraged in QML: use normal, required, or singleton properties "
                            "instead.\nNote: 'myUserCP2' assumed to be a potential context "
                            "property because it is not declared as required property."_L1,
                            8, 22)
               .addUnexpected("Unqualified access"_L1)
               .build()
            << defaultOptions;
}

void TestQmllint::dirtyQmlCode()
{
    QFETCH(QString, filename);
    QFETCH(Result, result);
    QFETCH(CallQmllintOptions, options);

    QEXPECT_FAIL("attachedPropertyAccess", "We cannot discern between types and instances", Abort);
    QEXPECT_FAIL("attachedPropertyNested", "We cannot discern between types and instances", Abort);
    QEXPECT_FAIL("BadLiteralBindingDate",
                 "We're currently not able to verify any non-trivial QString conversion that "
                 "requires QQmlStringConverters",
                 Abort);
    QEXPECT_FAIL("bad tranlsation binding (qsTr)", "We currently do not check translation binding",
                 Abort);

    QEXPECT_FAIL("BadTranslationMixWithMacros",
                 "Translation macros are currently invisible to QQmlJSTypePropagator", Abort);

    options.readSettings = result.flags.testFlag(Result::UseSettings);
    const QJsonArray warnings = callQmllint(filename, options, fromResultFlags(result.flags));

    checkResult(
            warnings, result,
            [] {
                QEXPECT_FAIL("BadLiteralBindingDate",
                             "We're currently not able to verify any non-trivial QString "
                             "conversion that "
                             "requires QQmlStringConverters",
                             Abort);
                QEXPECT_FAIL("BadTranslationMixWithMacros",
                             "Translation macros are currently invisible to QQmlJSTypePropagator",
                             Abort);
            },
            [] {
                QEXPECT_FAIL("badAttachedPropertyNested",
                             "We cannot discern between types and instances", Abort);
            },
            [] {});
}

static void addLocationOffsetTo(TestQmllint::Result *result, qsizetype lineOffset,
                                qsizetype columnOffset = 0)
{
    auto adjustLineAndColumn = [lineOffset, columnOffset](auto &lineAndColumnHolder) {
        if (lineAndColumnHolder.line != 0)
            lineAndColumnHolder.line += lineOffset;
        if (lineAndColumnHolder.column != 0)
            lineAndColumnHolder.column += columnOffset;
    };

    for (auto &expectedMessage : result->expectedMessages)
        adjustLineAndColumn(expectedMessage);
    for (auto &badMessage : result->unexpectedMessages)
        adjustLineAndColumn(badMessage);
    for (auto &expectedFix : result->expectedFixes) {
        adjustLineAndColumn(expectedFix);
        for (auto &edit : expectedFix.edits)
            adjustLineAndColumn(edit);
    }
}

void TestQmllint::dirtyQmlSnippet_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<Result>("result");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;

    QTest::newRow("assignLhsLocation")
            << u"id: root; property int i; Item { Component.onCompleted: i = root.i + 5 }"_s
            << ResultBuilder()
               .addExpected("Unqualified access"_L1, 1, 57)
               .addUnexpected("Unqualified access"_L1, 1, 66)
               .build()
            << defaultOptions;
    QTest::newRow("assignToMissingProperty")
            << u"Rectangle { id: foo } function f() { foo.bar = 42; }"_s
            << ResultBuilder().addExpected("Member \"bar\" not found on type"_L1, 1, 38).build()
            << defaultOptions;
    CallQmllintOptions withQuickPlugin;
    withQuickPlugin.plugins << "Quick"_L1;
    QTest::newRow("color-hex")
            << u"property color myColor: \"#12345\""_s
            << ResultBuilder::singleExpected("Invalid color"_L1, 1, 25)
            << withQuickPlugin;
    QTest::newRow("color-hex2")
            << u"property color myColor: \"#123456789\""_s
            << ResultBuilder::singleExpected("Invalid color"_L1, 1, 25)
            << withQuickPlugin;
    QTest::newRow("color-hex3")
            << u"property color myColor: \"##123456\""_s
            << ResultBuilder::singleExpected("Invalid color"_L1, 1, 25)
            << withQuickPlugin;
    QTest::newRow("color-hex4")
            << u"property color myColor: \"#123456#\""_s
            << ResultBuilder::singleExpected("Invalid color"_L1, 1, 25)
            << withQuickPlugin;
    QTest::newRow("color-hex5")
            << u"property color myColor: \"#HELLOL\""_s
            << ResultBuilder::singleExpected("Invalid color"_L1, 1, 25)
            << withQuickPlugin;
    QTest::newRow("color-hex6")
            << u"property color myColor: \"#1234567\""_s
            << ResultBuilder::singleExpected("Invalid color"_L1, 1, 25)
            << withQuickPlugin;
    QTest::newRow("color-name")
            << u"property color myColor: \"lbue\""_s
            << ResultBuilder()
               .addExpected("Invalid color \"lbue\""_L1, 1, 25)
               .addFix("Did you mean \"blue\"?", Edit{ "blue"_L1, 1, 25 })
               .build()
            << withQuickPlugin;
    QTest::newRow("componentExactlyOneChild1")
            << u"Component { Item {} Item {} }"_s
            << ResultBuilder::singleExpected("Components must have exactly one child"_L1, 1, 1)
            << defaultOptions;
    QTest::newRow("componentExactlyOneChild2")
            << u"Component {}"_s
            << ResultBuilder::singleExpected("Components must have exactly one child"_L1, 1, 1)
            << defaultOptions;
    {
        CallQmllintOptions options;
        options.rootUrls.append(testFile("ContextProperties/src"_L1));

        static constexpr QLatin1StringView warningHead =
                "Potential context property access detected. Context properties are discouraged in "
                "QML: use normal, required, or singleton properties instead.\n"_L1;

        static constexpr QLatin1StringView warningRequiredPropertyBit =
                "Note: '%1' assumed to be a potential context property because it "
                "is not declared as required property.\n"_L1;

        const QString cppFile = testFile("ContextProperties/src/main.cpp");
        const QString cppFileInSubfolder =
                testFile("ContextProperties/src/Sub/Sub2/Sub3/register.cpp");

        const QString warning =
                QString(warningHead)
                        .append("Note: candidate context property declaration '%1' at %2:%3:%4"_L1);
        const QString myContextProperty1Warning = warning.arg(
                "myContextProperty1"_L1, cppFile, QString::number(14), QString::number(45));
        const QString myContextProperty1WarningWithRequiredProperty =
                QString(warningHead)
                        .append(warningRequiredPropertyBit)
                        .append("Note: candidate context property declaration '%1' at %5:%6:%7"_L1)
                        .arg("myContextProperty1"_L1, cppFile, QString::number(14),
                             QString::number(45));

        const QString unqualifiedAccessWarning = "Unqualified access"_L1;

        QTest::newRow("contextProperties")
                << uR"(required property int myRequired
                       property var a: myContextProperty1
                       property var b: myContextProperty2
                       property var c: myContextProperty3)"_s
                << ResultBuilder()
                   .addExpected(myContextProperty1Warning, 2, 40)
                   .addExpected(warning.arg("myContextProperty2"_L1, cppFile,
                                            QString::number(16), QString::number(19)), 3, 40)
                   .addExpected(warning.arg("myContextProperty3"_L1, cppFileInSubfolder,
                                            QString::number(14), QString::number(21)), 4, 40)
                   .build()
                << options;

        QTest::newRow("contextProperties2")
                << u"property var d: myContextProperty4"_s
                << ResultBuilder()
                   .addExpected(QString(warningHead)
                                .append(warningRequiredPropertyBit)
                                .append("Note: candidate context property declaration '%1' at "
                                        "%2:%3:%4\n"
                                        "Note: candidate context property declaration '%1' at %5:%6:%7"_L1)
                                .arg("myContextProperty4"_L1, cppFileInSubfolder,
                                     QString::number(18), QString::number(55),
                                     cppFileInSubfolder, QString::number(21),
                                     QString::number(53)), 1, 17)
                   .build()
                << options;

        QTest::newRow("contextPropertiesImplicitWrapping")
                << u"property Component c: Item { function f() { return myContextProperty1; } }"_s
                << ResultBuilder()
                   .addExpected(unqualifiedAccessWarning, 1, 52)
                   .addExpected(myContextProperty1WarningWithRequiredProperty, 1, 52)
                   .build()
                << options;

        QTest::newRow("contextPropertiesImplicitWrappingWithRequired")
                << u"property Component c: Item { required property int i;"
                   u"function f() { return myContextProperty1; } }"_s
                << ResultBuilder()
                   .addExpected(unqualifiedAccessWarning, 1, 76)
                   .addExpected(myContextProperty1Warning, 1, 76)
                   .build()
                << options;

        QTest::newRow("contextPropertiesInlineComponent")
                << u"component IC: Item { function f() { return myContextProperty1; } }"_s
                << ResultBuilder()
                   .addExpected(unqualifiedAccessWarning, 1, 44)
                   .addExpected(myContextProperty1WarningWithRequiredProperty, 1, 44)
                   .build()
                << options;

        QTest::newRow("contextPropertiesInlineComponentWithRequired")
                << u"component IC: Item { required property int i;"
                   u"function f() { return myContextProperty1; } }"_s
                << ResultBuilder()
                   .addExpected(unqualifiedAccessWarning, 1, 68)
                   .addExpected(myContextProperty1Warning, 1, 68)
                   .build()
                << options;

        QTest::newRow("contextPropertiesRootElement")
                << u"function f() { return myContextProperty1; }"_s
                << ResultBuilder()
                   .addExpected(unqualifiedAccessWarning, 1, 23)
                   .addExpected(myContextProperty1WarningWithRequiredProperty, 1, 23)
                   .build()
                << options;

        QTest::newRow("contextPropertiesRootElementWithRequired")
                << u"required property int i; function f() { return myContextProperty1; }"_s
                << ResultBuilder()
                   .addExpected(unqualifiedAccessWarning, 1, 48)
                   .addExpected(myContextProperty1Warning, 1, 48)
                   .build()
                << options;

        QTest::newRow("contextPropertiesNotHidden")
                << u"required property int myContextProperty2; function f() { return myContextProperty1; }"_s
                << ResultBuilder()
                   .addExpected(unqualifiedAccessWarning, 1, 65)
                   .addExpected(myContextProperty1Warning, 1, 65)
                   .build()
                << options;

        QTest::newRow("contextPropertiesNotHidden2")
                << u"required property int unrelated; function f() { return myContextProperty1; }"_s
                << ResultBuilder()
                   .addExpected(unqualifiedAccessWarning, 1, 56)
                   .addExpected(myContextProperty1Warning, 1, 56)
                   .build()
                << options;

        QTest::newRow("contextPropertiesWithoutRequired")
                << u"property var a: myContextProperty1"_s
                << ResultBuilder()
                   .addExpected(unqualifiedAccessWarning, 1, 17)
                   .addExpected(myContextProperty1WarningWithRequiredProperty, 1, 17)
                   .build()
                << options;
    }

    QTest::newRow("duplicateBinding")
            << u"property int i; i: 42; i: 43;"_s
            << ResultBuilder()
               .addExpected("Duplicate binding on property 'i'"_L1, 1, 27)
               .addExpected("Note: previous binding on 'i' here"_L1, 1, 20)
               .build()
            << defaultOptions;
    QTest::newRow("duplicateBinding2")
            << u"property int i: 42; i: 43;"_s
            << ResultBuilder()
               .addExpected("Duplicate binding on property 'i'"_L1, 1, 24)
               .addExpected("Note: previous binding on 'i' here"_L1, 1, 17)
               .build()
            << defaultOptions;
    QTest::newRow("duplicateGrouped")
            << u"Text { font.pixelSize: 5; font.pixelSize: 10; }"_s
            << ResultBuilder()
               .addExpected("Duplicate binding on property 'pixelSize'"_L1, 1, 43)
               .addExpected("Note: previous binding on 'pixelSize' here"_L1, 1, 24)
               .build()
            << defaultOptions;
    QTest::newRow("duplicateGrouped2")
            << u"Text { font.pixelSize: 5; font { pixelSize: 10 } }"_s
            << ResultBuilder()
               .addExpected("Duplicate binding on property 'pixelSize'"_L1, 1, 45)
               .addExpected("Note: previous binding on 'pixelSize' here"_L1, 1, 24)
               .build()
            << defaultOptions;
    QTest::newRow("duplicateGrouped3")
            << u"Text { font { pixelSize: 5; pixelSize: 10 } }"_s
            << ResultBuilder()
               .addExpected("Duplicate binding on property 'pixelSize'"_L1, 1, 40)
               .addExpected("Note: previous binding on 'pixelSize' here"_L1, 1, 26)
               .build()
            << defaultOptions;
    QTest::newRow("duplicateInlineComponent")
            << u"component A: QtObject {}\n"_s
               u"Item { component A: Item {} }\n"_s
            << ResultBuilder()
               .addExpected("Duplicate inline component 'A'"_L1, 2, 8)
               .addExpected("Note: previous component named 'A' here"_L1, 1, 1)
               .build()
            << defaultOptions;
    QTest::newRow("enumsAreNotTypes")
            << u"function f(a: enum) {}"_s
            << ResultBuilder()
               .addExpected("QML does not have an `enum` type. Use int, or use double if the "
                            "enum's underlying type does not fit into int."_L1, 1, 15)
               .addUnexpected("QML enumerations are not types"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("equality-with-coercion")
            << u"function f(a: int, b: string): bool { return a == b; }"_s
            << ResultBuilder::singleExpected("== and != may perform type coercion, use === or !== to "
                                             "avoid it."_L1, 1, 46)
            << defaultOptions;
    QTest::newRow("equality-with-coercion2")
            << u"function f(a: int): bool { return a == {}; }"_s
            << ResultBuilder::singleExpected("== and != may perform type coercion, use === or !== to "
                                             "avoid it."_L1, 1, 35)
            << defaultOptions;
    QTest::newRow("equality-with-coercion3")
            << u"function f(a: int): bool { return a == true; }"_s
            << ResultBuilder::singleExpected("== and != may perform type coercion, use === or !== to "
                                             "avoid it."_L1, 1, 35)
            << defaultOptions;
    QTest::newRow("duplicateObjectBinding")
            << u"property Item i; i: Item {} i: Item {}"_s
            << ResultBuilder()
               .addExpected("Duplicate binding on property 'i'"_L1, 1, 29)
               .addExpected("Note: previous binding on 'i' here"_L1, 1, 18)
               .build()
            << defaultOptions;
    QTest::newRow("duplicateObjectBinding2")
            << u"property Item i: Item {} i: Item {}"_s
            << ResultBuilder()
               .addExpected("Duplicate binding on property 'i'"_L1, 1, 26)
               .addExpected("Note: previous binding on 'i' here"_L1, 1, 15)
               .build()
            << defaultOptions;
    QTest::newRow("enum")
            << u"enum Hello { World, Kitty, World, dlrow }"_s
            << ResultBuilder()
               .addExpected("Enum key 'World' has already been declared"_L1, 1, 28)
               .addExpected("Note: previous declaration of 'World' here"_L1, 1, 14)
               .addExpected("Enum keys should start with an uppercase"_L1, 1, 35)
               .build()
            << defaultOptions;
    QTest::newRow("enumEntryMatchesEnum")
            << u"enum A { A, B, C }"_s
            << ResultBuilder::singleExpected("Enum entry should be named differently than the enum "
                                             "itself to avoid confusion."_L1, 1, 10)
            << defaultOptions;
    QTest::newRow("final-override-warning-from-parser")
            << u"virtual final property int evil"_s
            << ResultBuilder::singleExpected("The 'virtual' cannot be combined with 'final', as "
                                             "these attributes are mutually exclusive"_L1, 1, 1)
            << defaultOptions;
    QTest::newRow("functionDefinitionInGroupedProperty")
            // should not crash for now, see QTBUG-142091 to get the actual warning
            << u"Item { foo { bar: Array.from((i) => (1)) } }"_s
            << ResultBuilder::ignoredResult()
            << defaultOptions;
    QTest::newRow("invalidLint")
            << u"Item {} // qmllint disable ThisCategoryDoesNotExist\n"_s
            << ResultBuilder::singleExpected(
                   "qmllint directive on unknown category \"ThisCategoryDoesNotExist\""_L1, 1, 11)
            << defaultOptions;

    QTest::newRow("missingTypeAccessMethod")
            << u"MyThing { id: bad } Component.onCompleted: console.log(bad.asdf())"_s
            << ResultBuilder()
               .addExpected("MyThing was not found"_L1)
               .addUnexpected("asdf"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("missingTypeAccessProperty")
            << u"MyThing { id: bad } Component.onCompleted: console.log(bad.asdf)"_s
            << ResultBuilder()
               .addExpected("MyThing was not found"_L1)
               .addUnexpected("asdf"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("missingTypeBinding")
            << u"MyThing { asdf: 123 }"_s
            << ResultBuilder()
               .addExpected("MyThing was not found"_L1)
               .addUnexpected("asdf"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("missingTypeDefaultBinding")
            << u"MyThing { Item{} }"_s
            << ResultBuilder()
               .addExpected("MyThing was not found"_L1)
               .addUnexpected("default"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("missingTypeObjectBinding")
            << u"MyThing { asdf: Item{} }"_s
            << ResultBuilder()
               .addExpected("MyThing was not found"_L1)
               .addUnexpected("asdf"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("nonRootEnum1")
            << u"Item { enum E { A, B, C } }"_s
            << ResultBuilder::singleExpected(
                   "Enum declared outside the root element. It won't be accessible."_L1, 1, 8)
            << defaultOptions;
    QTest::newRow("nonRootEnum2")
            << u"Component { enum E1 { A, B, C } Item { enum E2 { A, B, C } } }"_s
            << ResultBuilder()
               .addExpected("Enum declared outside the root element. It won't be accessible."_L1, 1, 13)
               .addUnexpected("Enum declared outside the root element. It won't be accessible."_L1, 1, 40)
               .build()
            << defaultOptions;
    QTest::newRow("pragmaFunctionSignatureBehavior")
            << u"pragma FunctionSignatureBehavior: Ignored\n"_s
               u"import QtQuick\n"_s
               u"Item { function f() { return unqualified(); } }"_s
            << ResultBuilder::singleExpected("Unqualified", 3, 30)
            << defaultOptions;
    QTest::newRow("preferNonVarProperties")
            << u"readonly property var i: 1     \n"_s
               u"readonly property var r: 1.0   \n"_s
               u"readonly property var s: \"s\" \n"_s
               u"readonly property var b: true  \n"_s
            << ResultBuilder()
               .addExpected("Prefer more specific type int over var", 1, 1)
               .addExpected("Prefer more specific type real or double over var", 2, 1)
               .addExpected("Prefer more specific type string over var", 3, 1)
               .addExpected("Prefer more specific type bool over var", 4, 1)
               .build()
            << defaultOptions;
    QTest::newRow("renamedDoesntCrash")
            << u"import QML\n"
               u"import QtQuick as QQ\n"
               u"QQ.Item { component Yo: MissingType {} }"_s
            << ResultBuilder::singleExpected("MissingType was not found."_L1, 3, 25)
            << defaultOptions;
    QTest::newRow("reservedIdentifier")
            << u"property int interface"_s
            << ResultBuilder::singleExpected(
                   "Reserved keyword \"interface\" cannot be used as a QML identifier.", 1, 14)
            << defaultOptions;
    QTest::newRow("requiredInInlineComponent")
            << u"Item { component Foo: Item { required property var bla; } } Foo {}"_s
            << ResultBuilder::singleExpected(
                   "Component is missing required property bla from Foo"_L1, 1, 61)
            << defaultOptions;
    QTest::newRow("requiredPropertyOwnerMixup")
            << u"component Foo: Item { required property var bla }\n"_s
               u"Foo { Item { property int bla: 43 } }\n"_s
               u"Foo {}\n"_s
            << ResultBuilder()
               .addExpected("Component is missing required property bla from Foo"_L1, 2, 1)
               .addExpected("Component is missing required property bla from Foo"_L1, 3, 1)
               .build()
            << defaultOptions;
    QTest::newRow("requiredPropertyInChild")
            << u"Item { required property var r1 }"_s
            << ResultBuilder::singleExpected(
                   "Component is missing required property r1 from Item"_L1, 1, 1)
            << defaultOptions;
    QTest::newRow("requiredPropertyInRoot")
            << u"component Foo: Item { required property var bla }\n"_s
               u"Foo { Item { property int bla: 43 } }\n"_s
               u"Foo {}\n"_s
            << ResultBuilder()
               .addExpected("Component is missing required property bla from Foo"_L1, 2, 1)
               .addExpected("Component is missing required property bla from Foo"_L1, 3, 1)
               .build()
            << defaultOptions;
    QTest::newRow("requiredPropertyUnsatisfiedByAlias")
            << u"component Foo: Item { required property var r1; }\n"
               "Item {\n"
               "property alias r1: foo.r1;\n"
               "Foo { id: foo }\n"
               "}"_s
            << ResultBuilder::singleExpected(
                   "Component is missing required property r1 from Foo"_L1, 4, 1)
            << defaultOptions;
    QTest::newRow("requiredUnsatisfiedByAlias2")
            << u"component Foo: Item { required property var r1; }\n"
               u"Item {\n"
               "property alias r1: foo.r1;\n"
               u"Item{ Item{ Item{ Item {Foo { id: foo }}}}}\n"
               "}"_s
            << ResultBuilder::singleExpected(
                   "Component is missing required property r1 from Foo"_L1, 4, 25)
            << defaultOptions;
    QTest::newRow("requiredUnsatisfiedByAlias3")
            << u"component Foo: Item { required property var r1; }\n"
               u"Item{ id: i1; property alias r1: i2.r2;\n"
               u"Item{ id: i2; property alias r2: i3.r3;\n"
               u"Item{ id: i3; property alias r3: foo.r1;\n"
               u"Foo { id: foo }\n"
               u"}\n"
               u"}\n"
               u"}"_s
            << ResultBuilder::singleExpected(
                   "Component is missing required property r1 from Foo"_L1, 5, 1)
            << defaultOptions;
    QTest::newRow("requiredFromBaseUnsatisfiedByAlias")
            << u"component Base: Item { required property var r1; }\n"
               u"component Foo: Base {}\n"
               u"Foo { id: foo }"_s
            << ResultBuilder::singleExpected(
                   "Component is missing required property r1 from Base"_L1, 3, 1)
            << defaultOptions;
    QTest::newRow("requiredFromBaseUnsatisfiedByAlias2")
            << u"component Base: Item { required property var r1; }\n"
               u"component Foo: Base {}\n"
               u"Item{ Item{ Item{ Item {Foo { id: foo }}}}}"_s
            << ResultBuilder::singleExpected(
                   "Component is missing required property r1 from Base"_L1, 3, 25)
            << defaultOptions;
    QTest::newRow("requiredFromBaseUnsatisfiedByAlias3")
            << u"component Base: Item { required property var r1; }\n"
               u"component Foo: Base {}\n"
               u"Item{ id: i1; property alias r1: i2.r2;\n"
               u"Item{ id: i2; property alias r2: i3.r3;\n"
               u"Item{ id: i3; property alias r3: foo.r1;\n"
               u"Foo { id: foo }\n"
               u"}\n"
               u"}\n"
               u"}"_s
            << ResultBuilder::singleExpected(
                   "Component is missing required property r1 from Base"_L1, 6, 1)
            << defaultOptions;
    QTest::newRow("requiredInComponent")
            << u"Item { Component { id: comp; required property var bla; Item {} } }"_s
            << ResultBuilder::singleExpected("Component objects cannot declare new properties."_L1,
                                             1, 30)
            << defaultOptions;
    QTest::newRow("testSnippet")
            << u"property int qwer: \"Hello\""_s
            << ResultBuilder::singleExpected("Cannot assign literal of type string to int"_L1)
            << defaultOptions;
    QTest::newRow("typeInstantiatedRecursively")
            << u"Snippet {}"_s
            << ResultBuilder::singleExpected(
                   "Type \"Snippet\" can't be instantiated recursively"_L1, 1, 1)
            << defaultOptions;
    QTest::newRow("typeInstantiatedRecursivelyInlineComponent")
            << u"component MyIC: Item { MyIC {} }"_s
            << ResultBuilder::singleExpected("Type \"MyIC\" can't be instantiated recursively"_L1,
                                             1, 24)
            << defaultOptions;
    QTest::newRow("typeInstantiatedRecursivelyInlineComponent2")
            << u"component MyIC: Item { Snippet {} }"_s
            << ResultBuilder::singleExpected(
                   "Type \"Snippet\" can't be instantiated recursively"_L1, 1, 24)
            << defaultOptions;
    QTest::newRow("unintentionalEmptyBlock-dirty")
            << u"property var v: {}"_s
            << ResultBuilder::singleExpected("Unintentional empty block, use ({}) for empty object "
                                             "literal"_L1, 1, 17)
            << defaultOptions;
    QTest::newRow("unspecializedList")
            << u"property list l"_s
            << ResultBuilder::singleExpected("list was not found. Did you add all imports and "
                                             "dependencies? list is not a type. It requires an "
                                             "element type argument (eg. list<int>)"_L1, 1, 1)
            << defaultOptions;
    QTest::newRow("upperCaseId")
            << u"id: Root"_s
            << ResultBuilder::singleExpected("Id must start with a lower case letter or an '_'"_L1,
                                             1, 5)
            << defaultOptions;
}

void TestQmllint::dirtyQmlSnippet()
{
    QFETCH(QString, code);
    QFETCH(Result, result);
    QFETCH(CallQmllintOptions, options);

    QString qmlCode;
    if (code.startsWith("import"_L1) || code.startsWith("pragma"_L1)) {
        qmlCode = code;
    } else {
        qmlCode = "import QtQuick\nItem {\n%1}"_L1.arg(code);
        addLocationOffsetTo(&result, 2);
    }

    const QJsonArray warnings =
            callQmllintOnSnippet(qmlCode, options, fromResultFlags(result.flags));

    checkResult(warnings, result, [] { }, [] { }, [] { });
}

void TestQmllint::cleanQmlSnippet_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;

    QTest::newRow("assignToVariantProperty")
            << u"Rectangle { id: foo; property var bar; } function f() { foo.bar.foo.bar = 42; }"_s
            << defaultOptions;
    QTest::newRow("color-hex") << u"property color myColor: \"#123456\""_s << defaultOptions;
    QTest::newRow("color-hex2") << u"property color myColor: \"#FFFFFFFF\""_s << defaultOptions;
    QTest::newRow("color-hex3") << u"property color myColor: \"#A0AAff1f\""_s << defaultOptions;
    QTest::newRow("color-hex4") << u"property color myColor: \"#A0A\""_s << defaultOptions;
    QTest::newRow("color-hex5") << u"property color myColor: \"#A0AB\""_s << defaultOptions;
    QTest::newRow("color-name") << u"property color myColor: \"blue\""_s << defaultOptions;
    QTest::newRow("color-name2") << u"property color myColor\nmyColor: \"grEen\""_s
                                 << defaultOptions;
    QTest::newRow("color-transparent") << u"property color myColor: \"transparent\""_s << defaultOptions;
    {
        CallQmllintOptions options;
        options.rootUrls.append(testFile("ContextProperties/src"_L1));

        QTest::newRow("contextPropertiesHidden")
                << u"property int myContextProperty1: 42; property var a: myContextProperty1"_s
                << options;
    }
    QTest::newRow("disableInvalidLint")
            << u" // qmllint disable ThisCategoryDoesNotExist invalid-lint-directive\n"_s
            << defaultOptions;
    QTest::newRow("duplicateList") << u"Item {} Item {}"_s << defaultOptions;
    QTest::newRow("duplicateList2")
            << u"property list<Item> myList; myList: Item {} myList: Item {}"_s << defaultOptions;
    QTest::newRow("enum") << u"enum Hello { World, Kitty, DlroW }"_s << defaultOptions;
    QTest::newRow("equality-with-coercion")
            << u"function f(a:int, b: int): bool { return a == b; }"_s << defaultOptions;
    QTest::newRow("equality-with-coercion2")
            << u"function f(a, b) { return a == null && b == undefined; }"_s << defaultOptions;
    QTest::newRow("listAssignment") << u"import TestTypes\n"
                                       "BirthdayParty {\n"
                                       "    function f() {\n"
                                       "        guests = [];\n"
                                       "    }\n"
                                       "}"_s << defaultOptions;
    QTest::newRow("lowerCaseId") << u"id: root"_s << defaultOptions;
    QTest::newRow("importPrecedence") << uR"(import QtQuick
                                         import ModuleWithMenuBar
                                         import QtQuick.Controls

                                         Item {
                                           width: comp1.custWidth
                                           MenuBar {
                                             id: comp1
                                           }
                                         })"_s
                                      << defaultOptions;
    QTest::newRow("pragmaFunctionSignatureBehavior")
            << u"pragma FunctionSignatureBehavior: Ignored\n"_s
               u"import QtQuick\n"_s
               u"Item { property rect rect: ({ x: 12, y: 13 }); }"_s
            << defaultOptions;
    QTest::newRow("preferNonVarProperties_nonReadOnly")
            << u"property var i: 1     \n"_s
               u"property var r: 1.0   \n"_s
               u"property var s: \"s\" \n"_s
               u"property var b: true  \n"_s
            << defaultOptions;
    QTest::newRow("requiredInComponent2")
            << u"Item { Component { id: comp; Item { required property var bla } } }"_s
            << defaultOptions;
    QTest::newRow("requiredInInlineComponent")
            << u"Item { component Foo: Item { required property var bla; } }"_s << defaultOptions;
    QTest::newRow("requiredInRoot") << u"required property var r1"_s << defaultOptions;
    QTest::newRow("requiredSatisfiedByAlias")
            << u"component Foo: Item { required property var r1; }\n"
               "property alias r1: foo.r1;\n"
               "Foo { id: foo }"_s << defaultOptions;
    QTest::newRow("requiredSatisfiedByAlias2")
            << u"component Foo: Item { required property var r1; }\n"
               u"property alias r1: foo.r1;\n"
               u"Item{ Item{ Item{ Item {Foo { id: foo }}}}}"_s << defaultOptions;
    QTest::newRow("requiredSatisfiedByAlias3")
            << u"component Foo: Item { required property var r1; }\n"
               u"property alias r1: i1.r1;\n"
               u"Item{ id: i1; property alias r1: i2.r2;\n"
               u"Item{ id: i2; property alias r2: i3.r3;\n"
               u"Item{ id: i3; property alias r3: foo.r1;\n"
               u"Foo { id: foo }\n"
               u"}\n"
               u"}\n"
               u"}"_s << defaultOptions;
    QTest::newRow("requiredFromBaseSatisfiedByAlias")
            << u"component Base: Item { required property var r1; }\n"
               u"component Foo: Base {}\n"
               u"property alias r1: foo.r1;\n"
               u"Foo { id: foo }"_s << defaultOptions;
    QTest::newRow("requiredFromBaseSatisfiedByAlias2")
            << u"component Base: Item { required property var r1; }\n"
               u"component Foo: Base {}\n"
               u"property alias r1: foo.r1;\n"
               u"Item{ Item{ Item{ Item {Foo { id: foo }}}}}"_s << defaultOptions;
    QTest::newRow("requiredFromBaseSatisfiedByAlias3")
            << u"component Base: Item { required property var r1; }\n"
               u"component Foo: Base {}\n"
               u"property alias r1: i1.r1;\n"
               u"Item{ id: i1; property alias r1: i2.r2;\n"
               u"Item{ id: i2; property alias r2: i3.r3;\n"
               u"Item{ id: i3; property alias r3: foo.r1;\n"
               u"Foo { id: foo }\n"
               u"}\n"
               u"}\n"
               u"}"_s << defaultOptions;
    QTest::newRow("requiredFromBaseShadowedAndSatisfiedByBinding")
            << u"component Base: Item { property var r1; }\n"
               u"component Foo: Base { required property var r1; } // qmllint disable property-override\n"
               u"Foo { r1: 42 }"_s
            << defaultOptions;
    QTest::newRow("testSnippet") << u"property int qwer: 123"_s << defaultOptions;
    QTest::newRow("underScoreId") << u"id: _Root"_s << defaultOptions;
    QTest::newRow("underScoreGroupedProperty")
            << u"component Foo: Item { property alias _bar: theItem; Item { id: theItem; } }\n"
               "Foo { _bar { width: 10 } }"_s
            << defaultOptions;
    QTest::newRow("unintentionalEmptyBlock-clean")
            << uR"(
                    property var p1: ({})
                    property var p2: {
                        {}
                        return {}
                    }
                )"_s
            << defaultOptions;
    CallQmllintOptions withUnusedImports;
    withUnusedImports.categorySeverityOverrides[qmlUnusedImports.name().toString()] =
            QQmlSA::WarningSeverity::Warning;
    QTest::newRow("used-import") << uR"(import QtQuick
import QtQuick.Controls
Item { id: root; Component.onCompleted: { root.ToolTip.text = "Hola" }})"_s
                                 << withUnusedImports;
    QTest::newRow("used-import2") << uR"(import QtQuick
import QtQuick.Controls as QQC
Item { id: root; Component.onCompleted: { root.QQC.ToolTip.text = "Hola" }})"_s
                                  << withUnusedImports;
    QTest::newRow("usefulExpressionStatement") << u"x: y + 3;"_s << defaultOptions;
    QTest::newRow("usefulExpressionStatement") << u"x: 3;"_s << defaultOptions;
    QTest::newRow("void") << u"function f(): void {}"_s << defaultOptions;
    QTest::newRow("ambiguity-enum-and-chained-attached-property")
            << u"import EnumList\nFlexboxLayout { direction: FlexboxLayout.Row; }"_s
            << defaultOptions;
    QTest::newRow("ambiguity-enum-and-attached-property")
            << u"import QtQuick\n"
               "Item {\n"
               "    id: myItem\n"
               "    enum MyEnum { Component }\n"
               "    Item { property var myP: myItem.Component.completed() }\n"
               "}"_s
            << defaultOptions;
    QTest::newRow("ambiguity-enum-and-attached-property2")
            << u"import QtQuick\n"
               "Item {\n"
               "    id: myItem\n"
               "    enum Component { SomeValue }\n"
               "    Item { property var myP: myItem.Component.completed() }\n"
               "}"_s
            << defaultOptions;
    QTest::newRow("ambiguity-enum-and-attached-property-sanity")
            << u"import QtQuick\n"
               "Item {\n"
               "    id: myItem\n"
               "    Item { property var myP: myItem.Component.completed() }\n"
               "}"_s
            << defaultOptions;
}

void TestQmllint::cleanQmlSnippet()
{
    QFETCH(QString, code);
    QFETCH(CallQmllintOptions, options);

    const QString qmlCode = code.startsWith("import"_L1) || code.startsWith("pragma"_L1)
            ? code : "import QtQuick\nItem {%1}"_L1.arg(code);
    const Result result = ResultBuilder::cleanResult();

    const QJsonArray warnings =
            callQmllintOnSnippet(qmlCode, options, fromResultFlags(result.flags));
    checkResult(warnings, result, [] { }, [] { }, [] { });
}

void TestQmllint::dirtyJsSnippet_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<Result>("result");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;

    QTest::newRow("assignmentInCondition")
            << u"let xxx = 3; if (xxx=3) return;"_s
            << ResultBuilder::singleExpected("Assignment in condition: did you mean to use \"===\" "
                                             "or \"==\" instead of \"=\"?"_L1, 1, 21)
            << defaultOptions;
    QTest::newRow("assignmentWarningLocation")
            << u"console.log(a = 1)"_s
            << ResultBuilder::singleExpected("Unqualified access"_L1, 1, 13)
            << defaultOptions;
    // PatternProperty::isValidAssignmentPattern: shorthand assignments in object literal warn.
    QTest::newRow("badObjectInitializer")
            << u"let x, y; let obj = { \nx = 42, \ny = 9001 }"_s
            << ResultBuilder()
               .addExpected("Invalid shorthand property initializer"_L1, 2, 3)
               .addExpected("Invalid shorthand property initializer"_L1, 3, 3)
               .build()
            << defaultOptions;
    // PatternProperty::isValidAssignmentPattern: trailing comma variant.
    QTest::newRow("badObjectInitializerTrailingComma")
            << u"let a; let obj = { \na = 1, }"_s
            << ResultBuilder::singleExpected("Invalid shorthand property initializer"_L1, 2, 3)
            << defaultOptions;
    // ObjectPattern::isValidAssignmentPattern: nested objects — all shorthand assignments warn.
    QTest::newRow("badObjectInitializerNested")
            << u"let a, b; let obj = { \ninner: { \na = 1, \nb = 2 } }"_s
            << ResultBuilder()
               .addExpected("Invalid shorthand property initializer"_L1, 3, 3)
               .addExpected("Invalid shorthand property initializer"_L1, 4, 3)
               .build()
            << defaultOptions;
    // Nested object literal: shorthand assignment in inner object is invalid.
    QTest::newRow("badObjectInitializerNestedSingle")
            << u"let a; let obj = { \ninner: { \na = 1 } }"_s
            << ResultBuilder::singleExpected("Invalid shorthand property initializer"_L1, 3, 3)
            << defaultOptions;
    // ArrayPattern::isValidAssignmentPattern: shorthand assignment inside array literal warns.
    QTest::newRow("badObjectInitializerInArray")
            << u"let a; let arr = [{ \na = 1 }]"_s
            << ResultBuilder::singleExpected("Invalid shorthand property initializer"_L1, 2, 3)
            << defaultOptions;
    // PatternProperty::isValidAssignmentPattern: multiple warnings collected in same object.
    QTest::newRow("badObjectInitializerMultiple")
            << u"let a, b, c; let obj = { \na = 1, \nb = 2, \nc = 3 }"_s
            << ResultBuilder()
               .addExpected("Invalid shorthand property initializer"_L1, 2, 3)
               .addExpected("Invalid shorthand property initializer"_L1, 3, 3)
               .addExpected("Invalid shorthand property initializer"_L1, 4, 3)
               .build()
            << defaultOptions;
    QTest::newRow("codeAfterBreak")
            << u"for (;;) { break; return 1;}"_s
            << ResultBuilder::singleExpected("Unreachable code"_L1, 1, 19)
            << defaultOptions;
    QTest::newRow("codeAfterContinue")
            << u"for (;;) { continue; return 1;}"_s
            << ResultBuilder::singleExpected("Unreachable code"_L1, 1, 22)
            << defaultOptions;
    QTest::newRow("codeAfterReturn")
            << u"return 1; return 2;"_s
            << ResultBuilder::singleExpected("Unreachable code"_L1, 1, 11)
            << defaultOptions;
    QTest::newRow("codeAfterThrow")
            << u"for (;;) { throw 1; return 1;}"_s
            << ResultBuilder::singleExpected("Unreachable code"_L1, 1, 21)
            << defaultOptions;
    QTest::newRow("comma")
            << u"return 2, 3"_s
            << ResultBuilder::singleExpected("Do not use comma expressions."_L1, 1, 9)
            << defaultOptions;
    QTest::newRow("comma2")
            << u"for (;;) { return 2, 3; }"_s
            << ResultBuilder::singleExpected("Do not use comma expressions."_L1, 1, 20)
            << defaultOptions;
    QTest::newRow("confusing-minuses")
            << u"let a = 0, b = 0; return a-- - b;"_s
            << ResultBuilder::singleExpected("Confusing minuses"_L1, 1, 27) << defaultOptions;
    QTest::newRow("confusing-minuses2")
            << u"let a = 0, b = 0; return a-- - -b;"_s
            << ResultBuilder::singleExpected("Confusing minuses"_L1, 1, 27) << defaultOptions;
    QTest::newRow("confusing-minuses3")
            << u"let a = 0, b = 0; return a-- - --b;"_s
            << ResultBuilder::singleExpected("Confusing minuses"_L1, 1, 27) << defaultOptions;
    QTest::newRow("confusing-pluses")
            << u"let a = 0, b = 0; return a++ + b;"_s
            << ResultBuilder::singleExpected("Confusing pluses"_L1, 1, 27) << defaultOptions;
    QTest::newRow("confusing-pluses2")
            << u"let a = 0, b = 0; return a++ + +b;"_s
            << ResultBuilder::singleExpected("Confusing pluses"_L1, 1, 27) << defaultOptions;
    QTest::newRow("confusing-pluses3")
            << u"let a = 0, b = 0; return a++ + ++b;"_s
            << ResultBuilder::singleExpected("Confusing pluses"_L1, 1, 27) << defaultOptions;
    QTest::newRow("constructor")
            << u"return new Boolean();"_s
            << ResultBuilder::singleExpected("Do not use 'Boolean' as a constructor."_L1, 1, 12)
            << defaultOptions;
    QTest::newRow("constructor2")
            << u"return new Math();"_s
            << ResultBuilder::singleExpected("Do not use 'Math' as a constructor."_L1, 1, 12)
            << defaultOptions;
    QTest::newRow("constructorArray")
            << u"return new Array(1, 2);"_s
            << ResultBuilder()
               .addExpected("Array has confusing semantics, use an array literal ([]) instead."_L1, 1, 12)
               .addFix("Replace with array literal"_L1, Edits{ { "["_L1, 1, 8 }, { "]"_L1, 1, 22 } })
               .build()
            << defaultOptions;
    QTest::newRow("doubleConst")
            << u"const x = 4; const x = 4;"_s
            << ResultBuilder()
               .addExpected("Identifier 'x' has already been declared"_L1, 1, 20)
               .addExpected("Note: previous declaration of 'x' here"_L1, 1, 7)
               .build()
            << defaultOptions;
    QTest::newRow("doubleLet")
            << u"let x = 4; let x = 4;"_s
            << ResultBuilder()
               .addExpected("Identifier 'x' has already been declared"_L1, 1, 16)
               .addExpected("Note: previous declaration of 'x' here"_L1, 1, 5)
               .build()
            << defaultOptions;
    QTest::newRow("eval")
            << u"let x = eval();"_s
            << ResultBuilder::singleExpected("Do not use 'eval'"_L1, 1, 9)
            << defaultOptions;
    QTest::newRow("eval2")
            << u"let x = eval(\"1 + 1\");"_s
            << ResultBuilder::singleExpected("Do not use 'eval'"_L1, 1, 9)
            << defaultOptions;
    QTest::newRow("equality-with-coercion")
            << u"let a = 0, b = \"0\"; return a == b;"_s
            << ResultBuilder::singleExpected(
                   "== and != may perform type coercion, use === or !== to avoid it."_L1, 1, 28)
            << defaultOptions;
    QTest::newRow("functionAfterThrow")
            << u"throw 1; function f() {}; let x = 1; function g() {}; let y = 1;"_s
            << ResultBuilder()
               .addExpected("Unreachable code"_L1, 1, 27)
               .addExpected("Unreachable code"_L1, 1, 55)
               .build()
            << defaultOptions;
    QTest::newRow("functionAfterThrow2")
            << u"throw 1; if (x) { function f() {}; } let x = 1; if (x) { function g() {};} let y = 1;"_s
            << ResultBuilder()
               .addExpected("Unreachable code"_L1, 1, 38)
               .addExpected("Unreachable code"_L1, 1, 76)
               .build()
            << defaultOptions;
    {
        CallQmllintOptions options;
        options.categorySeverityOverrides.insert(qmlFunctionUsedBeforeDeclaration.name().toString(),
                                                 QQmlJS::WarningSeverity::Warning);
        QTest::newRow("functionUsedBeforeDeclaration")
                << u"fff(); function fff() {}"_s
                << ResultBuilder()
                   .addExpected("Function 'fff' is used here before its declaration"_L1, 1, 1)
                   .addExpected("Note: declaration of 'fff' here"_L1, 1, 17)
                   .build()
                << options;
    }
    QTest::newRow("indirectEval") << u"let x = (1, eval)();"_s
                                  << ResultBuilder::singleExpected("Do not use 'eval'"_L1, 1, 13)
                                  << defaultOptions;
    QTest::newRow("indirectEval2")
            << u"let x = (1, eval)(\"1 + 1\");"_s
            << ResultBuilder::singleExpected("Do not use 'eval'"_L1, 1, 13)
            << defaultOptions;
    QTest::newRow("redundantOptionalChainingNonVoidableBase")
            << u"/a/?.flags"_s
            << ResultBuilder::singleExpected(
                   "Redundant optional chaining for lookup on non-voidable and non-nullable type "
                   "QRegularExpression"_L1, 1, 6)
            << defaultOptions;
    QTest::newRow("math_typo")
            << u"Math.minj(1,2)"_s
            << ResultBuilder()
               .addExpected("Member \"minj\" not found on Math object", 1, 6)
               .addExpected("Did you mean \"min\"?", 1, 6)
               .build()
            << defaultOptions;
    QTest::newRow("shadowArgument")
            << u"function f(a) { const a = 33; }"_s
            << ResultBuilder()
               .addExpected("Identifier 'a' has already been declared"_L1, 1, 23)
               .addExpected("Note: previous declaration of 'a' here"_L1, 1, 12)
               .build()
            << defaultOptions;
    QTest::newRow("shadowFunction")
            << u"function f() {} const f = 33"_s
            << ResultBuilder()
               .addExpected("Identifier 'f' has already been declared"_L1, 1, 23)
               .addExpected("Note: previous declaration of 'f' here"_L1, 1, 10)
               .build()
            << defaultOptions;
    QTest::newRow("shadowFunction2")
            << u"const f = 33; function f() {}"_s
            << ResultBuilder()
               .addExpected("Identifier 'f' has already been declared"_L1, 1, 24)
               .addExpected("Note: previous declaration of 'f' here"_L1, 1, 7)
               .build()
            << defaultOptions;
    QTest::newRow("unterminatedCaseBlock")
            << uR"(
                switch (0) {
                case 0:                 // ok: empty
                case 1:                 // ok: terminated
                    return 1
                case 2:                 // ko: non-empty, non-terminated, no comment
                    console.log("2")
                default:                // ko: non-empty, non-terminated, no comment
                    1 + 1
                case 3:                 // ok: comment
                    4 + 4
                    // fallthrough
                case 4:                 // ok: comment (case insensitive)
                    5 + 5
                    // FaLlThRoUgH
                case 5:                 // ok: nothing to fall through to ...
                    1 + 2
                })"_s
            << ResultBuilder()
               .addExpected("Non-empty case block potentially falls through to the next case or "
                            "default statement. Add \"// fallthrough\" at the end of the block to "
                            "silence this warning."_L1, 6, 17)
               .addExpected("Non-empty case block potentially falls through to the next case or "
                            "default statement. Add \"// fallthrough\" at the end of the block to "
                            "silence this warning."_L1, 8, 17)
               .addUnexpected("Non-empty case block potentially falls through to the next case or "
                              "default statement. Add \"// fallthrough\" at the end of the block "
                              "to silence this warning."_L1, 3, 17)
               .addUnexpected("Non-empty case block potentially falls through to the next case or "
                              "default statement. Add \"// fallthrough\" at the end of the block "
                              "to silence this warning."_L1, 4, 17)
               .addUnexpected("Non-empty case block potentially falls through to the next case or "
                              "default statement. Add \"// fallthrough\" at the end of the block "
                              "to silence this warning."_L1, 10, 17)
               .addUnexpected("Non-empty case block potentially falls through to the next case or "
                              "default statement. Add \"// fallthrough\" at the end of the block "
                              "to silence this warning."_L1, 13, 17)
               .build()
            << defaultOptions;
    constexpr QLatin1String unterminatedCaseMsg =
            "Non-empty case block potentially falls through to the next case or default statement. "
            "Add \"// fallthrough\" at the end of the block to silence this warning."_L1;
    QTest::newRow("unterminatedCaseBlockNested")
            << uR"(switch (0) {
                case 0: // all cases should be KO, including this one
                    switch (2) {
                        case 1: { let i = 34; let j = 45; }
                        case 2: switch (2) { default: f(); }
                        case 3: someLabel: f();
                        case 5: try { f(); } finally {}
                        case 5.5: try { } finally { f(); }
                        case 6.1: for (;;) { break; }
                        case 6.2: do { return; } while (false)
                        case 6.3: while (true) { return; }
                        case 6.4: if (false) { return; }
                        case -1: return; // dummy
                    }
                case -1: return // dummy
                })"_s
            << ResultBuilder()
               .addExpected(unterminatedCaseMsg, 2, 17)
               .addExpected(unterminatedCaseMsg, 4, 25)
               .addExpected(unterminatedCaseMsg, 5, 25)
               .addExpected(unterminatedCaseMsg, 6, 25)
               .addExpected(unterminatedCaseMsg, 7, 25)
               .addExpected(unterminatedCaseMsg, 8, 25)
               .addExpected(unterminatedCaseMsg, 9, 25)
               .addExpected(unterminatedCaseMsg, 10, 25)
               .addExpected(unterminatedCaseMsg, 11, 25)
               .addExpected(unterminatedCaseMsg, 12, 25)
               .build()
            << defaultOptions;
    QTest::newRow("unterminatedCaseBlockNested2")
            << uR"(switch (0) {
                case 0: // one case is KO, so this one too
                    switch (2) {
                        case 1: return;
                        default: f(); // not ok
                    }
                case -1: return // dummy
                })"_s
            << ResultBuilder::singleExpected(
                   "Non-empty case block potentially falls through to the next case or default "
                   "statement. Add \"// fallthrough\" at the end of the block to silence this "
                   "warning."_L1, 2, 17)
            << defaultOptions;
    {
        CallQmllintOptions options;
        options.categorySeverityOverrides.insert(qmlVoid.name().toString(), QQmlJS::WarningSeverity::Warning);
        QTest::newRow("void")
                << u"void 1;"_s
                << ResultBuilder::singleExpected("Do not use void expressions"_L1, 1, 1)
                << options;
        QTest::newRow("void2")
                << u"void(0)"_s
                << ResultBuilder::singleExpected("Do not use void expressions"_L1, 1, 1)
                << options;
    }
    QTest::newRow("uselessExpressionStatement")
            << u"x + 3;"_s
            << ResultBuilder::singleExpected("Expression statement has no obvious effect."_L1, 1, 1)
            << defaultOptions;
    QTest::newRow("uselessExpressionStatement2")
            << u"for (;;) { x + 3; return x; }"_s
            << ResultBuilder::singleExpected("Expression statement has no obvious effect."_L1, 1, 12)
            << defaultOptions;
    QTest::newRow("varVariableInBlockScope")
            << uR"(let i = 0;
        {
            var j = 1;
        }
        i = j;)"_s
            << ResultBuilder::singleExpected("var declaration in block scope is hoisted to "
                                             "function scope"_L1, 3, 17)
            << defaultOptions;
}

void TestQmllint::dirtyJsSnippet()
{
    QFETCH(QString, code);
    QFETCH(Result, result);
    QFETCH(CallQmllintOptions, options);

    static constexpr QLatin1StringView templateString =
            "import QtQuick\nItem { function f() {\n%1}}"_L1;

    // templateString adds 2 newlines before snippet
    addLocationOffsetTo(&result, 2);

    const QString qmlCode = templateString.arg(code);
    const QJsonArray warnings =
            callQmllintOnSnippet(qmlCode, options, fromResultFlags(result.flags));

    checkResult(warnings, result, [] { }, [] { }, [] { });
}

void TestQmllint::cleanJsSnippet_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;

    QTest::newRow("codeAfterBreak") << u"for (;;) { if (x) break; return 1;}"_s << defaultOptions;
    QTest::newRow("codeAfterContinue")
            << u"for (;;) { if (x) continue; return 1;}"_s << defaultOptions;
    QTest::newRow("codeAfterReturn") << u"if (x) return 1; return 2;"_s << defaultOptions;
    QTest::newRow("codeAfterReturn2")
            << u"switch (1) { case 1: break; default: return 4;} return 3;"_s << defaultOptions;
    QTest::newRow("codeAfterThrow") << u"for (;;) { if (x) throw 1; return 1;}"_s << defaultOptions;
    QTest::newRow("comma") << u"let i, end; for (i = 0, end = 42; i < end; ++i) {}"_s
                           << defaultOptions;
    QTest::newRow("constructor") << u"function F() {}; return new F();"_s << defaultOptions;
    QTest::newRow("constructorArray") << u"return new Array();"_s << defaultOptions;
    QTest::newRow("constructorArray2") << u"return new Array(42);"_s << defaultOptions;
    QTest::newRow("doubleInDifferentScopes")
            << u"const a = 42; for (let a = 1; a < 10; ++a) {}"_s << defaultOptions;
    QTest::newRow("doubleVar") << u"var x = 5; var y = 5"_s << defaultOptions;
    QTest::newRow("functionAfterThrow") << u"throw 1; function f() {}"_s << defaultOptions;
    QTest::newRow("notAssignmentInCondition")
            << u"let x = 3; if (x==3) return;"_s << defaultOptions;

    QTest::newRow("equality-with-coercion") << u"let a = 0; return a == null;"_s << defaultOptions;
    QTest::newRow("equality-with-coercion2")
            << u"let a = 0; return a == undefined;"_s << defaultOptions;
    QTest::newRow("shadowArgument")
            << u"function f(a) { if (1){ const a = 33; } }"_s << defaultOptions;
    QTest::newRow("shadowFunction") << u"function f() { function f() {} }"_s << defaultOptions;
    QTest::newRow("testSnippet") << u"let x = 5"_s << defaultOptions;
    QTest::newRow("terminatedCaseBlocks") << uR"(switch (0) {
                case 0: // all cases should be OK, so this one should be OK too!
                    switch (2) {
                        case 1: { let i = 34; return; }
                        case 2: switch (2) { default:  return; }
                        case 3: someLabel: return;
                        case 5: try { return; } finally {}
                        case 5.5: try { } finally { return; }
                        case 6: if (false) { return; } else { return; }
                        case -1: return; // dummy
                    }
                case -1: return; // dummy
                })"_s << defaultOptions;
    QTest::newRow("usefulExpressionStatement") << u"x += 3;"_s << defaultOptions;
    QTest::newRow("usefulExpressionStatement2")
            << u"for (;;) { x /= 3; return x; }"_s << defaultOptions;
    QTest::newRow("varUsedBeforeDeclarationWithIgnore")
            << u"// qmllint disable var-used-before-declaration\n"
               "f(x) ;\n"
               "// qmllint enable var-used-before-declaration\n"
               "let x = 3;"_s
            << defaultOptions;

    // Assignment in destructuring binding pattern — NOT an object literal, no warning.
    QTest::newRow("destructuringAssignmentInBinding")
            << u"let { a = 42 } = {}; return a;"_s << defaultOptions;
    // Default parameter in function — NOT an object literal, no warning.
    QTest::newRow("defaultParameterInFunction")
            << u"function g(a = 1) { return a; } return g();"_s << defaultOptions;
    // Destructuring in for-of — NOT an object literal, no warning.
    QTest::newRow("destructuringInForOf")
            << u"for (const { a = 0 } of [{}]) { return a; }"_s << defaultOptions;
    // Destructuring assignment (lhs of =) — NOT an object literal, no warning.
    QTest::newRow("destructuringAssignmentLhs")
            << u"let a; ({ a = 1 } = { }); return a;"_s << defaultOptions;
    QTest::newRow("arrowFunctionDestructuringDefault")
            << u"const f = ({ a = 0 } = {}) => a; return f();"_s << defaultOptions;
    // Nested object literal: destructuring assignment is valid when done inside
    // a nested expression (IIFE), not as an object literal shorthand initializer.
    QTest::newRow("nestedObjectLiteralWithDestructuringAssignment")
            << u"let obj = { inner: (function() { let a; ({ a = 1 } = {}); return a; })() }; return obj.inner;"_s
            << defaultOptions;
    // Colon-property in object literal — NOT a shorthand assignment, no warning.
    QTest::newRow("objectLiteralColonProperty")
            << u"let obj = { a: 1, b: 2 }; return obj;"_s << defaultOptions;
    // Shorthand property (no initializer) — valid, no warning.
    QTest::newRow("objectLiteralShorthandProperty")
            << u"let a = 1; let obj = { a }; return obj;"_s << defaultOptions;
}

void TestQmllint::cleanJsSnippet()
{
    QFETCH(QString, code);
    QFETCH(CallQmllintOptions, options);

    const QString qmlCode = "import QtQuick\nItem { function f() {\n%1}}"_L1.arg(code);
    const Result result = ResultBuilder::cleanResult();

    const QJsonArray warnings =
            callQmllintOnSnippet(qmlCode, options, fromResultFlags(result.flags));
    checkResult(warnings, result, [] { }, [] { }, [] { });
}

using ExpectedProperties = QHash<QString, int>;

void TestQmllint::contextPropertiesFromRootUrls_data()
{
    QTest::addColumn<QStringList>("rootUrls");
    QTest::addColumn<ExpectedProperties>("expectedProperties");
    QTest::addColumn<bool>("disableGrep");

    for (const bool disableGrep : { false, true }) {
        QTest::addRow("grep%s", disableGrep ? "-fallback" : "")
                << QStringList{ testFile("ContextProperties/src"_L1) }
                << ExpectedProperties{ { "myContextProperty1", 1 },
                                       { "myContextProperty2", 1 },
                                       { "myContextProperty3", 1 },
                                       { "myContextProperty4", 2 } }
                << disableGrep;
    }
}

void TestQmllint::contextPropertiesFromRootUrls()
{
    QFETCH(QStringList, rootUrls);
    QFETCH(ExpectedProperties, expectedProperties);
    QFETCH(bool, disableGrep);

    if (disableGrep)
        qputenv("QT_QML_NO_GREP", "1");
    const auto properties = QQmlJS::HeuristicContextProperties::collectFromCppSourceDirs(rootUrls);

    if (disableGrep)
        qunsetenv("QT_QML_NO_GREP");

    QCOMPARE(properties.size(), expectedProperties.size());

    for (auto [key, value] : expectedProperties.asKeyValueRange()) {
        QVERIFY(properties.contains(key));
        QCOMPARE(properties.definitionsForName(key).size(), value);
    }
}

void TestQmllint::contextPropertiesFromUser()
{
    QQmlToolingSettings settings("contextProperties");
    settings.addOption(QQmlJS::UserContextProperties::s_unqualifiedAccessDisabledKey,
                       "myCP1,myCP2"_L1);
    settings.addOption(QQmlJS::UserContextProperties::s_onUsageWarnedKey, "myCP3,myCP4"_L1);
    QQmlJS::UserContextProperties properties(settings);

    QCOMPARE(properties.unqualifiedAccessDisabled().size(), 2);
    QCOMPARE(properties.onUsageWarned().size(), 2);

    QVERIFY(properties.isUnqualifiedAccessDisabled("myCP1"_L1));
    QVERIFY(!properties.isOnUsageWarned("myCP1"_L1));
    QVERIFY(properties.isUnqualifiedAccessDisabled("myCP2"_L1));

    QVERIFY(properties.isOnUsageWarned("myCP3"_L1));
    QVERIFY(!properties.isUnqualifiedAccessDisabled("myCP3"_L1));
    QVERIFY(properties.isOnUsageWarned("myCP4"_L1));

    QVERIFY(!properties.isUnqualifiedAccessDisabled("doesNotExist"_L1));
    QVERIFY(!properties.isOnUsageWarned("doesNotExist"_L1));
}

#if QT_CONFIG(qmlcontextpropertydump)
void TestQmllint::contextPropertiesFromHeuristicWrite()
{
    using namespace QQmlJS;
    HeuristicContextProperties properties;
    properties.add("myCP1",
                   HeuristicContextProperty{ "myPath1/myFile1.cpp", SourceLocation{ 1, 2, 3, 4 } });
    properties.add(
            "myCP2",
            HeuristicContextProperty{ "myPath2/myFile2.cpp", SourceLocation{ 10, 20, 30, 40 } });
    properties.add("my_cp_3",
                   HeuristicContextProperty{ "myPath3/myFile3.cpp",
                                             SourceLocation{ 100, 200, 300, 400 } });
    properties.add("my_cp_3",
                   HeuristicContextProperty{ "myPath3/myFile3.cpp",
                                             SourceLocation{ 101, 202, 303, 404 } });
    properties.add("my_cp_3",
                   HeuristicContextProperty{ "myPath3/myFile3.cpp",
                                             SourceLocation{ 111, 222, 333, 444 } });

    QTemporaryDir myBuild;
    QVERIFY(myBuild.isValid());
    properties.writeCache(myBuild.path());

    QFile f(myBuild.filePath(".qt/contextPropertyDump.ini"_L1));
    QVERIFY(f.open(QFile::ReadOnly | QFile::Text));
    const QString fileContent = f.readAll();
    QCOMPARE(fileContent, R"([cachedHeuristicList]
1\name=myCP1
2\name=myCP2
3\name=my_cp_3
size=3

[property_myCP1]
1\fileName=myPath1/myFile1.cpp
1\sourceLocation="1,2,3,4"
size=1

[property_myCP2]
1\fileName=myPath2/myFile2.cpp
1\sourceLocation="10,20,30,40"
size=1

[property_my_cp_3]
1\fileName=myPath3/myFile3.cpp
1\sourceLocation="100,200,300,400"
2\fileName=myPath3/myFile3.cpp
2\sourceLocation="101,202,303,404"
3\fileName=myPath3/myFile3.cpp
3\sourceLocation="111,222,333,444"
size=3
)"_L1);
}

void TestQmllint::contextPropertiesFromHeuristicRead()
{
    using namespace QQmlJS;

    HeuristicContextProperties properties;
    properties.add("myCP1",
                   HeuristicContextProperty{ "myPath1/myFile1.cpp", SourceLocation{ 1, 2, 3, 4 } });
    properties.add(
            "myCP2",
            HeuristicContextProperty{ "myPath2/myFile2.cpp", SourceLocation{ 10, 20, 30, 40 } });
    properties.add("my_cp_3",
                   HeuristicContextProperty{ "myPath3/myFile3.cpp",
                                             SourceLocation{ 100, 200, 300, 400 } });
    properties.add("my_cp_3",
                   HeuristicContextProperty{ "myPath3/myFile3.cpp",
                                             SourceLocation{ 101, 202, 303, 404 } });
    properties.add("my_cp_3",
                   HeuristicContextProperty{ "myPath3/myFile3.cpp",
                                             SourceLocation{ 111, 222, 333, 444 } });

    QTemporaryDir myBuild;
    QVERIFY(myBuild.isValid());
    properties.writeCache(myBuild.path());
    QSettings settings(myBuild.filePath(".qt/contextPropertyDump.ini"_L1), QSettings::IniFormat);
    const auto readBack = QQmlJS::HeuristicContextProperties::collectFrom(&settings);
    QCOMPARE(readBack, properties);
}

void TestQmllint::contextPropertiesFromHeuristicLint()
{
    const QString filename = testFile("ContextProperties/qml/MyContextProperties.qml"_L1);

    CallQmllintOptions options;
    options.qrcToFilePaths.insert("qt/qml/MyModule/file.qml"_L1, filename);
    options.qrcToFilePaths.insert("qt/qml/MyModule/qmldir"_L1,
                                  testFile("ContextProperties/build/qmldir"_L1));

    const QJsonArray warnings = callQmllint(filename, options, CallQmllintCheck::ShouldFail);
    checkResult(
            warnings,
            ResultBuilder()
                .addExpected("Potential context property access detected. Context properties are "
                             "discouraged in QML: use normal, required, or singleton properties "
                             "instead.\nNote: 'myContextProperty1' assumed to be a potential "
                             "context property because it is not declared as required "
                             "property.\nNote: candidate context property declaration "
                             "'myContextProperty1' at myPath1/myFile1.cpp:3:4"_L1,
                             4, 21)
                .build(),
            [] { }, [] { }, [] { });
}
#endif

void TestQmllint::cleanQmlCode_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;

    QTest::newRow("2Behavior") << QStringLiteral("2behavior.qml") << defaultOptions;
    QTest::newRow("Accessible") << QStringLiteral("accessible.qml") << defaultOptions;
    QTest::newRow("AddressableValue") << QStringLiteral("addressableValue.qml") << defaultOptions;
    QTest::newRow("AttachedProps") << QStringLiteral("AttachedProps.qml") << defaultOptions;
    QTest::newRow("AttachedType") << QStringLiteral("AttachedType.qml") << defaultOptions;
    QTest::newRow("BindingBeforeDeclaration")
            << QStringLiteral("bindingBeforeDeclaration.qml") << defaultOptions;
    QTest::newRow("BindingTypeMismatch")
            << QStringLiteral("bindingTypeMismatch.qml") << defaultOptions;
    QTest::newRow("BindingTypeMismatchFunction")
            << QStringLiteral("bindingTypeMismatchFunction.qml") << defaultOptions;
    QTest::newRow("BindingsOnGroupAndAttachedProperties")
            << QStringLiteral("goodBindingsOnGroupAndAttached.qml") << defaultOptions;
    QTest::newRow("CustomParserSignalHandler")
            << QStringLiteral("customParserSignalHandler.qml") << defaultOptions;
    QTest::newRow("CustomParserUnqualifiedAccess")
            << QStringLiteral("customParserUnqualifiedAccess.qml") << defaultOptions;
    QTest::newRow("duplicateImportsClean")
            << QStringLiteral("duplicateImportsClean.qml") << defaultOptions;
    QTest::newRow("EnumAccess1") << QStringLiteral("EnumAccess1.qml") << defaultOptions;
    QTest::newRow("EnumAccess2") << QStringLiteral("EnumAccess2.qml") << defaultOptions;
    QTest::newRow("EnumAccessCpp") << QStringLiteral("EnumAccessCpp.qml") << defaultOptions;
    QTest::newRow("GoodModulePrefix") << QStringLiteral("goodModulePrefix.qml") << defaultOptions;
    QTest::newRow("groupedPropertyBinding")
            << QStringLiteral("groupedPropertyBinding.qml") << defaultOptions;
    QTest::newRow("ID overrides property") << QStringLiteral("accessibleId.qml") << defaultOptions;
    QTest::newRow("ImportDirectoryQmldir")
            << QStringLiteral("Things/LintDirectly.qml") << defaultOptions;
    QTest::newRow("ImportPrecedence") << QStringLiteral("Connections.qml") << defaultOptions;
    QTest::newRow("ImportQMLModule") << QStringLiteral("importQMLModule.qml") << defaultOptions;
    QTest::newRow("InlineComponent") << QStringLiteral("inlineComponent.qml") << defaultOptions;
    QTest::newRow("InlineComponentWithComponents")
            << QStringLiteral("inlineComponentWithComponents.qml") << defaultOptions;
    QTest::newRow("InlineComponentsChained")
            << QStringLiteral("inlineComponentsChained.qml") << defaultOptions;
    QTest::newRow("JS_with_pragma_and_import")
            << QStringLiteral("QTBUG-45916.js") << defaultOptions;
    QTest::newRow("ListProperty") << QStringLiteral("ListProperty.qml") << defaultOptions;
    QTest::newRow("ParentEnum") << QStringLiteral("parentEnum.qml") << defaultOptions;
    QTest::newRow("QEventPoint") << QStringLiteral("qEventPoint.qml") << defaultOptions;
    QTest::newRow("QML_importing_JS") << QStringLiteral("importing_js.qml") << defaultOptions;
    QTest::newRow("QObject.hasOwnProperty")
            << QStringLiteral("qobjectHasOwnProperty.qml") << defaultOptions;
    QTest::newRow("QQmlEasingEnums::Type")
            << QStringLiteral("animationEasing.qml") << defaultOptions;
    QTest::newRow("QQmlScriptString") << QStringLiteral("scriptstring.qml") << defaultOptions;
    QTest::newRow("QVariant") << QStringLiteral("qvariant.qml") << defaultOptions;
    QTest::newRow("QtQuick.Window 2.1") << QStringLiteral("qtquickWindow21.qml") << defaultOptions;
    QTest::newRow("ScriptInTemplate") << QStringLiteral("scriptInTemplate.qml") << defaultOptions;
    QTest::newRow("Signals") << QStringLiteral("Signal.qml") << defaultOptions;
    QTest::newRow("Simple_QML") << QStringLiteral("Simple.qml") << defaultOptions;
    QTest::newRow("StringToDateTime") << QStringLiteral("stringToDateTime.qml") << defaultOptions;
    QTest::newRow("Unused imports (multi)") << QStringLiteral("unused_multi.qml") << defaultOptions;
    QTest::newRow("Unused static module") << QStringLiteral("unused_static.qml") << defaultOptions;
    QTest::newRow("Unversioned change signal without arguments")
            << QStringLiteral("unversionChangedSignalSansArguments.qml") << defaultOptions;
    QTest::newRow("Used imports") << QStringLiteral("used.qml") << defaultOptions;
    QTest::newRow("ValidLiterals") << QStringLiteral("validLiterals.qml") << defaultOptions;
    QTest::addRow("ValidTranslations")
            << u"translations/qsTranslateTranslation.qml"_s << defaultOptions;
    QTest::addRow("ValidTranslations2") << u"translations/qsTrTranslation.qml"_s << defaultOptions;
    QTest::addRow("ValidTranslations3")
            << u"translations/qsTrIdTranslation.qml"_s << defaultOptions;
    QTest::addRow("ValidTranslations4") << u"translations/Good.qml"_s << defaultOptions;
    QTest::newRow("VariantMapGetPropertyLookup")
            << QStringLiteral("variantMapLookup.qml") << defaultOptions;
    QTest::newRow("WriteListProperty") << QStringLiteral("writeListProperty.qml") << defaultOptions;
    QTest::newRow("aliasGroup") << QStringLiteral("aliasGroup.qml") << defaultOptions;
    QTest::newRow("aliasToList") << QStringLiteral("aliasToList.qml") << defaultOptions;
    QTest::newRow("aliasToRequiredProperty")
            << QStringLiteral("aliasToRequiredPropertyIsNotRequiredItself.qml") << defaultOptions;
    QTest::newRow("anchors1") << QStringLiteral("anchors1.qml") << defaultOptions;
    QTest::newRow("anchors2") << QStringLiteral("anchors2.qml") << defaultOptions;
    QTest::newRow("asCast") << QStringLiteral("asCast.qml") << defaultOptions;
    QTest::newRow("asValueTypeGood") << QStringLiteral("asValueTypeGood.qml") << defaultOptions;
    QTest::newRow("attached") << QStringLiteral("attached.qml") << defaultOptions;
    QTest::newRow("attachedImportUse") << QStringLiteral("attachedImportUse.qml") << defaultOptions;
    QTest::newRow("attachedPropertyAssignments")
            << QStringLiteral("attachedPropertyAssignments.qml") << defaultOptions;
    QTest::newRow("attachedTypeIndirect")
            << QStringLiteral("attachedTypeIndirect.qml") << defaultOptions;
    QTest::newRow("boundComponents") << QStringLiteral("boundComponents.qml") << defaultOptions;
    QTest::newRow("bytearray") << QStringLiteral("bytearray.qml") << defaultOptions;
    QTest::newRow("callBase") << QStringLiteral("callBase.qml") << defaultOptions;
    QTest::newRow("callLater") << QStringLiteral("callLater.qml") << defaultOptions;
    QTest::newRow("catchIdentifier")
            << QStringLiteral("catchIdentifierNoWarning.qml") << defaultOptions;
    QTest::newRow("compositeSingleton")
            << QStringLiteral("compositesingleton.qml") << defaultOptions;
    QTest::newRow("confusingImport") << QStringLiteral("Dialog.qml") << defaultOptions;
    QTest::newRow("connectionNoParent")
            << QStringLiteral("connectionNoParent.qml") << defaultOptions; // QTBUG-97600
    QTest::newRow("constInvokable") << QStringLiteral("useConstInvokable.qml") << defaultOptions;
    QTest::newRow("constructorProperty")
            << QStringLiteral("constructorProperty.qml") << defaultOptions;
    QTest::newRow("cppPropertyChangeHandlers")
            << QStringLiteral("goodCppPropertyChangeHandlers.qml") << defaultOptions;
    QTest::newRow("customParser") << QStringLiteral("customParser.qml") << defaultOptions;
    QTest::newRow("customParser.recursive")
            << QStringLiteral("customParser.recursive.qml") << defaultOptions;
    QTest::addRow("deceptiveLayout") << u"deceptiveLayout.qml"_s << defaultOptions;
    QTest::newRow("declared property of JS object")
            << QStringLiteral("bareQt.qml") << defaultOptions;
    QTest::newRow("defaultImport") << QStringLiteral("defaultImport.qml") << defaultOptions;
    QTest::newRow("defaultProperty") << QStringLiteral("defaultProperty.qml") << defaultOptions;
    QTest::newRow("defaultPropertyComponent")
            << QStringLiteral("defaultPropertyComponent.qml") << defaultOptions;
    QTest::newRow("defaultPropertyComponent2")
            << QStringLiteral("defaultPropertyComponent.2.qml") << defaultOptions;
    QTest::newRow("defaultPropertyList")
            << QStringLiteral("defaultPropertyList.qml") << defaultOptions;
    QTest::newRow("defaultPropertyListModel")
            << QStringLiteral("defaultPropertyListModel.qml") << defaultOptions;
    QTest::newRow("defaultPropertyVar")
            << QStringLiteral("defaultPropertyVar.qml") << defaultOptions;
    QTest::newRow("dependsOnDuplicateType")
            << QStringLiteral("dependsOnDuplicateType.qml") << defaultOptions;
    QTest::newRow("deprecatedFunctionOverride")
            << QStringLiteral("deprecatedFunctionOverride.qml") << defaultOptions;
    QTest::newRow("dontCheckJSTypes") << QStringLiteral("dontCheckJSTypes.qml") << defaultOptions;
    QTest::newRow("dontConfuseMemberPrintWithGlobalPrint")
            << QStringLiteral("findMemberPrint.qml") << defaultOptions;
    QTest::newRow("duplicateQmldirImport")
            << QStringLiteral("qmldirImport/duplicate.qml") << defaultOptions;
    QTest::newRow("enumFromQtQml") << QStringLiteral("enumFromQtQml.qml") << defaultOptions;
    QTest::newRow("enumList") << QStringLiteral("enumListTest.qml") << defaultOptions;
    QTest::newRow("enumProperty") << QStringLiteral("enumProperty.qml") << defaultOptions;
    QTest::newRow("enumsOfScrollBar") << QStringLiteral("enumsOfScrollBar.qml") << defaultOptions;
    QTest::newRow("esmodule") << QStringLiteral("esmodule.mjs") << defaultOptions;
    QTest::newRow("externalEnumProperty")
            << QStringLiteral("externalEnumProperty.qml") << defaultOptions;
    QTest::newRow("fileSelectorDuplciateImport")
            << QStringLiteral("FileSelector/main.qml") << defaultOptions;

    CallQmllintOptions withResourceFiles = defaultOptions;
    withResourceFiles.resources.append(testFile("FileSelector3/resources.qrc"));
    QTest::newRow("fileSelectorCmopatibleType")
            << QStringLiteral("FileSelector3/App.qml") << withResourceFiles;
    QTest::newRow("fileSelectorCompatibleFileSelectedType")
            << QStringLiteral("FileSelector3/+Material/App.qml") << withResourceFiles;
    QTest::newRow("fileSelectorWithoutUnselected")
            << QStringLiteral("FileSelector4/Main.qml") << defaultOptions;

    QTest::newRow("forLoop") << QStringLiteral("forLoop.qml") << defaultOptions;
    QTest::newRow("goodAlias") << QStringLiteral("goodAlias.qml") << defaultOptions;
    QTest::newRow("goodAliasObject") << QStringLiteral("goodAliasObject.qml") << defaultOptions;
    QTest::newRow("goodAttachedProperty")
            << QStringLiteral("goodAttachedProperty.qml") << defaultOptions;
    QTest::newRow("goodGeneralizedGroup")
            << QStringLiteral("goodGeneralizedGroup.qml") << defaultOptions;
    QTest::newRow("goodParent") << QStringLiteral("goodParent.qml") << defaultOptions;
    QTest::newRow("goodTypeAssertion") << QStringLiteral("goodTypeAssertion.qml") << defaultOptions;
    QTest::newRow("grouped scope failure") << QStringLiteral("groupedScope.qml") << defaultOptions;
    QTest::newRow("groupedAttachedLayout")
            << QStringLiteral("groupedAttachedLayout.qml") << defaultOptions;
    QTest::newRow("groupedProperty (valueSource+interceptor)")
            << QStringLiteral("groupedProperty_valueSource_interceptor.qml") << defaultOptions;
    QTest::newRow("groupedPropertyAssignments")
            << QStringLiteral("groupedPropertyAssignments.qml") << defaultOptions;
    QTest::newRow("ignoreWarnings") << QStringLiteral("ignoreWarnings.qml") << defaultOptions;
    QTest::newRow("importWithPrefix") << QStringLiteral("ImportWithPrefix.qml") << defaultOptions;
    QTest::newRow("initReadonly") << QStringLiteral("initReadonly.qml") << defaultOptions;
    QTest::newRow("interceptor") << QStringLiteral("interceptor.qml") << defaultOptions;
    QTest::newRow("interceptor+valueSource")
            << QStringLiteral("interceptor_valueSource.qml") << defaultOptions;
    QTest::newRow("nonConflictingDuplicateBinding.qml")
            << QStringLiteral("nonConflictingDuplicateBinding.qml") << defaultOptions;
    QTest::newRow("itemviewattached") << QStringLiteral("itemViewAttached.qml") << defaultOptions;
    QTest::newRow("javascriptMethodsInModule")
            << QStringLiteral("javascriptMethodsInModuleGood.qml") << defaultOptions;
    QTest::newRow("jsLibrary") << QStringLiteral("jsLibrary.qml") << defaultOptions;
    QTest::newRow("jsVarDeclarations") << QStringLiteral("jsVarDeclarations.qml") << defaultOptions;
    QTest::newRow("jsmoduleimport") << QStringLiteral("jsmoduleimport.qml") << defaultOptions;
    QTest::newRow("jsonArrayIsRecognized")
            << QStringLiteral("jsonArrayIsRecognized.qml") << defaultOptions;
    QTest::newRow("jsonObjectIsRecognized")
            << QStringLiteral("jsonObjectIsRecognized.qml") << defaultOptions;
    QTest::newRow("layouts depends quick") << QStringLiteral("layouts.qml") << defaultOptions;
    QTest::newRow("listPropertyMethods")
            << QStringLiteral("listPropertyMethods.qml") << defaultOptions;
    QTest::newRow("locale") << QStringLiteral("locale.qml") << defaultOptions;
    QTest::newRow("matchByName") << QStringLiteral("matchByName.qml") << defaultOptions;
    QTest::newRow("methodInScope") << QStringLiteral("MethodInScope.qml") << defaultOptions;
    QTest::newRow("methodsInJavascript")
            << QStringLiteral("javascriptMethods.qml") << defaultOptions;
    QTest::newRow("multiDefaultProperty")
            << QStringLiteral("multiDefaultPropertyOk.qml") << defaultOptions;
    QTest::newRow("multiExtension") << QStringLiteral("multiExtension.qml") << defaultOptions;
    QTest::newRow("multilineStringEscaped")
            << QStringLiteral("multilineStringEscaped.qml") << defaultOptions;
    QTest::newRow("noMissingNotify") << QStringLiteral("noMissingNotify.qml") << defaultOptions;
    QTest::newRow("noWarningBindableOnly")
            << QStringLiteral("noWarningBindableOnly.qml") << defaultOptions;
    QTest::newRow("nullBindingFunction")
            << QStringLiteral("nullBindingFunction.qml") << defaultOptions;
    QTest::newRow("objectArray") << QStringLiteral("objectArray.qml") << defaultOptions;
    QTest::newRow("objectBindingOnVarProperty")
            << QStringLiteral("objectBoundToVar.qml") << defaultOptions;
    QTest::newRow("on binding in grouped property")
            << QStringLiteral("onBindingInGroupedProperty.qml") << defaultOptions;
    QTest::newRow("onlyMajorVersion") << QStringLiteral("onlyMajorVersion.qml") << defaultOptions;
    QTest::newRow("OpaqueTest") << QStringLiteral("OpaqueTest.qml") << defaultOptions;
    QTest::newRow("optionalChainingCall")
            << QStringLiteral("optionalChainingCall.qml") << defaultOptions;
#ifdef HAS_QC_BASIC
    QTest::newRow("overlay") << QStringLiteral("overlayFromControls.qml") << defaultOptions;
#endif
    QTest::newRow("overridescript") << QStringLiteral("overridescript.qml") << defaultOptions;
    QTest::newRow("prefixedAttachedProperty")
            << QStringLiteral("prefixedAttachedProperty.qml") << defaultOptions;
    QTest::newRow("propertyBindingValue")
            << QStringLiteral("propertyBindingValue.qml") << defaultOptions;
    QTest::newRow("propertyDelegate") << QStringLiteral("propertyDelegate.qml") << defaultOptions;
    QTest::newRow("propertyMapUsage") << QStringLiteral("propertyMapUsage.qml") << defaultOptions;
    QTest::newRow("propertyOverride") << QStringLiteral("propertyOverride.qml") << defaultOptions;
    QTest::newRow("propertyWithOn") << QStringLiteral("switcher.qml") << defaultOptions;
    QTest::newRow("qjsroot") << QStringLiteral("qjsroot.qml") << defaultOptions;
    QTest::newRow("qmlRootMethods") << QStringLiteral("qmlRootMethods.qml") << defaultOptions;
    QTest::newRow("qmldirAndQmltypes") << QStringLiteral("qmldirAndQmltypes.qml") << defaultOptions;
    QTest::newRow("qmldirImportAndDepend")
            << QStringLiteral("qmldirImportAndDepend/good.qml") << defaultOptions;
    QTest::newRow("qmodelIndex") << QStringLiteral("qmodelIndex.qml") << defaultOptions;
    QTest::newRow("qtquickdialog") << QStringLiteral("qtquickdialog.qml") << defaultOptions;
    QTest::newRow("qualifiedAttached") << QStringLiteral("Drawer.qml") << defaultOptions;
    QTest::addRow("regExp") << u"regExp.qml"_s << defaultOptions;
    QTest::newRow("renamedTypeUsageFromOtherFile")
            << u"qmldirs/renameFileToMultipleNames/AnotherFileClean.qml"_s << defaultOptions;
    QTest::newRow("requiredPropertyInGroupedPropertyScope")
            << QStringLiteral("requiredPropertyInGroupedPropertyScope.qml") << defaultOptions;
    QTest::newRow("requiredPropertySetViaOnBinding")
            << QStringLiteral("requiredPropertySetViaOnBinding.qml") << defaultOptions;
    QTest::newRow("RequiredPropertyBaseWithAlias")
            << QStringLiteral("RequiredPropertyBaseWithAlias.qml") << defaultOptions;
    QTest::newRow("requiredWithRootLevelAlias")
            << QStringLiteral("RequiredWithRootLevelAlias.qml") << defaultOptions;
    QTest::newRow("required_property_in_Component")
            << QStringLiteral("requiredPropertyInComponent.qml") << defaultOptions;
    QTest::newRow("retrieveFunction") << QStringLiteral("retrieveFunction.qml") << defaultOptions;
    QTest::newRow("scopedAndUnscopedEnums") << QStringLiteral("enumValid.qml") << defaultOptions;
    QTest::newRow("segFault") << QStringLiteral("SegFault.qml") << defaultOptions;
    QTest::newRow("selfReferential") << QStringLiteral("SelfReferential.qml") << defaultOptions;
    QTest::newRow("setRequiredTroughAlias")
            << QStringLiteral("setRequiredPropertyThroughAlias.qml") << defaultOptions;
    QTest::newRow("setRequiredTroughAliasOfAlias")
            << QStringLiteral("setRequiredPropertyThroughAliasOfAlias.qml") << defaultOptions;
    QTest::newRow("shapes") << QStringLiteral("shapes.qml") << defaultOptions;
    QTest::newRow("singleElementAssignedToList")
            << QStringLiteral("singleElementAssignedToList.qml") << defaultOptions;
    QTest::newRow("stringLength") << QStringLiteral("stringLength.qml") << defaultOptions;
    QTest::newRow("stringLength2") << QStringLiteral("stringLength2.qml") << defaultOptions;
    QTest::newRow("stringLength3") << QStringLiteral("stringLength3.qml") << defaultOptions;
    QTest::newRow("stringToByteArray") << QStringLiteral("stringToByteArray.qml") << defaultOptions;
    QTest::newRow("template literal (substitution)")
            << QStringLiteral("templateStringSubstitution.qml") << defaultOptions;
    QTest::newRow("thisObject") << QStringLiteral("thisObject.qml") << defaultOptions;
    QTest::newRow("uiQml") << QStringLiteral("FormUser.qml") << defaultOptions;
    QTest::newRow("unexportedCppBase") << QStringLiteral("unexportedCppBase.qml") << defaultOptions;
    QTest::newRow("unknownBuiltinFont") << QStringLiteral("ButtonLoader.qml") << defaultOptions;
    QTest::newRow("unknownPropertyDuplicateBinding.qml")
            << QStringLiteral("unknownPropertyDuplicateBinding.qml") << defaultOptions;
    QTest::newRow("unnotifiableReadOutsideBinding")
            << QStringLiteral("unnotifiableReadOutsideBinding.qml") << defaultOptions;
    QTest::newRow("v4SequenceMethods") << QStringLiteral("v4SequenceMethods.qml") << defaultOptions;
    QTest::newRow("valueSource") << QStringLiteral("valueSource.qml") << defaultOptions;
    QTest::newRow("var") << QStringLiteral("var.qml") << defaultOptions;
    QTest::newRow("recognizeComponentWithinItself1")
            << u"recognizeComponentWithinItself/A.qml"_s << defaultOptions;
    QTest::newRow("recognizeComponentWithinItself2")
            << u"recognizeComponentWithinItself/B.qml"_s << defaultOptions;
    {
        CallQmllintOptions options = defaultOptions;
        options.resources << testFile("mymodulewithsingleton-build/.qt/rcc/app_raw_qml_0.qrc")
                          << testFile("mymodulewithsingleton-build/.qt/rcc/qmake_app.qrc");
        QTest::newRow("singletonWithEmptyPrefix")
                << u"mymodulewithsingleton-source/MyModuleWithSingleton/Singleton.qml"_s << options;
        QTest::newRow("singletonWithEmptyPrefix2")
                << u"mymodulewithsingleton-build/MyModuleWithSingleton/Singleton.qml"_s << options;
    }
}

void TestQmllint::cleanQmlCode()
{
    QFETCH(QString, filename);
    QFETCH(CallQmllintOptions, options);

    const auto guard = qScopeGuard([&]() {
        if (!options.resources.isEmpty())
            m_linter.clearCache();
    });

    const QJsonArray warnings = callQmllint(filename, options);
    checkResult(warnings, ResultBuilder::cleanResult());
}

void TestQmllint::compilerWarnings_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");
    QTest::addColumn<bool>("enableCompilerWarnings");

    QTest::newRow("listIndices")
            << QStringLiteral("listIndices.qml")
            << ResultBuilder::cleanResult()
            << true;
    QTest::newRow("lazyAndDirect")
            << QStringLiteral("LazyAndDirect/Lazy.qml") << ResultBuilder::cleanResult() << true;
    QTest::newRow("qQmlV4Function")
            << QStringLiteral("varargs.qml")
            << ResultBuilder::cleanResult()
            << true;
    QTest::newRow("multiGrouped")
            << QStringLiteral("multiGrouped.qml")
            << ResultBuilder::cleanResult()
            << true;

    QTest::newRow("shadowable")
            << QStringLiteral("shadowable.qml")
            << ResultBuilder::singleExpected("with type NotSoSimple can be shadowed"_L1)
            << true;
    QTest::newRow("tooFewParameters")
            << QStringLiteral("tooFewParams.qml")
            << ResultBuilder::singleExpected(
                   "Could not compile binding for a: No matching override found"_L1)
            << true;
    QTest::newRow("javascriptVariableArgs")
            << QStringLiteral("javascriptVariableArgs.qml")
            << ResultBuilder()
               .addExpected("Could not compile binding for onCompleted: "
                            "Function expects 0 arguments, but 2 were provided"_L1)
               .build()
            << true;
    QTest::newRow("unknownTypeInRegister")
            << QStringLiteral("unknownTypeInRegister.qml")
            << ResultBuilder()
               .addExpected("Could not determine signature of function foo: "
                            "Functions without type annotations won't be compiled"_L1)
               .build()
            << true;
    QTest::newRow("pragmaStrict")
            << QStringLiteral("pragmaStrict.qml")
            << ResultBuilder()
               .addExpected("Could not determine signature of function add: "
                            "Functions without type annotations won't be compiled"_L1)
               .build()
            << true;
    QTest::newRow("generalizedGroupHint")
            << QStringLiteral("generalizedGroupHint.qml")
            << ResultBuilder()
               .addExpected("Could not determine signature of binding for myColor: "
                            "Could not find property \"myColor\". "
                            "You may want use ID-based grouped properties here."_L1)
               .build()
            << true;
    QTest::newRow("invalidIdLookup")
            << QStringLiteral("invalidIdLookup.qml")
            << ResultBuilder()
               .addExpected("Could not compile binding for objectName: "
                            "Cannot retrieve a non-object type by ID: stateMachine"_L1)
               .build()
            << true;
    QTest::newRow("returnTypeAnnotation-component")
            << QStringLiteral("returnTypeAnnotation_component.qml")
            << ResultBuilder()
               .addExpected("Could not compile function comp: function without return type "
                            "annotation returns returnTypeAnnotation_component::c with type Comp. "
                            "This may prevent proper compilation to Cpp."_L1)
               .build()
            << true;
    QTest::newRow("returnTypeAnnotation-enum")
            << QStringLiteral("returnTypeAnnotation_enum.qml")
            << ResultBuilder()
               .addExpected("Could not compile function enumeration: function without return type "
                            "annotation returns QQuickText::HAlignment::AlignRight. "
                            "This may prevent proper compilation to Cpp."_L1)
               .build()
            << true;
    QTest::newRow("returnTypeAnnotation-method")
            << QStringLiteral("returnTypeAnnotation_method.qml")
            << ResultBuilder()
               .addExpected("Could not compile function method: function without return type "
                            "annotation returns returnTypeAnnotation_method::f(...). This may "
                            "prevent proper compilation to Cpp."_L1)
               .build()
            << true;
    QTest::newRow("returnTypeAnnotation-property")
            << QStringLiteral("returnTypeAnnotation_property.qml")
            << ResultBuilder()
               .addExpected("Could not compile function prop: function without return type "
                            "annotation returns returnTypeAnnotation_property::i with type int. "
                            "This may prevent proper compilation to Cpp."_L1)
               .build()
            << true;
    QTest::newRow("returnTypeAnnotation-type")
            << QStringLiteral("returnTypeAnnotation_type.qml")
            << ResultBuilder()
               .addExpected("Could not compile function type: function without return type "
                            "annotation returns double. This may prevent proper compilation to "
                            "Cpp."_L1)
               .build()
            << true;

    QTest::newRow("functionAssign1")
            << QStringLiteral("functionAssign1.qml") << ResultBuilder::cleanResult() << true;
    QTest::newRow("functionAssign2")
            << QStringLiteral("functionAssign2.qml") << ResultBuilder::cleanResult() << true;

    // We want to see the warning about the missing property only once.
    QTest::newRow("unresolvedType2")
            << QStringLiteral("unresolvedType2.qml")
            << ResultBuilder()
               .addExpected("Could not determine signature of binding for text: "
                            "Could not find property \"text\"."_L1)
               .addUnexpected("Cannot resolve property type  for binding on text."_L1)
               .build()
            << true;
}

void TestQmllint::compilerWarnings()
{
    QFETCH(QString, filename);
    QFETCH(Result, result);
    QFETCH(bool, enableCompilerWarnings);

    auto categories = QQmlJSLogger::builtinCategories();

    auto category = std::find_if(categories.begin(), categories.end(), [](const QQmlJS::LoggerCategory& category) {
        return category.id() == qmlCompiler;
    });
    Q_ASSERT(category != categories.end());

    if (enableCompilerWarnings)
        category->setSeverity(QQmlJS::WarningSeverity::Warning);

    runTest(filename, result, {}, {}, {}, UseDefaultImports, &categories);
}

void TestQmllint::batches_data()
{

    QTest::addColumn<Batch>("batch");

    QTest::addRow("AConstructB")
            << Batch().addCleanFile("AConstructB/A.qml"_L1).addCleanFile("AConstructB/B.qml"_L1);
    QTest::addRow("BConstructA")
            << Batch().addCleanFile("BConstructA/A.qml"_L1).addCleanFile("BConstructA/B.qml"_L1);

    QTest::addRow("BadRecursiveConstruction")
            << Batch().addCleanFile("BadRecursiveConstruction/A.qml"_L1)
                       .addDirtyFile("BadRecursiveConstruction/B.qml"_L1,
                                     ResultBuilder()
                                             .addExpected("Could not find property \"hello\""_L1)
                                             .build());

    QTest::addRow("RecursiveGroupedProperties")
            << Batch().addCleanFile("RecursiveGroupedProperties/A.qml"_L1)
                       .addCleanFile("RecursiveGroupedProperties/B.qml"_L1);

    QTest::addRow("RecursiveLists") << Batch().addCleanFile("RecursiveLists/A.qml"_L1)
                                               .addCleanFile("RecursiveLists/B.qml"_L1);
}

void TestQmllint::batches()
{
    QFETCH(Batch, batch);

    QQmlJSLinter::LintOptions options;
    options.setFlag(QQmlJSLinter::Silent);
    options.setFlag(QQmlJSLinter::GenerateJson);

    for (const auto &element : std::as_const(batch.elements)) {
        QVERIFY(m_linter.prepareFileForBatchLinting(testFile("batches/" + element.file), nullptr,
                                                    options, m_defaultImportPaths, { }, { },
                                                    m_categories));
    }

    for (const auto &element : std::as_const(batch.elements)) {
        QQmlJSLinter::Result result = m_linter.lintFileInBatch(testFile("batches/" + element.file));
        checkResult(result.json["warnings"].toArray(), element.result);
    }
}

void TestQmllint::cycles_data()
{
    QTest::addColumn<QString>("filename");
    const QString dataFolder = testFile("cycles");

    for (const auto &file : QDirListing(dataFolder, QDirListing::IteratorFlag::Recursive)) {
        if (!file.isFile())
            continue;
        QTest::addRow("%s", qPrintable(QStringView(file.filePath().mid(dataFolder.size() + 1))))
                << file.filePath();
        QQmlJSLinter::LintOptions options;
        options.setFlag(QQmlJSLinter::Silent);
        options.setFlag(QQmlJSLinter::GenerateJson);
        m_linter.prepareFileForBatchLinting(file.filePath(), nullptr, options, m_defaultImportPaths,
                                            { }, { }, m_categories);
    }
}

void TestQmllint::cycles()
{
    QFETCH(QString, filename);

    QQmlJSLinter::Result result = m_linter.lintFileInBatch(filename);

    QCOMPARE(result.status, QQmlJSLinter::LintSuccess);
}

void TestQmllint::jsFileInBatch() {
    QQmlJSLinter::LintOptions options = QQmlJSLinter::LintOption::GenerateJson;
    std::array filenames {"SharedFunctions.js", "dontCheckJSTypes.qml"};
    for (const auto& filename: filenames) {
        m_linter.prepareFileForBatchLinting(testFile(filename), nullptr, options,
                                            QLibraryInfo::paths(QLibraryInfo::QmlImportsPath), { },
                                            { }, m_categories);
    }

    for (const auto& filename: filenames) {
        QQmlJSLinter::Result result = m_linter.lintFileInBatch(testFile(filename));
        checkResult(result.json["warnings"].toArray(), ResultBuilder::cleanResult());
    }
}

QString TestQmllint::runQmllint(const QString &fileToLint,
                                std::function<void(QProcess &)> handleResult,
                                const QStringList &extraArgs, bool ignoreSettings,
                                bool addImportDirs, bool absolutePath, const Environment &env)
{
    auto qmlImportDir = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    QStringList args;

    QString absoluteFilePath =
            QFileInfo(fileToLint).isAbsolute() ? fileToLint : testFile(fileToLint);

    args << QFileInfo(absoluteFilePath).fileName();

    if (addImportDirs) {
        args << QStringLiteral("-I") << qmlImportDir
             << QStringLiteral("-I") << dataDirectory();
    }

    if (ignoreSettings)
        args << QStringLiteral("--ignore-settings");

    if (absolutePath)
        args << QStringLiteral("--absolute-path");

    args << extraArgs;
    args << QStringLiteral("--silent");
    QString errors;
    auto verify = [&](bool isSilent) {
        QProcess process;
        QProcessEnvironment processEnv = QProcessEnvironment::systemEnvironment();
        for (const auto &entry : env)
            processEnv.insert(entry.first, entry.second);

        process.setProcessEnvironment(processEnv);
        process.setWorkingDirectory(QFileInfo(absoluteFilePath).absolutePath());
        process.start(m_qmllintPath, args);
        handleResult(process);
        errors = process.readAllStandardError();

        QStringList lines = errors.split(u'\n', Qt::SkipEmptyParts);

        auto end = std::remove_if(lines.begin(), lines.end(), [](const QString &line) {
            return !line.startsWith("Warning: ") && !line.startsWith("Error: ");
        });

        std::sort(lines.begin(), end);
        auto it = std::unique(lines.begin(), end);
        if (it != end) {
            qDebug() << "The warnings and errors were generated more than once:";
            do {
                qDebug() << *it;
            } while (++it != end);
            QTest::qFail("Duplicate warnings and errors", __FILE__, __LINE__);
        }

        if (isSilent) {
            QTest::qVerify(errors.isEmpty(), "errors.isEmpty()", "Silent mode outputs messages",
                           __FILE__, __LINE__);
        }

        if (QTest::currentTestFailed()) {
            qDebug().noquote() << "Command:" << process.program() << args.join(u' ');
            qDebug() << "Exit status:" << process.exitStatus();
            qDebug() << "Exit code:" << process.exitCode();
            qDebug() << "stderr:" << errors;
            qDebug() << "stdout:" << process.readAllStandardOutput();
        }
    };
    verify(true);
    args.removeLast();
    verify(false);
    return errors;
}

QString TestQmllint::runQmllint(const QString &fileToLint, bool shouldSucceed,
                                const QStringList &extraArgs, bool ignoreSettings,
                                bool addImportDirs, bool absolutePath, const Environment &env)
{
    return runQmllint(
            fileToLint,
            [&](QProcess &process) {
                QVERIFY(process.waitForFinished());
                QCOMPARE(process.exitStatus(), QProcess::NormalExit);

                if (shouldSucceed)
                    QCOMPARE(process.exitCode(), 0);
                else
                    QVERIFY(process.exitCode() != 0);
            },
            extraArgs, ignoreSettings, addImportDirs, absolutePath, env);
}

static void writeQrcFileMapping(const QHash<QString, QString> &mapping, const QString &outputFile)
{
    QFile file(outputFile);
    QVERIFY(file.open(QFile::WriteOnly));

    QXmlStreamWriter writer(&file);
    writer.writeStartDocument();
    writer.writeStartElement("RCC"_L1);
    writer.writeStartElement("qresource"_L1);
    writer.writeAttribute("prefix"_L1, ""_L1);
    for (const auto &pair : mapping.asKeyValueRange()) {
        writer.writeStartElement("file"_L1);
        writer.writeAttribute("alias"_L1, pair.first);
        writer.writeCharacters(pair.second);
        writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
}

QJsonArray TestQmllint::callQmllintImpl(const QString &fileToLint, const QString &content,
                                        const CallQmllintOptions &options, CallQmllintChecks checks)
{
    QJsonArray result;

    const QFileInfo info = QFileInfo(fileToLint);
    const QString lintedFile = info.isAbsolute() ? fileToLint : testFile(fileToLint);

    QQmlJSLinter::Result lintResult;
    QQmlJSLinter::LintOptions lintOptions;
    lintOptions.setFlag(QQmlJSLinter::Silent);
    lintOptions.setFlag(QQmlJSLinter::GenerateJson);

    auto &plugins = m_linter.plugins();

    for (auto &plugin : plugins)
        plugin.setEnabled(options.plugins.contains(plugin.name()));


    const QStringList resolvedImportPaths = options.defaultImports == UseDefaultImports
            ? m_defaultImportPaths + options.importPaths
            : options.importPaths;
    if (options.type == LintFile) {
        QList<QQmlJS::LoggerCategory> resolvedCategories =
                options.categories != nullptr ? *options.categories : m_categories;

        for (const auto &[cat, severity] : options.categorySeverityOverrides.asKeyValueRange()) {
            for (QQmlJS::LoggerCategory &category : resolvedCategories) {
                if (category.name() != cat)
                    continue;
                category.setSeverity(severity);
                break;
            }
        }

        if (options.readSettings) {
            QQmlToolingSettings settings(QLatin1String("qmllint"), { "General"_L1, "Warnings"_L1 });
            if (settings.search(lintedFile).isValid())
                QQmlJS::LoggingUtils::updateLogSeverities(resolvedCategories, settings, nullptr);
        }

        QList<QString> resourceFiles = options.resources;
        QTemporaryDir qrcFileDir;
        if (!options.qrcToFilePaths.isEmpty()) {
            [&qrcFileDir] { QVERIFY(qrcFileDir.isValid()); }();
            const QString qrcFile = qrcFileDir.filePath("a.qrc");
            writeQrcFileMapping(options.qrcToFilePaths, qrcFile);
            resourceFiles.append(qrcFile);
        }

        QTest::failOnWarning();
        lintResult = m_linter.lintFile(lintedFile, content.isEmpty() ? nullptr : &content,
                                       lintOptions, resolvedImportPaths, options.qmldirFiles,
                                       resourceFiles, resolvedCategories);
    } else {
        lintResult = m_linter.lintModule(fileToLint, lintOptions, resolvedImportPaths,
                                         options.resources);
    }

    [&]() {
        const bool success = lintResult.status == QQmlJSLinter::LintSuccess;
        const QByteArray errorOutput = QJsonDocument(lintResult.json).toJson();
        QVERIFY2(success == (checks.testFlag(ShouldSucceed)), errorOutput);
        result = lintResult.json[u"warnings"_s].toArray();
    }();

    if (lintResult.status == QQmlJSLinter::LintSuccess
        || lintResult.status == QQmlJSLinter::HasWarnings) {
        testFixes(lintResult.logger.get(), checks.testFlag(ShouldSucceed), options.importPaths,
                  options.qmldirFiles, options.resources, options.defaultImports,
                  options.categories, checks.testFlag(HasAutoFix), options.readSettings,
                  info.baseName() + u".fixed.qml"_s);
    }
    return result;
}

QJsonArray TestQmllint::callQmllint(const QString &fileToLint, const CallQmllintOptions &options,
                                    CallQmllintChecks checks)
{
    return callQmllintImpl(fileToLint, QString(), options, checks);
}

QJsonArray TestQmllint::callQmllintOnSnippet(const QString &snippet,
                                             const CallQmllintOptions &options,
                                             CallQmllintChecks checks)
{
    m_linter.prepareScopeForReuse(testFile("Snippet.qml"));

    if (options.rootUrls.isEmpty())
        return callQmllintImpl("Snippet.qml", snippet, options, checks);
    QTemporaryDir dir;
    [&dir]() { QVERIFY(dir.isValid()); }();
    if (QTest::currentTestFailed())
        return { };
    const auto contextProperties =
            QQmlJS::HeuristicContextProperties::collectFromCppSourceDirs(options.rootUrls);
    contextProperties.writeCache(dir.path());
    return callQmllintImpl(dir.filePath("Snippet.qml"), snippet, options, checks);
}

void TestQmllint::testFixes(QQmlJSLogger *logger, bool shouldSucceed, QStringList importPaths,
                            QStringList qmldirFiles, QStringList resources,
                            DefaultImportOption defaultImports,
                            QList<QQmlJS::LoggerCategory> *categories, bool autoFixable,
                            bool readSettings, const QString &fixedPath)
{
    QString fixedCode;
    QQmlJSLinter::FixResult fixResult = QQmlJSLinter::applyFixes(logger, &fixedCode, true);

    if (autoFixable) {
        QCOMPARE(fixResult, QQmlJSLinter::FixSuccess);
        // Check that the fixed version of the file actually passes qmllint now
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QFile file(dir.filePath("Fixed.qml"));
        QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(file.errorString()));
        file.write(fixedCode.toUtf8());
        file.flush();
        file.close();

        CallQmllintOptions options;
        options.importPaths = importPaths;
        options.qmldirFiles = qmldirFiles;
        options.resources = resources;
        options.defaultImports = defaultImports;
        options.categories = categories;
        options.readSettings = readSettings;

        callQmllint(QFileInfo(file).absoluteFilePath(), options);

        if (QFileInfo(fixedPath).exists()) {
            QFile fixedFile(fixedPath);
            QVERIFY(fixedFile.open(QFile::ReadOnly));
            QString fixedFileContents = QString::fromUtf8(fixedFile.readAll());
#ifdef Q_OS_WIN
            fixedCode = fixedCode.replace(u"\r\n"_s, u"\n"_s);
            fixedFileContents = fixedFileContents.replace(u"\r\n"_s, u"\n"_s);
#endif

            QCOMPARE(fixedCode, fixedFileContents);
        }
    } else {
        if (shouldSucceed)
            QCOMPARE(fixResult, QQmlJSLinter::NothingToFix);
        else
            QVERIFY(fixResult == QQmlJSLinter::FixSuccess
                    || fixResult == QQmlJSLinter::NothingToFix);
    }
}

void TestQmllint::runTest(const QString &testFile, const Result &result, QStringList importDirs,
                          QStringList qmltypesFiles, QStringList resources,
                          DefaultImportOption defaultImports,
                          QList<QQmlJS::LoggerCategory> *categories)
{
    CallQmllintOptions options;
    options.importPaths = importDirs;
    options.qmldirFiles = qmltypesFiles;
    options.resources = resources;
    options.defaultImports = defaultImports;
    options.categories = categories;
    options.readSettings = result.flags.testFlag(Result::Flag::UseSettings);

    const QJsonArray warnings = callQmllint(testFile, options, fromResultFlags(result.flags));
    checkResult(warnings, result);
}

void TestQmllint::runTest(const QString &testFile, const Result &result, CallQmllintOptions options)
{
    options.readSettings = result.flags.testFlag(Result::Flag::UseSettings);

    const QJsonArray warnings = callQmllint(testFile, options, fromResultFlags(result.flags));
    checkResult(warnings, result);
}

static QtMsgType typeStringToMsgType(const QString &type)
{
    if (type == u"debug")
        return QtDebugMsg;
    if (type == u"info")
        return QtInfoMsg;
    if (type == u"warning")
        return QtWarningMsg;
    if (type == u"critical")
        return QtCriticalMsg;
    if (type == u"fatal")
        return QtFatalMsg;

    Q_UNREACHABLE();
}

struct SimplifiedWarning
{
    QString message;
    quint32 line;
    quint32 column;
    QtMsgType type;

    SimplifiedWarning(QJsonValueConstRef warning)
        : message(warning[u"message"].toString()),
          line(warning[u"line"].toInt()),
          column(warning[u"column"].toInt()),
          type(typeStringToMsgType(warning[u"type"].toString()))
    {
    }

    QString toString() const { return u"%1:%2: %3"_s.arg(line).arg(column).arg(message); }

    std::tuple<QString, quint32, quint32, QtMsgType> asTuple() const
    {
        return std::make_tuple(message, line, column, type);
    }

    friend bool comparesEqual(const SimplifiedWarning& a, const SimplifiedWarning& b) noexcept
    {
        return a.asTuple() == b.asTuple();
    }
    friend Qt::strong_ordering compareThreeWay(const SimplifiedWarning& a, const SimplifiedWarning& b) noexcept {
        return QtOrderingPrivate::compareThreeWayMulti(a.asTuple(), b.asTuple());
    }
    Q_DECLARE_STRONGLY_ORDERED(SimplifiedWarning)
};

template<typename ExpectedMessageFailureHandler, typename BadMessageFailureHandler,
         typename ReplacementFailureHandler>
void TestQmllint::checkResult(const QJsonArray &warnings, const Result &result,
                              ExpectedMessageFailureHandler onExpectedMessageFailures,
                              BadMessageFailureHandler onBadMessageFailures,
                              ReplacementFailureHandler onReplacementFailures)
{
    if (result.flags.testFlag(Result::Flag::NoMessages))
        QVERIFY2(warnings.isEmpty(), qPrintable(QJsonDocument(warnings).toJson()));

    for (const Message &msg : result.expectedMessages) {
        // output.contains() expect fails:
        onExpectedMessageFailures();

        searchWarnings(warnings, msg.text, msg.severity, msg.line, msg.column);
    }

    for (const Message &msg : result.unexpectedMessages) {
        // !output.contains() expect fails:
        onBadMessageFailures();

        searchWarnings(warnings, msg.text, msg.severity, msg.line, msg.column, StringNotContained);
    }

    for (const Fix &fix: result.expectedFixes) {
        onReplacementFailures();
        searchFixes(warnings, fix.text, fix.edits, fix.line, fix.column);
    }

    // check for duplicates
    QList<SimplifiedWarning> sortedWarnings;
    std::transform(warnings.begin(), warnings.end(), std::back_inserter(sortedWarnings),
                   [](QJsonValueConstRef ref) { return SimplifiedWarning(ref); });
    std::sort(sortedWarnings.begin(), sortedWarnings.end());
    const auto firstDuplicate =
            std::adjacent_find(sortedWarnings.constBegin(), sortedWarnings.constEnd());
    for (auto it = firstDuplicate; it != sortedWarnings.constEnd();
         it = std::adjacent_find(it + 1, sortedWarnings.constEnd())) {
        qDebug() << "Found duplicate warning: " << it->toString();
    }
    QVERIFY2(firstDuplicate == sortedWarnings.constEnd(), "Found duplicate warnings!");
}

static bool foundAllExpectedEdits(const TestQmllint::Edits &expectedEdits,
                                  const QJsonArray &actualEdits)
{
    for (const TestQmllint::Edit &expectedEdit : expectedEdits) {
        bool found = false;
        for (const auto &actualEdit : actualEdits) {
            auto replacement = actualEdit[u"replacement"_s].toString();

#ifdef Q_OS_WIN
            // Replacements can contain native line endings
            // but we need them to be uniform in order for them to conform to our test data
            replacement = replacement.replace(u"\r\n"_s, u"\n"_s);
#endif

            if (replacement != expectedEdit.replacement)
                continue;

            if (expectedEdit.line != 0 || expectedEdit.column != 0) {
                const auto actualLocation = actualEdit[u"location"_s].toObject();
                if (actualLocation[u"line"_s].toInt() != int(expectedEdit.line))
                    continue;
                if (actualLocation[u"column"_s].toInt() != int(expectedEdit.column))
                    continue;
            }

            found = true;
            break;
        }

        if (!found)
            return false;
    }
    return true;
}

static bool warningsContainFix(const QJsonArray &warnings, const QString &substring,
                                 const TestQmllint::Edits &edits, quint32 line, quint32 column)
{
    for (const QJsonValueConstRef warningJson : warnings) {
        for (const QJsonValueConstRef fix : warningJson[u"suggestions"].toArray()) {
            if (!fix[u"message"].toString().contains(substring))
                continue;

            if (line != 0 || column != 0) {
                const quint32 fixLine = fix[u"line"].toInt();
                const quint32 fixColumn = fix[u"column"].toInt();
                if (fixLine != line || fixColumn != column)
                    continue;
            }

            const QJsonArray editsJson = fix[u"documentEdits"_s].toArray();
            if (!foundAllExpectedEdits(edits, editsJson))
                continue;

            return true;
        }
    }
    return false;
}

void TestQmllint::searchFixes(const QJsonArray &warnings, const QString &substring,
                              const Edits &edits, quint32 line, quint32 column)
{
    const bool contains = warningsContainFix(warnings, substring, edits, line, column);
    const auto toDescription = [](const QJsonArray &warnings, const QString &substring,
                                  quint32 line, quint32 column) {
        const auto indentedJson =
                QString::fromUtf8(QJsonDocument(warnings).toJson(QJsonDocument::Indented));
        QString msg = QStringLiteral("qmllint output:\n%1\nIt must contain replacement '%2'")
                              .arg(indentedJson, substring);
        if (line != 0 || column != 0)
            msg += u" (%1:%2)"_s.arg(line).arg(column);

        return msg;
    };

    if (!contains)
        qDebug().noquote() << toDescription(warnings, substring, line, column);
    QVERIFY(contains);
}

void TestQmllint::searchWarnings(const QJsonArray &warnings, const QString &substring,
                                 QtMsgType type, quint32 line, quint32 column,
                                 ContainOption shouldContain)
{
    bool contains = false;

    for (const QJsonValueConstRef warningJson : warnings) {
        SimplifiedWarning warning(warningJson);

        if (warning.message.contains(substring)) {
            bool locationMismatch = false;
            if (line != 0 || column != 0) {
                if (warning.line != line || warning.column != column)
                    locationMismatch = true;
            }

            if (warning.type == type && !locationMismatch) {
                contains = true;
                break;
            }
        }

        for (const QJsonValueConstRef fix : warningJson[u"suggestions"].toArray()) {
            const QString fixMessage = fix[u"message"].toString();
            if (fixMessage.contains(substring)) {
                contains = true;
                break;
            }
        }
    }

    const auto toDescription = [](const QJsonArray &warnings, const QString &substring,
                                  quint32 line, quint32 column, bool must = true) {
        QString msg = QStringLiteral("qmllint output:\n%1\nIt %2 contain '%3'")
                              .arg(QString::fromUtf8(
                                           QJsonDocument(warnings).toJson(QJsonDocument::Indented)),
                                   must ? u"must" : u"must NOT", substring);
        if (line != 0 || column != 0)
            msg += u" (%1:%2)"_s.arg(line).arg(column);

        return msg;
    };

    if (shouldContain == StringContained) {
        if (!contains)
            qDebug().noquote() << toDescription(warnings, substring, line, column);
        QVERIFY(contains);
    } else {
        if (contains)
            qDebug().noquote() << toDescription(warnings, substring, line, column, false);
        QVERIFY(!contains);
    }
}

void TestQmllint::requiredProperty()
{
    runTest("requiredProperty.qml", ResultBuilder::cleanResult());

    runTest("requiredMissingProperty.qml",
            ResultBuilder()
            .addExpected("Property \"foo\" was marked as required but does not exist."_L1)
            .build());

    runTest("requiredPropertyBindings.qml", ResultBuilder::cleanResult());
    runTest("requiredPropertyBindingsNow.qml",
            ResultBuilder()
            .addExpected("Component is missing required property required_now_string from Base"_L1)
            .addExpected("Component is missing required property required_defined_here_string from Derived"_L1)
            .build());
    runTest("requiredPropertyBindingsLater.qml",
            ResultBuilder()
            .addExpected("Component is missing required property required_later_string from Base"_L1)
            .addExpected("Property marked as required in Derived"_L1)
            .addExpected("Component is missing required property required_even_later_string "
                         "from Base (marked as required by here)"_L1)
            .build());
}

void TestQmllint::settingsFile()
{
    QVERIFY(runQmllint("settings/unqualifiedSilent/unqualified.qml", true, warningsShouldFailArgs(), false)
                    .isEmpty());
    QVERIFY(runQmllint("settings/unusedImportWarning/unused.qml", false, warningsShouldFailArgs(), false)
                    .contains(QStringLiteral("Warning: %1:2:1: Unused import")
                                      .arg(testFile("settings/unusedImportWarning/unused.qml"))));
    QVERIFY(runQmllint("settings/bare/bare.qml", false, warningsShouldFailArgs(), false, false)
                    .contains(
                            u"Failed to import QtQuick. Are your import paths set up properly?"_s));
    QVERIFY(runQmllint("settings/qmltypes/qmltypes.qml", false, warningsShouldFailArgs(), false)
                    .contains(QStringLiteral("not a qmldir file. Assuming qmltypes.")));
    QVERIFY(runQmllint("settings/qmlimports/qmlimports.qml", true, warningsShouldFailArgs(), false).isEmpty());
}

void TestQmllint::additionalImplicitImport()
{
    // We're polluting the resource file system here, so let's clean up afterwards.
    const auto guard = qScopeGuard([this]() {m_linter.clearCache(); });
    runTest("additionalImplicitImport.qml", ResultBuilder::cleanResult(), {}, {},
            { testFile("implicitImportResource.qrc") });
}

void TestQmllint::qrcUrlImport()
{
    const auto guard = qScopeGuard([this]() { m_linter.clearCache(); });
    CallQmllintOptions options;
    options.resources.append(testFile("untitled/qrcUrlImport.qrc"));

    const QJsonArray warnings = callQmllint(testFile("untitled/main.qml"), options);
    checkResult(warnings, ResultBuilder::cleanResult());
}

void TestQmllint::incorrectImportFromHost_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<Result>("result");

    QTest::newRow("NonexistentFile")
            << QStringLiteral("importNonexistentFile.qml")
            << ResultBuilder()
               .addExpected("File or directory you are trying to import does not exist"_L1, 1, 1)
               .build();
#ifndef Q_OS_WIN
    // there is no /dev/null device on Win
    QTest::newRow("NullDevice")
            << QStringLiteral("importNullDevice.qml")
            << ResultBuilder()
               .addExpected("is neither a file nor a directory. Are sure the import path is "
                            "correct?"_L1, 1, 1)
               .build();
#endif
}

void TestQmllint::incorrectImportFromHost()
{
    QFETCH(QString, filename);
    QFETCH(Result, result);

    runTest(filename, result);
}

void TestQmllint::attachedPropertyReuse()
{
    auto categories = QQmlJSLogger::builtinCategories();
    auto category = std::find_if(categories.begin(), categories.end(), [](const QQmlJS::LoggerCategory& category) {
        return category.id() == qmlAttachedPropertyReuse;
    });
    Q_ASSERT(category != categories.end());

    CallQmllintOptions options;
    options.categorySeverityOverrides[qmlAttachedPropertyReuse.name().toString()] = QQmlSA::WarningSeverity::Warning;
    options.plugins << "Quick"_L1;

    runTest("attachedPropNotReused.qml",
            ResultBuilder()
            .addExpected("Using attached type QQuickKeyNavigationAttached already initialized in a "
                         "parent scope"_L1)
            .build(), options);

    runTest("attachedPropEnum.qml", ResultBuilder::cleanResult(), {}, {}, {}, UseDefaultImports, &categories);
    runTest("MyStyle/ToolBar.qml",
            ResultBuilder()
            .addExpected("Using attached type MyStyle already initialized in a parent scope"_L1, 10, 16)
            .addFix("Reference it by id instead"_L1, Edit{ "control."_L1, 10, 16 })
            .setFlag(Result::AutoFixable)
            .build(), options);
    runTest("pluginQuick_multipleAttachedPropertyReuse.qml",
            ResultBuilder::singleExpected(
                "Using attached type Test already initialized in a parent scope"_L1),
            options);
}

void TestQmllint::missingBuiltinsNoCrash()
{
    // We cannot use the normal linter here since the other tests might have cached the builtins
    // alread
    TestLinter linter(m_defaultImportPaths);

    QQmlJSLinter::LintOptions lintOptions;
    lintOptions.setFlag(QQmlJSLinter::Silent);
    lintOptions.setFlag(QQmlJSLinter::GenerateJson);
    QQmlJSLinter::Result result = linter.lintFile(testFile("missingBuiltinsNoCrash.qml"), nullptr,
                                                  lintOptions, { }, { }, { }, { });
    QVERIFY2(result.status != QQmlJSLinter::LintSuccess, QJsonDocument(result.json).toJson());

    checkResult(result.json[u"warnings"_s].toArray(),
                ResultBuilder::singleExpected(u"Failed to import QtQuick. Are your import paths "
                                              "set up properly?"_s));
}

void TestQmllint::stubQtQuickNoCrash()
{
    // QTBUG-149360: A module that resolves without usable type information, such
    // as a stub that only contains "module QtQuick", used to crash the Quick lint
    // plugin on two independent paths:
    // * when StateNoItemChildrenValidator::shouldRun dereferenced a null Element.
    // * when GenericPass::resolveAttached dereferenced a null scope (obtained
    //   from a null Element).
    // qmllint has to exit normally instead of segfaulting.
    QProcess process;
    process.setWorkingDirectory(testFile("stubQuick"));
    process.start(m_qmllintPath,
                  warningsShouldFailArgs()
                          << QStringLiteral("--bare")
                          << QStringLiteral("-I")
                          << QFileInfo(testFile("stubQuick")).absoluteFilePath()
                          << QStringLiteral("test.qml"));

    QVERIFY(process.waitForFinished());

    // The point of this test is that qmllint terminates normally instead of
    // segfaulting.
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QVERIFY(process.exitCode() != 0);

    // The type information is unusable, so warnings are expected.
    QVERIFY(process.readAllStandardError().contains("Item was not found"));
}

void TestQmllint::missingRegistration_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<Result>("result");
    QTest::addColumn<CallQmllintOptions>("options");

    CallQmllintOptions defaultOptions;

    QTest::newRow("alias") << uR"(import MissingRegistration
import QtQuick
Item {
    MyObject { id: obj }
    property alias myAlias: obj.asdf
})"_s
                           << ResultBuilder()
                                      .addExpected("Failed to import MissingRegistration"_L1)
                                      .addExpected("MyObject was not found"_L1)
                                      .addFix("Did you mean \"QtObject\"?"_L1)
                                      .addUnexpected("Cannot resolve alias \"myAlias\""_L1)
                                      .build()
                           << defaultOptions;

    QTest::newRow("alias2") << uR"(import MissingRegistration
import QtQuick
Item {
    id: root
    property MyObject myObj
    property alias myAlias2: root.myObj.someProperty
})"_s
                            << ResultBuilder()
                                       .addExpected("Failed to import MissingRegistration"_L1)
                                       .addExpected("MyObject was not found"_L1, 5, 5)
                                       .addUnexpected("Cannot deduce type of alias \"myAlias2\""_L1)
                                       .build()
                            << defaultOptions;

    QTest::newRow("attachedType")
            << uR"(import MissingRegistration
import QtQuick
Item {
    MyObject.onHelloWorld: function() {}
})"_s
            << ResultBuilder()
                       .addExpected("unknown attached property scope MyObject"_L1)
                       .addUnexpected("Type MyObject is used but it is not resolved"_L1)
                       .build()
            << defaultOptions;

    QTest::newRow("attachedType2")
            << uR"(import MissingRegistration
import QtQuick
MyItem {
    Component.onCompleted: function() {}
    Component.thisPropertyDoesNotExist: function() {}
})"_s
            << ResultBuilder()
                       .addExpected("MyItem was not found"_L1, 3, 1)
                       .addExpected("Could not find property \"thisPropertyDoesNotExist\""_L1, 5,
                                    15)
                       .addUnexpected("unknown attached property scope Component"_L1)
                       .addUnexpected("Type MyItem is used but it is not resolved"_L1)
                       .addUnexpected("Component"_L1)
                       .addUnexpected("onCompleted"_L1)
                       .build()
            << defaultOptions;

    QTest::newRow("defaultProperty")
            << uR"(import MissingRegistration
import QtQuick
Item {
    component IC: Item { default property MyObject myp }
    IC {
        MyObject{}
    }
})"_s
            << ResultBuilder()
                       .addExpected(
                               "MyObject was not found. Did you add all imports and dependencies?"_L1,
                               4, 26)
                       .addExpected(
                               "MyObject was not found. Did you add all imports and dependencies?"_L1,
                               6, 9)
                       .addUnexpected("incomplete"_L1)
                       .build()
            << defaultOptions;

    QTest::newRow("groupedProperty") << uR"(import MissingRegistration
MyObject {
    foo.bar: "hello"
    foo.bar.foo.bar: 1234
})"_s
                                     << ResultBuilder()
                                                .addExpected("MyObject was not found"_L1, 2, 1)
                                                .addUnexpected("foo"_L1)
                                                .addUnexpected("bar"_L1)
                                                .build()
                                     << defaultOptions;

    QTest::newRow("missingType") << u"function f() {return MySingleton.someProperty; }"_s
                                 << ResultBuilder()
                                    .addExpected("MySingleton was not found"_L1, 1, 22)
                                    .addUnexpected("someProperty"_L1)
                                    .build()
                                 << defaultOptions;

    QTest::newRow("propertyChangeHandler")
            << u"MyItem { onMyPropertyChanged: {console.log(\"hello\");}"_s
            << ResultBuilder().addUnexpected("onMyPropertyChanged"_L1).build() << defaultOptions;

    QTest::newRow("signalChangehandler")
            << u"MyItem { onMySignal: {console.log(\"hello\");} }"_s
            << ResultBuilder().addUnexpected("onMySignal"_L1).build() << defaultOptions;

    QTest::newRow("objectBindings")
            << uR"(import MissingRegistration
import QtQuick
Item {
    property MyObject1 myObj: MyObject2{}
})"_s
            << ResultBuilder()
                       .addExpected("MyObject1 was not found"_L1, 4, 5)
                       .addExpected("MyObject2 was not found"_L1, 4, 31)
                       .addUnexpected("Property \"myObj\" has incomplete type \"MyObject1\""_L1)
                       .build()
            << defaultOptions;
}

void TestQmllint::missingRegistration()
{
    dirtyQmlSnippet();
}

void TestQmllint::absolutePath()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QString absPathOutput = runQmllint("memberNotFound.qml", false, warningsShouldFailArgs(), true, true, true);
    QString relPathOutput = runQmllint("memberNotFound.qml", false, warningsShouldFailArgs(), true, true, false);
    const QString absolutePath = QFileInfo(testFile("memberNotFound.qml")).absoluteFilePath();

    QVERIFY(absPathOutput.contains(absolutePath));
    QVERIFY(!relPathOutput.contains(absolutePath));

    runQmllint("memberNotFound.qml", false,
               warningsShouldFailArgs() << "--json" << temp.filePath("absolute.json"_L1), true,
               true, true);
    runQmllint("memberNotFound.qml", false,
               warningsShouldFailArgs() << "--json" << temp.filePath("relative.json"_L1), true,
               true, false);

    QFile absoluteJsonFile(temp.filePath("absolute.json"_L1));
    QVERIFY(absoluteJsonFile.open(QFile::ReadOnly | QFile::Text));
    const QByteArray absoluteJson = absoluteJsonFile.readAll();
    QVERIFY(absoluteJson.contains(absolutePath.toUtf8()));

    QFile relativeJsonFile(temp.filePath("relative.json"_L1));
    QVERIFY(relativeJsonFile.open(QFile::ReadOnly | QFile::Text));
    const QByteArray relativeJson = relativeJsonFile.readAll();
    QVERIFY(!relativeJson.contains(absolutePath.toUtf8()));
}

void TestQmllint::importMultipartUri()
{
    runTest("here.qml", ResultBuilder::cleanResult(), {}, { testFile("Elsewhere/qmldir") });
}

void TestQmllint::lintModule_data()
{
    QTest::addColumn<QString>("module");
    QTest::addColumn<QStringList>("importPaths");
    QTest::addColumn<QStringList>("resources");
    QTest::addColumn<Result>("result");

    QTest::addRow("Things")
            << u"Things"_s
            << QStringList()
            << QStringList()
            << ResultBuilder()
               .addExpected("Type \"QPalette\" not found. Used in SomethingEntirelyStrange.palette"_L1)
               .addExpected("Type \"CustomPalette\" is not fully resolved. Used in "
                            "SomethingEntirelyStrange.palette2"_L1)
               .build();
    QTest::addRow("missingQmltypes")
            << u"Fake5Compat.GraphicalEffects.private"_s
            << QStringList()
            << QStringList()
            << ResultBuilder::singleExpected("QML types file does not exist"_L1);

    QTest::addRow("moduleWithQrc")
            << u"moduleWithQrc"_s
            << QStringList({ testFile("hidden") })
            << QStringList({
                               testFile("hidden/qmake_moduleWithQrc.qrc"),
                               testFile("hidden/moduleWithQrc_raw_qml_0.qrc")
                           })
            << ResultBuilder::cleanResult();
    QTest::newRow(("ImportFileSelector"))
            << QStringLiteral("FileSelector") << QStringList() << QStringList()
            << ResultBuilder()
               .addExpected("Ambiguous type detected. ToolBar 1.0 is defined multiple times."_L1)
               .setFlag(Result::UseSettings)
               .build();
    QTest::newRow(("ImportFileSelector2"))
            << QStringLiteral("FileSelector2") << QStringList() << QStringList()
            << ResultBuilder()
               .addExpected("Ambiguous type detected. ToolBar 1.0 is defined multiple times."_L1)
               .addExpected("Ambiguous type detected. Broken 1.0 is defined multiple times."_L1)
               .addUnexpected("Type ToolBar is ambiguous due to file selector usage, ignoring %1"_L1
                              .arg(testFile("FileSelector2/+Material/ToolBar.qml")))
               .setFlag(Result::UseSettings)
               .build();
}

void TestQmllint::lintModule()
{
    QFETCH(QString, module);
    QFETCH(QStringList, importPaths);
    QFETCH(QStringList, resources);
    QFETCH(Result, result);

    CallQmllintOptions options;
    options.importPaths = importPaths;
    options.resources = resources;
    options.type = LintModule;

    const QJsonArray warnings = callQmllint(module, options, fromResultFlags(result.flags));
    checkResult(warnings, result);
}

void TestQmllint::testLineEndings()
{
    QQmlJSLinter::LintOptions lintOptions;
    lintOptions.setFlag(QQmlJSLinter::Silent);
    {
        const auto textWithLF = QString::fromUtf16(u"import QtQuick 2.0\nimport QtTest 2.0 // qmllint disable unused-imports\n"
            "import QtTest 2.0 // qmllint disable\n\nItem {\n    @Deprecated {}\n    property string deprecated\n\n    "
            "property string a: root.a // qmllint disable unqualifi77777777777777777777777777777777777777777777777777777"
            "777777777777777777777777777777777777ed\n    property string b: root.a // qmllint di000000000000000000000000"
            "000000000000000000inyyyyyyyyg c: root.a\n    property string d: root.a\n    // qmllint enable unqualified\n\n    "
            "//qmllint d       4isable\n    property string e: root.a\n    Component.onCompleted: {\n        console.log"
            "(deprecated);\n    }\n    // qmllint enable\n\n}\n");

        const auto lintResult = m_linter.lintFile(testFile("snippetWithLF.qml"), &textWithLF,
                                                  lintOptions, { }, { }, { }, { })
                                        .status;

        QCOMPARE(lintResult, QQmlJSLinter::LintResult::HasWarnings);
    }
    {
        const auto textWithCRLF = QString::fromUtf16(u"import QtQuick 2.0\nimport QtTest 2.0 // qmllint disable unused-imports\n"
        "import QtTest 2.0 // qmllint disable\n\nItem {\n    @Deprecated {}\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r"
        "\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\r\n    property string deprecated\n\n    property string a: root.a "
        "// qmllint disable unqualifi77777777777777777777777777777777777777777777777777777777777777777777777777777777777777777ed\n    "
        "property string b: root.a // qmllint di000000000000000000000000000000000000000000inyyyyyyyyg c: root.a\n    property string d: "
        "root.a\n    // qmllint enable unqualified\n\n    //qmllint d       4isable\n    property string e: root.a\n    Component.onCompleted: "
        "{\n        console.log(deprecated);\n    }\n    // qmllint enable\n\n}\n");

        const auto lintResult = m_linter.lintFile(testFile("snippetWithCRLF.qml"), &textWithCRLF,
                                                  lintOptions, { }, { }, { }, { })
                                        .status;

        QCOMPARE(lintResult, QQmlJSLinter::LintResult::HasWarnings);
    }
}

void TestQmllint::valueTypesFromString()
{
    constexpr auto expectedMsg = "Construction from string is deprecated. Use structured value "
                                 "type construction instead for type \"%1\""_L1;
    constexpr auto fixMsg = "Replace string by structured value construction"_L1;
    CallQmllintOptions options;
    options.plugins << "Quick"_L1;
    runTest("valueTypesFromString.qml",
            ResultBuilder()
            .addExpected(expectedMsg.arg("QPointF"_L1))
            .addExpected(expectedMsg.arg("QSizeF"_L1))
            .addExpected(expectedMsg.arg("QRectF"_L1))
            .addExpected(expectedMsg.arg("QVector2D"_L1))
            .addExpected(expectedMsg.arg("QVector3D"_L1))
            .addExpected(expectedMsg.arg("QVector4D"_L1))
            .addExpected(expectedMsg.arg("QQuaternion"_L1))
            .addExpected(expectedMsg.arg("QMatrix4x4"_L1))
            .addFix(fixMsg, Edit{ "({ width: 30, height: 50 })"_L1 })
            .addFix(fixMsg, Edit{ "({ x: 10, y: 20, width: 30, height: 50 })"_L1 })
            .addFix(fixMsg, Edit{ "({ x: 30, y: 50 })"_L1 })
            .addFix(fixMsg, Edit{ "({ x: 1, y: 2 })"_L1 })
            .addFix(fixMsg, Edit{ "({ x: 1, y: 2 })"_L1 })
            .addFix(fixMsg, Edit{ "({ x: 1, y: 2, z: 3 })"_L1 })
            .addFix(fixMsg, Edit{ "({ x: 1, y: 2, z: 3, w: 4 })"_L1 })
            .addFix(fixMsg, Edit{ "({ scalar: 1, x: 2, y: 3, z: 4 })"_L1 })
            .addFix(fixMsg, Edit{ "({ m11: 1, m12: 2, m13: 3, m14: 4, m21: 5, m22: 6, m23: 7, m24: 8, m31: 9, m32: 10, m33: 11, m34: 12, m41: 13, m42: 14, m43: 15, m44: 16 })"_L1 })
            .build(), options);
}

#if QT_CONFIG(library)
void TestQmllint::hasTestPlugin()
{
    bool pluginFound = false;
    for (const QQmlJSLinter::Plugin &plugin : m_linter.plugins()) {
        if (plugin.name() != "testPlugin")
            continue;

        pluginFound = true;
        QCOMPARE(plugin.author(), u"Qt"_s);
        QCOMPARE(plugin.description(), u"A test plugin for tst_qmllint"_s);
        QCOMPARE(plugin.version(), u"1.0"_s);

        for (auto &category : plugin.categories()) {
            if (category.name() == u"testPlugin.TestDefaultValue") {
                QCOMPARE(category.severity(), QQmlJS::WarningSeverity::Disable);
            } else if (category.name() == u"testPlugin.TestDefaultValue2") {
                QCOMPARE(category.severity(), QQmlJS::WarningSeverity::Info);
            } else if (category.name() == u"testPlugin.test") {
                QCOMPARE(category.severity(), QQmlJS::WarningSeverity::Warning);
            } else if (category.name() == u"testPlugin.TestDefaultValue3"){
                QCOMPARE(category.severity(), QQmlJS::WarningSeverity::Warning);
            } else if (category.name() == u"testPlugin.TestDefaultValue4"){
                QCOMPARE(category.severity(), QQmlJS::WarningSeverity::Error);
            } else {
                QFAIL("This category was not tested!");
            }
        }
    }
    QVERIFY(pluginFound);
}
void TestQmllint::testPlugin_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<Result>("expectedErrors");

    QTest::addRow("elementpass_pluginTest")
            << testFile(u"testPluginData/elementpass_pluginTest.qml"_s)
            << ResultBuilder::singleExpected("ElementTest OK"_L1, 4, 5);
    QTest::addRow("propertypass_pluginTest_read")
            << testFile(u"testPluginData/propertypass_pluginTest.qml"_s)
            << ResultBuilder()
               // Property on any type
               .addExpected("Saw read on Text property x in scope Text"_L1, 8, 12)
               .addExpected("Saw read on Text property x in scope Item"_L1, 21, 25)
               // JavaScript
               .addExpected("Saw read on ObjectPrototype property log in scope Item"_L1, 21, 17)
               .addExpected("Saw read on ObjectPrototype property log in scope Item"_L1, 22, 14)
               .build();
    QTest::addRow("propertypass_pluginTest_write")
            << testFile(u"testPluginData/propertypass_pluginTest.qml"_s)
            << ResultBuilder()
               .addExpected("Saw write on Text property x with value int in scope Item"_L1, 23, 9)
               .build();
    QTest::addRow("propertypass_pluginTest_binding")
            << testFile(u"testPluginData/propertypass_pluginTest.qml"_s)
            << ResultBuilder()
               // Specific binding for specific property
               .addExpected("Saw binding on Text property text with value NULL (and type 3) in scope Text"_L1, 6, 15 )
               .addExpected("Saw binding on Text property x with value NULL (and type 2) in scope Text"_L1, 7, 12 )
               .addExpected("Saw binding on Item property x with value NULL (and type 2) in scope Item"_L1, 11, 8 )
               .addExpected("Saw binding on ListView property model with value ListModel (and type 8) in scope ListView"_L1, 16, 16 )
               .addExpected("Saw binding on ListView property height with value NULL (and type 2) in scope ListView"_L1, 17, 17)
               .build();

    QTest::addRow("sourceLocations")
            << testFile(u"testPluginData/sourceLocations_pluginTest.qml"_s)
            << ResultBuilder()
               // Specific binding for specific property
               .addExpected("Saw binding on Item property x with value QString (and type 8) in scope Item"_L1, 5, 12)
               .addExpected("Saw binding on Item property EnterKey.type with value Qt::EnterKeyType (and type 8) in scope QQuickEnterKeyAttached"_L1, 8, 24)
               .addExpected("Saw binding on Item property x with value QJSPrimitiveValue (and type 8) in scope Item"_L1, 11, 12)
               .addExpected("Saw binding on Item property x with value NULL (and type 2) in scope Item"_L1, 14, 12)
               .addExpected("Saw binding on Item property onXChanged with value function (and type 8) in scope Item"_L1, 18, 21)
               .addExpected("Saw read on ObjectPrototype property log in scope Item"_L1, 21, 36)
               .addExpected("Saw binding on Item property onXChanged with value QVariant (and type 8) in scope Item"_L1, 22, 21)
               .addExpected("Saw write on Item property x with value double in scope Item"_L1, 30, 13)
               .addExpected("Saw write on Item property x with value int in scope Item"_L1, 35, 31)
               .addExpected("Saw read on Item property x in scope Item"_L1, 35, 46)
               .build();
    QTest::addRow("propertypass_pluginTest_call")
            << testFile(u"testPluginData/propertypass_pluginTest.qml"_s)
            << ResultBuilder()
               .addExpected("Saw call on ObjectPrototype property log in scope Item"_L1, 21, 17)
               .addExpected("Saw call on ObjectPrototype property log in scope Item"_L1, 22, 14)
               .addExpected("Saw call on ObjectPrototype property abs in scope Item"_L1, 26, 22)
               .addExpected("Saw call on Item property abs in scope Item"_L1, 32, 16)
               .addExpected("Saw call on  property now in scope Item"_L1, 39, 22) // happening for Date.now())
               .build();
    QTest::addRow("propertypass_pluginTest_translations")
            << testFile(u"testPluginData/translations_pluginTest.qml"_s)
            << ResultBuilder()
               // translations
               .addExpected("Saw call on ObjectPrototype property qsTr in scope Item"_L1, 4, 34)
               // should actually be qsTranslate, but better qsTr than nothing!
               // see also test "propertypass_pluginTest_qsTranslateEdgeCase" below
               .addExpected("Saw call on ObjectPrototype property qsTr in scope Item"_L1, 5, 35)
               .addExpected("Saw call on ObjectPrototype property qsTrId in scope Item"_L1, 6, 36)
               .addExpected("Saw call on ObjectPrototype property qsTr in scope Item"_L1, 7, 46)
               .addExpected("Saw call on ObjectPrototype property qsTranslate in scope Item"_L1, 8, 47)
               .addExpected("Saw call on ObjectPrototype property qsTrId in scope Item"_L1, 9, 48)
               .build();

    QTest::addRow("controlsWithQuick_pluginTest")
            << testFile(u"testPluginData/controlsWithQuick_pluginTest.qml"_s)
            << ResultBuilder::singleExpected("QtQuick.Controls, QtQuick and QtQuick.Window present"_L1);
    QTest::addRow("controlsWithoutQuick_pluginTest")
            << testFile(u"testPluginData/controlsWithoutQuick_pluginTest.qml"_s)
            << ResultBuilder::singleExpected("QtQuick.Controls and NO QtQuick present"_L1);

    // Verify that none of the passes do anything when they're not supposed to
    QTest::addRow("nothing_pluginTest")
            << testFile(u"testPluginData/nothing_pluginTest.qml"_s) << ResultBuilder::cleanResult();
    QTest::addRow("settings_pluginTest")
            << testFile(u"settings/plugin/elementpass_pluginTest.qml"_s)
            << ResultBuilder::cleanResultWithSettings();
    QTest::addRow("old_settings_pluginTest")
            << testFile(u"settings/pluginOld/elementpass_pluginTest.qml"_s)
            << ResultBuilder::cleanResultWithSettings();
    QTest::addRow("nosettings_pluginTest")
            << testFile(u"settings/plugin/elementpass_pluginTest.qml"_s)
            << ResultBuilder::singleExpected("ElementTest OK"_L1);
    QTest::addRow("multipleDocumentEditsFixSuggestion")
            << testFile("testPluginData/multipleDocumentEditsFixSuggestion_pluginTest.qml")
            << ResultBuilder()
               .addExpected("Multiple document edits"_L1, 3, 1)
               .addFix("Rename and add pragma"_L1,
                       Edits{ { "pragma Yep\n"_L1, 1, 1 }, { "NewTypeName"_L1, 3, 1 } })
               .build();
}

void TestQmllint::testPlugin()
{
    QFETCH(QString, fileName);
    QFETCH(Result, expectedErrors);

    CallQmllintOptions options;
    options.plugins << "testPlugin"_L1;
    runTest(fileName, expectedErrors, options);
}

void TestQmllint::testPluginHelpCommandLine()
{
    auto qmllintOutput = [this](const QString& filename, const QStringList& args) {
        QString output;
        QString errorOutput;
        runQmllint(
                testFile(filename),
                [&](QProcess &process) {
                    QVERIFY(process.waitForFinished());
                    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
                    QCOMPARE(process.exitCode(), 0);
                    output = process.readAllStandardOutput();
                    errorOutput = process.readAllStandardError();
                },
                args);
        return std::pair<QString, QString>{ output, errorOutput };
    };
    {
        // make sure plugin warnings are documented by --help
        const auto [helpText, error] = qmllintOutput(u"testPluginData/nothing_pluginTest.qml"_s,
                                                     QStringList{ u"--help"_s });
        QVERIFY(helpText.contains(u"--Quick.property-changes-parsed"_s));
    }
}

void TestQmllint::testPluginCommandLine()
{
    // make sure plugin warnings are accepted as options
    const QString warnings =
            runQmllint(testFile(u"testPluginData/nothing_pluginTest.qml"_s), true,
                       QStringList{ u"--Quick.property-changes-parsed"_s, u"disable"_s });
    // should not contain a warning about --Quick.property-changes-parsed being an unknown option
    // and no warnings
    QVERIFY(warnings.isEmpty());
}

// TODO: Eventually tests for (real) plugins need to be moved into a separate file
void TestQmllint::quickPlugin()
{
    const auto &plugins = m_linter.plugins();

    const bool pluginFound =
            std::find_if(plugins.cbegin(), plugins.cend(),
                         [](const auto &plugin) { return plugin.name() == "Quick"; })
            != plugins.cend();
    QVERIFY(pluginFound);

    CallQmllintOptions withQuickPlugin;
    withQuickPlugin.plugins << "Quick"_L1;

    runTest("pluginQuick_anchors.qml",
            ResultBuilder()
            .addExpected("Cannot specify left, right, and horizontalCenter anchors at the same time."_L1)
            .addExpected("Cannot specify top, bottom, and verticalCenter anchors at the same time."_L1)
            .addExpected("Baseline anchor cannot be used in conjunction with top, bottom, or verticalCenter anchors."_L1)
            .addExpected("Cannot assign literal of type null to QQuickAnchorLine"_L1, 5, 35)
            .addExpected("Cannot assign literal of type null to QQuickAnchorLine"_L1, 6, 33)
            .build(), withQuickPlugin);
    runTest("pluginQuick_anchorsUndefined.qml", ResultBuilder::cleanResult(), withQuickPlugin);
    runTest("pluginQuick_layoutChildren.qml",
            ResultBuilder()
            .addExpected("Detected anchors on an item that is managed by a layout. This is undefined behavior; use Layout.alignment instead."_L1)
            .addExpected("Detected x on an item that is managed by a layout. This is undefined behavior; use Layout.leftMargin or Layout.rightMargin instead."_L1)
            .addExpected("Detected y on an item that is managed by a layout. This is undefined behavior; use Layout.topMargin or Layout.bottomMargin instead."_L1)
            .addExpected("Detected height on an item that is managed by a layout. This is undefined behavior; use implictHeight or Layout.preferredHeight instead."_L1)
            .addExpected("Detected width on an item that is managed by a layout. This is undefined behavior; use implicitWidth or Layout.preferredWidth instead."_L1)
            .addExpected("Cannot specify anchors for items inside Grid. Grid will not function."_L1)
            .addExpected("Cannot specify x for items inside Grid. Grid will not function."_L1)
            .addExpected("Cannot specify y for items inside Grid. Grid will not function."_L1)
            .addExpected("Cannot specify y for items inside Grid. Grid will not function."_L1)
            .addExpected("Cannot specify y for items inside Grid. Grid will not function."_L1)
            .addExpected("Cannot specify anchors for items inside Flow. Flow will not function."_L1)
            .addExpected("Cannot specify anchors for items inside Flow. Flow will not function."_L1)
            .addExpected("Cannot specify anchors for items inside Flow. Flow will not function."_L1)
            .addExpected("Cannot specify anchors for items inside Flow. Flow will not function."_L1)
            .addExpected("Cannot specify x for items inside Flow. Flow will not function."_L1)
            .addExpected("Cannot specify x for items inside Flow. Flow will not function."_L1)
            .addExpected("Cannot specify y for items inside Flow. Flow will not function."_L1)
            .build(), withQuickPlugin);
    runTest("pluginQuick_attached.qml",
            ResultBuilder()
            .addExpected("ToolTip attached property must be attached to an object deriving from Item"_L1)
            .addExpected("SplitView attached property must be attached to an object deriving from Item"_L1)
            .addExpected("ScrollIndicator attached property must be attached to an object deriving from Flickable"_L1)
            .addExpected("ScrollBar attached property must be attached to an object deriving from Flickable or ScrollView"_L1)
            .addExpected("Accessible attached property must be attached to an object deriving from Item or Action"_L1)
            .addExpected("EnterKey attached property must be attached to an object deriving from Item"_L1)
            .addExpected("LayoutMirroring attached property must be attached to an object deriving from Item or Window"_L1)
            .addExpected("Layout attached property must be attached to an object deriving from Item"_L1)
            .addExpected("StackView attached property must be attached to an object deriving from Item"_L1)
            .addExpected("TextArea attached property must be attached to an object deriving from Flickable"_L1)
            .addExpected("StackLayout attached property must be attached to an object deriving from Item"_L1)
            .addExpected("SwipeDelegate attached property must be attached to an object deriving from Item"_L1)
            .addExpected("SwipeView attached property must be attached to an object deriving from Item"_L1)
            .build(), withQuickPlugin);

    runTest("pluginQuick_tumblerGood.qml", ResultBuilder().addUnexpected("Tembler"_L1).build(), withQuickPlugin);

    runTest("pluginQuick_swipeDelegate.qml",
            ResultBuilder()
            .addExpected("SwipeDelegate: Cannot use horizontal anchors with contentItem; unable to layout the item."_L1, 6, 43)
            .addExpected("SwipeDelegate: Cannot use horizontal anchors with background; unable to layout the item."_L1, 7, 43)
            .addExpected("SwipeDelegate: Cannot set both behind and left/right properties"_L1, 9, 9)
            .addExpected("SwipeDelegate: Cannot use horizontal anchors with contentItem; unable to layout the item."_L1, 13, 47)
            .addExpected("SwipeDelegate: Cannot use horizontal anchors with background; unable to layout the item."_L1, 14, 42)
            .addExpected("SwipeDelegate: Cannot set both behind and left/right properties"_L1, 16, 9)
            .build(), withQuickPlugin);

    runTest("pluginQuick_varProp.qml",
            ResultBuilder()
            .addExpected("Unexpected type for property \"contentItem\" expected QQuickPathView, QQuickListView got QQuickItem"_L1)
            .addExpected("Unexpected type for property \"columnWidthProvider\" expected function got null"_L1)
            .addExpected("Unexpected type for property \"textFromValue\" expected function got null"_L1)
            .addExpected("Unexpected type for property \"valueFromText\" expected function got int"_L1)
            .addExpected("Unexpected type for property \"rowHeightProvider\" expected function got int"_L1)
            .build(), withQuickPlugin);
    runTest("pluginQuick_varPropClean.qml", ResultBuilder::cleanResult(), withQuickPlugin);
    runTest("pluginQuick_attachedClean.qml", ResultBuilder::cleanResult(), withQuickPlugin);
    runTest("pluginQuick_attachedIgnore.qml", ResultBuilder::cleanResult(), withQuickPlugin);
    runTest("pluginQuick_noCrashOnUneresolved.qml", ResultBuilder::ignoredResult(), withQuickPlugin); // we don't care about the specific warnings

    runTest("pluginQuick_propertyChangesParsed.qml",
            ResultBuilder()
            .addExpected("Property \"myColor\" is custom-parsed in PropertyChanges. "
                         "You should phrase this binding as \"foo.myColor: Qt.rgba(0.5, ...\""_L1, 12, 30)
            .addExpected("You should remove any bindings on the \"target\" property and avoid "
                         "custom-parsed bindings in PropertyChanges."_L1, 11, 29)
            .addExpected("Unknown property \"notThere\" in PropertyChanges."_L1, 13, 31)
            .build(), withQuickPlugin);
    runTest("pluginQuick_propertyChangesInvalidTarget.qml", ResultBuilder::ignoredResult(), withQuickPlugin); // we don't care about the specific warnings
    runTest("pluginQuick_stateWithLegalChildren.qml", ResultBuilder::cleanResult(), withQuickPlugin);
    runTest("pluginQuick_stateWithIllegalChildren.qml",
            ResultBuilder()
            .addExpected("A State cannot have a child item of type Rectangle"_L1, 5, 9)
            .addExpected("A State cannot have a child item of type Item"_L1, 6, 9)
            .build(), withQuickPlugin);
    runTest("pluginQuick_AccessibleOnAction.qml", ResultBuilder::cleanResult(), withQuickPlugin);
    runTest("pluginQuick_AccessibleOnAction2.qml", ResultBuilder::cleanResult(), withQuickPlugin);
}

void TestQmllint::hasQdsPlugin()
{
    const auto &plugins = m_linter.plugins();

    const bool pluginFound =
            std::find_if(plugins.cbegin(), plugins.cend(),
                         [](const auto &plugin) { return plugin.name() == "QtDesignStudio"; })
            != plugins.cend();
    QVERIFY(pluginFound);
}

void TestQmllint::qdsPlugin_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<Result>("expectedResult");

    QTest::addRow("WhiteListedFunctions")
            << u"qdsPlugin/WhiteListedFunctions.ui.qml"_s << ResultBuilder::cleanResult();
    QTest::addRow("WhiteListedFunctionsDate")
            << u"qdsPlugin/WhiteListedFunctionsDate.ui.qml"_s << ResultBuilder::cleanResult();
    QTest::addRow("FunctionInsideConnections")
            << u"qdsPlugin/FunctionInsideConnections.ui.qml"_s << ResultBuilder::cleanResult();
    {
        const QString warning =
                u"Arbitrary functions and function calls outside of a Connections object are not "
                u"supported in a UI file (.ui.qml)"_s;

        QTest::addRow("BlackListedFunctions")
                << u"qdsPlugin/BlackListedFunctions.ui.qml"_s
                << ResultBuilder()
                   .addExpected(warning, 7, 9)
                   .addExpected(warning, 8, 14)
                   .addExpected(warning, 12, 38)
                   .addExpected(warning, 13, 35)
                   .build();
    }

    QTest::addRow("UnsupportedBindings")
            << u"qdsPlugin/UnsupportedBindings.ui.qml"_s
            << ResultBuilder()
               .addExpected("Referencing the parent of the root item is not supported in a UI file "
                            "(.ui.qml)"_L1, 4, 25)
               .addExpected("Imperative JavaScript assignments can break the visual tooling in "
                            "Qt Design Studio."_L1, 7, 24)
               .addExpected("Imperative JavaScript assignments can break the visual tooling in "
                            "Qt Design Studio."_L1, 8, 24)
               .build();

    QTest::addRow("SupportedBindings")
            << u"qdsPlugin/SupportedBindings.ui.qml"_s
            << ResultBuilder::cleanResult();

    QTest::addRow("UnsupportedElements")
            << u"qdsPlugin/UnsupportedElements.ui.qml"_s
            << ResultBuilder()
               .addExpected("This type (ApplicationWindow) is not supported in a UI file (.ui.qml)"_L1, 4, 1)
               .addExpected("This type (ShaderEffect) is not supported in a UI file (.ui.qml)"_L1, 7, 27)
               .addExpected("This type (Drawer) is not supported in a UI file (.ui.qml)"_L1, 6, 9)
               .addExpected("This id (bool) might be ambiguous and is not supported in a UI file (.ui.qml)"_L1, 11, 13)
               .build();
    QTest::addRow("SupportedElements")
            << u"qdsPlugin/SupportedElements.ui.qml"_s
            << ResultBuilder::cleanResult();

    QTest::addRow("UnsupportedRootElement")
            << u"qdsPlugin/UnsupportedRootElement.ui.qml"_s
            << ResultBuilder::singleExpected("This type (QtObject) is not supported as a root "
                                             "element of a UI file (.ui.qml)."_L1, 3, 1);

    QTest::addRow("UnsupportedRootElement2")
            << u"qdsPlugin/UnsupportedRootElement2.ui.qml"_s
            << ResultBuilder::singleExpected("This type (ListModel) is not supported as a root "
                                             "element of a UI file (.ui.qml)."_L1, 4, 1);

    {
        const QString functionError =
                u"Arbitrary functions and function calls outside of a Connections object are not supported in a UI file (.ui.qml)"_s;
        QTest::addRow("UnsupportedBlock")
                << u"qdsPlugin/UnsupportedBlock.ui.qml"_s
                << ResultBuilder()
                   .addExpected(functionError, 5, 29)
                   .addExpected(functionError, 6, 30)
                   .addExpected(functionError, 8, 29)
                   .addExpected(functionError, 7, 32)
                   .build();
        QTest::addRow("SupportedBlock")
                << u"qdsPlugin/SupportedBlock.ui.qml"_s
                << ResultBuilder::cleanResult();

        QTest::addRow("UnsupportedFunction")
                << u"qdsPlugin/UnsupportedFunction.ui.qml"_s
                << ResultBuilder()
                   .addExpected(functionError, 4, 5)
                   .addExpected(functionError, 13, 9)
                   .addUnexpected(functionError, 7, 9)
                   .addUnexpected(functionError, 10, 9)
                   .build();
    }
    {
        const QString warning = u"Do not mix translation functions"_s;
        QTest::addRow("BadTranslationMix")
                << u"qdsPlugin/BadMix.ui.qml"_s
                << ResultBuilder()
                   .addExpected(warning, 5, 49)
                   .addExpected(warning, 6, 56)
                   .build();
    }
}

void TestQmllint::qdsPlugin()
{
    QFETCH(QString, fileName);
    QFETCH(Result, expectedResult);

    CallQmllintOptions withQdsPlugin;
    withQdsPlugin.plugins << "QtDesignStudio"_L1;

    runTest(fileName, expectedResult, withQdsPlugin);
}

#endif // QT_CONFIG(library)

void TestQmllint::environment_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<bool>("shouldSucceed");
    QTest::addColumn<QStringList>("extraArgs");
    QTest::addColumn<Environment>("env");
    QTest::addColumn<QString>("expectedWarning");

    const QString fileThatNeedsImportPath = testFile(u"NeedImportPath.qml"_s);
    const QString importPath = testFile(u"ImportPath"_s);
    const QString invalidImportPath = testFile(u"ImportPathThatDoesNotExist"_s);
    const QString noWarningExpected;

    QTest::addRow("missing-import-dir")
            << fileThatNeedsImportPath << false << warningsShouldFailArgs()
            << Environment{ { u"QML_IMPORT_PATH"_s, importPath } } << noWarningExpected;

    QTest::addRow("import-dir-via-arg")
            << fileThatNeedsImportPath << true << QStringList{ u"-I"_s, importPath }
            << Environment{ { u"QML_IMPORT_PATH"_s, invalidImportPath } } << noWarningExpected;

    QTest::addRow("import-dir-via-env")
            << fileThatNeedsImportPath << true << QStringList{ u"-E"_s }
            << Environment{ { u"QML_IMPORT_PATH"_s, importPath } }
            << u"Using import directories passed from environment variable \"QML_IMPORT_PATH\": \"%1\"."_s
                       .arg(importPath);

    QTest::addRow("import-dir-via-env2")
            << fileThatNeedsImportPath << true << QStringList{ u"-E"_s }
            << Environment{ { u"QML2_IMPORT_PATH"_s, importPath } }
            << u"Using import directories passed from the deprecated environment variable \"QML2_IMPORT_PATH\": \"%1\"."_s
                       .arg(importPath);
}

void TestQmllint::environment()
{
    QFETCH(QString, file);
    QFETCH(bool, shouldSucceed);
    QFETCH(QStringList, extraArgs);
    QFETCH(Environment, env);
    QFETCH(QString, expectedWarning);

    const QString output = runQmllint(file, shouldSucceed, extraArgs, false, true, false, env);
    if (!expectedWarning.isEmpty()) {
        QVERIFY(output.contains(expectedWarning));
    }
}

void TestQmllint::maxWarnings()
{
    // warnings are not fatal by default
    runQmllint(testFile("badScript.qml"), true);
    // or when max-warnings is set to -1
    runQmllint(testFile("badScript.qml"), true, {"-W", "-1"});
    // 1 warning => should fail
    runQmllint(testFile("badScript.qml"), false, {"--max-warnings", "0"});
    // only 2 warning => should exit normally
    runQmllint(testFile("badScript.qml"), true, {"--max-warnings", "2"});
}

void TestQmllint::ignoreSettingsNotCommandLineOptions()
{
    const QString importPath = testFile(u"ImportPath"_s);
    // makes sure that ignore settings only ignores settings and not command line options like
    // "-I".
    const QString output = runQmllint(testFile(u"NeedImportPath.qml"_s), true,
                                      QStringList{ u"-I"_s, importPath }, true);
    // should not complain about not finding the module that is in importPath
    QCOMPARE(output, QString());
}

void TestQmllint::backslashedQmldirPath()
{
    const QString qmldirPath
            = testFile(u"ImportPath/ModuleInImportPath/qmldir"_s).replace('/', QDir::separator());
    const QString output = runQmllint(
            testFile(u"something.qml"_s), true, QStringList{ u"-i"_s, qmldirPath });
    QVERIFY2(output.isEmpty(), qPrintable(output));
}

#if QT_CONFIG(process)
void TestQmllint::importRelScript()
{
    QProcess proc;
    proc.start(m_qmllintPath, { QStringLiteral(TST_QMLLINT_IMPORT_REL_SCRIPT_ARGS) });
    QVERIFY(proc.waitForFinished());
    const QByteArray output = proc.readAllStandardOutput();
    QVERIFY2(output.isEmpty(), output.constData());
    const QByteArray errors = proc.readAllStandardError();
    QVERIFY2(errors.isEmpty(), errors.constData());
}
#endif

void TestQmllint::replayImportWarnings()
{
    QJsonArray warnings =
            callQmllint(testFile(u"duplicateTypeUserUser.qml"_s), CallQmllintOptions{});

    // No warning because the offending import is indirect.
    QVERIFY2(warnings.isEmpty(), qPrintable(QJsonDocument(warnings).toJson()));

    // No cache clearing here. We want the warnings restored.
    warnings = callQmllint(testFile(u"DuplicateTypeUser.qml"_s), CallQmllintOptions{},
                           CallQmllintCheck::ShouldFail);

    // Warning because the offending import is now direct.
    searchWarnings(warnings, "Ambiguous type detected. T 1.0 is defined multiple times.");
}

void TestQmllint::errorCategory()
{
    {
        const QString output = runQmllint(testFile(u"HasUnqualified.qml"_s), false,
                                          QStringList{ u"--unqualified"_s, u"error"_s });
        QVERIFY(output.startsWith("Error: "));
    }
    {
        const QString output = runQmllint(testFile(u"HasUnqualified.qml"_s), true);
        QVERIFY(output.startsWith("Warning: "));
    }

}

void TestQmllint::unrecognizedIniSection()
{
    const QString iniFilePath = testFile("UnrecognizedIniSection/.qmllint.ini"_L1);
    const QString qmlFilePath = testFile("UnrecognizedIniSection/file.qml"_L1);

    bool shouldSucceed = true;
    bool ignoreSettings = false;
    const auto output = runQmllint(qmlFilePath, shouldSucceed, {}, ignoreSettings);
    QVERIFY(output.contains("Unrecognized section \"Warning\" in %1"_L1.arg(iniFilePath)));
}

void TestQmllint::shadow_data()
{
    // note: use the same column as dirtyQmlSnippet_data() to reuse dirtyQmlSnippet() in shadow().
    QTest::addColumn<QString>("code");
    QTest::addColumn<Result>("result");
    QTest::addColumn<CallQmllintOptions>("options");

    CallQmllintOptions defaultOptions;
    defaultOptions.categorySeverityOverrides[qmlShadow.name().toString()] =
            QQmlJS::WarningSeverity::Warning;
    // filename of the snippet is empty
    const QString fileName = testFile("Snippet.qml");

    QTest::newRow("duplicatedMethod")
            << u"function hello() {}"
               u"function hello() {}"_s
            << ResultBuilder()
               .addExpected("Duplicated method name \"hello\", \"hello\" is already a method."_L1, 1, 29)
               .addUnexpected("Method \"hello\" already exists in base type"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("duplicatedProperty")
            << u"property int hello;"
               u"property int hello;"_s
            << ResultBuilder()
               .addExpected("Duplicated property name \"hello\", \"hello\" is already a property."_L1, 1, 33)
               .addUnexpected("Property \"hello\" already exists in base type"_L1)
               .build()
            << defaultOptions;
    QTest::newRow("duplicatedSignal")
            << u"signal hello();"
               u"signal hello();"_s
            << ResultBuilder()
               .addExpected("Duplicated signal name \"hello\", \"hello\" is already a signal."_L1, 1, 23)
               .addUnexpected("Signal \"hello\" already exists in base type"_L1)
               .build()
            << defaultOptions;

    QTest::newRow("idShadowsMember-noUsage") // shadowing but no usage -> don't warn
            << u"id: i\n"_s
               u"property int i\n"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("idShadowsMember-property")
            << u"id: i\n"_s
               u"property int i\n"_s
               u"property var v: i\n"_s
            << ResultBuilder()
               .addExpected("Id for object Item shadows property \"i\""_L1, 3, 17)
               .addExpected("Note: Id defined here"_L1, 1, 5)
               .build()
            << defaultOptions;
    QTest::newRow("idShadowsMember-method")
            << u"id: i\n"_s
               u"function i() {}\n"_s
               u"property var v: i\n"_s
            << ResultBuilder()
               .addExpected("Id for object Item shadows method \"i\""_L1, 3, 17)
               .addExpected("Note: Id defined here"_L1, 1, 5)
               .build()
            << defaultOptions;
    QTest::newRow("idShadowsMember-signal")
            << u"id: a\n"_s
               u"signal a\n"_s
               u"property var v: a\n"_s
            << ResultBuilder()
               .addExpected("Id for object Item shadows signal \"a\""_L1, 3, 17)
               .addExpected("Note: Id defined here"_L1, 1, 5)
               .build()
            << defaultOptions;
    QTest::newRow("idShadowsMember-baseProperty")
            << u"id: i\n"_s
               u"component C : Item { property int i }\n"_s
               u"C { property var v: i }\n"_s
            << ResultBuilder::singleExpected("Id for object Item shadows property \"i\""_L1, 3, 21)
            << defaultOptions;
    QTest::newRow("idShadowsMember-baseMethod")
            << u"id: i\n"_s
               u"component C : Item { function i() {} }\n"_s
               u"C { property var v: i }\n"_s
            << ResultBuilder::singleExpected("Id for object Item shadows method \"i\""_L1, 3, 21)
            << defaultOptions;
    QTest::newRow("idShadowsMember-noDuplicateWarnings") // 1 shadowing, 2 usages -> 1 warning
            << u"id: i\n"_s
               u"property int i\n"_s
               u"property var v1: i\n"_s
               u"property var v2: i\n"_s
            << ResultBuilder()
               .addExpected("Id for object Item shadows property \"i\""_L1, 3, 18)
               .addUnexpected("Id for object Item shadows property \"i\""_L1, 4, 18)
               .build()
            << defaultOptions;
    QTest::newRow("idShadowsMember-componentBoundaries-Bound")
            << u"pragma ComponentBehavior: Bound\n"_s
               u"import QtQuick\n"_s
               u"Item {\n"_s
               u"    id: i\n"_s
               u"    component C : Item { property int i; property var v: i }\n"_s
               u"}\n"_s
            << ResultBuilder()
               .addExpected("Id for object Item shadows property \"i\""_L1, 5, 58)
               .addExpected("Note: Id defined here"_L1, 4, 9)
               .build()
            << defaultOptions;
    QTest::newRow("idShadowsMember-componentBoundaries-Unbound")
            << u"pragma ComponentBehavior: Unbound\n"_s
               u"import QtQuick\n"_s
               u"Item {\n"_s
               u"    id: i\n"_s
               u"    component C : Item { property int i; property var v: i }\n"_s
               u"}\n"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("idShadowsMember-disableDirective")
            << u"id: i\n"_s
               u"property int i\n"_s
               u"property var v: i // qmllint disable id-shadows-member\n"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;

    QTest::newRow("shadowMethod")
            << u"component IC: Item { function f() {} }\n"
               u"IC { function f() {} }"_s
            << ResultBuilder::singleExpected("Method \"f\" already exists in base type \"IC\""_L1, 2, 15)
            << defaultOptions;
    QTest::newRow("shadowMethod2")
            << u"component IC: Item { function f() {} }\n"
               u"IC { function f(a,b,c) {} }"_s
            << ResultBuilder::singleExpected("Method \"f\" already exists in base type \"IC\""_L1, 2, 15)
            << defaultOptions;
    QTest::newRow("shadowMethodWithProperty")
            << u"component IC: Item { function f() {} }\n"
               u"IC { property int f; }"_s
            << ResultBuilder::singleExpected("Method \"f\" already exists in base type \"IC\""_L1, 2, 19)
            << defaultOptions;
    QTest::newRow("shadowMethodWithSignal")
            << u"component IC: Item { function f() {} }\n"
               u"IC { signal f; }"_s
            << ResultBuilder::singleExpected("Method \"f\" already exists in base type \"IC\""_L1, 2, 13)
            << defaultOptions;
    QTest::newRow("shadowSignal")
            << u"component IC: Item { signal f }\n"
               u"IC { signal f }"_s
            << ResultBuilder::singleExpected("Signal \"f\" already exists in base type \"IC\""_L1, 2, 13)
            << defaultOptions;
    QTest::newRow("shadowSignal2")
            << u"component IC: Item { signal f }\n"
               u"IC { signal f(a:int,b:string,c:string) }"_s
            << ResultBuilder::singleExpected("Signal \"f\" already exists in base type \"IC\""_L1, 2, 13)
            << defaultOptions;
    QTest::newRow("shadowSignalWithProperty")
            << u"component IC: Item { signal f }\n"
               u"IC { property int f; }"_s
            << ResultBuilder::singleExpected("Signal \"f\" already exists in base type \"IC\""_L1, 2, 19)
            << defaultOptions;
    QTest::newRow("shadowSignalWithMethod")
            << u"component IC: Item { signal f }\n"
               u"IC { function f() {} }"_s
            << ResultBuilder::singleExpected("Signal \"f\" already exists in base type \"IC\""_L1, 2, 15)
            << defaultOptions;

    QTest::newRow("shadowProperty")
            << u"component IC: Item { property int f }\n"
               u"IC { property int f }"_s
            << ResultBuilder::singleExpected("Property \"f\" already exists in base type \"IC\""_L1, 2, 19)
            << defaultOptions;
    QTest::newRow("shadowProperty2")
            << u"component IC: Item { property int f }\n"
               u"IC { property string f }"_s
            << ResultBuilder::singleExpected("Property \"f\" already exists in base type \"IC\""_L1, 2, 22)
            << defaultOptions;
    QTest::newRow("shadowPropertyWithSignal")
            << u"component IC: Item { property int f }\n"
               u"IC { signal f; }"_s
            << ResultBuilder::singleExpected("Property \"f\" already exists in base type \"IC\""_L1, 2, 13)
            << defaultOptions;
    QTest::newRow("shadowPropertyWithMethod")
            << u"component IC: Item { property int f }\n"
               u"IC { function f() {} }"_s
            << ResultBuilder::singleExpected("Property \"f\" already exists in base type \"IC\""_L1, 2, 15)
            << defaultOptions;

    QTest::newRow("shadowFinalWithProperty")
            << u"component IC: Item { final property int f; }\n"
               u"IC { property var f; }"_s
            << ResultBuilder::singleExpected("Member \"f\" shadows final member \"f\" from base "
                                             "type \"IC\", use a different name."_L1, 2, 19)
            << defaultOptions;
    QTest::newRow("shadowFinalWithOverride")
            << u"component IC: Item { final property int f; }\n"
               u"IC { override property var f; }"_s
            << ResultBuilder::singleExpected(
                   "Member \"f\" overrides final member \"f\" from base type \"IC\", use a "
                   "different name and remove the \"override\""_L1, 2, 28)
            << defaultOptions;
    QTest::newRow("shadowPropertyWithFinal")
            << u"component IC: Item { property int f; }\n"
               u"IC { final property var f; }"_s
            << ResultBuilder::singleExpected("Property \"f\" already exists in base type \"IC\""_L1, 2, 25)
            << defaultOptions;
    QTest::newRow("shadowMissingPropertyWithOverride")
            << u"Item { override property var blablabla; }"_s
            << ResultBuilder::singleExpected("Member \"blablabla\" does not override anything. "
                                             "Consider removing \"override\"."_L1, 1, 30)
            << defaultOptions;
    QTest::newRow("shadowPropertyWithOverride")
            << u"component IC: Item { property int f; }\n"
               u"IC { override property var f; }"_s
            << ResultBuilder::singleExpected(
                   "Member \"f\" overrides a non-virtual member from base type \"IC\", use a "
                   "different name or mark the property as virtual in the base type."_L1, 2, 28)
            << defaultOptions;
    QTest::newRow("shadowPropertyWithVirtual")
            << u"component IC: Item { property int f; }\n"
               u"IC { virtual property var f; }"_s
            << ResultBuilder::singleExpected("Property \"f\" already exists in base type \"IC\", "
                                             "use a different name."_L1, 2, 27)
            << defaultOptions;
    QTest::newRow("shadowVirtualWithVirtual")
            << u"component IC: Item { virtual property int f; }\n"
               u"IC { virtual property var f; }"_s
            << ResultBuilder::singleExpected(
                   "Member \"f\" shadows member \"f\" from base type \"IC\", use a different name "
                   "or add a final or override specifier."_L1, 2, 27)
            << defaultOptions;
    QTest::newRow("shadowVirtualWithOverride")
            << u"component IC: Item { virtual property int f; }\n"
               u"IC { override property var f; }"_s
            << ResultBuilder::cleanResult() << defaultOptions;
    QTest::newRow("shadowVirtualWithOverride2")
            << u"component IC: Item { virtual property int f; }\n"
               u"component IC2 :IC { override property var f; }\n"
               u"component IC3 :IC2 { override property var f; }\n"
               u"IC3 { override property var f; }"_s
            << ResultBuilder::cleanResult() << defaultOptions;
    QTest::newRow("shadowVirtualWithFinal") << u"component IC: Item { virtual property int f; }\n"
                                               u"IC { final property var f; }"_s
                                            << ResultBuilder::cleanResult() << defaultOptions;

    {
        CallQmllintOptions options = defaultOptions;
        options.importPaths.append(testFile("ImportPath"));
        QTest::newRow("shadowPropertyFromAnotherFile")
                << u"import ModuleInImportPath\n"
                   u"A { property int myProperty }"_s
                << ResultBuilder::singleExpected(
                       "Property \"myProperty\" already exists in base type \"A\""_L1, 2, 18)
                << options;
    }
}

void TestQmllint::shadow()
{
    // reuse testing logic from dirtyQmlSnippet
    dirtyQmlSnippet();
}

void TestQmllint::uselessExpressionStatements_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<Result>("result");
    QTest::addColumn<CallQmllintOptions>("options");

    const CallQmllintOptions defaultOptions;
    const auto warning = "Expression statement has no obvious effect."_L1;

    QTest::newRow("uselessExpressionStatement")
            << u"property int i: { let x = 0; 0 + 1; return i + 3; }"_s
            << ResultBuilder::singleExpected("Expression statement has no obvious effect."_L1, 1, 30)
            << defaultOptions;

    QTest::newRow("propertyDef-last-simple")
            << u"property int i: 1"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("propertyDef-last-block")
            << u"property int i: { 1 }"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("propertyDef-last-nested")
            << u"property int i: (1)"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("propertyDef-last-blockNested")
            << u"property int i: { (1) }"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("propertyDef-last-complex")
            << u"property int i: { 1 + i < 0 ? ~i : i**i }"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;

    QTest::newRow("propertyBinding-last-simple")
            << u"x: 1"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("propertyBinding-last-block")
            << u"x: { 1 }"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("propertyBinding-last-nested")
            << u"x: (1)"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("propertyBinding-last-blockNested")
            << u"x: { (1) }"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("propertyBinding-last-complex")
            << u"x: { 1 + x < 0 ? ~x : x**x }"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;

    QTest::newRow("propertyDef-dirty1")
            << u"property int i: { 1; 1 }"_s
            << ResultBuilder::singleExpected(warning, 1, 19)
            << defaultOptions;
    QTest::newRow("propertyDef-dirty2")
            << u"property int i: { if (true) 1; 1 }"_s
            << ResultBuilder::singleExpected(warning, 1, 29)
            << defaultOptions;
    QTest::newRow("propertyBinding-dirty1")
            << u"x: { 1; 1 }"_s
            << ResultBuilder::singleExpected(warning, 1, 6)
            << defaultOptions;
    QTest::newRow("propertyBinding-dirty2")
            << u"x: { if (true) 1; 1 }"_s
            << ResultBuilder::singleExpected(warning, 1, 16)
            << defaultOptions;

    QTest::newRow("signalHandler1")
            << u"onXChanged: 1"_s
            << ResultBuilder::singleExpected(warning, 1, 13)
            << defaultOptions;
    QTest::newRow("signalHandler2")
            << u"onXChanged: { 1 }"_s
            << ResultBuilder::singleExpected(warning, 1, 15)
            << defaultOptions;
    QTest::newRow("signalHandler3")
            << u"onXChanged: 9 / 8"_s
            << ResultBuilder::singleExpected(warning, 1, 13)
            << defaultOptions;
    QTest::newRow("signalHandler4")
            << u"id: item; onXChanged: item.dumpItemTree()"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("attachedSignalHandler")
            << u"Component.onCompleted: 1"_s
            << ResultBuilder::singleExpected(warning, 1, 24)
            << defaultOptions;

    QTest::newRow("function1")
            << u"function f() { 1 }"_s
            << ResultBuilder::singleExpected(warning, 1, 16)
            << defaultOptions;
    QTest::newRow("function2")
            << u"function f() { 1; return 1 }"_s
            << ResultBuilder::singleExpected(warning, 1, 16)
            << defaultOptions;
    QTest::newRow("function3")
            << u"function f() { { x + 1 } }"_s
            << ResultBuilder::singleExpected(warning, 1, 18)
            << defaultOptions;

    QTest::newRow("id")
            << u"id: a"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("groupedProperty1")
            << u"anchors { right: anchors.right }"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("groupedProperty2")
            << u"anchors.right: anchors.right"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
    QTest::newRow("groupedProperty3")
            << u"Text { font.bold: { 1; true } }"_s
            << ResultBuilder::singleExpected(warning, 1, 19)
            << defaultOptions;

    QTest::newRow("recursive")
            << uR"( function f() {
                        let i = 0
                        {
                            1;
                            (2)
                        }

                        try {
                            3
                        } catch(e) {
                            4
                        }

                        for (;;)
                            5
                        for (let i in ii) // qmllint disable unqualified
                            6
                        for (let i of ii) // qmllint disable unqualified
                            7

                        while (true)
                            8

                        if (true)
                            9
                        else
                            10

                        switch (i) {
                        case 0:
                            11
                            break
                        default:
                            12
                            break;
                        case 2:
                            13
                        }

                        with (1) // qmllint disable with
                            14

                    label: 15

                        16
                    })"_s
            << ResultBuilder()
               .addExpected(warning, 4, 29)     // block
               .addExpected(warning, 5, 29)     // nested
               .addExpected(warning, 9, 29)     // try
               .addExpected(warning, 11, 29)    // catch
               .addExpected(warning, 15, 29)    // for
               .addExpected(warning, 17, 29)    // for-in
               .addExpected(warning, 19, 29)    // for-of
               .addExpected(warning, 22, 29)    // while
               .addExpected(warning, 25, 29)    // if
               .addExpected(warning, 27, 29)    // else
               .addExpected(warning, 31, 29)    // case 0
               .addExpected(warning, 34, 29)    // default
               .addExpected(warning, 37, 29)    // case 2
               .addExpected(warning, 41, 29)    // with
               .addExpected(warning, 43, 28)    // label
               .addExpected(warning, 45, 25)    // bare
               .build()
            << defaultOptions;

    QTest::newRow("emptyCaseClause")
            << uR"(width: {
                        switch (0) {
                        case 1:
                        default:
                            1;
                        }
                    })"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;

    QTest::newRow("emptyBlock")
            << uR"(width: {
                        if (true)
                            1;
                        else {}
                    })"_s
            << ResultBuilder::cleanResult()
            << defaultOptions;
}

void TestQmllint::uselessExpressionStatements()
{
    QEXPECT_FAIL("attachedSignalHandler", "", Abort);
    QEXPECT_FAIL("groupedProperty3", "", Abort);
    if (QTest::currentDataTag() == "attachedSignalHandler"_L1
            || QTest::currentDataTag() == "groupedProperty3"_L1) {
        QFAIL("Incomplete attached and grouped properties support");
    }

    // reuse testing logic from dirtyQmlSnippet
    dirtyQmlSnippet();
}

void TestQmllint::useProperFunction_data()
{
    // note: use the same column as dirtyQmlSnippet_data() to reuse dirtyQmlSnippet() in shadow().
    QTest::addColumn<QString>("code");
    QTest::addColumn<Result>("result");
    QTest::addColumn<CallQmllintOptions>("options");

    CallQmllintOptions defaultOptions;
    defaultOptions.categorySeverityOverrides[qmlUseProperFunction.name().toString()] =
            QQmlJS::WarningSeverity::Warning;

    QTest::newRow("shadowedMethod")
            << u"function foo() {}\n property bool foo: false"_s
            << ResultBuilder::singleExpected("Duplicated property name \"foo\", \"foo\" is already "
                                             "a method."_L1)
            << defaultOptions;
    QTest::newRow("shadowedSignal")
            << u"MouseArea { Component.onCompleted: pressed(); }"_s
            << ResultBuilder::singleExpected("Property \"pressed\" is not a method")
            << defaultOptions;
    QTest::newRow("shadowedSignalWithId")
            << u"MouseArea { id: mouseArea; Component.onCompleted: mouseArea.pressed() }"_s
            << ResultBuilder::singleExpected("Property \"pressed\" is not a method")
            << defaultOptions;
    QTest::newRow("shadowedSlot")
            << u"ObjectModel { property bool move: false; Component.onCompleted: move(); }"_s
            << ResultBuilder::singleExpected("Property \"move\" is not a method") << defaultOptions;
    QTest::newRow("callJSValue")
            << u"import CallJSValue\n"
               u"TypeWithQJSValue {\n"
               u"     Component.onCompleted: jsValue(42);\n"
               u"}\n"_s
            << ResultBuilder::singleExpected("Property \"jsValue\" is a QJSValue property. It may "
                                             "or may not be a method. Use a regular Q_INVOKABLE "
                                             "instead."_L1)
            << defaultOptions;
    QTest::newRow("callVarProp")
            << u"import QtQml\n"
               u"QtObject {\n"
               u"    property var foo: () => {}\n"
               u"    Component.onCompleted: foo()\n"
               u"}\n"_s
            << ResultBuilder::singleExpected("Property \"foo\" is a var property. It may or may "
                                             "not be a method. Use a regular function instead."_L1)
            << defaultOptions;
}

void TestQmllint::useProperFunction()
{
    // reuse testing logic from dirtyQmlSnippet
    dirtyQmlSnippet();
}

void TestQmllint::noSettingsPollution_data()
{
    const QString aFile = testFile(u"NoSettingsPollution/A.qml"_s);
    const QString bFile = testFile(u"NoSettingsPollution/folder/B.qml"_s);

    QTest::addColumn<QStringList>("args");
    QTest::addColumn<QString>("expectedOutput");

    const QString warning =
            "Warning: %1:5:10: Do not use comma expressions. [comma]\n        1, 1\n         ^\n"_L1
                    .arg(aFile);

    QTest::addRow("a-then-b") << QStringList{ aFile, bFile } << warning;
    QTest::addRow("b-then-a") << QStringList{ bFile, aFile } << warning;
    QTest::addRow("same-file-twice") << QStringList{ bFile, bFile } << u""_s;
}

void TestQmllint::noSettingsPollution()
{
    QFETCH(QStringList, args);
    QFETCH(QString, expectedOutput);

    QProcess qmllint;
    qmllint.setProgram(m_qmllintPath);
    qmllint.setArguments(args);
    qmllint.setProcessChannelMode(QProcess::MergedChannels);
    qmllint.start(QProcess::ReadWrite | QProcess::Text);
    QVERIFY(qmllint.waitForFinished());
    const QString output = qmllint.readAllStandardOutput();
    QCOMPARE(output, expectedOutput);
}

void TestQmllint::syntaxIsEssential()
{
    const auto &builtins = QQmlJSLogger::builtinCategories();
    builtins.first().name();
    const auto it = std::find_if(builtins.cbegin(), builtins.cend(), [](const auto &c) {
        return c.name() == "syntax"_L1;
    });
    QVERIFY(it != builtins.cend());
    QVERIFY(it->isEssential());
}

void TestQmllint::essentialCantBeLowered()
{
    const auto warning = "In order to ensure the proper function of qmllint, the severity of the "
                         "essential category syntax cannot be lowered."_L1;
    {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(m_qmllintPath, { "--syntax=info"_L1, testFile("dummy.qml") });
        QVERIFY(process.waitForFinished());
        const QString output = process.readAllStandardOutput();
        QVERIFY(output.contains(warning));
    }
    {
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(m_qmllintPath, { testFile("essentialCantBeLowered/file.qml") });
        QVERIFY(process.waitForFinished());
        const QString output = process.readAllStandardOutput();
        QVERIFY(output.contains(warning));
    }
}

void TestQmllint::essentialCanBeRaised()
{
    CallQmllintOptions options;
    options.readSettings = true;
    const QJsonArray json = callQmllint(testFile("essentialCanBeRaised/file.qml"), options,
                                        CallQmllintCheck::ShouldFail);

    const QString error = "Nested inline components are not supported"_L1;
    bool foundSyntaxError = false;
    for (const auto &entry : json) {
        const QJsonObject &message = entry.toObject();
        if (message["id"] == "syntax"_L1 && message["type"] == "critical"_L1
                && message["message"].toString().contains(error)) {
            foundSyntaxError = true;
            break;
        }
    }
    QVERIFY(foundSyntaxError);
}

static const auto OnlyExplicitCategories_unqualified = "7:26: Unqualified access"_L1;
static const auto OnlyExplicitCategories_unqualifiedFix = "property bool b: root.i === 1"_L1;
static const auto OnlyExplicitCategories_comma = "11:10: Do not use comma expressions"_L1;
static const auto OnlyExplicitCategories_ifAssign = "12:15: Assignment in condition"_L1;

void TestQmllint::onlyExplicitCategories()
{
    const QString qmlFilePath = testFile("OnlyExplicitCategories/file.qml"_L1);
    bool shouldSucceed = true;
    QStringList extraArgs;
    bool ignoreSettings = false;

    QString output = runQmllint(qmlFilePath, shouldSucceed, extraArgs, ignoreSettings);
    QVERIFY(output.contains(OnlyExplicitCategories_unqualified));
    QVERIFY(output.contains(OnlyExplicitCategories_unqualifiedFix));
    QVERIFY(output.contains(OnlyExplicitCategories_comma));
    QVERIFY(!output.contains(OnlyExplicitCategories_ifAssign)); // per .qmllint.ini

    extraArgs << "--ignore-settings"_L1;
    output = runQmllint(qmlFilePath, shouldSucceed, extraArgs, ignoreSettings);
    QVERIFY(output.contains(OnlyExplicitCategories_unqualified));
    QVERIFY(output.contains(OnlyExplicitCategories_unqualifiedFix));
    QVERIFY(output.contains(OnlyExplicitCategories_comma));
    QVERIFY(output.contains(OnlyExplicitCategories_ifAssign));

    extraArgs << "--only-explicit-categories"_L1;
    output = runQmllint(qmlFilePath, shouldSucceed, extraArgs, ignoreSettings);
    QVERIFY(output.isEmpty());

    extraArgs << QStringList{ "--unqualified=warning"_L1 };
    output = runQmllint(qmlFilePath, shouldSucceed, extraArgs, ignoreSettings);
    QVERIFY(output.contains(OnlyExplicitCategories_unqualified));
    QVERIFY(output.contains(OnlyExplicitCategories_unqualifiedFix));
    QVERIFY(!output.contains(OnlyExplicitCategories_comma));
    QVERIFY(!output.contains(OnlyExplicitCategories_ifAssign));
}

void TestQmllint::onlyExplicitCategoriesIni()
{
    const QString qmlFilePath = testFile("OnlyExplicitCategoriesIni/file.qml"_L1);
    bool shouldSucceed = true;
    QStringList extraArgs;
    bool ignoreSettings = false;

    const QString output = runQmllint(qmlFilePath, shouldSucceed, extraArgs, ignoreSettings);
    QVERIFY(!output.contains(OnlyExplicitCategories_unqualified));
    QVERIFY(!output.contains(OnlyExplicitCategories_unqualifiedFix));
    QVERIFY(output.contains(OnlyExplicitCategories_comma));
    QVERIFY(!output.contains(OnlyExplicitCategories_ifAssign));
}

void TestQmllint::crashes()
{
    CallQmllintOptions options;
    CallQmllintChecks checks;
    const QJsonArray warnings = callQmllint(testFile("propertyChangesCrash.qml"), options, checks);

    QVERIFY(warnings.size() <= 2);

    checkResult(warnings,
                ResultBuilder()
                .addExpected("FooBar was not found. Did you add all imports and dependencies?"_L1)
                .build());
}

void TestQmllint::weakPointers()
{
    // linting scenario from QTBUG-146688, where grouped properties types disappeared when linting
    // these files in exact that order:
    // HomePage populates all other qml scopes
    // NavBarForm.ui.qml used to drop data weakly referenced from NewTaskForm to NavBar
    // NewTask can't find the type weakly referenced from NewTaskForm's "backbutton" property and
    // emits bogus warnings.
    std::array filesToLint = {
        testFile("weakpointers/HomePage.qml"),
        testFile("weakpointers/NavBarForm.ui.qml"),
        testFile("weakpointers/NewTask.qml"),
    };

    // actual testing, prove that this bug does not happen anymore in linter:
    for (const auto &file : filesToLint) {
        m_linter.prepareFileForBatchLinting(file, nullptr, { }, m_defaultImportPaths, { }, { },
                                            m_categories);
    }

    for (const auto &file : filesToLint) {
        auto result = m_linter.lintFileInBatch(file);
        QCOMPARE(result.status, QQmlJSLinter::LintSuccess);
    }
}

void TestQmllint::registerMergeTreeRecursion()
{
    const QString filename = testFile("deeplyMergedTypes.qml");

    runQmllint(filename, [&](QProcess &process) {
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);
    });
}

QTEST_GUILESS_MAIN(TestQmllint)
#include "tst_qmllint.moc"
