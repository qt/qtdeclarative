// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/private/qemulationdetector_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomlinewriter_p.h>
#include <QtQmlDom/private/qqmldomlinewriterfactory_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlToolingSettings/private/qqmltoolingsettings_p.h>
#include <QtQmlFormat/private/qqmlformatoptions_p.h>

#include "tst_qmlformat_base.h"

using namespace QQmlJS::Dom;

class TestQmlformat : public TestQmlformatBase
{
    Q_OBJECT

public:
    enum class RunOption { OnCopy, OrigToCopy };
    TestQmlformat();

private Q_SLOTS:
#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
    void testExample();
    void testExample_data();
    void normalizeExample();
    void normalizeExample_data();
#endif
    void plainJS_data();
    void plainJS();

    void ecmascriptModule();

    void qml_data();
    void qml();

    void qmlSnippet_data();
    void qmlSnippet();

    void semicolonRule_data();
    void semicolonRule();

    void disableViaComments_data();
    void disableViaComments();

    void normalizedId_data();
    void normalizedId();

private:
    QString formatInMemory(const QString &fileToFormat, bool *didSucceed = nullptr,
                           LineWriterOptions options = LineWriterOptions(),
                           WriteOutChecks extraChecks = WriteOutCheck::ReparseCompare,
                           WriteOutChecks largeChecks = WriteOutCheck::None);
    QString formatSnippetInMemory(const QString &snippet, bool *didSucceed,
                                  LineWriterOptions options);
    QString formatInMemoryImpl(bool *didSucceed, LineWriterOptions options,
                               WriteOutChecks extraChecks, WriteOutChecks largeChecks,
                               std::shared_ptr<DomEnvironment> &&env, FileToLoad &&fileToLoad);
};

// Don't fail on warnings because we read a lot of QML files that might intentionally be malformed.
TestQmlformat::TestQmlformat()
    : TestQmlformatBase(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::DoNotFailOnWarnings)
{
}

void TestQmlformat::plainJS_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("fileFormatted");

    QTest::newRow("simpleStatement") << "simpleJSStatement.js"
                                     << "simpleJSStatement.formatted.js";
    QTest::newRow("simpleFunction") << "simpleOnelinerJSFunc.js"
                                    << "simpleOnelinerJSFunc.formatted.js";
    QTest::newRow("simpleLoop") << "simpleLoop.js"
                                << "simpleLoop.formatted.js";
    QTest::newRow("messyIfStatement") << "messyIfStatement.js"
                                      << "messyIfStatement.formatted.js";
    QTest::newRow("lambdaFunctionWithLoop") << "lambdaFunctionWithLoop.js"
                                            << "lambdaFunctionWithLoop.formatted.js";
    QTest::newRow("lambdaWithIfElse") << "lambdaWithIfElse.js"
                                      << "lambdaWithIfElse.formatted.js";
    QTest::newRow("nestedLambdaWithIfElse") << "lambdaWithIfElseInsideLambda.js"
                                            << "lambdaWithIfElseInsideLambda.formatted.js";
    QTest::newRow("twoFunctions") << "twoFunctions.js"
                                  << "twoFunctions.formatted.js";
    QTest::newRow("pragma") << "pragma.js"
                            << "pragma.formatted.js";
    QTest::newRow("classConstructor") << "class.js"
                                      << "class.formatted.js";
    QTest::newRow("legacyDirectives") << "directives.js"
                                      << "directives.formatted.js";
    QTest::newRow("legacyDirectivesWithComments") << "directivesWithComments.js"
                                                  << "directivesWithComments.formatted.js";
    QTest::newRow("preserveOptionalTokens") << "preserveOptionalTokens.js"
                                            << "preserveOptionalTokens.formatted.js";
    QTest::newRow("noSuperfluousSpaceInsertions.fail_pragma")
            << "noSuperfluousSpaceInsertions.fail_pragma.js"
            << "noSuperfluousSpaceInsertions.fail_pragma.formatted.js";
    QTest::newRow("fromAsIdentifier") << "fromAsIdentifier.js"
                                      << "fromAsIdentifier.formatted.js";
    QTest::newRow("caseWithComment") << "caseWithComment.js"
                                     << "caseWithComment.formatted.js";
    QTest::newRow("longStatementList") << "longStatementList.js"
                                       << "longStatementList.formatted.js";
}

void TestQmlformat::plainJS()
{
    QFETCH(QString, file);
    QFETCH(QString, fileFormatted);

    bool wasSuccessful;
    LineWriterOptions opts;
#ifdef Q_OS_WIN
    opts.lineEndings = QQmlJS::Dom::LineWriterOptions::LineEndings::Windows;
#endif
    QString output = formatInMemory(testFile(file), &wasSuccessful, opts, WriteOutCheck::None);

    QVERIFY(wasSuccessful && !output.isEmpty());

    // TODO(QTBUG-119770)
    QEXPECT_FAIL("legacyDirectivesWithComments", "see QTBUG-119770", Abort);
    QEXPECT_FAIL("noSuperfluousSpaceInsertions.fail_pragma",
                 "Not all cases have been covered yet (QTBUG-133315)", Abort);
    auto exp = readTestFile(fileFormatted);
    QCOMPARE(output, exp);
}

void TestQmlformat::ecmascriptModule()
{
    QString file("esm.mjs");
    QString formattedFile("esm.formatted.mjs");

    bool wasSuccessful;
    LineWriterOptions opts;
#ifdef Q_OS_WIN
    opts.lineEndings = QQmlJS::Dom::LineWriterOptions::LineEndings::Windows;
#endif
    QString output = formatInMemory(testFile(file), &wasSuccessful, opts, WriteOutCheck::None);

    QVERIFY(wasSuccessful && !output.isEmpty());

    auto exp = readTestFile(formattedFile);
    QCOMPARE(output, readTestFile(formattedFile));
}

#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
void TestQmlformat::testExample_data()
{
    if (QTestPrivate::isRunningArmOnX86())
        QSKIP("Crashes in QEMU. (timeout)");
    QTest::addColumn<QString>("file");

    QString examples = QLatin1String(SRCDIR) + "/../../../../examples/";
    QString tests = QLatin1String(SRCDIR) + "/../../../../tests/";

    QStringList exampleFiles;
    QStringList testFiles;
    QStringList files;
    exampleFiles << findFiles(QDir(examples));
    testFiles << findFiles(QDir(tests));

    // Actually this test is an e2e test and not the unit test.
    // At the moment of writing, CI lacks providing instruments for the automated tests
    // which might be time-consuming, as for example this one.
    // Therefore as part of QTBUG-122990 this test was copied to the /manual/e2e/qml/qmlformat
    // however very small fraction of the test data is still preserved here for the sake of
    // testing automatically at least a small part of the examples
    const int nBatch = 10;
    files << exampleFiles.mid(0, nBatch) << exampleFiles.mid(exampleFiles.size() / 2, nBatch)
          << exampleFiles.mid(exampleFiles.size() - nBatch, nBatch);
    files << testFiles.mid(0, nBatch) << testFiles.mid(exampleFiles.size() / 2, nBatch)
          << testFiles.mid(exampleFiles.size() - nBatch, nBatch);

    for (const QString &file : files)
        QTest::newRow(qPrintable(file)) << file;
}

void TestQmlformat::normalizeExample_data()
{
    if (QTestPrivate::isRunningArmOnX86())
        QSKIP("Crashes in QEMU. (timeout)");
    QTest::addColumn<QString>("file");

    QString examples = QLatin1String(SRCDIR) + "/../../../../examples/";
    QString tests = QLatin1String(SRCDIR) + "/../../../../tests/";

    // normalizeExample is similar to testExample, so we test it only on nExamples + nTests
    // files to avoid making too many
    QStringList files;
    const int nExamples = 10;
    int i = 0;
    for (const auto &f : findFiles(QDir(examples))) {
        files << f;
        if (++i == nExamples)
            break;
    }
    const int nTests = 10;
    i = 0;
    for (const auto &f : findFiles(QDir(tests))) {
        files << f;
        if (++i == nTests)
            break;
    }

    for (const QString &file : files)
        QTest::newRow(qPrintable(file)) << file;
}
#endif

#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
void TestQmlformat::testExample()
{
    QFETCH(QString, file);
    const bool isInvalid = isInvalidFile(QFileInfo(file));
    bool wasSuccessful;
    LineWriterOptions opts;
    opts.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;
    QString output = formatInMemory(file, &wasSuccessful, opts);

    if (!isInvalid)
        QVERIFY(wasSuccessful && !output.isEmpty());
}

void TestQmlformat::normalizeExample()
{
    QFETCH(QString, file);
    const bool isInvalid = isInvalidFile(QFileInfo(file));
    bool wasSuccessful;
    LineWriterOptions opts;
    opts.attributesSequence = LineWriterOptions::AttributesSequence::Normalize;
    QString output = formatInMemory(file, &wasSuccessful, opts);

    if (!isInvalid)
        QVERIFY(wasSuccessful && !output.isEmpty());
}
#endif

QString TestQmlformat::formatInMemory(const QString &fileToFormat, bool *didSucceed,
                                      LineWriterOptions options, WriteOutChecks extraChecks,
                                      WriteOutChecks largeChecks)
{
    auto env = DomEnvironment::create(
            QStringList(), // as we load no dependencies we do not need any paths
            QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                    | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

    return formatInMemoryImpl(didSucceed, options, extraChecks, largeChecks, std::move(env),
                              FileToLoad::fromFileSystem(env, fileToFormat));
}

QString TestQmlformat::formatInMemoryImpl(bool *didSucceed, LineWriterOptions options,
                                          WriteOutChecks extraChecks, WriteOutChecks largeChecks,
                                          std::shared_ptr<DomEnvironment> &&env,
                                          FileToLoad &&fileToLoad)
{
    DomItem tFile;
    env->loadFile(fileToLoad,
                  [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; });
    env->loadPendingDependencies();
    MutableDomItem myFile = tFile.field(Fields::currentItem);

    bool writtenOut = false;
    QString resultStr;
    if (myFile.field(Fields::isValid).value().toBool()) {
        WriteOutChecks checks = extraChecks;
        const qsizetype largeFileSize = 32000;
        if (tFile.field(Fields::code).value().toString().size() > largeFileSize)
            checks = largeChecks;

        QTextStream res(&resultStr);
        auto lw = QQmlJS::Dom::createLineWriter([&res](QStringView s) { res << s; },
                QLatin1String("*testStream*"), options);
        DomItem qmlFile = tFile.field(Fields::currentItem);
        OutWriter ow(getFileItemOwner(qmlFile), *lw);
        ow.indentNextlines = true;
        writtenOut = qmlFile.writeOutForFile(ow, checks);
        lw->eof();
        res.flush();
    }
    if (didSucceed)
        *didSucceed = writtenOut;
    return resultStr;
}

QString TestQmlformat::formatSnippetInMemory(const QString &snippet, bool *didSucceed,
                                             LineWriterOptions options)
{
    auto env = DomEnvironment::create(
            QStringList(), // as we load no dependencies we do not need any paths
            QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                    | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

    return formatInMemoryImpl(didSucceed, options, WriteOutCheck::None, WriteOutCheck::None,
                              std::move(env),
                              FileToLoad::fromMemory(env, testFile("file.qml"), snippet));
}

void TestQmlformat::qml_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("fileFormatted");

    QTest::newRow("example1") << "Example1.qml"
                              << "Example1.formatted.qml";
    QTest::newRow("annotation") << "Annotations.qml"
                                << "Annotations.formatted.qml";
    QTest::newRow("front inline") << "FrontInline.qml"
                                  << "FrontInline.formatted.qml";
    QTest::newRow("if blocks") << "IfBlocks.qml"
                               << "IfBlocks.formatted.qml";
    QTest::newRow("read-only properties") << "readOnlyProps.qml"
                                          << "readOnlyProps.formatted.qml";
    QTest::newRow("states and transitions") << "statesAndTransitions.qml"
                                            << "statesAndTransitions.formatted.qml";
    QTest::newRow("large bindings") << "largeBindings.qml"
                                    << "largeBindings.formatted.qml";
    QTest::newRow("verbatim strings") << "verbatimString.qml"
                                      << "verbatimString.formatted.qml";
    QTest::newRow("inline components") << "inlineComponents.qml"
                                       << "inlineComponents.formatted.qml";
    QTest::newRow("nested ifs") << "nestedIf.qml"
                                << "nestedIf.formatted.qml";
    QTest::newRow("QTBUG-85003") << "QtBug85003.qml"
                                 << "QtBug85003.formatted.qml";
    QTest::newRow("nested functions") << "nestedFunctions.qml"
                                      << "nestedFunctions.formatted.qml";
    QTest::newRow("multiline comments") << "multilineComment.qml"
                                        << "multilineComment.formatted.qml";
    QTest::newRow("for of") << "forOf.qml"
                            << "forOf.formatted.qml";
    QTest::newRow("property names") << "propertyNames.qml"
                                    << "propertyNames.formatted.qml";
    QTest::newRow("empty object") << "emptyObject.qml"
                                  << "emptyObject.formatted.qml";
    QTest::newRow("arrow functions") << "arrowFunctions.qml"
                                     << "arrowFunctions.formatted.qml";
    QTest::newRow("forWithLet") << "forWithLet.qml"
                                << "forWithLet.formatted.qml";
    QTest::newRow("dontRemoveComments") << "dontRemoveComments.qml"
                                        << "dontRemoveComments.formatted.qml";
    QTest::newRow("ecmaScriptClassInQml") << "ecmaScriptClassInQml.qml"
                                          << "ecmaScriptClassInQml.formatted.qml";
    QTest::newRow("arrowFunctionWithBinding") << "arrowFunctionWithBinding.qml"
                                              << "arrowFunctionWithBinding.formatted.qml";
    QTest::newRow("blanklinesAfterComment") << "blanklinesAfterComment.qml"
                                            << "blanklinesAfterComment.formatted.qml";
    QTest::newRow("pragmaValueList") << "pragma.qml"
                                     << "pragma.formatted.qml";
    QTest::newRow("objectDestructuring") << "objectDestructuring.qml"
                                         << "objectDestructuring.formatted.qml";
    QTest::newRow("destructuringFunctionParameter")
            << "destructuringFunctionParameter.qml"
            << "destructuringFunctionParameter.formatted.qml";
    QTest::newRow("ellipsisFunctionArgument") << "ellipsisFunctionArgument.qml"
                                              << "ellipsisFunctionArgument.formatted.qml";
    QTest::newRow("importStatements") << "importStatements.qml"
                                      << "importStatements.formatted.qml";
    QTest::newRow("arrayEndComma") << "arrayEndComma.qml"
                                   << "arrayEndComma.formatted.qml";
    QTest::newRow("escapeChars") << "escapeChars.qml"
                                 << "escapeChars.formatted.qml";
    QTest::newRow("javascriptBlock") << "javascriptBlock.qml"
                                     << "javascriptBlock.formatted.qml";
    QTest::newRow("enumWithValues") << "enumWithValues.qml"
                                    << "enumWithValues.formatted.qml";
    QTest::newRow("typeAnnotatedSignal") << "signal.qml"
                                         << "signal.formatted.qml";
    // plainJS
    QTest::newRow("nestedLambdaWithIfElse") << "lambdaWithIfElseInsideLambda.js"
                                            << "lambdaWithIfElseInsideLambda.formatted.js";
    QTest::newRow("noSuperfluousSpaceInsertions") << "noSuperfluousSpaceInsertions.qml"
                                                  << "noSuperfluousSpaceInsertions.formatted.qml";
    QTest::newRow("noSuperfluousSpaceInsertions.fail_id")
            << "noSuperfluousSpaceInsertions.fail_id.qml"
            << "noSuperfluousSpaceInsertions.fail_id.formatted.qml";
    QTest::newRow("noSuperfluousSpaceInsertions.fail_QtObject")
            << "noSuperfluousSpaceInsertions_QtObject.qml"
            << "noSuperfluousSpaceInsertions_QtObject.formatted.qml";
    QTest::newRow("noSuperfluousSpaceInsertions_signal")
            << "noSuperfluousSpaceInsertions_signal.qml"
            << "noSuperfluousSpaceInsertions_signal.formatted.qml";
    QTest::newRow("noSuperfluousSpaceInsertions_enum")
            << "noSuperfluousSpaceInsertions_enum.qml"
            << "noSuperfluousSpaceInsertions_enum.formatted.qml";
    QTest::newRow("noSuperfluousSpaceInsertions.fail_parameters")
            << "noSuperfluousSpaceInsertions.fail_parameters.qml"
            << "noSuperfluousSpaceInsertions.fail_parameters.formatted.qml";
    QTest::newRow("nonInitializedPropertyInComponent")
            << "nonInitializedPropertyInComponent.qml"
            << "nonInitializedPropertyInComponent.formatted.qml";
    QTest::newRow("fromAsIdentifier") << "fromAsIdentifier.qml"
                                      << "fromAsIdentifier.formatted.qml";
    QTest::newRow("finalProperties") << "finalProperties.qml"
                                     << "finalProperties.formatted.qml";
    QTest::newRow("commentsStressTest_enum") << "commentsStressTest_enum.qml"
                                             << "commentsStressTest_enum.formatted.qml";

    QTest::newRow("commentInEnum") << "commentInEnum.qml"
                                   << "commentInEnum.formatted.qml";
    QTest::newRow("commentInQmlObject") << "commentInQmlObject.qml"
                                        << "commentInQmlObject.formatted.qml";
    QTest::newRow("commentsOnArrayAndObjectPatterns")
            << "commentsOnArrayAndObjectPatterns.qml"
            << "commentsOnArrayAndObjectPatterns.formatted.qml";
    QTest::newRow("commentsOnGenerator") << "commentsOnGenerator.qml"
                                         << "commentsOnGenerator.formatted.qml";
    QTest::newRow("commentsOnTryCatchFinally") << "commentsOnTryCatchFinally.qml"
                                               << "commentsOnTryCatchFinally.formatted.qml";
}
void TestQmlformat::qml()
{
    QFETCH(QString, file);
    QFETCH(QString, fileFormatted);

    bool wasSuccessful;
    LineWriterOptions opts;
    opts.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;
#ifdef Q_OS_WIN
    opts.lineEndings = QQmlJS::Dom::LineWriterOptions::LineEndings::Windows;
#endif
    QString output = formatInMemory(testFile(file), &wasSuccessful, opts, WriteOutCheck::None);
    QVERIFY(wasSuccessful && !output.isEmpty());
    auto exp = readTestFile(fileFormatted);
    QEXPECT_FAIL("noSuperfluousSpaceInsertions.fail_id",
                 "Not all cases have been covered yet (QTBUG-133315, QTBUG-123386)", Abort);
    QEXPECT_FAIL("noSuperfluousSpaceInsertions.fail_parameters",
                 "Not all cases have been covered yet (QTBUG-133315, QTBUG-123386)", Abort);
    QCOMPARE(output, exp);
}

void TestQmlformat::qmlSnippet_data()
{
    QTest::addColumn<QString>("unformatted");
    QTest::addColumn<QString>("expectedResult");
    QTest::addColumn<LineWriterOptions>("opts");

    LineWriterOptions defaultOptions ;
    defaultOptions.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;
    // the expected snippets use unix newlines, also on windows
    defaultOptions.lineEndings = LineWriterOptions::LineEndings::Unix;

    {
        LineWriterOptions maxColumnWidth = defaultOptions;
        maxColumnWidth.maxLineLength = 80;
        QTest::addRow("LambdaMaxColumnWidth")
                << uR"(Item { Component.onCompleted: () => { console.info("a"); A.b.c = rs => { let x = 3; return x; }; }})"_s
                << uR"(Item {
    Component.onCompleted: () => {
        console.info("a");
        A.b.c = rs => {
            let x = 3;
            return x;
        };
    }
}
)"_s
                << maxColumnWidth;
    }

    QTest::addRow("QmlObjectComment")
            << u"EIUMAPQTE.ResourceMapHistoryControls { // qmllint disable required\n}"_s
            << u"EIUMAPQTE.ResourceMapHistoryControls { // qmllint disable required\n}\n"_s
            << defaultOptions;
}

void TestQmlformat::qmlSnippet()
{
    QFETCH(QString, unformatted);
    QFETCH(QString, expectedResult);
    QFETCH(LineWriterOptions, opts);

    constexpr QLatin1String snippetTemplate = R"(import QtQuick

%1)"_L1;

    bool wasSuccessful;
    QString output = formatSnippetInMemory(snippetTemplate.arg(unformatted), &wasSuccessful, opts);
    QVERIFY(wasSuccessful && !output.isEmpty());
    QCOMPARE(output, snippetTemplate.arg(expectedResult));
}

void TestQmlformat::semicolonRule_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("formattedFile");
    QTest::addColumn<LineWriterOptions>("opts");
    {
        LineWriterOptions opts;
        opts.semicolonRule = LineWriterOptions::SemicolonRule::Always;
        QTest::newRow("keywords-always") << "semicolon/keywords.js"
                                         << "semicolon/keywords.always.formatted.js" << opts;
        QTest::newRow("restrictedChars-always")
                << "semicolon/restrictedChars.js"
                << "semicolon/restrictedChars.always.formatted.js" << opts;
        QTest::newRow("emptyStatements-always")
                << "semicolon/emptyStatements.qml"
                << "semicolon/emptyStatements.always.formatted.qml" << opts;
    }
    {
        LineWriterOptions opts;
        opts.semicolonRule = LineWriterOptions::SemicolonRule::Essential;
        QTest::newRow("keywords-essential") << "semicolon/keywords.js"
                                            << "semicolon/keywords.essential.formatted.js" << opts;
        QTest::newRow("restrictedChars-essential")
                << "semicolon/restrictedChars.js"
                << "semicolon/restrictedChars.essential.formatted.js" << opts;
        QTest::newRow("emptyStatements-essential")
                << "semicolon/emptyStatements.qml"
                << "semicolon/emptyStatements.essential.formatted.qml" << opts;
    }
}

void TestQmlformat::semicolonRule()
{
    QFETCH(QString, file);
    QFETCH(QString, formattedFile);
    QFETCH(LineWriterOptions, opts);

    bool wasSuccessful = false;

#ifdef Q_OS_WIN
    opts.lineEndings = QQmlJS::Dom::LineWriterOptions::LineEndings::Windows;
#endif
    QString output = formatInMemory(testFile(file), &wasSuccessful, opts, WriteOutCheck::None);

    QVERIFY(wasSuccessful && !output.isEmpty());
    QCOMPARE(output, readTestFile(formattedFile));
}

void TestQmlformat::disableViaComments_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("fileFormatted");
    QTest::addColumn<LineWriterOptions>("opts");
    {
        // Basic test for the disableViaComments feature
        LineWriterOptions opts;
        opts.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;
        QTest::newRow("disableFormat") << "disableViaComments/qmlobject.qml"
                                       << "disableViaComments/qmlobject.formatted.qml" << opts;
        QTest::newRow("disableFormatScriptExpressions")
                << "disableViaComments/scriptExpressions.qml"
                << "disableViaComments/scriptExpressions.formatted.qml" << opts;
    }
    {
        // Basic test non-functional disableViaComments feature, i.e in normalized mode and
        // sortImports
        LineWriterOptions opt1;
        opt1.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;
        opt1.sortImports = true;
        QTest::newRow("disableFormatInSortImports")
                << "disableViaComments/qmlobject.qml"
                << "disableViaComments/qmlobject.nodisable.qml" << opt1;
        opt1.attributesSequence = LineWriterOptions::AttributesSequence::Normalize;
        QTest::newRow("disableFormatInNormalizedMode")
                << "disableViaComments/scriptExpressions.qml"
                << "disableViaComments/scriptExpressions.nodisable.qml" << opt1;
    }
    {
        // Basic test for the disableViaComments feature
        LineWriterOptions opts;
        opts.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;
        QTest::newRow("disableFormatFuzzy") << "disableViaComments/fuzzy.qml"
                                            << "disableViaComments/fuzzy.formatted.qml" << opts;
    }
}

void TestQmlformat::disableViaComments()
{
    QFETCH(QString, file);
    QFETCH(QString, fileFormatted);
    QFETCH(LineWriterOptions, opts);

    bool wasSuccessful = false;

#ifdef Q_OS_WIN
    opts.lineEndings = QQmlJS::Dom::LineWriterOptions::LineEndings::Windows;
#endif
    QString output = formatInMemory(testFile(file), &wasSuccessful, opts, WriteOutCheck::None);
    QVERIFY(wasSuccessful && !output.isEmpty());
    auto exp = readTestFile(fileFormatted);
    QCOMPARE(output, exp);
}

void TestQmlformat::normalizedId_data()
{
    QTest::addColumn<QString>("fileNameBase");

    QTest::addRow("soloId") << "normalizedSoloId";
    QTest::addRow("soloIdCommented") << "normalizedSoloIdCommented";
    QTest::addRow("idAndItem") << "normalizedIdAndItem";
    QTest::addRow("idAndItemCommented") << "normalizedIdAndItemCommented";
}

void TestQmlformat::normalizedId()
{
    QFETCH(QString, fileNameBase);

    bool wasSuccessful = false;

    LineWriterOptions opts;
    opts.attributesSequence = QQmlJS::Dom::LineWriterOptions::AttributesSequence::Normalize;
#ifdef Q_OS_WIN
    opts.lineEndings = QQmlJS::Dom::LineWriterOptions::LineEndings::Windows;
#endif

    const QString file = testFile(fileNameBase + ".qml");
    QString output = formatInMemory(file, &wasSuccessful, opts, WriteOutCheck::None);
    QVERIFY(wasSuccessful && !output.isEmpty());
    auto exp = readTestFile(fileNameBase + ".formatted.qml");
    QCOMPARE(output, exp);
}

QTEST_MAIN(TestQmlformat)
#include "tst_qmlformat.moc"
