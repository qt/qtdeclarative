// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_highlighting.h"

#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlLS/private/qqmlsemantictokens_p.h>
#include <QtCore/qlibraryinfo.h>
#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>

#include <qlist.h>

using namespace QLspSpecification;

tst_qmlls_highlighting::tst_qmlls_highlighting()
    : QQmlDataTest(QT_QMLLS_HIGHLIGHTS_DATADIR) , m_highlightingDataDir(QT_QMLLS_HIGHLIGHTS_DATADIR + "/highlights"_L1)
{
}

// Token encoding as in:
// https://microsoft.github.io/language-server-protocol/specifications/specification-3-16/#textDocument_semanticTokens
void tst_qmlls_highlighting::encodeSemanticTokens_data()
{
    QTest::addColumn<Highlights>("highlights");
    QTest::addColumn<QList<int>>("expectedMemoryLayout");

    {
        Highlights c;
        c.highlights().insert(0, Token());
        QTest::addRow("empty-token-single") << c << QList {0, 0, 0, 0, 0};
    }
    {
        Highlights c;
        QQmlJS::SourceLocation loc(0, 1, 1, 1);
        c.highlights().insert(0, Token(loc, 0, 0));
        QTest::addRow("single-token") << c << QList {0, 0, 1, 0, 0};
    }
    {
        Highlights c;
        Token t1(QQmlJS::SourceLocation(0, 1, 1, 1), 0, 0);
        Token t2(QQmlJS::SourceLocation(1, 1, 3, 3), 0, 0);
        c.highlights().insert(t1.offset, t1);
        c.highlights().insert(t2.offset, t2);
        QTest::addRow("different-lines") << c << QList {0, 0, 1, 0, 0, 2, 2, 1, 0, 0};
    }
    {
        Highlights c;
        Token t1(QQmlJS::SourceLocation(0, 1, 1, 1), 0, 0);
        Token t2(QQmlJS::SourceLocation(1, 1, 1, 3), 0, 0);
        c.highlights().insert(t1.offset, t1);
        c.highlights().insert(t2.offset, t2);
        QTest::addRow("same-line-different-column") << c << QList {0, 0, 1, 0, 0, 0, 2, 1, 0, 0};
    }
    {
        Highlights c;
        Token t1(QQmlJS::SourceLocation(0, 1, 1, 1), 1, 0);
        c.highlights().insert(t1.offset, t1);
        QTest::addRow("token-type") << c << QList {0, 0, 1, 1, 0};
    }
    {
        Highlights c;
        Token t1(QQmlJS::SourceLocation(0, 1, 1, 1), 1, 1);
        c.highlights().insert(t1.offset, t1);
        QTest::addRow("token-modifier") << c << QList {0, 0, 1, 1, 1};
    }
}

void tst_qmlls_highlighting::encodeSemanticTokens()
{
    QFETCH(Highlights, highlights);
    QFETCH(QList<int>, expectedMemoryLayout);
    const auto encoded = HighlightingUtils::encodeSemanticTokens(highlights);
    QCOMPARE(encoded, expectedMemoryLayout);
}

struct LineLength
{
    quint32 startLine;
    quint32 length;
};

void tst_qmlls_highlighting::sourceLocationsFromMultiLineToken_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QList<LineLength>>("expectedLines");

    QTest::addRow("multilineComment1") << R"("line 1
line 2
line 3 ")" << QList{ LineLength{ 1, 7 }, LineLength{ 2, 6 }, LineLength{ 3, 8 } };

    QTest::addRow("prePostNewlines") <<
            R"("

")" << QList{ LineLength{ 1, 1 }, LineLength{ 2, 0 }, LineLength{ 3, 1 } };
    QTest::addRow("windows-newline")
            << QString::fromUtf8("\"test\r\nwindows\r\nnewline\"")
            << QList{ LineLength{ 1, 5 }, LineLength{ 2, 7 }, LineLength{ 3, 8 } };
}

void tst_qmlls_highlighting::sourceLocationsFromMultiLineToken()
{
    QFETCH(QString, source);
    QFETCH(QList<LineLength>, expectedLines);
    using namespace QQmlJS::AST;

    QQmlJS::Engine jsEngine;
    QQmlJS::Lexer lexer(&jsEngine);
    lexer.setCode(source, 1, true);
    QQmlJS::Parser parser(&jsEngine);
    parser.parseExpression();
    const auto expression = parser.expression();

    auto *literal = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(expression);
    const auto locs =
            HighlightingUtils::sourceLocationsFromMultiLineToken(source, literal->literalToken);

    [&]() {
        QCOMPARE(locs.size(), expectedLines.size());

        for (auto i = 0; i < locs.size(); ++i) {
            QCOMPARE(locs[i].startLine, expectedLines[i].startLine);
            QCOMPARE(locs[i].length, expectedLines[i].length);
        }
    }();

    if (QTest::currentTestFailed()) {

        qDebug() << "Actual locations";
        for (auto i = 0; i < locs.size(); ++i) {
            qDebug() << "Startline :" << locs[i].startLine << "Length " << locs[i].length;
        }

        qDebug() << "Expected locations";
        for (auto i = 0; i < expectedLines.size(); ++i) {
            qDebug() << "Startline :" << expectedLines[i].startLine
                     << "Length :" << expectedLines[i].length;
        }
    }
}

void tst_qmlls_highlighting::highlights_data()
{
    using namespace QQmlJS::Dom;
    using namespace HighlightingUtils;
    QTest::addColumn<DomItem>("fileItem");
    QTest::addColumn<Token>("expectedHighlightedToken");

    const auto fileObject = [](const QString &filePath){
        QFile f(filePath);
        DomItem file;
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return file;
        QString code = f.readAll();
        DomCreationOptions options;
        options.setFlag(DomCreationOption::WithScriptExpressions);
        options.setFlag(DomCreationOption::WithSemanticAnalysis);
        options.setFlag(DomCreationOption::WithRecovery);

        QStringList dirs = {QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath)};
        auto envPtr = DomEnvironment::create(
                dirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded, options);
        envPtr->loadBuiltins();
        envPtr->loadFile(FileToLoad::fromMemory(envPtr, filePath, code),
                         [&file](Path, const DomItem &, const DomItem &newIt) {
                             file = newIt.fileObject();
                         });
        envPtr->loadPendingDependencies();
        return file;
    };

    { // Comments
        const auto filePath = m_highlightingDataDir + "/comments.qml";
        const auto fileItem = fileObject(filePath);
        // Copyright (C) 2023 The Qt Company Ltd.
        QTest::addRow("single-line-1")
                << fileItem
                << Token(QQmlJS::SourceLocation(0, 41, 1, 1),
                         int(SemanticTokenProtocolTypes::Comment), 0);

        /* single line comment    */
        QTest::addRow("single-line-2")
                << fileItem
                << Token(QQmlJS::SourceLocation(162, 28, 9, 1),
                         int(SemanticTokenProtocolTypes::Comment), 0);

        // Multiline comments are split into multiple locations
        QTest::addRow("multiline-first-line")
                << fileItem
                << Token(QQmlJS::SourceLocation(133, 2, 5, 1),
                         int(SemanticTokenProtocolTypes::Comment), 0);
        QTest::addRow("multiline-second-line")
                << fileItem
                << Token(QQmlJS::SourceLocation(136, 21, 6, 1),
                         int(SemanticTokenProtocolTypes::Comment), 0);
        QTest::addRow("multiline-third-line")
                << fileItem
                << Token(QQmlJS::SourceLocation(158, 2, 7, 1),
                         int(SemanticTokenProtocolTypes::Comment), 0);

        // Comments Inside Js blocks
        QTest::addRow("inside-js")
                << fileItem
                << Token(QQmlJS::SourceLocation(232, 5, 13, 9),
                         int(SemanticTokenProtocolTypes::Comment), 0);
    }
    { // Imports
        const auto filePath = m_highlightingDataDir + "/imports.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("import-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(112, 6, 4, 1),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("module-uri")
                << fileItem
                << Token(QQmlJS::SourceLocation(119, 7, 4, 8),
                         int(SemanticTokenProtocolTypes::Namespace), 0);
        QTest::addRow("directory-uri")
                << fileItem
                << Token(QQmlJS::SourceLocation(152, 3, 6, 8),
                         int(SemanticTokenProtocolTypes::String), 0);
        QTest::addRow("as-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(156, 2, 6, 12),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("version-number")
                << fileItem
                << Token(QQmlJS::SourceLocation(140, 4, 5, 14),
                         int(SemanticTokenProtocolTypes::Number), 0);
        QTest::addRow("qualified-namespace")
                << fileItem
                << Token(QQmlJS::SourceLocation(159, 6, 6, 15),
                         int(SemanticTokenProtocolTypes::Namespace), 0);
    }
    { // Bindings
        const auto filePath = m_highlightingDataDir + "/bindings.qml";
        const auto fileItem = fileObject(filePath);

        // normal binding
        QTest::addRow("normalBinding")
                << fileItem
                << Token(QQmlJS::SourceLocation(189, 1, 11, 5),
                         int(SemanticTokenProtocolTypes::Property), 0);
        // on binding
        QTest::addRow("on-binding")
                << fileItem
                << Token(QQmlJS::SourceLocation(175, 5, 9, 17),
                         int(SemanticTokenProtocolTypes::Property), 0);
        QTest::addRow("on-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(172, 2, 9, 14),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
    }
    { // Pragmas
        const auto filePath = m_highlightingDataDir + "/pragmas.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("pragma-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(112, 6, 4, 1),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("pragma-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(136, 25, 5, 8),
                         int(SemanticTokenProtocolTypes::Variable), 0);
        QTest::addRow("pragma-value")
                << fileItem
                << Token(QQmlJS::SourceLocation(198, 4, 6, 27),
                         int(SemanticTokenProtocolTypes::Variable), 0);
    }
    { // Enums
        const auto filePath = m_highlightingDataDir + "/enums.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("enum-keyword") << fileItem
                                      << Token(QQmlJS::SourceLocation(158, 4, 8, 5),
                                               int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("enum-name") << fileItem
                                   << Token(QQmlJS::SourceLocation(163, 3, 8, 10),
                                            int(SemanticTokenProtocolTypes::Enum), 0);
        QTest::addRow("enum-item") << fileItem
                                   << Token(QQmlJS::SourceLocation(177, 3, 9, 9),
                                            int(SemanticTokenProtocolTypes::EnumMember), 0);
        QTest::addRow("enum-value") << fileItem
                                    << Token(QQmlJS::SourceLocation(196, 1, 10, 15),
                                             int(SemanticTokenProtocolTypes::Number), 0);
        QTest::addRow("namespace-enum") << fileItem
                                        << Token(QQmlJS::SourceLocation(225, 1, 13, 21),
                                                 int(SemanticTokenProtocolTypes::Namespace), 0);
        QTest::addRow("component-enum") << fileItem
                                        << Token(QQmlJS::SourceLocation(227, 11, 13, 23),
                                                 int(SemanticTokenProtocolTypes::Type), 0);
        QTest::addRow("enum-name-1") << fileItem
                                     << Token(QQmlJS::SourceLocation(239, 1, 13, 35),
                                              int(SemanticTokenProtocolTypes::Enum), 0);
        QTest::addRow("enum-member-1") << fileItem
                                       << Token(QQmlJS::SourceLocation(241, 4, 13, 37),
                                                int(SemanticTokenProtocolTypes::EnumMember), 0);
    }
    { // objects and inline components
        const auto filePath = m_highlightingDataDir + "/objectAndComponent.qml";
        const auto fileItem = fileObject(filePath);

        // object
        QTest::addRow("object-identifier")
                << fileItem
                << Token(QQmlJS::SourceLocation(169, 4, 8, 5),
                         int(SemanticTokenProtocolTypes::Type), 0);
        QTest::addRow("object-id-property")
                << fileItem
                << Token(QQmlJS::SourceLocation(184, 2, 9, 9),
                         int(SemanticTokenProtocolTypes::Property), 0);
        QTest::addRow("object-id-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(188, 5, 9, 13),
                         int(SemanticTokenProtocolTypes::Variable), 0);

        // component
        QTest::addRow("component-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(139, 9, 7, 5),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("component-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(149, 6, 7, 15),
                         int(SemanticTokenProtocolTypes::Type), 0);
    }
    { // property definition
        const auto filePath = m_highlightingDataDir + "/properties.qml";
        const auto fileItem = fileObject(filePath);

        int definitionModifier = 1 << int(SemanticTokenModifiers::Definition);
        QTest::addRow("property-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(154, 8, 8, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("property-type")
                << fileItem
                << Token(QQmlJS::SourceLocation(163, 3, 8, 18),
                         int(SemanticTokenProtocolTypes::Type), 0);
        QTest::addRow("property-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(167, 1, 8, 22),
                         int(SemanticTokenProtocolTypes::Property),
                         definitionModifier);
        int readOnlyModifier = definitionModifier | (1 << int(SemanticTokenModifiers::Readonly));
        QTest::addRow("readonly-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(177, 8, 9, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("readonly-modifier")
                << fileItem
                << Token(QQmlJS::SourceLocation(199, 2, 9, 31),
                         int(SemanticTokenProtocolTypes::Property),
                         readOnlyModifier);
        int requiredModifier = definitionModifier | (1 << int(SemanticTokenModifiers::Abstract));
        QTest::addRow("required-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(210, 8, 10, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("required-modifier")
                << fileItem
                << Token(QQmlJS::SourceLocation(232, 3, 10, 31),
                         int(SemanticTokenProtocolTypes::Property),
                         requiredModifier);
        int defaultModifier =
                definitionModifier | (1 << int(SemanticTokenModifiers::DefaultLibrary));
        QTest::addRow("default-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(244, 7, 11, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("default-modifier")
                << fileItem
                << Token(QQmlJS::SourceLocation(265, 4, 11, 30),
                         int(SemanticTokenProtocolTypes::Property), defaultModifier);
    }
    {
        // methods and signals, lambda functions
        const auto filePath = m_highlightingDataDir + "/methodAndSignal.qml";
        const auto fileItem = fileObject(filePath);

        QTest::addRow("signal-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(139, 6, 7, 5),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("signal-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(146, 1, 7, 12),
                         int(SemanticTokenProtocolTypes::Method), 0);
        QTest::addRow("signal-type")
                << fileItem
                << Token(QQmlJS::SourceLocation(163, 3, 8, 14),
                         int(SemanticTokenProtocolTypes::Type), 0);
        QTest::addRow("signal-type-2")
                << fileItem
                << Token(QQmlJS::SourceLocation(186, 3, 9, 17),
                         int(SemanticTokenProtocolTypes::Type), 0);
        QTest::addRow("function-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(195, 8, 10, 5),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("function-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(204, 1, 10, 14),
                         int(SemanticTokenProtocolTypes::Method), 0);
        QTest::addRow("function-prm-type")
                << fileItem
                << Token(QQmlJS::SourceLocation(209, 3, 10, 19),
                         int(SemanticTokenProtocolTypes::Type), 0);
        QTest::addRow("function-prm-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(206, 1, 10, 16),
                         int(SemanticTokenProtocolTypes::Parameter), 0);
        QTest::addRow("function-rtn-type")
                << fileItem
                << Token(QQmlJS::SourceLocation(216, 3, 10, 26),
                         int(SemanticTokenProtocolTypes::Type), 0);
        // lambda function keywords
        QTest::addRow("function-keyword-rhs")
                << fileItem
                << Token(QQmlJS::SourceLocation(344, 8, 16, 24),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("function-keyword-rhs-1")
                << fileItem
                << Token(QQmlJS::SourceLocation(441, 8, 19, 20),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("function-keyword-in-function-body")
                << fileItem
                << Token(QQmlJS::SourceLocation(536, 8, 21, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("nested-function-identifier")
                << fileItem
                << Token(QQmlJS::SourceLocation(545, 6, 21, 18),
                         int(SemanticTokenProtocolTypes::Method), 0);
        QTest::addRow("lambda-undefined-arg")
                << fileItem
                << Token(QQmlJS::SourceLocation(409, 1, 17, 33),
                         int(SemanticTokenProtocolTypes::Variable), 0);
        QTest::addRow("yield-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(697, 5, 25, 50),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
    }
    { // literals
        const auto filePath = m_highlightingDataDir + "/literals.qml";
        const auto fileItem = fileObject(filePath);

        QTest::addRow("number") << fileItem
                                << Token(QQmlJS::SourceLocation(155, 3, 7, 21),
                                         int(SemanticTokenProtocolTypes::Number), 0);
        QTest::addRow("singleline-string")
                << fileItem
                << Token(QQmlJS::SourceLocation(182, 8, 8, 24),
                         int(SemanticTokenProtocolTypes::String), 0);
        QTest::addRow("multiline-string-first")
                << fileItem
                << Token(QQmlJS::SourceLocation(214, 6, 9, 24),
                         int(SemanticTokenProtocolTypes::String), 0);
        QTest::addRow("multiline-string-second")
                << fileItem
                << Token(QQmlJS::SourceLocation(221, 16, 10, 1),
                         int(SemanticTokenProtocolTypes::String), 0);
        QTest::addRow("multiline-with-newlines-l1")
                << fileItem
                << Token(QQmlJS::SourceLocation(313, 10, 13, 24),
                         int(SemanticTokenProtocolTypes::String), 0);
        QTest::addRow("multiline-with-newlines-l2")
                << fileItem
                << Token(QQmlJS::SourceLocation(324, 16, 14, 1),
                         int(SemanticTokenProtocolTypes::String), 0);
        QTest::addRow("boolean") << fileItem
                                 << Token(QQmlJS::SourceLocation(260, 4, 11, 22),
                                          int(SemanticTokenProtocolTypes::Keyword),
                                          0);
        QTest::addRow("null") << fileItem
                              << Token(QQmlJS::SourceLocation(285, 4, 12, 21),
                                       int(SemanticTokenProtocolTypes::Keyword), 0);
    }
    { // identifiers
        const auto filePath = m_highlightingDataDir + "/Identifiers.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("js-property")
                << fileItem
                << Token(QQmlJS::SourceLocation(222, 3, 10, 13),
                         int(SemanticTokenProtocolTypes::Variable), 0);
        QTest::addRow("property-id")
                << fileItem
                << Token(QQmlJS::SourceLocation(302, 4, 12, 19),
                         int(SemanticTokenProtocolTypes::Property),
                         (1 << int(SemanticTokenModifiers::Readonly)));
        QTest::addRow("property-changed")
                << fileItem
                << Token(QQmlJS::SourceLocation(451, 11, 18, 9),
                         int(SemanticTokenProtocolTypes::Method), 0);
        QTest::addRow("signal") << fileItem
                                << Token(QQmlJS::SourceLocation(474, 7, 19, 9),
                                         int(SemanticTokenProtocolTypes::Method), 0);

        QTest::addRow("attached-id")
                << fileItem
                << Token(QQmlJS::SourceLocation(512, 4, 23, 5),
                         int(SemanticTokenProtocolTypes::Type), 0);
        QTest::addRow("attached-signalhandler")
                << fileItem
                << Token(QQmlJS::SourceLocation(517, 9, 23, 10),
                         int(SemanticTokenProtocolTypes::Method), 0);
        QTest::addRow("propchanged-handler")
                << fileItem
                << Token(QQmlJS::SourceLocation(572, 13, 27, 5),
                         int(SemanticTokenProtocolTypes::Method), 0);
        QTest::addRow("method-id")
                << fileItem
                << Token(QQmlJS::SourceLocation(597, 1, 28, 9),
                         int(SemanticTokenProtocolTypes::Method), 0);
        QTest::addRow("signal-handler")
                << fileItem
                << Token(QQmlJS::SourceLocation(656, 9, 32, 5),
                         int(SemanticTokenProtocolTypes::Method), 0);

        QTest::addRow("enum-name-usage") << fileItem
                                         << Token(QQmlJS::SourceLocation(790, 1, 36, 35),
                                                  int(SemanticTokenProtocolTypes::Enum), 0);
        QTest::addRow("enum-member-usage") << fileItem
                                           << Token(QQmlJS::SourceLocation(792, 4, 36, 37),
                                                    int(SemanticTokenProtocolTypes::EnumMember), 0);
    }
    { // script expressions
        const auto filePath = m_highlightingDataDir + "/scriptExpressions.qml";
        const auto fileItem = fileObject(filePath);

        QTest::addRow("var-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(192, 3, 11, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("const-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(217, 5, 12, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        const auto modifier = (1 << int(SemanticTokenModifiers::Readonly));
        QTest::addRow("const-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(223, 10, 12, 15),
                         int(SemanticTokenProtocolTypes::Variable), modifier);
        QTest::addRow("do-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(248, 2, 13, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("if-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(287, 2, 15, 13),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("continue-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(319, 8, 16, 17),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("else-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(341, 4, 17, 13),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("while-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(382, 5, 19, 11),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("switch-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(418, 6, 20, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("case-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(444, 4, 21, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("return-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(464, 6, 22, 13),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("default-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(483, 7, 23, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("break-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(504, 5, 24, 13),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("try-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(529, 3, 26, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("catch-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(560, 5, 28, 11),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("finally-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(601, 7, 30, 11),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("for-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(620, 3, 31, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("throw-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(661, 5, 32, 13),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("for-declaration")
                << fileItem
                << Token(QQmlJS::SourceLocation(625, 5, 31, 14),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("destructuring")
                << fileItem
                << Token(QQmlJS::SourceLocation(1511, 2, 73, 16),
                         int(SemanticTokenProtocolTypes::Variable),
                         (1 << int(SemanticTokenModifiers::Readonly)));
        QTest::addRow("obj-destructuring")
                << fileItem
                << Token(QQmlJS::SourceLocation(1589, 2, 76, 17),
                         int(SemanticTokenProtocolTypes::Variable),
                         (1 << int(SemanticTokenModifiers::Readonly)));
        QTest::addRow("this-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(2661, 4, 115, 19),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("super-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(2677, 5, 116, 9),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
        QTest::addRow("new-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(2718, 3, 118, 16),
                         int(SemanticTokenProtocolTypes::Keyword), 0);
    }
    { // namespaced items
        const auto filePath = m_highlightingDataDir + "/namespace.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("namespace") << fileItem
                                   << Token(QQmlJS::SourceLocation(134, 3, 5, 1),
                                   int(SemanticTokenProtocolTypes::Namespace), 0);
        QTest::addRow("type") << fileItem
                                   << Token(QQmlJS::SourceLocation(138, 4, 5, 5),
                                   int(SemanticTokenProtocolTypes::Type), 0);
    }
    { // miscellaneous
        const auto filePath = m_highlightingDataDir + "/misc.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("typeModifiers") << fileItem
                                   << Token(QQmlJS::SourceLocation(147, 4, 6, 14),
                                   int(SemanticTokenProtocolTypes::Decorator), 0);
        QTest::addRow("globalVar") << fileItem
                                   << Token(QQmlJS::SourceLocation(234, 4, 9, 19),
                                            int(SemanticTokenProtocolTypes::Variable), 0);
        QTest::addRow("globalMethod") << fileItem
                                      << Token(QQmlJS::SourceLocation(239, 3, 9, 24),
                                               int(SemanticTokenProtocolTypes::Method), 0);
        QTest::addRow("globalMethodNewMember") << fileItem
                                               << Token(QQmlJS::SourceLocation(267, 4, 10, 23),
                                                        int(SemanticTokenProtocolTypes::Method), 0);
        QTest::addRow("globalMethodCallExpr") << fileItem
                                              << Token(QQmlJS::SourceLocation(310, 4, 11, 36),
                                                       int(SemanticTokenProtocolTypes::Method), 0);
        QTest::addRow("globalVarCallExpr") << fileItem
                                           << Token(QQmlJS::SourceLocation(300, 9, 11, 26),
                                                    int(SemanticTokenProtocolTypes::Variable), 0);
        QTest::addRow("globalVarMath") << fileItem
                                       << Token(QQmlJS::SourceLocation(337, 4, 12, 20),
                                                int(SemanticTokenProtocolTypes::Variable), 0);
    }
}

void tst_qmlls_highlighting::highlights()
{
    using namespace QQmlJS::Dom;
    QFETCH(DomItem, fileItem);
    QFETCH(Token, expectedHighlightedToken);

    Highlights h;
    HighlightingVisitor hv(h, std::nullopt);

    fileItem.visitTree(QQmlJS::Dom::Path(), hv, VisitOption::Default, emptyChildrenVisitor,
                   emptyChildrenVisitor);

    const auto highlights = h.highlights();
    QVERIFY(highlights.contains(expectedHighlightedToken.offset));
    QCOMPARE(highlights.value(expectedHighlightedToken.offset), expectedHighlightedToken);
}

void tst_qmlls_highlighting::rangeOverlapsWithSourceLocation_data()
{
    QTest::addColumn<QQmlJS::SourceLocation>("sourceLocation");
    QTest::addColumn<HighlightsRange>("range");
    QTest::addColumn<bool>("overlaps");

    QTest::addRow("sl-inside-range")
            << QQmlJS::SourceLocation(5, 1, 1, 1) << HighlightsRange{ 0, 100 } << true;
    QTest::addRow("sl-exceeds-rightBoundRange")
            << QQmlJS::SourceLocation(5, 1000, 1, 1) << HighlightsRange{ 0, 100 } << true;
    QTest::addRow("sl-exceeds-leftRightBoundRange")
            << QQmlJS::SourceLocation(5, 1000, 1, 1) << HighlightsRange{ 8, 100 } << true;
    QTest::addRow("sl-exceeds-leftBoundRange")
            << QQmlJS::SourceLocation(5, 100, 1, 1) << HighlightsRange{ 8, 1000 } << true;
    QTest::addRow("no-overlaps") << QQmlJS::SourceLocation(5, 100, 1, 1)
                                 << HighlightsRange{ 8000, 100000 } << false;
}

void tst_qmlls_highlighting::rangeOverlapsWithSourceLocation()
{
    QFETCH(QQmlJS::SourceLocation, sourceLocation);
    QFETCH(HighlightsRange, range);
    QFETCH(bool, overlaps);
    QVERIFY(overlaps == HighlightingUtils::rangeOverlapsWithSourceLocation(sourceLocation, range));
}

void tst_qmlls_highlighting::updateResultID_data()
{
    QTest::addColumn<QByteArray>("currentId");
    QTest::addColumn<QByteArray>("expectedNextId");

    QTest::addRow("zero-to-one") << QByteArray("0") << QByteArray("1");
    QTest::addRow("nine-to-ten") << QByteArray("9") << QByteArray("10");
    QTest::addRow("nineteen-to-twenty") << QByteArray("19") << QByteArray("20");
    QTest::addRow("twodigit-to-threedigit") << QByteArray("99") << QByteArray("100");
}

void tst_qmlls_highlighting::updateResultID()
{
    QFETCH(QByteArray, currentId);
    QFETCH(QByteArray, expectedNextId);

    HighlightingUtils::updateResultID(currentId);
    QCOMPARE(currentId, expectedNextId);
}

void tst_qmlls_highlighting::computeDiff_data()
{
    QTest::addColumn<QList<int>>("oldData");
    QTest::addColumn<QList<int>>("newData");
    QTest::addColumn<QList<SemanticTokensEdit>>("expected");

    {
        QList<int> oldData { 2,5,3,0,3, 0,5,4,1,0, 3,2,7,2,0};
        QList<int> newData {  3,5,3,0,3, 0,5,4,1,0, 3,2,7,2,0};
        SemanticTokensEdit expected;
        expected.start = 0;
        expected.deleteCount = 1;
        expected.data = QList{3};
        QTest::addRow("simple") << oldData << newData << QList{expected};
    }
    {
        QList<int> oldData { 0, 0, 5, 5, 0};
        QList<int> newData { 3, 3, 3, 3, 3, 0, 0, 5, 5, 0};
        SemanticTokensEdit expected;
        expected.start = 0;
        expected.deleteCount = 0;
        expected.data = QList{3, 3, 3, 3, 3};
        QTest::addRow("prepend") << oldData << newData << QList{expected};
    }
    {
        QList<int> oldData { 3, 3, 3, 3, 3, 0, 0, 5, 5, 0};
        QList<int> newData { 0, 0, 5, 5, 0};
        SemanticTokensEdit expected;
        expected.start = 0;
        expected.deleteCount = 5;
        expected.data = {};
        QTest::addRow("remove-front") << oldData << newData << QList{expected};
    }
    {
        QList<int> oldData { 0, 0, 5, 5, 0};
        QList<int> newData {  0, 0, 5, 5, 0, 1, 0, 23, 5, 0};
        SemanticTokensEdit expected;
        expected.start = 5;
        expected.deleteCount = 0;
        expected.data = QList{1, 0, 23, 5, 0};
        QTest::addRow("append") << oldData << newData << QList{expected};
    }
    {
        QList<int> oldData { 0, 0, 5, 5, 0, 1, 0, 23, 5, 0};
        QList<int> newData { 0, 0, 5, 5, 0};
        SemanticTokensEdit expected;
        expected.start = 5;
        expected.deleteCount = 5;
        expected.data = {};
        QTest::addRow("remove-back") << oldData << newData << QList{expected};
    }
    {
        QList<int> oldData { 0, 0, 5, 5, 0, 1, 0, 23, 5, 0};
        QList<int> newData { 0, 0, 5, 5, 0, 3, 3, 3, 3, 3, 1, 0, 23, 5, 0};
        SemanticTokensEdit expected;
        expected.start = 5;
        expected.deleteCount = 0;
        expected.data = QList{3, 3, 3, 3, 3};
        QTest::addRow("insert-middle") << oldData << newData << QList{expected};
    }
    {
        QList<int> oldData { 0, 0, 5, 5, 0, 3, 3, 3, 3, 3, 1, 0, 23, 5, 0};
        QList<int> newData { 0, 0, 5, 5, 0, 1, 0, 23, 5, 0};
        SemanticTokensEdit expected;
        expected.start = 5;
        expected.deleteCount = 5;
        expected.data = {};
        QTest::addRow("remove-middle") << oldData << newData << QList{expected};
    }
}

void tst_qmlls_highlighting::computeDiff()
{
    QFETCH(QList<int>, oldData);
    QFETCH(QList<int>, newData);
    QFETCH(QList<SemanticTokensEdit>, expected);

    const auto edits = HighlightingUtils::computeDiff(oldData, newData);
    QCOMPARE(edits.size(), expected.size());

    qsizetype i = 0;
    for (const auto &edit : edits) {
        QCOMPARE(edit.start, expected.at(i).start);
        QCOMPARE(edit.deleteCount, expected.at(i).deleteCount);
        QCOMPARE(edit.data, expected.at(i).data);
        ++i;
    }
}


QTEST_MAIN(tst_qmlls_highlighting)
