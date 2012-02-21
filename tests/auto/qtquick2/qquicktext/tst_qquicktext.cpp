/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
    void contentSize();

    void lineLaidOut();

    void imgTagsBaseUrl_data();
    void imgTagsBaseUrl();
    void imgTagsAlign_data();
    void imgTagsAlign();
    void imgTagsMultipleImages();
    void imgTagsElide();
    void imgTagsUpdates();
    void imgTagsError();
    void fontSizeMode_data();
    void fontSizeMode();
    void fontSizeModeMultiline_data();
    void fontSizeModeMultiline();
    void multilengthStrings_data();
    void multilengthStrings();
    void fontFormatSizes_data();
    void fontFormatSizes();

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

    qreal lineHeight = myText->contentHeight() / 3.;

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
    // keyboard input direction from QInputMethod::inputDirection()
    text->setText("");
    QCOMPARE(text->hAlign(), qApp->inputMethod()->inputDirection() == Qt::LeftToRight ?
                                  QQuickText::AlignLeft : QQuickText::AlignRight);
    text->setHAlign(QQuickText::AlignRight);
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);

    delete canvas;

    // alignment of Text with no text set to it
    QString componentStr = "import QtQuick 2.0\nText {}";
    QDeclarativeComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());
    QCOMPARE(textObject->hAlign(), qApp->inputMethod()->inputDirection() == Qt::LeftToRight ?
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
        QCOMPARE(textObject->styleColor(), QColor("black"));
        QCOMPARE(textObject->linkColor(), QColor("blue"));

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
        QCOMPARE(textObject->linkColor(), QColor("blue"));

        delete textObject;
    }

    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { linkColor: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

        QCOMPARE(textObject->styleColor(), QColor("black"));
        QCOMPARE(textObject->color(), QColor("black"));
        QCOMPARE(textObject->linkColor(), QColor(colorStrings.at(i)));

        delete textObject;
    }

    for (int i = 0; i < colorStrings.size(); i++)
    {
        for (int j = 0; j < colorStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nText { "
                    "color: \"" + colorStrings.at(i) + "\"; "
                    "styleColor: \"" + colorStrings.at(j) + "\"; "
                    "linkColor: \"" + colorStrings.at(j) + "\"; "
                    "text: \"Hello World\" }";
            QDeclarativeComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

            QCOMPARE(textObject->color(), QColor(colorStrings.at(i)));
            QCOMPARE(textObject->styleColor(), QColor(colorStrings.at(j)));
            QCOMPARE(textObject->linkColor(), QColor(colorStrings.at(j)));

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
    } {
        QString colorStr = "#001234";
        QColor testColor(colorStr);

        QString componentStr = "import QtQuick 2.0\nText { color: \"" + colorStr + "\"; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QSignalSpy spy(textObject, SIGNAL(colorChanged()));

        QCOMPARE(textObject->color(), testColor);
        textObject->setColor(testColor);
        QCOMPARE(textObject->color(), testColor);
        QCOMPARE(spy.count(), 0);

        testColor = QColor("black");
        textObject->setColor(testColor);
        QCOMPARE(textObject->color(), testColor);
        QCOMPARE(spy.count(), 1);
    } {
        QString colorStr = "#001234";
        QColor testColor(colorStr);

        QString componentStr = "import QtQuick 2.0\nText { styleColor: \"" + colorStr + "\"; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QSignalSpy spy(textObject, SIGNAL(styleColorChanged()));

        QCOMPARE(textObject->styleColor(), testColor);
        textObject->setStyleColor(testColor);
        QCOMPARE(textObject->styleColor(), testColor);
        QCOMPARE(spy.count(), 0);

        testColor = QColor("black");
        textObject->setStyleColor(testColor);
        QCOMPARE(textObject->styleColor(), testColor);
        QCOMPARE(spy.count(), 1);
    } {
        QString colorStr = "#001234";
        QColor testColor(colorStr);

        QString componentStr = "import QtQuick 2.0\nText { linkColor: \"" + colorStr + "\"; text: \"Hello World\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QSignalSpy spy(textObject, SIGNAL(linkColorChanged()));

        QCOMPARE(textObject->linkColor(), testColor);
        textObject->setLinkColor(testColor);
        QCOMPARE(textObject->linkColor(), testColor);
        QCOMPARE(spy.count(), 0);

        testColor = QColor("black");
        textObject->setLinkColor(testColor);
        QCOMPARE(textObject->linkColor(), testColor);
        QCOMPARE(spy.count(), 1);
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
    QCOMPARE(myText->height(), qreal(qCeil(h * 1.5)));

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
    QTest::addColumn<QString>("width");
    QTest::addColumn<QString>("wrap");
    QTest::addColumn<QString>("elide");
    QTest::newRow("plain") << "The quick red fox jumped over the lazy brown dog" << "50" << "Text.NoWrap" << "Text.ElideNone";
    QTest::newRow("richtext") << "<b>The quick red fox jumped over the lazy brown dog</b>" <<" 50" << "Text.NoWrap" << "Text.ElideNone";
    QTest::newRow("plain, 0 width") << "The quick red fox jumped over the lazy brown dog" << "0" << "Text.NoWrap" << "Text.ElideNone";
    QTest::newRow("plain, elide") << "The quick red fox jumped over the lazy brown dog" << "50" << "Text.NoWrap" << "Text.ElideRight";
    QTest::newRow("plain, 0 width, elide") << "The quick red fox jumped over the lazy brown dog" << "0" << "Text.NoWrap" << "Text.ElideRight";
    QTest::newRow("richtext, 0 width") << "<b>The quick red fox jumped over the lazy brown dog</b>" <<" 0" << "Text.NoWrap" << "Text.ElideNone";
    QTest::newRow("plain_wrap") << "The quick red fox jumped over the lazy brown dog" << "50" << "Text.Wrap" << "Text.ElideNone";
    QTest::newRow("richtext_wrap") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "50" << "Text.Wrap" << "Text.ElideNone";
    QTest::newRow("plain_wrap, 0 width") << "The quick red fox jumped over the lazy brown dog" << "0" << "Text.Wrap" << "Text.ElideNone";
    QTest::newRow("plain_wrap, elide") << "The quick red fox jumped over the lazy brown dog" << "50" << "Text.Wrap" << "Text.ElideRight";
    QTest::newRow("plain_wrap, 0 width, elide") << "The quick red fox jumped over the lazy brown dog" << "0" << "Text.Wrap" << "Text.ElideRight";
    QTest::newRow("richtext_wrap, 0 width") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "0" << "Text.Wrap" << "Text.ElideNone";
}

void tst_qquicktext::implicitSize()
{
    QFETCH(QString, text);
    QFETCH(QString, width);
    QFETCH(QString, wrap);
    QFETCH(QString, elide);
    QString componentStr = "import QtQuick 2.0\nText { "
            "text: \"" + text + "\"; "
            "width: " + width + "; "
            "wrapMode: " + wrap + "; "
            "elide: " + elide + "; "
            "maximumLineCount: 1 }";
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

void tst_qquicktext::contentSize()
{
    QString componentStr = "import QtQuick 2.0\nText { width: 75; height: 16; font.pixelSize: 10 }";
    QDeclarativeComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickText *textObject = qobject_cast<QQuickText *>(object.data());

    QSignalSpy spy(textObject, SIGNAL(contentSizeChanged()));

    textObject->setText("The quick red fox jumped over the lazy brown dog");

    QVERIFY(textObject->contentWidth() > textObject->width());
    QVERIFY(textObject->contentHeight() < textObject->height());
    QCOMPARE(spy.count(), 1);

    textObject->setWrapMode(QQuickText::WordWrap);
    QVERIFY(textObject->contentWidth() <= textObject->width());
    QVERIFY(textObject->contentHeight() > textObject->height());
    QCOMPARE(spy.count(), 2);

    textObject->setElideMode(QQuickText::ElideRight);
    QVERIFY(textObject->contentWidth() <= textObject->width());
    QVERIFY(textObject->contentHeight() < textObject->height());
    QCOMPARE(spy.count(), 3);
    int spyCount = 3;
    qreal elidedWidth = textObject->contentWidth();

    textObject->setText("The quickredfoxjumpedoverthe lazy brown dog");
    QVERIFY(textObject->contentWidth() <= textObject->width());
    QVERIFY(textObject->contentHeight() < textObject->height());
    // this text probably won't have the same elided width, but it's not guaranteed.
    if (textObject->contentWidth() != elidedWidth)
        QCOMPARE(spy.count(), ++spyCount);
    else
        QCOMPARE(spy.count(), spyCount);

    textObject->setElideMode(QQuickText::ElideNone);
    QVERIFY(textObject->contentWidth() > textObject->width());
    QVERIFY(textObject->contentHeight() > textObject->height());
    QCOMPARE(spy.count(), ++spyCount);
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

void tst_qquicktext::imgTagsBaseUrl_data()
{
    QTest::addColumn<QUrl>("src");
    QTest::addColumn<QUrl>("baseUrl");
    QTest::addColumn<QUrl>("contextUrl");
    QTest::addColumn<qreal>("imgHeight");

    QTest::newRow("absolute local")
            << testFileUrl("images/heart200.png")
            << QUrl()
            << QUrl()
            << 181.;
    QTest::newRow("relative local context 1")
            << QUrl("images/heart200.png")
            << QUrl()
            << testFileUrl("/app.qml")
            << 181.;
    QTest::newRow("relative local context 2")
            << QUrl("heart200.png")
            << QUrl()
            << testFileUrl("images/app.qml")
            << 181.;
    QTest::newRow("relative local base 1")
            << QUrl("images/heart200.png")
            << testFileUrl("")
            << testFileUrl("nonexistant/app.qml")
            << 181.;
    QTest::newRow("relative local base 2")
            << QUrl("heart200.png")
            << testFileUrl("images/")
            << testFileUrl("nonexistant/app.qml")
            << 181.;
    QTest::newRow("base relative to local context")
            << QUrl("heart200.png")
            << testFileUrl("images/")
            << testFileUrl("/app.qml")
            << 181.;

    QTest::newRow("absolute remote")
            << QUrl("http://127.0.0.1:14453/images/heart200.png")
            << QUrl()
            << QUrl()
            << 181.;
    QTest::newRow("relative remote base 1")
            << QUrl("images/heart200.png")
            << QUrl("http://127.0.0.1:14453/")
            << testFileUrl("nonexistant/app.qml")
            << 181.;
    QTest::newRow("relative remote base 2")
            << QUrl("heart200.png")
            << QUrl("http://127.0.0.1:14453/images/")
            << testFileUrl("nonexistant/app.qml")
            << 181.;
}

void tst_qquicktext::imgTagsBaseUrl()
{
    QFETCH(QUrl, src);
    QFETCH(QUrl, baseUrl);
    QFETCH(QUrl, contextUrl);
    QFETCH(qreal, imgHeight);

    TestHTTPServer server(14453);
    server.serveDirectory(testFile(""));

    QByteArray baseUrlFragment;
    if (!baseUrl.isEmpty())
        baseUrlFragment = "; baseUrl: \"" + baseUrl.toEncoded() + "\"";
    QByteArray componentStr = "import QtQuick 2.0\nText { text: \"This is a test <img src=\\\"" + src.toEncoded() + "\\\">\"" + baseUrlFragment + " }";

    QDeclarativeComponent component(&engine);
    component.setData(componentStr, contextUrl);
    QScopedPointer<QObject> object(component.create());
    QQuickText *textObject = qobject_cast<QQuickText *>(object.data());
    QVERIFY(textObject);

    QCoreApplication::processEvents();

    QTRY_COMPARE(textObject->height(), imgHeight);
}

void tst_qquicktext::imgTagsAlign_data()
{
    QTest::addColumn<QString>("src");
    QTest::addColumn<int>("imgHeight");
    QTest::addColumn<QString>("align");
    QTest::newRow("heart-bottom") << "data/images/heart200.png" << 181 <<  "bottom";
    QTest::newRow("heart-middle") << "data/images/heart200.png" << 181 <<  "middle";
    QTest::newRow("heart-top") << "data/images/heart200.png" << 181 <<  "top";
    QTest::newRow("starfish-bottom") << "data/images/starfish_2.png" << 217 <<  "bottom";
    QTest::newRow("starfish-middle") << "data/images/starfish_2.png" << 217 <<  "middle";
    QTest::newRow("starfish-top") << "data/images/starfish_2.png" << 217 <<  "top";
}

void tst_qquicktext::imgTagsAlign()
{
    QFETCH(QString, src);
    QFETCH(int, imgHeight);
    QFETCH(QString, align);
    QString componentStr = "import QtQuick 2.0\nText { text: \"This is a test <img src=\\\"" + src + "\\\" align=\\\"" + align + "\\\"> of image.\" }";
    QDeclarativeComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

    QVERIFY(textObject != 0);
    QVERIFY(textObject->height() == imgHeight);

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
    QVERIFY(textPrivate != 0);

    QRectF br = textPrivate->layout.boundingRect();
    if (align == "bottom")
        QVERIFY(br.y() == imgHeight - br.height());
    else if (align == "middle")
        QVERIFY(br.y() == imgHeight / 2.0 - br.height() / 2.0);
    else if (align == "top")
        QVERIFY(br.y() == 0);

    delete textObject;
}

void tst_qquicktext::imgTagsMultipleImages()
{
    QString componentStr = "import QtQuick 2.0\nText { text: \"This is a starfish<img src=\\\"data/images/starfish_2.png\\\" width=\\\"60\\\" height=\\\"60\\\" > and another one<img src=\\\"data/images/heart200.png\\\" width=\\\"85\\\" height=\\\"85\\\">.\" }";

    QDeclarativeComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

    QVERIFY(textObject != 0);
    QVERIFY(textObject->height() == 85);

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
    QVERIFY(textPrivate != 0);
    QVERIFY(textPrivate->visibleImgTags.count() == 2);

    delete textObject;
}

void tst_qquicktext::imgTagsElide()
{
    QQuickView *canvas = createView(testFile("imgTagsElide.qml"));
    QQuickText *myText = canvas->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != 0);

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(myText);
    QVERIFY(textPrivate != 0);
    QVERIFY(textPrivate->visibleImgTags.count() == 0);
    myText->setMaximumLineCount(20);
    QTRY_VERIFY(textPrivate->visibleImgTags.count() == 1);

    delete myText;
    delete canvas;
}

void tst_qquicktext::imgTagsUpdates()
{
    QQuickView *canvas = createView(testFile("imgTagsUpdates.qml"));
    QQuickText *myText = canvas->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != 0);

    QSignalSpy spy(myText, SIGNAL(contentSizeChanged()));

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(myText);
    QVERIFY(textPrivate != 0);

    myText->setText("This is a heart<img src=\"images/heart200.png\">.");
    QVERIFY(textPrivate->visibleImgTags.count() == 1);
    QVERIFY(spy.count() == 1);

    myText->setMaximumLineCount(2);
    myText->setText("This is another heart<img src=\"images/heart200.png\">.");
    QTRY_VERIFY(textPrivate->visibleImgTags.count() == 1);

    // if maximumLineCount is set and the img tag doesn't have an explicit size
    // we relayout twice.
    QVERIFY(spy.count() == 3);

    delete myText;
    delete canvas;
}

void tst_qquicktext::imgTagsError()
{
    QString componentStr = "import QtQuick 2.0\nText { text: \"This is a starfish<img src=\\\"data/images/starfish_2.pn\\\" width=\\\"60\\\" height=\\\"60\\\">.\" }";

    QDeclarativeComponent textComponent(&engine);
    QTest::ignoreMessage(QtWarningMsg, "file::2:1: QML Text: Cannot open: file:data/images/starfish_2.pn");
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

    QVERIFY(textObject != 0);
    delete textObject;
}

void tst_qquicktext::fontSizeMode_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("canElide");
    QTest::newRow("plain") << "The quick red fox jumped over the lazy brown dog" << true;
    QTest::newRow("richtext") << "<b>The quick red fox jumped over the lazy brown dog</b>" << false;
}

void tst_qquicktext::fontSizeMode()
{
    QFETCH(QString, text);
    QFETCH(bool, canElide);

    QQuickView *canvas = createView(testFile("fontSizeMode.qml"));
    canvas->show();

    QQuickText *myText = canvas->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != 0);

    myText->setText(text);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);

    qreal originalWidth = myText->contentWidth();
    qreal originalHeight = myText->contentHeight();

    // The original text unwrapped should exceed the width of the item.
    QVERIFY(originalWidth > myText->width());
    QVERIFY(originalHeight < myText->height());

    QFont font = myText->font();
    font.setPixelSize(64);

    myText->setFont(font);
    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // Font size reduced to fit within the width of the item.
    qreal horizontalFitWidth = myText->contentWidth();
    qreal horizontalFitHeight = myText->contentHeight();
    QVERIFY(horizontalFitWidth <= myText->width() + 2); // rounding
    QVERIFY(horizontalFitHeight <= myText->height() + 2);

    if (canElide) {
        // Elide won't affect the size with HorizontalFit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), horizontalFitWidth);
        QCOMPARE(myText->contentHeight(), horizontalFitHeight);

        myText->setElideMode(QQuickText::ElideLeft);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), horizontalFitWidth);
        QCOMPARE(myText->contentHeight(), horizontalFitHeight);

        myText->setElideMode(QQuickText::ElideMiddle);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), horizontalFitWidth);
        QCOMPARE(myText->contentHeight(), horizontalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // Font size increased to fill the height of the item.
    qreal verticalFitHeight = myText->contentHeight();
    QVERIFY(myText->contentWidth() > myText->width());
    QVERIFY(verticalFitHeight <= myText->height() + 2);
    QVERIFY(verticalFitHeight > originalHeight);

    if (canElide) {
        // Elide won't affect the height of a single line with VerticalFit but will crop the width.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(myText->truncated());
        QVERIFY(myText->contentWidth() <= myText->width() + 2);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideLeft);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(myText->truncated());
        QVERIFY(myText->contentWidth() <= myText->width() + 2);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideMiddle);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(myText->truncated());
        QVERIFY(myText->contentWidth() <= myText->width() + 2);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::Fit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // Should be the same as HorizontalFit with no wrapping.
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    if (canElide) {
        // Elide won't affect the size with Fit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), horizontalFitWidth);
        QCOMPARE(myText->contentHeight(), horizontalFitHeight);

        myText->setElideMode(QQuickText::ElideLeft);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), horizontalFitWidth);
        QCOMPARE(myText->contentHeight(), horizontalFitHeight);

        myText->setElideMode(QQuickText::ElideMiddle);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), horizontalFitWidth);
        QCOMPARE(myText->contentHeight(), horizontalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::FixedSize);
    myText->setWrapMode(QQuickText::Wrap);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);

    originalWidth = myText->contentWidth();
    originalHeight = myText->contentHeight();

    // The original text wrapped should exceed the height of the item.
    QVERIFY(originalWidth <= myText->width() + 2);
    QVERIFY(originalHeight > myText->height());

    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // HorizontalFit should reduce the font size to minimize wrapping, which brings it back to the
    // same size as without text wrapping.
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    if (canElide) {
        // Elide won't affect the size with HorizontalFit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), horizontalFitWidth);
        QCOMPARE(myText->contentHeight(), horizontalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // VerticalFit should reduce the size to the wrapped text within the vertical height.
    verticalFitHeight = myText->contentHeight();
    qreal verticalFitWidth = myText->contentWidth();
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QVERIFY(verticalFitHeight <= myText->height() + 2);
    QVERIFY(verticalFitHeight < originalHeight);

    if (canElide) {
        // Elide won't affect the height or width of a wrapped text with VerticalFit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::Fit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // Should be the same as VerticalFit with wrapping.
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    if (canElide) {
        // Elide won't affect the size with Fit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::FixedSize);
    myText->setMaximumLineCount(2);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);

    // The original text wrapped should exceed the height of the item.
    QVERIFY(originalWidth <= myText->width() + 2);
    QVERIFY(originalHeight > myText->height());

    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // HorizontalFit should reduce the font size to minimize wrapping, which brings it back to the
    // same size as without text wrapping.
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    if (canElide) {
        // Elide won't affect the size with HorizontalFit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), horizontalFitWidth);
        QCOMPARE(myText->contentHeight(), horizontalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // VerticalFit should reduce the size to the wrapped text within the vertical height.
    verticalFitHeight = myText->contentHeight();
    verticalFitWidth = myText->contentWidth();
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QVERIFY(verticalFitHeight <= myText->height() + 2);
    QVERIFY(verticalFitHeight < originalHeight);

    if (canElide) {
        // Elide won't affect the height or width of a wrapped text with VerticalFit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::Fit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // Should be the same as VerticalFit with wrapping.
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    if (canElide) {
        // Elide won't affect the size with Fit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }
}

void tst_qquicktext::fontSizeModeMultiline_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("canElide");
    QTest::newRow("plain") << "The quick red fox jumped\n over the lazy brown dog" << true;
    QTest::newRow("richtext") << "<b>The quick red fox jumped<br/> over the lazy brown dog</b>" << false;
}

void tst_qquicktext::fontSizeModeMultiline()
{
    QFETCH(QString, text);
    QFETCH(bool, canElide);

    QQuickView *canvas = createView(testFile("fontSizeMode.qml"));
    canvas->show();

    QQuickText *myText = canvas->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != 0);

    myText->setText(text);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);

    qreal originalWidth = myText->contentWidth();
    qreal originalHeight = myText->contentHeight();
    QCOMPARE(myText->lineCount(), 2);

    // The original text unwrapped should exceed the width and height of the item.
    QVERIFY(originalWidth > myText->width());
    QVERIFY(originalHeight > myText->height());

    QFont font = myText->font();
    font.setPixelSize(64);

    myText->setFont(font);
    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // Font size reduced to fit within the width of the item.
    QCOMPARE(myText->lineCount(), 2);
    qreal horizontalFitWidth = myText->contentWidth();
    qreal horizontalFitHeight = myText->contentHeight();
    QVERIFY(horizontalFitWidth <= myText->width() + 2); // rounding
    QVERIFY(horizontalFitHeight > myText->height());

    if (canElide) {
        // Right eliding will remove the last line
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(myText->truncated());
        QCOMPARE(myText->lineCount(), 1);
        QVERIFY(myText->contentWidth() <= myText->width() + 2);
        QVERIFY(myText->contentHeight() <= myText->height() + 2);

        // Left or middle eliding wont have any effect.
        myText->setElideMode(QQuickText::ElideLeft);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), horizontalFitWidth);
        QCOMPARE(myText->contentHeight(), horizontalFitHeight);

        myText->setElideMode(QQuickText::ElideMiddle);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), horizontalFitWidth);
        QCOMPARE(myText->contentHeight(), horizontalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // Font size reduced to fit within the height of the item.
    qreal verticalFitWidth = myText->contentWidth();
    qreal verticalFitHeight = myText->contentHeight();
    QVERIFY(verticalFitWidth <= myText->width() + 2);
    QVERIFY(verticalFitHeight <= myText->height() + 2);

    if (canElide) {
        // Elide will have no effect.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QVERIFY(myText->contentWidth() <= myText->width() + 2);
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideLeft);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideMiddle);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::Fit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // Should be the same as VerticalFit with no wrapping.
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    if (canElide) {
        // Elide won't affect the size with Fit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideLeft);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideMiddle);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::FixedSize);
    myText->setWrapMode(QQuickText::Wrap);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);

    originalWidth = myText->contentWidth();
    originalHeight = myText->contentHeight();

    // The original text wrapped should exceed the height of the item.
    QVERIFY(originalWidth <= myText->width() + 2);
    QVERIFY(originalHeight > myText->height());

    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // HorizontalFit should reduce the font size to minimize wrapping, which brings it back to the
    // same size as without text wrapping.
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    if (canElide) {
        // Text will be elided vertically with HorizontalFit
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(myText->truncated());
        QVERIFY(myText->contentWidth() <= myText->width() + 2);
        QVERIFY(myText->contentHeight() <= myText->height() + 2);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // VerticalFit should reduce the size to the wrapped text within the vertical height.
    verticalFitHeight = myText->contentHeight();
    verticalFitWidth = myText->contentWidth();
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QVERIFY(verticalFitHeight <= myText->height() + 2);
    QVERIFY(verticalFitHeight < originalHeight);

    if (canElide) {
        // Elide won't affect the height or width of a wrapped text with VerticalFit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::Fit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // Should be the same as VerticalFit with wrapping.
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    if (canElide) {
        // Elide won't affect the size with Fit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::FixedSize);
    myText->setMaximumLineCount(2);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);

    // The original text wrapped should exceed the height of the item.
    QVERIFY(originalWidth <= myText->width() + 2);
    QVERIFY(originalHeight > myText->height());

    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // HorizontalFit should reduce the font size to minimize wrapping, which brings it back to the
    // same size as without text wrapping.
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    if (canElide) {
        // Elide won't affect the size with HorizontalFit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(myText->truncated());
        QVERIFY(myText->contentWidth() <= myText->width() + 2);
        QVERIFY(myText->contentHeight() <= myText->height() + 2);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // VerticalFit should reduce the size to the wrapped text within the vertical height.
    verticalFitHeight = myText->contentHeight();
    verticalFitWidth = myText->contentWidth();
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QVERIFY(verticalFitHeight <= myText->height() + 2);
    QVERIFY(verticalFitHeight < originalHeight);

    if (canElide) {
        // Elide won't affect the height or width of a wrapped text with VerticalFit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }

    myText->setFontSizeMode(QQuickText::Fit);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    // Should be the same as VerticalFit with wrapping.
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    if (canElide) {
        // Elide won't affect the size with Fit.
        myText->setElideMode(QQuickText::ElideRight);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
        QVERIFY(!myText->truncated());
        QCOMPARE(myText->contentWidth(), verticalFitWidth);
        QCOMPARE(myText->contentHeight(), verticalFitHeight);

        myText->setElideMode(QQuickText::ElideNone);
        QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    }
}

void tst_qquicktext::multilengthStrings_data()
{
    QTest::addColumn<QString>("source");
    QTest::newRow("No Wrap") << testFile("multilengthStrings.qml");
    QTest::newRow("Wrap") << testFile("multilengthStringsWrapped.qml");
}

void tst_qquicktext::multilengthStrings()
{
    QFETCH(QString, source);

    QScopedPointer<QQuickView> canvas(createView(source));
    canvas->show();

    QQuickText *myText = canvas->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != 0);

    const QString longText = "the quick brown fox jumped over the lazy dog";
    const QString mediumText = "the brown fox jumped over the dog";
    const QString shortText = "fox jumped dog";

    myText->setText(longText);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    const qreal longWidth = myText->contentWidth();
    const qreal longHeight = myText->contentHeight();

    myText->setText(mediumText);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    const qreal mediumWidth = myText->contentWidth();
    const qreal mediumHeight = myText->contentHeight();

    myText->setText(shortText);
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);
    const qreal shortWidth = myText->contentWidth();
    const qreal shortHeight = myText->contentHeight();

    myText->setElideMode(QQuickText::ElideRight);
    myText->setText(longText + QLatin1Char('\x9c') + mediumText + QLatin1Char('\x9c') + shortText);

    myText->setSize(QSizeF(longWidth, longHeight));
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);

    QCOMPARE(myText->contentWidth(), longWidth);
    QCOMPARE(myText->contentHeight(), longHeight);
    QCOMPARE(myText->truncated(), false);

    myText->setSize(QSizeF(mediumWidth, mediumHeight));
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);

    QCOMPARE(myText->contentWidth(), mediumWidth);
    QCOMPARE(myText->contentHeight(), mediumHeight);
#ifdef Q_OS_MAC
    QEXPECT_FAIL("Wrap", "QTBUG-24310", Continue);
#endif
    QCOMPARE(myText->truncated(), true);

    myText->setSize(QSizeF(shortWidth, shortHeight));
    QTRY_COMPARE(QQuickItemPrivate::get(myText)->polishScheduled, false);

    QCOMPARE(myText->contentWidth(), shortWidth);
    QCOMPARE(myText->contentHeight(), shortHeight);
    QCOMPARE(myText->truncated(), true);
}

void tst_qquicktext::fontFormatSizes_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("textWithTag");
    QTest::addColumn<bool>("fontIsBigger");

    QTest::newRow("fs1") << "Hello world!" << "Hello <font size=\"1\">world</font>!" << false;
    QTest::newRow("fs2") << "Hello world!" << "Hello <font size=\"2\">world</font>!" << false;
    QTest::newRow("fs3") << "Hello world!" << "Hello <font size=\"3\">world</font>!" << false;
    QTest::newRow("fs4") << "Hello world!" << "Hello <font size=\"4\">world</font>!" << true;
    QTest::newRow("fs5") << "Hello world!" << "Hello <font size=\"5\">world</font>!" << true;
    QTest::newRow("fs6") << "Hello world!" << "Hello <font size=\"6\">world</font>!" << true;
    QTest::newRow("fs7") << "Hello world!" << "Hello <font size=\"7\">world</font>!" << true;
    QTest::newRow("h1") << "This is<br/>a font<br/> size test." << "This is <h1>a font</h1> size test." << true;
    QTest::newRow("h2") << "This is<br/>a font<br/> size test." << "This is <h2>a font</h2> size test." << true;
    QTest::newRow("h3") << "This is<br/>a font<br/> size test." << "This is <h3>a font</h3> size test." << true;
    QTest::newRow("h4") << "This is<br/>a font<br/> size test." << "This is <h4>a font</h4> size test." << true;
    QTest::newRow("h5") << "This is<br/>a font<br/> size test." << "This is <h5>a font</h5> size test." << false;
    QTest::newRow("h6") << "This is<br/>a font<br/> size test." << "This is <h6>a font</h6> size test." << false;
}

void tst_qquicktext::fontFormatSizes()
{
    QFETCH(QString, text);
    QFETCH(QString, textWithTag);
    QFETCH(bool, fontIsBigger);

    QQuickView *view = new QQuickView;
    {
        view->setSource(testFileUrl("pointFontSizes.qml"));
        view->show();

        QQuickText *qtext = view->rootObject()->findChild<QQuickText*>("text");
        QQuickText *qtextWithTag = view->rootObject()->findChild<QQuickText*>("textWithTag");
        QVERIFY(qtext != 0);
        QVERIFY(qtextWithTag != 0);

        qtext->setText(text);
        qtextWithTag->setText(textWithTag);

        for (int size = 6; size < 100; size += 4) {
            view->rootObject()->setProperty("pointSize", size);
            if (fontIsBigger)
                QVERIFY(qtext->height() <= qtextWithTag->height());
            else
                QVERIFY(qtext->height() >= qtextWithTag->height());
        }
    }

    {
        view->setSource(testFileUrl("pixelFontSizes.qml"));
        QQuickText *qtext = view->rootObject()->findChild<QQuickText*>("text");
        QQuickText *qtextWithTag = view->rootObject()->findChild<QQuickText*>("textWithTag");
        QVERIFY(qtext != 0);
        QVERIFY(qtextWithTag != 0);

        qtext->setText(text);
        qtextWithTag->setText(textWithTag);

        for (int size = 6; size < 100; size += 4) {
            view->rootObject()->setProperty("pixelSize", size);
            if (fontIsBigger)
                QVERIFY(qtext->height() <= qtextWithTag->height());
            else
                QVERIFY(qtext->height() >= qtextWithTag->height());
        }
    }
    delete view;
}

QTEST_MAIN(tst_qquicktext)

#include "tst_qquicktext.moc"
