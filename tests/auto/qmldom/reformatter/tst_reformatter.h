// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef TST_QMLDOMCODEFORMATTER_H
#define TST_QMLDOMCODEFORMATTER_H
#include <QtQmlDom/private/qqmldomlinewriter_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

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
private slots:

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
        DomItem env = DomEnvironment::create(
                qmltypeDirs,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);
        QString testFilePath = baseDir + QLatin1Char('/') + inFile;
        DomItem tFile;
        env.loadBuiltins();
        env.loadFile(
                testFilePath, QString(),
                [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; },
                LoadOption::DefaultLoad);
        env.loadPendingDependencies();

        MutableDomItem myFile = tFile.field(Fields::currentItem);

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
        int iLine = 0;
        auto writeReformatted = [fullRes]() {
            qDebug().noquote().nospace() << "Reformatted output:\n"
                                         << "-----------------\n"
                                         << fullRes << "-----------------\n";
        };
        while (!line.isNull() && !resLine.isNull()) {
            ++iLine;
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

private:
};

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE

#endif // TST_QMLDOMSCANNER_H
