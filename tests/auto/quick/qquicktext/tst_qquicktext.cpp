// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QTextDocument>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qjsvalue.h>
#include <QtQuick/private/qquicktext_p.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquickpixmapcache_p.h>
#include <QtQuickTest/QtQuickTest>
#include <private/qquicktext_p_p.h>
#include <private/qsginternaltextnode_p.h>
#include <private/qquickvaluetypes_p.h>
#include <QFontMetrics>
#include <qmath.h>
#include <QtQuick/QQuickView>
#include <QtQuick/qquickitemgrabresult.h>
#include <private/qguiapplication_p.h>
#include <limits.h>
#include <QtGui/QMouseEvent>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/testhttpserver_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtGui/private/qhighdpiscaling_p.h>

DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)

Q_DECLARE_METATYPE(QQuickText::TextFormat)

typedef QVector<QPointF> PointVector;
Q_DECLARE_METATYPE(PointVector);

typedef qreal (*ExpectedBaseline)(QQuickText *item);
Q_DECLARE_METATYPE(ExpectedBaseline)

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

QT_BEGIN_NAMESPACE
extern void qt_setQtEnableTestFont(bool value);

class tst_qquicktext : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquicktext();

private slots:
    void cleanup();
    void text();
    void width();
    void wrap();
    void elide();
    void elideParentChanged();
    void elideRelayoutAfterZeroWidth_data();
    void elideRelayoutAfterZeroWidth();
    void multilineElide_data();
    void multilineElide();
    void implicitElide_data();
    void implicitElide();
    void textFormat();
    void clipRectOutsideViewportDynamicallyChanged();

    void baseUrl();
    void embeddedImages_data();
    void embeddedImages();

    void lineCount();
    void lineHeight();

    // ### these tests may be trivial
    void horizontalAlignment();
    void horizontalAlignment_RightToLeft();
    void verticalAlignment();
    void hAlignImplicitWidth();
    void font();
    void style();
    void color();
    void smooth();
    void renderType();
    void antialiasing();

    // QQuickFontValueType
    void weight();
    void underline();
    void overline();
    void strikeout();
    void capitalization();
    void letterSpacing();
    void wordSpacing();

    void linkInteraction_data();
    void linkInteraction();

    void implicitSize_data();
    void implicitSize();
    void implicitSizeChangeRewrap();
    void implicitSizeMaxLineCount();
    void dependentImplicitSizes();
    void contentSize();
    void implicitSizeBinding_data();
    void implicitSizeBinding();
    void geometryChanged();

    void boundingRect_data();
    void boundingRect();
    void clipRect();
    void largeTextObservesViewport_data();
    void largeTextObservesViewport();
    void largeTextInDelayedLoader();
    void lineLaidOut();
    void lineLaidOutRelayout();
    void lineLaidOutFontUpdate();
    void lineLaidOutHAlign();
    void lineLaidOutImplicitWidth();

    void imgTagsBaseUrl_data();
    void imgTagsBaseUrl();
    void imgTagsAlign_data();
    void imgTagsAlign();
    void imgTagsMultipleImages();
    void imgTagsElide();
    void imgTagsUpdates();
    void imgTagsError();
    void imgSize_data();
    void imgSize();
    void fontSizeMode_data();
    void fontSizeMode();
    void fontSizeModeMultiline_data();
    void fontSizeModeMultiline();
    void multilengthStrings_data();
    void multilengthStrings();
    void fontFormatSizes_data();
    void fontFormatSizes();

    void baselineOffset_data();
    void baselineOffset();

    void htmlLists();
    void htmlLists_data();

    void elideBeforeMaximumLineCount();

    void hover();

    void growFromZeroWidth();

    void padding();
    void paddingInLoader();

    void hintingPreference();

    void zeroWidthAndElidedDoesntRender();

    void hAlignWidthDependsOnImplicitWidth_data();
    void hAlignWidthDependsOnImplicitWidth();

    void fontInfo();

    void initialContentHeight();

    void verticallyAlignedImageInTable();

    void transparentBackground();

    void displaySuperscriptedTag();

    void textObjectRespectsDpr_data();
    void textObjectRespectsDpr();

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

    QQmlEngine engine;

    QQuickView *createView(const QString &filename);
    int numberOfNonWhitePixels(int fromX, int toX, const QImage &image);
};

void tst_qquicktext::cleanup()
{
    QVERIFY(QGuiApplication::topLevelWindows().isEmpty());
}

tst_qquicktext::tst_qquicktext()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
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
    qt_setQtEnableTestFont(true);
}

QQuickView *tst_qquicktext::createView(const QString &filename)
{
    QQuickView *window = new QQuickView(nullptr);

    window->setSource(QUrl::fromLocalFile(filename));
    return window;
}

void tst_qquicktext::text()
{
    {
        QQmlComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"\" }", QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->text(), QString(""));
        QCOMPARE(textObject->width(), qreal(0));
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"" + standard.at(i) + "\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->text(), standard.at(i));
        QVERIFY(textObject->width() > 0);
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"" + richText.at(i) + "\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QString expected = richText.at(i);
        QCOMPARE(textObject->text(), expected.replace("\\\"", "\""));
        QVERIFY(textObject->width() > 0);
    }
}

void tst_qquicktext::width()
{
    // uses Font metrics to find the width for standard and document to find the width for rich
    {
        QQmlComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"\" }", QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->width(), 0.);
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

            metricWidth = layout.boundingRect().width();
        } else {
            QFontMetricsF fm(f);
            metricWidth = fm.size(Qt::TextExpandTabs | Qt::TextShowMnemonic, standard.at(i)).width();
        }

        QString componentStr = "import QtQuick 2.0\nText { text: \"" + standard.at(i) + "\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QVERIFY(textObject->boundingRect().width() > 0);
        QCOMPARE(textObject->width(), qreal(metricWidth));
        QVERIFY(textObject->textFormat() == QQuickText::AutoText); // setting text doesn't change format
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QVERIFY(Qt::mightBeRichText(richText.at(i))); // self-test

        QString componentStr = "import QtQuick 2.0\nText { text: \"" + richText.at(i) + "\"; textFormat: Text.RichText }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY(textObject != nullptr);

        QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
        QVERIFY(textPrivate != nullptr);
        QVERIFY(textPrivate->extra.isAllocated());

        QTextDocument *doc = textPrivate->extra->doc;
        QVERIFY(doc != nullptr);

        QCOMPARE(int(textObject->width()), int(doc->idealWidth()));
        QCOMPARE(textObject->textFormat(), QQuickText::RichText);
    }
}

void tst_qquicktext::wrap()
{
    int textHeight = 0;
    // for specified width and wrap set true
    {
        QQmlComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"Hello\"; wrapMode: Text.WordWrap; width: 300 }", QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));
        textHeight = textObject->height();

        QCOMPARE(textObject->wrapMode(), QQuickText::WordWrap);
        QCOMPARE(textObject->width(), 300.);
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { wrapMode: Text.WordWrap; width: 30; text: \"" + standard.at(i) + "\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->width(), 30.);
        QVERIFY(textObject->height() > textHeight);

        int oldHeight = textObject->height();
        textObject->setWidth(100);
        QVERIFY(textObject->height() < oldHeight);
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { wrapMode: Text.WordWrap; width: 30; text: \"" + richText.at(i) + "\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->width(), 30.);
        QVERIFY(textObject->height() > textHeight);

        qreal oldHeight = textObject->height();
        textObject->setWidth(100);
        QVERIFY(textObject->height() < oldHeight);
    }

    // Check that increasing width from idealWidth will cause a relayout
    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { wrapMode: Text.WordWrap; textFormat: Text.RichText; width: 30; text: \"" + richText.at(i) + "\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->width(), 30.);
        QVERIFY(textObject->height() > textHeight);

        QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
        QVERIFY(textPrivate != nullptr);
        QVERIFY(textPrivate->extra.isAllocated());

        QTextDocument *doc = textPrivate->extra->doc;
        QVERIFY(doc != nullptr);
        textObject->setWidth(doc->idealWidth());
        QCOMPARE(textObject->width(), doc->idealWidth());
        QVERIFY(textObject->height() > textHeight);

        qreal oldHeight = textObject->height();
        textObject->setWidth(100);
        QVERIFY(textObject->height() < oldHeight);
    }

    // richtext again with a fixed height
    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { wrapMode: Text.WordWrap; width: 30; height: 50; text: \"" + richText.at(i) + "\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->width(), 30.);
        QVERIFY(textObject->implicitHeight() > textHeight);

        qreal oldHeight = textObject->implicitHeight();
        textObject->setWidth(100);
        QVERIFY(textObject->implicitHeight() < oldHeight);
    }

    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\n Text {}", QUrl());
        QScopedPointer<QObject> object(component.create());
        QQuickText *textObject = qobject_cast<QQuickText *>(object.data());
        QVERIFY(textObject);

        QSignalSpy spy(textObject, SIGNAL(wrapModeChanged()));

        QCOMPARE(textObject->wrapMode(), QQuickText::NoWrap);

        textObject->setWrapMode(QQuickText::Wrap);
        QCOMPARE(textObject->wrapMode(), QQuickText::Wrap);
        QCOMPARE(spy.size(), 1);

        textObject->setWrapMode(QQuickText::Wrap);
        QCOMPARE(spy.size(), 1);

        textObject->setWrapMode(QQuickText::NoWrap);
        QCOMPARE(textObject->wrapMode(), QQuickText::NoWrap);
        QCOMPARE(spy.size(), 2);
    }
}

void tst_qquicktext::elide()
{
    for (QQuickText::TextElideMode m = QQuickText::ElideLeft; m<=QQuickText::ElideNone; m=QQuickText::TextElideMode(int(m)+1)) {
        const char* elidename[]={"ElideLeft", "ElideRight", "ElideMiddle", "ElideNone"};
        QString elide = "elide: Text." + QString(elidename[int(m)]) + ";";

        // XXX Poor coverage.

        {
            QQmlComponent textComponent(&engine);
            textComponent.setData(("import QtQuick 2.0\nText { text: \"\"; "+elide+" width: 100 }").toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
            QVERIFY2(textObject, qPrintable(textComponent.errorString()));

            QCOMPARE(textObject->elideMode(), m);
            QCOMPARE(textObject->width(), 100.);
        }

        for (int i = 0; i < standard.size(); i++)
        {
            QString componentStr = "import QtQuick 2.0\nText { "+elide+" width: 100; text: \"" + standard.at(i) + "\" }";
            QQmlComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
            QVERIFY2(textObject, qPrintable(textComponent.errorString()));

            QCOMPARE(textObject->elideMode(), m);
            QCOMPARE(textObject->width(), 100.);

            if (m != QQuickText::ElideNone && !standard.at(i).contains('\n'))
                QVERIFY(textObject->contentWidth() <= textObject->width());
        }

        for (int i = 0; i < richText.size(); i++)
        {
            QString componentStr = "import QtQuick 2.0\nText { "+elide+" width: 100; text: \"" + richText.at(i) + "\" }";
            QQmlComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
            QVERIFY2(textObject, qPrintable(textComponent.errorString()));

            QCOMPARE(textObject->elideMode(), m);
            QCOMPARE(textObject->width(), 100.);

            if (m != QQuickText::ElideNone && standard.at(i).contains("<br>"))
                QVERIFY(textObject->contentWidth() <= textObject->width());
        }
    }
}

// QTBUG-60328
// Tests that text with elide set is rendered after
// having its parent cleared and then set again.
void tst_qquicktext::elideParentChanged()
{
    QQuickView window;
    window.setSource(testFileUrl("elideParentChanged.qml"));
    QTRY_COMPARE(window.status(), QQuickView::Ready);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickItem *root = window.rootObject();
    QVERIFY(root);
    QCOMPARE(root->childItems().size(), 1);

    // Store a snapshot of the scene so that we can compare it later.
    QSharedPointer<QQuickItemGrabResult> grabResult = root->grabToImage();
    QTRY_VERIFY(!grabResult->image().isNull());
    const QImage expectedItemImageGrab(grabResult->image());

    // Clear the text's parent. It shouldn't render anything.
    QQuickItem *text = root->childItems().first();
    text->setParentItem(nullptr);
    QCOMPARE(text->width(), 0.0);
    QCOMPARE(text->height(), 0.0);

    // Set the parent back to what it was. The text should
    // be rendered identically to how it was before.
    text->setParentItem(root);
    QCOMPARE(text->width(), 100.0);
    QCOMPARE(text->height(), 30.0);

    grabResult = root->grabToImage();
    QTRY_VERIFY(!grabResult->image().isNull());
    const QImage actualItemImageGrab(grabResult->image());
    QCOMPARE(actualItemImageGrab, expectedItemImageGrab);
}

void tst_qquicktext::elideRelayoutAfterZeroWidth_data()
{
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("no_margins") << QByteArray("elideZeroWidth.qml");
    QTest::newRow("with_margins") << QByteArray("elideZeroWidthWithMargins.qml");
}

void tst_qquicktext::elideRelayoutAfterZeroWidth()
{
    QFETCH(const QByteArray, fileName);
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl(fileName.constData()));
    QScopedPointer<QObject> root(component.create());
    QVERIFY2(root, qPrintable(component.errorString()));
    QVERIFY(root->property("ok").toBool());
}

void tst_qquicktext::multilineElide_data()
{
    QTest::addColumn<QQuickText::TextFormat>("format");
    QTest::newRow("plain") << QQuickText::PlainText;
    QTest::newRow("styled") << QQuickText::StyledText;
}

void tst_qquicktext::multilineElide()
{
    QFETCH(QQuickText::TextFormat, format);
    QScopedPointer<QQuickView> window(createView(testFile("multilineelide.qml")));

    QQuickText *myText = qobject_cast<QQuickText*>(window->rootObject());
    QVERIFY(myText != nullptr);
    myText->setTextFormat(format);

    QCOMPARE(myText->lineCount(), 3);
    QCOMPARE(myText->truncated(), true);

    qreal lineHeight = myText->contentHeight() / 3.;

    // Set a valid height greater than the truncated content height and ensure the line count is
    // unchanged.
    myText->setHeight(200);
    QCOMPARE(myText->lineCount(), 3);
    QCOMPARE(myText->truncated(), true);

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
}

void tst_qquicktext::implicitElide_data()
{
    QTest::addColumn<QString>("width");
    QTest::addColumn<QString>("initialText");
    QTest::addColumn<QString>("text");

    QTest::newRow("maximum width, empty")
            << "Math.min(implicitWidth, 100)"
            << "";
    QTest::newRow("maximum width, short")
            << "Math.min(implicitWidth, 100)"
            << "the";
    QTest::newRow("maximum width, long")
            << "Math.min(implicitWidth, 100)"
            << "the quick brown fox jumped over the lazy dog";
    QTest::newRow("reset width, empty")
            << "implicitWidth > 100 ? 100 : undefined"
            << "";
    QTest::newRow("reset width, short")
            << "implicitWidth > 100 ? 100 : undefined"
            << "the";
    QTest::newRow("reset width, long")
            << "implicitWidth > 100 ? 100 : undefined"
            << "the quick brown fox jumped over the lazy dog";
}

void tst_qquicktext::implicitElide()
{
    QFETCH(QString, width);
    QFETCH(QString, initialText);

    QString componentStr =
            "import QtQuick 2.0\n"
            "Text {\n"
                "width: " + width + "\n"
                "text: \"" + initialText + "\"\n"
                "elide: Text.ElideRight\n"
            "}";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickText *textObject = qobject_cast<QQuickText*>(textComponent.create());

    QVERIFY2(textObject, qPrintable(textComponent.errorString()));
    QVERIFY(textObject->contentWidth() <= textObject->width());

    textObject->setText("the quick brown fox jumped over");

    QVERIFY(textObject->contentWidth() > 0);
    QVERIFY(textObject->contentWidth() <= textObject->width());
}

void tst_qquicktext::textFormat()
{
    {
        QQmlComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"Hello\"; textFormat: Text.RichText }", QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->textFormat(), QQuickText::RichText);

        QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
        QVERIFY(textPrivate != nullptr);
        QVERIFY(textPrivate->richText);
    }
    {
        QQmlComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"<b>Hello</b>\" }", QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->textFormat(), QQuickText::AutoText);

        QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
        QVERIFY(textPrivate != nullptr);
        QVERIFY(textPrivate->styledText);
    }
    {
        QQmlComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nText { text: \"<b>Hello</b>\"; textFormat: Text.PlainText }", QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->textFormat(), QQuickText::PlainText);
    }

    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\n Text {}", QUrl());
        QScopedPointer<QObject> object(component.create());
        QQuickText *text = qobject_cast<QQuickText *>(object.data());
        QVERIFY(text);

        QSignalSpy spy(text, &QQuickText::textFormatChanged);

        QCOMPARE(text->textFormat(), QQuickText::AutoText);

        text->setTextFormat(QQuickText::StyledText);
        QCOMPARE(text->textFormat(), QQuickText::StyledText);
        QCOMPARE(spy.size(), 1);

        text->setTextFormat(QQuickText::StyledText);
        QCOMPARE(spy.size(), 1);

        text->setTextFormat(QQuickText::AutoText);
        QCOMPARE(text->textFormat(), QQuickText::AutoText);
        QCOMPARE(spy.size(), 2);
    }

    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\n Text { text: \"<b>Hello</b>\" }", QUrl());
        QScopedPointer<QObject> object(component.create());
        QQuickText *text = qobject_cast<QQuickText *>(object.data());
        QVERIFY(text);
        QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(text);
        QVERIFY(textPrivate);

        QCOMPARE(text->textFormat(), QQuickText::AutoText);
        QVERIFY(!textPrivate->layout.formats().isEmpty());

        text->setTextFormat(QQuickText::StyledText);
        QVERIFY(!textPrivate->layout.formats().isEmpty());

        text->setTextFormat(QQuickText::PlainText);
        QVERIFY(textPrivate->layout.formats().isEmpty());

        text->setTextFormat(QQuickText::AutoText);
        QVERIFY(!textPrivate->layout.formats().isEmpty());
    }

    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nText { text: \"Hello\"; elide: Text.ElideRight }", QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(component.create());
        QQuickText *text = qobject_cast<QQuickText *>(object.data());
        QVERIFY(text);
        QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(text);
        QVERIFY(textPrivate);

        // underline a mnemonic
        QVector<QTextLayout::FormatRange> formats;
        QTextLayout::FormatRange range;
        range.start = 0;
        range.length = 1;
        range.format.setFontUnderline(true);
        formats << range;

        // the mnemonic format should be retained
        textPrivate->layout.setFormats(formats);
        text->forceLayout();
        QCOMPARE(textPrivate->layout.formats(), formats);

        // and carried over to the elide layout
        text->setWidth(text->implicitWidth() - 1);
        QVERIFY(textPrivate->elideLayout);
        QCOMPARE(textPrivate->elideLayout->formats(), formats);

        // but cleared when the text changes
        text->setText("Changed");
        QVERIFY(textPrivate->elideLayout);
        QVERIFY(textPrivate->layout.formats().isEmpty());
    }
}

void tst_qquicktext::clipRectOutsideViewportDynamicallyChanged()
{
    // QTBUG-106205
    QScopedPointer<QQuickView> view(createView(testFile("qtbug_106205.qml")));
    view->setWidth(100);
    view->setHeight(200);
    view->showNormal();
    QQuickItem *root = view->rootObject();
    QVERIFY(root);
    QVERIFY(QTest::qWaitForWindowExposed(view.get()));

    auto clipRectMatches = [&]() -> bool {
        auto *textOutsideInitialViewport = root->findChild<QQuickText *>("textOutsideViewport");
        if (!textOutsideInitialViewport)
            return false;
        auto *clipNode = QQuickItemPrivate::get(textOutsideInitialViewport)->clipNode();

        return textOutsideInitialViewport->clipRect() == clipNode->clipRect();
    };
    QTRY_VERIFY(clipRectMatches());
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
            QQmlComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
            QVERIFY2(textObject, qPrintable(textComponent.errorString()));

            QCOMPARE((int)textObject->hAlign(), (int)horizontalAlignmentments.at(j));
        }
    }

    for (int i = 0; i < richText.size(); i++)
    {
        for (int j=0; j < horizontalAlignmentmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nText { horizontalAlignment: \"" + horizontalAlignmentmentStrings.at(j) + "\"; text: \"" + richText.at(i) + "\" }";
            QQmlComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
            QVERIFY2(textObject, qPrintable(textComponent.errorString()));

            QCOMPARE((int)textObject->hAlign(), (int)horizontalAlignmentments.at(j));
        }
    }

}

void tst_qquicktext::horizontalAlignment_RightToLeft()
{
    QScopedPointer<QQuickView> window(createView(testFile("horizontalAlignment_RightToLeft.qml")));
    QQuickText *text = window->rootObject()->findChild<QQuickText*>("text");
    QVERIFY(text != nullptr);
    window->showNormal();

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(text);
    QVERIFY(textPrivate != nullptr);

    QTRY_VERIFY(textPrivate->layout.lineCount());

    // implicit alignment should follow the reading direction of RTL text
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() > window->width()/2);

    // explicitly left aligned text
    text->setHAlign(QQuickText::AlignLeft);
    QCOMPARE(text->hAlign(), QQuickText::AlignLeft);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() < window->width()/2);

    // explicitly right aligned text
    text->setHAlign(QQuickText::AlignRight);
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() > window->width()/2);

    // change to rich text
    QString textString = text->text();
    text->setText(QString("<i>") + textString + QString("</i>"));
    text->setTextFormat(QQuickText::RichText);
    text->resetHAlign();

    // implicitly aligned rich text should follow the reading direction of text
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->extra.isAllocated());
    QVERIFY(textPrivate->extra->doc->defaultTextOption().alignment() & Qt::AlignLeft);

    // explicitly left aligned rich text
    text->setHAlign(QQuickText::AlignLeft);
    QCOMPARE(text->hAlign(), QQuickText::AlignLeft);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->extra->doc->defaultTextOption().alignment() & Qt::AlignRight);

    // explicitly right aligned rich text
    text->setHAlign(QQuickText::AlignRight);
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->extra->doc->defaultTextOption().alignment() & Qt::AlignLeft);

    text->setText(textString);
    text->setTextFormat(QQuickText::PlainText);

    // explicitly center aligned
    text->setHAlign(QQuickText::AlignHCenter);
    QCOMPARE(text->hAlign(), QQuickText::AlignHCenter);
    QCOMPARE(text->effectiveHAlign(), text->hAlign());
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() < window->width()/2);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().right() > window->width()/2);

    // reseted alignment should go back to following the text reading direction
    text->resetHAlign();
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() > window->width()/2);

    // mirror the text item
    QQuickItemPrivate::get(text)->setLayoutMirror(true);

    // mirrored implicit alignment should continue to follow the reading direction of the text
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), QQuickText::AlignRight);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() > window->width()/2);

    // mirrored explicitly right aligned behaves as left aligned
    text->setHAlign(QQuickText::AlignRight);
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);
    QCOMPARE(text->effectiveHAlign(), QQuickText::AlignLeft);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() < window->width()/2);

    // mirrored explicitly left aligned behaves as right aligned
    text->setHAlign(QQuickText::AlignLeft);
    QCOMPARE(text->hAlign(), QQuickText::AlignLeft);
    QCOMPARE(text->effectiveHAlign(), QQuickText::AlignRight);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() > window->width()/2);

    // disable mirroring
    QQuickItemPrivate::get(text)->setLayoutMirror(false);
    text->resetHAlign();

    // English text should be implicitly left aligned
    text->setText("Hello world!");
    QCOMPARE(text->hAlign(), QQuickText::AlignLeft);
    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().left() < window->width()/2);

    // empty text with implicit alignment follows the system locale-based
    // keyboard input direction from QInputMethod::inputDirection()
    text->setText("");
    QCOMPARE(text->hAlign(), qApp->inputMethod()->inputDirection() == Qt::LeftToRight ?
                                  QQuickText::AlignLeft : QQuickText::AlignRight);
    text->setHAlign(QQuickText::AlignRight);
    QCOMPARE(text->hAlign(), QQuickText::AlignRight);

    window.reset();

    // alignment of Text with no text set to it
    QString componentStr = "import QtQuick 2.0\nText {}";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
    QVERIFY2(textObject, qPrintable(textComponent.errorString()));
    QCOMPARE(textObject->hAlign(), qApp->inputMethod()->inputDirection() == Qt::LeftToRight ?
                                  QQuickText::AlignLeft : QQuickText::AlignRight);
}

int tst_qquicktext::numberOfNonWhitePixels(int fromX, int toX, const QImage &image)
{
    int pixels = 0;
    for (int x = fromX; x < toX; ++x) {
        for (int y = 0; y < image.height(); ++y) {
            if (image.pixel(x, y) != qRgb(255, 255, 255))
                pixels++;
        }
    }
    return pixels;
}

static inline QByteArray msgNotGreaterThan(int n1, int n2)
{
    return QByteArray::number(n1) + QByteArrayLiteral(" is not greater than ") + QByteArray::number(n2);
}

static inline QByteArray msgNotLessThan(int n1, int n2)
{
    return QByteArray::number(n1) + QByteArrayLiteral(" is not less than ") + QByteArray::number(n2);
}

void tst_qquicktext::hAlignImplicitWidth()
{
    SKIP_IF_NO_WINDOW_GRAB;

    QQuickView view(testFileUrl("hAlignImplicitWidth.qml"));
    view.setFlags(view.flags() | Qt::WindowStaysOnTopHint); // Prevent being obscured by other windows.
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickText *text = view.rootObject()->findChild<QQuickText*>("textItem");
    QVERIFY(text != nullptr);

    // Try to check whether alignment works by checking the number of black
    // pixels in the thirds of the grabbed image.
    // QQuickWindow::grabWindow() scales the returned image by the devicePixelRatio of the screen.
    const qreal devicePixelRatio = view.screen()->devicePixelRatio();
    const int windowWidth = 220 * devicePixelRatio;
    const int textWidth = qCeil(text->implicitWidth()) * devicePixelRatio;
    QVERIFY2(textWidth < windowWidth, "System font too large.");
    const int sectionWidth = textWidth / 3;
    const int centeredSection1 = (windowWidth - textWidth) / 2;
    const int centeredSection2 = centeredSection1 + sectionWidth;
    const int centeredSection3 = centeredSection2 + sectionWidth;
    const int centeredSection3End = centeredSection3 + sectionWidth;

    {
        // Left Align
        QImage image = view.grabWindow();
        const int left = numberOfNonWhitePixels(centeredSection1, centeredSection2, image);
        const int mid = numberOfNonWhitePixels(centeredSection2, centeredSection3, image);
        const int right = numberOfNonWhitePixels(centeredSection3, centeredSection3End, image);
        QVERIFY2(left > mid, msgNotGreaterThan(left, mid).constData());
        QVERIFY2(mid > right, msgNotGreaterThan(mid, right).constData());
    }
    {
        // HCenter Align
        text->setHAlign(QQuickText::AlignHCenter);
        text->setText("Reset"); // set dummy string to force relayout once original text is set again
        text->setText("AA\nBBBBBBB\nCCCCCCCCCCCCCCCC");
        QImage image = view.grabWindow();
        const int left = numberOfNonWhitePixels(centeredSection1, centeredSection2, image);
        const int mid = numberOfNonWhitePixels(centeredSection2, centeredSection3, image);
        const int right = numberOfNonWhitePixels(centeredSection3, centeredSection3End, image);
        QVERIFY2(left < mid, msgNotLessThan(left, mid).constData());
        QVERIFY2(mid > right, msgNotGreaterThan(mid, right).constData());
    }
    {
        // Right Align
        text->setHAlign(QQuickText::AlignRight);
        text->setText("Reset"); // set dummy string to force relayout once original text is set again
        text->setText("AA\nBBBBBBB\nCCCCCCCCCCCCCCCC");
        QImage image = view.grabWindow();
        const int left = numberOfNonWhitePixels(centeredSection1, centeredSection2, image);
        const int mid = numberOfNonWhitePixels(centeredSection2, centeredSection3, image);
        const int right = numberOfNonWhitePixels(centeredSection3, centeredSection3End, image);
        QVERIFY2(left < mid, msgNotLessThan(left, mid).constData());
        QVERIFY2(mid < right, msgNotLessThan(mid, right).constData());
    }
}

void tst_qquicktext::verticalAlignment()
{
    //test one align each, and then test if two align fails.

    for (int i = 0; i < standard.size(); i++)
    {
        for (int j=0; j < verticalAlignmentmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nText { verticalAlignment: \"" + verticalAlignmentmentStrings.at(j) + "\"; text: \"" + standard.at(i) + "\" }";
            QQmlComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

            QVERIFY(textObject != nullptr);
            QCOMPARE((int)textObject->vAlign(), (int)verticalAlignmentments.at(j));
        }
    }

    for (int i = 0; i < richText.size(); i++)
    {
        for (int j=0; j < verticalAlignmentmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nText { verticalAlignment: \"" + verticalAlignmentmentStrings.at(j) + "\"; text: \"" + richText.at(i) + "\" }";
            QQmlComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

            QVERIFY(textObject != nullptr);
            QCOMPARE((int)textObject->vAlign(), (int)verticalAlignmentments.at(j));
        }
    }

}

void tst_qquicktext::font()
{
    //test size, then bold, then italic, then family
    {
        QString componentStr = "import QtQuick 2.0\nText { font.pointSize: 40; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QCOMPARE(textObject->font().pointSize(), 40);
        QCOMPARE(textObject->font().bold(), false);
        QCOMPARE(textObject->font().italic(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nText { font.pixelSize: 40; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QCOMPARE(textObject->font().pixelSize(), 40);
        QCOMPARE(textObject->font().bold(), false);
        QCOMPARE(textObject->font().italic(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nText { font.bold: true; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QCOMPARE(textObject->font().bold(), true);
        QCOMPARE(textObject->font().italic(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nText { font.italic: true; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QCOMPARE(textObject->font().italic(), true);
        QCOMPARE(textObject->font().bold(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nText { font.family: \"Helvetica\"; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QCOMPARE(textObject->font().family(), QString("Helvetica"));
        QCOMPARE(textObject->font().bold(), false);
        QCOMPARE(textObject->font().italic(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nText { font.family: \"\"; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QCOMPARE(textObject->font().family(), QString(""));
    }
}

void tst_qquicktext::style()
{
    //test style
    for (int i = 0; i < styles.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { style: \"" + styleStrings.at(i) + "\"; styleColor: \"white\"; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QCOMPARE((int)textObject->style(), (int)styles.at(i));
        QCOMPARE(textObject->styleColor(), QColor("white"));
    }
    QString componentStr = "import QtQuick 2.0\nText { text: \"Hello World\" }";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
    QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

    QRectF brPre = textObject->boundingRect();
    textObject->setStyle(QQuickText::Outline);
    QRectF brPost = textObject->boundingRect();

    QVERIFY(brPre.width() < brPost.width());
    QVERIFY(brPre.height() < brPost.height());
}

void tst_qquicktext::color()
{
    //test style
    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { color: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QCOMPARE(textObject->color(), QColor(colorStrings.at(i)));
        QCOMPARE(textObject->styleColor(), QColor("black"));
        QCOMPARE(textObject->linkColor(), QColor("blue"));
    }

    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { styleColor: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QCOMPARE(textObject->styleColor(), QColor(colorStrings.at(i)));
        // default color to black?
        QCOMPARE(textObject->color(), QColor("black"));
        QCOMPARE(textObject->linkColor(), QColor("blue"));

        QSignalSpy colorSpy(textObject, SIGNAL(colorChanged()));
        QSignalSpy linkColorSpy(textObject, SIGNAL(linkColorChanged()));

        textObject->setColor(QColor("white"));
        QCOMPARE(textObject->color(), QColor("white"));
        QCOMPARE(colorSpy.size(), 1);

        textObject->setLinkColor(QColor("black"));
        QCOMPARE(textObject->linkColor(), QColor("black"));
        QCOMPARE(linkColorSpy.size(), 1);

        textObject->setColor(QColor("white"));
        QCOMPARE(colorSpy.size(), 1);

        textObject->setLinkColor(QColor("black"));
        QCOMPARE(linkColorSpy.size(), 1);

        textObject->setColor(QColor("black"));
        QCOMPARE(textObject->color(), QColor("black"));
        QCOMPARE(colorSpy.size(), 2);

        textObject->setLinkColor(QColor("blue"));
        QCOMPARE(textObject->linkColor(), QColor("blue"));
        QCOMPARE(linkColorSpy.size(), 2);
    }

    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nText { linkColor: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QCOMPARE(textObject->styleColor(), QColor("black"));
        QCOMPARE(textObject->color(), QColor("black"));
        QCOMPARE(textObject->linkColor(), QColor(colorStrings.at(i)));
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
            QQmlComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
            QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

            QCOMPARE(textObject->color(), QColor(colorStrings.at(i)));
            QCOMPARE(textObject->styleColor(), QColor(colorStrings.at(j)));
            QCOMPARE(textObject->linkColor(), QColor(colorStrings.at(j)));
        }
    }
    {
        QString colorStr = "#AA001234";
        QColor testColor("#001234");
        testColor.setAlpha(170);

        QString componentStr = "import QtQuick 2.0\nText { color: \"" + colorStr + "\"; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QCOMPARE(textObject->color(), testColor);
    } {
        QString colorStr = "#001234";
        QColor testColor(colorStr);

        QString componentStr = "import QtQuick 2.0\nText { color: \"" + colorStr + "\"; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QSignalSpy spy(textObject, SIGNAL(colorChanged()));

        QCOMPARE(textObject->color(), testColor);
        textObject->setColor(testColor);
        QCOMPARE(textObject->color(), testColor);
        QCOMPARE(spy.size(), 0);

        testColor = QColor("black");
        textObject->setColor(testColor);
        QCOMPARE(textObject->color(), testColor);
        QCOMPARE(spy.size(), 1);
    } {
        QString colorStr = "#001234";
        QColor testColor(colorStr);

        QString componentStr = "import QtQuick 2.0\nText { styleColor: \"" + colorStr + "\"; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QSignalSpy spy(textObject, SIGNAL(styleColorChanged()));

        QCOMPARE(textObject->styleColor(), testColor);
        textObject->setStyleColor(testColor);
        QCOMPARE(textObject->styleColor(), testColor);
        QCOMPARE(spy.size(), 0);

        testColor = QColor("black");
        textObject->setStyleColor(testColor);
        QCOMPARE(textObject->styleColor(), testColor);
        QCOMPARE(spy.size(), 1);
    } {
        QString colorStr = "#001234";
        QColor testColor(colorStr);

        QString componentStr = "import QtQuick 2.0\nText { linkColor: \"" + colorStr + "\"; text: \"Hello World\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
        QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));

        QSignalSpy spy(textObject, SIGNAL(linkColorChanged()));

        QCOMPARE(textObject->linkColor(), testColor);
        textObject->setLinkColor(testColor);
        QCOMPARE(textObject->linkColor(), testColor);
        QCOMPARE(spy.size(), 0);

        testColor = QColor("black");
        textObject->setLinkColor(testColor);
        QCOMPARE(textObject->linkColor(), testColor);
        QCOMPARE(spy.size(), 1);
    }
}

void tst_qquicktext::smooth()
{
    for (int i = 0; i < standard.size(); i++)
    {
        {
            QString componentStr = "import QtQuick 2.0\nText { smooth: false; text: \"" + standard.at(i) + "\" }";
            QQmlComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
            QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));
            QCOMPARE(textObject->smooth(), false);
        }
        {
            QString componentStr = "import QtQuick 2.0\nText { text: \"" + standard.at(i) + "\" }";
            QQmlComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
            QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));
            QCOMPARE(textObject->smooth(), true);
        }
    }
    for (int i = 0; i < richText.size(); i++)
    {
        {
            QString componentStr = "import QtQuick 2.0\nText { smooth: false; text: \"" + richText.at(i) + "\" }";
            QQmlComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
            QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));
            QCOMPARE(textObject->smooth(), false);
        }
        {
            QString componentStr = "import QtQuick 2.0\nText { text: \"" + richText.at(i) + "\" }";
            QQmlComponent textComponent(&engine);
            textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
            QScopedPointer<QObject> object(textComponent.create());
            QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
            QVERIFY2(textObject != nullptr, qPrintable(textComponent.errorString()));
            QCOMPARE(textObject->smooth(), true);
        }
    }
}

void tst_qquicktext::renderType()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n Text {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickText *text = qobject_cast<QQuickText *>(object.data());
    QVERIFY(text);

    QSignalSpy spy(text, SIGNAL(renderTypeChanged()));

    QCOMPARE(text->renderType(), QQuickText::QtRendering);

    text->setRenderType(QQuickText::NativeRendering);
    QCOMPARE(text->renderType(), QQuickText::NativeRendering);
    QCOMPARE(spy.size(), 1);

    text->setRenderType(QQuickText::NativeRendering);
    QCOMPARE(spy.size(), 1);

    text->setRenderType(QQuickText::QtRendering);
    QCOMPARE(text->renderType(), QQuickText::QtRendering);
    QCOMPARE(spy.size(), 2);
}

void tst_qquicktext::antialiasing()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n Text {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickText *text = qobject_cast<QQuickText *>(object.data());
    QVERIFY(text);

    QSignalSpy spy(text, SIGNAL(antialiasingChanged(bool)));

    QCOMPARE(text->antialiasing(), true);

    text->setAntialiasing(false);
    QCOMPARE(text->antialiasing(), false);
    QCOMPARE(spy.size(), 1);

    text->setAntialiasing(false);
    QCOMPARE(spy.size(), 1);

    text->resetAntialiasing();
    QCOMPARE(text->antialiasing(), true);
    QCOMPARE(spy.size(), 2);

    // QTBUG-39047
    component.setData("import QtQuick 2.0\n Text { antialiasing: true }", QUrl());
    object.reset(component.create());
    text = qobject_cast<QQuickText *>(object.data());
    QVERIFY(text);
    QCOMPARE(text->antialiasing(), true);
}

void tst_qquicktext::weight()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE((int)textObject->font().weight(), int(QQuickFontEnums::Normal));
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { font.weight: Font.Bold; text: \"Hello world!\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE((int)textObject->font().weight(), int(QQuickFontEnums::Bold));
    }
}

void tst_qquicktext::underline()
{
    QQuickView view(testFileUrl("underline.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QQuickText *textObject = view.rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(textObject != nullptr);
    QCOMPARE(textObject->font().overline(), false);
    QCOMPARE(textObject->font().underline(), true);
    QCOMPARE(textObject->font().strikeOut(), false);
}

void tst_qquicktext::overline()
{
    QQuickView view(testFileUrl("overline.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QQuickText *textObject = view.rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(textObject != nullptr);
    QCOMPARE(textObject->font().overline(), true);
    QCOMPARE(textObject->font().underline(), false);
    QCOMPARE(textObject->font().strikeOut(), false);
}

void tst_qquicktext::strikeout()
{
    QQuickView view(testFileUrl("strikeout.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QQuickText *textObject = view.rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(textObject != nullptr);
    QCOMPARE(textObject->font().overline(), false);
    QCOMPARE(textObject->font().underline(), false);
    QCOMPARE(textObject->font().strikeOut(), true);
}

void tst_qquicktext::capitalization()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE((int)textObject->font().capitalization(), int(QQuickFontEnums::MixedCase));
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.capitalization: \"AllUppercase\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE((int)textObject->font().capitalization(), int(QQuickFontEnums::AllUppercase));
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.capitalization: \"AllLowercase\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE((int)textObject->font().capitalization(), int(QQuickFontEnums::AllLowercase));
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.capitalization: \"SmallCaps\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE((int)textObject->font().capitalization(), int(QQuickFontEnums::SmallCaps));
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.capitalization: \"Capitalize\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE((int)textObject->font().capitalization(), int(QQuickFontEnums::Capitalize));
    }
}

void tst_qquicktext::letterSpacing()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->font().letterSpacing(), 0.0);
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.letterSpacing: -2 }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->font().letterSpacing(), -2.);
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.letterSpacing: 3 }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->font().letterSpacing(), 3.);
    }
}

void tst_qquicktext::wordSpacing()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->font().wordSpacing(), 0.0);
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.wordSpacing: -50 }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->font().wordSpacing(), -50.);
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.wordSpacing: 200 }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->font().wordSpacing(), 200.);
    }
}

class EventSender : public QQuickItem
{
public:
    void sendEvent(QEvent *event) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
            mousePressEvent(static_cast<QMouseEvent *>(event));
            break;
        case QEvent::MouseButtonRelease:
            mouseReleaseEvent(static_cast<QMouseEvent *>(event));
            break;
        case QEvent::MouseMove:
            mouseMoveEvent(static_cast<QMouseEvent *>(event));
            break;
        case QEvent::HoverEnter:
            hoverEnterEvent(static_cast<QHoverEvent *>(event));
            break;
        case QEvent::HoverLeave:
            hoverLeaveEvent(static_cast<QHoverEvent *>(event));
            break;
        case QEvent::HoverMove:
            hoverMoveEvent(static_cast<QHoverEvent *>(event));
            break;
        default:
            qWarning() << "Trying to send unsupported event type";
            break;
        }
    }
};

class LinkTest : public QObject
{
    Q_OBJECT
public:
    LinkTest() {}

    QString clickedLink;
    QString hoveredLink;

public slots:
    void linkClicked(QString l) { clickedLink = l; }
    void linkHovered(QString l) { hoveredLink = l; }
};

class TextMetrics
{
public:
    TextMetrics(const QString &text, Qt::TextElideMode elideMode = Qt::ElideNone)
    {
        QString adjustedText = text;
        adjustedText.replace(QLatin1Char('\n'), QChar(QChar::LineSeparator));
        if (elideMode == Qt::ElideLeft)
            adjustedText = QChar(0x2026) + adjustedText;
        else if (elideMode == Qt::ElideRight)
            adjustedText = adjustedText + QChar(0x2026);

        layout.setText(adjustedText);
        QTextOption option;
        option.setUseDesignMetrics(true);
        layout.setTextOption(option);

        layout.beginLayout();
        qreal height = 0;
        QTextLine line = layout.createLine();
        while (line.isValid()) {
            line.setLineWidth(FLT_MAX);
            line.setPosition(QPointF(0, height));
            height += line.height();
            line = layout.createLine();
        }
        layout.endLayout();
    }

    qreal width() const { return layout.maximumWidth(); }

    QRectF characterRectangle(
            int position,
            int hAlign = Qt::AlignLeft,
            int vAlign = Qt::AlignTop,
            const QSizeF &bounds = QSizeF(240, 320)) const
    {
        qreal dy = 0;
        switch (vAlign) {
        case Qt::AlignBottom:
            dy = bounds.height() - layout.boundingRect().height();
            break;
        case Qt::AlignVCenter:
            dy = (bounds.height() - layout.boundingRect().height()) / 2;
            break;
        default:
            break;
        }

        for (int i = 0; i < layout.lineCount(); ++i) {
            QTextLine line = layout.lineAt(i);
            if (position >= line.textStart() + line.textLength())
                continue;
            qreal dx = 0;
            switch (hAlign) {
            case Qt::AlignRight:
                dx = bounds.width() - line.naturalTextWidth();
                break;
            case Qt::AlignHCenter:
                dx = (bounds.width() - line.naturalTextWidth()) / 2;
                break;
            default:
                break;
            }

            QRectF rect;
            rect.setLeft(dx + line.cursorToX(position, QTextLine::Leading));
            rect.setRight(dx + line.cursorToX(position, QTextLine::Trailing));
            rect.setTop(dy + line.y());
            rect.setBottom(dy + line.y() + line.height());

            return rect;
        }
        return QRectF();
    }

    QTextLayout layout;
};

void tst_qquicktext::linkInteraction_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<qreal>("width");
    QTest::addColumn<QString>("bindings");
    QTest::addColumn<PointVector>("mousePositions");
    QTest::addColumn<QString>("clickedLink");
    QTest::addColumn<QString>("hoverEnterLink");
    QTest::addColumn<QString>("hoverMoveLink");

    const QString singleLineText = "this text has a <a href=\\\"http://qt-project.org/single\\\">link</a> in it";
    const QString singleLineLink = "http://qt-project.org/single";
    const QString multipleLineText = "this text<br/>has <a href=\\\"http://qt-project.org/multiple\\\">multiple<br/>lines</a> in it";
    const QString multipleLineLink = "http://qt-project.org/multiple";
    const QString nestedText = "this text has a <a href=\\\"http://qt-project.org/outer\\\">nested <a href=\\\"http://qt-project.org/inner\\\">link</a> in it</a>";
    const QString outerLink = "http://qt-project.org/outer";
    const QString innerLink = "http://qt-project.org/inner";

    {
        const TextMetrics metrics("this text has a link in it");

        QTest::newRow("click on link")
                << singleLineText << 240.
                << ""
                << (PointVector() << metrics.characterRectangle(18).center())
                << singleLineLink
                << singleLineLink << singleLineLink;
        QTest::newRow("click on text")
                << singleLineText << 240.
                << ""
                << (PointVector() << metrics.characterRectangle(13).center())
                << QString()
                << QString() << QString();
        QTest::newRow("drag within link")
                << singleLineText << 240.
                << ""
                << (PointVector()
                    << metrics.characterRectangle(17).center()
                    << metrics.characterRectangle(19).center())
                << singleLineLink
                << singleLineLink << singleLineLink;
        QTest::newRow("drag away from link")
                << singleLineText << 240.
                << ""
                << (PointVector()
                    << metrics.characterRectangle(18).center()
                    << metrics.characterRectangle(13).center())
                << QString()
                << singleLineLink << QString();
        QTest::newRow("drag on to link")
                << singleLineText << 240.
                << ""
                << (PointVector()
                    << metrics.characterRectangle(13).center()
                    << metrics.characterRectangle(18).center())
                << QString()
                << QString() << singleLineLink;
        QTest::newRow("click on bottom right aligned link")
                << singleLineText << 240.
                << "horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignBottom"
                << (PointVector() << metrics.characterRectangle(18, Qt::AlignRight, Qt::AlignBottom).center())
                << singleLineLink
                << singleLineLink << singleLineLink;
        QTest::newRow("click on mirrored link")
                << singleLineText << 240.
                << "horizontalAlignment: Text.AlignLeft; LayoutMirroring.enabled: true"
                << (PointVector() << metrics.characterRectangle(18, Qt::AlignRight, Qt::AlignTop).center())
                << singleLineLink
                << singleLineLink << singleLineLink;
        QTest::newRow("click on center aligned link")
                << singleLineText << 240.
                << "horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter"
                << (PointVector() << metrics.characterRectangle(18, Qt::AlignHCenter, Qt::AlignVCenter).center())
                << singleLineLink
                << singleLineLink << singleLineLink;
        QTest::newRow("click on rich text link")
                << singleLineText << 240.
                << "textFormat: Text.RichText"
                << (PointVector() << metrics.characterRectangle(18).center())
                << singleLineLink
                << singleLineLink << singleLineLink;
        QTest::newRow("click on rich text")
                << singleLineText << 240.
                << "textFormat: Text.RichText"
                << (PointVector() << metrics.characterRectangle(13).center())
                << QString()
                << QString() << QString();
        QTest::newRow("click on bottom right aligned rich text link")
                << singleLineText << 240.
                << "textFormat: Text.RichText; horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignBottom"
                << (PointVector() << metrics.characterRectangle(18, Qt::AlignRight, Qt::AlignBottom).center())
                << singleLineLink
                << singleLineLink << singleLineLink;
        QTest::newRow("click on center aligned rich text link")
                << singleLineText << 240.
                << "textFormat: Text.RichText; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter"
                << (PointVector() << metrics.characterRectangle(18, Qt::AlignHCenter, Qt::AlignVCenter).center())
                << singleLineLink
                << singleLineLink << singleLineLink;
    } {
        const TextMetrics metrics("this text has a li", Qt::ElideRight);
        QTest::newRow("click on right elided link")
                << singleLineText << metrics.width() +  2
                << "elide: Text.ElideRight"
                << (PointVector() << metrics.characterRectangle(17).center())
                << singleLineLink
                << singleLineLink << singleLineLink;
    } {
        const TextMetrics metrics("ink in it", Qt::ElideLeft);
        QTest::newRow("click on left elided link")
                << singleLineText << metrics.width() +  2
                << "elide: Text.ElideLeft"
                << (PointVector() << metrics.characterRectangle(2).center())
                << singleLineLink
                << singleLineLink << singleLineLink;
    } {
        const TextMetrics metrics("this text\nhas multiple\nlines in it");
        QTest::newRow("click on second line")
                << multipleLineText << 240.
                << ""
                << (PointVector() << metrics.characterRectangle(18).center())
                << multipleLineLink
                << multipleLineLink << multipleLineLink;
        QTest::newRow("click on third line")
                << multipleLineText << 240.
                << ""
                << (PointVector() << metrics.characterRectangle(25).center())
                << multipleLineLink
                << multipleLineLink << multipleLineLink;
        QTest::newRow("drag from second line to third")
                << multipleLineText << 240.
                << ""
                << (PointVector()
                    << metrics.characterRectangle(18).center()
                    << metrics.characterRectangle(25).center())
                << multipleLineLink
                << multipleLineLink << multipleLineLink;
        QTest::newRow("click on rich text second line")
                << multipleLineText << 240.
                << "textFormat: Text.RichText"
                << (PointVector() << metrics.characterRectangle(18).center())
                << multipleLineLink
                << multipleLineLink << multipleLineLink;
        QTest::newRow("click on rich text third line")
                << multipleLineText << 240.
                << "textFormat: Text.RichText"
                << (PointVector() << metrics.characterRectangle(25).center())
                << multipleLineLink
                << multipleLineLink << multipleLineLink;
        QTest::newRow("drag rich text from second line to third")
                << multipleLineText << 240.
                << "textFormat: Text.RichText"
                << (PointVector()
                    << metrics.characterRectangle(18).center()
                    << metrics.characterRectangle(25).center())
                << multipleLineLink
                << multipleLineLink << multipleLineLink;
    } {
        const TextMetrics metrics("this text has a nested link in it");
        QTest::newRow("click on left outer link")
                << nestedText << 240.
                << ""
                << (PointVector() << metrics.characterRectangle(22).center())
                << outerLink
                << outerLink << outerLink;
        QTest::newRow("click on right outer link")
                << nestedText << 240.
                << ""
                << (PointVector() << metrics.characterRectangle(27).center())
                << outerLink
                << outerLink << outerLink;
        QTest::newRow("click on inner link left")
                << nestedText << 240.
                << ""
                << (PointVector() << metrics.characterRectangle(23).center())
                << innerLink
                << innerLink << innerLink;
        QTest::newRow("click on inner link right")
                << nestedText << 240.
                << ""
                << (PointVector() << metrics.characterRectangle(26).center())
                << innerLink
                << innerLink << innerLink;
        QTest::newRow("drag from inner to outer link")
                << nestedText << 240.
                << ""
                << (PointVector()
                    << metrics.characterRectangle(25).center()
                    << metrics.characterRectangle(30).center())
                << QString()
                << innerLink << outerLink;
        QTest::newRow("drag from outer to inner link")
                << nestedText << 240.
                << ""
                << (PointVector()
                    << metrics.characterRectangle(30).center()
                    << metrics.characterRectangle(25).center())
                << QString()
                << outerLink << innerLink;
        QTest::newRow("click on left outer rich text link")
                << nestedText << 240.
                << "textFormat: Text.RichText"
                << (PointVector() << metrics.characterRectangle(22).center())
                << outerLink
                << outerLink << outerLink;
        QTest::newRow("click on right outer rich text link")
                << nestedText << 240.
                << "textFormat: Text.RichText"
                << (PointVector() << metrics.characterRectangle(27).center())
                << outerLink
                << outerLink << outerLink;
        QTest::newRow("click on inner rich text link left")
                << nestedText << 240.
                << "textFormat: Text.RichText"
                << (PointVector() << metrics.characterRectangle(23).center())
                << innerLink
                << innerLink << innerLink;
        QTest::newRow("click on inner rich text link right")
                << nestedText << 240.
                << "textFormat: Text.RichText"
                << (PointVector() << metrics.characterRectangle(26).center())
                << innerLink
                << innerLink << innerLink;
        QTest::newRow("drag from inner to outer rich text link")
                << nestedText << 240.
                << "textFormat: Text.RichText"
                << (PointVector()
                    << metrics.characterRectangle(25).center()
                    << metrics.characterRectangle(30).center())
                << QString()
                << innerLink << outerLink;
        QTest::newRow("drag from outer to inner rich text link")
                << nestedText << 240.
                << "textFormat: Text.RichText"
                << (PointVector()
                    << metrics.characterRectangle(30).center()
                    << metrics.characterRectangle(25).center())
                << QString()
                << outerLink << innerLink;
    }
}

void tst_qquicktext::linkInteraction()
{
    QFETCH(QString, text);
    QFETCH(qreal, width);
    QFETCH(QString, bindings);
    QFETCH(PointVector, mousePositions);
    QFETCH(QString, clickedLink);
    QFETCH(QString, hoverEnterLink);
    QFETCH(QString, hoverMoveLink);

    QString componentStr =
            "import QtQuick 2.2\nText {\n"
                "width: " + QString::number(width) + "\n"
                "height: 320\n"
                "text: \"" + text + "\"\n"
                "" + bindings + "\n"
            "}";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

    QVERIFY(textObject != nullptr);

    LinkTest test;
    QObject::connect(textObject, SIGNAL(linkActivated(QString)), &test, SLOT(linkClicked(QString)));
    QObject::connect(textObject, SIGNAL(linkHovered(QString)), &test, SLOT(linkHovered(QString)));

    QVERIFY(mousePositions.size() > 0);

    QPointF mousePosition = mousePositions.first();
    auto globalPos = textObject->mapToGlobal(mousePosition);
    {
        QHoverEvent he(QEvent::HoverEnter, mousePosition, globalPos, QPointF());
        static_cast<EventSender*>(static_cast<QQuickItem*>(textObject))->sendEvent(&he);

        QMouseEvent me(QEvent::MouseButtonPress, mousePosition, globalPos, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        static_cast<EventSender*>(static_cast<QQuickItem*>(textObject))->sendEvent(&me);
    }

    QCOMPARE(test.hoveredLink, hoverEnterLink);
    QCOMPARE(textObject->hoveredLink(), hoverEnterLink);
    QCOMPARE(textObject->linkAt(mousePosition.x(), mousePosition.y()), hoverEnterLink);

    for (int i = 1; i < mousePositions.size(); ++i) {
        mousePosition = mousePositions.at(i);
        auto globalPos = textObject->mapToGlobal(mousePosition);

        QHoverEvent he(QEvent::HoverMove, mousePosition, globalPos, QPointF());
        static_cast<EventSender*>(static_cast<QQuickItem*>(textObject))->sendEvent(&he);

        QMouseEvent me(QEvent::MouseMove, mousePosition, globalPos, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        static_cast<EventSender*>(static_cast<QQuickItem*>(textObject))->sendEvent(&me);
    }

    QCOMPARE(test.hoveredLink, hoverMoveLink);
    QCOMPARE(textObject->hoveredLink(), hoverMoveLink);
    QCOMPARE(textObject->linkAt(mousePosition.x(), mousePosition.y()), hoverMoveLink);

    {
        QHoverEvent he(QEvent::HoverLeave, mousePosition, globalPos, QPointF());
        static_cast<EventSender*>(static_cast<QQuickItem*>(textObject))->sendEvent(&he);

        QMouseEvent me(QEvent::MouseButtonRelease, mousePosition, globalPos, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        static_cast<EventSender*>(static_cast<QQuickItem*>(textObject))->sendEvent(&me);
    }

    QCOMPARE(test.clickedLink, clickedLink);
    QCOMPARE(test.hoveredLink, QString());
    QCOMPARE(textObject->hoveredLink(), QString());
    QCOMPARE(textObject->linkAt(-1, -1), QString());
}

void tst_qquicktext::baseUrl()
{
    QUrl localUrl("file:///tests/text.qml");
    QUrl remoteUrl("http://www.qt-project.org/test.qml");

    QQmlComponent textComponent(&engine);
    textComponent.setData("import QtQuick 2.0\n Text {}", localUrl);
    QQuickText *textObject = qobject_cast<QQuickText *>(textComponent.create());
    QVERIFY2(textObject, qPrintable(textComponent.errorString()));

    QCOMPARE(textObject->baseUrl(), localUrl);

    QSignalSpy spy(textObject, SIGNAL(baseUrlChanged()));

    textObject->setBaseUrl(localUrl);
    QCOMPARE(textObject->baseUrl(), localUrl);
    QCOMPARE(spy.size(), 0);

    textObject->setBaseUrl(remoteUrl);
    QCOMPARE(textObject->baseUrl(), remoteUrl);
    QCOMPARE(spy.size(), 1);

    textObject->resetBaseUrl();
    QCOMPARE(textObject->baseUrl(), localUrl);
    QCOMPARE(spy.size(), 2);
}

void tst_qquicktext::embeddedImages_data()
{
    // Cancel some mess left by clipRectOutsideViewportDynamicallyChanged():
    qmlClearTypeRegistrations();

    QTest::addColumn<QUrl>("qmlfile");
    QTest::addColumn<QString>("error");
    QTest::addColumn<QSize>("expectedImageSize");

    QTest::newRow("local") << testFileUrl("embeddedImagesLocal.qml") << "" << QSize(100, 100);
    QTest::newRow("local-error") << testFileUrl("embeddedImagesLocalError.qml")
        << testFileUrl("embeddedImagesLocalError.qml").toString()+":3:1: QML (QQuick)?Text: Cannot open: " + testFileUrl("http/notexists.png").toString()
         << QSize();
    QTest::newRow("local-relative") << testFileUrl("embeddedImagesLocalRelative.qml") << "" << QSize(100, 100);
    QTest::newRow("remote") << testFileUrl("embeddedImagesRemote.qml") << "" << QSize(100, 100);
    QTest::newRow("remote-error") << testFileUrl("embeddedImagesRemoteError.qml")
                                  << testFileUrl("embeddedImagesRemoteError.qml").toString()+":3:1: QML (QQuick)?Text: Error transferring {{ServerBaseUrl}}/notexists.png - server replied: Not found"
                                   << QSize();
    QTest::newRow("remote-relative") << testFileUrl("embeddedImagesRemoteRelative.qml") << "" << QSize(100, 100);
    QTest::newRow("resource") << testFileUrl("embeddedImageResource.qml") << "" << QSize(16, 16);
}

void tst_qquicktext::embeddedImages()
{
    // Tests QTBUG-9900, QTBUG-125526

    QFETCH(QUrl, qmlfile);
    QFETCH(QString, error);
    QFETCH(QSize, expectedImageSize);

    TestHTTPServer server;
    QVERIFY2(server.listen(), qPrintable(server.errorString()));
    server.serveDirectory(testFile("http"));
    error.replace(QStringLiteral("{{ServerBaseUrl}}"), server.baseUrl().toString());

    if (!error.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(error.toLatin1()));

    QQuickView view;
    view.rootContext()->setContextProperty(QStringLiteral("serverBaseUrl"), server.baseUrl());
    QVERIFY(QQuickTest::showView(view, qmlfile));
    QQuickText *textObject = qobject_cast<QQuickText*>(view.rootObject());

    QVERIFY(textObject != nullptr);
    QTRY_COMPARE(textObject->resourcesLoading(), 0);

    if (expectedImageSize.isValid()) {
        QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
        QVERIFY(textPrivate != nullptr);
        QVERIFY(textPrivate->extra.isAllocated());
        QTextDocument *doc = textPrivate->extra->doc;
        QVERIFY(doc);
        const auto formats = doc->allFormats();
        const auto it = std::find_if(formats.begin(), formats.end(), [](const auto &format){
            return format.objectType() == QTextFormat::ImageObject;
        });
        QCOMPARE_NE(it, formats.end());
        const QTextImageFormat format = (*it).toImageFormat();
        QImage image = doc->resource(QTextDocument::ImageResource, format.name()).value<QImage>();
        qCDebug(lcTests) << "found image?" << format.name() << image;
        QCOMPARE(image.size(), expectedImageSize);
    } else {
        QCOMPARE(textObject->width(), 16); // default size of QTextDocument broken image icon
        QCOMPARE(textObject->height(), 16);
    }

    // QTextDocument images are cached in QTextDocumentPrivate::cachedResources,
    // so verify that we don't redundantly cache them in QQuickPixmapCache
    QCOMPARE(QQuickPixmapCache::instance()->m_cache.size(), 0);
}

void tst_qquicktext::lineCount()
{
    QScopedPointer<QQuickView> window(createView(testFile("lineCount.qml")));

    QQuickText *myText = window->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != nullptr);

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

    // QTBUG-84458
    myText->resetMaximumLineCount();
    myText->setText("qqqqq\nqqqqq");
    QCOMPARE(myText->lineCount(), 2);
    myText->setText("qqqqq\nqqqqq\nqqqqq");
    QCOMPARE(myText->lineCount(), 3);
    myText->setText("");
    QCOMPARE(myText->lineCount(), 1);

    myText->setText("qqqqq\nqqqqq\nqqqqq");
    QCOMPARE(myText->lineCount(), 3);
    myText->setFontSizeMode(QQuickText::HorizontalFit);
    myText->setText("");
    QCOMPARE(myText->lineCount(), 1);

    myText->setText("qqqqq\nqqqqq\nqqqqq");
    QCOMPARE(myText->lineCount(), 3);
    myText->setFontSizeMode(QQuickText::VerticalFit);
    myText->setText("");
    QCOMPARE(myText->lineCount(), 1);

    myText->setText("qqqqq\nqqqqq\nqqqqq");
    QCOMPARE(myText->lineCount(), 3);
    myText->setFontSizeMode(QQuickText::Fit);
    myText->setText("");
    QCOMPARE(myText->lineCount(), 1);

    QScopedPointer<QQuickView> layoutWindow(createView(testFile("lineLayoutHAlign.qml")));
    QQuickText *lineLaidOut = layoutWindow->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(lineLaidOut != nullptr);

    lineLaidOut->setText("qqqqq\nqqqqq\nqqqqq");
    QCOMPARE(lineLaidOut->lineCount(), 3);
    lineLaidOut->setFontSizeMode(QQuickText::FixedSize);
    lineLaidOut->setText("");
    QCOMPARE(lineLaidOut->lineCount(), 1);
}

void tst_qquicktext::lineHeight()
{
    QScopedPointer<QQuickView> window(createView(testFile("lineHeight.qml")));

    QQuickText *myText = window->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != nullptr);

    QCOMPARE(myText->lineHeight(), qreal(1));
    QCOMPARE(myText->lineHeightMode(), QQuickText::ProportionalHeight);

    qreal h = myText->height();
    QVERIFY(myText->lineCount() != 0);
    const qreal h1stLine = h / myText->lineCount();

    myText->setLineHeight(1.5);
    QCOMPARE(myText->height(), qreal(qCeil(h)) * 1.5);
    QCOMPARE(myText->contentHeight(), qreal(qCeil(h)) * 1.5);

    myText->setLineHeightMode(QQuickText::FixedHeight);
    myText->setLineHeight(20);
    QCOMPARE(myText->height(), myText->lineCount() * 20.0);

    myText->setText("Lorem ipsum sit <b>amet</b>, consectetur adipiscing elit. Integer felis nisl, varius in pretium nec, venenatis non erat. Proin lobortis interdum dictum.");
    myText->setLineHeightMode(QQuickText::ProportionalHeight);
    myText->setLineHeight(1.0);

    qreal h2 = myText->height();
    myText->setLineHeight(2.0);
    QCOMPARE(myText->height(), h2 * 2.0);

    myText->setLineHeightMode(QQuickText::FixedHeight);
    myText->setLineHeight(10);
    QVERIFY(myText->lineCount() > 1);
    QCOMPARE(myText->height(), h1stLine + (myText->lineCount() - 1) * 10.0);
    QCOMPARE(myText->contentHeight(), h1stLine + (myText->lineCount() - 1) * 10.0);
    QCOMPARE(myText->implicitHeight(), h1stLine + (myText->lineCount() - 1) * 10.0);

    myText->setLineHeightMode(QQuickText::ProportionalHeight);
    myText->setLineHeight(0.5);
    const qreal reducedHeight = h1stLine + (myText->lineCount() - 1) * h1stLine * 0.5;
    QVERIFY(qAbs(myText->height() - reducedHeight) < 1.0); // allow a deviation of one pixel because the exact height depends on the font.
    QVERIFY(qAbs(myText->contentHeight() - reducedHeight) < 1.0);
    QVERIFY(qAbs(myText->implicitHeight() - reducedHeight) < 1.0);
}

void tst_qquicktext::implicitSize_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("width");
    QTest::addColumn<QString>("wrap");
    QTest::addColumn<QString>("elide");
    QTest::addColumn<QString>("format");
    QTest::newRow("plain") << "The quick red fox jumped over the lazy brown dog" << "50" << "Text.NoWrap" << "Text.ElideNone" << "Text.PlainText";
    QTest::newRow("richtext") << "<b>The quick red fox jumped over the lazy brown dog</b>" <<" 50" << "Text.NoWrap" << "Text.ElideNone" << "Text.RichText";
    QTest::newRow("styledtext") << "<b>The quick red fox jumped over the lazy brown dog</b>" <<" 50" << "Text.NoWrap" << "Text.ElideNone" << "Text.StyledText";
    QTest::newRow("plain, 0 width") << "The quick red fox jumped over the lazy brown dog" << "0" << "Text.NoWrap" << "Text.ElideNone" << "Text.PlainText";
    QTest::newRow("plain, elide") << "The quick red fox jumped over the lazy brown dog" << "50" << "Text.NoWrap" << "Text.ElideRight" << "Text.PlainText";
    QTest::newRow("plain, 0 width, elide") << "The quick red fox jumped over the lazy brown dog" << "0" << "Text.NoWrap" << "Text.ElideRight" << "Text.PlainText";
    QTest::newRow("richtext, 0 width") << "<b>The quick red fox jumped over the lazy brown dog</b>" <<" 0" << "Text.NoWrap" << "Text.ElideNone" << "Text.RichText";
    QTest::newRow("styledtext, 0 width") << "<b>The quick red fox jumped over the lazy brown dog</b>" <<" 0" << "Text.NoWrap" << "Text.ElideNone" << "Text.StyledText";
    QTest::newRow("plain_wrap") << "The quick red fox jumped over the lazy brown dog" << "50" << "Text.Wrap" << "Text.ElideNone" << "Text.PlainText";
    QTest::newRow("richtext_wrap") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "50" << "Text.Wrap" << "Text.ElideNone" << "Text.RichText";
    QTest::newRow("styledtext_wrap") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "50" << "Text.Wrap" << "Text.ElideNone" << "Text.StyledText";
    QTest::newRow("plain_wrap, 0 width") << "The quick red fox jumped over the lazy brown dog" << "0" << "Text.Wrap" << "Text.ElideNone" << "Text.PlainText";
    QTest::newRow("plain_wrap, elide") << "The quick red fox jumped over the lazy brown dog" << "50" << "Text.Wrap" << "Text.ElideRight" << "Text.PlainText";
    QTest::newRow("plain_wrap, 0 width, elide") << "The quick red fox jumped over the lazy brown dog" << "0" << "Text.Wrap" << "Text.ElideRight" << "Text.PlainText";
    QTest::newRow("richtext_wrap, 0 width") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "0" << "Text.Wrap" << "Text.ElideNone" << "Text.RichText";
    QTest::newRow("styledtext_wrap, 0 width") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "0" << "Text.Wrap" << "Text.ElideNone" << "Text.StyledText";
}

void tst_qquicktext::implicitSize()
{
#ifdef Q_OS_ANDROID
    QSKIP("This segfaults on Android, QTBUG-103096");
#endif

    QFETCH(QString, text);
    QFETCH(QString, width);
    QFETCH(QString, format);
    QFETCH(QString, wrap);
    QFETCH(QString, elide);
    QString componentStr = "import QtQuick 2.0\nText { "
            "property real iWidth: implicitWidth; "
            "text: \"" + text + "\"; "
            "width: " + width + "; "
            "textFormat: " + format + "; "
            "wrapMode: " + wrap + "; "
            "elide: " + elide + "; "
            "maximumLineCount: 2 }";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickText *textObject = qobject_cast<QQuickText*>(object.data());
    QVERIFY2(textObject, qPrintable(textComponent.errorString()));

    QVERIFY(textObject->width() < textObject->implicitWidth());
    QCOMPARE(textObject->height(), textObject->implicitHeight());
    QCOMPARE(textObject->property("iWidth").toReal(), textObject->implicitWidth());

    textObject->resetWidth();
    QCOMPARE(textObject->width(), textObject->implicitWidth());
    QCOMPARE(textObject->height(), textObject->implicitHeight());
}

void tst_qquicktext::implicitSizeMaxLineCount()
{
    QScopedPointer<QQuickText> textObject(new QQuickText);

    textObject->setText("1st line");
    const auto referenceWidth = textObject->implicitWidth();

    textObject->setText(textObject->text() + "\n2nd long long long long long line");
    QCOMPARE_GT(textObject->implicitWidth(), referenceWidth);

    textObject->setMaximumLineCount(1);
    QCOMPARE_EQ(textObject->implicitWidth(), referenceWidth);
}

void tst_qquicktext::dependentImplicitSizes()
{
    QQmlComponent component(&engine, testFile("implicitSizes.qml"));
    QScopedPointer<QObject> object(component.create());
    QVERIFY(object.data());

    QQuickText *reference = object->findChild<QQuickText *>("reference");
    QQuickText *fixedWidthAndHeight = object->findChild<QQuickText *>("fixedWidthAndHeight");
    QQuickText *implicitWidthFixedHeight = object->findChild<QQuickText *>("implicitWidthFixedHeight");
    QQuickText *fixedWidthImplicitHeight = object->findChild<QQuickText *>("fixedWidthImplicitHeight");
    QQuickText *cappedWidthAndHeight = object->findChild<QQuickText *>("cappedWidthAndHeight");
    QQuickText *cappedWidthFixedHeight = object->findChild<QQuickText *>("cappedWidthFixedHeight");
    QQuickText *fixedWidthCappedHeight = object->findChild<QQuickText *>("fixedWidthCappedHeight");
    QQuickText *defaultWidthNoWrapNoElide = object->findChild<QQuickText *>("defaultWidthNoWrapNoElide");

    QVERIFY(reference);
    QVERIFY(fixedWidthAndHeight);
    QVERIFY(implicitWidthFixedHeight);
    QVERIFY(fixedWidthImplicitHeight);
    QVERIFY(cappedWidthAndHeight);
    QVERIFY(cappedWidthFixedHeight);
    QVERIFY(fixedWidthCappedHeight);

    QCOMPARE(reference->width(), reference->implicitWidth());
    QCOMPARE(reference->height(), reference->implicitHeight());

    QVERIFY(fixedWidthAndHeight->width() < fixedWidthAndHeight->implicitWidth());
    QVERIFY(fixedWidthAndHeight->height() < fixedWidthAndHeight->implicitHeight());
    QCOMPARE(fixedWidthAndHeight->implicitWidth(), reference->implicitWidth());
    QVERIFY(fixedWidthAndHeight->implicitHeight() > reference->implicitHeight());

    QCOMPARE(implicitWidthFixedHeight->width(), implicitWidthFixedHeight->implicitWidth());
    QVERIFY(implicitWidthFixedHeight->height() < implicitWidthFixedHeight->implicitHeight());
    QCOMPARE(implicitWidthFixedHeight->implicitWidth(), reference->implicitWidth());
    QCOMPARE(implicitWidthFixedHeight->implicitHeight(), reference->implicitHeight());

    QVERIFY(fixedWidthImplicitHeight->width() < fixedWidthImplicitHeight->implicitWidth());
    QCOMPARE(fixedWidthImplicitHeight->height(), fixedWidthImplicitHeight->implicitHeight());
    QCOMPARE(fixedWidthImplicitHeight->implicitWidth(), reference->implicitWidth());
    QCOMPARE(fixedWidthImplicitHeight->implicitHeight(), fixedWidthAndHeight->implicitHeight());

    QVERIFY(cappedWidthAndHeight->width() < cappedWidthAndHeight->implicitWidth());
    QVERIFY(cappedWidthAndHeight->height() < cappedWidthAndHeight->implicitHeight());
    QCOMPARE(cappedWidthAndHeight->implicitWidth(), reference->implicitWidth());
    QCOMPARE(cappedWidthAndHeight->implicitHeight(), fixedWidthAndHeight->implicitHeight());

    QVERIFY(cappedWidthFixedHeight->width() < cappedWidthAndHeight->implicitWidth());
    QVERIFY(cappedWidthFixedHeight->height() < cappedWidthFixedHeight->implicitHeight());
    QCOMPARE(cappedWidthFixedHeight->implicitWidth(), reference->implicitWidth());
    QCOMPARE(cappedWidthFixedHeight->implicitHeight(), fixedWidthAndHeight->implicitHeight());

    QVERIFY(fixedWidthCappedHeight->width() < fixedWidthCappedHeight->implicitWidth());
    QVERIFY(fixedWidthCappedHeight->height() < fixedWidthCappedHeight->implicitHeight());
    QCOMPARE(fixedWidthCappedHeight->implicitWidth(), reference->implicitWidth());
    QCOMPARE(fixedWidthCappedHeight->implicitHeight(), fixedWidthAndHeight->implicitHeight());

    // QTBUG-129143
    // If we don't define width or height, width should be the same as implicitWidth
    QCOMPARE(defaultWidthNoWrapNoElide->width(), defaultWidthNoWrapNoElide->implicitWidth());
    QCOMPARE(defaultWidthNoWrapNoElide->height(), defaultWidthNoWrapNoElide->implicitHeight());
    QCOMPARE(fixedWidthCappedHeight->implicitWidth(), reference->implicitWidth());
}

void tst_qquicktext::contentSize()
{
    QString componentStr = "import QtQuick 2.0\nText { width: 75; height: 16; font.pixelSize: 10 }";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickText *textObject = qobject_cast<QQuickText *>(object.data());
    QVERIFY2(textObject, qPrintable(textComponent.errorString()));

    QSignalSpy spySize(textObject, SIGNAL(contentSizeChanged()));
    QSignalSpy spyWidth(textObject, SIGNAL(contentWidthChanged(qreal)));
    QSignalSpy spyHeight(textObject, SIGNAL(contentHeightChanged(qreal)));

    textObject->setText("The quick red fox jumped over the lazy brown dog");

    QVERIFY(textObject->contentWidth() > textObject->width());
    QVERIFY(textObject->contentHeight() < textObject->height());
    QCOMPARE(spySize.size(), 1);
    QCOMPARE(spyWidth.size(), 1);
    QCOMPARE(spyHeight.size(), 0);

    textObject->setWrapMode(QQuickText::WordWrap);
    QVERIFY(textObject->contentWidth() <= textObject->width());
    QVERIFY(textObject->contentHeight() > textObject->height());
    QCOMPARE(spySize.size(), 2);
    QCOMPARE(spyWidth.size(), 2);
    QCOMPARE(spyHeight.size(), 1);

    textObject->setElideMode(QQuickText::ElideRight);
    QVERIFY(textObject->contentWidth() <= textObject->width());
    QVERIFY(textObject->contentHeight() < textObject->height());
    QCOMPARE(spySize.size(), 3);
    QCOMPARE(spyWidth.size(), 3);
    QCOMPARE(spyHeight.size(), 2);
    int spyCount = 3;
    qreal elidedWidth = textObject->contentWidth();

    textObject->setText("The quickredfoxjumpedoverthe lazy brown dog");
    QVERIFY(textObject->contentWidth() <= textObject->width());
    QVERIFY(textObject->contentHeight() < textObject->height());
    // this text probably won't have the same elided width, but it's not guaranteed.
    if (textObject->contentWidth() != elidedWidth)
        QCOMPARE(spySize.size(), ++spyCount);
    else
        QCOMPARE(spySize.size(), spyCount);

    textObject->setElideMode(QQuickText::ElideNone);
    QVERIFY(textObject->contentWidth() > textObject->width());
    QVERIFY(textObject->contentHeight() > textObject->height());
    QCOMPARE(spySize.size(), ++spyCount);
    QCOMPARE(spyWidth.size(), spyCount);
    QCOMPARE(spyHeight.size(), 3);
}

void tst_qquicktext::geometryChanged()
{
    // Test that text is re-laid out when the geometry of the item by verifying changes in content
    // size.  Implicit width is also tested as that in combination with item geometry provides a
    // reference for expected content sizes.

    QString componentStr = "import QtQuick 2.0\nText { font.family: \"__Qt__Box__Engine__\"; font.pixelSize: 10 }";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickText *textObject = qobject_cast<QQuickText *>(object.data());
    QVERIFY2(textObject, qPrintable(textComponent.errorString()));

    const qreal implicitHeight = textObject->implicitHeight();

    const qreal widths[] = { 100, 2000, 3000, -100, 100 };
    const qreal heights[] = { implicitHeight, 2000, 3000, -implicitHeight, implicitHeight };

    QCOMPARE(textObject->implicitWidth(), 0.);
    QVERIFY(implicitHeight > 0.);
    QCOMPARE(textObject->width(), textObject->implicitWidth());
    QCOMPARE(textObject->height(), implicitHeight);
    QCOMPARE(textObject->contentWidth(), textObject->implicitWidth());
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setText("The quick red fox jumped over the lazy brown dog");

    const qreal implicitWidth = textObject->implicitWidth();

    QVERIFY(implicitWidth > 0.);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), textObject->implicitWidth());
    QCOMPARE(textObject->height(), textObject->implicitHeight());
    QCOMPARE(textObject->contentWidth(), textObject->implicitWidth());
    QCOMPARE(textObject->contentHeight(), textObject->implicitHeight());

    // Changing the geometry with no eliding, or wrapping doesn't change the content size.
    for (int i = 0; i < 5; ++i) {
        textObject->setWidth(widths[i]);
        QCOMPARE(textObject->implicitWidth(), implicitWidth);
        QCOMPARE(textObject->implicitHeight(), implicitHeight);
        QCOMPARE(textObject->width(), widths[i]);
        QCOMPARE(textObject->height(), implicitHeight);
        QCOMPARE(textObject->contentWidth(), implicitWidth);
        QCOMPARE(textObject->contentHeight(), implicitHeight);
    }

    // With eliding enabled the content width is bounded to the item width, but is never
    // larger than the implicit width.
    textObject->setElideMode(QQuickText::ElideRight);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), 100.);
    QCOMPARE(textObject->height(), implicitHeight);
    QVERIFY(textObject->contentWidth() <= 100.);
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setWidth(2000.);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), 2000.);
    QCOMPARE(textObject->height(), implicitHeight);
    QCOMPARE(textObject->contentWidth(), implicitWidth);
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setWidth(3000.);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), 3000.);
    QCOMPARE(textObject->height(), implicitHeight);
    QCOMPARE(textObject->contentWidth(), implicitWidth);
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setWidth(-100);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), -100.);
    QCOMPARE(textObject->height(), implicitHeight);
    QCOMPARE(textObject->contentWidth(), 0.);
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setWidth(100.);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), 100.);
    QCOMPARE(textObject->height(), implicitHeight);
    QVERIFY(textObject->contentWidth() <= 100.);
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    // With wrapping enabled the implicit height changes with the width.
    textObject->setElideMode(QQuickText::ElideNone);
    textObject->setWrapMode(QQuickText::Wrap);
    const qreal wrappedImplicitHeight = textObject->implicitHeight();

    QVERIFY(wrappedImplicitHeight > implicitHeight);

    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->width(), 100.);
    QCOMPARE(textObject->height(), wrappedImplicitHeight);
    QVERIFY(textObject->contentWidth() <= 100.);
    QCOMPARE(textObject->contentHeight(), wrappedImplicitHeight);

    textObject->setWidth(2000.);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), 2000.);
    QCOMPARE(textObject->height(), implicitHeight);
    QCOMPARE(textObject->contentWidth(), implicitWidth);
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setWidth(3000.);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), 3000.);
    QCOMPARE(textObject->height(), implicitHeight);
    QCOMPARE(textObject->contentWidth(), implicitWidth);
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setWidth(-100);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), -100.);
    QCOMPARE(textObject->height(), implicitHeight);
    QCOMPARE(textObject->contentWidth(), implicitWidth);    // 0 or negative width item won't wrap.
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setWidth(100.);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), wrappedImplicitHeight);
    QCOMPARE(textObject->width(), 100.);
    QCOMPARE(textObject->height(), wrappedImplicitHeight);
    QVERIFY(textObject->contentWidth() <= 100.);
    QCOMPARE(textObject->contentHeight(), wrappedImplicitHeight);

    // With no eliding or maximum line count the content height is the same as the implicit height.
    for (int i = 0; i < 5; ++i) {
        textObject->setHeight(heights[i]);
        QCOMPARE(textObject->implicitWidth(), implicitWidth);
        QCOMPARE(textObject->implicitHeight(), wrappedImplicitHeight);
        QCOMPARE(textObject->width(), 100.);
        QCOMPARE(textObject->height(), heights[i]);
        QVERIFY(textObject->contentWidth() <= 100.);
        QCOMPARE(textObject->contentHeight(), wrappedImplicitHeight);
    }

    // The implicit height is unaffected by eliding but the content height will change.
    textObject->setElideMode(QQuickText::ElideRight);

    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), wrappedImplicitHeight);
    QCOMPARE(textObject->width(), 100.);
    QCOMPARE(textObject->height(), implicitHeight);
    QVERIFY(textObject->contentWidth() <= 100.);
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setHeight(2000);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), wrappedImplicitHeight);
    QCOMPARE(textObject->width(), 100.);
    QCOMPARE(textObject->height(), 2000.);
    QVERIFY(textObject->contentWidth() <= 100.);
    QCOMPARE(textObject->contentHeight(), wrappedImplicitHeight);

    textObject->setHeight(3000);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), wrappedImplicitHeight);
    QCOMPARE(textObject->width(), 100.);
    QCOMPARE(textObject->height(), 3000.);
    QVERIFY(textObject->contentWidth() <= 100.);
    QCOMPARE(textObject->contentHeight(), wrappedImplicitHeight);

    textObject->setHeight(-implicitHeight);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), wrappedImplicitHeight);
    QCOMPARE(textObject->width(), 100.);
    QCOMPARE(textObject->height(), -implicitHeight);
    QVERIFY(textObject->contentWidth() <= 0.);
    QCOMPARE(textObject->contentHeight(), implicitHeight);  // content height is never less than font height. seems a little odd in this instance.

    textObject->setHeight(implicitHeight);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), wrappedImplicitHeight);
    QCOMPARE(textObject->width(), 100.);
    QCOMPARE(textObject->height(), implicitHeight);
    QVERIFY(textObject->contentWidth() <= 100.);
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    // Varying the height with a maximum line count but no eliding won't affect the content height.
    textObject->setElideMode(QQuickText::ElideNone);
    textObject->setMaximumLineCount(2);
    textObject->resetHeight();

    const qreal maxLineCountImplicitHeight = textObject->implicitHeight();
    QVERIFY(maxLineCountImplicitHeight > implicitHeight);
    QVERIFY(maxLineCountImplicitHeight < wrappedImplicitHeight);

    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->width(), 100.);
    QCOMPARE(textObject->height(), maxLineCountImplicitHeight);
    QVERIFY(textObject->contentWidth() <= 100.);
    QCOMPARE(textObject->contentHeight(), maxLineCountImplicitHeight);

    for (int i = 0; i < 5; ++i) {
        textObject->setHeight(heights[i]);
        QCOMPARE(textObject->implicitWidth(), implicitWidth);
        QCOMPARE(textObject->implicitHeight(), maxLineCountImplicitHeight);
        QCOMPARE(textObject->width(), 100.);
        QCOMPARE(textObject->height(), heights[i]);
        QVERIFY(textObject->contentWidth() <= 100.);
        QCOMPARE(textObject->contentHeight(), maxLineCountImplicitHeight);
    }

    // Varying the width with a maximum line count won't increase the implicit height beyond the
    // height of the maximum number of lines.
    textObject->setWidth(2000.);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), 2000.);
    QCOMPARE(textObject->height(), implicitHeight);
    QCOMPARE(textObject->contentWidth(), implicitWidth);
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setWidth(3000.);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), 3000.);
    QCOMPARE(textObject->height(), implicitHeight);
    QCOMPARE(textObject->contentWidth(), implicitWidth);
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setWidth(-100);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), implicitHeight);
    QCOMPARE(textObject->width(), -100.);
    QCOMPARE(textObject->height(), implicitHeight);
    QCOMPARE(textObject->contentWidth(), implicitWidth);    // 0 or negative width item won't wrap.
    QCOMPARE(textObject->contentHeight(), implicitHeight);

    textObject->setWidth(50.);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), maxLineCountImplicitHeight);
    QCOMPARE(textObject->width(), 50.);
    QCOMPARE(textObject->height(), implicitHeight);
    QVERIFY(textObject->contentWidth() <= 50.);
    QCOMPARE(textObject->contentHeight(), maxLineCountImplicitHeight);

    textObject->setWidth(100.);
    QCOMPARE(textObject->implicitWidth(), implicitWidth);
    QCOMPARE(textObject->implicitHeight(), maxLineCountImplicitHeight);
    QCOMPARE(textObject->width(), 100.);
    QCOMPARE(textObject->height(), implicitHeight);
    QVERIFY(textObject->contentWidth() <= 100.);
    QCOMPARE(textObject->contentHeight(), maxLineCountImplicitHeight);
}

void tst_qquicktext::implicitSizeBinding_data()
{
    implicitSize_data();
}

void tst_qquicktext::implicitSizeBinding()
{
    QFETCH(QString, text);
    QFETCH(QString, wrap);
    QFETCH(QString, format);
    QString componentStr = "import QtQuick 2.0\nText { text: \"" + text + "\"; width: implicitWidth; height: implicitHeight; wrapMode: " + wrap + "; textFormat: " + format + " }";

    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickText *textObject = qobject_cast<QQuickText *>(object.data());
    QVERIFY2(textObject, qPrintable(textComponent.errorString()));

    QCOMPARE(textObject->width(), textObject->implicitWidth());
    QCOMPARE(textObject->height(), textObject->implicitHeight());

    textObject->resetWidth();
    QCOMPARE(textObject->width(), textObject->implicitWidth());
    QCOMPARE(textObject->height(), textObject->implicitHeight());

    textObject->resetHeight();
    QCOMPARE(textObject->width(), textObject->implicitWidth());
    QCOMPARE(textObject->height(), textObject->implicitHeight());
}

void tst_qquicktext::boundingRect_data()
{
    QTest::addColumn<QString>("format");
    QTest::newRow("PlainText") << "Text.PlainText";
    QTest::newRow("StyledText") << "Text.StyledText";
    QTest::newRow("RichText") << "Text.RichText";
}

void tst_qquicktext::boundingRect()
{
    QFETCH(QString, format);

    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n Text { textFormat:" + format.toUtf8() + "}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickText *text = qobject_cast<QQuickText *>(object.data());
    QVERIFY(text);

    QCOMPARE(text->boundingRect().x(), qreal(0));
    QCOMPARE(text->boundingRect().y(), qreal(0));
    QCOMPARE(text->boundingRect().width(), qreal(0));
    QCOMPARE(text->boundingRect().height(), qreal(qCeil(QFontMetricsF(text->font()).height())));

    text->setText("Hello World");

    QTextLayout layout(text->text());
    layout.setFont(text->font());

    if (!qmlDisableDistanceField()) {
        QTextOption option;
        option.setUseDesignMetrics(true);
        layout.setTextOption(option);
    }
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    QCOMPARE(text->boundingRect().x(), qreal(0));
    QCOMPARE(text->boundingRect().y(), qreal(0));
    QCOMPARE(text->boundingRect().width(), line.naturalTextWidth());

    QFontMetricsF fontMetrics(QGuiApplication::font());
    qreal leading = fontMetrics.leading();
    qreal ascent = fontMetrics.ascent();
    qreal descent = fontMetrics.descent();

    bool leadingOverflow = text->textFormat() == QQuickText::RichText && qCeil(ascent + descent) < qCeil(ascent + descent + leading);
    if (leadingOverflow)
        QEXPECT_FAIL("", "See QTBUG-82954", Continue);
    QCOMPARE(text->boundingRect().height(), line.height());

    // the size of the bounding rect shouldn't be bounded by the size of item.
    text->setWidth(text->width() / 2);
    QCOMPARE(text->boundingRect().x(), qreal(0));
    QCOMPARE(text->boundingRect().y(), qreal(0));
    QCOMPARE(text->boundingRect().width(), line.naturalTextWidth());

    if (leadingOverflow)
        QEXPECT_FAIL("", "See QTBUG-82954", Continue);
    QCOMPARE(text->boundingRect().height(), line.height());

    text->setHeight(text->height() * 2);
    QCOMPARE(text->boundingRect().x(), qreal(0));
    QCOMPARE(text->boundingRect().y(), qreal(0));
    QCOMPARE(text->boundingRect().width(), line.naturalTextWidth());

    if (leadingOverflow)
        QEXPECT_FAIL("", "See QTBUG-82954", Continue);
    QCOMPARE(text->boundingRect().height(), line.height());

    text->setHAlign(QQuickText::AlignRight);
    QCOMPARE(text->boundingRect().x(), text->width() - line.naturalTextWidth());
    QCOMPARE(text->boundingRect().y(), qreal(0));
    QCOMPARE(text->boundingRect().width(), line.naturalTextWidth());

    if (leadingOverflow)
        QEXPECT_FAIL("", "See QTBUG-82954", Continue);
    QCOMPARE(text->boundingRect().height(), line.height());

    QQuickItemPrivate::get(text)->setLayoutMirror(true);
    QCOMPARE(text->boundingRect().x(), qreal(0));
    QCOMPARE(text->boundingRect().y(), qreal(0));
    QCOMPARE(text->boundingRect().width(), line.naturalTextWidth());

    if (leadingOverflow)
        QEXPECT_FAIL("", "See QTBUG-82954", Continue);
    QCOMPARE(text->boundingRect().height(), line.height());

    text->setHAlign(QQuickText::AlignLeft);
    QCOMPARE(text->boundingRect().x(), text->width() - line.naturalTextWidth());
    QCOMPARE(text->boundingRect().y(), qreal(0));
    QCOMPARE(text->boundingRect().width(), line.naturalTextWidth());

    if (leadingOverflow)
        QEXPECT_FAIL("", "See QTBUG-82954", Continue);
    QCOMPARE(text->boundingRect().height(), line.height());

    text->setWrapMode(QQuickText::Wrap);
    QCOMPARE(text->boundingRect().right(), text->width());
    QCOMPARE(text->boundingRect().y(), qreal(0));
    QVERIFY(text->boundingRect().width() < line.naturalTextWidth());
    QVERIFY(text->boundingRect().height() > line.height());

    text->setVAlign(QQuickText::AlignBottom);
    QCOMPARE(text->boundingRect().right(), text->width());
    QCOMPARE(text->boundingRect().bottom(), text->height());
    QVERIFY(text->boundingRect().width() < line.naturalTextWidth());
    QVERIFY(text->boundingRect().height() > line.height());
}

void tst_qquicktext::clipRect()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n Text {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickText *text = qobject_cast<QQuickText *>(object.data());
    QVERIFY(text);

    QTextLayout layout;
    layout.setFont(text->font());

    QCOMPARE(text->clipRect().x(), qreal(0));
    QCOMPARE(text->clipRect().y(), qreal(0));
    QCOMPARE(text->clipRect().width(), text->width());
    QCOMPARE(text->clipRect().height(), text->height());

    text->setText("Hello World");

    QCOMPARE(text->clipRect().x(), qreal(0));
    QCOMPARE(text->clipRect().y(), qreal(0));
    QCOMPARE(text->clipRect().width(), text->width());
    QCOMPARE(text->clipRect().height(), text->height());

    // Clip rect follows the item not content dimensions.
    text->setWidth(text->width() / 2);
    QCOMPARE(text->clipRect().x(), qreal(0));
    QCOMPARE(text->clipRect().y(), qreal(0));
    QCOMPARE(text->clipRect().width(), text->width());
    QCOMPARE(text->clipRect().height(), text->height());

    text->setHeight(text->height() * 2);
    QCOMPARE(text->clipRect().x(), qreal(0));
    QCOMPARE(text->clipRect().y(), qreal(0));
    QCOMPARE(text->clipRect().width(), text->width());
    QCOMPARE(text->clipRect().height(), text->height());

    // Setting a style adds a small amount of padding to the clip rect.
    text->setStyle(QQuickText::Outline);
    QCOMPARE(text->clipRect().x(), qreal(-1));
    QCOMPARE(text->clipRect().y(), qreal(0));
    QCOMPARE(text->clipRect().width(), text->width() + 2);
    QCOMPARE(text->clipRect().height(), text->height() + 2);
}

void tst_qquicktext::largeTextObservesViewport_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QQuickText::TextFormat>("textFormat");
    QTest::addColumn<int>("linesAboveViewport");
    QTest::addColumn<bool>("parentIsViewport");

    QString text;
    {
        QStringList lines;
        // "line 100" is 8 characters; many lines are longer, some are shorter
        // so we populate 1250 lines, 11389 characters
        const int lineCount = QQuickTextPrivate::largeTextSizeThreshold / 8;
        lines.reserve(lineCount);
        for (int i = 0; i < lineCount; ++i)
            lines << QLatin1String("line ") + QString::number(i);
        text = lines.join('\n');
    }
    Q_ASSERT(text.size() > QQuickTextPrivate::largeTextSizeThreshold);

    // by default, the root item acts as the viewport:
    // QSGInternalTextNode doesn't populate lines of text beyond the bottom of the window
    QTest::newRow("default plain text") << text << QQuickText::PlainText << 0 << false;
    // make the rectangle into a viewport item, and move the text upwards:
    // QSGInternalTextNode doesn't populate lines of text beyond the bottom of the viewport rectangle
    QTest::newRow("clipped plain text") << text << QQuickText::PlainText << 10 << true;

    {
        QStringList lines;
        // "line 100" is 8 characters; many lines are longer, some are shorter
        // so we populate 1250 lines, 11389 characters
        const int lineCount = QQuickTextPrivate::largeTextSizeThreshold / 8;
        lines.reserve(lineCount);
        for (int i = 0; i < lineCount; ++i) {
            if (i > 0 && i % 50 == 0)
                lines << QLatin1String("<h1>chapter ") + QString::number(i / 50) + QLatin1String("</h1>");
            lines << QLatin1String("<p>line ") + QString::number(i) + QLatin1String("</p>");
        }
        text = lines.join('\n');
    }
    Q_ASSERT(text.size() > QQuickTextPrivate::largeTextSizeThreshold);

    // by default, the root item acts as the viewport:
    // QQuickTextNodeEngine doesn't populate blocks beyond the bottom of the window
    QTest::newRow("default styled text") << text << QQuickText::StyledText << 0 << false;
    // make the rectangle into a viewport item, and move the text upwards:
    // QQuickTextNodeEngine doesn't populate blocks that don't intersect the viewport rectangle
    QTest::newRow("clipped styled text") << text << QQuickText::StyledText << 10 << true;
    // get a chapter heading into the viewport
    QTest::newRow("heading visible") << text << QQuickText::StyledText << 90 << true;
}

void tst_qquicktext::largeTextObservesViewport()
{
    QFETCH(QString, text);
    QFETCH(QQuickText::TextFormat, textFormat);
    QFETCH(int, linesAboveViewport);
    QFETCH(bool, parentIsViewport);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("viewport.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QQuickText *textItem = window.rootObject()->findChild<QQuickText*>();
    QVERIFY(textItem);
    QQuickItem *viewportItem = textItem->parentItem();
    QQuickTextPrivate *textPriv = QQuickTextPrivate::get(textItem);
    QSGInternalTextNode *node = static_cast<QSGInternalTextNode *>(textPriv->paintNode);
    QFontMetricsF fm(textItem->font());
    const qreal expectedTextHeight = (parentIsViewport ? viewportItem->height() : window.height() - viewportItem->y());
    const qreal lineSpacing = qCeil(fm.height());
    // A paragraph break is the same as an extra line break; so since our "lines" are paragraphs in StyledText,
    // visually, with StyledText we skip down 10 "lines", but the first paragraph you see says "line 5".
    // It's OK anyway for the test, because QSGTextNode::addTextLayout() treats the paragraph breaks like lines of text.
    const int expectedLastLine = linesAboveViewport + int(expectedTextHeight / lineSpacing);

    viewportItem->setFlag(QQuickItem::ItemIsViewport, parentIsViewport);
    textItem->setY(lineSpacing * -linesAboveViewport);
    textItem->setTextFormat(textFormat);
    textItem->setText(text);
    qCDebug(lcTests) << "text size" << textItem->text().size() << "lines" << textItem->lineCount() << "font" << textItem->font();
    Q_ASSERT(textItem->text().size() > QQuickTextPrivate::largeTextSizeThreshold);
    QVERIFY(textItem->flags().testFlag(QQuickItem::ItemObservesViewport)); // large text sets this flag automatically
    QCOMPARE(textItem->viewportItem(), parentIsViewport ? viewportItem : viewportItem->parentItem());
    QTRY_VERIFY(node->renderedLineRange().first >= 0); // wait for rendering
    auto renderedLineRange = node->renderedLineRange();
    qCDebug(lcTests) << "first line rendered" << renderedLineRange.first
                     << "expected" << linesAboveViewport
                     << "first line past viewport" << renderedLineRange.second
                     << "expected last line" << expectedLastLine
                     << "based on available height" << expectedTextHeight
                     << "and line height" << lineSpacing << "rather than spacing" << fm.lineSpacing();
//    QTest::qWait(2000); // uncomment for visual check
    QCOMPARE(renderedLineRange.first, linesAboveViewport);
    // if linesAboveViewport == 90, a chapter heading is visible, and those take more space
    QVERIFY(qAbs(renderedLineRange.second - (expectedLastLine + 1)) < (linesAboveViewport > 80 ? 4 : 2));
}

void tst_qquicktext::largeTextInDelayedLoader() // QTBUG-115687
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("loaderActiveOnVisible.qml")));
    auto flick = view.rootObject()->findChild<QQuickFlickable*>();
    QVERIFY(flick);
    auto textItem = view.rootObject()->findChild<QQuickText*>();
    QVERIFY(textItem);
    QQuickTextPrivate *textPriv = QQuickTextPrivate::get(textItem);
    QSGInternalTextNode *node = static_cast<QSGInternalTextNode *>(textPriv->paintNode);
    const auto initialLineRange = node->renderedLineRange();
    qCDebug(lcTests) << "first line rendered" << initialLineRange.first
                     << "; first line past viewport" << initialLineRange.second;
    flick->setContentY(500);
    QTRY_COMPARE_NE(node->renderedLineRange(), initialLineRange);
    const auto scrolledLineRange = node->renderedLineRange();
    qCDebug(lcTests) << "after scroll: first line rendered" << scrolledLineRange.first
                     << "; first line past viewport" << scrolledLineRange.second;
    // We scrolled a good bit more than one window-height, so we must render a
    // non-overlapping range of text some distance past the initial blocks.
    QCOMPARE_GT(scrolledLineRange.first, initialLineRange.second);
}

void tst_qquicktext::lineLaidOut()
{
    QScopedPointer<QQuickView> window(createView(testFile("lineLayout.qml")));

    QQuickText *myText = window->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != nullptr);

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(myText);
    QVERIFY(textPrivate != nullptr);

    QVERIFY(!textPrivate->extra.isAllocated());

    for (int i = 0; i < textPrivate->layout.lineCount(); ++i) {
        QRectF r = textPrivate->layout.lineAt(i).rect();
        QCOMPARE(r.width(), i * 15);
        if (i >= 30)
            QCOMPARE(r.x(), r.width() + 30);
        if (i >= 60) {
            QCOMPARE(r.x(), r.width() * 2 + 60);
            QCOMPARE(r.height(), qreal(20));
        }
    }

    // Ensure that isLast was correctly emitted
    int lastLineNumber = myText->property("lastLineNumber").toInt();
    QCOMPARE(lastLineNumber, myText->lineCount() - 1);
    // Ensure that only one line was considered last (after changing its width)
    bool receivedMultipleLastLines = myText->property("receivedMultipleLastLines").toBool();
    QVERIFY(!receivedMultipleLastLines);
}

void tst_qquicktext::lineLaidOutRelayout()
{
    QScopedPointer<QQuickView> window(createView(testFile("lineLayoutRelayout.qml")));

    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));

    QQuickText *myText = window->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != nullptr);

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(myText);
    QVERIFY(textPrivate != nullptr);

    QVERIFY(!textPrivate->extra.isAllocated());

    qreal y = 0.0;
    for (int i = 0; i < textPrivate->layout.lineCount(); ++i) {
        QTextLine line = textPrivate->layout.lineAt(i);
        const QRectF r = line.rect();
        if (r.x() == 0) {
            QCOMPARE(r.y(), y);
        } else {
            if (qFuzzyIsNull(r.y()))
                y = 0.0;
            QCOMPARE(r.x(), myText->width() / 2);
            QCOMPARE(r.y(), y);
        }
        y += line.height();
    }
}

void tst_qquicktext::lineLaidOutFontUpdate()
{
    QScopedPointer<QQuickView> window(createView(testFile("lineLayoutFontUpdate.qml")));

    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window.data()));

    auto *myText = window->rootObject()->findChild<QQuickText*>("exampleText");
    QVERIFY(myText != nullptr);

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(myText);
    QVERIFY(textPrivate != nullptr);

    QCOMPARE(textPrivate->layout.lineCount(), 2);

    QTextLine firstLine = textPrivate->layout.lineAt(0);
    QTextLine secondLine = textPrivate->layout.lineAt(1);

    QCOMPARE(firstLine.rect().x(), secondLine.rect().x() + 40);
    QCOMPARE(firstLine.rect().width(), secondLine.rect().width() - 40);
}

void tst_qquicktext::lineLaidOutHAlign()
{
    QScopedPointer<QQuickView> window(createView(testFile("lineLayoutHAlign.qml")));

    QQuickText *myText = window->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != nullptr);

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(myText);
    QVERIFY(textPrivate != nullptr);

    QCOMPARE(textPrivate->layout.lineCount(), 1);

    QVERIFY(textPrivate->layout.lineAt(0).naturalTextRect().x() < 0.0);
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
            << QUrl("http://testserver/images/heart200.png")
            << QUrl()
            << QUrl()
            << 181.;
    QTest::newRow("relative remote base 1")
            << QUrl("images/heart200.png")
            << QUrl("http://testserver/")
            << testFileUrl("nonexistant/app.qml")
            << 181.;
    QTest::newRow("relative remote base 2")
            << QUrl("heart200.png")
            << QUrl("http://testserver/images/")
            << testFileUrl("nonexistant/app.qml")
            << 181.;
}

void tst_qquicktext::lineLaidOutImplicitWidth()
{
    QScopedPointer<QQuickView> window(createView(testFile("lineLayoutImplicitWidth.qml")));

    QQuickText *myText = window->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != nullptr);

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(myText);
    QVERIFY(textPrivate != nullptr);

    // Retrieve the saved implicitWidth values of each rendered line
    QVariant widthsProperty = myText->property("lineImplicitWidths");
    QVERIFY(!widthsProperty.isNull());
    QVERIFY(widthsProperty.isValid());
    QVERIFY(widthsProperty.canConvert<QJSValue>());
    QJSValue widthsValue = widthsProperty.value<QJSValue>();
    QVERIFY(widthsValue.isArray());
    int lineCount = widthsValue.property("length").toInt();
    QVERIFY(lineCount > 0);

    // Create the same text layout by hand
    // Note that this approach needs additional processing for styled text,
    // so we only use it for plain text here.
    QTextLayout layout;
    layout.setCacheEnabled(true);
    layout.setText(myText->text());
    layout.setTextOption(textPrivate->layout.textOption());
    layout.setFont(myText->font());
    layout.beginLayout();
    for (QTextLine line = layout.createLine(); line.isValid(); line = layout.createLine()) {
        line.setLineWidth(myText->width());
    }
    layout.endLayout();

    // Line count of the just created layout should match the rendered text
    QCOMPARE(lineCount, layout.lineCount());

    // Go through each line and verify that the values emitted by lineLaidOut are correct
    for (int i = 0; i < layout.lineCount(); ++i) {
        qreal implicitWidth = widthsValue.property(i).toNumber();
        QVERIFY(implicitWidth > 0);

        QTextLine line = layout.lineAt(i);
        QCOMPARE(implicitWidth, line.naturalTextWidth());
    }
}

static QUrl substituteTestServerUrl(const QUrl &serverUrl, const QUrl &testUrl)
{
    QUrl result = testUrl;
    if (result.host() == QStringLiteral("testserver")) {
        result.setScheme(serverUrl.scheme());
        result.setHost(serverUrl.host());
        result.setPort(serverUrl.port());
    }
    return result;
}

void tst_qquicktext::imgTagsBaseUrl()
{
    QFETCH(QUrl, src);
    QFETCH(QUrl, baseUrl);
    QFETCH(QUrl, contextUrl);
    QFETCH(qreal, imgHeight);

    TestHTTPServer server;
    QVERIFY2(server.listen(), qPrintable(server.errorString()));
    server.serveDirectory(testFile(""));

    src = substituteTestServerUrl(server.baseUrl(), src);
    baseUrl = substituteTestServerUrl(server.baseUrl(), baseUrl);
    contextUrl = substituteTestServerUrl(server.baseUrl(), contextUrl);

    QByteArray baseUrlFragment;
    if (!baseUrl.isEmpty())
        baseUrlFragment = "; baseUrl: \"" + baseUrl.toEncoded() + "\"";
    QByteArray componentStr = "import QtQuick 2.0\nText { text: \"This is a test <img src=\\\"" + src.toEncoded() + "\\\">\"" + baseUrlFragment + " }";

    QQmlComponent component(&engine);
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
    QTest::newRow("heart-bottom") << "images/heart200.png" << 181 <<  "bottom";
    QTest::newRow("heart-middle") << "images/heart200.png" << 181 <<  "middle";
    QTest::newRow("heart-top") << "images/heart200.png" << 181 <<  "top";
    QTest::newRow("starfish-bottom") << "images/starfish_2.png" << 217 <<  "bottom";
    QTest::newRow("starfish-middle") << "images/starfish_2.png" << 217 <<  "middle";
    QTest::newRow("starfish-top") << "images/starfish_2.png" << 217 <<  "top";
}

void tst_qquicktext::imgTagsAlign()
{
    QFETCH(QString, src);
    QFETCH(int, imgHeight);
    QFETCH(QString, align);
    QString componentStr = "import QtQuick 2.0\nText { text: \"This is a test <img src=\\\"" + src + "\\\" align=\\\"" + align + "\\\"> of image.\" }";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), testFileUrl("."));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

    QVERIFY(textObject != nullptr);
    QCOMPARE(textObject->height(), qreal(imgHeight));

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
    QVERIFY(textPrivate != nullptr);

    QRectF br = textPrivate->layout.boundingRect();
    if (align == "bottom")
        QVERIFY(br.y() == imgHeight - br.height());
    else if (align == "middle")
        QVERIFY(br.y() == imgHeight / 2.0 - br.height() / 2.0);
    else if (align == "top")
        QCOMPARE(br.y(), qreal(0));
}

void tst_qquicktext::imgTagsMultipleImages()
{
    QString componentStr = "import QtQuick 2.0\nText { text: \"This is a starfish<img src=\\\"images/starfish_2.png\\\" width=\\\"60\\\" height=\\\"60\\\" > and another one<img src=\\\"images/heart200.png\\\" width=\\\"85\\\" height=\\\"85\\\">.\" }";

    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), testFileUrl("."));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

    QVERIFY(textObject != nullptr);
    QCOMPARE(textObject->height(), qreal(85));

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
    QVERIFY(textPrivate != nullptr);
    QCOMPARE(textPrivate->extra->visibleImgTags.size(), 2);
}

void tst_qquicktext::imgTagsElide()
{
    QScopedPointer<QQuickView> window(createView(testFile("imgTagsElide.qml")));
    QScopedPointer<QQuickText> myText(window->rootObject()->findChild<QQuickText*>("myText"));
    QVERIFY(myText);

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(myText.data());
    QVERIFY(textPrivate != nullptr);
    QCOMPARE(textPrivate->extra->visibleImgTags.size(), 0);
    myText->setMaximumLineCount(20);
    QTRY_COMPARE(textPrivate->extra->visibleImgTags.size(), 1);
}

void tst_qquicktext::imgTagsUpdates()
{
    QScopedPointer<QQuickView> window(createView(testFile("imgTagsUpdates.qml")));
    QScopedPointer<QQuickText> myText(window->rootObject()->findChild<QQuickText*>("myText"));
    QVERIFY(myText);

    QSignalSpy spy(myText.data(), SIGNAL(contentSizeChanged()));

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(myText.data());
    QVERIFY(textPrivate != nullptr);

    myText->setText("This is a heart<img src=\"images/heart200.png\">.");
    QCOMPARE(textPrivate->extra->visibleImgTags.size(), 1);
    QCOMPARE(spy.size(), 1);

    myText->setMaximumLineCount(2);
    myText->setText("This is another heart<img src=\"images/heart200.png\">.");
    QTRY_COMPARE(textPrivate->extra->visibleImgTags.size(), 1);

    // if maximumLineCount is set and the img tag doesn't have an explicit size
    // we relayout twice.
    QCOMPARE(spy.size(), 3);
}

void tst_qquicktext::imgTagsError()
{
    QString componentStr = "import QtQuick 2.0\nText { text: \"This is a starfish<img src=\\\"images/starfish_2.pn\\\" width=\\\"60\\\" height=\\\"60\\\">.\" }";

    QQmlComponent textComponent(&engine);
    const QString expectedMessage(
            testFileUrl(".").toString()
            + ":2:1: QML (QQuick)?Text: Cannot open: "
            + testFileUrl("images/starfish_2.pn").toString());
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(expectedMessage.toLatin1()));
    textComponent.setData(componentStr.toLatin1(), testFileUrl("."));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

    QVERIFY(textObject != nullptr);
}

void tst_qquicktext::imgSize_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<qint64>("width");
    QTest::addColumn<qint64>("height");
    QTest::addColumn<QQuickText::TextFormat>("format");

    QTest::newRow("negative (styled text)") << QStringLiteral("images/starfish_2.png")
                                         << qint64(-0x7FFFFF)
                                         << qint64(-0x7FFFFF)
                                         << QQuickText::StyledText;
    QTest::newRow("negative (rich text)") << QStringLiteral("images/starfish_2.png")
                                         << qint64(-0x7FFFFF)
                                         << qint64(-0x7FFFFF)
                                         << QQuickText::RichText;
    QTest::newRow("large (styled text)") << QStringLiteral("images/starfish_2.png")
                                         << qint64(0x7FFFFF)
                                         << qint64(0x7FFFFF)
                                         << QQuickText::StyledText;
    QTest::newRow("large (right text)") << QStringLiteral("images/starfish_2.png")
                                        << qint64(0x7FFFFF)
                                        << qint64(0x7FFFFF)
                                        << QQuickText::RichText;
    QTest::newRow("medium (styled text)") << QStringLiteral("images/starfish_2.png")
                                         << qint64(0x10000)
                                         << qint64(0x10000)
                                         << QQuickText::StyledText;
    QTest::newRow("medium (right text)") << QStringLiteral("images/starfish_2.png")
                                        << qint64(0x10000)
                                        << qint64(0x10000)
                                        << QQuickText::RichText;
    QTest::newRow("out-of-bounds (styled text)") << QStringLiteral("images/starfish_2.png")
                                                 << (qint64(INT_MAX) + 1)
                                                 << (qint64(INT_MAX) + 1)
                                                 << QQuickText::StyledText;
    QTest::newRow("out-of-bounds (rich text)") << QStringLiteral("images/starfish_2.png")
                                                 << (qint64(INT_MAX) + 1)
                                                 << (qint64(INT_MAX) + 1)
                                                 << QQuickText::RichText;
    QTest::newRow("negative out-of-bounds (styled text)") << QStringLiteral("images/starfish_2.png")
                                                 << (qint64(INT_MIN) - 1)
                                                 << (qint64(INT_MIN) - 1)
                                                 << QQuickText::StyledText;
    QTest::newRow("negative out-of-bounds (rich text)") << QStringLiteral("images/starfish_2.png")
                                                 << (qint64(INT_MIN) - 1)
                                                 << (qint64(INT_MIN) - 1)
                                                 << QQuickText::RichText;
    QTest::newRow("large non-existent (styled text)") << QStringLiteral("a")
                                                 << qint64(0x7FFFFF)
                                                 << qint64(0x7FFFFF)
                                                 << QQuickText::StyledText;
    QTest::newRow("medium non-existent (styled text)") << QStringLiteral("a")
                                                 << qint64(0x10000)
                                                 << qint64(0x10000)
                                                 << QQuickText::StyledText;
    QTest::newRow("out-of-bounds non-existent (styled text)") << QStringLiteral("a")
                                                 << (qint64(INT_MAX) + 1)
                                                 << (qint64(INT_MAX) + 1)
                                                 << QQuickText::StyledText;
    QTest::newRow("large non-existent (rich text)") << QStringLiteral("a")
                                                 << qint64(0x7FFFFF)
                                                 << qint64(0x7FFFFF)
                                                 << QQuickText::RichText;
    QTest::newRow("medium non-existent (rich text)") << QStringLiteral("a")
                                                 << qint64(0x10000)
                                                 << qint64(0x10000)
                                                 << QQuickText::RichText;
}

void tst_qquicktext::imgSize()
{
    QFETCH(QString, url);
    QFETCH(qint64, width);
    QFETCH(qint64, height);
    QFETCH(QQuickText::TextFormat, format);

    // Reusing imgTagsUpdates.qml here, since it is just an empty Text component
    QScopedPointer<QQuickView> window(createView(testFile("imgTagsUpdates.qml")));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QScopedPointer<QQuickText> myText(window->rootObject()->findChild<QQuickText*>("myText"));
    QVERIFY(myText);

    myText->setTextFormat(format);

    QString imgStr = QStringLiteral("<img src=\"%1\" width=\"%2\" height=\"%3\" />")
            .arg(url)
            .arg(width)
            .arg(height);
    myText->setText(imgStr);

    QVERIFY(QQuickTest::qWaitForPolish(myText.data()));
}

void tst_qquicktext::fontSizeMode_data()
{
    QTest::addColumn<QString>("text");
    QTest::newRow("plain") << "The quick red fox jumped over the lazy brown dog";
    QTest::newRow("styled") << "<b>The quick red fox jumped over the lazy brown dog</b>";
}

void tst_qquicktext::fontSizeMode()
{
    QFETCH(QString, text);

    QScopedPointer<QQuickView> window(createView(testFile("fontSizeMode.qml")));
    window->show();

    QQuickText *myText = window->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != nullptr);

    myText->setText(text);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    qreal originalWidth = myText->contentWidth();
    qreal originalHeight = myText->contentHeight();

    // The original text unwrapped should exceed the width of the item.
    QVERIFY(originalWidth > myText->width());
    QVERIFY(originalHeight < myText->height());

    QFont font = myText->font();
    font.setPixelSize(64);

    myText->setFont(font);
    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // Font size reduced to fit within the width of the item.
    qreal horizontalFitWidth = myText->contentWidth();
    qreal horizontalFitHeight = myText->contentHeight();
    QVERIFY(horizontalFitWidth <= myText->width() + 2); // rounding
    QVERIFY(horizontalFitHeight <= myText->height() + 2);

    // Elide won't affect the size with HorizontalFit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    myText->setElideMode(QQuickText::ElideLeft);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    myText->setElideMode(QQuickText::ElideMiddle);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // Font size increased to fill the height of the item.
    qreal verticalFitHeight = myText->contentHeight();
    QVERIFY(myText->contentWidth() > myText->width());
    QVERIFY(verticalFitHeight <= myText->height() + 2);
    QVERIFY(verticalFitHeight > originalHeight);

    // Elide won't affect the height of a single line with VerticalFit but will crop the width.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(myText->truncated());
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideLeft);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(myText->truncated());
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideMiddle);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(myText->truncated());
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::Fit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // Should be the same as HorizontalFit with no wrapping.
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    // Elide won't affect the size with Fit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    myText->setElideMode(QQuickText::ElideLeft);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    myText->setElideMode(QQuickText::ElideMiddle);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::FixedSize);
    myText->setWrapMode(QQuickText::Wrap);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    originalWidth = myText->contentWidth();
    originalHeight = myText->contentHeight();

    // The original text wrapped should exceed the height of the item.
    QVERIFY(originalWidth <= myText->width() + 2);
    QVERIFY(originalHeight > myText->height());

    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // HorizontalFit should reduce the font size to minimize wrapping, which brings it back to the
    // same size as without text wrapping.
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    // Elide won't affect the size with HorizontalFit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // VerticalFit should reduce the size to the wrapped text within the vertical height.
    verticalFitHeight = myText->contentHeight();
    qreal verticalFitWidth = myText->contentWidth();
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QVERIFY(verticalFitHeight <= myText->height() + 2);
    QVERIFY(verticalFitHeight < originalHeight);

    // Elide won't affect the height or width of a wrapped text with VerticalFit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::Fit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // Should be the same as VerticalFit with wrapping.
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    // Elide won't affect the size with Fit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::FixedSize);
    myText->setMaximumLineCount(2);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    // The original text wrapped should exceed the height of the item.
    QVERIFY(originalWidth <= myText->width() + 2);
    QVERIFY(originalHeight > myText->height());

    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // HorizontalFit should reduce the font size to minimize wrapping, which brings it back to the
    // same size as without text wrapping.
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    // Elide won't affect the size with HorizontalFit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // VerticalFit should reduce the size to the wrapped text within the vertical height.
    verticalFitHeight = myText->contentHeight();
    verticalFitWidth = myText->contentWidth();
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QVERIFY(verticalFitHeight <= myText->height() + 2);
    QVERIFY(verticalFitHeight < originalHeight);

    // Elide won't affect the height or width of a wrapped text with VerticalFit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::Fit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // Should be the same as VerticalFit with wrapping.
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    // Elide won't affect the size with Fit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    // Growing height needs to update the baselineOffset when AlignBottom is used
    // and text is NOT wrapped
    myText->setVAlign(QQuickText::AlignBottom);
    myText->setFontSizeMode(QQuickText::Fit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    int baselineOffset = myText->baselineOffset();
    myText->setHeight(myText->height() * 2);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(myText->baselineOffset() > baselineOffset);

    // Growing height needs to update the baselineOffset when AlignBottom is used
    // and the text is wrapped
    myText->setVAlign(QQuickText::AlignBottom);
    myText->setFontSizeMode(QQuickText::Fit);
    myText->setWrapMode(QQuickText::NoWrap);
    myText->resetMaximumLineCount();
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    baselineOffset = myText->baselineOffset();
    myText->setHeight(myText->height() * 2);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(myText->baselineOffset() > baselineOffset);

    // Check baselineOffset for the HorizontalFit case
    myText->setVAlign(QQuickText::AlignBottom);
    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QSignalSpy baselineOffsetSpy(myText, SIGNAL(baselineOffsetChanged(qreal)));
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    const qreal oldBaselineOffset = myText->baselineOffset();
    myText->setHeight(myText->height() + 42);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QCOMPARE(baselineOffsetSpy.size(), 1);
    QCOMPARE(myText->baselineOffset(), oldBaselineOffset + 42);
    myText->setHeight(myText->height() - 42);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QCOMPARE(baselineOffsetSpy.size(), 2);
    QCOMPARE(myText->baselineOffset(), oldBaselineOffset);
}

void tst_qquicktext::fontSizeModeMultiline_data()
{
    QTest::addColumn<QString>("text");
    QTest::newRow("plain") << "The quick red fox jumped\n over the lazy brown dog";
    QTest::newRow("styledtext") << "<b>The quick red fox jumped<br/> over the lazy brown dog</b>";
}

void tst_qquicktext::fontSizeModeMultiline()
{
    QFETCH(QString, text);

    QScopedPointer<QQuickView> window(createView(testFile("fontSizeMode.qml")));
    window->show();

    QQuickText *myText = window->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != nullptr);

    myText->setText(text);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

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
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // Font size reduced to fit within the width of the item.
    QCOMPARE(myText->lineCount(), 2);
    qreal horizontalFitWidth = myText->contentWidth();
    qreal horizontalFitHeight = myText->contentHeight();
    QVERIFY(horizontalFitWidth <= myText->width() + 2); // rounding
    QVERIFY(horizontalFitHeight > myText->height());

    // Right eliding will remove the last line
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(myText->truncated());
    QCOMPARE(myText->lineCount(), 1);
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QVERIFY(myText->contentHeight() <= myText->height() + 2);

    // Left or middle eliding wont have any effect.
    myText->setElideMode(QQuickText::ElideLeft);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    myText->setElideMode(QQuickText::ElideMiddle);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // Font size reduced to fit within the height of the item.
    qreal verticalFitWidth = myText->contentWidth();
    qreal verticalFitHeight = myText->contentHeight();
    QVERIFY(verticalFitWidth <= myText->width() + 2);
    QVERIFY(verticalFitHeight <= myText->height() + 2);

    // Elide will have no effect.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideLeft);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideMiddle);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::Fit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // Should be the same as VerticalFit with no wrapping.
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    // Elide won't affect the size with Fit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideLeft);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideMiddle);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::FixedSize);
    myText->setWrapMode(QQuickText::Wrap);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    originalWidth = myText->contentWidth();
    originalHeight = myText->contentHeight();

    // The original text wrapped should exceed the height of the item.
    QVERIFY(originalWidth <= myText->width() + 2);
    QVERIFY(originalHeight > myText->height());

    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // HorizontalFit should reduce the font size to minimize wrapping, which brings it back to the
    // same size as without text wrapping.
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    // Text will be elided vertically with HorizontalFit
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(myText->truncated());
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QVERIFY(myText->contentHeight() <= myText->height() + 2);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // VerticalFit should reduce the size to the wrapped text within the vertical height.
    verticalFitHeight = myText->contentHeight();
    verticalFitWidth = myText->contentWidth();
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QVERIFY(verticalFitHeight <= myText->height() + 2);
    QVERIFY(verticalFitHeight < originalHeight);

    // Elide won't affect the height or width of a wrapped text with VerticalFit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::Fit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // Should be the same as VerticalFit with wrapping.
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    // Elide won't affect the size with Fit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::FixedSize);
    myText->setMaximumLineCount(2);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    // The original text wrapped should exceed the height of the item.
    QVERIFY(originalWidth <= myText->width() + 2);
    QVERIFY(originalHeight > myText->height());

    myText->setFontSizeMode(QQuickText::HorizontalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // HorizontalFit should reduce the font size to minimize wrapping, which brings it back to the
    // same size as without text wrapping.
    QCOMPARE(myText->contentWidth(), horizontalFitWidth);
    QCOMPARE(myText->contentHeight(), horizontalFitHeight);

    // Elide won't affect the size with HorizontalFit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(myText->truncated());
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QVERIFY(myText->contentHeight() <= myText->height() + 2);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::VerticalFit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // VerticalFit should reduce the size to the wrapped text within the vertical height.
    verticalFitHeight = myText->contentHeight();
    verticalFitWidth = myText->contentWidth();
    QVERIFY(myText->contentWidth() <= myText->width() + 2);
    QVERIFY(verticalFitHeight <= myText->height() + 2);
    QVERIFY(verticalFitHeight < originalHeight);

    // Elide won't affect the height or width of a wrapped text with VerticalFit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    myText->setFontSizeMode(QQuickText::Fit);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    // Should be the same as VerticalFit with wrapping.
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    // Elide won't affect the size with Fit.
    myText->setElideMode(QQuickText::ElideRight);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    QVERIFY(!myText->truncated());
    QCOMPARE(myText->contentWidth(), verticalFitWidth);
    QCOMPARE(myText->contentHeight(), verticalFitHeight);

    myText->setElideMode(QQuickText::ElideNone);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
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

    QScopedPointer<QQuickView> window(createView(source));
    window->show();

    QQuickText *myText = window->rootObject()->findChild<QQuickText*>("myText");
    QVERIFY(myText != nullptr);

    const QString longText = "the quick brown fox jumped over the lazy dog";
    const QString mediumText = "the brown fox jumped over the dog";
    const QString shortText = "fox jumped dog";

    myText->setText(longText);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    const qreal longWidth = myText->contentWidth();
    const qreal longHeight = myText->contentHeight();

    myText->setText(mediumText);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    const qreal mediumWidth = myText->contentWidth();
    const qreal mediumHeight = myText->contentHeight();

    myText->setText(shortText);
    QVERIFY(QQuickTest::qWaitForPolish(myText));
    const qreal shortWidth = myText->contentWidth();
    const qreal shortHeight = myText->contentHeight();

    myText->setElideMode(QQuickText::ElideRight);
    myText->setText(longText + QLatin1Char('\x9c') + mediumText + QLatin1Char('\x9c') + shortText);

    myText->setSize(QSizeF(longWidth, longHeight));
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    QCOMPARE(myText->contentWidth(), longWidth);
    QCOMPARE(myText->contentHeight(), longHeight);
    QCOMPARE(myText->truncated(), false);

    myText->setSize(QSizeF(mediumWidth, mediumHeight));
    QVERIFY(QQuickTest::qWaitForPolish(myText));

    QCOMPARE(myText->contentWidth(), mediumWidth);
    QCOMPARE(myText->contentHeight(), mediumHeight);
    QCOMPARE(myText->truncated(), true);

    myText->setSize(QSizeF(shortWidth, shortHeight));
    QVERIFY(QQuickTest::qWaitForPolish(myText));

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

    {
        QQuickView view;
        QVERIFY(QQuickTest::showView(view, testFileUrl("pointFontSizes.qml")));
        QQuickText *qtext = view.rootObject()->findChild<QQuickText*>("text");
        QQuickText *qtextWithTag = view.rootObject()->findChild<QQuickText*>("textWithTag");
        QVERIFY(qtext != nullptr);
        QVERIFY(qtextWithTag != nullptr);

        qtext->setText(text);
        qtextWithTag->setText(textWithTag);

        for (int size = 6; size < 100; size += 4) {
            view.rootObject()->setProperty("pointSize", size);
            if (fontIsBigger)
                QVERIFY(qtext->height() <= qtextWithTag->height());
            else
                QVERIFY(qtext->height() >= qtextWithTag->height());
        }
    }

    {
        QQuickView view;
        QVERIFY(QQuickTest::showView(view, testFileUrl("pixelFontSizes.qml")));
        QQuickText *qtext = view.rootObject()->findChild<QQuickText*>("text");
        QQuickText *qtextWithTag = view.rootObject()->findChild<QQuickText*>("textWithTag");
        QVERIFY(qtext != nullptr);
        QVERIFY(qtextWithTag != nullptr);

        qtext->setText(text);
        qtextWithTag->setText(textWithTag);

        for (int size = 6; size < 100; size += 4) {
            view.rootObject()->setProperty("pixelSize", size);
            if (fontIsBigger)
                QVERIFY(qtext->height() <= qtextWithTag->height());
            else
                QVERIFY(qtext->height() >= qtextWithTag->height());
        }
    }
}

static qreal expectedBaselineTop(QQuickText *item)
{
    QFontMetricsF fm(item->font());
    return fm.ascent() + item->topPadding();
}

static qreal expectedBaselineBottom(QQuickText *item)
{
    QFontMetricsF fm(item->font());
    return item->height() - item->contentHeight() - item->bottomPadding() + fm.ascent();
}

static qreal expectedBaselineCenter(QQuickText *item)
{
    QFontMetricsF fm(item->font());
    return ((item->height() - item->contentHeight() - item->topPadding() - item->bottomPadding()) / 2) + fm.ascent() + item->topPadding();
}

static qreal expectedBaselineBold(QQuickText *item)
{
    QFont font = item->font();
    font.setBold(true);
    QFontMetricsF fm(font);
    return fm.ascent() + item->topPadding();
}

static qreal expectedBaselineImage(QQuickText *item)
{
    QFontMetricsF fm(item->font());
    // The line is positioned so the bottom of the line is aligned with the bottom of the image,
    // or image height - line height and the baseline is line position + ascent.  Because
    // QTextLine's height is rounded up this can give slightly different results to image height
    // - descent.
    return 181 - qCeil(fm.height()) + fm.ascent() + item->topPadding();
}

static qreal expectedBaselineCustom(QQuickText *item)
{
    QFontMetricsF fm(item->font());
    return 16 + fm.ascent() + item->topPadding();
}

static qreal expectedBaselineScaled(QQuickText *item)
{
    QFont font = item->font();
    QTextLayout layout(item->text().replace(QLatin1Char('\n'), QChar::LineSeparator));

    qreal low = 0;
    qreal high = 10000;

    while (low < high) {
        layout.setFont(font);
        qreal width = 0;
        layout.beginLayout();
        for (QTextLine line = layout.createLine(); line.isValid(); line = layout.createLine()) {
            line.setLineWidth(FLT_MAX);
            width = qMax(line.naturalTextWidth(), width);
        }
        layout.endLayout();

        if (width > item->width()) {
            high = font.pointSizeF();
            font.setPointSizeF((high + low) / 2);
        } else {
            low = font.pointSizeF();

            // When fontSizeMode != FixedSize, the font size will be scaled to a value
            // The goal is to find a pointSize that uses as much space as possible while
            // still fitting inside the available space. 0.01 is chosen as the threshold.
            if ((high - low) < qreal(0.01)) {
                QFontMetricsF fm(layout.font());
                return fm.ascent() + item->topPadding();
            }

            font.setPointSizeF((high + low) / 2);
        }
    }
    return item->topPadding();
}

static qreal expectedBaselineFixedBottom(QQuickText *item)
{
    QFontMetricsF fm(item->font());
    qreal dy = item->text().contains(QLatin1Char('\n'))
            ? 160
            : 180;
    return dy + fm.ascent() - item->bottomPadding();
}

static qreal expectedBaselineProportionalBottom(QQuickText *item)
{
    QFontMetricsF fm(item->font());
    qreal dy = item->text().contains(QLatin1Char('\n'))
            ? 200 - (qCeil(fm.height()) * 3)
            : 200 - (qCeil(fm.height()) * 1.5);
    return dy + fm.ascent() - item->bottomPadding();
}

void tst_qquicktext::baselineOffset_data()
{
    qRegisterMetaType<ExpectedBaseline>();
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("wrappedText");
    QTest::addColumn<QByteArray>("bindings");
    QTest::addColumn<ExpectedBaseline>("expectedBaseline");
    QTest::addColumn<ExpectedBaseline>("expectedBaselineEmpty");

    QTest::newRow("top align")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; verticalAlignment: Text.AlignTop")
            << &expectedBaselineTop
            << &expectedBaselineTop;
    QTest::newRow("bottom align")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; verticalAlignment: Text.AlignBottom")
            << &expectedBaselineBottom
            << &expectedBaselineBottom;
    QTest::newRow("center align")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; verticalAlignment: Text.AlignVCenter")
            << &expectedBaselineCenter
            << &expectedBaselineCenter;

    QTest::newRow("bold")
            << "<b>hello world</b>"
            << "<b>hello<br/>world</b>"
            << QByteArray("height: 200")
            << &expectedBaselineBold
            << &expectedBaselineTop;

    QTest::newRow("richText")
            << "<b>hello world</b>"
            << "<b>hello<br/>world</b>"
            << QByteArray("height: 200; textFormat: Text.RichText")
            << &expectedBaselineTop
            << &expectedBaselineTop;

    QTest::newRow("elided")
            << "hello world"
            << "hello\nworld"
            << QByteArray("width: 20; height: 8; elide: Text.ElideRight")
            << &expectedBaselineTop
            << &expectedBaselineTop;

    QTest::newRow("elided bottom align")
            << "hello world"
            << "hello\nworld!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            << QByteArray("width: 200; height: 200; elide: Text.ElideRight; verticalAlignment: Text.AlignBottom")
            << &expectedBaselineBottom
            << &expectedBaselineBottom;

    QTest::newRow("image")
            << "hello <img src=\"images/heart200.png\" /> world"
            << "hello <img src=\"images/heart200.png\" /><br/>world"
            << QByteArray("height: 200\n; baseUrl: \"") + testFileUrl("reference").toEncoded() + QByteArray("\"")
            << &expectedBaselineImage
            << &expectedBaselineTop;

    QTest::newRow("customLine")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; onLineLaidOut: (line) => { line.y += 16; }")
            << &expectedBaselineCustom
            << &expectedBaselineCustom;

    QTest::newRow("scaled font")
            << "hello world"
            << "hello\nworld"
            << QByteArray("width: 200; minimumPointSize: 1; font.pointSize: 10000; fontSizeMode: Text.HorizontalFit")
            << &expectedBaselineScaled
            << &expectedBaselineTop;

    QTest::newRow("fixed line height top align")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; lineHeightMode: Text.FixedHeight; lineHeight: 20; verticalAlignment: Text.AlignTop")
            << &expectedBaselineTop
            << &expectedBaselineTop;

    QTest::newRow("fixed line height bottom align")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; lineHeightMode: Text.FixedHeight; lineHeight: 20; verticalAlignment: Text.AlignBottom")
            << &expectedBaselineFixedBottom
            << &expectedBaselineFixedBottom;

    QTest::newRow("proportional line height top align")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; lineHeightMode: Text.ProportionalHeight; lineHeight: 1.5; verticalAlignment: Text.AlignTop")
            << &expectedBaselineTop
            << &expectedBaselineTop;

    QTest::newRow("proportional line height bottom align")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; lineHeightMode: Text.ProportionalHeight; lineHeight: 1.5; verticalAlignment: Text.AlignBottom")
            << &expectedBaselineProportionalBottom
            << &expectedBaselineProportionalBottom;

    QTest::newRow("top align with padding")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; topPadding: 10; bottomPadding: 20; verticalAlignment: Text.AlignTop")
            << &expectedBaselineTop
            << &expectedBaselineTop;
    QTest::newRow("bottom align with padding")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; topPadding: 10; bottomPadding: 20; verticalAlignment: Text.AlignBottom")
            << &expectedBaselineBottom
            << &expectedBaselineBottom;
    QTest::newRow("center align with padding")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; topPadding: 10; bottomPadding: 20; verticalAlignment: Text.AlignVCenter")
            << &expectedBaselineCenter
            << &expectedBaselineCenter;

    QTest::newRow("bold width padding")
            << "<b>hello world</b>"
            << "<b>hello<br/>world</b>"
            << QByteArray("height: 200; topPadding: 10; bottomPadding: 20")
            << &expectedBaselineBold
            << &expectedBaselineTop;

    QTest::newRow("richText with padding")
            << "<b>hello world</b>"
            << "<b>hello<br/>world</b>"
            << QByteArray("height: 200; topPadding: 10; bottomPadding: 20; textFormat: Text.RichText")
            << &expectedBaselineTop
            << &expectedBaselineTop;

    QTest::newRow("elided with padding")
            << "hello world"
            << "hello\nworld"
            << QByteArray("width: 20; height: 8; topPadding: 10; bottomPadding: 20; elide: Text.ElideRight")
            << &expectedBaselineTop
            << &expectedBaselineTop;

    QTest::newRow("elided bottom align with padding")
            << "hello world"
            << "hello\nworld!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
            << QByteArray("width: 200; height: 200; topPadding: 10; bottomPadding: 20; elide: Text.ElideRight; verticalAlignment: Text.AlignBottom")
            << &expectedBaselineBottom
            << &expectedBaselineBottom;

    QTest::newRow("image with padding")
            << "hello <img src=\"images/heart200.png\" /> world"
            << "hello <img src=\"images/heart200.png\" /><br/>world"
            << QByteArray("height: 200\n; topPadding: 10; bottomPadding: 20; baseUrl: \"") + testFileUrl("reference").toEncoded() + QByteArray("\"")
            << &expectedBaselineImage
            << &expectedBaselineTop;

    QTest::newRow("customLine with padding")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; topPadding: 10; bottomPadding: 20; "
                          "onLineLaidOut: (line) => { line.y += 16; }")
            << &expectedBaselineCustom
            << &expectedBaselineCustom;

    QTest::newRow("scaled font with padding")
            << "hello world"
            << "hello\nworld"
            << QByteArray("width: 200; topPadding: 10; bottomPadding: 20; minimumPointSize: 1; font.pointSize: 10000; fontSizeMode: Text.HorizontalFit")
            << &expectedBaselineScaled
            << &expectedBaselineTop;

    QTest::newRow("fixed line height top align with padding")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; topPadding: 10; bottomPadding: 20; lineHeightMode: Text.FixedHeight; lineHeight: 20; verticalAlignment: Text.AlignTop")
            << &expectedBaselineTop
            << &expectedBaselineTop;

    QTest::newRow("fixed line height bottom align with padding")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; topPadding: 10; bottomPadding: 20; lineHeightMode: Text.FixedHeight; lineHeight: 20; verticalAlignment: Text.AlignBottom")
            << &expectedBaselineFixedBottom
            << &expectedBaselineFixedBottom;

    QTest::newRow("proportional line height top align with padding")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; topPadding: 10; bottomPadding: 20; lineHeightMode: Text.ProportionalHeight; lineHeight: 1.5; verticalAlignment: Text.AlignTop")
            << &expectedBaselineTop
            << &expectedBaselineTop;

    QTest::newRow("proportional line height bottom align with padding")
            << "hello world"
            << "hello\nworld"
            << QByteArray("height: 200; topPadding: 10; bottomPadding: 20; lineHeightMode: Text.ProportionalHeight; lineHeight: 1.5; verticalAlignment: Text.AlignBottom")
            << &expectedBaselineProportionalBottom
            << &expectedBaselineProportionalBottom;
}

void tst_qquicktext::baselineOffset()
{
    QFETCH(QString, text);
    QFETCH(QString, wrappedText);
    QFETCH(QByteArray, bindings);
    QFETCH(ExpectedBaseline, expectedBaseline);
    QFETCH(ExpectedBaseline, expectedBaselineEmpty);

    QQmlComponent component(&engine);
    component.setData(
            "import QtQuick 2.6\n"
            "Text {\n"
                + bindings + "\n"
            "}", QUrl());

    QScopedPointer<QObject> object(component.create());

    QQuickText *item = qobject_cast<QQuickText *>(object.data());
    QVERIFY(item);

    {
        qreal baseline = expectedBaselineEmpty(item);

        QCOMPARE(item->baselineOffset(), baseline);

        item->setText(text);
        if (expectedBaseline != expectedBaselineEmpty)
            baseline = expectedBaseline(item);

        QCOMPARE(item->baselineOffset(), baseline);

        item->setText(wrappedText);
        QCOMPARE(item->baselineOffset(), expectedBaseline(item));
    }

    QFont font = item->font();
    font.setPointSize(font.pointSize() + 8);

    {
        QCOMPARE(item->baselineOffset(), expectedBaseline(item));

        item->setText(text);
        qreal baseline = expectedBaseline(item);
        QCOMPARE(item->baselineOffset(), baseline);

        item->setText(QString());
        if (expectedBaselineEmpty != expectedBaseline)
            baseline = expectedBaselineEmpty(item);

        QCOMPARE(item->baselineOffset(), baseline);
    }
}

void tst_qquicktext::htmlLists()
{
    QFETCH(QString, text);
    QFETCH(int, nbLines);

    QScopedPointer<QQuickView>view(createView(testFile("htmlLists.qml")));
    QQuickText *textObject = view->rootObject()->findChild<QQuickText*>("myText");

    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(textObject);
    QVERIFY(textPrivate != nullptr);
    QVERIFY(textPrivate->extra.isAllocated());

    QVERIFY(textObject != nullptr);
    textObject->setText(text);

    view->show();
    view->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(view.get()));

    QCOMPARE(textPrivate->extra->doc->lineCount(), nbLines);
}

void tst_qquicktext::htmlLists_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("nbLines");

    QTest::newRow("ordered list") << "<ol><li>one<li>two<li>three" << 3;
    QTest::newRow("ordered list closed") << "<ol><li>one</li></ol>" << 1;
    QTest::newRow("ordered list alpha") << "<ol type=\"a\"><li>one</li><li>two</li></ol>" << 2;
    QTest::newRow("ordered list upper alpha") << "<ol type=\"A\"><li>one</li><li>two</li></ol>" << 2;
    QTest::newRow("ordered list roman") << "<ol type=\"i\"><li>one</li><li>two</li></ol>" << 2;
    QTest::newRow("ordered list upper roman") << "<ol type=\"I\"><li>one</li><li>two</li></ol>" << 2;
    QTest::newRow("ordered list bad") << "<ol type=\"z\"><li>one</li><li>two</li></ol>" << 2;
    QTest::newRow("unordered list") << "<ul><li>one<li>two" << 2;
    QTest::newRow("unordered list closed") << "<ul><li>one</li><li>two</li></ul>" << 2;
    QTest::newRow("unordered list disc") << "<ul type=\"disc\"><li>one</li><li>two</li></ul>" << 2;
    QTest::newRow("unordered list square") << "<ul type=\"square\"><li>one</li><li>two</li></ul>" << 2;
    QTest::newRow("unordered list bad") << "<ul type=\"bad\"><li>one</li><li>two</li></ul>" << 2;
}

void tst_qquicktext::elideBeforeMaximumLineCount()
{   // QTBUG-31471
    QQmlComponent component(&engine, testFile("elideBeforeMaximumLineCount.qml"));

    QScopedPointer<QObject> object(component.create());

    QQuickText *item = qobject_cast<QQuickText *>(object.data());
    QVERIFY(item);

    QCOMPARE(item->lineCount(), 2);
}

void tst_qquicktext::hover()
{   // QTBUG-33842
    QQmlComponent component(&engine, testFile("hover.qml"));

    QScopedPointer<QObject> object(component.create());

    QQuickWindow *window = qobject_cast<QQuickWindow *>(object.data());
    QVERIFY(window);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    QQuickMouseArea *mouseArea = window->property("mouseArea").value<QQuickMouseArea *>();
    QVERIFY(mouseArea);
    QQuickText *textItem = window->property("textItem").value<QQuickText *>();
    QVERIFY(textItem);

    QVERIFY(!mouseArea->property("wasHovered").toBool());

    QPoint center(window->width() / 2, window->height() / 2);
    QPoint delta(window->width() / 10, window->height() / 10);

    QTest::mouseMove(window, center - delta);
    QTest::mouseMove(window, center + delta);

    QVERIFY(mouseArea->property("wasHovered").toBool());
}

void tst_qquicktext::growFromZeroWidth()
{
    QQmlComponent component(&engine, testFile("growFromZeroWidth.qml"));

    QScopedPointer<QObject> object(component.create());

    QQuickText *text = qobject_cast<QQuickText *>(object.data());
    QVERIFY(text);

    QCOMPARE(text->lineCount(), 3);

    text->setWidth(80);

    // the new width should force our contents to wrap
    QVERIFY(text->lineCount() > 3);
}

void tst_qquicktext::padding()
{
    QScopedPointer<QQuickView> window(new QQuickView);
    window->setSource(testFileUrl("padding.qml"));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QScopedPointer<QQuickItem> root(window->rootObject());
    QVERIFY(root);
    QQuickText *obj = qobject_cast<QQuickText*>(root.data());
    QVERIFY(obj != nullptr);

    qreal cw = obj->contentWidth();
    qreal ch = obj->contentHeight();

    QVERIFY(cw > 0);
    QVERIFY(ch > 0);

    QCOMPARE(obj->topPadding(), 20.0);
    QCOMPARE(obj->leftPadding(), 30.0);
    QCOMPARE(obj->rightPadding(), 40.0);
    QCOMPARE(obj->bottomPadding(), 50.0);

    QCOMPARE(obj->implicitWidth(), cw + obj->leftPadding() + obj->rightPadding());
    QCOMPARE(obj->implicitHeight(), ch + obj->topPadding() + obj->bottomPadding());

    obj->setTopPadding(2.25);
    QCOMPARE(obj->topPadding(), 2.25);
    QCOMPARE(obj->implicitHeight(), ch + obj->topPadding() + obj->bottomPadding());

    obj->setLeftPadding(3.75);
    QCOMPARE(obj->leftPadding(), 3.75);
    QCOMPARE(obj->implicitWidth(), cw + obj->leftPadding() + obj->rightPadding());

    obj->setRightPadding(4.4);
    QCOMPARE(obj->rightPadding(), 4.4);
    QCOMPARE(obj->implicitWidth(), cw + obj->leftPadding() + obj->rightPadding());

    obj->setBottomPadding(1.11);
    QCOMPARE(obj->bottomPadding(), 1.11);
    QCOMPARE(obj->implicitHeight(), ch + obj->topPadding() + obj->bottomPadding());

    obj->setWidth(cw / 2);
    obj->setElideMode(QQuickText::ElideRight);
    QCOMPARE(obj->implicitWidth(), cw + obj->leftPadding() + obj->rightPadding());
    QCOMPARE(obj->implicitHeight(), ch + obj->topPadding() + obj->bottomPadding());

    obj->setLeftPadding(0);
    QCOMPARE(obj->implicitWidth(), cw + obj->leftPadding() + obj->rightPadding());

    obj->setWidth(cw);
    obj->setRightPadding(cw);
    QCOMPARE(obj->contentWidth(), 0);

    for (int incr = 1; incr < 50 && qFuzzyIsNull(obj->contentWidth()); ++incr)
        obj->setWidth(cw + incr);
    QVERIFY(obj->contentWidth() > 0);
    qCDebug(lcTests) << "increasing Text width from" << cw << "to" << obj->width()
                     << "rendered a character: contentWidth now" << obj->contentWidth();

    obj->setElideMode(QQuickText::ElideNone);
    obj->resetWidth();

    obj->setWrapMode(QQuickText::WordWrap);
    QCOMPARE(obj->implicitWidth(), cw + obj->leftPadding() + obj->rightPadding());
    QCOMPARE(obj->implicitHeight(), ch + obj->topPadding() + obj->bottomPadding());
    obj->setWrapMode(QQuickText::NoWrap);

    obj->setText("Qt");
    QVERIFY(obj->contentWidth() < cw);
    QCOMPARE(obj->contentHeight(), ch);
    cw = obj->contentWidth();

    QCOMPARE(obj->implicitWidth(), cw + obj->leftPadding() + obj->rightPadding());
    QCOMPARE(obj->implicitHeight(), ch + obj->topPadding() + obj->bottomPadding());

    obj->setFont(QFont("Courier", 96));
    QVERIFY(obj->contentWidth() > cw);
    QVERIFY(obj->contentHeight() > ch);
    cw = obj->contentWidth();
    ch = obj->contentHeight();

    QCOMPARE(obj->implicitWidth(), cw + obj->leftPadding() + obj->rightPadding());
    QCOMPARE(obj->implicitHeight(), ch + obj->topPadding() + obj->bottomPadding());

    obj->resetTopPadding();
    QCOMPARE(obj->topPadding(), 10.0);
    obj->resetLeftPadding();
    QCOMPARE(obj->leftPadding(), 10.0);
    obj->resetRightPadding();
    QCOMPARE(obj->rightPadding(), 10.0);
    obj->resetBottomPadding();
    QCOMPARE(obj->bottomPadding(), 10.0);

    obj->resetPadding();
    QCOMPARE(obj->padding(), 0.0);
    QCOMPARE(obj->topPadding(), 0.0);
    QCOMPARE(obj->leftPadding(), 0.0);
    QCOMPARE(obj->rightPadding(), 0.0);
    QCOMPARE(obj->bottomPadding(), 0.0);
}

void tst_qquicktext::paddingInLoader() // QTBUG-83413
{
    QQuickView view;
    QVERIFY(QQuickTest::showView(view, testFileUrl("paddingInLoader.qml")));
    QQuickText *qtext = view.rootObject()->findChild<QQuickText*>();
    QVERIFY(qtext);
    QQuickTextPrivate *textPrivate = QQuickTextPrivate::get(qtext);
    QVERIFY(textPrivate);
    QCOMPARE(qtext->contentWidth(), 0); // does not render text, because width == rightPadding
    QCOMPARE(textPrivate->availableWidth(), 0);

    qtext->setLeftPadding(qtext->width());
    qtext->setRightPadding(0);
    QCOMPARE(qtext->contentWidth(), 0); // does not render text, because width == leftPadding
    QCOMPARE(textPrivate->availableWidth(), 0);

    qtext->setRightPadding(qtext->width());
    QCOMPARE(qtext->contentWidth(), 0); // does not render text: available space is negative
    QCOMPARE(textPrivate->availableWidth(), -qtext->width());

    qtext->setLeftPadding(2);
    qtext->setRightPadding(2);
    QVERIFY(qtext->contentWidth() > 0); // finally space is available to render text
    QCOMPARE(textPrivate->availableWidth(), qtext->width() - 4);
}

void tst_qquicktext::hintingPreference()
{
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\" }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE((int)textObject->font().hintingPreference(), (int)QFont::PreferDefaultHinting);
    }
    {
        QString componentStr = "import QtQuick 2.0\nText { text: \"Hello world!\"; font.hintingPreference: Font.PreferNoHinting }";
        QQmlComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QScopedPointer<QObject> object(textComponent.create());
        QQuickText *textObject = qobject_cast<QQuickText*>(object.data());

        QVERIFY(textObject != nullptr);
        QCOMPARE((int)textObject->font().hintingPreference(), (int)QFont::PreferNoHinting);
    }
}


void tst_qquicktext::zeroWidthAndElidedDoesntRender()
{
    // Tests QTBUG-34990

    QQmlComponent component(&engine, testFile("ellipsisText.qml"));

    QScopedPointer<QObject> object(component.create());

    QQuickText *text = qobject_cast<QQuickText *>(object.data());
    QVERIFY(text);

    QCOMPARE(text->contentWidth(), 0.0);

    QQuickText *reference = text->findChild<QQuickText *>("elidedRef");
    QVERIFY(reference);

    text->setWidth(10);
    QCOMPARE(text->contentWidth(), reference->contentWidth());
}

void tst_qquicktext::hAlignWidthDependsOnImplicitWidth_data()
{
    QTest::addColumn<QQuickText::HAlignment>("horizontalAlignment");
    QTest::addColumn<QQuickText::TextElideMode>("elide");
    QTest::addColumn<int>("extraWidth");

    QTest::newRow("AlignHCenter, ElideNone, 0 extraWidth") << QQuickText::AlignHCenter << QQuickText::ElideNone << 0;
    QTest::newRow("AlignRight, ElideNone, 0 extraWidth") << QQuickText::AlignRight << QQuickText::ElideNone << 0;
    QTest::newRow("AlignHCenter, ElideRight, 0 extraWidth") << QQuickText::AlignHCenter << QQuickText::ElideRight << 0;
    QTest::newRow("AlignRight, ElideRight, 0 extraWidth") << QQuickText::AlignRight << QQuickText::ElideRight << 0;
    QTest::newRow("AlignHCenter, ElideNone, 20 extraWidth") << QQuickText::AlignHCenter << QQuickText::ElideNone << 20;
    QTest::newRow("AlignRight, ElideNone, 20 extraWidth") << QQuickText::AlignRight << QQuickText::ElideNone << 20;
    QTest::newRow("AlignHCenter, ElideRight, 20 extraWidth") << QQuickText::AlignHCenter << QQuickText::ElideRight << 20;
    QTest::newRow("AlignRight, ElideRight, 20 extraWidth") << QQuickText::AlignRight << QQuickText::ElideRight << 20;
}

void tst_qquicktext::hAlignWidthDependsOnImplicitWidth()
{
    QFETCH(QQuickText::HAlignment, horizontalAlignment);
    QFETCH(QQuickText::TextElideMode, elide);
    QFETCH(int, extraWidth);

    QScopedPointer<QQuickView> window(new QQuickView);
    window->setSource(testFileUrl("hAlignWidthDependsOnImplicitWidth.qml"));
    QTRY_COMPARE(window->status(), QQuickView::Ready);

    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickItem *rect = window->rootObject();
    QVERIFY(rect);

    QVERIFY(rect->setProperty("horizontalAlignment", horizontalAlignment));
    QVERIFY(rect->setProperty("elide", elide));
    QVERIFY(rect->setProperty("extraWidth", extraWidth));

    QImage image = window->grabWindow();
    const int rectX = 100 * window->screen()->devicePixelRatio();
    QCOMPARE(numberOfNonWhitePixels(0, rectX - 1, image), 0);

    QVERIFY(rect->setProperty("text", "this is mis-aligned"));
    image = window->grabWindow();
    QCOMPARE(numberOfNonWhitePixels(0, rectX - 1, image), 0);
}

void tst_qquicktext::fontInfo()
{
    QQmlComponent component(&engine, testFile("fontInfo.qml"));

    QScopedPointer<QObject> object(component.create());
    QObject *root = object.data();
    QVERIFY2(root, qPrintable(component.errorString()));

    QQuickText *main = root->findChild<QQuickText *>("main");
    QVERIFY(main);
    QCOMPARE(main->font().pixelSize(), 1000);

    QQuickText *copy = root->findChild<QQuickText *>("copy");
    QVERIFY(copy);
    QCOMPARE(copy->font().family(), QFontInfo(QFont()).family());
    QVERIFY(copy->font().pixelSize() < 1000);
}

void tst_qquicktext::initialContentHeight()
{
    QQmlComponent component(&engine, testFile("contentHeight.qml"));
    QVERIFY(component.isReady());
    QScopedPointer<QObject> object(component.create());
    QObject *root = object.data();
    QVERIFY(root);
    QQuickText *text = qobject_cast<QQuickText *>(root);
    QVERIFY(text);
    QCOMPARE(text->height(), text->contentHeight());
}

void tst_qquicktext::implicitSizeChangeRewrap()
{
    QScopedPointer<QQuickView> window(new QQuickView);
    window->setSource(testFileUrl("implicitSizeChangeRewrap.qml"));
    QTRY_COMPARE(window->status(), QQuickView::Ready);

    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QObject *root = window->rootObject();

    QQuickText *text = root->findChild<QQuickText *>("text");
    QVERIFY(text != nullptr);

    QVERIFY(text->contentWidth() < window->width());
}

void tst_qquicktext::verticallyAlignedImageInTable()
{
    QScopedPointer<QQuickView> window(new QQuickView);
    window->setSource(testFileUrl("verticallyAlignedImageInTable.qml"));
    QTRY_COMPARE(window->status(), QQuickView::Ready);

    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    // Don't crash
}

void tst_qquicktext::transparentBackground()
{
    SKIP_IF_NO_WINDOW_GRAB;

    QScopedPointer<QQuickView> window(new QQuickView);
    window->setSource(testFileUrl("transparentBackground.qml"));
    QTRY_COMPARE(window->status(), QQuickView::Ready);

    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QImage img = window->grabWindow();
    QCOMPARE(img.isNull(), false);

    QColor color = img.pixelColor(0, 0);
    QCOMPARE(color.red(), 255);
    QCOMPARE(color.blue(), 255);
    QCOMPARE(color.green(), 255);
}

void tst_qquicktext::displaySuperscriptedTag()
{
    SKIP_IF_NO_WINDOW_GRAB;

    QScopedPointer<QQuickView> window(new QQuickView);
    window->setSource(testFileUrl("displaySuperscriptedTag.qml"));
    QTRY_COMPARE(window->status(), QQuickView::Ready);

    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickText *text = window->findChild<QQuickText *>("text");
    QVERIFY(text);

    QImage img = window->grabWindow();
    QCOMPARE(img.isNull(), false);

    QColor color = img.pixelColor(1, static_cast<int>(text->contentHeight()) / 4 * 3);
    QCOMPARE(color.red(), 255);
    QCOMPARE(color.blue(), 255);
    QCOMPARE(color.green(), 255);
}

void tst_qquicktext::textObjectRespectsDpr_data()
{
    QTest::addColumn<qreal>("devicePixelRatio");
    QTest::newRow("1x") << 1.0;
    QTest::newRow("2x") << 2.0;
}

class CustomTextObject : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    using QObject::QObject;

    QSizeF intrinsicSize(QTextDocument *, int, const QTextFormat &) override
    {
        return {100, 100};
    }

    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *,
                    int, const QTextFormat &) override
    {
        drawnDpr = static_cast<QImage*>(painter->device())->devicePixelRatio();
        drawnRect = rect;
    }

    qreal drawnDpr = 0.0;
    QRectF drawnRect;
};

void tst_qquicktext::textObjectRespectsDpr()
{
    QFETCH(qreal, devicePixelRatio);

    // Compute a global scale factor that results in the target devicePixelRatio,
    // even in the presence of an existing system-level devicePixelRatio.
    const auto *screen = qGuiApp->primaryScreen();
    const auto systemDevicePixelRatio = screen->devicePixelRatio();
    qreal globalScaleFactor = devicePixelRatio / systemDevicePixelRatio;
    QHighDpiScaling::setGlobalFactor(globalScaleFactor);
    QGuiApplicationPrivate::resetCachedDevicePixelRatio();
    auto reset = qScopeGuard([]{
        QHighDpiScaling::setGlobalFactor(1);
        QGuiApplicationPrivate::resetCachedDevicePixelRatio();
    });
    QCOMPARE(screen->devicePixelRatio(), devicePixelRatio);

    QScopedPointer<QQuickView> window(new QQuickView);
    window->setSource(testFileUrl("textObjectDpr.qml"));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    auto *text = window->findChild<QQuickText *>();
    QVERIFY(text);
    auto *textPrivate = QQuickTextPrivate::get(text);
    QVERIFY(textPrivate != nullptr);
    QVERIFY(textPrivate->extra.isAllocated());

    auto *textDocument = textPrivate->extra->doc;
    QVERIFY(textDocument);
    auto *documentLayout = textDocument->documentLayout();
    QVERIFY(documentLayout);

    CustomTextObject customTextObject;
    documentLayout->registerHandler(QTextCharFormat::UserObject, &customTextObject);

    QTextCursor cursor(textDocument);
    QTextCharFormat format;
    format.setObjectType(QTextCharFormat::UserObject);
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);

    // We've modified the internal state of the QQuickText, so make sure
    // to not touch setText or any of the APIs that will update the text
    // document, while still triggering a relayout/render of the text.
    text->invalidate();

    QTRY_VERIFY(!customTextObject.drawnRect.isNull());

    QCOMPARE(customTextObject.drawnDpr, devicePixelRatio);
    QCOMPARE(customTextObject.drawnRect.size(), customTextObject.intrinsicSize(textDocument, 0, {}));
}

QT_END_NAMESPACE

QTEST_MAIN(tst_qquicktext)

#include "tst_qquicktext.moc"
