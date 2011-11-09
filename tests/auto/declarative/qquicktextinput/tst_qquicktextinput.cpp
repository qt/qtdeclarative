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
#include <QtTest/QSignalSpy>
#include "../shared/util.h"
#include <QtDeclarative/qdeclarativeengine.h>
#include <QFile>
#include <QtDeclarative/qquickview.h>
#include <QtGui/qguiapplication.h>
#include <QInputPanel>
#include <private/qquicktextinput_p.h>
#include <private/qquicktextinput_p_p.h>
#include <QDebug>
#include <QDir>
#include <QStyle>
#include <QInputContext>
#include <private/qsgdistancefieldglyphcache_p.h>
#include <QtOpenGL/QGLShaderProgram>
#include <math.h>

#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

#include "qplatformdefs.h"

Q_DECLARE_METATYPE(QQuickTextInput::SelectionMode)
DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)

#define QTBUG_21691
#define QTBUG_21691_MESSAGE "QTBUG-21691: The test needs to be rewritten to not use QInputContext"


QString createExpectedFileIfNotFound(const QString& filebasename, const QImage& actual)
{
    // XXX This will be replaced by some clever persistent platform image store.
    QString persistent_dir = TESTDATA("");
    QString arch = "unknown-architecture"; // QTest needs to help with this.

    QString expectfile = persistent_dir + QDir::separator() + filebasename + "-" + arch + ".png";

    if (!QFile::exists(expectfile)) {
        actual.save(expectfile);
        qWarning() << "created" << expectfile;
    }

    return expectfile;
}

class tst_qquicktextinput : public QObject

{
    Q_OBJECT
public:
    tst_qquicktextinput();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void text();
    void width();
    void font();
    void color();
    void selection();
    void isRightToLeft_data();
    void isRightToLeft();
    void moveCursorSelection_data();
    void moveCursorSelection();
    void moveCursorSelectionSequence_data();
    void moveCursorSelectionSequence();
    void dragMouseSelection();
    void mouseSelectionMode_data();
    void mouseSelectionMode();
    void tripleClickSelectsAll();

    void horizontalAlignment_data();
    void horizontalAlignment();
    void horizontalAlignment_RightToLeft();

    void positionAt();

    void maxLength();
    void masks();
    void validators();
    void inputMethods();

    void passwordCharacter();
    void cursorDelegate();
    void cursorVisible();
    void cursorRectangle();
    void navigation();
    void navigation_RTL();
    void copyAndPaste();
    void canPasteEmpty();
    void canPaste();
    void readOnly();

    void openInputPanel();
    void setHAlignClearCache();
    void focusOutClearSelection();

    void echoMode();
#ifdef QT_GUI_PASSWORD_ECHO_DELAY
    void passwordEchoDelay();
#endif
    void geometrySignals();
    void testQtQuick11Attributes();
    void testQtQuick11Attributes_data();

    void preeditAutoScroll();
    void preeditCursorRectangle();
    void inputMethodComposing();
    void cursorRectangleSize();

    void QTBUG_19956();
    void QTBUG_19956_data();
    void QTBUG_19956_regexp();

private:
    void simulateKey(QQuickView *, int key);

    QDeclarativeEngine engine;
    QStringList standard;
    QStringList colorStrings;
};
void tst_qquicktextinput::initTestCase()
{
}

void tst_qquicktextinput::cleanupTestCase()
{

}
tst_qquicktextinput::tst_qquicktextinput()
{
    standard << "the quick brown fox jumped over the lazy dog"
        << "It's supercalifragisiticexpialidocious!"
        << "Hello, world!"
        << "!dlrow ,olleH"
        << " spacey   text ";

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
}

void tst_qquicktextinput::text()
{
    {
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData("import QtQuick 2.0\nTextInput {  text: \"\"  }", QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());

        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->text(), QString(""));

        delete textinputObject;
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextInput { text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());

        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->text(), standard.at(i));

        delete textinputObject;
    }

}

void tst_qquicktextinput::width()
{
    // uses Font metrics to find the width for standard
    {
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData("import QtQuick 2.0\nTextInput {  text: \"\" }", QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());

        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->width(), 0.0);

        delete textinputObject;
    }

    bool requiresUnhintedMetrics = !qmlDisableDistanceField();

    for (int i = 0; i < standard.size(); i++)
    {
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

            metricWidth = ceil(layout.boundingRect().width());
        } else {
            QFontMetricsF fm(f);
            metricWidth = fm.width(standard.at(i));
        }

        QString componentStr = "import QtQuick 2.0\nTextInput { text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());

        QVERIFY(textinputObject != 0);
        int delta = abs(int(int(textinputObject->width()) - metricWidth));
        QVERIFY(delta <= 3.0); // As best as we can hope for cross-platform.

        delete textinputObject;
    }
}

void tst_qquicktextinput::font()
{
    //test size, then bold, then italic, then family
    {
        QString componentStr = "import QtQuick 2.0\nTextInput {  font.pointSize: 40; text: \"Hello World\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());

        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->font().pointSize(), 40);
        QCOMPARE(textinputObject->font().bold(), false);
        QCOMPARE(textinputObject->font().italic(), false);

        delete textinputObject;
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextInput {  font.bold: true; text: \"Hello World\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());

        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->font().bold(), true);
        QCOMPARE(textinputObject->font().italic(), false);

        delete textinputObject;
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextInput {  font.italic: true; text: \"Hello World\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());

        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->font().italic(), true);
        QCOMPARE(textinputObject->font().bold(), false);

        delete textinputObject;
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextInput {  font.family: \"Helvetica\"; text: \"Hello World\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());

        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->font().family(), QString("Helvetica"));
        QCOMPARE(textinputObject->font().bold(), false);
        QCOMPARE(textinputObject->font().italic(), false);

        delete textinputObject;
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextInput {  font.family: \"\"; text: \"Hello World\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());

        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->font().family(), QString(""));

        delete textinputObject;
    }
}

void tst_qquicktextinput::color()
{
    //test color
    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextInput {  color: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());
        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->color(), QColor(colorStrings.at(i)));

        delete textinputObject;
    }

    //test selection color
    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextInput {  selectionColor: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());
        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->selectionColor(), QColor(colorStrings.at(i)));

        delete textinputObject;
    }

    //test selected text color
    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextInput {  selectedTextColor: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());
        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->selectedTextColor(), QColor(colorStrings.at(i)));

        delete textinputObject;
    }

    {
        QString colorStr = "#AA001234";
        QColor testColor("#001234");
        testColor.setAlpha(170);

        QString componentStr = "import QtQuick 2.0\nTextInput {  color: \"" + colorStr + "\"; text: \"Hello World\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());

        QVERIFY(textinputObject != 0);
        QCOMPARE(textinputObject->color(), testColor);

        delete textinputObject;
    }
}

void tst_qquicktextinput::selection()
{
    QString testStr = standard[0];
    QString componentStr = "import QtQuick 2.0\nTextInput {  text: \""+ testStr +"\"; }";
    QDeclarativeComponent textinputComponent(&engine);
    textinputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());
    QVERIFY(textinputObject != 0);


    //Test selection follows cursor
    for (int i=0; i<= testStr.size(); i++) {
        textinputObject->setCursorPosition(i);
        QCOMPARE(textinputObject->cursorPosition(), i);
        QCOMPARE(textinputObject->selectionStart(), i);
        QCOMPARE(textinputObject->selectionEnd(), i);
        QVERIFY(textinputObject->selectedText().isNull());
    }

    textinputObject->setCursorPosition(0);
    QVERIFY(textinputObject->cursorPosition() == 0);
    QVERIFY(textinputObject->selectionStart() == 0);
    QVERIFY(textinputObject->selectionEnd() == 0);
    QVERIFY(textinputObject->selectedText().isNull());

    // Verify invalid positions are ignored.
    textinputObject->setCursorPosition(-1);
    QVERIFY(textinputObject->cursorPosition() == 0);
    QVERIFY(textinputObject->selectionStart() == 0);
    QVERIFY(textinputObject->selectionEnd() == 0);
    QVERIFY(textinputObject->selectedText().isNull());

    textinputObject->setCursorPosition(textinputObject->text().count()+1);
    QVERIFY(textinputObject->cursorPosition() == 0);
    QVERIFY(textinputObject->selectionStart() == 0);
    QVERIFY(textinputObject->selectionEnd() == 0);
    QVERIFY(textinputObject->selectedText().isNull());

    //Test selection
    for (int i=0; i<= testStr.size(); i++) {
        textinputObject->select(0,i);
        QCOMPARE(testStr.mid(0,i), textinputObject->selectedText());
    }
    for (int i=0; i<= testStr.size(); i++) {
        textinputObject->select(i,testStr.size());
        QCOMPARE(testStr.mid(i,testStr.size()-i), textinputObject->selectedText());
    }

    textinputObject->setCursorPosition(0);
    QVERIFY(textinputObject->cursorPosition() == 0);
    QVERIFY(textinputObject->selectionStart() == 0);
    QVERIFY(textinputObject->selectionEnd() == 0);
    QVERIFY(textinputObject->selectedText().isNull());

    //Test Error Ignoring behaviour
    textinputObject->setCursorPosition(0);
    QVERIFY(textinputObject->selectedText().isNull());
    textinputObject->select(-10,0);
    QVERIFY(textinputObject->selectedText().isNull());
    textinputObject->select(100,110);
    QVERIFY(textinputObject->selectedText().isNull());
    textinputObject->select(0,-10);
    QVERIFY(textinputObject->selectedText().isNull());
    textinputObject->select(0,100);
    QVERIFY(textinputObject->selectedText().isNull());
    textinputObject->select(0,10);
    QVERIFY(textinputObject->selectedText().size() == 10);
    textinputObject->select(-10,10);
    QVERIFY(textinputObject->selectedText().size() == 10);
    textinputObject->select(100,101);
    QVERIFY(textinputObject->selectedText().size() == 10);
    textinputObject->select(0,-10);
    QVERIFY(textinputObject->selectedText().size() == 10);
    textinputObject->select(0,100);
    QVERIFY(textinputObject->selectedText().size() == 10);

    textinputObject->deselect();
    QVERIFY(textinputObject->selectedText().isNull());
    textinputObject->select(0,10);
    QVERIFY(textinputObject->selectedText().size() == 10);
    textinputObject->deselect();
    QVERIFY(textinputObject->selectedText().isNull());

    delete textinputObject;
}

void tst_qquicktextinput::isRightToLeft_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("emptyString");
    QTest::addColumn<bool>("firstCharacter");
    QTest::addColumn<bool>("lastCharacter");
    QTest::addColumn<bool>("middleCharacter");
    QTest::addColumn<bool>("startString");
    QTest::addColumn<bool>("midString");
    QTest::addColumn<bool>("endString");

    const quint16 arabic_str[] = { 0x0638, 0x0643, 0x00646, 0x0647, 0x0633, 0x0638, 0x0643, 0x00646, 0x0647, 0x0633, 0x0647};
    QTest::newRow("Empty") << "" << false << false << false << false << false << false << false;
    QTest::newRow("Neutral") << "23244242" << false << false << false << false << false << false << false;
    QTest::newRow("LTR") << "Hello world" << false << false << false << false << false << false << false;
    QTest::newRow("RTL") << QString::fromUtf16(arabic_str, 11) << false << true << true << true << true << true << true;
    QTest::newRow("Bidi RTL + LTR + RTL") << QString::fromUtf16(arabic_str, 11) + QString("Hello world") + QString::fromUtf16(arabic_str, 11) << false << true << true << false << true << true << true;
    QTest::newRow("Bidi LTR + RTL + LTR") << QString("Hello world") + QString::fromUtf16(arabic_str, 11) + QString("Hello world") << false << false << false << true << false << false << false;
}

void tst_qquicktextinput::isRightToLeft()
{
    QFETCH(QString, text);
    QFETCH(bool, emptyString);
    QFETCH(bool, firstCharacter);
    QFETCH(bool, lastCharacter);
    QFETCH(bool, middleCharacter);
    QFETCH(bool, startString);
    QFETCH(bool, midString);
    QFETCH(bool, endString);

    QQuickTextInput textInput;
    textInput.setText(text);

    // first test that the right string is delivered to the QString::isRightToLeft()
    QCOMPARE(textInput.isRightToLeft(0,0), text.mid(0,0).isRightToLeft());
    QCOMPARE(textInput.isRightToLeft(0,1), text.mid(0,1).isRightToLeft());
    QCOMPARE(textInput.isRightToLeft(text.count()-2, text.count()-1), text.mid(text.count()-2, text.count()-1).isRightToLeft());
    QCOMPARE(textInput.isRightToLeft(text.count()/2, text.count()/2 + 1), text.mid(text.count()/2, text.count()/2 + 1).isRightToLeft());
    QCOMPARE(textInput.isRightToLeft(0,text.count()/4), text.mid(0,text.count()/4).isRightToLeft());
    QCOMPARE(textInput.isRightToLeft(text.count()/4,3*text.count()/4), text.mid(text.count()/4,3*text.count()/4).isRightToLeft());
    if (text.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML TextInput: isRightToLeft(start, end) called with the end property being smaller than the start.");
    QCOMPARE(textInput.isRightToLeft(3*text.count()/4,text.count()-1), text.mid(3*text.count()/4,text.count()-1).isRightToLeft());

    // then test that the feature actually works
    QCOMPARE(textInput.isRightToLeft(0,0), emptyString);
    QCOMPARE(textInput.isRightToLeft(0,1), firstCharacter);
    QCOMPARE(textInput.isRightToLeft(text.count()-2, text.count()-1), lastCharacter);
    QCOMPARE(textInput.isRightToLeft(text.count()/2, text.count()/2 + 1), middleCharacter);
    QCOMPARE(textInput.isRightToLeft(0,text.count()/4), startString);
    QCOMPARE(textInput.isRightToLeft(text.count()/4,3*text.count()/4), midString);
    if (text.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML TextInput: isRightToLeft(start, end) called with the end property being smaller than the start.");
    QCOMPARE(textInput.isRightToLeft(3*text.count()/4,text.count()-1), endString);
}

void tst_qquicktextinput::moveCursorSelection_data()
{
    QTest::addColumn<QString>("testStr");
    QTest::addColumn<int>("cursorPosition");
    QTest::addColumn<int>("movePosition");
    QTest::addColumn<QQuickTextInput::SelectionMode>("mode");
    QTest::addColumn<int>("selectionStart");
    QTest::addColumn<int>("selectionEnd");
    QTest::addColumn<bool>("reversible");

    // () contains the text selected by the cursor.
    // <> contains the actual selection.

    QTest::newRow("(t)he|characters")
            << standard[0] << 0 << 1 << QQuickTextInput::SelectCharacters << 0 << 1 << true;
    QTest::newRow("do(g)|characters")
            << standard[0] << 43 << 44 << QQuickTextInput::SelectCharacters << 43 << 44 << true;
    QTest::newRow("jum(p)ed|characters")
            << standard[0] << 23 << 24 << QQuickTextInput::SelectCharacters << 23 << 24 << true;
    QTest::newRow("jumped( )over|characters")
            << standard[0] << 26 << 27 << QQuickTextInput::SelectCharacters << 26 << 27 << true;
    QTest::newRow("(the )|characters")
            << standard[0] << 0 << 4 << QQuickTextInput::SelectCharacters << 0 << 4 << true;
    QTest::newRow("( dog)|characters")
            << standard[0] << 40 << 44 << QQuickTextInput::SelectCharacters << 40 << 44 << true;
    QTest::newRow("( jumped )|characters")
            << standard[0] << 19 << 27 << QQuickTextInput::SelectCharacters << 19 << 27 << true;
    QTest::newRow("th(e qu)ick|characters")
            << standard[0] << 2 << 6 << QQuickTextInput::SelectCharacters << 2 << 6 << true;
    QTest::newRow("la(zy d)og|characters")
            << standard[0] << 38 << 42 << QQuickTextInput::SelectCharacters << 38 << 42 << true;
    QTest::newRow("jum(ped ov)er|characters")
            << standard[0] << 23 << 29 << QQuickTextInput::SelectCharacters << 23 << 29 << true;
    QTest::newRow("()the|characters")
            << standard[0] << 0 << 0 << QQuickTextInput::SelectCharacters << 0 << 0 << true;
    QTest::newRow("dog()|characters")
            << standard[0] << 44 << 44 << QQuickTextInput::SelectCharacters << 44 << 44 << true;
    QTest::newRow("jum()ped|characters")
            << standard[0] << 23 << 23 << QQuickTextInput::SelectCharacters << 23 << 23 << true;

    QTest::newRow("<(t)he>|words")
            << standard[0] << 0 << 1 << QQuickTextInput::SelectWords << 0 << 3 << true;
    QTest::newRow("<do(g)>|words")
            << standard[0] << 43 << 44 << QQuickTextInput::SelectWords << 41 << 44 << true;
    QTest::newRow("<jum(p)ed>|words")
            << standard[0] << 23 << 24 << QQuickTextInput::SelectWords << 20 << 26 << true;
    QTest::newRow("<jumped( )>over|words,ltr")
            << standard[0] << 26 << 27 << QQuickTextInput::SelectWords << 20 << 27 << false;
    QTest::newRow("jumped<( )over>|words,rtl")
            << standard[0] << 27 << 26 << QQuickTextInput::SelectWords << 26 << 31 << false;
    QTest::newRow("<(the )>quick|words,ltr")
            << standard[0] << 0 << 4 << QQuickTextInput::SelectWords << 0 << 4 << false;
    QTest::newRow("<(the )quick>|words,rtl")
            << standard[0] << 4 << 0 << QQuickTextInput::SelectWords << 0 << 9 << false;
    QTest::newRow("<lazy( dog)>|words,ltr")
            << standard[0] << 40 << 44 << QQuickTextInput::SelectWords << 36 << 44 << false;
    QTest::newRow("lazy<( dog)>|words,rtl")
            << standard[0] << 44 << 40 << QQuickTextInput::SelectWords << 40 << 44 << false;
    QTest::newRow("<fox( jumped )>over|words,ltr")
            << standard[0] << 19 << 27 << QQuickTextInput::SelectWords << 16 << 27 << false;
    QTest::newRow("fox<( jumped )over>|words,rtl")
            << standard[0] << 27 << 19 << QQuickTextInput::SelectWords << 19 << 31 << false;
    QTest::newRow("<th(e qu)ick>|words")
            << standard[0] << 2 << 6 << QQuickTextInput::SelectWords << 0 << 9 << true;
    QTest::newRow("<la(zy d)og|words>")
            << standard[0] << 38 << 42 << QQuickTextInput::SelectWords << 36 << 44 << true;
    QTest::newRow("<jum(ped ov)er>|words")
            << standard[0] << 23 << 29 << QQuickTextInput::SelectWords << 20 << 31 << true;
    QTest::newRow("<()>the|words")
            << standard[0] << 0 << 0 << QQuickTextInput::SelectWords << 0 << 0 << true;
    QTest::newRow("dog<()>|words")
            << standard[0] << 44 << 44 << QQuickTextInput::SelectWords << 44 << 44 << true;
    QTest::newRow("jum<()>ped|words")
            << standard[0] << 23 << 23 << QQuickTextInput::SelectWords << 23 << 23 << true;

    QTest::newRow("Hello<(,)> |words")
            << standard[2] << 5 << 6 << QQuickTextInput::SelectWords << 5 << 6 << true;
    QTest::newRow("Hello<(, )>world|words,ltr")
            << standard[2] << 5 << 7 << QQuickTextInput::SelectWords << 5 << 7 << false;
    QTest::newRow("Hello<(, )world>|words,rtl")
            << standard[2] << 7 << 5 << QQuickTextInput::SelectWords << 5 << 12 << false;
    QTest::newRow("<Hel(lo, )>world|words,ltr")
            << standard[2] << 3 << 7 << QQuickTextInput::SelectWords << 0 << 7 << false;
    QTest::newRow("<Hel(lo, )world>|words,rtl")
            << standard[2] << 7 << 3 << QQuickTextInput::SelectWords << 0 << 12 << false;
    QTest::newRow("<Hel(lo)>,|words")
            << standard[2] << 3 << 5 << QQuickTextInput::SelectWords << 0 << 5 << true;
    QTest::newRow("Hello<()>,|words")
            << standard[2] << 5 << 5 << QQuickTextInput::SelectWords << 5 << 5 << true;
    QTest::newRow("Hello,<()>|words")
            << standard[2] << 6 << 6 << QQuickTextInput::SelectWords << 6 << 6 << true;
    QTest::newRow("Hello<,( )>world|words,ltr")
            << standard[2] << 6 << 7 << QQuickTextInput::SelectWords << 5 << 7 << false;
    QTest::newRow("Hello,<( )world>|words,rtl")
            << standard[2] << 7 << 6 << QQuickTextInput::SelectWords << 6 << 12 << false;
    QTest::newRow("Hello<,( world)>|words,ltr")
            << standard[2] << 6 << 12 << QQuickTextInput::SelectWords << 5 << 12 << false;
    QTest::newRow("Hello,<( world)>|words,rtl")
            << standard[2] << 12 << 6 << QQuickTextInput::SelectWords << 6 << 12 << false;
    QTest::newRow("Hello<,( world!)>|words,ltr")
            << standard[2] << 6 << 13 << QQuickTextInput::SelectWords << 5 << 13 << false;
    QTest::newRow("Hello,<( world!)>|words,rtl")
            << standard[2] << 13 << 6 << QQuickTextInput::SelectWords << 6 << 13 << false;
    QTest::newRow("Hello<(, world!)>|words")
            << standard[2] << 5 << 13 << QQuickTextInput::SelectWords << 5 << 13 << true;
    // Fails due to an issue with QTextBoundaryFinder and punctuation at the end of strings.
    // QTBUG-11365
    // QTest::newRow("world<(!)>|words")
    //         << standard[2] << 12 << 13 << QQuickTextInput::SelectWords << 12 << 13 << true;
    QTest::newRow("world!<()>)|words")
            << standard[2] << 13 << 13 << QQuickTextInput::SelectWords << 13 << 13 << true;
    QTest::newRow("world<()>!)|words")
            << standard[2] << 12 << 12 << QQuickTextInput::SelectWords << 12 << 12 << true;

    QTest::newRow("<(,)>olleH |words")
            << standard[3] << 7 << 8 << QQuickTextInput::SelectWords << 7 << 8 << true;
    QTest::newRow("<dlrow( ,)>olleH|words,ltr")
            << standard[3] << 6 << 8 << QQuickTextInput::SelectWords << 1 << 8 << false;
    QTest::newRow("dlrow<( ,)>olleH|words,rtl")
            << standard[3] << 8 << 6 << QQuickTextInput::SelectWords << 6 << 8 << false;
    QTest::newRow("<dlrow( ,ol)leH>|words,ltr")
            << standard[3] << 6 << 10 << QQuickTextInput::SelectWords << 1 << 13 << false;
    QTest::newRow("dlrow<( ,ol)leH>|words,rtl")
            << standard[3] << 10 << 6 << QQuickTextInput::SelectWords << 6 << 13 << false;
    QTest::newRow(",<(ol)leH>,|words")
            << standard[3] << 8 << 10 << QQuickTextInput::SelectWords << 8 << 13 << true;
    QTest::newRow(",<()>olleH|words")
            << standard[3] << 8 << 8 << QQuickTextInput::SelectWords << 8 << 8 << true;
    QTest::newRow("<()>,olleH|words")
            << standard[3] << 7 << 7 << QQuickTextInput::SelectWords << 7 << 7 << true;
    QTest::newRow("<dlrow( )>,olleH|words,ltr")
            << standard[3] << 6 << 7 << QQuickTextInput::SelectWords << 1 << 7 << false;
    QTest::newRow("dlrow<( ),>olleH|words,rtl")
            << standard[3] << 7 << 6 << QQuickTextInput::SelectWords << 6 << 8 << false;
    QTest::newRow("<(dlrow )>,olleH|words,ltr")
            << standard[3] << 1 << 7 << QQuickTextInput::SelectWords << 1 << 7 << false;
    QTest::newRow("<(dlrow ),>olleH|words,rtl")
            << standard[3] << 7 << 1 << QQuickTextInput::SelectWords << 1 << 8 << false;
    QTest::newRow("<(!dlrow )>,olleH|words,ltr")
            << standard[3] << 0 << 7 << QQuickTextInput::SelectWords << 0 << 7 << false;
    QTest::newRow("<(!dlrow ),>olleH|words,rtl")
            << standard[3] << 7 << 0 << QQuickTextInput::SelectWords << 0 << 8 << false;
    QTest::newRow("(!dlrow ,)olleH|words")
            << standard[3] << 0 << 8 << QQuickTextInput::SelectWords << 0 << 8 << true;
    QTest::newRow("<(!)>dlrow|words")
            << standard[3] << 0 << 1 << QQuickTextInput::SelectWords << 0 << 1 << true;
    QTest::newRow("<()>!dlrow|words")
            << standard[3] << 0 << 0 << QQuickTextInput::SelectWords << 0 << 0 << true;
    QTest::newRow("!<()>dlrow|words")
            << standard[3] << 1 << 1 << QQuickTextInput::SelectWords << 1 << 1 << true;

    QTest::newRow(" <s(pac)ey>   text |words")
            << standard[4] << 1 << 4 << QQuickTextInput::SelectWords << 1 << 7 << true;
    QTest::newRow(" spacey   <t(ex)t> |words")
            << standard[4] << 11 << 13 << QQuickTextInput::SelectWords << 10 << 14 << false; // Should be reversible. QTBUG-11365
    QTest::newRow("<( )>spacey   text |words|ltr")
            << standard[4] << 0 << 1 << QQuickTextInput::SelectWords << 0 << 1 << false;
    QTest::newRow("<( )spacey>   text |words|rtl")
            << standard[4] << 1 << 0 << QQuickTextInput::SelectWords << 0 << 7 << false;
    QTest::newRow("spacey   <text( )>|words|ltr")
            << standard[4] << 14 << 15 << QQuickTextInput::SelectWords << 10 << 15 << false;
//    QTBUG-11365
//    QTest::newRow("spacey   text<( )>|words|rtl")
//            << standard[4] << 15 << 14 << QQuickTextInput::SelectWords << 14 << 15 << false;
    QTest::newRow("<()> spacey   text |words")
            << standard[4] << 0 << 0 << QQuickTextInput::SelectWords << 0 << 0 << false;
    QTest::newRow(" spacey   text <()>|words")
            << standard[4] << 15 << 15 << QQuickTextInput::SelectWords << 15 << 15 << false;
}

void tst_qquicktextinput::moveCursorSelection()
{
    QFETCH(QString, testStr);
    QFETCH(int, cursorPosition);
    QFETCH(int, movePosition);
    QFETCH(QQuickTextInput::SelectionMode, mode);
    QFETCH(int, selectionStart);
    QFETCH(int, selectionEnd);
    QFETCH(bool, reversible);

    QString componentStr = "import QtQuick 2.0\nTextInput {  text: \""+ testStr +"\"; }";
    QDeclarativeComponent textinputComponent(&engine);
    textinputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());
    QVERIFY(textinputObject != 0);

    textinputObject->setCursorPosition(cursorPosition);
    textinputObject->moveCursorSelection(movePosition, mode);

    QCOMPARE(textinputObject->selectedText(), testStr.mid(selectionStart, selectionEnd - selectionStart));
    QCOMPARE(textinputObject->selectionStart(), selectionStart);
    QCOMPARE(textinputObject->selectionEnd(), selectionEnd);

    if (reversible) {
        textinputObject->setCursorPosition(movePosition);
        textinputObject->moveCursorSelection(cursorPosition, mode);

        QCOMPARE(textinputObject->selectedText(), testStr.mid(selectionStart, selectionEnd - selectionStart));
        QCOMPARE(textinputObject->selectionStart(), selectionStart);
        QCOMPARE(textinputObject->selectionEnd(), selectionEnd);
    }

    delete textinputObject;
}

void tst_qquicktextinput::moveCursorSelectionSequence_data()
{
    QTest::addColumn<QString>("testStr");
    QTest::addColumn<int>("cursorPosition");
    QTest::addColumn<int>("movePosition1");
    QTest::addColumn<int>("movePosition2");
    QTest::addColumn<int>("selection1Start");
    QTest::addColumn<int>("selection1End");
    QTest::addColumn<int>("selection2Start");
    QTest::addColumn<int>("selection2End");

    // () contains the text selected by the cursor.
    // <> contains the actual selection.
    // ^ is the revised cursor position.
    // {} contains the revised selection.

    QTest::newRow("the {<quick( bro)wn> f^ox} jumped|ltr")
            << standard[0]
            << 9 << 13 << 17
            << 4 << 15
            << 4 << 19;
    QTest::newRow("the quick<( {bro)wn> f^ox} jumped|rtl")
            << standard[0]
            << 13 << 9 << 17
            << 9 << 15
            << 10 << 19;
    QTest::newRow("the {<quick( bro)wn> ^}fox jumped|ltr")
            << standard[0]
            << 9 << 13 << 16
            << 4 << 15
            << 4 << 16;
    QTest::newRow("the quick<( {bro)wn> ^}fox jumped|rtl")
            << standard[0]
            << 13 << 9 << 16
            << 9 << 15
            << 10 << 16;
    QTest::newRow("the {<quick( bro)wn^>} fox jumped|ltr")
            << standard[0]
            << 9 << 13 << 15
            << 4 << 15
            << 4 << 15;
    QTest::newRow("the quick<( {bro)wn^>} f^ox jumped|rtl")
            << standard[0]
            << 13 << 9 << 15
            << 9 << 15
            << 10 << 15;
    QTest::newRow("the {<quick() ^}bro)wn> fox|ltr")
            << standard[0]
            << 9 << 13 << 10
            << 4 << 15
            << 4 << 10;
    QTest::newRow("the quick<( {^bro)wn>} fox|rtl")
            << standard[0]
            << 13 << 9 << 10
            << 9 << 15
            << 10 << 15;
    QTest::newRow("the {<quick^}( bro)wn> fox|ltr")
            << standard[0]
            << 9 << 13 << 9
            << 4 << 15
            << 4 << 9;
    QTest::newRow("the quick{<(^ bro)wn>} fox|rtl")
            << standard[0]
            << 13 << 9 << 9
            << 9 << 15
            << 9 << 15;
    QTest::newRow("the {<qui^ck}( bro)wn> fox|ltr")
            << standard[0]
            << 9 << 13 << 7
            << 4 << 15
            << 4 << 9;
    QTest::newRow("the {<qui^ck}( bro)wn> fox|rtl")
            << standard[0]
            << 13 << 9 << 7
            << 9 << 15
            << 4 << 15;
    QTest::newRow("the {<^quick}( bro)wn> fox|ltr")
            << standard[0]
            << 9 << 13 << 4
            << 4 << 15
            << 4 << 9;
    QTest::newRow("the {<^quick}( bro)wn> fox|rtl")
            << standard[0]
            << 13 << 9 << 4
            << 9 << 15
            << 4 << 15;
    QTest::newRow("the{^ <quick}( bro)wn> fox|ltr")
            << standard[0]
            << 9 << 13 << 3
            << 4 << 15
            << 3 << 9;
    QTest::newRow("the{^ <quick}( bro)wn> fox|rtl")
            << standard[0]
            << 13 << 9 << 3
            << 9 << 15
            << 3 << 15;
    QTest::newRow("{t^he <quick}( bro)wn> fox|ltr")
            << standard[0]
            << 9 << 13 << 1
            << 4 << 15
            << 0 << 9;
    QTest::newRow("{t^he <quick}( bro)wn> fox|rtl")
            << standard[0]
            << 13 << 9 << 1
            << 9 << 15
            << 0 << 15;

    QTest::newRow("{<He(ll)o>, w^orld}!|ltr")
            << standard[2]
            << 2 << 4 << 8
            << 0 << 5
            << 0 << 12;
    QTest::newRow("{<He(ll)o>, w^orld}!|rtl")
            << standard[2]
            << 4 << 2 << 8
            << 0 << 5
            << 0 << 12;

    QTest::newRow("!{dlro^w ,<o(ll)eH>}|ltr")
            << standard[3]
            << 9 << 11 << 5
            << 8 << 13
            << 1 << 13;
    QTest::newRow("!{dlro^w ,<o(ll)eH>}|rtl")
            << standard[3]
            << 11 << 9 << 5
            << 8 << 13
            << 1 << 13;

    QTest::newRow("{<(^} sp)acey>   text |ltr")
            << standard[4]
            << 0 << 3 << 0
            << 0 << 7
            << 0 << 0;
    QTest::newRow("{<( ^}sp)acey>   text |ltr")
            << standard[4]
            << 0 << 3 << 1
            << 0 << 7
            << 0 << 1;
    QTest::newRow("<( {s^p)acey>}   text |rtl")
            << standard[4]
            << 3 << 0 << 2
            << 0 << 7
            << 1 << 7;
    QTest::newRow("<( {^sp)acey>}   text |rtl")
            << standard[4]
            << 3 << 0 << 1
            << 0 << 7
            << 1 << 7;

    QTest::newRow(" spacey   <te(xt {^)>}|rtl")
            << standard[4]
            << 15 << 12 << 15
            << 10 << 15
            << 15 << 15;
//    QTBUG-11365
//    QTest::newRow(" spacey   <te(xt{^ )>}|rtl")
//            << standard[4]
//            << 15 << 12 << 14
//            << 10 << 15
//            << 14 << 15;
    QTest::newRow(" spacey   {<te(x^t} )>|ltr")
            << standard[4]
            << 12 << 15 << 13
            << 10 << 15
            << 10 << 14;
//    QTBUG-11365
//    QTest::newRow(" spacey   {<te(xt^} )>|ltr")
//            << standard[4]
//            << 12 << 15 << 14
//            << 10 << 15
//            << 10 << 14;
}

void tst_qquicktextinput::moveCursorSelectionSequence()
{
    QFETCH(QString, testStr);
    QFETCH(int, cursorPosition);
    QFETCH(int, movePosition1);
    QFETCH(int, movePosition2);
    QFETCH(int, selection1Start);
    QFETCH(int, selection1End);
    QFETCH(int, selection2Start);
    QFETCH(int, selection2End);

    QString componentStr = "import QtQuick 2.0\nTextInput {  text: \""+ testStr +"\"; }";
    QDeclarativeComponent textinputComponent(&engine);
    textinputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());
    QVERIFY(textinputObject != 0);

    textinputObject->setCursorPosition(cursorPosition);

    textinputObject->moveCursorSelection(movePosition1, QQuickTextInput::SelectWords);
    QCOMPARE(textinputObject->selectedText(), testStr.mid(selection1Start, selection1End - selection1Start));
    QCOMPARE(textinputObject->selectionStart(), selection1Start);
    QCOMPARE(textinputObject->selectionEnd(), selection1End);

    textinputObject->moveCursorSelection(movePosition2, QQuickTextInput::SelectWords);
    QCOMPARE(textinputObject->selectedText(), testStr.mid(selection2Start, selection2End - selection2Start));
    QCOMPARE(textinputObject->selectionStart(), selection2Start);
    QCOMPARE(textinputObject->selectionEnd(), selection2End);

    delete textinputObject;
}

void tst_qquicktextinput::dragMouseSelection()
{
    QString qmlfile = TESTDATA("mouseselection_true.qml");

    QQuickView canvas(QUrl::fromLocalFile(qmlfile));

    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);

    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());

    QVERIFY(canvas.rootObject() != 0);
    QQuickTextInput *textInputObject = qobject_cast<QQuickTextInput *>(canvas.rootObject());
    QVERIFY(textInputObject != 0);

    // press-and-drag-and-release from x1 to x2
    int x1 = 10;
    int x2 = 70;
    int y = textInputObject->height()/2;
    QTest::mousePress(&canvas, Qt::LeftButton, 0, QPoint(x1,y));
    QTest::mouseMove(&canvas, QPoint(x2, y));
    QTest::mouseRelease(&canvas, Qt::LeftButton, 0, QPoint(x2,y));
    QTest::qWait(100);
    QString str1;
    QVERIFY((str1 = textInputObject->selectedText()).length() > 3);
    QVERIFY(str1.length() > 3);

    // press and drag the current selection.
    x1 = 40;
    x2 = 100;
    QTest::mousePress(&canvas, Qt::LeftButton, 0, QPoint(x1,y));
    QTest::mouseMove(&canvas, QPoint(x2, y));
    QTest::mouseRelease(&canvas, Qt::LeftButton, 0, QPoint(x2,y));
    QTest::qWait(300);
    QString str2 = textInputObject->selectedText();
    QVERIFY(str2.length() > 3);

    QVERIFY(str1 != str2);
}

void tst_qquicktextinput::mouseSelectionMode_data()
{
    QTest::addColumn<QString>("qmlfile");
    QTest::addColumn<bool>("selectWords");

    // import installed
    QTest::newRow("SelectWords") << TESTDATA("mouseselectionmode_words.qml") << true;
    QTest::newRow("SelectCharacters") << TESTDATA("mouseselectionmode_characters.qml") << false;
    QTest::newRow("default") << TESTDATA("mouseselectionmode_default.qml") << false;
}

void tst_qquicktextinput::mouseSelectionMode()
{
    QFETCH(QString, qmlfile);
    QFETCH(bool, selectWords);

    QString text = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    QQuickView canvas(QUrl::fromLocalFile(qmlfile));

    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());

    QVERIFY(canvas.rootObject() != 0);
    QQuickTextInput *textInputObject = qobject_cast<QQuickTextInput *>(canvas.rootObject());
    QVERIFY(textInputObject != 0);

    // press-and-drag-and-release from x1 to x2
    int x1 = 10;
    int x2 = 70;
    int y = textInputObject->height()/2;
    QTest::mousePress(&canvas, Qt::LeftButton, 0, QPoint(x1,y));
    QTest::mouseMove(&canvas, QPoint(x2,y)); // doesn't work
    QTest::mouseRelease(&canvas, Qt::LeftButton, 0, QPoint(x2,y));
    QTest::qWait(300);
    if (selectWords) {
        QTRY_COMPARE(textInputObject->selectedText(), text);
    } else {
        QTRY_VERIFY(textInputObject->selectedText().length() > 3);
        QVERIFY(textInputObject->selectedText() != text);
    }
}

void tst_qquicktextinput::horizontalAlignment_data()
{
    QTest::addColumn<int>("hAlign");
    QTest::addColumn<QString>("expectfile");

    QTest::newRow("L") << int(Qt::AlignLeft) << "halign_left";
    QTest::newRow("R") << int(Qt::AlignRight) << "halign_right";
    QTest::newRow("C") << int(Qt::AlignHCenter) << "halign_center";
}

void tst_qquicktextinput::horizontalAlignment()
{
    QSKIP("Image comparison of text is almost guaranteed to fail during development");

    QFETCH(int, hAlign);
    QFETCH(QString, expectfile);

    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("horizontalAlignment.qml")));

    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());
    QObject *ob = canvas.rootObject();
    QVERIFY(ob != 0);
    ob->setProperty("horizontalAlignment",hAlign);
    QImage actual = canvas.grabFrameBuffer();

    expectfile = createExpectedFileIfNotFound(expectfile, actual);

    QImage expect(expectfile);

    QCOMPARE(actual,expect);
}

void tst_qquicktextinput::horizontalAlignment_RightToLeft()
{
    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("horizontalAlignment_RightToLeft.qml")));
    QQuickTextInput *textInput = canvas.rootObject()->findChild<QQuickTextInput*>("text");
    QVERIFY(textInput != 0);
    canvas.show();

    const QString rtlText = textInput->text();

    QQuickTextInputPrivate *textInputPrivate = QQuickTextInputPrivate::get(textInput);
    QVERIFY(textInputPrivate != 0);
    QVERIFY(-textInputPrivate->hscroll > canvas.width()/2);

    // implicit alignment should follow the reading direction of RTL text
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QVERIFY(-textInputPrivate->hscroll > canvas.width()/2);

    // explicitly left aligned
    textInput->setHAlign(QQuickTextInput::AlignLeft);
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignLeft);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QVERIFY(-textInputPrivate->hscroll < canvas.width()/2);

    // explicitly right aligned
    textInput->setHAlign(QQuickTextInput::AlignRight);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QVERIFY(-textInputPrivate->hscroll > canvas.width()/2);

    // explicitly center aligned
    textInput->setHAlign(QQuickTextInput::AlignHCenter);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignHCenter);
    QVERIFY(-textInputPrivate->hscroll < canvas.width()/2);
    QVERIFY(-textInputPrivate->hscroll + textInputPrivate->width > canvas.width()/2);

    // reseted alignment should go back to following the text reading direction
    textInput->resetHAlign();
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QVERIFY(-textInputPrivate->hscroll > canvas.width()/2);

    // mirror the text item
    QQuickItemPrivate::get(textInput)->setLayoutMirror(true);

    // mirrored implicit alignment should continue to follow the reading direction of the text
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QVERIFY(-textInputPrivate->hscroll > canvas.width()/2);

    // explicitly right aligned behaves as left aligned
    textInput->setHAlign(QQuickTextInput::AlignRight);
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QCOMPARE(textInput->effectiveHAlign(), QQuickTextInput::AlignLeft);
    QVERIFY(-textInputPrivate->hscroll < canvas.width()/2);

    // mirrored explicitly left aligned behaves as right aligned
    textInput->setHAlign(QQuickTextInput::AlignLeft);
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignLeft);
    QCOMPARE(textInput->effectiveHAlign(), QQuickTextInput::AlignRight);
    QVERIFY(-textInputPrivate->hscroll > canvas.width()/2);

    // disable mirroring
    QQuickItemPrivate::get(textInput)->setLayoutMirror(false);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    textInput->resetHAlign();

    // English text should be implicitly left aligned
    textInput->setText("Hello world!");
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignLeft);
    QVERIFY(-textInputPrivate->hscroll < canvas.width()/2);

    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());

    // If there is no commited text, the preedit text should determine the alignment.
    textInput->setText(QString());
    { QInputMethodEvent ev(rtlText, QList<QInputMethodEvent::Attribute>()); QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &ev); }
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    { QInputMethodEvent ev("Hello world!", QList<QInputMethodEvent::Attribute>()); QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &ev); }
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignLeft);

    // Clear pre-edit text.  TextInput should maybe do this itself on setText, but that may be
    // redundant as an actual input method may take care of it.
    { QInputMethodEvent ev; QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &ev); }

#ifdef Q_OS_MAC
    // empty text with implicit alignment follows the system locale-based
    // keyboard input direction from QGuiApplication::keyboardInputDirection
    QEXPECT_FAIL("", "QTBUG-18040", Abort);
#endif
    textInput->setText("");
    QCOMPARE(textInput->hAlign(), QGuiApplication::keyboardInputDirection() == Qt::LeftToRight ?
                                  QQuickTextInput::AlignLeft : QQuickTextInput::AlignRight);
    if (QGuiApplication::keyboardInputDirection() == Qt::LeftToRight)
        QVERIFY(-textInputPrivate->hscroll < canvas.width()/2);
    else
        QVERIFY(-textInputPrivate->hscroll > canvas.width()/2);
    textInput->setHAlign(QQuickTextInput::AlignRight);
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QVERIFY(-textInputPrivate->hscroll > canvas.width()/2);


#ifdef Q_OS_MAC
    QEXPECT_FAIL("", "QTBUG-18040", Abort); // alignment of TextInput with no text set to it
#endif
    QString componentStr = "import QtQuick 2.0\nTextInput {}";
    QDeclarativeComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickTextInput *textObject = qobject_cast<QQuickTextInput*>(textComponent.create());
    QCOMPARE(textObject->hAlign(), QGuiApplication::keyboardInputDirection() == Qt::LeftToRight ?
                                  QQuickTextInput::AlignLeft : QQuickTextInput::AlignRight);
    delete textObject;
}

void tst_qquicktextinput::positionAt()
{
    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("positionAt.qml")));
    QVERIFY(canvas.rootObject() != 0);
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);

    QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput *>(canvas.rootObject());
    QVERIFY(textinputObject != 0);

    // Check autoscrolled...
    QFontMetrics fm(textinputObject->font());

    int pos = textinputObject->positionAt(textinputObject->width()/2);
    int textWidth = 0;
    int textLeftWidthBegin = 0;
    int textLeftWidthEnd = 0;
    if (!qmlDisableDistanceField()) {
        QTextLayout layout(textinputObject->text());

        QTextOption option;
        option.setUseDesignMetrics(true);
        layout.setTextOption(option);

        layout.beginLayout();
        QTextLine line = layout.createLine();
        layout.endLayout();

        textLeftWidthBegin = floor(line.cursorToX(pos - 1));
        textLeftWidthEnd = ceil(line.cursorToX(pos + 1));
        textWidth = floor(line.horizontalAdvance());
    } else {
        textWidth = fm.width(textinputObject->text());
        textLeftWidthBegin = fm.width(textinputObject->text().left(pos - 1));
        textLeftWidthEnd = fm.width(textinputObject->text().left(pos + 1));
    }

    QVERIFY(textLeftWidthBegin <= textWidth - textinputObject->width() / 2);
    QVERIFY(textLeftWidthEnd >= textWidth - textinputObject->width() / 2);

    int x = textinputObject->positionToRectangle(pos + 1).x() - 1;
    QCOMPARE(textinputObject->positionAt(x, QQuickTextInput::CursorBetweenCharacters), pos + 1);
    QCOMPARE(textinputObject->positionAt(x, QQuickTextInput::CursorOnCharacter), pos);

    // Check without autoscroll...
    textinputObject->setAutoScroll(false);
    pos = textinputObject->positionAt(textinputObject->width()/2);

    if (!qmlDisableDistanceField()) {
        QTextLayout layout(textinputObject->text());

        QTextOption option;
        option.setUseDesignMetrics(true);
        layout.setTextOption(option);

        layout.beginLayout();
        QTextLine line = layout.createLine();
        layout.endLayout();

        textLeftWidthBegin = floor(line.cursorToX(pos - 1));
        textLeftWidthEnd = ceil(line.cursorToX(pos + 1));
    } else {
        textLeftWidthBegin = fm.width(textinputObject->text().left(pos - 1));
        textLeftWidthEnd = fm.width(textinputObject->text().left(pos + 1));
    }

    QVERIFY(textLeftWidthBegin <= textinputObject->width() / 2);
    QVERIFY(textLeftWidthEnd >= textinputObject->width() / 2);

    x = textinputObject->positionToRectangle(pos + 1).x() - 1;
    QCOMPARE(textinputObject->positionAt(x, QQuickTextInput::CursorBetweenCharacters), pos + 1);
    QCOMPARE(textinputObject->positionAt(x, QQuickTextInput::CursorOnCharacter), pos);

    const qreal x0 = textinputObject->positionToRectangle(pos).x();
    const qreal x1 = textinputObject->positionToRectangle(pos + 1).x();

    QString preeditText = textinputObject->text().mid(0, pos);
    textinputObject->setText(textinputObject->text().mid(pos));
    textinputObject->setCursorPosition(0);

    QInputMethodEvent inputEvent(preeditText, QList<QInputMethodEvent::Attribute>());
    QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &inputEvent);

    // Check all points within the preedit text return the same position.
    QCOMPARE(textinputObject->positionAt(0), 0);
    QCOMPARE(textinputObject->positionAt(x0 / 2), 0);
    QCOMPARE(textinputObject->positionAt(x0), 0);

    // Verify positioning returns to normal after the preedit text.
    QCOMPARE(textinputObject->positionAt(x1), 1);
    QCOMPARE(textinputObject->positionToRectangle(1).x(), x1);
}

void tst_qquicktextinput::maxLength()
{
    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("maxLength.qml")));
    QVERIFY(canvas.rootObject() != 0);
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);

    QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput *>(canvas.rootObject());
    QVERIFY(textinputObject != 0);
    QVERIFY(textinputObject->text().isEmpty());
    QVERIFY(textinputObject->maxLength() == 10);
    foreach (const QString &str, standard) {
        QVERIFY(textinputObject->text().length() <= 10);
        textinputObject->setText(str);
        QVERIFY(textinputObject->text().length() <= 10);
    }

    textinputObject->setText("");
    QTRY_VERIFY(textinputObject->hasActiveFocus() == true);
    for (int i=0; i<20; i++) {
        QTRY_COMPARE(textinputObject->text().length(), qMin(i,10));
        //simulateKey(&canvas, Qt::Key_A);
        QTest::keyPress(&canvas, Qt::Key_A);
        QTest::keyRelease(&canvas, Qt::Key_A, Qt::NoModifier ,10);
        QTest::qWait(50);
    }
}

void tst_qquicktextinput::masks()
{
    //Not a comprehensive test of the possible masks, that's done elsewhere (QLineEdit)
    //QString componentStr = "import QtQuick 2.0\nTextInput {  inputMask: 'HHHHhhhh'; }";
    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("masks.qml")));
    canvas.show();
    canvas.requestActivateWindow();
    QVERIFY(canvas.rootObject() != 0);
    QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput *>(canvas.rootObject());
    QVERIFY(textinputObject != 0);
    QTRY_VERIFY(textinputObject->hasActiveFocus() == true);
    QVERIFY(textinputObject->text().length() == 0);
    QCOMPARE(textinputObject->inputMask(), QString("HHHHhhhh; "));
    for (int i=0; i<10; i++) {
        QTRY_COMPARE(qMin(i,8), textinputObject->text().length());
        QCOMPARE(i>=4, textinputObject->hasAcceptableInput());
        //simulateKey(&canvas, Qt::Key_A);
        QTest::keyPress(&canvas, Qt::Key_A);
        QTest::keyRelease(&canvas, Qt::Key_A, Qt::NoModifier ,10);
        QTest::qWait(50);
    }
}

void tst_qquicktextinput::validators()
{
    // Note that this test assumes that the validators are working properly
    // so you may need to run their tests first. All validators are checked
    // here to ensure that their exposure to QML is working.

    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("validators.qml")));
    canvas.show();
    canvas.requestActivateWindow();

    QVERIFY(canvas.rootObject() != 0);

    QQuickTextInput *intInput = qobject_cast<QQuickTextInput *>(qvariant_cast<QObject *>(canvas.rootObject()->property("intInput")));
    QVERIFY(intInput);
    intInput->setFocus(true);
    QTRY_VERIFY(intInput->hasActiveFocus());
    QTest::keyPress(&canvas, Qt::Key_1);
    QTest::keyRelease(&canvas, Qt::Key_1, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(intInput->text(), QLatin1String("1"));
    QCOMPARE(intInput->hasAcceptableInput(), false);
    QTest::keyPress(&canvas, Qt::Key_2);
    QTest::keyRelease(&canvas, Qt::Key_2, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(intInput->text(), QLatin1String("1"));
    QCOMPARE(intInput->hasAcceptableInput(), false);
    QTest::keyPress(&canvas, Qt::Key_1);
    QTest::keyRelease(&canvas, Qt::Key_1, Qt::NoModifier ,10);
    QTest::qWait(50);
    QCOMPARE(intInput->text(), QLatin1String("11"));
    QCOMPARE(intInput->hasAcceptableInput(), true);
    QTest::keyPress(&canvas, Qt::Key_0);
    QTest::keyRelease(&canvas, Qt::Key_0, Qt::NoModifier ,10);
    QTest::qWait(50);
    QCOMPARE(intInput->text(), QLatin1String("11"));
    QCOMPARE(intInput->hasAcceptableInput(), true);

    QQuickTextInput *dblInput = qobject_cast<QQuickTextInput *>(qvariant_cast<QObject *>(canvas.rootObject()->property("dblInput")));
    QTRY_VERIFY(dblInput);
    dblInput->setFocus(true);
    QVERIFY(dblInput->hasActiveFocus() == true);
    QTest::keyPress(&canvas, Qt::Key_1);
    QTest::keyRelease(&canvas, Qt::Key_1, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(dblInput->text(), QLatin1String("1"));
    QCOMPARE(dblInput->hasAcceptableInput(), false);
    QTest::keyPress(&canvas, Qt::Key_2);
    QTest::keyRelease(&canvas, Qt::Key_2, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(dblInput->text(), QLatin1String("12"));
    QCOMPARE(dblInput->hasAcceptableInput(), true);
    QTest::keyPress(&canvas, Qt::Key_Period);
    QTest::keyRelease(&canvas, Qt::Key_Period, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(dblInput->text(), QLatin1String("12."));
    QCOMPARE(dblInput->hasAcceptableInput(), true);
    QTest::keyPress(&canvas, Qt::Key_1);
    QTest::keyRelease(&canvas, Qt::Key_1, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(dblInput->text(), QLatin1String("12.1"));
    QCOMPARE(dblInput->hasAcceptableInput(), true);
    QTest::keyPress(&canvas, Qt::Key_1);
    QTest::keyRelease(&canvas, Qt::Key_1, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(dblInput->text(), QLatin1String("12.11"));
    QCOMPARE(dblInput->hasAcceptableInput(), true);
    QTest::keyPress(&canvas, Qt::Key_1);
    QTest::keyRelease(&canvas, Qt::Key_1, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(dblInput->text(), QLatin1String("12.11"));
    QCOMPARE(dblInput->hasAcceptableInput(), true);

    QQuickTextInput *strInput = qobject_cast<QQuickTextInput *>(qvariant_cast<QObject *>(canvas.rootObject()->property("strInput")));
    QTRY_VERIFY(strInput);
    strInput->setFocus(true);
    QVERIFY(strInput->hasActiveFocus() == true);
    QTest::keyPress(&canvas, Qt::Key_1);
    QTest::keyRelease(&canvas, Qt::Key_1, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(strInput->text(), QLatin1String(""));
    QCOMPARE(strInput->hasAcceptableInput(), false);
    QTest::keyPress(&canvas, Qt::Key_A);
    QTest::keyRelease(&canvas, Qt::Key_A, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(strInput->text(), QLatin1String("a"));
    QCOMPARE(strInput->hasAcceptableInput(), false);
    QTest::keyPress(&canvas, Qt::Key_A);
    QTest::keyRelease(&canvas, Qt::Key_A, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(strInput->text(), QLatin1String("aa"));
    QCOMPARE(strInput->hasAcceptableInput(), true);
    QTest::keyPress(&canvas, Qt::Key_A);
    QTest::keyRelease(&canvas, Qt::Key_A, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(strInput->text(), QLatin1String("aaa"));
    QCOMPARE(strInput->hasAcceptableInput(), true);
    QTest::keyPress(&canvas, Qt::Key_A);
    QTest::keyRelease(&canvas, Qt::Key_A, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(strInput->text(), QLatin1String("aaaa"));
    QCOMPARE(strInput->hasAcceptableInput(), true);
    QTest::keyPress(&canvas, Qt::Key_A);
    QTest::keyRelease(&canvas, Qt::Key_A, Qt::NoModifier ,10);
    QTest::qWait(50);
    QTRY_COMPARE(strInput->text(), QLatin1String("aaaa"));
    QCOMPARE(strInput->hasAcceptableInput(), true);
}

void tst_qquicktextinput::inputMethods()
{
    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("inputmethods.qml")));
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);

    // test input method hints
    QVERIFY(canvas.rootObject() != 0);
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(canvas.rootObject());
    QVERIFY(input != 0);
    QVERIFY(input->inputMethodHints() & Qt::ImhNoPredictiveText);
    input->setInputMethodHints(Qt::ImhUppercaseOnly);
    QVERIFY(input->inputMethodHints() & Qt::ImhUppercaseOnly);

    input->setFocus(true);
    QVERIFY(input->hasActiveFocus() == true);
    // test that input method event is committed
    QInputMethodEvent event;
    event.setCommitString( "My ", -12, 0);
    QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &event);
    QCOMPARE(input->text(), QString("My Hello world!"));

    input->setCursorPosition(2);
    event.setCommitString("Your", -2, 2);
    QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &event);
    QCOMPARE(input->text(), QString("Your Hello world!"));
    QCOMPARE(input->cursorPosition(), 4);

    input->setCursorPosition(7);
    event.setCommitString("Goodbye", -2, 5);
    QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &event);
    QCOMPARE(input->text(), QString("Your Goodbye world!"));
    QCOMPARE(input->cursorPosition(), 12);

    input->setCursorPosition(8);
    event.setCommitString("Our", -8, 4);
    QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &event);
    QCOMPARE(input->text(), QString("Our Goodbye world!"));
    QCOMPARE(input->cursorPosition(), 7);
}

/*
TextInput element should only handle left/right keys until the cursor reaches
the extent of the text, then they should ignore the keys.

*/
void tst_qquicktextinput::navigation()
{
    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("navigation.qml")));
    canvas.show();
    canvas.requestActivateWindow();

    QVERIFY(canvas.rootObject() != 0);

    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(qvariant_cast<QObject *>(canvas.rootObject()->property("myInput")));

    QVERIFY(input != 0);
    input->setCursorPosition(0);
    QTRY_VERIFY(input->hasActiveFocus() == true);
    simulateKey(&canvas, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus() == false);
    simulateKey(&canvas, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus() == true);
    //QT-2944: If text is selected, ensure we deselect upon cursor motion
    input->setCursorPosition(input->text().length());
    input->select(0,input->text().length());
    QVERIFY(input->selectionStart() != input->selectionEnd());
    simulateKey(&canvas, Qt::Key_Right);
    QVERIFY(input->selectionStart() == input->selectionEnd());
    QVERIFY(input->selectionStart() == input->text().length());
    QVERIFY(input->hasActiveFocus() == true);
    simulateKey(&canvas, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus() == false);
    simulateKey(&canvas, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus() == true);

    // Up and Down should NOT do Home/End, even on Mac OS X (QTBUG-10438).
    input->setCursorPosition(2);
    QCOMPARE(input->cursorPosition(),2);
    simulateKey(&canvas, Qt::Key_Up);
    QCOMPARE(input->cursorPosition(),2);
    simulateKey(&canvas, Qt::Key_Down);
    QCOMPARE(input->cursorPosition(),2);
}

void tst_qquicktextinput::navigation_RTL()
{
    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("navigation.qml")));
    canvas.show();
    canvas.requestActivateWindow();

    QVERIFY(canvas.rootObject() != 0);

    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(qvariant_cast<QObject *>(canvas.rootObject()->property("myInput")));

    QVERIFY(input != 0);
    const quint16 arabic_str[] = { 0x0638, 0x0643, 0x00646, 0x0647, 0x0633, 0x0638, 0x0643, 0x00646, 0x0647, 0x0633, 0x0647};
    input->setText(QString::fromUtf16(arabic_str, 11));

    input->setCursorPosition(0);
    QTRY_VERIFY(input->hasActiveFocus() == true);

    // move off
    simulateKey(&canvas, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus() == false);

    // move back
    simulateKey(&canvas, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus() == true);

    input->setCursorPosition(input->text().length());
    QVERIFY(input->hasActiveFocus() == true);

    // move off
    simulateKey(&canvas, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus() == false);

    // move back
    simulateKey(&canvas, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus() == true);
}

void tst_qquicktextinput::copyAndPaste() {
#ifndef QT_NO_CLIPBOARD

#ifdef Q_OS_MAC
    {
        PasteboardRef pasteboard;
        OSStatus status = PasteboardCreate(0, &pasteboard);
        if (status == noErr)
            CFRelease(pasteboard);
        else
            QSKIP("This machine doesn't support the clipboard");
    }
#endif

    QString componentStr = "import QtQuick 2.0\nTextInput { text: \"Hello world!\" }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    // copy and paste
    QCOMPARE(textInput->text().length(), 12);
    textInput->select(0, textInput->text().length());;
    textInput->copy();
    QCOMPARE(textInput->selectedText(), QString("Hello world!"));
    QCOMPARE(textInput->selectedText().length(), 12);
    textInput->setCursorPosition(0);
    QVERIFY(textInput->canPaste());
    textInput->paste();
    QCOMPARE(textInput->text(), QString("Hello world!Hello world!"));
    QCOMPARE(textInput->text().length(), 24);

    // can paste
    QVERIFY(textInput->canPaste());
    textInput->setReadOnly(true);
    QVERIFY(!textInput->canPaste());
    textInput->setReadOnly(false);
    QVERIFY(textInput->canPaste());

    // select word
    textInput->setCursorPosition(0);
    textInput->selectWord();
    QCOMPARE(textInput->selectedText(), QString("Hello"));

    // select all and cut
    textInput->selectAll();
    textInput->cut();
    QCOMPARE(textInput->text().length(), 0);
    textInput->paste();
    QCOMPARE(textInput->text(), QString("Hello world!Hello world!"));
    QCOMPARE(textInput->text().length(), 24);

    // clear copy buffer
    QClipboard *clipboard = QGuiApplication::clipboard();
    QVERIFY(clipboard);
    clipboard->clear();
    QVERIFY(!textInput->canPaste());

    // test that copy functionality is disabled
    // when echo mode is set to hide text/password mode
    int index = 0;
    while (index < 4) {
        QQuickTextInput::EchoMode echoMode = QQuickTextInput::EchoMode(index);
        textInput->setEchoMode(echoMode);
        textInput->setText("My password");
        textInput->select(0, textInput->text().length());;
        textInput->copy();
        if (echoMode == QQuickTextInput::Normal) {
            QVERIFY(!clipboard->text().isEmpty());
            QCOMPARE(clipboard->text(), QString("My password"));
            clipboard->clear();
        } else {
            QVERIFY(clipboard->text().isEmpty());
        }
        index++;
    }

    delete textInput;
#endif
}

void tst_qquicktextinput::canPasteEmpty() {
#ifndef QT_NO_CLIPBOARD

    QGuiApplication::clipboard()->clear();

    QString componentStr = "import QtQuick 2.0\nTextInput { text: \"Hello world!\" }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    QLineControl lc;
    bool cp = !lc.isReadOnly() && QGuiApplication::clipboard()->text().length() != 0;
    QCOMPARE(textInput->canPaste(), cp);

#endif
}

void tst_qquicktextinput::canPaste() {
#ifndef QT_NO_CLIPBOARD

    QGuiApplication::clipboard()->setText("Some text");

    QString componentStr = "import QtQuick 2.0\nTextInput { text: \"Hello world!\" }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    QLineControl lc;
    bool cp = !lc.isReadOnly() && QGuiApplication::clipboard()->text().length() != 0;
    QCOMPARE(textInput->canPaste(), cp);

#endif
}

void tst_qquicktextinput::passwordCharacter()
{
    QString componentStr = "import QtQuick 2.0\nTextInput { text: \"Hello world!\"; font.family: \"Helvetica\"; echoMode: TextInput.Password }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    textInput->setPasswordCharacter("X");
    qreal implicitWidth = textInput->implicitWidth();
    textInput->setPasswordCharacter(".");

    // QTBUG-12383 content is updated and redrawn
    QVERIFY(textInput->implicitWidth() < implicitWidth);

    delete textInput;
}

void tst_qquicktextinput::cursorDelegate()
{
    QQuickView view(QUrl::fromLocalFile(TESTDATA("cursorTest.qml")));
    view.show();
    view.requestActivateWindow();
    QQuickTextInput *textInputObject = view.rootObject()->findChild<QQuickTextInput*>("textInputObject");
    QVERIFY(textInputObject != 0);
    QVERIFY(textInputObject->findChild<QQuickItem*>("cursorInstance"));
    //Test Delegate gets created
    textInputObject->setFocus(true);
    QQuickItem* delegateObject = textInputObject->findChild<QQuickItem*>("cursorInstance");
    QVERIFY(delegateObject);
    QCOMPARE(delegateObject->property("localProperty").toString(), QString("Hello"));
    //Test Delegate gets moved
    for (int i=0; i<= textInputObject->text().length(); i++) {
        textInputObject->setCursorPosition(i);
        QCOMPARE(textInputObject->cursorRectangle().x(), qRound(delegateObject->x()));
        QCOMPARE(textInputObject->cursorRectangle().y(), qRound(delegateObject->y()));
    }
    textInputObject->setCursorPosition(0);
    QCOMPARE(textInputObject->cursorRectangle().x(), qRound(delegateObject->x()));
    QCOMPARE(textInputObject->cursorRectangle().y(), qRound(delegateObject->y()));
    //Test Delegate gets deleted
    textInputObject->setCursorDelegate(0);
    QVERIFY(!textInputObject->findChild<QQuickItem*>("cursorInstance"));
}

void tst_qquicktextinput::cursorVisible()
{
    QQuickView view(QUrl::fromLocalFile(TESTDATA("cursorVisible.qml")));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());

    QQuickTextInput input;
    QSignalSpy spy(&input, SIGNAL(cursorVisibleChanged(bool)));

    QCOMPARE(input.isCursorVisible(), false);

    input.setCursorVisible(true);
    QCOMPARE(input.isCursorVisible(), true);
    QCOMPARE(spy.count(), 1);

    input.setCursorVisible(false);
    QCOMPARE(input.isCursorVisible(), false);
    QCOMPARE(spy.count(), 2);

    input.setFocus(true);
    QCOMPARE(input.isCursorVisible(), false);
    QCOMPARE(spy.count(), 2);

    input.setParentItem(view.rootObject());
    QCOMPARE(input.isCursorVisible(), true);
    QCOMPARE(spy.count(), 3);

    input.setFocus(false);
    QCOMPARE(input.isCursorVisible(), false);
    QCOMPARE(spy.count(), 4);

    input.setFocus(true);
    QCOMPARE(input.isCursorVisible(), true);
    QCOMPARE(spy.count(), 5);

    QQuickView alternateView;
    alternateView.show();
    alternateView.requestActivateWindow();
    QTest::qWaitForWindowShown(&alternateView);

    QCOMPARE(input.isCursorVisible(), false);
    QCOMPARE(spy.count(), 6);

    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QCOMPARE(input.isCursorVisible(), true);
    QCOMPARE(spy.count(), 7);
}

void tst_qquicktextinput::cursorRectangle()
{
    QSKIP("QTBUG-21689");

    QString text = "Hello World!";

    QQuickTextInput input;
    input.setText(text);
    QFontMetricsF fm(input.font());
    input.setWidth(fm.width(text.mid(0, 5)));

    QRect r;

    // some tolerance for different fonts.
#ifdef Q_OS_LINUX
    const int error = 2;
#else
    const int error = 5;
#endif


    for (int i = 0; i <= 5; ++i) {
        input.setCursorPosition(i);
        r = input.cursorRectangle();
        int textWidth = fm.width(text.mid(0, i));

        QVERIFY(r.left() < textWidth + error);
        QVERIFY(r.right() > textWidth - error);
        QCOMPARE(input.inputMethodQuery(Qt::ImCursorRectangle).toRect(), r);
    }

    // Check the cursor rectangle remains within the input bounding rect when auto scrolling.
    QVERIFY(r.left() < input.boundingRect().width());
    QVERIFY(r.right() >= input.width() - error);

    for (int i = 6; i < text.length(); ++i) {
        input.setCursorPosition(i);
        QCOMPARE(r, input.cursorRectangle());
        QCOMPARE(input.inputMethodQuery(Qt::ImCursorRectangle).toRect(), r);
    }

    for (int i = text.length() - 2; i >= 0; --i) {
        input.setCursorPosition(i);
        r = input.cursorRectangle();
        QVERIFY(r.right() >= 0);
        QCOMPARE(input.inputMethodQuery(Qt::ImCursorRectangle).toRect(), r);
    }

    input.setText("Hi!");
    input.setHAlign(QQuickTextInput::AlignRight);
    r = input.cursorRectangle();
    QVERIFY(r.left() < input.boundingRect().width());
    QVERIFY(r.right() >= input.width() - error);
}

void tst_qquicktextinput::readOnly()
{
    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("readOnly.qml")));
    canvas.show();
    canvas.requestActivateWindow();

    QVERIFY(canvas.rootObject() != 0);

    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(qvariant_cast<QObject *>(canvas.rootObject()->property("myInput")));

    QVERIFY(input != 0);
    QTRY_VERIFY(input->hasActiveFocus() == true);
    QVERIFY(input->isReadOnly() == true);
    QString initial = input->text();
    for (int k=Qt::Key_0; k<=Qt::Key_Z; k++)
        simulateKey(&canvas, k);
    simulateKey(&canvas, Qt::Key_Return);
    simulateKey(&canvas, Qt::Key_Space);
    simulateKey(&canvas, Qt::Key_Escape);
    QCOMPARE(input->text(), initial);

    input->setCursorPosition(3);
    input->setReadOnly(false);
    QCOMPARE(input->isReadOnly(), false);
    QCOMPARE(input->cursorPosition(), input->text().length());
}

void tst_qquicktextinput::echoMode()
{
    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("echoMode.qml")));
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());

    QVERIFY(canvas.rootObject() != 0);

    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(qvariant_cast<QObject *>(canvas.rootObject()->property("myInput")));

    QVERIFY(input != 0);
    QTRY_VERIFY(input->hasActiveFocus() == true);
    QString initial = input->text();
    Qt::InputMethodHints ref;
    QCOMPARE(initial, QLatin1String("ABCDefgh"));
    QCOMPARE(input->echoMode(), QQuickTextInput::Normal);
    QCOMPARE(input->displayText(), input->text());
    //Normal
    ref &= ~Qt::ImhHiddenText;
    ref &= ~(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    QCOMPARE(input->inputMethodHints(), ref);
    input->setEchoMode(QQuickTextInput::NoEcho);
    QCOMPARE(input->text(), initial);
    QCOMPARE(input->displayText(), QLatin1String(""));
    QCOMPARE(input->passwordCharacter(), QLatin1String("*"));
    //NoEcho
    ref |= Qt::ImhHiddenText;
    ref |= (Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    QCOMPARE(input->inputMethodHints(), ref);
    input->setEchoMode(QQuickTextInput::Password);
    //Password
    ref |= Qt::ImhHiddenText;
    ref |= (Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    QCOMPARE(input->text(), initial);
    QCOMPARE(input->displayText(), QLatin1String("********"));
    QCOMPARE(input->inputMethodHints(), ref);
    input->setPasswordCharacter(QChar('Q'));
    QCOMPARE(input->passwordCharacter(), QLatin1String("Q"));
    QCOMPARE(input->text(), initial);
    QCOMPARE(input->displayText(), QLatin1String("QQQQQQQQ"));
    input->setEchoMode(QQuickTextInput::PasswordEchoOnEdit);
    //PasswordEchoOnEdit
    ref &= ~Qt::ImhHiddenText;
    ref |= (Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText);
    QCOMPARE(input->inputMethodHints(), ref);
    QCOMPARE(input->text(), initial);
    QCOMPARE(input->displayText(), QLatin1String("QQQQQQQQ"));
    QCOMPARE(input->inputMethodQuery(Qt::ImSurroundingText).toString(), QLatin1String("QQQQQQQQ"));
    QTest::keyPress(&canvas, Qt::Key_A);//Clearing previous entry is part of PasswordEchoOnEdit
    QTest::keyRelease(&canvas, Qt::Key_A, Qt::NoModifier ,10);
    QCOMPARE(input->text(), QLatin1String("a"));
    QCOMPARE(input->displayText(), QLatin1String("a"));
    QCOMPARE(input->inputMethodQuery(Qt::ImSurroundingText).toString(), QLatin1String("a"));
    input->setFocus(false);
    QVERIFY(input->hasActiveFocus() == false);
    QCOMPARE(input->displayText(), QLatin1String("Q"));
    QCOMPARE(input->inputMethodQuery(Qt::ImSurroundingText).toString(), QLatin1String("Q"));
    input->setFocus(true);
    QVERIFY(input->hasActiveFocus());
    QInputMethodEvent inputEvent;
    inputEvent.setCommitString(initial);
    QGuiApplication::sendEvent(input, &inputEvent);
    QCOMPARE(input->text(), initial);
    QCOMPARE(input->displayText(), initial);
    QCOMPARE(input->inputMethodQuery(Qt::ImSurroundingText).toString(), initial);
}

#ifdef QT_GUI_PASSWORD_ECHO_DELAY
void tst_qdeclarativetextinput::passwordEchoDelay()
{
    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("echoMode.qml")));
    canvas.show();
    canvas.setFocus();
    QGuiApplication::setActiveWindow(&canvas);
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());

    QVERIFY(canvas.rootObject() != 0);

    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(qvariant_cast<QObject *>(canvas.rootObject()->property("myInput")));

    QChar fillChar = QLatin1Char('*');

    input->setEchoMode(QDeclarativeTextInput::Password);
    QCOMPARE(input->displayText(), QString(8, fillChar));
    input->setText(QString());
    QCOMPARE(input->displayText(), QString());

    QTest::keyPress(&canvas, '0');
    QTest::keyPress(&canvas, '1');
    QTest::keyPress(&canvas, '2');
    QCOMPARE(input->displayText(), QString(2, fillChar) + QLatin1Char('2'));
    QTest::keyPress(&canvas, '3');
    QTest::keyPress(&canvas, '4');
    QCOMPARE(input->displayText(), QString(4, fillChar) + QLatin1Char('4'));
    QTest::keyPress(&canvas, Qt::Key_Backspace);
    QCOMPARE(input->displayText(), QString(4, fillChar));
    QTest::keyPress(&canvas, '4');
    QCOMPARE(input->displayText(), QString(4, fillChar) + QLatin1Char('4'));
    QTest::qWait(QT_GUI_PASSWORD_ECHO_DELAY);
    QTRY_COMPARE(input->displayText(), QString(5, fillChar));
    QTest::keyPress(&canvas, '5');
    QCOMPARE(input->displayText(), QString(5, fillChar) + QLatin1Char('5'));
    input->setFocus(false);
    QVERIFY(!input->hasFocus());
    QCOMPARE(input->displayText(), QString(6, fillChar));
    input->setFocus(true);
    QTRY_VERIFY(input->hasFocus());
    QCOMPARE(input->displayText(), QString(6, fillChar));
    QTest::keyPress(&canvas, '6');
    QCOMPARE(input->displayText(), QString(6, fillChar) + QLatin1Char('6'));

    QInputMethodEvent ev;
    ev.setCommitString(QLatin1String("7"));
    QGuiApplication::sendEvent(&canvas, &ev);
    QCOMPARE(input->displayText(), QString(7, fillChar) + QLatin1Char('7'));
}
#endif


void tst_qquicktextinput::simulateKey(QQuickView *view, int key)
{
    QKeyEvent press(QKeyEvent::KeyPress, key, 0);
    QKeyEvent release(QKeyEvent::KeyRelease, key, 0);

    QGuiApplication::sendEvent(view, &press);
    QGuiApplication::sendEvent(view, &release);
}

#ifndef QTBUG_21691
class MyInputContext : public QInputContext
{
public:
    MyInputContext() : updateReceived(false), eventType(QEvent::None) {}
    ~MyInputContext() {}

    QString identifierName() { return QString(); }
    QString language() { return QString(); }

    void reset() {}

    bool isComposing() const { return false; }

    void update() { updateReceived = true; }

    void mouseHandler(int x, QMouseEvent *event)
    {
        cursor = x;
        eventType = event->type();
        eventPosition = event->pos();
        eventGlobalPosition = event->globalPos();
        eventButton = event->button();
        eventButtons = event->buttons();
        eventModifiers = event->modifiers();
    }

    void sendPreeditText(const QString &text, int cursor)
    {
        QList<QInputMethodEvent::Attribute> attributes;
        attributes.append(QInputMethodEvent::Attribute(
                QInputMethodEvent::Cursor, cursor, text.length(), QVariant()));

        QInputMethodEvent event(text, attributes);
        sendEvent(event);
    }

    bool updateReceived;
    int cursor;
    QEvent::Type eventType;
    QPoint eventPosition;
    QPoint eventGlobalPosition;
    Qt::MouseButton eventButton;
    Qt::MouseButtons eventButtons;
    Qt::KeyboardModifiers eventModifiers;
};
#endif

void tst_qquicktextinput::openInputPanel()
{
    QQuickView view(QUrl::fromLocalFile(TESTDATA("openInputPanel.qml")));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());

    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(view.rootObject());
    QVERIFY(input);

    // check default values
    QVERIFY(input->focusOnPress());
    QVERIFY(!input->hasActiveFocus());
    qDebug() << &input << qApp->inputPanel()->inputItem();
    QCOMPARE(qApp->inputPanel()->inputItem(), static_cast<QObject*>(0));
    QEXPECT_FAIL("", "QTBUG-21946", Abort);
    QCOMPARE(qApp->inputPanel()->visible(), false);

    // input panel should open on focus
    QPoint centerPoint(view.width()/2, view.height()/2);
    Qt::KeyboardModifiers noModifiers = 0;
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QVERIFY(input->hasActiveFocus());
    QCOMPARE(qApp->inputPanel()->inputItem(), input);
    QCOMPARE(qApp->inputPanel()->visible(), true);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);

    // input panel should be re-opened when pressing already focused TextInput
    qApp->inputPanel()->hide();
    QCOMPARE(qApp->inputPanel()->visible(), false);
    QVERIFY(input->hasActiveFocus());
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QCOMPARE(qApp->inputPanel()->visible(), true);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);

    // input panel should stay visible if focus is lost to another text inputor
    QSignalSpy inputPanelVisibilitySpy(qApp->inputPanel(), SIGNAL(visibleChanged()));
    QQuickTextInput anotherInput;
    anotherInput.setParentItem(view.rootObject());
    anotherInput.setFocus(true);
    QCOMPARE(qApp->inputPanel()->visible(), true);
    QCOMPARE(qApp->inputPanel()->inputItem(), qobject_cast<QObject*>(&anotherInput));
    QCOMPARE(inputPanelVisibilitySpy.count(), 0);

    anotherInput.setFocus(false);
    QCOMPARE(qApp->inputPanel()->inputItem(), static_cast<QObject*>(0));
    QCOMPARE(view.activeFocusItem(), view.rootItem());
    anotherInput.setFocus(true);

    // input item should be null if focus is lost to an item that doesn't accept inputs
    QQuickItem item;
    item.setParentItem(view.rootObject());
    item.setFocus(true);
    QCOMPARE(qApp->inputPanel()->inputItem(), static_cast<QObject*>(0));
    QCOMPARE(view.activeFocusItem(), &item);

    qApp->inputPanel()->hide();

    // input panel should not be opened if TextInput is read only
    input->setReadOnly(true);
    input->setFocus(true);
    QCOMPARE(qApp->inputPanel()->visible(), false);
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QCOMPARE(qApp->inputPanel()->visible(), false);

    // input panel should not be opened if focusOnPress is set to false
    input->setFocusOnPress(false);
    input->setFocus(false);
    input->setFocus(true);
    QCOMPARE(qApp->inputPanel()->visible(), false);
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);
    QCOMPARE(qApp->inputPanel()->visible(), false);

    // input panel should open when openSoftwareInputPanel is called
    input->openSoftwareInputPanel();
    QCOMPARE(qApp->inputPanel()->visible(), true);

    // input panel should close when closeSoftwareInputPanel is called
    input->closeSoftwareInputPanel();
    QCOMPARE(qApp->inputPanel()->visible(), false);
}

class MyTextInput : public QQuickTextInput
{
public:
    MyTextInput(QQuickItem *parent = 0) : QQuickTextInput(parent)
    {
        nbPaint = 0;
    }
    virtual QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data)
    {
       nbPaint++;
       return QQuickTextInput::updatePaintNode(node, data);
    }
    int nbPaint;
};

void tst_qquicktextinput::setHAlignClearCache()
{
    QQuickView view;
    MyTextInput input;
    input.setText("Hello world");
    input.setParentItem(view.rootItem());
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(input.nbPaint, 1);
    input.setHAlign(QQuickTextInput::AlignRight);
    //Changing the alignment should trigger a repaint
    QTRY_COMPARE(input.nbPaint, 2);
}

void tst_qquicktextinput::focusOutClearSelection()
{
    QQuickView view;
    QQuickTextInput input;
    QQuickTextInput input2;
    input.setText(QLatin1String("Hello world"));
    input.setFocus(true);
    input2.setParentItem(view.rootItem());
    input.setParentItem(view.rootItem());
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    input.select(2,5);
    //The selection should work
    QTRY_COMPARE(input.selectedText(), QLatin1String("llo"));
    input2.setFocus(true);
    QGuiApplication::processEvents();
    //The input lost the focus selection should be cleared
    QTRY_COMPARE(input.selectedText(), QLatin1String(""));
}

void tst_qquicktextinput::geometrySignals()
{
    QDeclarativeComponent component(&engine, TESTDATA("geometrySignals.qml"));
    QObject *o = component.create();
    QVERIFY(o);
    QCOMPARE(o->property("bindingWidth").toInt(), 400);
    QCOMPARE(o->property("bindingHeight").toInt(), 500);
    delete o;
}

void tst_qquicktextinput::testQtQuick11Attributes()
{
    QFETCH(QString, code);
    QFETCH(QString, warning);
    QFETCH(QString, error);

    QDeclarativeEngine engine;
    QObject *obj;

    QDeclarativeComponent valid(&engine);
    valid.setData("import QtQuick 2.0; TextInput { " + code.toUtf8() + " }", QUrl(""));
    obj = valid.create();
    QVERIFY(obj);
    QVERIFY(valid.errorString().isEmpty());
    delete obj;

    QDeclarativeComponent invalid(&engine);
    invalid.setData("import QtQuick 1.0; TextInput { " + code.toUtf8() + " }", QUrl(""));
    QTest::ignoreMessage(QtWarningMsg, warning.toUtf8());
    obj = invalid.create();
    QCOMPARE(invalid.errorString(), error);
    delete obj;
}

void tst_qquicktextinput::testQtQuick11Attributes_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<QString>("error");

    QTest::newRow("canPaste") << "property bool foo: canPaste"
        << "<Unknown File>:1: ReferenceError: Can't find variable: canPaste"
        << "";

    QTest::newRow("moveCursorSelection") << "Component.onCompleted: moveCursorSelection(0, TextEdit.SelectCharacters)"
        << "<Unknown File>:1: ReferenceError: Can't find variable: moveCursorSelection"
        << "";

    QTest::newRow("deselect") << "Component.onCompleted: deselect()"
        << "<Unknown File>:1: ReferenceError: Can't find variable: deselect"
        << "";
}

static void sendPreeditText(const QString &text, int cursor)
{
    QInputMethodEvent event(text, QList<QInputMethodEvent::Attribute>()
            << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, cursor, text.length(), QVariant()));
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &event);
}

void tst_qquicktextinput::preeditAutoScroll()
{
    QString preeditText = "califragisiticexpialidocious!";

    QQuickView view(QUrl::fromLocalFile(TESTDATA("preeditAutoScroll.qml")));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(view.rootObject());
    QVERIFY(input);
    QVERIFY(input->hasActiveFocus());

    input->setWidth(input->implicitWidth());

    QSignalSpy cursorRectangleSpy(input, SIGNAL(cursorRectangleChanged()));
    int cursorRectangleChanges = 0;

    // test the text is scrolled so the preedit is visible.
    sendPreeditText(preeditText.mid(0, 3), 1);
    QVERIFY(input->positionAt(0) != 0);
    QVERIFY(input->cursorRectangle().left() < input->boundingRect().width());
    QCOMPARE(cursorRectangleSpy.count(), ++cursorRectangleChanges);

    // test the text is scrolled back when the preedit is removed.
    QInputMethodEvent imEvent;
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &imEvent);
    QCOMPARE(input->positionAt(0), 0);
    QCOMPARE(input->positionAt(input->width()), 5);
    QCOMPARE(cursorRectangleSpy.count(), ++cursorRectangleChanges);

    QTextLayout layout(preeditText);
    if (!qmlDisableDistanceField()) {
        QTextOption option;
        option.setUseDesignMetrics(true);
        layout.setTextOption(option);
    }
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    // test if the preedit is larger than the text input that the
    // character preceding the cursor is still visible.
    qreal x = input->positionToRectangle(0).x();
    for (int i = 0; i < 3; ++i) {
        sendPreeditText(preeditText, i + 1);
        int width = ceil(line.cursorToX(i, QTextLine::Trailing)) - floor(line.cursorToX(i));
        QVERIFY(input->cursorRectangle().right() >= width - 3);
        QVERIFY(input->positionToRectangle(0).x() < x);
        QCOMPARE(cursorRectangleSpy.count(), ++cursorRectangleChanges);
        x = input->positionToRectangle(0).x();
    }
    for (int i = 1; i >= 0; --i) {
        sendPreeditText(preeditText, i + 1);
        int width = ceil(line.cursorToX(i, QTextLine::Trailing)) - floor(line.cursorToX(i));
        QVERIFY(input->cursorRectangle().right() >= width - 3);
        QVERIFY(input->positionToRectangle(0).x() > x);
        QCOMPARE(cursorRectangleSpy.count(), ++cursorRectangleChanges);
        x = input->positionToRectangle(0).x();
    }

    // Test incrementing the preedit cursor doesn't cause further
    // scrolling when right most text is visible.
    sendPreeditText(preeditText, preeditText.length() - 3);
    QCOMPARE(cursorRectangleSpy.count(), ++cursorRectangleChanges);
    x = input->positionToRectangle(0).x();
    for (int i = 2; i >= 0; --i) {
        sendPreeditText(preeditText, preeditText.length() - i);
        QCOMPARE(input->positionToRectangle(0).x(), x);
        QCOMPARE(cursorRectangleSpy.count(), ++cursorRectangleChanges);
    }
    for (int i = 1; i <  3; ++i) {
        sendPreeditText(preeditText, preeditText.length() - i);
        QCOMPARE(input->positionToRectangle(0).x(), x);
        QCOMPARE(cursorRectangleSpy.count(), ++cursorRectangleChanges);
    }

    // Test disabling auto scroll.
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &imEvent);

    input->setAutoScroll(false);
    sendPreeditText(preeditText.mid(0, 3), 1);
    QCOMPARE(input->positionAt(0), 0);
    QCOMPARE(input->positionAt(input->width()), 5);
}

void tst_qquicktextinput::preeditCursorRectangle()
{
    QString preeditText = "super";

    QQuickView view(QUrl::fromLocalFile(TESTDATA("inputMethodEvent.qml")));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(view.rootObject());
    QVERIFY(input);

    QRect currentRect;

    QInputMethodQueryEvent query(Qt::ImCursorRectangle);
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &query);
    QRect previousRect = query.value(Qt::ImCursorRectangle).toRect();

    // Verify that the micro focus rect is positioned the same for position 0 as
    // it would be if there was no preedit text.
    sendPreeditText(preeditText, 0);
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &query);
    currentRect = query.value(Qt::ImCursorRectangle).toRect();
    QCOMPARE(currentRect, previousRect);

    QSignalSpy inputSpy(input, SIGNAL(cursorRectangleChanged()));
    QSignalSpy panelSpy(qGuiApp->inputPanel(), SIGNAL(cursorRectangleChanged()));

    // Verify that the micro focus rect moves to the left as the cursor position
    // is incremented.
    for (int i = 1; i <= 5; ++i) {
        sendPreeditText(preeditText, i);
        QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &query);
        currentRect = query.value(Qt::ImCursorRectangle).toRect();
        QVERIFY(previousRect.left() < currentRect.left());
        QVERIFY(inputSpy.count() > 0); inputSpy.clear();
        QVERIFY(panelSpy.count() > 0); panelSpy.clear();
        previousRect = currentRect;
    }

    // Verify that if there is no preedit cursor then the micro focus rect is the
    // same as it would be if it were positioned at the end of the preedit text.
    sendPreeditText(preeditText, 0);
    QInputMethodEvent imEvent(preeditText, QList<QInputMethodEvent::Attribute>());
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &imEvent);
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &query);
    currentRect = query.value(Qt::ImCursorRectangle).toRect();
    QCOMPARE(currentRect, previousRect);
    QVERIFY(inputSpy.count() > 0);
    QVERIFY(panelSpy.count() > 0);
}

void tst_qquicktextinput::inputMethodComposing()
{
    QString text = "supercalifragisiticexpialidocious!";

    QQuickView view(QUrl::fromLocalFile(TESTDATA("inputContext.qml")));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(view.rootObject());
    QVERIFY(input);
    QSignalSpy spy(input, SIGNAL(inputMethodComposingChanged()));

    QCOMPARE(input->isInputMethodComposing(), false);
    {
        QInputMethodEvent event(text.mid(3), QList<QInputMethodEvent::Attribute>());
        QGuiApplication::sendEvent(input, &event);
    }
    QCOMPARE(input->isInputMethodComposing(), true);
    QCOMPARE(spy.count(), 1);

    {
        QInputMethodEvent event(text.mid(12), QList<QInputMethodEvent::Attribute>());
        QGuiApplication::sendEvent(input, &event);
    }
    QCOMPARE(spy.count(), 1);

    {
        QInputMethodEvent event;
        QGuiApplication::sendEvent(input, &event);
    }
    QCOMPARE(input->isInputMethodComposing(), false);
    QCOMPARE(spy.count(), 2);
}

void tst_qquicktextinput::cursorRectangleSize()
{
    QQuickView *canvas = new QQuickView(QUrl::fromLocalFile(TESTDATA("positionAt.qml")));
    QVERIFY(canvas->rootObject() != 0);
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);

    QQuickTextInput *textInput = qobject_cast<QQuickTextInput *>(canvas->rootObject());
    QVERIFY(textInput != 0);
    textInput->setFocus(Qt::OtherFocusReason);
    QRectF cursorRect = textInput->positionToRectangle(textInput->cursorPosition());
    QRectF microFocusFromScene = canvas->inputMethodQuery(Qt::ImCursorRectangle).toRectF();
    QInputMethodQueryEvent event(Qt::ImCursorRectangle);
    qApp->sendEvent(qApp->inputPanel()->inputItem(), &event);

    QRectF microFocusFromApp = event.value(Qt::ImCursorRectangle).toRectF();

    QCOMPARE(microFocusFromScene.size(), cursorRect.size());
    QCOMPARE(microFocusFromApp.size(), cursorRect.size());

    delete canvas;
}

void tst_qquicktextinput::tripleClickSelectsAll()
{
    QString qmlfile = TESTDATA("positionAt.qml");
    QQuickView view(QUrl::fromLocalFile(qmlfile));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);

    QTRY_COMPARE(&view, qGuiApp->focusWindow());

    QQuickTextInput* input = qobject_cast<QQuickTextInput*>(view.rootObject());
    QVERIFY(input);

    QLatin1String hello("Hello world!");
    input->setSelectByMouse(true);
    input->setText(hello);

    // Clicking on the same point inside TextInput three times in a row
    // should trigger a triple click, thus selecting all the text.
    QPoint pointInside = input->pos().toPoint() + QPoint(2,2);
    QTest::mouseDClick(&view, Qt::LeftButton, 0, pointInside);
    QTest::mouseClick(&view, Qt::LeftButton, 0, pointInside);
    QGuiApplication::processEvents();
    QCOMPARE(input->selectedText(), hello);

    // Now it simulates user moving the mouse between the second and the third click.
    // In this situation, we don't expect a triple click.
    QPoint pointInsideButFar = QPoint(input->width(),input->height()) - QPoint(2,2);
    QTest::mouseDClick(&view, Qt::LeftButton, 0, pointInside);
    QTest::mouseClick(&view, Qt::LeftButton, 0, pointInsideButFar);
    QGuiApplication::processEvents();
    QVERIFY(input->selectedText().isEmpty());

    // And now we press the third click too late, so no triple click event is triggered.
    QTest::mouseDClick(&view, Qt::LeftButton, 0, pointInside);
    QGuiApplication::processEvents();
    QTest::qWait(QApplication::doubleClickInterval() + 1);
    QTest::mouseClick(&view, Qt::LeftButton, 0, pointInside);
    QGuiApplication::processEvents();
    QVERIFY(input->selectedText().isEmpty());
}

void tst_qquicktextinput::QTBUG_19956_data()
{
    QTest::addColumn<QString>("url");
    QTest::newRow("intvalidator") << "qtbug-19956int.qml";
    QTest::newRow("doublevalidator") << "qtbug-19956double.qml";
}

void tst_qquicktextinput::QTBUG_19956()
{
    QFETCH(QString, url);

    QQuickView canvas(QUrl::fromLocalFile(TESTDATA(url)));
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QVERIFY(canvas.rootObject() != 0);
    QQuickTextInput *input = qobject_cast<QQuickTextInput*>(canvas.rootObject());
    QVERIFY(input);
    input->setFocus(true);
    QVERIFY(input->hasActiveFocus());

    QCOMPARE(canvas.rootObject()->property("topvalue").toInt(), 30);
    QCOMPARE(canvas.rootObject()->property("bottomvalue").toInt(), 10);
    QCOMPARE(canvas.rootObject()->property("text").toString(), QString("20"));
    QVERIFY(canvas.rootObject()->property("acceptableInput").toBool());

    canvas.rootObject()->setProperty("topvalue", 15);
    QCOMPARE(canvas.rootObject()->property("topvalue").toInt(), 15);
    QVERIFY(!canvas.rootObject()->property("acceptableInput").toBool());

    canvas.rootObject()->setProperty("topvalue", 25);
    QCOMPARE(canvas.rootObject()->property("topvalue").toInt(), 25);
    QVERIFY(canvas.rootObject()->property("acceptableInput").toBool());

    canvas.rootObject()->setProperty("bottomvalue", 21);
    QCOMPARE(canvas.rootObject()->property("bottomvalue").toInt(), 21);
    QVERIFY(!canvas.rootObject()->property("acceptableInput").toBool());

    canvas.rootObject()->setProperty("bottomvalue", 10);
    QCOMPARE(canvas.rootObject()->property("bottomvalue").toInt(), 10);
    QVERIFY(canvas.rootObject()->property("acceptableInput").toBool());
}

void tst_qquicktextinput::QTBUG_19956_regexp()
{
    QQuickView canvas(QUrl::fromLocalFile(TESTDATA("qtbug-19956regexp.qml")));
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QVERIFY(canvas.rootObject() != 0);
    QQuickTextInput *input = qobject_cast<QQuickTextInput*>(canvas.rootObject());
    QVERIFY(input);
    input->setFocus(true);
    QVERIFY(input->hasActiveFocus());

    canvas.rootObject()->setProperty("regexvalue", QRegExp("abc"));
    QCOMPARE(canvas.rootObject()->property("regexvalue").toRegExp(), QRegExp("abc"));
    QCOMPARE(canvas.rootObject()->property("text").toString(), QString("abc"));
    QVERIFY(canvas.rootObject()->property("acceptableInput").toBool());

    canvas.rootObject()->setProperty("regexvalue", QRegExp("abcd"));
    QCOMPARE(canvas.rootObject()->property("regexvalue").toRegExp(), QRegExp("abcd"));
    QVERIFY(!canvas.rootObject()->property("acceptableInput").toBool());

    canvas.rootObject()->setProperty("regexvalue", QRegExp("abc"));
    QCOMPARE(canvas.rootObject()->property("regexvalue").toRegExp(), QRegExp("abc"));
    QVERIFY(canvas.rootObject()->property("acceptableInput").toBool());
}

QTEST_MAIN(tst_qquicktextinput)

#include "tst_qquicktextinput.moc"
