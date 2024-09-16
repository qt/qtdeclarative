// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qqmljsengine_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsastvisitor_p.h>
#include <private/qqmljsast_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQmlDom/private/qqmldomastdumper_p.h>

#include <qtest.h>
#include <QDir>
#include <QDebug>
#include <QRegularExpression>
#include <cstdlib>

class tst_qqmlparser : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmlparser();

private slots:
    void initTestCase() override;
#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
    void qmlParser_data();
    void qmlParser();
#endif
    void invalidEscapeSequence();
    void stringLiteral();
    void codeLocationsWithContinuationStringLiteral();
    void codeLocationsWithContinuationStringLiteral_data();
    void noSubstitutionTemplateLiteral();
    void templateLiteral();
    void numericSeparator_data();
    void numericSeparator();
    void invalidNumericSeparator_data();
    void invalidNumericSeparator();
    void leadingSemicolonInClass();
    void templatedReadonlyProperty();
    void qmlImportInJS();
    void typeAnnotations_data();
    void typeAnnotations();
    void disallowedTypeAnnotations_data();
    void disallowedTypeAnnotations();
    void semicolonPartOfExpressionStatement();
    void typeAssertion_data();
    void typeAssertion();
    void annotations_data();
    void annotations();
    void invalidImportVersion_data();
    void invalidImportVersion();

private:
    QStringList excludedDirs;

    QStringList findFiles(const QDir &);
};

namespace check {

using namespace QQmlJS;

class Check: public AST::Visitor
{
    QList<AST::Node *> nodeStack;

public:
    void operator()(AST::Node *node)
    {
        AST::Node::accept(node, this);
    }

    virtual void checkNode(AST::Node *node)
    {
        if (! nodeStack.isEmpty()) {
            AST::Node *parent = nodeStack.last();
            const qsizetype parentBegin = parent->firstSourceLocation().begin();
            const qsizetype parentEnd = parent->lastSourceLocation().end();

            if (node->firstSourceLocation().begin() < parentBegin)
                qDebug() << "first source loc failed: node:" << node->kind << "at" << node->firstSourceLocation().startLine << "/" << node->firstSourceLocation().startColumn
                         << "parent" << parent->kind << "at" << parent->firstSourceLocation().startLine << "/" << parent->firstSourceLocation().startColumn;
            if (node->lastSourceLocation().end() > parentEnd)
                qDebug() << "last source loc failed: node:" << node->kind << "at" << node->lastSourceLocation().startLine << "/" << node->lastSourceLocation().startColumn
                         << "parent" << parent->kind << "at" << parent->lastSourceLocation().startLine << "/" << parent->lastSourceLocation().startColumn;

            QVERIFY(node->firstSourceLocation().begin() >= parentBegin);
            QVERIFY(node->lastSourceLocation().end() <= parentEnd);
        }
    }

    bool preVisit(AST::Node *node) override
    {
        checkNode(node);
        nodeStack.append(node);
        return true;
    }

    void postVisit(AST::Node *) override
    {
        nodeStack.removeLast();
    }

    void throwRecursionDepthError() final
    {
        QFAIL("Maximum statement or expression depth exceeded");
    }
};

struct TypeAnnotationObserver: public AST::Visitor
{
    bool typeAnnotationSeen = false;

    void operator()(AST::Node *node)
    {
        AST::Node::accept(node, this);
    }

    bool visit(AST::TypeAnnotation *) override
    {
        typeAnnotationSeen = true;
        return true;
    }

    void throwRecursionDepthError() final
    {
        QFAIL("Maximum statement or expression depth exceeded");
    }
};

struct ExpressionStatementObserver: public AST::Visitor
{
    int expressionsSeen = 0;
    bool endsWithSemicolon = true;

    void operator()(AST::Node *node)
    {
        AST::Node::accept(node, this);
    }

    bool visit(AST::ExpressionStatement *statement) override
    {
        ++expressionsSeen;
        endsWithSemicolon = endsWithSemicolon
                && (statement->lastSourceLocation().end() == statement->semicolonToken.end());
        return true;
    }

    void throwRecursionDepthError() final
    {
        QFAIL("Maximum statement or expression depth exceeded");
    }
};

class CheckLocations : public Check
{
public:
    CheckLocations(const QString &code)
    {
        m_codeStr = code;
        m_code = code.split('\u000A');
    }

    void checkNode(AST::Node *node) override
    {
        SourceLocation first = node->firstSourceLocation();
        SourceLocation last = node->lastSourceLocation();
        int startLine = first.startLine - 1;
        int endLine = last.startLine - 1;
        QVERIFY(startLine >= 0 && startLine < m_code.size());
        QVERIFY(endLine >= 0 && endLine < m_code.size());
        const int length = last.offset + last.length - first.offset;
        QString expected = m_code.join('\n').mid(first.offset, length);
        int startColumn = first.startColumn - 1;
        QString found;
        while (startLine < endLine) {
            found.append(m_code.at(startLine).mid(startColumn)).append('\n');
            ++startLine;
            startColumn = 0;
        }
        found.append(m_code.at(endLine).mid(startColumn,
                                            last.startColumn + last.length - startColumn - 1));
        ++startLine;
        // handle possible continuation strings correctly
        while (found.size() != length && startLine < m_code.size()) {
            const QString line = m_code.at(startLine);
            found.append('\n');
            if (length - found.size() > line.size())
                found.append(line);
            else
                found.append(line.left(length - found.size()));
            ++startLine;
        }
        QCOMPARE(expected, found);
        SourceLocation combined(first.offset, quint32(last.end() - first.begin()),
                                first.startLine, first.startColumn);
        SourceLocation cStart = combined.startZeroLengthLocation();
        SourceLocation cEnd = combined.endZeroLengthLocation(m_codeStr);
        QCOMPARE(cStart.begin(), first.begin());
        QCOMPARE(cStart.end(), first.begin());
        QCOMPARE(cStart.startLine, first.startLine);
        QCOMPARE(cStart.startColumn, first.startColumn);
        QCOMPARE(cEnd.begin(), last.end());
        QCOMPARE(cEnd.end(), last.end());
        QCOMPARE(cEnd.startLine, uint(startLine));
        int lastNewline = found.lastIndexOf(QLatin1Char('\n'));
        if (lastNewline < 0)
            lastNewline = -int(combined.startColumn);
        QCOMPARE(cEnd.startColumn, found.size() - lastNewline);
    }
private:
    QString m_codeStr;
    QStringList m_code;
};

}

tst_qqmlparser::tst_qqmlparser()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qqmlparser::initTestCase()
{
    QQmlDataTest::initTestCase();
    // Add directories you want excluded here

    // These snippets are not expected to run on their own.
    excludedDirs << "doc/src/snippets/qml/visualdatamodel_rootindex";
    excludedDirs << "doc/src/snippets/qml/qtbinding";
    excludedDirs << "doc/src/snippets/qml/imports";
    excludedDirs << "doc/src/snippets/qtquick1/visualdatamodel_rootindex";
    excludedDirs << "doc/src/snippets/qtquick1/qtbinding";
    excludedDirs << "doc/src/snippets/qtquick1/imports";
}

QStringList tst_qqmlparser::findFiles(const QDir &d)
{
    for (int ii = 0; ii < excludedDirs.size(); ++ii) {
        QString s = excludedDirs.at(ii);
        if (d.absolutePath().endsWith(s))
            return QStringList();
    }

    QStringList rv;

    const QStringList files = d.entryList(
            QStringList() << QLatin1String("*.qml") << QLatin1String("*.js"), QDir::Files);
    for (const QString &file : files)
        rv << d.absoluteFilePath(file);

    const QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    for (const QString &dir : dirs) {
        QDir sub = d;
        sub.cd(dir);
        rv << findFiles(sub);
    }

    return rv;
}

/*
This test checks all the qml and js files in the QtQml UI source tree
and ensures that the subnode's source locations are inside parent node's source locations
*/

#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
void tst_qqmlparser::qmlParser_data()
{
    QTest::addColumn<QString>("file");

    QString examples = QLatin1String(SRCDIR) + "/../../../../examples/";
    QString tests = QLatin1String(SRCDIR) + "/../../../../tests/";

    QStringList files;
    files << findFiles(QDir(examples));
    files << findFiles(QDir(tests));

    for (const QString &file : std::as_const(files))
        QTest::newRow(qPrintable(file)) << file;
}
#endif

#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
void tst_qqmlparser::qmlParser()
{
    QFETCH(QString, file);

    using namespace QQmlJS;

    QString code;

    QFile f(file);
    if (f.open(QFile::ReadOnly))
        code = QString::fromUtf8(f.readAll());

    const bool qmlMode = file.endsWith(QLatin1String(".qml"));

    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(code, 1, qmlMode);
    Parser parser(&engine);
    bool ok = qmlMode ? parser.parse() : parser.parseProgram();

    if (ok) {
        check::Check chk;
        chk(parser.rootNode());
    }
}
#endif

void tst_qqmlparser::invalidEscapeSequence()
{
    using namespace QQmlJS;

    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(QLatin1String("\"\\"), 1);
    Parser parser(&engine);
    parser.parse();
}

void tst_qqmlparser::stringLiteral()
{
    using namespace QQmlJS;

    Engine engine;
    Lexer lexer(&engine);
    QString code("'hello string'");
    lexer.setCode(code , 1);
    Parser parser(&engine);
    QVERIFY(parser.parseExpression());
    AST::ExpressionNode *expression = parser.expression();
    QVERIFY(expression);
    auto *literal = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(expression);
    QVERIFY(literal);
    QCOMPARE(literal->value, u"hello string");
    QCOMPARE(literal->firstSourceLocation().begin(), 0);
    QCOMPARE(literal->lastSourceLocation().end(), quint32(code.size()));

    // test for correct handling escape sequences inside strings
    QLatin1String leftCode("'hello\\n\\tstring'");
    QLatin1String plusCode(" + ");
    QLatin1String rightCode("'\\nbye'");
    code = leftCode + plusCode + rightCode;
    lexer.setCode(code , 1);
    QVERIFY(parser.parseExpression());

    expression = parser.expression();
    QVERIFY(expression);
    auto *binaryExpression = QQmlJS::AST::cast<QQmlJS::AST::BinaryExpression *>(expression);
    QVERIFY(binaryExpression);

    literal = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(binaryExpression->left);
    QVERIFY(literal);
    QCOMPARE(literal->value, u"hello\n\tstring");
    QCOMPARE(literal->firstSourceLocation().begin(), 0);
    QCOMPARE(literal->firstSourceLocation().startLine, 1u);
    QCOMPARE(literal->lastSourceLocation().end(), quint32(leftCode.size()));

    QVERIFY(binaryExpression->right);
    literal = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(binaryExpression->right);
    QVERIFY(literal);
    QCOMPARE(literal->value, u"\nbye");
    qsizetype offset = leftCode.size() + plusCode.size();
    QCOMPARE(literal->firstSourceLocation().begin(), offset);
    QCOMPARE(literal->firstSourceLocation().startLine, 1u);
    QCOMPARE(literal->lastSourceLocation().end(), quint32(code.size()));

    leftCode = QLatin1String("'\u000Ahello\u000Abye'");
    code = leftCode + plusCode + rightCode;
    lexer.setCode(code, 1);
    QVERIFY(parser.parseExpression());
    expression = parser.expression();
    QVERIFY(expression);

    binaryExpression = QQmlJS::AST::cast<QQmlJS::AST::BinaryExpression *>(expression);
    QVERIFY(binaryExpression);

    literal = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(binaryExpression->left);
    QVERIFY(literal);
    QCOMPARE(literal->value, u"\nhello\nbye");
    QCOMPARE(literal->firstSourceLocation().begin(), 0);
    QCOMPARE(literal->firstSourceLocation().startLine, 1u);
    QCOMPARE(literal->lastSourceLocation().end(), leftCode.size());

    literal = QQmlJS::AST::cast<QQmlJS::AST::StringLiteral *>(binaryExpression->right);
    QVERIFY(literal);
    QCOMPARE(literal->value, u"\nbye");
    offset = leftCode.size() + plusCode.size();
    QCOMPARE(literal->firstSourceLocation().begin(), offset);
    QCOMPARE(literal->lastSourceLocation().startLine, 3u);
    QCOMPARE(literal->lastSourceLocation().end(), code.size());

}

void tst_qqmlparser::codeLocationsWithContinuationStringLiteral()
{
    using namespace QQmlJS;
    QFETCH(QString, code);
    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(code, 1);
    Parser parser(&engine);
    QVERIFY(parser.parse());

    check::CheckLocations chk(code);
    chk(parser.rootNode());
}

void tst_qqmlparser::codeLocationsWithContinuationStringLiteral_data()
{
    QTest::addColumn<QString>("code");
    QString code("A {\u000A"
                 "    property string dummy: \"this\u000A"
                 "                             may break lexer\"\u000A"
                 "    B { }\u000A"
                 "}");
    QTest::newRow("withTextBeforeLF") << code;
    code = QString("A {\u000A"
                   "    property string dummy: \"\u000A"
                   "                             may break lexer\"\u000A"
                   "    B { }\u000A"
                   "}");
    QTest::newRow("withoutTextBeforeLF") << code;
    code = QString("A {\u000A"
                   "    property string dummy: \"this\\\u000A"
                   "                             may break lexer\"\u000A"
                   "    B { }\u000A"
                   "}");
    QTest::newRow("withTextBeforeEscapedLF") << code;
    code = QString("A {\u000A"
                   "    property string dummy: \"th\\\"is\u000A"
                   "                             may break lexer\"\u000A"
                   "    B { }\u000A"
                   "}");
    QTest::newRow("withTextBeforeWithEscapeSequence") << code;
    code = QString("A {\u000A"
                   "    property string first: \"\u000A"
                   "                             first\"\u000A"
                   "    property string dummy: \"th\\\"is\u000A"
                   "                             may break lexer\"\u000A"
                   "    B { }\u000A"
                   "}");
    QTest::newRow("withTextBeforeLFwithEscapeSequenceCombined") << code;
    // reference data
    code = QString("A {\u000A"
                   "    B {\u000A"
                   "        property int dummy: 1\u000A"
                   "    }\u000A"
                   "    C {\u000A"
                   "        D { }\u000A"
                   "    }\u000A"
                   "}");
    QTest::newRow("noStringLiteralAtAll") << code;
}

void tst_qqmlparser::noSubstitutionTemplateLiteral()
{
    using namespace QQmlJS;

    Engine engine;
    Lexer lexer(&engine);
    QLatin1String code("`hello template`");
    lexer.setCode(code, 1);
    Parser parser(&engine);
    QVERIFY(parser.parseExpression());
    AST::ExpressionNode *expression = parser.expression();
    QVERIFY(expression);

    auto *literal = QQmlJS::AST::cast<QQmlJS::AST::TemplateLiteral *>(expression);
    QVERIFY(literal);

    QCOMPARE(literal->value, u"hello template");
    QCOMPARE(literal->firstSourceLocation().begin(), 0u);
    QCOMPARE(literal->lastSourceLocation().end(), quint32(code.size()));
}

void tst_qqmlparser::templateLiteral()
{
    using namespace QQmlJS;

    Engine engine;
    Lexer lexer(&engine);
    QLatin1String code("`one plus one equals ${1+1}!`");
    lexer.setCode(code, 1);
    Parser parser(&engine);
    QVERIFY(parser.parseExpression());
    AST::ExpressionNode *expression = parser.expression();
    QVERIFY(expression);

    auto *templateLiteral = QQmlJS::AST::cast<QQmlJS::AST::TemplateLiteral *>(expression);
    QVERIFY(templateLiteral);

    QCOMPARE(templateLiteral->firstSourceLocation().begin(), 0);
    auto *e = templateLiteral->expression;
    QVERIFY(e);
}

void tst_qqmlparser::numericSeparator_data() {
    QTest::addColumn<QString>("code");
    QTest::addColumn<double>("expected_value");

    QTest::newRow("Separator in decimal literal") << "1_000_000_000" << 1000000000.0;
    QTest::newRow("Separator in fractional part") << "1000.22_33" << 1000.2233;
    QTest::newRow("Separator in exponent part") << "1e1_0_0" << std::pow(10, 100);
    QTest::newRow("Separator in positive exponent part") << "1e+1_0_0" << 1e100;
    QTest::newRow("Separator in negative exponent part") << "1e-1_0_0" << 1e-100;
    QTest::newRow("Separator in binary literal with b prefix") << "0b1010_0001_1000_0101" << static_cast<double>(0b1010000110000101);
    QTest::newRow("Separator in binary literal with B prefix") << "0B01_10_01_10" << static_cast<double>(0b01100110);
    QTest::newRow("Separator in octal literal with o prefix") << "0o1234_5670" << static_cast<double>(012345670);
    QTest::newRow("Separator in octal literal with O prefix") << "0O7777_0000" << static_cast<double>(077770000);
    QTest::newRow("Separator in hex literal with x prefix") << "0xA0_B0_C0" << static_cast<double>(0xA0B0C0);
    QTest::newRow("Separator in hex literal with X prefix") << "0X1000_AAAA" << static_cast<double>(0x1000AAAA);
}

void tst_qqmlparser::numericSeparator() {
    using namespace QQmlJS;

    QFETCH(QString, code);
    QFETCH(double, expected_value);

    QQmlJS::Engine engine;

    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(code, 1);

    QQmlJS::Parser parser(&engine);
    QVERIFY(parser.parseExpression());

    AST::ExpressionNode *expression = parser.expression();
    QVERIFY(expression);

    auto *literal = QQmlJS::AST::cast<QQmlJS::AST::NumericLiteral *>(expression);
    QVERIFY(literal);

    QCOMPARE(literal->value, expected_value);
    QCOMPARE(literal->firstSourceLocation().begin(), 0);
    QCOMPARE(literal->lastSourceLocation().end(), quint32(code.size()));
}

void tst_qqmlparser::invalidNumericSeparator_data() {
    QTest::addColumn<QString>("code");
    QTest::addColumn<QString>("error");

    QTest::newRow("Trailing numeric separator") << "1_" << "A trailing numeric separator is not allowed in numeric literals";
    QTest::newRow("Multiple numeric separators") << "1__2" << "There can be at most one numeric separator between digits";
}

void tst_qqmlparser::invalidNumericSeparator() {
    using namespace QQmlJS;

    QFETCH(QString, code);
    QFETCH(QString, error);

    QQmlJS::Engine engine;

    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(code, 1);

    QQmlJS::Parser parser(&engine);
    QVERIFY(!parser.parseExpression());

    QVERIFY(lexer.errorCode() != Lexer::NoError);
    QCOMPARE(lexer.errorMessage(), error);
}

void tst_qqmlparser::leadingSemicolonInClass()
{
    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(QLatin1String("class X{;n(){}}"), 1);
    QQmlJS::Parser parser(&engine);
    QVERIFY(parser.parseProgram());
}

void tst_qqmlparser::templatedReadonlyProperty()
{
    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(QLatin1String("A { readonly property list<B> listfoo: [ C{} ] }"), 1);
    QQmlJS::Parser parser(&engine);
    QVERIFY(parser.parse());
}

void tst_qqmlparser::qmlImportInJS()
{
    {
        QQmlJS::Engine engine;
        QQmlJS::Lexer lexer(&engine);
        lexer.setCode(QLatin1String(".import Test 1.0 as T"), 0, false);
        QQmlJS::Parser parser(&engine);
        QVERIFY(parser.parseProgram());
    }
    {
        QQmlJS::Engine engine;
        QQmlJS::Lexer lexer(&engine);
        lexer.setCode(QLatin1String(".import Test 1 as T"), 0, false);
        QQmlJS::Parser parser(&engine);
        QVERIFY(parser.parseProgram());
    }
    {
        QQmlJS::Engine engine;
        QQmlJS::Lexer lexer(&engine);
        lexer.setCode(QLatin1String(".import Test as T"), 0, false);
        QQmlJS::Parser parser(&engine);
        QVERIFY(parser.parseProgram());
    }
}

void tst_qqmlparser::typeAnnotations_data()
{
    QTest::addColumn<QString>("file");

    QString tests = dataDirectory() + "/typeannotations/";

    QStringList files;
    files << findFiles(QDir(tests));

    for (const QString &file: std::as_const(files))
        QTest::newRow(qPrintable(file)) << file;
}

void tst_qqmlparser::typeAnnotations()
{
    using namespace QQmlJS;

    QFETCH(QString, file);

    QString code;

    QFile f(file);
    if (f.open(QFile::ReadOnly))
        code = QString::fromUtf8(f.readAll());

    const bool qmlMode = file.endsWith(QLatin1String(".qml"));

    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(code, 1, qmlMode);
    Parser parser(&engine);
    bool ok = qmlMode ? parser.parse() : parser.parseProgram();
    QVERIFY(ok);

    check::TypeAnnotationObserver observer;
    observer(parser.rootNode());

    QVERIFY(observer.typeAnnotationSeen);
}

void tst_qqmlparser::disallowedTypeAnnotations_data()
{
    QTest::addColumn<QString>("file");

    QString tests = dataDirectory() + "/disallowedtypeannotations/";

    QStringList files;
    files << findFiles(QDir(tests));

    for (const QString &file: std::as_const(files))
        QTest::newRow(qPrintable(file)) << file;
}

void tst_qqmlparser::disallowedTypeAnnotations()
{
    using namespace QQmlJS;

    QFETCH(QString, file);

    QString code;

    QFile f(file);
    if (f.open(QFile::ReadOnly))
        code = QString::fromUtf8(f.readAll());

    const bool qmlMode = file.endsWith(QLatin1String(".qml"));

    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(code, 1, qmlMode);
    Parser parser(&engine);
    bool ok = qmlMode ? parser.parse() : parser.parseProgram();
    QVERIFY(!ok);
    QVERIFY2(parser.errorMessage().startsWith("Type annotations are not permitted "), qPrintable(parser.errorMessage()));
}

void tst_qqmlparser::semicolonPartOfExpressionStatement()
{
    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(QLatin1String("A { property int x: 1+1; property int y: 2+2 \n"
                                "tt: {'a': 5, 'b': 6}; ff: {'c': 'rrr'}}"), 1);
    QQmlJS::Parser parser(&engine);
    QVERIFY(parser.parse());

    check::ExpressionStatementObserver observer;
    observer(parser.rootNode());

    QCOMPARE(observer.expressionsSeen, 4);
    QVERIFY(observer.endsWithSemicolon);
}

void tst_qqmlparser::typeAssertion_data()
{
    QTest::addColumn<QString>("expression");
    QTest::addRow("as A")
            << QString::fromLatin1("A { onStuff: (b as A).happen() }");

    // Rabbits cannot be discerned from types on a syntactical level.
    // (rabbits would instead cause an error, as they are lower case)

    QTest::addRow("as Rabbit")
            << QString::fromLatin1("A { onStuff: (b as Rabbit).happen() }");
    QTest::addRow("as Rabbit paren")
            << QString::fromLatin1("A { onStuff: console.log((12 as Rabbit)); }");
    QTest::addRow("as Rabbit noparen")
            << QString::fromLatin1("A { onStuff: console.log(12 as Rabbit); }");
    QTest::addRow("property as Rabbit")
            << QString::fromLatin1("A { prop: (12 as Rabbit); }");
    QTest::addRow("property noparen as Rabbit")
            << QString::fromLatin1("A { prop: 12 as Rabbit; }");
}

void tst_qqmlparser::typeAssertion()
{
    QFETCH(QString, expression);

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(expression, 1);
    QQmlJS::Parser parser(&engine);
    QVERIFY(parser.parse());
}

void tst_qqmlparser::annotations_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("refFile");

    QString tests = dataDirectory() + "/annotations/";
    QString compare = dataDirectory() + "/noannotations/";

    QStringList files;
    files << findFiles(QDir(tests));

    QStringList refFiles;
    refFiles << findFiles(QDir(compare));

    for (const QString &file: std::as_const(files)) {
        auto fileNameStart = file.lastIndexOf(QDir::separator());
        auto fileName = QStringView(file).mid(fileNameStart, file.size()-fileNameStart);
        auto ref=std::find_if(refFiles.constBegin(),refFiles.constEnd(), [fileName](const QString &s){ return s.endsWith(fileName); });
        if (ref != refFiles.constEnd())
            QTest::newRow(qPrintable(file)) << file << *ref;
        else
            QTest::newRow(qPrintable(file)) << file << QString();
    }
}

void tst_qqmlparser::annotations()
{
    using namespace QQmlJS;

    QFETCH(QString, file);
    QFETCH(QString, refFile);

    QString code;
    QString refCode;

    QFile f(file);
    if (f.open(QFile::ReadOnly))
        code = QString::fromUtf8(f.readAll());
    QFile refF(refFile);
    if (!refFile.isEmpty() && refF.open(QFile::ReadOnly))
        refCode = QString::fromUtf8(refF.readAll());

    const bool qmlMode = true;

    Engine engine;
    Lexer lexer(&engine);
    lexer.setCode(code, 1, qmlMode);
    Parser parser(&engine);
    QVERIFY(parser.parse());

    if (!refCode.isEmpty()) {
        Engine engine2;
        Lexer lexer2(&engine2);
        lexer2.setCode(refCode, 1, qmlMode);
        Parser parser2(&engine2);
        QVERIFY(parser2.parse());

        using namespace QQmlJS::Dom;
        QString diff = astNodeDiff(parser.ast(), parser2.rootNode(), 3, AstDumperOption::NoAnnotations | AstDumperOption::NoLocations);
        QVERIFY2(diff.isEmpty(), qPrintable(diff));
    }
}

void tst_qqmlparser::invalidImportVersion_data()
{
    QTest::addColumn<QString>("expression");

    const QStringList segments = {
        "0", "255", "500", "3030303030303030303030303"
    };

    for (const QString &major : segments) {
        if (major != "0") {
            QTest::addRow("%s", qPrintable(major))
                    << QString::fromLatin1("import Foo %1").arg(major);
        }

        for (const QString &minor : segments) {
            if (major == "0" && minor == "0")
                continue;

            QTest::addRow("%s.%s", qPrintable(major), qPrintable(minor))
                    << QString::fromLatin1("import Foo %1.%2").arg(major).arg(minor);
        }
    }


}

void tst_qqmlparser::invalidImportVersion()
{
    QFETCH(QString, expression);

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(expression, 1);
    QQmlJS::Parser parser(&engine);
    QVERIFY(!parser.parse());

    QRegularExpression regexp(
                "^Invalid (major )?version. Version numbers must be >= 0 and < 255\\.$");
    QVERIFY(regexp.match(parser.errorMessage()).hasMatch());
}

QTEST_MAIN(tst_qqmlparser)

#include "tst_qqmlparser.moc"
