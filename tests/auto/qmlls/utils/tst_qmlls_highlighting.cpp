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
        auto envPtr = DomEnvironment::create(dirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies, options);
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
                << Token(QQmlJS::SourceLocation(0, 41, 1, 1), int(SemanticTokenTypes::Comment), 0);

        /* single line comment    */
        QTest::addRow("single-line-2") << fileItem
                                       << Token(QQmlJS::SourceLocation(162, 28, 9, 1),
                                                int(SemanticTokenTypes::Comment), 0);

        // Multiline comments are split into multiple locations
        QTest::addRow("multiline-first-line")
                << fileItem
                << Token(QQmlJS::SourceLocation(133, 2, 5, 1), int(SemanticTokenTypes::Comment), 0);
        QTest::addRow("multiline-second-line") << fileItem
                                               << Token(QQmlJS::SourceLocation(136, 21, 6, 1),
                                                        int(SemanticTokenTypes::Comment), 0);
        QTest::addRow("multiline-third-line")
                << fileItem
                << Token(QQmlJS::SourceLocation(158, 2, 7, 1), int(SemanticTokenTypes::Comment), 0);

        // Comments Inside Js blocks
        QTest::addRow("inside-js") << fileItem
                                   << Token(QQmlJS::SourceLocation(232, 5, 13, 9),
                                            int(SemanticTokenTypes::Comment), 0);
    }
    { // Imports
        const auto filePath = m_highlightingDataDir + "/imports.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("import-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(112, 6, 4, 1), int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("module-uri") << fileItem
                                    << Token(QQmlJS::SourceLocation(119, 7, 4, 8),
                                             int(SemanticTokenTypes::Namespace), 0);
        QTest::addRow("directory-uri")
                << fileItem
                << Token(QQmlJS::SourceLocation(152, 3, 6, 8), int(SemanticTokenTypes::String), 0);
        QTest::addRow("as-keyword") << fileItem
                                    << Token(QQmlJS::SourceLocation(156, 2, 6, 12),
                                             int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("version-number")
                << fileItem
                << Token(QQmlJS::SourceLocation(140, 4, 5, 14), int(SemanticTokenTypes::Number), 0);
        QTest::addRow("qualified-namespace") << fileItem
                                             << Token(QQmlJS::SourceLocation(159, 6, 6, 15),
                                                      int(SemanticTokenTypes::Namespace), 0);
    }
    { // Bindings
        const auto filePath = m_highlightingDataDir + "/bindings.qml";
        const auto fileItem = fileObject(filePath);

        // normal binding
        QTest::addRow("normalBinding") << fileItem
                                       << Token(QQmlJS::SourceLocation(189, 1, 11, 5),
                                                int(SemanticTokenTypes::Property), 0);
        // on binding
        QTest::addRow("on-binding") << fileItem
                                    << Token(QQmlJS::SourceLocation(175, 5, 9, 17),
                                             int(SemanticTokenTypes::Property), 0);
        QTest::addRow("on-keyword") << fileItem
                                    << Token(QQmlJS::SourceLocation(172, 2, 9, 14),
                                             int(SemanticTokenTypes::Keyword), 0);
    }
    { // Pragmas
        const auto filePath = m_highlightingDataDir + "/pragmas.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("pragma-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(112, 6, 4, 1), int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("pragma-name") << fileItem
                                     << Token(QQmlJS::SourceLocation(136, 25, 5, 8),
                                              int(SemanticTokenTypes::Variable), 0);
        QTest::addRow("pragma-value") << fileItem
                                      << Token(QQmlJS::SourceLocation(198, 4, 6, 27),
                                               int(SemanticTokenTypes::Variable), 0);
    }
    { // Enums
        const auto filePath = m_highlightingDataDir + "/enums.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("enum-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(141, 4, 7, 5), int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("enum-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(146, 3, 7, 10), int(SemanticTokenTypes::Enum), 0);
        QTest::addRow("enum-item") << fileItem
                                   << Token(QQmlJS::SourceLocation(160, 3, 8, 9),
                                            int(SemanticTokenTypes::EnumMember), 0);
        QTest::addRow("enum-value")
                << fileItem
                << Token(QQmlJS::SourceLocation(179, 1, 9, 15), int(SemanticTokenTypes::Number), 0);
    }
    { // objects and inline components
        const auto filePath = m_highlightingDataDir + "/objectAndComponent.qml";
        const auto fileItem = fileObject(filePath);

        // object
        QTest::addRow("object-identifier")
                << fileItem
                << Token(QQmlJS::SourceLocation(169, 4, 8, 5), int(SemanticTokenTypes::Type), 0);
        QTest::addRow("object-id-property") << fileItem
                                            << Token(QQmlJS::SourceLocation(184, 2, 9, 9),
                                                     int(SemanticTokenTypes::Property), 0);
        QTest::addRow("object-id-name") << fileItem
                                        << Token(QQmlJS::SourceLocation(188, 5, 9, 13),
                                                 int(SemanticTokenTypes::Variable), 0);

        // component
        QTest::addRow("component-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(139, 9, 7, 5), int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("component-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(149, 6, 7, 15), int(SemanticTokenTypes::Type), 0);
    }
    { // property definition
        const auto filePath = m_highlightingDataDir + "/properties.qml";
        const auto fileItem = fileObject(filePath);

        int definitionModifier = 1 << int(SemanticTokenModifiers::Definition);
        QTest::addRow("property-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(154, 8, 8, 9), int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("property-type")
                << fileItem
                << Token(QQmlJS::SourceLocation(163, 3, 8, 18), int(SemanticTokenTypes::Type), 0);
        QTest::addRow("property-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(167, 1, 8, 22), int(SemanticTokenTypes::Property),
                         definitionModifier);
        int readOnlyModifier = definitionModifier | (1 << int(SemanticTokenModifiers::Readonly));
        QTest::addRow("readonly-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(177, 8, 9, 9), int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("readonly-modifier")
                << fileItem
                << Token(QQmlJS::SourceLocation(199, 2, 9, 31), int(SemanticTokenTypes::Property),
                         readOnlyModifier);
        int requiredModifier = definitionModifier | (1 << int(SemanticTokenModifiers::Abstract));
        QTest::addRow("required-keyword") << fileItem
                                          << Token(QQmlJS::SourceLocation(210, 8, 10, 9),
                                                   int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("required-modifier")
                << fileItem
                << Token(QQmlJS::SourceLocation(232, 3, 10, 31), int(SemanticTokenTypes::Property),
                         requiredModifier);
        int defaultModifier =
                definitionModifier | (1 << int(SemanticTokenModifiers::DefaultLibrary));
        QTest::addRow("default-keyword") << fileItem
                                         << Token(QQmlJS::SourceLocation(244, 7, 11, 9),
                                                  int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("default-modifier")
                << fileItem
                << Token(QQmlJS::SourceLocation(265, 4, 11, 30), int(SemanticTokenTypes::Property),
                         defaultModifier);
    }
    {
        // methods and signals
        const auto filePath = m_highlightingDataDir + "/methodAndSignal.qml";
        const auto fileItem = fileObject(filePath);

        QTest::addRow("signal-keyword")
                << fileItem
                << Token(QQmlJS::SourceLocation(139, 6, 7, 5), int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("signal-name")
                << fileItem
                << Token(QQmlJS::SourceLocation(146, 1, 7, 12), int(SemanticTokenTypes::Method), 0);
        QTest::addRow("signal-type")
                << fileItem
                << Token(QQmlJS::SourceLocation(163, 3, 8, 14), int(SemanticTokenTypes::Type), 0);
        QTest::addRow("signal-type-2")
                << fileItem
                << Token(QQmlJS::SourceLocation(186, 3, 9, 17), int(SemanticTokenTypes::Type), 0);
        QTest::addRow("function-keyword") << fileItem
                                          << Token(QQmlJS::SourceLocation(195, 9, 10, 5),
                                                   int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("function-name") << fileItem
                                       << Token(QQmlJS::SourceLocation(204, 1, 10, 14),
                                                int(SemanticTokenTypes::Method), 0);
        QTest::addRow("function-prm-type")
                << fileItem
                << Token(QQmlJS::SourceLocation(209, 3, 10, 19), int(SemanticTokenTypes::Type), 0);
        QTest::addRow("function-prm-name") << fileItem
                                           << Token(QQmlJS::SourceLocation(206, 1, 10, 16),
                                                    int(SemanticTokenTypes::Parameter), 0);
        QTest::addRow("function-rtn-type")
                << fileItem
                << Token(QQmlJS::SourceLocation(216, 3, 10, 26), int(SemanticTokenTypes::Type), 0);
    }
    { // literals
        const auto filePath = m_highlightingDataDir + "/literals.qml";
        const auto fileItem = fileObject(filePath);

        QTest::addRow("number") << fileItem
                                << Token(QQmlJS::SourceLocation(155, 3, 7, 21),
                                         int(SemanticTokenTypes::Number), 0);
        QTest::addRow("singleline-string")
                << fileItem
                << Token(QQmlJS::SourceLocation(182, 8, 8, 24), int(SemanticTokenTypes::String), 0);
        QTest::addRow("multiline-string-first")
                << fileItem
                << Token(QQmlJS::SourceLocation(214, 6, 9, 24), int(SemanticTokenTypes::String), 0);
        QTest::addRow("multiline-string-second") << fileItem
                                                 << Token(QQmlJS::SourceLocation(221, 16, 10, 1),
                                                          int(SemanticTokenTypes::String), 0);
        QTest::addRow("boolean") << fileItem
                                 << Token(QQmlJS::SourceLocation(260, 4, 11, 22),
                                          int(SemanticTokenTypes::Keyword), 0);
        QTest::addRow("null") << fileItem
                              << Token(QQmlJS::SourceLocation(285, 4, 12, 21),
                                       int(SemanticTokenTypes::Keyword), 0);
    }
    { // identifiers
        const auto filePath = m_highlightingDataDir + "/Identifiers.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("js-property") << fileItem
                                     << Token(QQmlJS::SourceLocation(222, 3, 10, 13),
                                              int(SemanticTokenTypes::Variable), 0);
        QTest::addRow("property-id")
                << fileItem
                << Token(QQmlJS::SourceLocation(302, 4, 12, 19), int(SemanticTokenTypes::Property),
                         (1 << int(SemanticTokenModifiers::Readonly)));
        QTest::addRow("property-changed") << fileItem
                                          << Token(QQmlJS::SourceLocation(451, 11, 18, 9),
                                                   int(SemanticTokenTypes::Method), 0);
        QTest::addRow("signal") << fileItem
                                << Token(QQmlJS::SourceLocation(474, 7, 19, 9),
                                         int(SemanticTokenTypes::Method), 0);

        QTest::addRow("attached-id")
                << fileItem
                << Token(QQmlJS::SourceLocation(512, 4, 23, 5), int(SemanticTokenTypes::Type), 0);
        QTest::addRow("attached-signalhandler") << fileItem
                                                << Token(QQmlJS::SourceLocation(517, 9, 23, 10),
                                                         int(SemanticTokenTypes::Method), 0);
        QTest::addRow("propchanged-handler") << fileItem
                                             << Token(QQmlJS::SourceLocation(572, 13, 27, 5),
                                                      int(SemanticTokenTypes::Method), 0);
        QTest::addRow("method-id")
                << fileItem
                << Token(QQmlJS::SourceLocation(597, 1, 28, 9), int(SemanticTokenTypes::Method), 0);
        QTest::addRow("signal-handler")
                << fileItem
                << Token(QQmlJS::SourceLocation(656, 9, 32, 5), int(SemanticTokenTypes::Method), 0);
    }
}

void tst_qmlls_highlighting::highlights()
{
    using namespace QQmlJS::Dom;
    QFETCH(DomItem, fileItem);
    QFETCH(Token, expectedHighlightedToken);

    Highlights h;
    HighlightingVisitor hv(h);

    fileItem.visitTree(QQmlJS::Dom::Path(), hv, VisitOption::Default, emptyChildrenVisitor,
                   emptyChildrenVisitor);

    const auto highlights = h.highlights();
    QVERIFY(highlights.contains(expectedHighlightedToken.offset));
    QCOMPARE(highlights.value(expectedHighlightedToken.offset), expectedHighlightedToken);
}

QTEST_MAIN(tst_qmlls_highlighting)
