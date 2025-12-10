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
using namespace QmlHighlighting;

tst_qmlls_highlighting::tst_qmlls_highlighting()
    : QQmlDataTest(QT_QMLLS_HIGHLIGHTS_DATADIR) , m_highlightingDataDir(QT_QMLLS_HIGHLIGHTS_DATADIR + "/highlights"_L1)
{
}

// Token encoding as in:
// https://microsoft.github.io/language-server-protocol/specifications/specification-3-16/#textDocument_semanticTokens
void tst_qmlls_highlighting::encodeSemanticTokens_data()
{
    QTest::addColumn<HighlightsContainer>("highlights");
    QTest::addColumn<QList<int>>("expectedMemoryLayout");

    // The magic numbers below are used for semantic token encoding:
    // Each token is represented by 5 integers:
    // [deltaLine, deltaStartChar, length, tokenType, tokenModifiers]
    // For example:
    // - tokenType 25 means QmlHighlightKind::Unknown is map
    // - tokenType 1 means QmlHighlightKind::QmlType
    // - tokenModifiers 2 means QmlHighlightModifier::QmlPropertyDefinition
    // These values are mapped according to the enums in QmlHighlightKind and QmlHighlightModifier and using
    //  the mapper function mapToProtocolDefault(...)
    {
        HighlightsContainer c;
        HighlightToken t(QQmlJS::SourceLocation(0, 0, 1, 1), QmlHighlightKind::Unknown, QmlHighlightModifier::None);
        c.insert(t.loc.offset, t);
        QTest::addRow("empty-token-single") << c << QList {0, 0, 0, 25, 0};
    }
    {
        HighlightsContainer c;
        HighlightToken t(QQmlJS::SourceLocation(0, 1, 1, 1), QmlHighlightKind::Unknown, QmlHighlightModifier::None);
        c.insert(t.loc.offset, t);
        QTest::addRow("single-token") << c << QList {0, 0, 1, 25, 0};
    }
    {
        HighlightsContainer c;
        HighlightToken t1(QQmlJS::SourceLocation(0, 1, 1, 1), QmlHighlightKind::Unknown, QmlHighlightModifier::None);
        HighlightToken t2(QQmlJS::SourceLocation(1, 1, 3, 3), QmlHighlightKind::Unknown, QmlHighlightModifier::None);
        c.insert(t1.loc.offset, t1);
        c.insert(t2.loc.offset, t2);
        QTest::addRow("different-lines") << c << QList {0, 0, 1, 25, 0, 2, 2, 1, 25, 0};
    }
    {
        HighlightsContainer c;
        HighlightToken t1;
        t1.loc = QQmlJS::SourceLocation(0, 1, 1, 1);
        t1.kind = QmlHighlightKind::Unknown;
        HighlightToken t2;
        t2.loc = QQmlJS::SourceLocation(1, 1, 1, 3);
        t2.kind = QmlHighlightKind::Unknown;
        c.insert(t1.loc.offset, t1);
        c.insert(t2.loc.offset, t2);
        QTest::addRow("same-line-different-column") << c << QList {0, 0, 1, 25, 0, 0, 2, 1, 25, 0};
    }
    {
        HighlightsContainer c;
        HighlightToken t1;
        t1.loc = QQmlJS::SourceLocation(0, 1, 1, 1);
        t1.kind = QmlHighlightKind::QmlType;
        c.insert(t1.loc.offset, t1);
        QTest::addRow("token-type") << c << QList {0, 0, 1, 1, 0};
    }
    {
        HighlightsContainer c;
        HighlightToken t1;
        t1.loc = QQmlJS::SourceLocation(0, 1, 1, 1);
        t1.kind = QmlHighlightKind::QmlType;
        t1.modifiers = QmlHighlightModifier::QmlPropertyDefinition;
        c.insert(t1.loc.offset, t1);
        QTest::addRow("token-modifier") << c << QList {0, 0, 1, 1, 2};
    }
}

void tst_qmlls_highlighting::encodeSemanticTokens()
{
    QFETCH(HighlightsContainer, highlights);
    QFETCH(QList<int>, expectedMemoryLayout);

    const auto encoded = Utils::encodeSemanticTokens(highlights);
    [&]() {
        QCOMPARE(encoded, expectedMemoryLayout);
    }();

    if (QTest::currentTestFailed()) {
        qDebug() << "Actual encoded tokens: " << encoded;
        qDebug() << "Expected encoded tokens: " << expectedMemoryLayout;
    }
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
            Utils::sourceLocationsFromMultiLineToken(source, literal->literalToken);

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
    using namespace Utils;
    QTest::addColumn<DomItem>("fileItem");
    QTest::addColumn<HighlightToken>("expectedHighlightedToken");

    const auto fileObject = [](const QString &filePath){
        QFile f(filePath);
        DomItem file;
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return file;
        QString code = f.readAll();

        QStringList dirs = {QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath)};
        auto envPtr = DomEnvironment::create(
                dirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded, Extended);
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
        QTest::addRow("single-line-1")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(0, 41, 1, 1),
                         QmlHighlightKind::Comment, QmlHighlightModifier::None);

        /* single line comment    */
        QTest::addRow("single-line-2")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(162, 28, 9, 1),
                         QmlHighlightKind::Comment, QmlHighlightModifier::None);

        // Multiline comments are split into multiple locations
        QTest::addRow("multiline-first-line")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(133, 2, 5, 1),
                         QmlHighlightKind::Comment, QmlHighlightModifier::None);
        QTest::addRow("multiline-second-line")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(136, 21, 6, 1),
                         QmlHighlightKind::Comment, QmlHighlightModifier::None);
        QTest::addRow("multiline-third-line")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(158, 2, 7, 1),
                         QmlHighlightKind::Comment, QmlHighlightModifier::None);

        // Comments Inside Js blocks
        QTest::addRow("inside-js")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(232, 5, 13, 9),
                         QmlHighlightKind::Comment, QmlHighlightModifier::None);
    }
    { // Imports
        const auto filePath = m_highlightingDataDir + "/imports.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("import-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(112, 6, 4, 1),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("module-uri")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(119, 7, 4, 8),
                         QmlHighlightKind::QmlImportId, QmlHighlightModifier::None);
        QTest::addRow("directory-uri")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(152, 3, 6, 8),
                         QmlHighlightKind::String, QmlHighlightModifier::None);
        QTest::addRow("as-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(156, 2, 6, 12),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("version-number")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(140, 4, 5, 14),
                         QmlHighlightKind::Number, QmlHighlightModifier::None);
        QTest::addRow("qualified-namespace")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(159, 6, 6, 15),
                         QmlHighlightKind::QmlNamespace, QmlHighlightModifier::None);
    }
    { // Bindings
        const auto filePath = m_highlightingDataDir + "/bindings.qml";
        const auto fileItem = fileObject(filePath);

        // normal binding
        QTest::addRow("normalBinding")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(189, 1, 11, 5),
                         QmlHighlightKind::QmlProperty, QmlHighlightModifier::None);
        // on binding
        QTest::addRow("on-binding")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(175, 5, 9, 17),
                         QmlHighlightKind::QmlProperty, QmlHighlightModifier::None);
        QTest::addRow("on-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(172, 2, 9, 14),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
    }
    { // Pragmas
        const auto filePath = m_highlightingDataDir + "/pragmas.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("pragma-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(112, 6, 4, 1),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("pragma-name")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(136, 25, 5, 8),
                         QmlHighlightKind::QmlPragmaName, QmlHighlightModifier::None);
        QTest::addRow("pragma-value")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(198, 4, 6, 27),
                         QmlHighlightKind::QmlPragmaValue, QmlHighlightModifier::None);
    }
    { // Enums
        const auto filePath = m_highlightingDataDir + "/enums.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("enum-keyword") << fileItem
                                      << HighlightToken(QQmlJS::SourceLocation(158, 4, 8, 5),
                                               QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("enum-name") << fileItem
                                   << HighlightToken(QQmlJS::SourceLocation(163, 3, 8, 10),
                                            QmlHighlightKind::QmlEnumName, QmlHighlightModifier::None);
        QTest::addRow("enum-item") << fileItem
                                   << HighlightToken(QQmlJS::SourceLocation(177, 3, 9, 9),
                                            QmlHighlightKind::QmlEnumMember, QmlHighlightModifier::None);
        QTest::addRow("enum-value") << fileItem
                                    << HighlightToken(QQmlJS::SourceLocation(196, 1, 10, 15),
                                             QmlHighlightKind::Number, QmlHighlightModifier::None);
        QTest::addRow("namespace-enum") << fileItem
                                        << HighlightToken(QQmlJS::SourceLocation(225, 1, 13, 21),
                                                 QmlHighlightKind::QmlNamespace, QmlHighlightModifier::None);
        QTest::addRow("component-enum") << fileItem
                                        << HighlightToken(QQmlJS::SourceLocation(227, 11, 13, 23),
                                                 QmlHighlightKind::QmlType, QmlHighlightModifier::None);
        QTest::addRow("enum-name-1") << fileItem
                                     << HighlightToken(QQmlJS::SourceLocation(239, 1, 13, 35),
                                              QmlHighlightKind::QmlEnumName, QmlHighlightModifier::None);
        QTest::addRow("enum-member-1") << fileItem
                                       << HighlightToken(QQmlJS::SourceLocation(241, 4, 13, 37),
                                                QmlHighlightKind::QmlEnumMember, QmlHighlightModifier::None);
    }
    { // objects and inline components
        const auto filePath = m_highlightingDataDir + "/objectAndComponent.qml";
        const auto fileItem = fileObject(filePath);

        // object
        QTest::addRow("object-identifier")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(169, 4, 8, 5),
                         QmlHighlightKind::QmlType, QmlHighlightModifier::None);
        QTest::addRow("object-id-property")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(184, 2, 9, 9),
                         QmlHighlightKind::QmlProperty, QmlHighlightModifier::None);
        QTest::addRow("object-id-name")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(188, 5, 9, 13),
                         QmlHighlightKind::QmlLocalId, QmlHighlightModifier::None);

        // component
        QTest::addRow("component-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(139, 9, 7, 5),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("component-name")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(149, 6, 7, 15),
                         QmlHighlightKind::QmlType, QmlHighlightModifier::None);
    }
    { // property definition
        const auto filePath = m_highlightingDataDir + "/properties.qml";
        const auto fileItem = fileObject(filePath);

        QTest::addRow("property-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(154, 8, 8, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("property-type")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(163, 3, 8, 18),
                         QmlHighlightKind::QmlType, QmlHighlightModifier::None);
        QTest::addRow("property-name")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(167, 1, 8, 22),
                         QmlHighlightKind::QmlProperty,
                         QmlHighlightModifier::QmlPropertyDefinition);
        QTest::addRow("readonly-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(177, 8, 9, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("readonly-modifier")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(199, 2, 9, 31),
                         QmlHighlightKind::QmlProperty,
                         QmlHighlightModifier::QmlPropertyDefinition | QmlHighlightModifier::QmlReadonlyProperty);
        QTest::addRow("required-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(210, 8, 10, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("required-modifier")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(232, 3, 10, 31),
                         QmlHighlightKind::QmlProperty,
                         QmlHighlightModifier::QmlPropertyDefinition | QmlHighlightModifier::QmlRequiredProperty);
        QTest::addRow("default-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(244, 7, 11, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("default-modifier")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(265, 4, 11, 30),
                         QmlHighlightKind::QmlProperty,
                         QmlHighlightModifier::QmlPropertyDefinition | QmlHighlightModifier::QmlDefaultProperty);
        QTest::addRow("final-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(278, 5, 12, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("final-modifier")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(297, 5, 12, 28),
                         QmlHighlightKind::QmlProperty,
                         QmlHighlightModifier::QmlPropertyDefinition | QmlHighlightModifier::QmlFinalProperty);
        QTest::addRow("virtual-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(311, 7, 13, 9),
                                  QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("virtual-modifier")
                << fileItem
                << HighlightToken(
                           // Sloc of the "name" of the property
                           QQmlJS::SourceLocation(332, 1, 13, 30), QmlHighlightKind::QmlProperty,
                           QmlHighlightModifier::QmlPropertyDefinition
                                   | QmlHighlightModifier::QmlVirtualProperty);
        QTest::addRow("override-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(342, 8, 14, 9),
                                  QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("override-modifier")
                << fileItem
                << HighlightToken(
                           // Sloc of the "name" of the property
                           QQmlJS::SourceLocation(364, 1, 14, 31), QmlHighlightKind::QmlProperty,
                           QmlHighlightModifier::QmlPropertyDefinition
                                   | QmlHighlightModifier::QmlOverrideProperty);
    }
    {
        // methods and signals, lambda functions
        const auto filePath = m_highlightingDataDir + "/methodAndSignal.qml";
        const auto fileItem = fileObject(filePath);

        QTest::addRow("signal-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(139, 6, 7, 5),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("signal-name")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(146, 1, 7, 12),
                         QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
        QTest::addRow("signal-type")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(163, 3, 8, 14),
                         QmlHighlightKind::QmlType, QmlHighlightModifier::None);
        QTest::addRow("signal-type-2")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(186, 3, 9, 17),
                         QmlHighlightKind::QmlType, QmlHighlightModifier::None);
        QTest::addRow("function-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(195, 8, 10, 5),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("function-name")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(204, 1, 10, 14),
                         QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
        QTest::addRow("function-prm-type")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(209, 3, 10, 19),
                         QmlHighlightKind::QmlType, QmlHighlightModifier::None);
        QTest::addRow("function-prm-name")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(206, 1, 10, 16),
                         QmlHighlightKind::QmlMethodParameter, QmlHighlightModifier::None);
        QTest::addRow("function-rtn-type")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(216, 3, 10, 26),
                         QmlHighlightKind::QmlType, QmlHighlightModifier::None);
        // lambda function keywords
        QTest::addRow("function-keyword-rhs")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(344, 8, 16, 24),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("function-keyword-rhs-1")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(441, 8, 19, 20),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("function-keyword-in-function-body")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(536, 8, 21, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("nested-function-identifier")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(545, 6, 21, 18),
                         QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
        QTest::addRow("lambda-undefined-arg") << fileItem
                                              << HighlightToken(QQmlJS::SourceLocation(409, 1, 17, 33),
                                                       QmlHighlightKind::Unknown, QmlHighlightModifier::None);
        QTest::addRow("yield-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(697, 5, 25, 50),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
    }
    { // literals
        const auto filePath = m_highlightingDataDir + "/literals.qml";
        const auto fileItem = fileObject(filePath);

        QTest::addRow("number") << fileItem
                                << HighlightToken(QQmlJS::SourceLocation(155, 3, 7, 21),
                                         QmlHighlightKind::Number, QmlHighlightModifier::None);
        QTest::addRow("singleline-string")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(182, 8, 8, 24),
                         QmlHighlightKind::String, QmlHighlightModifier::None);
        QTest::addRow("multiline-string-first")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(214, 6, 9, 24),
                         QmlHighlightKind::String, QmlHighlightModifier::None);
        QTest::addRow("multiline-string-second")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(221, 16, 10, 1),
                         QmlHighlightKind::String, QmlHighlightModifier::None);
        QTest::addRow("multiline-with-newlines-l1")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(313, 10, 13, 24),
                         QmlHighlightKind::String, QmlHighlightModifier::None);
        QTest::addRow("multiline-with-newlines-l2")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(324, 16, 14, 1),
                         QmlHighlightKind::String, QmlHighlightModifier::None);
        QTest::addRow("boolean") << fileItem
                                 << HighlightToken(QQmlJS::SourceLocation(260, 4, 11, 22),
                                          QmlHighlightKind::QmlKeyword,
                                          QmlHighlightModifier::None);
        QTest::addRow("null") << fileItem
                              << HighlightToken(QQmlJS::SourceLocation(285, 4, 12, 21),
                                       QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("leftbacktick") << fileItem
                                      << HighlightToken(QQmlJS::SourceLocation(390, 1, 17, 43),
                                               QmlHighlightKind::String, QmlHighlightModifier::None);
        QTest::addRow("rightbacktick") << fileItem
                                       << HighlightToken(QQmlJS::SourceLocation(424, 1, 20, 5),
                                                QmlHighlightKind::String, QmlHighlightModifier::None);
        QTest::addRow("templatestringpartStart") << fileItem
                                            << HighlightToken(QQmlJS::SourceLocation(391, 5, 17, 44),
                                                     QmlHighlightKind::String, QmlHighlightModifier::None);
        QTest::addRow("templatestringpartEnd") << fileItem
                                            << HighlightToken(QQmlJS::SourceLocation(412, 7, 19, 1),
                                                     QmlHighlightKind::String, QmlHighlightModifier::None);
        QTest::addRow("templateExpressionPartB")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(403, 1, 18, 7),
                         QmlHighlightKind::QmlScopeObjectProperty, QmlHighlightModifier::None);
        QTest::addRow("templateExpressionPartK")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(407, 1, 18, 11),
                         QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
        QTest::addRow("dollarLeftBrace")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(401, 2, 18, 5),
                         QmlHighlightKind::Operator, QmlHighlightModifier::None);
        QTest::addRow("rightbrace")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(410, 1, 18, 14),
                         QmlHighlightKind::Operator, QmlHighlightModifier::None);
    }
    { // identifiers
        const auto filePath = m_highlightingDataDir + "/Identifiers.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("js-property")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(222, 3, 10, 13),
                         QmlHighlightKind::JsScopeVar, QmlHighlightModifier::None);
        QTest::addRow("property-id")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(302, 4, 12, 19),
                         QmlHighlightKind::QmlScopeObjectProperty,
                         QmlHighlightModifier::QmlReadonlyProperty);
        QTest::addRow("property-changed")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(451, 11, 18, 9),
                         QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
        QTest::addRow("signal") << fileItem
                                << HighlightToken(QQmlJS::SourceLocation(474, 7, 19, 9),
                                         QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);

        QTest::addRow("attached-id")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(512, 4, 23, 5),
                         QmlHighlightKind::QmlType, QmlHighlightModifier::None);
        QTest::addRow("attached-signalhandler")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(517, 9, 23, 10),
                         QmlHighlightKind::Field, QmlHighlightModifier::None);
        QTest::addRow("propchanged-handler") << fileItem
                                             << HighlightToken(QQmlJS::SourceLocation(572, 13, 27, 5),
                                                      QmlHighlightKind::QmlProperty, QmlHighlightModifier::None);
        QTest::addRow("method-id")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(597, 1, 28, 9),
                         QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
        QTest::addRow("signal-handler") << fileItem
                                        << HighlightToken(QQmlJS::SourceLocation(656, 9, 32, 5),
                                                 QmlHighlightKind::QmlProperty, QmlHighlightModifier::None);

        QTest::addRow("enum-name-usage") << fileItem
                                         << HighlightToken(QQmlJS::SourceLocation(790, 1, 36, 35),
                                                  QmlHighlightKind::QmlEnumName, QmlHighlightModifier::None);
        QTest::addRow("enum-member-usage") << fileItem
                                           << HighlightToken(QQmlJS::SourceLocation(792, 4, 36, 37),
                                                    QmlHighlightKind::QmlEnumMember, QmlHighlightModifier::None);
    }
    { // script expressions
        const auto filePath = m_highlightingDataDir + "/scriptExpressions.qml";
        const auto fileItem = fileObject(filePath);

        QTest::addRow("var-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(192, 3, 11, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("const-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(217, 5, 12, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("const-name")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(223, 10, 12, 15),
                         QmlHighlightKind::JsScopeVar, QmlHighlightModifier::QmlReadonlyProperty);
        QTest::addRow("do-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(248, 2, 13, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("if-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(287, 2, 15, 13),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("continue-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(319, 8, 16, 17),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("else-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(341, 4, 17, 13),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("while-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(382, 5, 19, 11),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("switch-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(418, 6, 20, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("case-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(444, 4, 21, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("return-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(464, 6, 22, 13),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("default-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(483, 7, 23, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("break-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(504, 5, 24, 13),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("try-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(529, 3, 26, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("catch-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(560, 5, 28, 11),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("finally-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(601, 7, 30, 11),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("for-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(620, 3, 31, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("throw-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(661, 5, 32, 13),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("for-declaration")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(625, 5, 31, 14),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("destructuring")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(1511, 2, 73, 16),
                         QmlHighlightKind::JsScopeVar,
                         QmlHighlightModifier::QmlReadonlyProperty);
        QTest::addRow("obj-destructuring")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(1589, 2, 76, 17),
                         QmlHighlightKind::JsScopeVar,
                         QmlHighlightModifier::QmlReadonlyProperty);
        QTest::addRow("this-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(2661, 4, 115, 19),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("super-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(2677, 5, 116, 9),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        QTest::addRow("new-keyword")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(2718, 3, 118, 16),
                         QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
    }
    { // namespaced items
        const auto filePath = m_highlightingDataDir + "/namespace.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("namespace") << fileItem
                                   << HighlightToken(QQmlJS::SourceLocation(134, 3, 5, 1),
                                   QmlHighlightKind::QmlNamespace, QmlHighlightModifier::None);
        QTest::addRow("type") << fileItem
                                   << HighlightToken(QQmlJS::SourceLocation(138, 4, 5, 5),
                                   QmlHighlightKind::QmlType, QmlHighlightModifier::None);
    }
    { // miscellaneous
        const auto filePath = m_highlightingDataDir + "/misc.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("typeModifiers") << fileItem
                                   << HighlightToken(QQmlJS::SourceLocation(147, 4, 6, 14),
                                   QmlHighlightKind::QmlTypeModifier, QmlHighlightModifier::None);
        QTest::addRow("globalVar") << fileItem
                                   << HighlightToken(QQmlJS::SourceLocation(234, 4, 9, 19),
                                            QmlHighlightKind::JsGlobalVar, QmlHighlightModifier::None);
        QTest::addRow("globalMethod") << fileItem
                                      << HighlightToken(QQmlJS::SourceLocation(239, 3, 9, 24),
                                               QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
        QTest::addRow("globalMethodNewMember") << fileItem
                                               << HighlightToken(QQmlJS::SourceLocation(267, 4, 10, 23),
                                                        QmlHighlightKind::JsGlobalMethod, QmlHighlightModifier::None);
        QTest::addRow("globalMethodCallExpr") << fileItem
                                              << HighlightToken(QQmlJS::SourceLocation(310, 4, 11, 36),
                                                       QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
        QTest::addRow("globalVarCallExpr") << fileItem
                                           << HighlightToken(QQmlJS::SourceLocation(300, 9, 11, 26),
                                                    QmlHighlightKind::Field, QmlHighlightModifier::None);
        QTest::addRow("globalVarMath") << fileItem
                                       << HighlightToken(QQmlJS::SourceLocation(337, 4, 12, 20),
                                                QmlHighlightKind::JsGlobalVar, QmlHighlightModifier::None);
    }
    { // property chains
        const auto filePath = m_highlightingDataDir + "/propertyChains.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("knownMemberOfInnerScope1")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(411, 3, 19, 13),
                         QmlHighlightKind::QmlScopeObjectProperty, QmlHighlightModifier::None);
        QTest::addRow("knownMemberOfInnerScope2")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(415, 3, 19, 17),
                         QmlHighlightKind::Field, QmlHighlightModifier::None);
        QTest::addRow("knownMemberOfInnerScope3")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(419, 3, 19, 21),
                         QmlHighlightKind::Field, QmlHighlightModifier::None);
        QTest::addRow("idInsideTheSameComponent1")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(688, 4, 33, 17),
                         QmlHighlightKind::QmlLocalId, QmlHighlightModifier::None);
        QTest::addRow("idInsideTheSameComponent2")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(693, 5, 33, 22),
                         QmlHighlightKind::Field, QmlHighlightModifier::None);
        QTest::addRow("idInsideTheSameComponent3")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(699, 8, 33, 28),
                         QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
        QTest::addRow("unknownMemberOfParentScope1")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(993, 12, 47, 33),
                         QmlHighlightKind::QmlExternalObjectProperty, QmlHighlightModifier::None);
        QTest::addRow("unresolvedLookup1") << fileItem
                                           << HighlightToken(QQmlJS::SourceLocation(1177, 3, 56, 13),
                                                    QmlHighlightKind::Unknown, QmlHighlightModifier::None);
        QTest::addRow("unresolvedLookup2") << fileItem
                                           << HighlightToken(QQmlJS::SourceLocation(1181, 3, 56, 17),
                                                    QmlHighlightKind::Field, QmlHighlightModifier::None);
        QTest::addRow("outerIDLookupInNestedComponent1")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(1480, 5, 69, 21),
                         QmlHighlightKind::QmlExternalId, QmlHighlightModifier::None);
        QTest::addRow("outerIDLookupInNestedComponent2")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(1486, 5, 69, 27),
                         QmlHighlightKind::Field, QmlHighlightModifier::None);
        QTest::addRow("outerIDLookupInNestedComponent3")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(1492, 7, 69, 33),
                         QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
        QTest::addRow("attachedPropertyChain1")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(1799, 7, 82, 17),
                         QmlHighlightKind::QmlLocalId, QmlHighlightModifier::None);
        QTest::addRow("attachedPropertyChain2")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(1807, 11, 82, 25),
                         QmlHighlightKind::Field, QmlHighlightModifier::None);
        QTest::addRow("attachedPropertyChain3")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(1819, 11, 82, 37),
                         QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
    }
    { // method in property chains
        const auto filePath = m_highlightingDataDir + "/methodInPropertyChains.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("chainedMethodName") << fileItem
                                           << HighlightToken(QQmlJS::SourceLocation(271, 11, 11, 60),
                                                    QmlHighlightKind::QmlMethod, QmlHighlightModifier::None);
    }
    { // known member of parent scope
        const auto filePath = m_highlightingDataDir + "/componentBound.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("outerIDWithComponentBound")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(296, 5, 15, 17),
                         QmlHighlightKind::QmlLocalId, QmlHighlightModifier::None);
    }
    { // known member of parent scope with no component bound
        const auto filePath = m_highlightingDataDir + "/componentNoBound.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("outerIDWithNoComponentBound")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(296, 5, 15, 17),
                         QmlHighlightKind::QmlExternalId, QmlHighlightModifier::None);
    }
    { // custom parsed property that looks like id
        const auto filePath = m_highlightingDataDir + "/idCrash.qml";
        const auto fileItem = fileObject(filePath);
        QTest::addRow("outerIDWithNoComponentBound")
                << fileItem
                << HighlightToken(QQmlJS::SourceLocation(121, 7, 9, 17), QmlHighlightKind::Unknown,
                                  QmlHighlightModifier::None);
    }
}

void tst_qmlls_highlighting::highlights()
{
    using namespace QQmlJS::Dom;
    QFETCH(DomItem, fileItem);
    QFETCH(HighlightToken, expectedHighlightedToken);

    const auto highlights = Utils::visitTokens(fileItem,std::nullopt);

    [&]() {
        QVERIFY(highlights.contains(expectedHighlightedToken.loc.offset));
        QCOMPARE(highlights.value(expectedHighlightedToken.loc.offset), expectedHighlightedToken);
    }();

    if (QTest::currentTestFailed()) {
        const auto& expected = expectedHighlightedToken;

        if (highlights.contains(expectedHighlightedToken.loc.offset)) {
            const auto& actual = highlights[expectedHighlightedToken.loc.offset];

            qDebug() << "Actual offset" << actual.loc.offset << "Expected offset" << expected.loc.offset;
            qDebug() << "Actual length" << actual.loc.length << "Expected length" << expected.loc.length;
            qDebug() << "Actual startLine" << actual.loc.startLine << "Expected startLine" << expected.loc.startLine;
            qDebug() << "Actual startColumn" << actual.loc.startColumn << "Expected startColumn" << expected.loc.startColumn;
            qDebug() << "Actual tokenType" << static_cast<int>(actual.kind) << "Expected tokenType" << static_cast<int>(expected.kind);
            qDebug() << "Actual tokenModifier" << static_cast<int>(actual.modifiers) << "Expected tokenModifier" << static_cast<int>(expected.modifiers);
        } else {
            qDebug() << "Expected token not found in highlights";
            const auto distance = [](const HighlightToken& actual, const HighlightToken& expected) {
                return qAbs(actual.loc.offset - expected.loc.offset);
            };
            double minDistance = std::numeric_limits<double>::max();
            const HighlightToken *closest = nullptr;

            for (const auto &item : highlights) {
                double dist = distance(item, expectedHighlightedToken);
                if (dist < minDistance) {
                    minDistance = dist;
                    closest = &item;
                }
            }

            if (closest) {
                qDebug() << "Closest token offset" << closest->loc.offset << "Expected offset" << expected.loc.offset;
                qDebug() << "Closest token length" << closest->loc.length << "Expected length" << expected.loc.length;
                qDebug() << "Closest token startLine" << closest->loc.startLine << "Expected startLine" << expected.loc.startLine;
                qDebug() << "Closest token startColumn" << closest->loc.startColumn << "Expected startColumn" << expected.loc.startColumn;
                qDebug() << "Closest token type" << static_cast<int>(closest->kind) << "Expected type" << static_cast<int>(expected.kind);
                qDebug() << "Closest token modifier" << static_cast<int>(closest->modifiers) << "Expected modifier" << static_cast<int>(expected.modifiers);
            }
        }
    }
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
    QVERIFY(overlaps == Utils::rangeOverlapsWithSourceLocation(sourceLocation, range));
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

    Utils::updateResultID(currentId);
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

    const auto edits = Utils::computeDiff(oldData, newData);
    QCOMPARE(edits.size(), expected.size());

    qsizetype i = 0;
    for (const auto &edit : edits) {
        QCOMPARE(edit.start, expected.at(i).start);
        QCOMPARE(edit.deleteCount, expected.at(i).deleteCount);
        QCOMPARE(edit.data, expected.at(i).data);
        ++i;
    }
}

static QQmlJS::Dom::DomItem fileObject(const QString &filePath)
{
    using namespace QQmlJS::Dom;
    QFile f(filePath);
    DomItem file;
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return file;
    QString code = f.readAll();

    QStringList dirs = { QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath) };
    auto envPtr = DomEnvironment::create(dirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded,
                                         Extended);
    envPtr->loadBuiltins();
    envPtr->loadFile(
            FileToLoad::fromMemory(envPtr, filePath, code),
            [&file](Path, const DomItem &, const DomItem &newIt) { file = newIt.fileObject(); });
    envPtr->loadPendingDependencies();
    return file;
};

void tst_qmlls_highlighting::crashes_data()
{
    QTest::addColumn<QString>("fileName");

    QTest::addRow("enums") << u"enums_qtbug.qml"_s;
    QTest::addRow("id") << u"idCrash.qml"_s;
}

void tst_qmlls_highlighting::crashes()
{
    QFETCH(QString, fileName);
    const auto filePath = m_highlightingDataDir + "/"_L1 + fileName;
    const auto fileItem = fileObject(filePath);

    HighlightingVisitor hv(fileItem, std::nullopt);
    const auto highlights = hv.highlights();
    QVERIFY(!highlights.isEmpty());
}

void tst_qmlls_highlighting::shiftHighlights_data()
{
    QTest::addColumn<QmlHighlighting::HighlightsContainer>("lastValidHighlights");
    QTest::addColumn<QString>("lastValidCode");
    QTest::addColumn<QString>("currentCode");
    QTest::addColumn<QmlHighlighting::HighlightsContainer>("expectedHighlights");

    const auto filePath = m_highlightingDataDir + "/highlightsShift.qml";
    const auto fileItem = fileObject(filePath);
    const auto originalHighlights = QmlHighlighting::Utils::visitTokens(fileItem, std::nullopt);
    const QString lastValidCode = fileItem.ownerAs<QQmlJS::Dom::QmlFile>()->code();

    const auto editedCode = [&](const QString &file) -> QString {
        const auto editedPath = m_highlightingDataDir + file;
        QFile editedFile(editedPath);
        if (!editedFile.open(QIODevice::ReadOnly | QIODevice::Text))
            return QString();
        return editedFile.readAll();
    };

    // delete port Qt from import QtQuick
    // Expect highlights are the same as the originals except Item token.
    {
        const QString currentCode = editedCode("/invalid/highlightsShift_del1.qml");

        QmlHighlighting::HighlightsContainer expectedHighlights = originalHighlights;
        expectedHighlights.remove(0);
        expectedHighlights.remove(7);
        const auto im = QQmlJS::SourceLocation(0, 2, 1, 1); // 'im' location
        expectedHighlights[im.offset] =
                HighlightToken(im, QmlHighlightKind::QmlKeyword, QmlHighlightModifier::None);
        const auto quick = QQmlJS::SourceLocation(3, 5, 1, 3); // 'Quick' location
        expectedHighlights[quick.offset] =
                HighlightToken(quick, QmlHighlightKind::QmlImportId, QmlHighlightModifier::None);

        QTest::addRow("modify-line-without-lineshift-from-lhs-rhs")
                << originalHighlights << lastValidCode << currentCode << expectedHighlights;
    }
    // modify Item { into Ite
    // Expect highlights are the same as the originals except Item token.
    {
        const QString currentCode = editedCode("/invalid/highlightsShift_del2.qml");

        QmlHighlighting::HighlightsContainer expectedHighlights = originalHighlights;
        const auto itemNewLoc = QQmlJS::SourceLocation(15, 3, 2, 1); // 'Ite' location
        expectedHighlights[itemNewLoc.offset] =
                HighlightToken(itemNewLoc, QmlHighlightKind::QmlType, QmlHighlightModifier::None);

        QTest::addRow("modify-line-without-lineshift-from-rhs")
                << originalHighlights << lastValidCode << currentCode << expectedHighlights;
    }
    // delete "import"  from import QtQuick
    // Expect highlights are the same as the originals QtQuick token should shift.
    // and import should be gone.
    {
        const QString currentCode = editedCode("/invalid/highlightsShift_del3.qml");

        QmlHighlighting::HighlightsContainer expectedHighlights = originalHighlights;
        // remove import QtQuick
        expectedHighlights.remove(0);
        expectedHighlights.remove(7);
        // insert QtQuick
        const auto qtquickNewLoc = QQmlJS::SourceLocation(1, 7, 1, 2);
        expectedHighlights[qtquickNewLoc.offset] = HighlightToken(
                qtquickNewLoc, QmlHighlightKind::QmlImportId, QmlHighlightModifier::None);

        QTest::addRow("modify-line-without-lineshift-from-lhs")
                << originalHighlights << lastValidCode << currentCode << expectedHighlights;
    }

    // delete inner content of Rectangle, leave it in invalid state
    {
        const QString currentCode = editedCode("/invalid/highlightsShift_del4.qml");

        QmlHighlighting::HighlightsContainer expectedHighlights = originalHighlights;
        // remove Rectangle content tokens
        expectedHighlights.remove(61); // width
        expectedHighlights.remove(68); // 100
        expectedHighlights.remove(80); // height
        expectedHighlights.remove(88); // 100

        QTest::addRow("delete-multiline-no-shift")
                << originalHighlights << lastValidCode << currentCode << expectedHighlights;
    }
    // delete width content of Rectangle, height should line shift up
    {
        const QString currentCode = editedCode("/invalid/highlightsShift_del5.qml");

        QmlHighlighting::HighlightsContainer expectedHighlights = originalHighlights;
        // remove Rectangle content tokens
        expectedHighlights.remove(61); // width
        expectedHighlights.remove(68); // 100

        // height should line shift up, and shifts column
        auto heightToken = expectedHighlights.take(80);
        expectedHighlights.remove(80);
        heightToken.loc.startLine = 5;
        heightToken.loc.startColumn = 1;
        heightToken.loc.offset = 51;
        expectedHighlights[heightToken.loc.offset] = heightToken;

        auto numberToken = expectedHighlights.take(88);
        expectedHighlights.remove(88);
        numberToken.loc.startLine = 5;
        numberToken.loc.startColumn = 9;
        numberToken.loc.offset = 59;
        expectedHighlights[numberToken.loc.offset] = numberToken;
        QTest::addRow("delete-multiline-line-column-shift")
                << originalHighlights << lastValidCode << currentCode << expectedHighlights;
    }
    // delete width content of Rectangle, height should line shift up, Rectangle should be erased
    // th: 100 part of width should line up and start at the end of upper line
    {
        const QString currentCode = editedCode("/invalid/highlightsShift_del6.qml");

        QmlHighlighting::HighlightsContainer expectedHighlights = originalHighlights;
        // remove Rectangle content tokens
        expectedHighlights.remove(41); // Rectangle

        // width
        auto widthToken = expectedHighlights.take(61);
        expectedHighlights.remove(61);
        widthToken.loc.startLine = 3;
        widthToken.loc.startColumn = 15;
        widthToken.loc.offset = 36;
        widthToken.loc.length = 2; // th part remained
        expectedHighlights[widthToken.loc.offset] = widthToken;
        // 100
        auto numberToken = expectedHighlights.take(68);
        expectedHighlights.remove(68);
        numberToken.loc.startLine = 3;
        numberToken.loc.startColumn = 19;
        numberToken.loc.offset = 40;
        expectedHighlights[numberToken.loc.offset] = numberToken;
        // height should line shift up
        auto heightToken = expectedHighlights.take(80);
        expectedHighlights.remove(80);
        heightToken.loc.startLine = 4;
        heightToken.loc.startColumn = 9;
        heightToken.loc.offset = 52;
        expectedHighlights[heightToken.loc.offset] = heightToken;

        auto heightNumber = expectedHighlights.take(88);
        expectedHighlights.remove(88);
        heightNumber.loc.startLine = 4;
        heightNumber.loc.startColumn = 17;
        heightNumber.loc.offset = 60;
        expectedHighlights[heightNumber.loc.offset] = heightNumber;

        QTest::addRow("delete-multiline-line-column-shift-2")
                << originalHighlights << lastValidCode << currentCode << expectedHighlights;
    }
    { // manual testing find: console.log(element)
        const auto filePath = m_highlightingDataDir + "/highlightsShift_consoleLog.qml";
        const auto fileItem = fileObject(filePath);
        const auto highlights = QmlHighlighting::Utils::visitTokens(fileItem, std::nullopt);
        const QString validCode = fileItem.ownerAs<QQmlJS::Dom::QmlFile>()->code();
        const QString currentCode = editedCode("/invalid/highlightsShift_consoleLog_del.qml");

        QmlHighlighting::HighlightsContainer expectedHighlights = highlights;
        // console
        auto consoleToken = expectedHighlights.take(108);
        expectedHighlights.remove(108);
        consoleToken.loc.offset = 107;
        expectedHighlights[consoleToken.loc.offset] = consoleToken;
        // log
        auto logToken = expectedHighlights.take(116);
        expectedHighlights.remove(116);
        logToken.loc.offset = 115;
        expectedHighlights[logToken.loc.offset] = logToken;
        // enumValue
        auto enumValueToken = expectedHighlights.take(120);
        expectedHighlights.remove(120);
        enumValueToken.loc.offset = 119;
        expectedHighlights[enumValueToken.loc.offset] = enumValueToken;

        QTest::addRow("delete-shift-console-log")
                << highlights << validCode << currentCode << expectedHighlights;
    }
    { // manual testing find: comments after deletion
        const auto filePath = m_highlightingDataDir + "/highlightsShift_withComments.qml";
        const auto fileItem = fileObject(filePath);
        const auto highlights = QmlHighlighting::Utils::visitTokens(fileItem, std::nullopt);
        const QString validCode = fileItem.ownerAs<QQmlJS::Dom::QmlFile>()->code();
        const QString currentCode = editedCode("/invalid/highlightsShift_withComments_del.qml");

        QmlHighlighting::HighlightsContainer expectedHighlights;
        // import
        expectedHighlights[0] = highlights[0];
        // QtQuick
        expectedHighlights[7] = highlights[7];
        // commentline 1
        auto commentLine1 = QmlHighlighting::HighlightToken(
                QQmlJS::SourceLocation(17, 25, 4, 1),
                QmlHighlightKind::Comment, QmlHighlightModifier::None);
        expectedHighlights[commentLine1.loc.offset] = commentLine1;
        // commentline 2
        auto commentLine2 = QmlHighlighting::HighlightToken(
                QQmlJS::SourceLocation(43, 23, 5, 1),
                QmlHighlightKind::Comment, QmlHighlightModifier::None);
        expectedHighlights[commentLine2.loc.offset] = commentLine2;

        QTest::addRow("delete-shift-comments-after-deletion")
                << highlights << validCode << currentCode << expectedHighlights;
    }
}

void tst_qmlls_highlighting::shiftHighlights()
{
    QFETCH(QmlHighlighting::HighlightsContainer, lastValidHighlights);
    QFETCH(QString, lastValidCode);
    QFETCH(QString, currentCode);
    QFETCH(QmlHighlighting::HighlightsContainer, expectedHighlights);

    HighlightsContainer actualHighlights =
            QmlHighlighting::Utils::shiftHighlights(lastValidHighlights, lastValidCode, currentCode);
    [&] {
        const auto actualEncoded = QmlHighlighting::Utils::encodeSemanticTokens(
                actualHighlights, QmlHighlighting::HighlightingMode::Default);
        const auto expectedEncoded = QmlHighlighting::Utils::encodeSemanticTokens(
                expectedHighlights, QmlHighlighting::HighlightingMode::Default);
        QCOMPARE(actualEncoded, expectedEncoded);
    }();

    if (QTest::currentTestFailed()) {
        auto [actual, expected] =
                std::mismatch(actualHighlights.begin(), actualHighlights.end(),
                              expectedHighlights.begin(), expectedHighlights.end());

        if (actual != actualHighlights.end() && expected != expectedHighlights.end()) {

            const auto msg = [](const QString &title, int actualValue, int expectedValue) {
                return QString("%1 : [Actual %2, Expected %3]")
                        .arg(title)
                        .arg(actualValue)
                        .arg(expectedValue);
            };
            qDebug() << msg("Offset", actual->loc.offset, expected->loc.offset);
            qDebug() << msg("Length", actual->loc.length, expected->loc.length);
            qDebug() << msg("StartLine", actual->loc.startLine, expected->loc.startLine);
            qDebug() << msg("StartColumn", actual->loc.startColumn, expected->loc.startColumn);
            qDebug() << msg("Kind", static_cast<int>(actual->kind),
                            static_cast<int>(expected->kind));
            qDebug() << msg("Modifiers", static_cast<int>(actual->modifiers),
                            static_cast<int>(expected->modifiers));
        }
    }
}

QTEST_MAIN(tst_qmlls_highlighting)
