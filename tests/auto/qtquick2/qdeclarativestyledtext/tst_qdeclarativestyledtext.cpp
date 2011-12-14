/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qtest.h>
#include <QtTest/QtTest>
#include <QtGui/QTextLayout>
#include <private/qdeclarativestyledtext_p.h>

class tst_qdeclarativestyledtext : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativestyledtext()
    {
    }

    struct Format {
        enum Type {
            Bold = 0x01,
            Underline = 0x02,
            Italic = 0x04
        };
        Format(int t, int s, int l)
            : type(t), start(s), length(l) {}
        int type;
        int start;
        int length;
    };
    typedef QList<Format> FormatList;

private slots:
    void textOutput();
    void textOutput_data();
};

Q_DECLARE_METATYPE(tst_qdeclarativestyledtext::FormatList);


// For malformed input all we test is that we get the expected text and format out.
// 
void tst_qdeclarativestyledtext::textOutput_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("output");
    QTest::addColumn<FormatList>("formats");

    QTest::newRow("bold") << "<b>bold</b>" << "bold" << (FormatList() << Format(Format::Bold, 0, 4));
    QTest::newRow("italic") << "<i>italic</i>" << "italic" << (FormatList() << Format(Format::Italic, 0, 6));
    QTest::newRow("strong") << "<strong>strong</strong>" << "strong" << (FormatList() << Format(Format::Bold, 0, 6));
    QTest::newRow("missing >") << "<b>text</b" << "text" << (FormatList() << Format(Format::Bold, 0, 4));
    QTest::newRow("missing b>") << "<b>text</" << "text" << (FormatList() << Format(Format::Bold, 0, 4));
    QTest::newRow("missing /b>") << "<b>text<" << "text" << (FormatList() << Format(Format::Bold, 0, 4));
    QTest::newRow("missing </b>") << "<b>text" << "text" << (FormatList() << Format(Format::Bold, 0, 4));
    QTest::newRow("nested") << "<b>text <i>italic</i> bold</b>" << "text italic bold" << (FormatList() << Format(Format::Bold, 0, 5) << Format(Format::Bold | Format::Italic, 5, 6) << Format(Format::Bold, 11, 5));
    QTest::newRow("bad nest") << "<b>text <i>italic</b></i>" << "text italic" << (FormatList() << Format(Format::Bold, 0, 5) << Format(Format::Bold | Format::Italic, 5, 6));
    QTest::newRow("font color") << "<font color=\"red\">red text</font>" << "red text" << (FormatList() << Format(0, 0, 8));
    QTest::newRow("font color: single quote") << "<font color='red'>red text</font>" << "red text" << (FormatList() << Format(0, 0, 8));
    QTest::newRow("font size") << "<font size=\"1\">text</font>" << "text" << (FormatList() << Format(0, 0, 4));
    QTest::newRow("font empty") << "<font>text</font>" << "text" << FormatList();
    QTest::newRow("font bad 1") << "<font ezis=\"blah\">text</font>" << "text" << FormatList();
    QTest::newRow("font bad 2") << "<font size=\"1>text</font>" << "" << FormatList();
    QTest::newRow("extra close") << "<b>text</b></b>" << "text" << (FormatList() << Format(Format::Bold, 0, 4));
    QTest::newRow("extra space") << "<b >text</b>" << "text" << (FormatList() << Format(Format::Bold, 0, 4));
    QTest::newRow("entities") << "&lt;b&gt;this &amp; that&lt;/b&gt;" << "<b>this & that</b>" << FormatList();
    QTest::newRow("newline") << "text<br>more text" << QLatin1String("text") + QChar(QChar::LineSeparator) + QLatin1String("more text") << FormatList();
    QTest::newRow("paragraph") << "text<p>more text" << QLatin1String("text") + QChar(QChar::LineSeparator) + QLatin1String("more text") << FormatList();
    QTest::newRow("paragraph closed") << "text<p>more text</p>more text" << QLatin1String("text") + QChar(QChar::LineSeparator) + QLatin1String("more text")  + QChar(QChar::LineSeparator) + QLatin1String("more text") << FormatList();
    QTest::newRow("paragraph closed bold") << "<b>text<p>more text</p>more text</b>" << QLatin1String("text") + QChar(QChar::LineSeparator) + QLatin1String("more text")  + QChar(QChar::LineSeparator) + QLatin1String("more text") << (FormatList() << Format(Format::Bold, 0, 24));
    QTest::newRow("self-closing newline") << "text<br/>more text" << QLatin1String("text") + QChar(QChar::LineSeparator) + QLatin1String("more text") << FormatList();
    QTest::newRow("empty") << "" << "" << FormatList();
    QTest::newRow("unknown tag") << "<a href='#'><foo>underline</foo></a> not" << "underline not" << (FormatList() << Format(Format::Underline, 0, 9));
    QTest::newRow("h0") << "<h0>head" << "head" << FormatList();
    QTest::newRow("h1") << "<h1>head" << QChar(QChar::LineSeparator) + QLatin1String("head") << (FormatList() << Format(Format::Bold, 0, 5));
    QTest::newRow("h2") << "<h2>head" << QChar(QChar::LineSeparator) + QLatin1String("head") << (FormatList() << Format(Format::Bold, 0, 5));
    QTest::newRow("h3") << "<h3>head" << QChar(QChar::LineSeparator) + QLatin1String("head") << (FormatList() << Format(Format::Bold, 0, 5));
    QTest::newRow("h4") << "<h4>head" << QChar(QChar::LineSeparator) + QLatin1String("head") << (FormatList() << Format(Format::Bold, 0, 5));
    QTest::newRow("h5") << "<h5>head" << QChar(QChar::LineSeparator) + QLatin1String("head") << (FormatList() << Format(Format::Bold, 0, 5));
    QTest::newRow("h6") << "<h6>head" << QChar(QChar::LineSeparator) + QLatin1String("head") << (FormatList() << Format(Format::Bold, 0, 5));
    QTest::newRow("h7") << "<h7>head" << "head" << FormatList();
}

void tst_qdeclarativestyledtext::textOutput()
{
    QFETCH(QString, input);
    QFETCH(QString, output);
    QFETCH(FormatList, formats);

    QTextLayout layout;
    QDeclarativeStyledText::parse(input, layout);

    QCOMPARE(layout.text(), output);

    QList<QTextLayout::FormatRange> layoutFormats = layout.additionalFormats();

    QCOMPARE(layoutFormats.count(), formats.count());
    for (int i = 0; i < formats.count(); ++i) {
        QCOMPARE(layoutFormats.at(i).start, formats.at(i).start);
        QCOMPARE(layoutFormats.at(i).length, formats.at(i).length);
        if (formats.at(i).type & Format::Bold)
            QVERIFY(layoutFormats.at(i).format.fontWeight() == QFont::Bold);
        else
            QVERIFY(layoutFormats.at(i).format.fontWeight() == QFont::Normal);
        QVERIFY(layoutFormats.at(i).format.fontItalic() == bool(formats.at(i).type & Format::Italic));
        QVERIFY(layoutFormats.at(i).format.fontUnderline() == bool(formats.at(i).type & Format::Underline));
    }
}


QTEST_MAIN(tst_qdeclarativestyledtext)

#include "tst_qdeclarativestyledtext.moc"
