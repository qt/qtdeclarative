// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtCore/qplugin.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(lexLog, "qt.qml.lex");

QT_USE_NAMESPACE
using namespace Qt::StringLiterals;
using namespace QQmlJS;

class TestLineByLineLex : public QQmlDataTest
{
    Q_OBJECT

public:
    TestLineByLineLex();

private Q_SLOTS:
    void initTestCase() override;

    void testLineByLineLex_data();
    void testLineByLineLex();

    void testFormatter_data();
    void testFormatter();

private:
    void runLex(const QString &fileToLex);

    QString m_qmljsrootgenPath;
    QString m_qmltyperegistrarPath;
    QString m_baseDir;
};

TestLineByLineLex::TestLineByLineLex()
    : QQmlDataTest(QT_QMLTEST_DATADIR), m_baseDir(QString::fromLocal8Bit(QT_QMLTEST_DATADIR))
{
}

void TestLineByLineLex::initTestCase()
{
    QQmlDataTest::initTestCase();
}

void TestLineByLineLex::testLineByLineLex_data()
{
    QTest::addColumn<QString>("filename");
    QTest::newRow("Simple_QML") << QStringLiteral("Simple.qml");
    QTest::newRow("QML_importing_JS") << QStringLiteral("importing_js.qml");
    QTest::newRow("JS_with_pragma_and_import") << QStringLiteral("QTBUG-45916.js");
}

void TestLineByLineLex::testLineByLineLex()
{
    QFETCH(QString, filename);

    QString filePath = m_baseDir + u"/linebylinelex/data/"_s + filename;
    runLex(filePath);
}

void TestLineByLineLex::testFormatter_data()
{
    QTest::addColumn<QString>("filename");
    QDir formatData(m_baseDir + u"/qmlformat/data"_s);
    bool hasTestData = false; // ### TODO: fix test to always have data
    for (const QFileInfo &fInfo :
         formatData.entryInfoList(QStringList({ u"*.qml"_s, u"*.js"_s }), QDir::Files)) {
        QTest::newRow(qPrintable(fInfo.fileName())) << fInfo.absoluteFilePath();
        hasTestData = true;
    }
    if (!hasTestData)
        QSKIP("No test data found!");
}

void TestLineByLineLex::testFormatter()
{
    QFETCH(QString, filename);

    runLex(filename);
}

void TestLineByLineLex::runLex(const QString &fileToLex)
{
    QFile f(fileToLex);
    QVERIFY2(f.open(QFile::ReadOnly), qPrintable(fileToLex));
    QString contents = QString::fromUtf8(f.readAll());
    bool isQml = fileToLex.endsWith(u".qml"_s);
    QQmlJS::Lexer lexer(nullptr);
    lexer.setCode(contents, 1, isQml, QQmlJS::Lexer::CodeContinuation::Reset);
    f.seek(0);
    QString line = QString::fromUtf8(f.readLine());
    QQmlJS::Lexer llLexer(nullptr, QQmlJS::Lexer::LexMode::LineByLine);
    QQmlJS::Lexer::State oldState = llLexer.state();
    llLexer.setCode(line, 1, isQml, QQmlJS::Lexer::CodeContinuation::Reset);
    int iLine = 0;
    qCDebug(lexLog) << "pre lex" << lexer;
    int tokenKind = lexer.lex();
    qCDebug(lexLog) << tokenKind << "post lex" << lexer;
    QList<int> extraTokens({ QQmlJSGrammar::T_COMMENT, QQmlJSGrammar::T_PARTIAL_COMMENT,
                             QQmlJSGrammar::T_PARTIAL_DOUBLE_QUOTE_STRING_LITERAL,
                             QQmlJSGrammar::T_PARTIAL_SINGLE_QUOTE_STRING_LITERAL,
                             QQmlJSGrammar::T_PARTIAL_TEMPLATE_HEAD,
                             QQmlJSGrammar::T_PARTIAL_TEMPLATE_MIDDLE });
    while (!line.isEmpty()) {
        QQmlJS::Lexer llLexer2(nullptr, QQmlJS::Lexer::LexMode::LineByLine);
        llLexer2.setState(oldState);
        ++iLine;
        llLexer2.setCode(line, iLine, isQml,
                         ((iLine == 1) ? QQmlJS::Lexer::CodeContinuation::Reset
                                       : QQmlJS::Lexer::CodeContinuation::Continue));
        qCDebug(lexLog) << "line:" << iLine << line;
        qCDebug(lexLog) << "llpre lex" << llLexer;
        int llTokenKind = llLexer.lex();
        qCDebug(lexLog) << llTokenKind << "llpost lex" << llLexer;
        qCDebug(lexLog) << "ll2pre lex" << llLexer2;
        int ll2TokenKind = llLexer2.lex();
        qCDebug(lexLog) << ll2TokenKind << "ll2post lex" << llLexer2;
        qCDebug(lexLog) << "token" << llTokenKind << ll2TokenKind;
        QCOMPARE(llTokenKind, ll2TokenKind);
        QCOMPARE(llLexer.state(), llLexer2.state());
        while (llTokenKind != QQmlJSGrammar::T_EOL) {
            if (!extraTokens.contains(llTokenKind)) {
                qCDebug(lexLog) << "comparing with global lexer" << llTokenKind << tokenKind;
                QCOMPARE(llTokenKind, tokenKind);
                QCOMPARE(llLexer.state(), lexer.state());
                qCDebug(lexLog) << "pre lex" << lexer;
                tokenKind = lexer.lex();
                qCDebug(lexLog) << tokenKind << "post lex" << lexer;
            }
            qCDebug(lexLog) << "llpre lex" << llLexer;
            llTokenKind = llLexer.lex();
            qCDebug(lexLog) << llTokenKind << "llpost lex" << llLexer;
            qCDebug(lexLog) << "ll2pre lex" << llLexer2;
            ll2TokenKind = llLexer2.lex();
            qCDebug(lexLog) << ll2TokenKind << "ll2post lex" << llLexer2;
            QCOMPARE(llTokenKind, ll2TokenKind);
            QCOMPARE(llLexer.state(), llLexer2.state());
        }
        oldState = llLexer.state();
        line = QString::fromUtf8(f.readLine());
        llLexer.setCode(line, -1, isQml, QQmlJS::Lexer::CodeContinuation::Continue);
    }
    QCOMPARE(llLexer.lex(), QQmlJSGrammar::EOF_SYMBOL);
    if (tokenKind != QQmlJSGrammar::EOF_SYMBOL)
        tokenKind = lexer.lex();
    QCOMPARE(tokenKind, QQmlJSGrammar::EOF_SYMBOL);
}

QTEST_MAIN(TestLineByLineLex)
#include "tst_linebylinelex.moc"
