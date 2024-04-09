// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLDOMCODEFORMATTER_H
#define TST_QMLDOMCODEFORMATTER_H
#include <QtQmlDom/private/qqmldomlinewriter_p.h>
#include <QtQmlDom/private/qqmldomindentinglinewriter_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlDom/private/qqmldomreformatter_p.h>

#include <QtTest/QtTest>
#include <QCborValue>
#include <QDebug>
#include <QLibraryInfo>

#include <memory>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

class TestReformatter : public QObject
{
    Q_OBJECT
public:
private:
    // TODO Move to a dedicated LineWriter factory / LineWriter API ?
    enum class LineWriterType { Default, Indenting };
    std::unique_ptr<LineWriter> getLineWriter(const SinkF &innerSink,
                                              const LineWriterOptions &lwOptions)
    {
        return lwOptions.maxLineLength > 0
                ? getLineWriter(LineWriterType::Indenting, innerSink, lwOptions)
                : getLineWriter(LineWriterType::Default, innerSink, lwOptions);
    }

    std::unique_ptr<LineWriter> getLineWriter(LineWriterType type, const SinkF &innerSink,
                                              const LineWriterOptions &lwOptions)
    {
        switch (type) {
        case LineWriterType::Indenting:
            return std::make_unique<IndentingLineWriter>(innerSink, QLatin1String("*testStream*"),
                                                         lwOptions);
        default:
            return std::make_unique<LineWriter>(innerSink, QLatin1String("*testStream*"),
                                                lwOptions);
        }
        Q_UNREACHABLE_RETURN(nullptr);
    }

    // "Unix" LineWriter (with '\n' line endings) is used by default,
    // under the assumption that line endings are properly tested in lineWriter() test.
    static LineWriterOptions defaultLineWriterOptions()
    {
        LineWriterOptions opts;
        opts.lineEndings = LineWriterOptions::LineEndings::Unix;
        return opts;
    }

    QString formatJSCode(const QString &jsCode,
                         const LineWriterOptions &lwOptions = defaultLineWriterOptions())
    {
        return formatPlainJS(jsCode, ScriptExpression::ExpressionType::JSCode, lwOptions);
    }

    QString formatJSModuleCode(const QString &jsCode,
                               const LineWriterOptions &lwOptions = defaultLineWriterOptions())
    {
        return formatPlainJS(jsCode, ScriptExpression::ExpressionType::ESMCode, lwOptions);
    }

    QString formatPlainJS(const QString &jsCode, ScriptExpression::ExpressionType exprType,
                          const LineWriterOptions &lwOptions = defaultLineWriterOptions())
    {
        QString resultStr;
        QTextStream res(&resultStr);
        auto lwPtr = getLineWriter([&res](QStringView s) { res << s; }, lwOptions);
        assert(lwPtr);
        OutWriter ow(*lwPtr);

        const ScriptExpression scriptItem(jsCode, exprType);
        scriptItem.writeOut(DomItem(), ow);

        lwPtr->flush(); // flush instead of eof to ignore line endings
        res.flush();
        return resultStr;
    }

private slots:
    void reindent_data()
    {
        QTest::addColumn<QString>("inFile");
        QTest::addColumn<QString>("outFile");

        QTest::newRow("file1") << QStringLiteral(u"file1.qml") << QStringLiteral(u"file1.qml");
        QTest::newRow("file1 unindented")
                << QStringLiteral(u"file1Unindented.qml") << QStringLiteral(u"file1.qml");
    }

    void reindent()
    {
        QFETCH(QString, inFile);
        QFETCH(QString, outFile);

        QFile fIn(QLatin1String(QT_QMLTEST_DATADIR) + QLatin1String("/reformatter/") + inFile);
        if (!fIn.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "could not open file" << inFile;
            return;
        }
        QFile fOut(QLatin1String(QT_QMLTEST_DATADIR) + QLatin1String("/reformatter/") + outFile);
        if (!fOut.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "could not open file" << outFile;
            return;
        }
        QTextStream in(&fIn);
        QTextStream out(&fOut);
        QString resultStr;
        QTextStream res(&resultStr);
        QString line = in.readLine();
        IndentingLineWriter lw([&res](QStringView s) { res << s; }, QLatin1String("*testStream*"));
        QList<SourceLocation *> sourceLocations;
        while (!line.isNull()) {
            SourceLocation *loc = new SourceLocation;
            sourceLocations.append(loc);
            lw.write(line, loc);
            lw.write(u"\n");
            line = in.readLine();
        }
        lw.eof();
        res.flush();
        QString fullRes = resultStr;
        res.seek(0);
        line = out.readLine();
        QString resLine = res.readLine();
        int iLoc = 0;
        int nextLoc = 0;
        while (!line.isNull() && !resLine.isNull()) {
            QCOMPARE(resLine, line);
            if (iLoc == nextLoc && iLoc < sourceLocations.size()) {
                QString l2 =
                        fullRes.mid(sourceLocations[iLoc]->offset, sourceLocations[iLoc]->length);
                if (!l2.contains(QLatin1Char('\n'))) {
                    QCOMPARE(l2, line);
                } else {
                    qDebug() << "skip checks of multiline location (line was split)" << l2;
                    iLoc -= l2.count(QLatin1Char('\n'));
                }
                ++nextLoc;
            } else {
                qDebug() << "skip multiline recover";
            }
            ++iLoc;
            line = out.readLine();
            resLine = res.readLine();
        }
        QCOMPARE(resLine.isNull(), line.isNull());
        for (auto sLoc : sourceLocations)
            delete sLoc;
    }

    void lineByLineReformatter_data()
    {
        QTest::addColumn<QString>("inFile");
        QTest::addColumn<QString>("outFile");
        QTest::addColumn<LineWriterOptions>("options");
        LineWriterOptions defaultOptions;
        LineWriterOptions noReorderOptions;
        noReorderOptions.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;

        QTest::newRow("file1") << QStringLiteral(u"file1.qml")
                               << QStringLiteral(u"file1Reformatted.qml") << defaultOptions;

        QTest::newRow("file2") << QStringLiteral(u"file2.qml")
                               << QStringLiteral(u"file2Reformatted.qml") << defaultOptions;

        QTest::newRow("commentedFile")
                << QStringLiteral(u"commentedFile.qml")
                << QStringLiteral(u"commentedFileReformatted.qml") << defaultOptions;

        QTest::newRow("required") << QStringLiteral(u"required.qml")
                                  << QStringLiteral(u"requiredReformatted.qml") << defaultOptions;

        QTest::newRow("inline") << QStringLiteral(u"inline.qml")
                                << QStringLiteral(u"inlineReformatted.qml") << defaultOptions;

        QTest::newRow("spread") << QStringLiteral(u"spread.qml")
                                << QStringLiteral(u"spreadReformatted.qml") << defaultOptions;

        QTest::newRow("template") << QStringLiteral(u"template.qml")
                                  << QStringLiteral(u"templateReformatted.qml") << defaultOptions;

        QTest::newRow("typeAnnotations")
                << QStringLiteral(u"typeAnnotations.qml")
                << QStringLiteral(u"typeAnnotationsReformatted.qml") << defaultOptions;

        QTest::newRow("file1NoReorder")
                << QStringLiteral(u"file1.qml") << QStringLiteral(u"file1Reformatted2.qml")
                << noReorderOptions;
    }

    void lineByLineReformatter()
    {
        QFETCH(QString, inFile);
        QFETCH(QString, outFile);
        QFETCH(LineWriterOptions, options);

        QString baseDir = QLatin1String(QT_QMLTEST_DATADIR) + QLatin1String("/reformatter");
        QStringList qmltypeDirs =
                QStringList({ baseDir, QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath) });
        auto envPtr = DomEnvironment::create(
                qmltypeDirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);
        QString testFilePath = baseDir + QLatin1Char('/') + inFile;
        DomItem tFile;
        envPtr->loadBuiltins();
        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, testFilePath),
                         [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; });
        envPtr->loadPendingDependencies();

        MutableDomItem myFile = tFile.field(Fields::currentItem);

        QString resultStr;
        QTextStream res(&resultStr);
        IndentingLineWriter lw([&res](QStringView s) { res << s; }, QLatin1String("*testStream*"),
                               options);
        OutWriter ow(lw);
        DomItem qmlFile = tFile.field(Fields::currentItem);
        qmlFile.writeOut(ow);
        lw.eof();
        res.flush();
        QString fullRes = resultStr;
        res.seek(0);
        QFile fOut(baseDir + QLatin1Char('/') + outFile);
        if (!fOut.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "could not open file" << outFile;
            return;
        }
        QTextStream out(&fOut);
        QString line = out.readLine();
        QString resLine = res.readLine();
        auto writeReformatted = [fullRes]() {
            qDebug().noquote().nospace() << "Reformatted output:\n"
                                         << "-----------------\n"
                                         << fullRes << "-----------------\n";
        };
        while (!line.isNull() && !resLine.isNull()) {
            if (resLine != line)
                writeReformatted();
            QCOMPARE(resLine, line);
            line = out.readLine();
            resLine = res.readLine();
        }
        if (resLine.isNull() != line.isNull()) {
            writeReformatted();
            qDebug() << "reformatted at end" << resLine.isNull() << resLine
                     << "reference at end:" << line.isNull() << line;
        }
        QCOMPARE(resLine.isNull(), line.isNull());
    }

    void manualReformatter_data()
    {
        LineWriterOptions noReorderOptions;
        QTest::addColumn<QString>("inFile");
        QTest::addColumn<QString>("outFile");
        QTest::addColumn<LineWriterOptions>("options");
        LineWriterOptions defaultOptions;

        noReorderOptions.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;

        QTest::newRow("file1") << QStringLiteral(u"file1.qml")
                               << QStringLiteral(u"file1Reformatted.qml") << defaultOptions;

        QTest::newRow("file2") << QStringLiteral(u"file2.qml")
                               << QStringLiteral(u"file2Reformatted.qml") << defaultOptions;

        QTest::newRow("commentedFile")
                << QStringLiteral(u"commentedFile.qml")
                << QStringLiteral(u"commentedFileReformatted2.qml") << defaultOptions;

        QTest::newRow("required") << QStringLiteral(u"required.qml")
                                  << QStringLiteral(u"requiredReformatted2.qml") << defaultOptions;

        QTest::newRow("inline") << QStringLiteral(u"inline.qml")
                                << QStringLiteral(u"inlineReformatted.qml") << defaultOptions;

        QTest::newRow("spread") << QStringLiteral(u"spread.qml")
                                << QStringLiteral(u"spreadReformatted.qml") << defaultOptions;

        QTest::newRow("template") << QStringLiteral(u"template.qml")
                                  << QStringLiteral(u"templateReformatted.qml") << defaultOptions;

        QTest::newRow("arrowFunctions")
                << QStringLiteral(u"arrowFunctions.qml")
                << QStringLiteral(u"arrowFunctionsReformatted.qml") << defaultOptions;

        QTest::newRow("file1NoReorder")
                << QStringLiteral(u"file1.qml") << QStringLiteral(u"file1Reformatted2.qml")
                << noReorderOptions;
        QTest::newRow("noMerge")
                << QStringLiteral(u"noMerge.qml") << QStringLiteral(u"noMergeReformatted.qml")
                << defaultOptions;
    }

    void manualReformatter()
    {
        QFETCH(QString, inFile);
        QFETCH(QString, outFile);
        QFETCH(LineWriterOptions, options);

        QString baseDir = QLatin1String(QT_QMLTEST_DATADIR) + QLatin1String("/reformatter");
        QStringList qmltypeDirs =
                QStringList({ baseDir, QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath) });
        auto envPtr = DomEnvironment::create(
                qmltypeDirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);
        QString testFilePath = baseDir + QLatin1Char('/') + inFile;
        DomItem tFile;
        envPtr->loadBuiltins();
        envPtr->loadFile(FileToLoad::fromFileSystem(envPtr, testFilePath),
                         [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; });
        envPtr->loadPendingDependencies();

        QString resultStr;
        QTextStream res(&resultStr);
        LineWriter lw([&res](QStringView s) { res << s; }, QLatin1String("*testStream*"), options);
        OutWriter ow(lw);
        ow.indentNextlines = true;
        DomItem qmlFile = tFile.field(Fields::currentItem);
        qmlFile.writeOut(ow);
        lw.eof();
        res.flush();
        QString fullRes = resultStr;
        res.seek(0);
        QFile fOut(baseDir + QLatin1Char('/') + outFile);
        if (!fOut.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "could not open file" << outFile;
            return;
        }
        QTextStream out(&fOut);
        QString line = out.readLine();
        QString resLine = res.readLine();
        auto writeReformatted = [fullRes]() {
            qDebug().noquote().nospace() << "Reformatted output:\n"
                                         << "-----------------\n"
                                         << fullRes << "-----------------\n";
        };
        while (!line.isNull() && !resLine.isNull()) {
            if (resLine != line)
                writeReformatted();
            QCOMPARE(resLine, line);
            line = out.readLine();
            resLine = res.readLine();
        }
        if (resLine.isNull() != line.isNull()) {
            writeReformatted();
            qDebug() << "reformatted at end" << resLine.isNull() << resLine
                     << "reference at end:" << line.isNull() << line;
        }
        QCOMPARE(resLine.isNull(), line.isNull());
    }

    void indentInfo()
    {
        IndentInfo i1(u"\n\n  ", 4);
        QCOMPARE(i1.trailingString, u"  ");
        QCOMPARE(i1.nNewlines, 2);
        QCOMPARE(i1.column, 2);
        IndentInfo i2(u"\r\n\r\n  ", 4);
        QCOMPARE(i2.trailingString, u"  ");
        QCOMPARE(i2.nNewlines, 2);
        QCOMPARE(i2.column, 2);
        IndentInfo i3(u"\n ", 4);
        QCOMPARE(i3.trailingString, u" ");
        QCOMPARE(i3.nNewlines, 1);
        QCOMPARE(i3.column, 1);
        IndentInfo i4(u"\r\n ", 4);
        QCOMPARE(i4.trailingString, u" ");
        QCOMPARE(i4.nNewlines, 1);
        QCOMPARE(i4.column, 1);
        IndentInfo i5(u"\n", 4);
        QCOMPARE(i5.trailingString, u"");
        QCOMPARE(i5.nNewlines, 1);
        QCOMPARE(i5.column, 0);
        IndentInfo i6(u"\r\n", 4);
        QCOMPARE(i6.trailingString, u"");
        QCOMPARE(i6.nNewlines, 1);
        QCOMPARE(i6.column, 0);
        IndentInfo i7(u"  ", 4);
        QCOMPARE(i7.trailingString, u"  ");
        QCOMPARE(i7.nNewlines, 0);
        QCOMPARE(i7.column, 2);
        IndentInfo i8(u"", 4);
        QCOMPARE(i8.trailingString, u"");
        QCOMPARE(i8.nNewlines, 0);
        QCOMPARE(i8.column, 0);
    }

    void lineWriter()
    {
        {
            QString res;
            LineWriterOptions opts;
            opts.lineEndings = LineWriterOptions::LineEndings::Unix;
            LineWriter lw([&res](QStringView v) { res.append(v); }, QLatin1String("*testStream*"),
                          opts);
            lw.write(u"a\nb");
            lw.write(u"c\r\nd");
            lw.write(u"e\rf");
            lw.write(u"g\r\n");
            lw.write(u"h\r");
            lw.write(u"\n");
            QCOMPARE(res, u"a\nbc\nde\nfg\nh\n\n");
        }
        {
            QString res;
            LineWriterOptions opts;
            opts.lineEndings = LineWriterOptions::LineEndings::Windows;
            LineWriter lw([&res](QStringView v) { res.append(v); }, QLatin1String("*testStream*"),
                          opts);
            lw.write(u"a\nb");
            lw.write(u"c\r\nd");
            lw.write(u"e\rf");
            lw.write(u"g\r\n");
            lw.write(u"h\r");
            lw.write(u"\n");
            QCOMPARE(res, u"a\r\nbc\r\nde\r\nfg\r\nh\r\n\r\n");
        }
        {
            QString res;
            LineWriterOptions opts;
            opts.lineEndings = LineWriterOptions::LineEndings::OldMacOs;
            LineWriter lw([&res](QStringView v) { res.append(v); }, QLatin1String("*testStream*"),
                          opts);
            lw.write(u"a\nb");
            lw.write(u"c\r\nd");
            lw.write(u"e\rf");
            lw.write(u"g\r\n");
            lw.write(u"h\r");
            lw.write(u"\n");
            QCOMPARE(res, u"a\rbc\rde\rfg\rh\r\r");
        }
    }

    void hoistableDeclaration_data()
    {
        QTest::addColumn<QString>("declarationToBeFormatted");
        QTest::addColumn<QString>("expectedFormattedDeclaration");

        QTest::newRow("Function") << QStringLiteral(u"function a(a,b){}")
                                  << QStringLiteral(u"function a(a, b) {}");
        QTest::newRow("AnonymousFunction") << QStringLiteral(u"let f=function (a,b){}")
                                           << QStringLiteral(u"let f = function (a, b) {}");
        QTest::newRow("Generator_lhs_star")
                << QStringLiteral(u"function* g(a,b){}") << QStringLiteral(u"function* g(a, b) {}");
        QTest::newRow("Generator_rhs_star")
                << QStringLiteral(u"function *g(a,b){}") << QStringLiteral(u"function* g(a, b) {}");
        QTest::newRow("AnonymousGenerator") << QStringLiteral(u"let g=function * (a,b){}")
                                            << QStringLiteral(u"let g = function* (a, b) {}");
    }

    // https://262.ecma-international.org/7.0/#prod-HoistableDeclaration
    void hoistableDeclaration()
    {
        QFETCH(QString, declarationToBeFormatted);
        QFETCH(QString, expectedFormattedDeclaration);

        QString formattedDeclaration = formatJSCode(declarationToBeFormatted);

        QCOMPARE(formattedDeclaration, expectedFormattedDeclaration);
    }

    void exportDeclarations_data()
    {
        QTest::addColumn<QString>("exportToBeFormatted");
        QTest::addColumn<QString>("expectedFormattedExport");
        // not exhaustive list of ExportDeclarations as per
        // https://262.ecma-international.org/7.0/#prod-ExportDeclaration

        // LexicalDeclaration
        QTest::newRow("LexicalDeclaration_let_Binding")
                << QStringLiteral(u"export let name") << QStringLiteral(u"export let name;");
        QTest::newRow("LexicalDeclaration_const_BindingList")
                << QStringLiteral(u"export const "
                                  u"n1=1,n2=2,n3=3,n4=4,n5=5")
                << QStringLiteral(u"export const "
                                  u"n1 = 1, n2 = 2, n3 = 3, n4 = 4, n5 = 5;");
        QTest::newRow("LexicalDeclaration_const_ArrayBinding")
                << QStringLiteral(u"export const "
                                  u"[a,b]=a_and_b")
                << QStringLiteral(u"export const "
                                  u"[a, b] = a_and_b;");
        QTest::newRow("LexicalDeclaration_let_ObjectBinding")
                << QStringLiteral(u"export let "
                                  u"{a,b:c}=a_and_b")
                << QStringLiteral(u"export let "
                                  u"{\na,\nb: c\n} = a_and_b;");

        // ClassDeclaration
        QTest::newRow("ClassDeclaration") << QStringLiteral(u"export "
                                                            u"class A extends B{}")
                                          << QStringLiteral(u"export "
                                                            u"class A extends B {}");

        // HoistableDeclaration
        QTest::newRow("HoistableDeclaration_FunctionDeclaration")
                << QStringLiteral(u"export "
                                  u"function a(a,b){}")
                << QStringLiteral(u"export "
                                  u"function a(a, b) {}");
        QTest::newRow("HoistableDeclaration_GeneratorDeclaration")
                << QStringLiteral(u"export "
                                  u"function * g(a,b){}")
                << QStringLiteral(u"export "
                                  u"function* g(a, b) {}");

        // export ExportClause ;
        QTest::newRow("ExportClause_Empty")
                << QStringLiteral(u"export{}") << QStringLiteral(u"export {};");
        QTest::newRow("ExportClause_1Specifier")
                << QStringLiteral(u"export{one}") << QStringLiteral(u"export { one };");
        QTest::newRow("ExportClause_Specifier_as")
                << QStringLiteral(u"export{one as o}") << QStringLiteral(u"export { one as o };");
        QTest::newRow("ExportClause_Specifier_as_StringLiteral")
                << QStringLiteral(u"export{one as \"s\"}")
                << QStringLiteral(u"export { one as \"s\" };");
        QTest::newRow("ExportClause_ExportsList")
                << QStringLiteral(u"export{one,two,three,four as fo,five}")
                << QStringLiteral(u"export { one, two, three, four as fo, five };");

        // export * FromClause ;
        QTest::newRow("star") << QStringLiteral(u"export * from \"design\"")
                              << QStringLiteral(u"export * from \"design\";");
        QTest::newRow("star_as_Specifier") << QStringLiteral(u"export * as star from \"design\"")
                                           << QStringLiteral(u"export * as star from \"design\";");

        // export ExportClause FromClause ;
        QTest::newRow("ExportClause")
                << QStringLiteral(u"export {i1 as n1,i2 as n2,nN} from \"M\"")
                << QStringLiteral(u"export { i1 as n1, i2 as n2, nN } from \"M\";");

        // export default HoistableDeclaration
        QTest::newRow("Default_AnonymousFunction")
                << QStringLiteral(u"export default function(a,b){}")
                << QStringLiteral(u"export default function (a, b) {}");
        QTest::newRow("Default_AnonymousGenerator")
                << QStringLiteral(u"export default function * (a,b){}")
                << QStringLiteral(u"export default function* (a, b) {}");
        QTest::newRow("Default_Function") << QStringLiteral(u"export default function a(a,b){}")
                                          << QStringLiteral(u"export default function a(a, b) {}");

        // export default ClassDeclaration
        QTest::newRow("Default_Class") << QStringLiteral(u"export default class A{}")
                                       << QStringLiteral(u"export default class A {}");
        QTest::newRow("Default_AnonymousClass")
                << QStringLiteral(u"export default class extends A{}")
                << QStringLiteral(u"export default class extends A{}");

        // export default Expression
        QTest::newRow("Default_Expression") << QStringLiteral(u"export default 1+1")
                                            << QStringLiteral(u"export default 1 + 1;");
        QTest::newRow("Default_ArrowFunctionExpression")
                << QStringLiteral(u"export default(x,y)=> x+2")
                << QStringLiteral(u"export default (x, y) => x + 2;");
    }

    // https://262.ecma-international.org/7.0/#prod-ExportDeclaration
    void exportDeclarations()
    {
        QFETCH(QString, exportToBeFormatted);
        QFETCH(QString, expectedFormattedExport);

        QString formattedExport = formatJSModuleCode(exportToBeFormatted);

        QEXPECT_FAIL("ExportClause_Specifier_as_StringLiteral",
                     "export {a as \"string name\"} declaration is not supported yet", Abort);
        QEXPECT_FAIL("star_as_Specifier", "export * as star declaration is not supported yet",
                     Abort);
        QEXPECT_FAIL("Default_AnonymousClass", "QTBUG-122291", Abort);
        QEXPECT_FAIL("Default_AnonymousFunction", "QTBUG-122291", Abort);
        QEXPECT_FAIL("Default_AnonymousGenerator", "QTBUG-122291", Abort);
        QCOMPARE(formattedExport, expectedFormattedExport);
    }

    void carryoverMJS_data()
    {
        QTest::addColumn<QString>("codeToBeFormatted");
        QTest::addColumn<int>("maxLineLength");
        QTest::addColumn<QString>("expectedFormattedCode");

        QTest::newRow("LongExportList_NoMaxLineLength")
                << QStringLiteral(u"export const n1=1,n2=2,n3=3,n4=4,n5=5") << -1
                << QStringLiteral(u"export const n1 = 1, n2 = 2, n3 = 3, n4 = 4, n5 = 5;");
        QTest::newRow("LongExportList_MaxLineLength20")
                << QStringLiteral(u"export const n1=1,n2=2,n3=3,n4=4,n5=5") << 20
                << QStringLiteral(u"export const n1 = 1,\n"
                                  u"  n2 = 2, n3 = 3,\n"
                                  u"  n4 = 4, n5 = 5;");
    }

    void carryoverMJS()
    {
        QFETCH(QString, codeToBeFormatted);
        QFETCH(int, maxLineLength);
        QFETCH(QString, expectedFormattedCode);

        LineWriterOptions lwOptions;
        lwOptions.maxLineLength = maxLineLength;
        // TODO maybe fetch this
        lwOptions.formatOptions.indentSize = 2;
        QString formattedCode = formatJSModuleCode(codeToBeFormatted, lwOptions);

        QEXPECT_FAIL("LongExportList_MaxLineLength20", "QTBUG-122260", Abort);
        QCOMPARE(formattedCode, expectedFormattedCode);
    }

    void importDeclarations_data()
    {
        QTest::addColumn<QString>("importToBeFormatted");
        QTest::addColumn<QString>("expectedFormattedImport");
        // not exhaustive list of ExportDeclarations as per
        // https://262.ecma-international.org/7.0/#prod-ImportDeclaration

        // import ModuleSpecifier;
        QTest::newRow("ModuleSpecifier")
                << QStringLiteral(u"import \"Module\"") << QStringLiteral(u"import \"Module\";");

        // import ImportClause FromClause ;
        QTest::newRow("NameSpaceImport") << QStringLiteral(u"import * as d from \"design\";")
                                         << QStringLiteral(u"import * as d from \"design\";");

        QTest::newRow("NamedImports") << QStringLiteral(u"import {b,cd as c,d} from \"M\";")
                                      << QStringLiteral(u"import { b, cd as c, d } from \"M\";");

        QTest::newRow("DefaultBindung") << QStringLiteral(u"import defaultExport from \"M\"")
                                        << QStringLiteral(u"import defaultExport from \"M\";");
        QTest::newRow("DefaultBindung_NameSpaceImport")
                << QStringLiteral(u"import defaultExport, * as m from \"M\";")
                << QStringLiteral(u"import defaultExport, * as m from \"M\";");
        QTest::newRow("DefaultBinding_NamedImports")
                << QStringLiteral(u"import defaultExport,{b,cd as c,d} from \"M\";")
                << QStringLiteral(u"import defaultExport, { b, cd as c, d } from \"M\";");

        QTest::newRow("ImportClause_Specifier_as_StringLiteral")
                << QStringLiteral(u"import{\"s\" as s} from \"M\"")
                << QStringLiteral(u"import { \"s\" as s } from \"M\";");
    }

    // https://262.ecma-international.org/7.0/#prod-ImportDeclaration
    void importDeclarations()
    {
        QFETCH(QString, importToBeFormatted);
        QFETCH(QString, expectedFormattedImport);

        QString formattedImport = formatJSModuleCode(importToBeFormatted);

        QEXPECT_FAIL(
                "ImportClause_Specifier_as_StringLiteral",
                "import {\"string literal export\" as alias } declaration is not supported yet",
                Abort);
        QCOMPARE(formattedImport, expectedFormattedImport);
    }

    void methodDefinitions_data()
    {
        QTest::addColumn<QString>("methodToBeFormatted");
        QTest::addColumn<QString>("expectedFormattedMethod");

        // ObjectInitializer
        QTest::newRow("ObjGetter") << QStringLiteral(u"const o={get a(){},}")
                                   << QStringLiteral(u"const o = {\nget a(){}\n}");
        QTest::newRow("ObjSetter") << QStringLiteral(u"const o={set a(a){},}")
                                   << QStringLiteral(u"const o = {\nset a(a){}\n}");
        QTest::newRow("ComputedObjPropertyGetter")
                << QStringLiteral(u"const o={get [a+b](){},}")
                << QStringLiteral(u"const o = {\nget [a + b](){}\n}");

        // Generator
        QTest::newRow("ObjPropertyGenerator")
                << QStringLiteral(u"const o={*a(){1+1;},}")
                << QStringLiteral(u"const o = {\n*a(){\n1 + 1;\n}\n}");
        QTest::newRow("ComputedClassPropertyGenerator")
                << QStringLiteral(u"class A{*[a+b](){}}")
                << QStringLiteral(u"class A {\n*[a + b](){}\n}");

        // ClassDefinitions
        QTest::newRow("ClassGetter") << QStringLiteral(u"class A{get a(){}}")
                                     << QStringLiteral(u"class A {\nget a(){}\n}");
        QTest::newRow("ClassSetter") << QStringLiteral(u"class A{set a(a){}}")
                                     << QStringLiteral(u"class A {\nset a(a){}\n}");
    }

    // https://262.ecma-international.org/7.0/#sec-method-definitions
    void methodDefinitions()
    {
        QFETCH(QString, methodToBeFormatted);
        QFETCH(QString, expectedFormattedMethod);

        QString formattedMethod = formatJSCode(methodToBeFormatted);

        QCOMPARE(formattedMethod, expectedFormattedMethod);
    }

    void statementList_data()
    {
        QTest::addColumn<QString>("codeToBeFormatted");
        QTest::addColumn<QString>("expectedFormattedCode");

        QTest::newRow("StatementsOnTheSameLine")
                << QStringLiteral(u"a=1;b=1;") << QStringLiteral(u"a = 1;\nb = 1;");

        QTest::newRow("StatementsOnSuccessiveLines")
                << QStringLiteral(u"a=1;\nb=1;") << QStringLiteral(u"a = 1;\nb = 1;");

        QTest::newRow("EmptyLineBetweenStatements")
                << QStringLiteral(u"a=1;\n\nb=1;") << QStringLiteral(u"a = 1;\n\nb = 1;");

        QTest::newRow("MultipleEmptyLinesBetweenStatements")
                << QStringLiteral(u"a=1;\n\n\n\n\n\nb=1;") << QStringLiteral(u"a = 1;\n\nb = 1;");

        QTest::newRow("MultilineStatementWithStatementOnTheFollowingLine")
                << QStringLiteral(u"console.log(\n\n);\nb = 1;")
                << QStringLiteral(u"console.log();\nb = 1;");

        QTest::newRow("StatementWithPostCommentAndStatementOnTheFollowingLine")
                << QStringLiteral(u"a=1;//\nb=1;") << QStringLiteral(u"a = 1;//\nb = 1;");

        QTest::newRow("StatementWithPostCommentAndEmptyLineToNextStatement")
                << QStringLiteral(u"a=1;//\n\nb=1;") << QStringLiteral(u"a = 1;//\n\nb = 1;");

        QTest::newRow("StatementWithPostCommentAndMultipleEmptyLinesToNextStatement")
                << QStringLiteral(u"a=1;//\n\n\n\n\nb=1;") << QStringLiteral(u"a = 1;//\n\nb = 1;");

        QTest::newRow("StatementsWithCommentInBetweenThem")
                << QStringLiteral(u"a=1;\n//\nb=1;") << QStringLiteral(u"a = 1;\n//\nb = 1;");

        QTest::newRow("StatementsWithCommentAndSingleEmptyLineInBetweenThem")
                << QStringLiteral(u"a=1;\n\n//\n\nb=1;")
                << QStringLiteral(u"a = 1;\n\n//\n\nb = 1;");

        QTest::newRow("StatementsWithCommentAndMultipleEmptyLinesInBetweenThem")
                << QStringLiteral(u"a=1;\n\n\n\n//\n\n\nb=1;")
                << QStringLiteral(u"a = 1;\n\n//\n\nb = 1;");

        QTest::newRow("StatementWithSingleEmptyLineAndPreCommentOnNextStatement")
                << QStringLiteral(u"a=1;\n\n//\nb=1;") << QStringLiteral(u"a = 1;\n\n//\nb = 1;");

        QTest::newRow("StatementWithMultipleEmptyLinesAndPreCommentOnNextStatement")
                << QStringLiteral(u"a=1;\n\n\n\n\n\n\n\n//\nb=1;")
                << QStringLiteral(u"a = 1;\n\n//\nb = 1;");
    }

    void statementList()
    {
        QFETCH(QString, codeToBeFormatted);
        QFETCH(QString, expectedFormattedCode);

        QString formattedCode = formatJSCode(codeToBeFormatted);

        QCOMPARE(formattedCode, expectedFormattedCode);
    }

private:
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif // TST_QMLDOMSCANNER_H
