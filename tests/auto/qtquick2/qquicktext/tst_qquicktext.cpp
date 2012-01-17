/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtTest/QSignalSpy>
#include <QTextDocument>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtQuick/private/qquicktext_p.h>
#include <private/qquicktext_p_p.h>
#include <private/qdeclarativevaluetype_p.h>
#include <QFontMetrics>
#include <QGraphicsSceneMouseEvent>
#include <qmath.h>
#include <QtQuick/QQuickView>
#include <private/qapplication_p.h>
#include <limits.h>
#include <QtGui/QMouseEvent>
#include "../../shared/util.h"
#include "testhttpserver.h"

DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)

class tst_qquicktext : public QDeclarativeDataTest
{
    Q_OBJECT
public:
    tst_qquicktext();

private slots:
    void text();
    void width();
    void wrap();
    void elide();
    void multilineElide();
    void textFormat();

    void alignments_data();
    void alignments();

    void baseUrl();
    void embeddedImages_data();
    void embeddedImages();

    void lineCount();
    void lineHeight();

    // ### these tests may be trivial
    void horizontalAlignment();
    void horizontalAlignment_RightToLeft();
    void verticalAlignment();
    void font();
    void style();
    void color();
    void smooth();

    // QDeclarativeFontValueType
    void weight();
    void underline();
    void overline();
    void strikeout();
    void capitalization();
    void letterSpacing();
    void wordSpacing();

    void clickLink();

    void implicitSize_data();
    void implicitSize();

    void lineLaidOut();


private:
    QStringList standard;
    QStringList richText;

    QStringList horizontalAlignmentmentStrings;
    QStringList verticalAlignmentmentStrings;

    QList<Qt::Alignment> verticalAlignmentments;
    QList<Qt::Alignment> horizontalAlignmentments;

    QStringList styleStrings;
    QList<QQuickText::TextStyle> styles;

    QStringList colorStrings;

    QDeclarativeEngine engine;

    QQuickView *createView(const QString &filename);
};

tst_qquicktext::tst_qquicktext()
{
    standard << "the quick brown fox jumped over the lazy dog"
            << "the quick brown fox\n jumped over the lazy dog";

    richText << "<i>the <b>quick</b> brown <a href=\\\"http://www.google.com\\\">fox</a> jumped over the <b>lazy</b> dog</i>"
            << "<i>the <b>quick</b> brown <a href=\\\"http://www.google.com\\\">fox</a><br>jumped over the <b>lazy</b> dog</i>";

    horizontalAlignmentmentStrings << "AlignLeft"
            << "AlignRight"
            << "AlignHCenter";

    verticalAlignmentmentStrings << "AlignTop"
            << "AlignBottom"
            << "AlignVCenter";

    horizontalAlignmentments << Qt::AlignLeft
            << Qt::AlignRight
            << Qt::AlignHCenter;

    verticalAlignmentments << Qt::AlignTop
            << Qt::AlignBottom
            << Qt::AlignVCenter;

    styleStrings << "Normal"
            << "Outline"
            << "Raised"
            << "Sunken";

    styles << QQuickText::Normal
            << QQuickText::Outline
            << QQuickText::Raised
            << QQuickText::Sunken;

    colorStrings << "aliceblue"
            << "antiquewhite"
            << "aqua"
            << "darkkhaki"
            << "darkolivegreen"
            << "dimgray"
            << "palevioletred"
            << "lightsteelblue"
            << "#000000"
            << "#AAAAAA"
            << "#FFFFFF"
            << "#2AC05F";
    //
    // need a different test to do alpha channel test
    // << "#AA0011DD"
    // << "#00F16B11";
    //
}

QQuickView *tst_qquicktext::createView(const QString &filename)
{
    QQuickView *canvas = new QQuickView(0);

    canvas->setSource(QUrl::fromLocalFile(filename));
    return canvas;
}

void tst_qquicktext::text()
{
    {
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"\" }", QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->text(), QString(""));
        QVERIFY(textObject->width() == 0);

        delete textObject;
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));

        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->text(), standard.at(i));
        QVERIFY(textObject->width() > 0);

        delete textObject;
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"" + richText.at(i) + "\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QString expected = richText.at(i);
        QCOMPARE(textObject->text(), expected.replace("\\\"", "\""));
        QVERIFY(textObject->width() > 0);

        delete textObject;
    }
}

void tst_qquicktext::width()
{
    // uses Font metrics to find the width for standard and document to find the width for rich
    {
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"\" }", QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->width(), 0.);

        delete textObject;
    }

    bool requiresUnhintedMetrics = !qmlDisableDistanceField();

    for (int i = 0; i < standard.size(); i++)
    {
        QVERIFY(!Qt::mightBeRichText(standard.at(i))); // self-test

        QFont f;
        qreal metricWidth = 0.0;

        if (requiresUnhintedMetrics) {
            QString s = standard.at(i);
            s.replace(QLatin1Char('\n'), QChar::LineSeparator);

            QTextLayout layout(s);
            layout.setFlags(Qt::TextExpandTabs | Qt::TextShowMnemonic);
            {
                QTextOption option;
                option.setUseDesignMetrics(true);
                layout.setTextOption(option);
            }

            layout.beginLayout();
            forever {
                QTextLine line = layout.createLine();
                if (!line.isValid())
                    break;
            }

            layout.endLayout();

            metricWidth = qCeil(layout.boundingRect().width());
        } else {
            QFontMetricsF fm(f);
            qreal metricWidth = fm.size(Qt::TextExpandTabs && Qt::TextShowMnemonic, standard.at(i)).width();
            metricWidth = qCeil(metricWidth);
        }

        QString componentStr = "import QtQuick 2.0\nText { text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QVERIFY(textObject->boundingRect().width() > 0);
        QCOMPARE(textObject->width(), qreal(metricWidth));
        QVERIFY(textObject->textFormat() == QQuickText::AutoText); // setting text doesn't change format

        delete textObject;
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QVERIFY(Qt::mightBeRichText(richText.at(i))); // self-test

        QString componentStr = "import QtQuick 2.0\nText { text: \"" + richText.at(i) + "\"; textFormat: Text.RichText }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());
        QVERIFY(textObject != 0);

        QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
        QVERIFY(textPrivate != 0);

        QTextDocument *doc = textPrivate->textDocument();
        QVERIFY(doc != 0);

        QCOMPARE(int(textObject->width()), int(doc->idealWidth()));
        QVERIFY(textObject->textFormat() == QQuickText::RichText);

        delete textObject;
    }
}

void tst_qquicktext::wrap()
{
    int textHeight = 0;
    // for specified width and wrap set true
    {
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"Hello\"; wrapMode: Text.WordWrap; width: 300 }", QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());
        textHeight = textObject->height();

        QVERIFY(textObject != 0);
        QVERIFY(textObject->wrapMode() == QQuickText::WordWrap);
        QCOMPARE(textObject->width(), 300.);

        delete textObject;
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { wrapMode: Text.WordWrap; width: 30; text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->width(), 30.);
        QVERIFY(textObject->height() > textHeight);

        int oldHeight = textObject->height();
        textObject->setWidth(100);
        QVERIFY(textObject->height() < oldHeight);

        delete textObject;
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { wrapMode: Text.WordWrap; width: 30; text: \"" + richText.at(i) + "\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->width(), 30.);
        QVERIFY(textObject->height() > textHeight);

        qreal oldHeight = textObject->height();
        textObject->setWidth(100);
        QVERIFY(textObject->height() < oldHeight);

        delete textObject;
    }

    // richtext again with a fixed height
    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { wrapMode: Text.WordWrap; width: 30; height: 50; text: \"" + richText.at(i) + "\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->width(), 30.);
        QVERIFY(textObject->implicitHeight() > textHeight);

        qreal oldHeight = textObject->implicitHeight();
        textObject->setWidth(100);
        QVERIFY(textObject->implicitHeight() < oldHeight);

        delete textObject;
    }
}

void tst_qquicktext::elide()
{
    for (QQuickText::TextElideMode m = QQuickText::ElideLeft; m<=QQuickText::ElideNone; m=QQuickText::TextElideMode(int(m)+1)) {
        const char* elidename[]={"ElideLeft", "ElideRight", "ElideMiddle", "ElideNone"};
        QString elide = "elide: Text." + QString(elidename[int(m)]) + ";";

        // XXX Poor coverage.

        {
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(("import QtQuick 2.0\nText { text: \"\"; "+elide+" width: 100 }").toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

            QCOMPARE(textObject->elideMode(), m);
            QCOMPARE(textObject->width(), 100.);

            delete textObject;
        }

        for (int i = 0; i < standard.size(); i++)
        {
            QString componentStr = "import QtQuick 2.0\nText { "+elide+" width: 100; text: \"" + standard.at(i) + "\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

            QCOMPARE(textObject->elideMode(), m);
            QCOMPARE(textObject->width(), 100.);

            delete textObject;
        }

        // richtext - does nothing
        for (int i = 0; i < richText.size(); i++)
        {
            QString componentStr = "import QtQuick 2.0\nText { "+elide+" width: 100; text: \"" + richText.at(i) + "\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

            QCOMPARE(textObject->elideMode(), m);
            QCOMPARE(textObject->width(), 100.);

            delete textObject;
        }
    }
}

void tst_qquicktext::multilineElide()
{
    QQuickView *canvas = createView(testFile("multilineelide.qml"));

    QQuickText *myText = qobject_cast<QQuickText*>(canvas->rootObject());
    QVERIFY(myText != 0);

    QCOMPARE(myText->lineCount(), 3);
    QCOMPARE(myText->truncated(), true);

    qreal lineHeight = myText->paintedHeight() / 3.;

    // reduce size and ensure fewer lines are drawn
    myText->setHeight(lineHeight * 2);
    QCOMPARE(myText->lineCount(), 2);

    myText->setHeight(lineHeight);
    QCOMPARE(myText->lineCount(), 1);

    myText->setHeight(5);
    QCOMPARE(myText->lineCount(), 1);

    myText->setHeight(lineHeight * 3);
    QCOMPARE(myText->lineCount(), 3);

    // remove max count and show all lines.
    myText->setHeight(1000);
    myText->resetMaximumLineCount();

    QCOMPARE(myText->truncated(), false);

    // reduce size again
    myText->setHeight(lineHeight * 2);
    QCOMPARE(myText->lineCount(), 2);
    QCOMPARE(myText->truncated(), true);

    // change line height
    myText->setLineHeight(1.1);
    QCOMPARE(myText->lineCount(), 1);

    delete canvas;
}

void tst_qquicktext::textFormat()
{
    {
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"Hello\"; textFormat: Text.RichText }", QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QVERIFY(textObject->textFormat() == QQuickText::RichText);

        QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
        QVERIFY(textPrivate != 0);
        QVERIFY(textPrivate->richText == true);

        delete textObject;
    }
    {
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"<b>Hello</b>\" }", QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QVERIFY(textObject->textFormat() == QQuickText::AutoText);

        QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
        QVERIFY(textPrivate != 0);
        QVERIFY(textPrivate->styledText == true);

        delete textObject;
    }
    {
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"<b>Hello</b>\"; textFormat: Text.PlainText }", QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QVERIFY(textObject->textFormat() == QQuickText::PlainText);

        delete textObject;
    }
}


void tst_qquicktext::alignments_data()
{
    QTest::addColumn<int>("hAlign");
    QTest::addColumn<int>("vAlign");
    QTest::addColumn<QString>("expectfile");

    QTest::newRow("LT") << int(Qt::AlignLeft) << int(Qt::AlignTop) << testFile("alignments_lt.png");
    QTest::newRow("RT") << int(Qt::AlignRight) << int(Qt::AlignTop) << testFile("alignments_rt.png");
    QTest::newRow("CT") << int(Qt::AlignHCenter) << int(Qt::AlignTop) << testFile("alignments_ct.png");

    QTest::newRow("LB") << int(Qt::AlignLeft) << int(Qt::AlignBottom) << testFile("alignments_lb.png");
    QTest::newRow("RB") << int(Qt::AlignRight) << int(Qt::AlignBottom) << testFile("alignments_rb.png");
    QTest::newRow("CB") << int(Qt::AlignHCenter) << int(Qt::AlignBottom) << testFile("alignments_cb.png");

    QTest::newRow("LC") << int(Qt::AlignLeft) << int(Qt::AlignVCenter) << testFile("alignments_lc.png");
    QTest::newRow("RC") << int(Qt::AlignRight) << int(Qt::AlignVCenter) << testFile("alignments_rc.png");
    QTest::newRow("CC") << int(Qt::AlignHCenter) << int(Qt::AlignVCenter) << testFile("alignments_cc.png");
}


void tst_qquicktext::alignments()
{
    QSKIP("Text alignment pixmap comparison tests will not work with scenegraph");
#if (0)// No widgets in scenegraph
    QFETCH(int, hAlign);
    QFETCH(int, vAlign);
    QFETCH(QString, expectfile);

    QQuickView *canvas = createView(testFile("alignments.qml"));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWait(50);
    QTRY_COMPARE(QGuiApplication::activeWindow(), static_cast<QWidget *>(canvas));

    QObject *ob = canvas->rootObject();
    QVERIFY(ob != 0);
    ob->setProperty("horizontalAlignment",hAlign);
    ob->setProperty("verticalAlignment",vAlign);
    QTRY_COMPARE(ob->property("running").toBool(),false);
    QImage actual(canvas->width(), canvas->height(), QImage::Format_RGB32);
    actual.fill(qRgb(255,255,255));
    QPainter p(&actual);
    canvas->render(&p);

    QImage expect(expectfile);
    if (QGuiApplicationPrivate::graphics_system_name == "raster" || QGuiApplicationPrivate::graphics_system_name == "") {
        QCOMPARE(actual,expect);
    }
    delete canvas;
#endif
}

//the alignment tests may be trivial o.oa
void tst_qquicktext::horizontalAlignment()
{
    //test one align each, and then test if two align fails.

    for (int i = 0; i < standard.size(); i++)
    {
        for (int j=0; j < horizontalAlignmentmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nText { horizontalAlignment: \"" + horizontalAlignmentmentStrings.at(j) + "\"; text: \"" + standard.at(i) + "\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

            QCOMPARE((int)textObject->hAlign(), (int)horizontalAlignmentments.at(j));

            delete textObject;
        }
    }

    for (int i = 0; i < richText.size(); i++)
    {
        for (int j=0; j < horizontalAlignmentmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nText { horizontalAlignment: \"" + horizontalAlignmentmentStrings.at(j) + "\"; text: \"" + richText.at(i) + "\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

            QCOMPARE((int)textObject->hAlign(), (int)horizontalAlignmentments.at(j));

            delete textObject;
        }
    }

}

void tst_qquicktext::horizontalAlignment_RightToLeft()
{
    QQuickView *canvas = createView(testFile("horizontalAlignment_RightToLeft.qml"));
    QQuickText *text = canvas->rootObject()->findChild<QQuickText*>("text");
    QVERIFY(text != 0);
    canvas->show();

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(text);
    QVERIFY(textPrivate != 0);

    QTRY_VERIFY(textPrivate->layout.lineCount());

    // implicit alignment should follow the reading direction of RTL text
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() > canvas->width()/2);

    // explicitly left aligned text
    text->setHAlign(QQuickText::AlignLeft);
    QCOMPARE(text->hAlign(), QQuickText::AlignLeft);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() < canvas->width()/2);

    // explicitly right aligned text
    text->setHAlign(QQuickText::AlignRight);
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() > canvas->width()/2);

    // change to rich text
    QString textString = text->text();
    text->setText(QString("<i>") + textString + QString("</i>"));
    text->setTextFormat(QQuickText::RichText);
    text->resetHAlign();

    // implicitly aligned rich text should follow the reading direction of text
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->textDocument()->defaultTextOption().alignment() & Qt::AlignLeft);

    // explicitly left aligned rich text
    text->setHAlign(QQuickText::AlignLeft);
    QCOMPARE(text->hAlign(), QQuickText::AlignLeft);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->textDocument()->defaultTextOption().alignment() & Qt::AlignRight);

    // explicitly right aligned rich text
    text->setHAlign(QQuickText::AlignRight);
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->textDocument()->defaultTextOption().alignment() & Qt::AlignLeft);

    text->setText(textString);
    text->setTextFormat(QQuickText::PlainText);

    // explicitly center aligned
    text->setHAlign(QQuickText::AlignHCenter);
    QCOMPARE(text->hAlign(), QQuickText::AlignHCenter);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() < canvas->width()/2);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().right() > canvas->width()/2);

    // reseted alignment should go back to following the text reading direction
    text->resetHAlign();
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() > canvas->width()/2);

    // mirror the text item
    QQuickItemPrivate::get(text)->setLayoutMirror(true);

    // mirrored implicit alignment should continue to follow the reading direction of the text
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), QQuickText::AlignRight);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() > canvas->width()/2);

    // mirrored explicitly right aligned behaves as left aligned
    text->setHAlign(QQuickText::AlignRight);
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), QQuickText::AlignLeft);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() < canvas->width()/2);

    // mirrored explicitly left aligned behaves as right aligned
    text->setHAlign(QQuickText::AlignLeft);
    QCOMPARE(text->hAlign(), QQuickText::AlignLeft);
    QCOMPARE(text->effectiveHAlign(), QQuickText::AlignRight);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() > canvas->width()/2);

    // disable mirroring
    QQuickItemPrivate::get(text)->setLayoutMirror(false);
    text->resetHAlign();

    // English text should be implicitly left aligned
    text->setText("Hello world!");
    QCOMPARE(text->hAlign(), QQuickText::AlignLeft);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() < canvas->width()/2);

    // empty text with implicit alignment follows the system locale-based
    // keyboard input direction from QInputPanel::inputDirection()
    text->setText("");
    QCOMPARE(text->hAlign(), qApp->inputPanel()->inputDirection() == Qt::LeftToRight ?
                                  QQuickText::AlignLeft : QQuickText::AlignRight);
    text->setHAlign(QQuickText::AlignRight);
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);

    delete canvas;

    // alignment of Text with no text set to it
    QString componentStr = "import QtQuick 2.0\nText {}";
    QDeclarativeComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());
    QCOMPARE(textObject->hAlign(), qApp->inputPanel()->inputDirection() == Qt::LeftToRight ?
                                  QQuickText::AlignLeft : QQuickText::AlignRight);
    delete textObject;
}

void tst_qquicktext::verticalAlignment()
{
    //test one align each, and then test if two align fails.

    for (int i = 0; i < standard.size(); i++)
    {
        for (int j=0; j < verticalAlignmentmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nText { verticalAlignment: \"" + verticalAlignmentmentStrings.at(j) + "\"; text: \"" + standard.at(i) + "\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

            QVERIFY(textObject != 0);
            QCOMPARE((int)textObject->vAlign(), (int)verticalAlignmentments.at(j));

            delete textObject;
        }
    }

    for (int i = 0; i < richText.size(); i++)
    {
        for (int j=0; j < verticalAlignmentmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nText { verticalAlignment: \"" + verticalAlignmentmentStrings.at(j) + "\"; text: \"" + richText.at(i) + "\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

            QVERIFY(textObject != 0);
            QCOMPARE((int)textObject->vAlign(), (int)verticalAlignmentments.at(j));

            delete textObject;
        }
    }

}

void tst_qquicktext::font()
{
    //test size, then bold, then italic, then family
    {
        QString componentStr = "import QtQuick 2.0\nText { font.pointSize: 40; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QCOMPARE(textObject->font().pointSize(), 40);
        QCOMPARE(textObject->font().bold(), false);
        QCOMPARE(textObject->font().italic(), false);

        delete textObject;
    }

    {
        QString componentStr = "import QtQuick 2.0\nText { font.pixelSize: 40; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QCOMPARE(textObject->font().pixelSize(), 40);
        QCOMPARE(textObject->font().bold(), false);
        QCOMPARE(textObject->font().italic(), false);

        delete textObject;
    }

    {
        QString componentStr = "import QtQuick 2.0\nText { font.bold: true; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QCOMPARE(textObject->font().bold(), true);
        QCOMPARE(textObject->font().italic(), false);

        delete textObject;
    }

    {
        QString componentStr = "import QtQuick 2.0\nText { font.italic: true; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QCOMPARE(textObject->font().italic(), true);
        QCOMPARE(textObject->font().bold(), false);

        delete textObject;
    }

    {
        QString componentStr = "import QtQuick 2.0\nText { font.family: \"Helvetica\"; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QCOMPARE(textObject->font().family(), QString("Helvetica"));
        QCOMPARE(textObject->font().bold(), false);
        QCOMPARE(textObject->font().italic(), false);

        delete textObject;
    }

    {
        QString componentStr = "import QtQuick 2.0\nText { font.family: \"\"; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QCOMPARE(textObject->font().family(), QString(""));

        delete textObject;
    }
}

void tst_qquicktext::style()
{
    //test style
    for (int i = 0; i < styles.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { style: \"" + styleStrings.at(i) + "\"; styleColor: \"white\"; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QCOMPARE((int)textObject->style(), (int)styles.at(i));
        QCOMPARE(textObject->styleColor(), QColor("white"));

        delete textObject;
    }
    QString componentStr = "import QtQuick 2.0\nText { text: \"Hello World\" }";
    QDeclarativeComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

    QRectF brPre = textObject->boundingRect();
    textObject->setStyle(QQuickText::Outline);
    QRectF brPost = textObject->boundingRect();

    QVERIFY(brPre.width() < brPost.width());
    QVERIFY(brPre.height() < brPost.height());

    delete textObject;
}

void tst_qquicktext::color()
{
    //test style
    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { color: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QCOMPARE(textObject->color(), QColor(colorStrings.at(i)));
        QCOMPARE(textObject->styleColor(), QColor());

        delete textObject;
    }

    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { styleColor: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QCOMPARE(textObject->styleColor(), QColor(colorStrings.at(i)));
        // default color to black?
        QCOMPARE(textObject->color(), QColor("black"));

        delete textObject;
    }

    for (int i = 0; i < colorStrings.size(); i++)
    {
        for (int j = 0; j < colorStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nText { color: \"" + colorStrings.at(i) + "\"; styleColor: \"" + colorStrings.at(j) + "\"; text: \"Hello World\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

            QCOMPARE(textObject->color(), QColor(colorStrings.at(i)));
            QCOMPARE(textObject->styleColor(), QColor(colorStrings.at(j)));

            delete textObject;
        }
    }
    {
        QString colorStr = "#AA001234";
        QColor testColor("#001234");
        testColor.setAlpha(170);

        QString componentStr = "import QtQuick 2.0\nText { color: \"" + colorStr + "\"; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QCOMPARE(textObject->color(), testColor);

        delete textObject;
    }
}

void tst_qquicktext::smooth()
{
    for (int i = 0; i < standard.size(); i++)
    {
        {
            QString componentStr = "import QtQuick 2.0\nText { smooth: true; text: \"" + standard.at(i) + "\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());
            QCOMPARE(textObject->smooth(), true);

            delete textObject;
        }
        {
            QString componentStr = "import QtQuick 2.0\nText { text: \"" + standard.at(i) + "\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());
            QCOMPARE(textObject->smooth(), false);

            delete textObject;
        }
    }
    for (int i = 0; i < richText.size(); i++)
    {
        {
            QString componentStr = "import QtQuick 2.0\nText { smooth: true; text: \"" + richText.at(i) + "\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());
            QCOMPARE(textObject->smooth(), true);

            delete textObject;
        }
        {
            QString componentStr = "import QtQuick 2.0\nText { text: \"" + richText.at(i) + "\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());
            QCOMPARE(textObject->smooth(), false);

            delete textObject;
        }
    }
}

void tst_qquicktext::weight()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE((int)textObject->font().weight(), (int)QDeclarativeFontValueType::Normal);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { font.weight: \"Bold\"; text: \"Hello world!\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE((int)textObject->font().weight(), (int)QDeclarativeFontValueType::Bold);

        delete textObject;
    }
}

void tst_qquicktext::underline()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().underline(), false);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { font.underline: true; text: \"Hello world!\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().underline(), true);

        delete textObject;
    }
}

void tst_qquicktext::overline()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().overline(), false);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { font.overline: true; text: \"Hello world!\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().overline(), true);

        delete textObject;
    }
}

void tst_qquicktext::strikeout()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().strikeOut(), false);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { font.strikeout: true; text: \"Hello world!\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().strikeOut(), true);

        delete textObject;
    }
}

void tst_qquicktext::capitalization()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE((int)textObject->font().capitalization(), (int)QDeclarativeFontValueType::MixedCase);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.capitalization: \"AllUppercase\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE((int)textObject->font().capitalization(), (int)QDeclarativeFontValueType::AllUppercase);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.capitalization: \"AllLowercase\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE((int)textObject->font().capitalization(), (int)QDeclarativeFontValueType::AllLowercase);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.capitalization: \"SmallCaps\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE((int)textObject->font().capitalization(), (int)QDeclarativeFontValueType::SmallCaps);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.capitalization: \"Capitalize\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE((int)textObject->font().capitalization(), (int)QDeclarativeFontValueType::Capitalize);

        delete textObject;
    }
}

void tst_qquicktext::letterSpacing()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().letterSpacing(), 0.0);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.letterSpacing: -2 }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().letterSpacing(), -2.);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.letterSpacing: 3 }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().letterSpacing(), 3.);

        delete textObject;
    }
}

void tst_qquicktext::wordSpacing()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().wordSpacing(), 0.0);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.wordSpacing: -50 }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().wordSpacing(), -50.);

        delete textObject;
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.wordSpacing: 200 }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->font().wordSpacing(), 200.);

        delete textObject;
    }
}




class EventSender : public QQuickItem
{
public:
    void sendEvent(QMouseEvent *event) {
        if (event->type() == QEvent::MouseButtonPress)
            mousePressEvent(event);
        else if (event->type() == QEvent::MouseButtonRelease)
            mouseReleaseEvent(event);
        else
            qWarning() << "Trying to send unsupported event type";
    }
};

class LinkTest : public QObject
{
    Q_OBJECT
public:
    LinkTest() {}

    QString link;

public slots:
    void linkClicked(QString l) { link = l; }
};

void tst_qquicktext::clickLink()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"<a href=\\\"http://qt.nokia.com\\\">Hello world!</a>\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QVERIFY(textObject != 0);

        LinkTest test;
        QObject::connect(textObject, SIGNAL(linkActivated(QString)), &test, SLOT(linkClicked(QString)));

        {
            QMouseEvent me(QEvent::MouseButtonPress,QPointF(textObject->x()/2, textObject->y()/2), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            static_cast<EventSender*>(static_cast<QQuickItem*>(textObject))->sendEvent(&me);

        }

        {
            QMouseEvent me(QEvent::MouseButtonRelease,QPointF(textObject->x()/2, textObject->y()/2), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            static_cast<EventSender*>(static_cast<QQuickItem*>(textObject))->sendEvent(&me);

        }


        QCOMPARE(test.link, QLatin1String("http://qt.nokia.com"));

        delete textObject;
    }
}

void tst_qquicktext::baseUrl()
{
    QUrl localUrl("file:///tests/text.qml");
    QUrl remoteUrl("http://qt.nokia.com/test.qml");

    QDeclarativeComponent textComponent(&engine);
    textComponent.setData("import QtQuick 2.0\n Text {}", localUrl);
    QQuickText *textObject = qobject_cast<QQuickText *>(textComponent.create());

    QCOMPARE(textObject->baseUrl(), localUrl);

    QSignalSpy spy(textObject, SIGNAL(baseUrlChanged()));

    textObject->setBaseUrl(localUrl);
    QCOMPARE(textObject->baseUrl(), localUrl);
    QCOMPARE(spy.count(), 0);

    textObject->setBaseUrl(remoteUrl);
    QCOMPARE(textObject->baseUrl(), remoteUrl);
    QCOMPARE(spy.count(), 1);

    textObject->resetBaseUrl();
    QCOMPARE(textObject->baseUrl(), localUrl);
    QCOMPARE(spy.count(), 2);
}

void tst_qquicktext::embeddedImages_data()
{
    QTest::addColumn<QUrl>("qmlfile");
    QTest::addColumn<QString>("error");
    QTest::newRow("local") << testFileUrl("embeddedImagesLocal.qml") << "";
    QTest::newRow("local-error") << testFileUrl("embeddedImagesLocalError.qml")
        << testFileUrl("embeddedImagesLocalError.qml").toString()+":3:1: QML Text: Cannot open: " + testFileUrl("http/notexists.png").toString();
    QTest::newRow("local") << testFileUrl("embeddedImagesLocalRelative.qml") << "";
    QTest::newRow("remote") << testFileUrl("embeddedImagesRemote.qml") << "";
    QTest::newRow("remote-error") << testFileUrl("embeddedImagesRemoteError.qml")
        << testFileUrl("embeddedImagesRemoteError.qml").toString()+":3:1: QML Text: Error downloading http://127.0.0.1:14453/notexists.png - server replied: Not found";
    QTest::newRow("remote") << testFileUrl("embeddedImagesRemoteRelative.qml") << "";
}

void tst_qquicktext::embeddedImages()
{
    // Tests QTBUG-9900

    QFETCH(QUrl, qmlfile);
    QFETCH(QString, error);

    TestHTTPServer server(14453);
    server.serveDirectory(testFile("http"));

    if (!error.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, error.toLatin1());

    QDeclarativeComponent textComponent(&engine, qmlfile);
    QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

    QVERIFY(textObject != 0);

    QTRY_COMPARE(textObject->resourcesLoading(), 0);

    QPixmap pm(testFile("http/exists.png"));
    if (error.isEmpty()) {
        QCOMPARE(textObject->width(), double(pm.width()));
        QCOMPARE(textObject->height(), double(pm.height()));
    } else {
        QVERIFY(16 != pm.width()); // check test is effective
        QCOMPARE(textObject->width(), 16.0); // default size of QTextDocument broken image icon
        QCOMPARE(textObject->height(), 16.0);
    }

    delete textObject;
}

void tst_qquicktext::lineCount()
{
    QQuickView *canvas = createView(testFile("lineCount.qml"));

    QQuickText *myText = canvas->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != 0);

    QVERIFY(myText->lineCount() > 1);
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->maximumLineCount(), INT_MAX);

    myText->setMaximumLineCount(2);
    QCOMPARE(myText->lineCount(), 2);
    QCOMPARE(myText->truncated(), true);
    QCOMPARE(myText->maximumLineCount(), 2);

    myText->resetMaximumLineCount();
    QCOMPARE(myText->maximumLineCount(), INT_MAX);
    QCOMPARE(myText->truncated(), false);

    myText->setElideMode(QQuickText::ElideRight);
    myText->setMaximumLineCount(2);
    QCOMPARE(myText->lineCount(), 2);
    QCOMPARE(myText->truncated(), true);
    QCOMPARE(myText->maximumLineCount(), 2);

    delete canvas;
}

void tst_qquicktext::lineHeight()
{
    QQuickView *canvas = createView(testFile("lineHeight.qml"));

    QQuickText *myText = canvas->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != 0);

    QVERIFY(myText->lineHeight() == 1);
    QVERIFY(myText->lineHeightMode() == QQuickText::ProportionalHeight);

    qreal h = myText->height();
    myText->setLineHeight(1.5);
    QVERIFY(myText->height() == qCeil(h * 1.5));

    myText->setLineHeightMode(QQuickText::FixedHeight);
    myText->setLineHeight(20);
    QCOMPARE(myText->height(), myText->lineCount() * 20.0);

    myText->setText("Lorem ipsum sit <b>amet</b>, consectetur adipiscing elit. Integer felis nisl, varius in pretium nec, venenatis non erat. Proin lobortis interdum dictum.");
    myText->setLineHeightMode(QQuickText::ProportionalHeight);
    myText->setLineHeight(1.0);

    qreal h2 = myText->height();
    myText->setLineHeight(2.0);
    QVERIFY(myText->height() == h2 * 2.0);

    myText->setLineHeightMode(QQuickText::FixedHeight);
    myText->setLineHeight(10);
    QCOMPARE(myText->height(), myText->lineCount() * 10.0);

    delete canvas;
}

void tst_qquicktext::implicitSize_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("wrap");
    QTest::newRow("plain") << "The quick red fox jumped over the lazy brown dog" << "Text.NoWrap";
    QTest::newRow("richtext") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "Text.NoWrap";
    QTest::newRow("plain_wrap") << "The quick red fox jumped over the lazy brown dog" << "Text.Wrap";
    QTest::newRow("richtext_wrap") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "Text.Wrap";
}

void tst_qquicktext::implicitSize()
{
    QFETCH(QString, text);
    QFETCH(QString, wrap);
    QString componentStr = "import QtQuick 2.0\nText { text: \"" + text + "\"; width: 50; wrapMode: " + wrap + " }";
    QDeclarativeComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

    QVERIFY(textObject->width() < textObject->implicitWidth());
    QVERIFY(textObject->height() == textObject->implicitHeight());

    textObject->resetWidth();
    QVERIFY(textObject->width() == textObject->implicitWidth());
    QVERIFY(textObject->height() == textObject->implicitHeight());

    delete textObject;
}

void tst_qquicktext::lineLaidOut()
{
    QQuickView *canvas = createView(testFile("lineLayout.qml"));

    QQuickText *myText = canvas->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != 0);

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(myText);
    QVERIFY(textPrivate != 0);

    QTextDocument *doc = textPrivate->textDocument();
    QVERIFY(doc == 0);

#if defined(Q_OS_MAC)
    QVERIFY(myText->lineCount() == textPrivate->linesRects.count());
#endif

    for (int i = 0; i < textPrivate->layout.lineCount(); ++i) {
        QRectF r = textPrivate->layout.lineAt(i).rect();
        QVERIFY(r.width() == i * 15);
        if (i >= 30)
            QVERIFY(r.x() == r.width() + 30);
        if (i >= 60) {
            QVERIFY(r.x() == r.width() * 2 + 60);
            QVERIFY(r.height() == 20);
        }
    }

    delete canvas;
}

QTEST_MAIN(tst_qquicktext)

#include "tst_qquicktext.moc"
