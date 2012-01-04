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
#include "../../shared/util.h"
#include <private/qinputpanel_p.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QFile>
#include <QtQuick/qquickview.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>
#include <QInputPanel>
#include <private/qquicktextinput_p.h>
#include <private/qquicktextinput_p_p.h>
#include <QDebug>
#include <QDir>
#include <QStyle>
#include <QtOpenGL/QGLShaderProgram>
#include <math.h>

#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

#include "qplatformdefs.h"
#include "../../shared/platforminputcontext.h"

Q_DECLARE_METATYPE(QQuickTextInput::SelectionMode)
DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)

QString createExpectedFileIfNotFound(const QString& filebasename, const QImage& actual)
{
    // XXX This will be replaced by some clever persistent platform image store.
    QString persistent_dir = QDeclarativeDataTest::instance()->dataDirectory();
    QString arch = "unknown-architecture"; // QTest needs to help with this.

    QString expectfile = persistent_dir + QDir::separator() + filebasename + "-" + arch + ".png";

    if (!QFile::exists(expectfile)) {
        actual.save(expectfile);
        qWarning() << "created" << expectfile;
    }

    return expectfile;
}

template <typename T> static T evaluate(QObject *scope, const QString &expression)
{
    QDeclarativeExpression expr(qmlContext(scope), scope, expression);
    T result = expr.evaluate().value<T>();
    if (expr.hasError())
        qWarning() << expr.error().toString();
    return result;
}

typedef QPair<int, QChar> Key;

class tst_qquicktextinput : public QDeclarativeDataTest

{
    Q_OBJECT
public:
    tst_qquicktextinput();

private slots:
    void cleanup();
    void text();
    void width();
    void font();
    void color();
    void wrap();
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
    void verticalAlignment();

    void boundingRect();

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
    void copyAndPasteKeySequence();
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
    void inputContextMouseHandler();
    void inputMethodComposing();
    void cursorRectangleSize();

    void getText_data();
    void getText();
    void insert_data();
    void insert();
    void remove_data();
    void remove();

    void keySequence_data();
    void keySequence();

    void undo_data();
    void undo();
    void redo_data();
    void redo();
    void undo_keypressevents_data();
    void undo_keypressevents();

    void QTBUG_19956();
    void QTBUG_19956_data();
    void QTBUG_19956_regexp();

    void negativeDimensions();

private:
    void simulateKey(QQuickView *, int key);

    void simulateKeys(QWindow *window, const QList<Key> &keys);
    void simulateKeys(QWindow *window, const QKeySequence &sequence);

    QDeclarativeEngine engine;
    QStringList standard;
    QStringList colorStrings;
};

typedef QList<int> IntList;
Q_DECLARE_METATYPE(IntList)

typedef QList<Key> KeyList;
Q_DECLARE_METATYPE(KeyList)

void tst_qquicktextinput::simulateKeys(QWindow *window, const QList<Key> &keys)
{
    for (int i = 0; i < keys.count(); ++i) {
        const int key = keys.at(i).first;
        const int modifiers = key & Qt::KeyboardModifierMask;
        const QString text = !keys.at(i).second.isNull() ? QString(keys.at(i).second) : QString();

        QKeyEvent press(QEvent::KeyPress, Qt::Key(key), Qt::KeyboardModifiers(modifiers), text);
        QKeyEvent release(QEvent::KeyRelease, Qt::Key(key), Qt::KeyboardModifiers(modifiers), text);

        QGuiApplication::sendEvent(window, &press);
        QGuiApplication::sendEvent(window, &release);
    }
}

void tst_qquicktextinput::simulateKeys(QWindow *window, const QKeySequence &sequence)
{
    for (uint i = 0; i < sequence.count(); ++i) {
        const int key = sequence[i];
        const int modifiers = key & Qt::KeyboardModifierMask;

        QTest::keyClick(window, Qt::Key(key & ~modifiers), Qt::KeyboardModifiers(modifiers));
    }
}

QList<Key> &operator <<(QList<Key> &keys, const QKeySequence &sequence)
{
    for (uint i = 0; i < sequence.count(); ++i)
        keys << Key(sequence[i], QChar());
    return keys;
}

template <int N> QList<Key> &operator <<(QList<Key> &keys, const char (&characters)[N])
{
    for (int i = 0; i < N - 1; ++i) {
        int key = QTest::asciiToKey(characters[i]);
        QChar character = QLatin1Char(characters[i]);
        keys << Key(key, character);
    }
    return keys;
}

QList<Key> &operator <<(QList<Key> &keys, Qt::Key key)
{
    keys << Key(key, QChar());
    return keys;
}

void tst_qquicktextinput::cleanup()
{
    // ensure not even skipped tests with custom input context leave it dangling
    QInputPanelPrivate *inputPanelPrivate = QInputPanelPrivate::get(qApp->inputPanel());
    inputPanelPrivate->testContext = 0;
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
        QCOMPARE(textinputObject->length(), 0);

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
        QCOMPARE(textinputObject->length(), standard.at(i).length());

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
        QString componentStr = "import QtQuick 2.0\nTextInput { text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent textinputComponent(&engine);
        textinputComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput*>(textinputComponent.create());

        QString s = standard.at(i);
        s.replace(QLatin1Char('\n'), QChar::LineSeparator);

        QTextLayout layout(s);
        layout.setFont(textinputObject->font());
        layout.setFlags(Qt::TextExpandTabs | Qt::TextShowMnemonic);
        if (requiresUnhintedMetrics) {
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

        qreal metricWidth = ceil(layout.boundingRect().width());

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

void tst_qquicktextinput::wrap()
{
    int textHeight = 0;
    // for specified width and wrap set true
    {
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nTextInput { text: \"Hello\"; wrapMode: Text.WrapAnywhere; width: 300 }", QUrl::fromLocalFile(""));
        QQuickTextInput *textObject = qobject_cast<QQuickTextInput*>(textComponent.create());
        textHeight = textObject->height();

        QVERIFY(textObject != 0);
        QVERIFY(textObject->wrapMode() == QQuickTextInput::WrapAnywhere);
        QCOMPARE(textObject->width(), 300.);

        delete textObject;
    }

    for (int i = 0; i < standard.count(); i++) {
        QString componentStr = "import QtQuick 2.0\nTextInput { wrapMode: Text.WrapAnywhere; width: 30; text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
        QQuickTextInput *textObject = qobject_cast<QQuickTextInput*>(textComponent.create());

        QVERIFY(textObject != 0);
        QCOMPARE(textObject->width(), 30.);
        QVERIFY(textObject->height() > textHeight);

        int oldHeight = textObject->height();
        textObject->setWidth(100);
        QVERIFY(textObject->height() < oldHeight);

        delete textObject;
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

    // test input method selection
    QSignalSpy selectionSpy(textinputObject, SIGNAL(selectedTextChanged()));
    textinputObject->setFocus(true);
    {
        QList<QInputMethodEvent::Attribute> attributes;
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 12, 5, QVariant());
        QInputMethodEvent event("", attributes);
        QApplication::sendEvent(textinputObject, &event);
    }
    QCOMPARE(selectionSpy.count(), 1);
    QCOMPARE(textinputObject->selectionStart(), 12);
    QCOMPARE(textinputObject->selectionEnd(), 17);

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
    QString qmlfile = testFile("mouseselection_true.qml");

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
    QTest::newRow("SelectWords") << testFile("mouseselectionmode_words.qml") << true;
    QTest::newRow("SelectCharacters") << testFile("mouseselectionmode_characters.qml") << false;
    QTest::newRow("default") << testFile("mouseselectionmode_default.qml") << false;
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

    QQuickView canvas(testFileUrl("horizontalAlignment.qml"));

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
    QQuickView canvas(testFileUrl("horizontalAlignment_RightToLeft.qml"));
    QQuickTextInput *textInput = canvas.rootObject()->findChild<QQuickTextInput*>("text");
    QVERIFY(textInput != 0);
    canvas.show();

    const QString rtlText = textInput->text();

    QQuickTextInputPrivate *textInputPrivate = QQuickTextInputPrivate::get(textInput);
    QVERIFY(textInputPrivate != 0);
    QVERIFY(textInputPrivate->boundingRect.left() > canvas.width()/2);

    // implicit alignment should follow the reading direction of RTL text
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QVERIFY(textInputPrivate->boundingRect.left() - textInputPrivate->hscroll > canvas.width()/2);

    // explicitly left aligned
    textInput->setHAlign(QQuickTextInput::AlignLeft);
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignLeft);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QVERIFY(textInputPrivate->boundingRect.left() - textInputPrivate->hscroll < canvas.width()/2);

    // explicitly right aligned
    textInput->setHAlign(QQuickTextInput::AlignRight);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QVERIFY(textInputPrivate->boundingRect.left() - textInputPrivate->hscroll > canvas.width()/2);

    // explicitly center aligned
    textInput->setHAlign(QQuickTextInput::AlignHCenter);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignHCenter);
    QVERIFY(textInputPrivate->boundingRect.left() - textInputPrivate->hscroll < canvas.width()/2);
    QVERIFY(textInputPrivate->boundingRect.right() - textInputPrivate->hscroll > canvas.width()/2);

    // reseted alignment should go back to following the text reading direction
    textInput->resetHAlign();
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QVERIFY(textInputPrivate->boundingRect.left() - textInputPrivate->hscroll > canvas.width()/2);

    // mirror the text item
    QQuickItemPrivate::get(textInput)->setLayoutMirror(true);

    // mirrored implicit alignment should continue to follow the reading direction of the text
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    QVERIFY(textInputPrivate->boundingRect.left() - textInputPrivate->hscroll > canvas.width()/2);

    // explicitly right aligned behaves as left aligned
    textInput->setHAlign(QQuickTextInput::AlignRight);
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QCOMPARE(textInput->effectiveHAlign(), QQuickTextInput::AlignLeft);
    QVERIFY(textInputPrivate->boundingRect.left() - textInputPrivate->hscroll < canvas.width()/2);

    // mirrored explicitly left aligned behaves as right aligned
    textInput->setHAlign(QQuickTextInput::AlignLeft);
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignLeft);
    QCOMPARE(textInput->effectiveHAlign(), QQuickTextInput::AlignRight);
    QVERIFY(textInputPrivate->boundingRect.left() - textInputPrivate->hscroll > canvas.width()/2);

    // disable mirroring
    QQuickItemPrivate::get(textInput)->setLayoutMirror(false);
    QCOMPARE(textInput->effectiveHAlign(), textInput->hAlign());
    textInput->resetHAlign();

    // English text should be implicitly left aligned
    textInput->setText("Hello world!");
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignLeft);
    QVERIFY(textInputPrivate->boundingRect.left() - textInputPrivate->hscroll < canvas.width()/2);

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

    // empty text with implicit alignment follows the system locale-based
    // keyboard input direction from QGuiApplication::keyboardInputDirection
    textInput->setText("");
    QCOMPARE(textInput->hAlign(), QGuiApplication::keyboardInputDirection() == Qt::LeftToRight ?
                                  QQuickTextInput::AlignLeft : QQuickTextInput::AlignRight);
    if (QGuiApplication::keyboardInputDirection() == Qt::LeftToRight)
        QVERIFY(textInput->boundingRect().left() < canvas.width()/2);
    else
        QVERIFY(textInput->boundingRect().left() > canvas.width()/2);
    textInput->setHAlign(QQuickTextInput::AlignRight);
    QCOMPARE(textInput->hAlign(), QQuickTextInput::AlignRight);
    QVERIFY(textInputPrivate->boundingRect.left() - textInputPrivate->hscroll > canvas.width()/2);

    QString componentStr = "import QtQuick 2.0\nTextInput {}";
    QDeclarativeComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickTextInput *textObject = qobject_cast<QQuickTextInput*>(textComponent.create());
    QCOMPARE(textObject->hAlign(), QGuiApplication::keyboardInputDirection() == Qt::LeftToRight ?
                                  QQuickTextInput::AlignLeft : QQuickTextInput::AlignRight);
    delete textObject;
}

void tst_qquicktextinput::verticalAlignment()
{
    QQuickView canvas(testFileUrl("horizontalAlignment.qml"));
    QQuickTextInput *textInput = canvas.rootObject()->findChild<QQuickTextInput*>("text");
    QVERIFY(textInput != 0);
    canvas.show();

    QQuickTextInputPrivate *textInputPrivate = QQuickTextInputPrivate::get(textInput);
    QVERIFY(textInputPrivate != 0);

    QCOMPARE(textInput->vAlign(), QQuickTextInput::AlignTop);
    QVERIFY(textInputPrivate->boundingRect.bottom() - textInputPrivate->vscroll < canvas.height() / 2);

    // bottom aligned
    textInput->setVAlign(QQuickTextInput::AlignBottom);
    QCOMPARE(textInput->vAlign(), QQuickTextInput::AlignBottom);
    QVERIFY(textInputPrivate->boundingRect.top() - textInputPrivate->vscroll > canvas.height() / 2);

    // explicitly center aligned
    textInput->setVAlign(QQuickTextInput::AlignVCenter);
    QCOMPARE(textInput->vAlign(), QQuickTextInput::AlignVCenter);
    QVERIFY(textInputPrivate->boundingRect.top() - textInputPrivate->vscroll < canvas.height() / 2);
    QVERIFY(textInputPrivate->boundingRect.bottom() - textInputPrivate->vscroll > canvas.height() / 2);
}

void tst_qquicktextinput::boundingRect()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0\n TextInput {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(object.data());
    QVERIFY(input);

    QCOMPARE(input->width() + input->cursorRectangle().width(), input->boundingRect().width());
    QCOMPARE(input->height(), input->boundingRect().height());

    input->setText("Hello World");
    QCOMPARE(input->width() + input->cursorRectangle().width(), input->boundingRect().width());
    QCOMPARE(input->height(), input->boundingRect().height());

    // bounding rect shouldn't exceed the size of the item, expect for the cursor width;
    input->setWidth(input->width() / 2);
    QCOMPARE(input->width() + input->cursorRectangle().width(), input->boundingRect().width());
    QCOMPARE(input->height(), input->boundingRect().height());

    input->setHeight(input->height() * 2);
    QCOMPARE(input->width() + input->cursorRectangle().width(), input->boundingRect().width());
    QCOMPARE(input->height(), input->boundingRect().height());

    QDeclarativeComponent cursorComponent(&engine);
    cursorComponent.setData("import QtQuick 2.0\nRectangle { height: 20; width: 8 }", QUrl());

    input->setCursorDelegate(&cursorComponent);

    // If a cursor delegate is used it's size should determine the excess width.
    QCOMPARE(input->width() + 8, input->boundingRect().width());
    QCOMPARE(input->height(), input->boundingRect().height());
}

void tst_qquicktextinput::positionAt()
{
    QQuickView canvas(testFileUrl("positionAt.qml"));
    QVERIFY(canvas.rootObject() != 0);
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);

    QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput *>(canvas.rootObject());
    QVERIFY(textinputObject != 0);

    // Check autoscrolled...

    int pos = evaluate<int>(textinputObject, QString("positionAt(%1)").arg(textinputObject->width()/2));

    QTextLayout layout(textinputObject->text());
    layout.setFont(textinputObject->font());

    if (!qmlDisableDistanceField()) {
        QTextOption option;
        option.setUseDesignMetrics(true);
        layout.setTextOption(option);
    }
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    int textLeftWidthBegin = floor(line.cursorToX(pos - 1));
    int textLeftWidthEnd = ceil(line.cursorToX(pos + 1));
    int textWidth = floor(line.horizontalAdvance());

    QVERIFY(textLeftWidthBegin <= textWidth - textinputObject->width() / 2);
    QVERIFY(textLeftWidthEnd >= textWidth - textinputObject->width() / 2);

    int x = textinputObject->positionToRectangle(pos + 1).x() - 1;
    QCOMPARE(evaluate<int>(textinputObject, QString("positionAt(%1, 0, TextInput.CursorBetweenCharacters)").arg(x)), pos + 1);
    QCOMPARE(evaluate<int>(textinputObject, QString("positionAt(%1, 0, TextInput.CursorOnCharacter)").arg(x)), pos);

    // Check without autoscroll...
    textinputObject->setAutoScroll(false);
    pos = evaluate<int>(textinputObject, QString("positionAt(%1)").arg(textinputObject->width() / 2));

    textLeftWidthBegin = floor(line.cursorToX(pos - 1));
    textLeftWidthEnd = ceil(line.cursorToX(pos + 1));

    QVERIFY(textLeftWidthBegin <= textinputObject->width() / 2);
    QVERIFY(textLeftWidthEnd >= textinputObject->width() / 2);

    x = textinputObject->positionToRectangle(pos + 1).x() - 1;
    QCOMPARE(evaluate<int>(textinputObject, QString("positionAt(%1, 0, TextInput.CursorBetweenCharacters)").arg(x)), pos + 1);
    QCOMPARE(evaluate<int>(textinputObject, QString("positionAt(%1, 0, TextInput.CursorOnCharacter)").arg(x)), pos);

    const qreal x0 = textinputObject->positionToRectangle(pos).x();
    const qreal x1 = textinputObject->positionToRectangle(pos + 1).x();

    QString preeditText = textinputObject->text().mid(0, pos);
    textinputObject->setText(textinputObject->text().mid(pos));
    textinputObject->setCursorPosition(0);

    {   QInputMethodEvent inputEvent(preeditText, QList<QInputMethodEvent::Attribute>());
        QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &inputEvent); }

    // Check all points within the preedit text return the same position.
    QCOMPARE(evaluate<int>(textinputObject, QString("positionAt(%1)").arg(0)), 0);
    QCOMPARE(evaluate<int>(textinputObject, QString("positionAt(%1)").arg(x0 / 2)), 0);
    QCOMPARE(evaluate<int>(textinputObject, QString("positionAt(%1)").arg(x0)), 0);

    // Verify positioning returns to normal after the preedit text.
    QCOMPARE(evaluate<int>(textinputObject, QString("positionAt(%1)").arg(x1)), 1);
    QCOMPARE(textinputObject->positionToRectangle(1).x(), x1);

    {   QInputMethodEvent inputEvent;
        QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &inputEvent); }

    // With wrapping.
    textinputObject->setWrapMode(QQuickTextInput::WrapAnywhere);

    const qreal y0 = line.height() / 2;
    const qreal y1 = line.height() * 3 / 2;

    QCOMPARE(evaluate<int>(textinputObject, QString("positionAt(%1, %2)").arg(x0).arg(y0)), pos);
    QCOMPARE(evaluate<int>(textinputObject, QString("positionAt(%1, %2)").arg(x1).arg(y0)), pos + 1);

    int newLinePos = evaluate<int>(textinputObject, QString("positionAt(%1, %2)").arg(x0).arg(y1));
    QVERIFY(newLinePos > pos);
    QCOMPARE(evaluate<int>(textinputObject, QString("positionAt(%1, %2)").arg(x1).arg(y1)), newLinePos + 1);
}

void tst_qquicktextinput::maxLength()
{
    QQuickView canvas(testFileUrl("maxLength.qml"));
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
    QQuickView canvas(testFileUrl("masks.qml"));
    canvas.show();
    canvas.requestActivateWindow();
    QVERIFY(canvas.rootObject() != 0);
    QQuickTextInput *textinputObject = qobject_cast<QQuickTextInput *>(canvas.rootObject());
    QVERIFY(textinputObject != 0);
    QTRY_VERIFY(textinputObject->hasActiveFocus() == true);
    QVERIFY(textinputObject->text().length() == 0);
    QCOMPARE(textinputObject->inputMask(), QString("HHHHhhhh; "));
    QCOMPARE(textinputObject->length(), 8);
    for (int i=0; i<10; i++) {
        QTRY_COMPARE(qMin(i,8), textinputObject->text().length());
        QCOMPARE(textinputObject->length(), 8);
        QCOMPARE(textinputObject->getText(0, qMin(i, 8)), QString(qMin(i, 8), 'a'));
        QCOMPARE(textinputObject->getText(qMin(i, 8), 8), QString(8 - qMin(i, 8), ' '));
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

    QQuickView canvas(testFileUrl("validators.qml"));
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
    QQuickView canvas(testFileUrl("inputmethods.qml"));
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

    // test that basic tentative commit gets to text property on preedit state
    input->setText("");
    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent preeditEvent("test", attributes);
    preeditEvent.setTentativeCommitString("test");
    QApplication::sendEvent(input, &preeditEvent);
    QCOMPARE(input->text(), QString("test"));

    // tentative commit not allowed present in surrounding text
    QInputMethodQueryEvent queryEvent(Qt::ImSurroundingText);
    QApplication::sendEvent(input, &queryEvent);
    QCOMPARE(queryEvent.value(Qt::ImSurroundingText).toString(), QString(""));

    // if text with tentative commit does not validate, not allowed to be part of text property
    input->setText(""); // ensure input state is reset
    QValidator *validator = new QIntValidator(0, 100);
    input->setValidator(validator);
    QApplication::sendEvent(input, &preeditEvent);
    QCOMPARE(input->text(), QString(""));
    input->setValidator(0);
    delete validator;

    // input should reset selection even if replacement parameters are out of bounds
    input->setText("text");
    input->setCursorPosition(0);
    input->moveCursorSelection(input->text().length());
    event.setCommitString("replacement", -input->text().length(), input->text().length());
    QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &event);
    QCOMPARE(input->selectionStart(), input->selectionEnd());
}

/*
TextInput element should only handle left/right keys until the cursor reaches
the extent of the text, then they should ignore the keys.

*/
void tst_qquicktextinput::navigation()
{
    QQuickView canvas(testFileUrl("navigation.qml"));
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
    QQuickView canvas(testFileUrl("navigation.qml"));
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

void tst_qquicktextinput::copyAndPasteKeySequence() {
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

    QString componentStr = "import QtQuick 2.0\nTextInput { text: \"Hello world!\"; focus: true }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    QQuickCanvas canvas;
    textInput->setParentItem(canvas.rootItem());
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(QGuiApplication::activeWindow(), &canvas);

    // copy and paste
    QVERIFY(textInput->hasActiveFocus());
    QCOMPARE(textInput->text().length(), 12);
    textInput->select(0, textInput->text().length());
    simulateKeys(&canvas, QKeySequence::Copy);
    QCOMPARE(textInput->selectedText(), QString("Hello world!"));
    QCOMPARE(textInput->selectedText().length(), 12);
    textInput->setCursorPosition(0);
    QVERIFY(textInput->canPaste());
    simulateKeys(&canvas, QKeySequence::Paste);
    QCOMPARE(textInput->text(), QString("Hello world!Hello world!"));
    QCOMPARE(textInput->text().length(), 24);

    // select all and cut
    simulateKeys(&canvas, QKeySequence::SelectAll);
    simulateKeys(&canvas, QKeySequence::Cut);
    QCOMPARE(textInput->text().length(), 0);
    simulateKeys(&canvas, QKeySequence::Paste);
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
        simulateKeys(&canvas, QKeySequence::Copy);
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

    bool cp = !textInput->isReadOnly() && QGuiApplication::clipboard()->text().length() != 0;
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

    bool cp = !textInput->isReadOnly() && QGuiApplication::clipboard()->text().length() != 0;
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
    QQuickView view(testFileUrl("cursorTest.qml"));
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
    QQuickView view(testFileUrl("cursorVisible.qml"));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());

    QQuickTextInput input;
    input.componentComplete();
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

    QString text = "Hello World!";

    QQuickTextInput input;
    input.setText(text);
    input.componentComplete();

    QTextLayout layout(text);
    layout.setFont(input.font());
    if (!qmlDisableDistanceField()) {
        QTextOption option;
        option.setUseDesignMetrics(true);
        layout.setTextOption(option);
    }
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    input.setWidth(line.cursorToX(5, QTextLine::Leading));
    input.setHeight(qCeil(line.height() * 3 / 2));

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

        QVERIFY(r.left() < qCeil(line.cursorToX(i, QTextLine::Trailing)));
        QVERIFY(r.right() >= qFloor(line.cursorToX(i , QTextLine::Leading)));
        QCOMPARE(input.inputMethodQuery(Qt::ImCursorRectangle).toRect(), r);
    }

    // Check the cursor rectangle remains within the input bounding rect when auto scrolling.
    QVERIFY(r.left() < input.width());
    QVERIFY(r.right() >= input.width() - error);

    for (int i = 6; i < text.length(); ++i) {
        input.setCursorPosition(i);
        QCOMPARE(r, input.cursorRectangle());
        QCOMPARE(input.inputMethodQuery(Qt::ImCursorRectangle).toRect(), r);
    }

    for (int i = text.length() - 2; i >= 0; --i) {
        input.setCursorPosition(i);
        r = input.cursorRectangle();
        QCOMPARE(r.top(), 0);
        QVERIFY(r.right() >= 0);
        QCOMPARE(input.inputMethodQuery(Qt::ImCursorRectangle).toRect(), r);
    }

    // Check vertical scrolling with word wrap.
    input.setWrapMode(QQuickTextInput::WordWrap);
    for (int i = 0; i <= 5; ++i) {
        input.setCursorPosition(i);
        r = input.cursorRectangle();

        QVERIFY(r.left() < qCeil(line.cursorToX(i, QTextLine::Trailing)));
        QVERIFY(r.right() >= qFloor(line.cursorToX(i , QTextLine::Leading)));
        QCOMPARE(r.top(), 0);
        QCOMPARE(input.inputMethodQuery(Qt::ImCursorRectangle).toRect(), r);
    }

    input.setCursorPosition(6);
    r = input.cursorRectangle();
    QCOMPARE(r.left(), 0);
    QVERIFY(r.bottom() >= input.height() - error);

    for (int i = 7; i < text.length(); ++i) {
        input.setCursorPosition(i);
        r = input.cursorRectangle();
        QVERIFY(r.bottom() >= input.height() - error);
    }

    for (int i = text.length() - 2; i >= 6; --i) {
        input.setCursorPosition(i);
        r = input.cursorRectangle();
        QVERIFY(r.bottom() >= input.height() - error);
    }

    for (int i = 5; i >= 0; --i) {
        input.setCursorPosition(i);
        r = input.cursorRectangle();
        QCOMPARE(r.top(), 0);
    }

    input.setText("Hi!");
    input.setHAlign(QQuickTextInput::AlignRight);
    r = input.cursorRectangle();
    QVERIFY(r.left() < input.width() + error);
    QVERIFY(r.right() >= input.width() - error);
}

void tst_qquicktextinput::readOnly()
{
    QQuickView canvas(testFileUrl("readOnly.qml"));
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
    QQuickView canvas(testFileUrl("echoMode.qml"));
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
    ref &= ~(Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText | Qt::ImhSensitiveData);
    QCOMPARE(input->inputMethodHints(), ref);
    input->setEchoMode(QQuickTextInput::NoEcho);
    QCOMPARE(input->text(), initial);
    QCOMPARE(input->displayText(), QLatin1String(""));
    QCOMPARE(input->passwordCharacter(), QLatin1String("*"));
    //NoEcho
    ref |= Qt::ImhHiddenText;
    ref |= (Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText | Qt::ImhSensitiveData);
    QCOMPARE(input->inputMethodHints(), ref);
    input->setEchoMode(QQuickTextInput::Password);
    //Password
    ref |= Qt::ImhHiddenText;
    ref |= (Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText | Qt::ImhSensitiveData);
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
    ref |= (Qt::ImhNoAutoUppercase | Qt::ImhNoPredictiveText | Qt::ImhSensitiveData);
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
void tst_qquicktextinput::passwordEchoDelay()
{
    QQuickView canvas(testFileUrl("echoMode.qml"));
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());

    QVERIFY(canvas.rootObject() != 0);

    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(qvariant_cast<QObject *>(canvas.rootObject()->property("myInput")));

    QChar fillChar = QLatin1Char('*');

    input->setEchoMode(QQuickTextInput::Password);
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
    QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &ev);
    QCOMPARE(input->displayText(), QString(7, fillChar) + QLatin1Char('7'));

    input->setCursorPosition(3);
    QCOMPARE(input->displayText(), QString(7, fillChar) + QLatin1Char('7'));
    QTest::keyPress(&canvas, 'a');
    QCOMPARE(input->displayText(), QString(3, fillChar) + QLatin1Char('a') + QString(5, fillChar));
    QTest::keyPress(&canvas, Qt::Key_Backspace);
    QCOMPARE(input->displayText(), QString(8, fillChar));
}
#endif


void tst_qquicktextinput::simulateKey(QQuickView *view, int key)
{
    QKeyEvent press(QKeyEvent::KeyPress, key, 0);
    QKeyEvent release(QKeyEvent::KeyRelease, key, 0);

    QGuiApplication::sendEvent(view, &press);
    QGuiApplication::sendEvent(view, &release);
}


void tst_qquicktextinput::openInputPanel()
{
    PlatformInputContext platformInputContext;
    QInputPanelPrivate *inputPanelPrivate = QInputPanelPrivate::get(qApp->inputPanel());
    inputPanelPrivate->testContext = &platformInputContext;

    QQuickView view(testFileUrl("openInputPanel.qml"));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());

    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(view.rootObject());
    QVERIFY(input);

    // check default values
    QVERIFY(input->focusOnPress());
    QVERIFY(!input->hasActiveFocus());
    QCOMPARE(qApp->inputPanel()->inputItem(), static_cast<QObject*>(0));
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
    anotherInput.componentComplete();
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
    input.componentComplete();
    input2.componentComplete();
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
    QDeclarativeComponent component(&engine, testFileUrl("geometrySignals.qml"));
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

    QQuickView view(testFileUrl("preeditAutoScroll.qml"));
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
    QVERIFY(evaluate<int>(input, QString("positionAt(0)")) != 0);
    QVERIFY(input->cursorRectangle().left() < input->boundingRect().width());
    QCOMPARE(cursorRectangleSpy.count(), ++cursorRectangleChanges);

    // test the text is scrolled back when the preedit is removed.
    QInputMethodEvent imEvent;
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &imEvent);
    QCOMPARE(evaluate<int>(input, QString("positionAt(%1)").arg(0)), 0);
    QCOMPARE(evaluate<int>(input, QString("positionAt(%1)").arg(input->width())), 5);
    QCOMPARE(cursorRectangleSpy.count(), ++cursorRectangleChanges);

    QTextLayout layout(preeditText);
    layout.setFont(input->font());
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
    QCOMPARE(evaluate<int>(input, QString("positionAt(%1)").arg(0)), 0);
    QCOMPARE(evaluate<int>(input, QString("positionAt(%1)").arg(input->width())), 5);
}

void tst_qquicktextinput::preeditCursorRectangle()
{
    QString preeditText = "super";

    QQuickView view(testFileUrl("inputMethodEvent.qml"));
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

void tst_qquicktextinput::inputContextMouseHandler()
{
    PlatformInputContext platformInputContext;
    QInputPanelPrivate *inputPanelPrivate = QInputPanelPrivate::get(qApp->inputPanel());
    inputPanelPrivate->testContext = &platformInputContext;

    QString text = "supercalifragisiticexpialidocious!";
    QQuickView view(testFileUrl("inputContext.qml"));
    QQuickTextInput *input = qobject_cast<QQuickTextInput *>(view.rootObject());
    QVERIFY(input);

    input->setFocus(true);
    input->setText("");

    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());

    QTextLayout layout(text);
    layout.setFont(input->font());
    if (!qmlDisableDistanceField()) {
        QTextOption option;
        option.setUseDesignMetrics(true);
        layout.setTextOption(option);
    }
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    const qreal x = line.cursorToX(2, QTextLine::Leading);
    const qreal y = line.height() / 2;
    QPoint position = QPointF(x, y).toPoint();

    QInputMethodEvent inputEvent(text.mid(0, 5), QList<QInputMethodEvent::Attribute>());
    QApplication::sendEvent(input, &inputEvent);

    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, position);
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, position);
    QGuiApplication::processEvents();

    QCOMPARE(platformInputContext.m_action, QInputPanel::Click);
    QCOMPARE(platformInputContext.m_invokeActionCallCount, 1);
    QCOMPARE(platformInputContext.m_cursorPosition, 2);
}

void tst_qquicktextinput::inputMethodComposing()
{
    QString text = "supercalifragisiticexpialidocious!";

    QQuickView view(testFileUrl("inputContext.qml"));
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
    QQuickView *canvas = new QQuickView(testFileUrl("positionAt.qml"));
    QVERIFY(canvas->rootObject() != 0);
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput *>(canvas->rootObject());

    // make sure cursor rectangle is not at (0,0)
    textInput->setX(10);
    textInput->setY(10);
    textInput->setCursorPosition(3);
    QVERIFY(textInput != 0);
    textInput->setFocus(true);
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);

    QInputMethodQueryEvent event(Qt::ImCursorRectangle);
    qApp->sendEvent(qApp->inputPanel()->inputItem(), &event);
    QRectF cursorRectFromQuery = event.value(Qt::ImCursorRectangle).toRectF();

    QRect cursorRectFromItem = textInput->cursorRectangle();
    QRectF cursorRectFromPositionToRectangle = textInput->positionToRectangle(textInput->cursorPosition());

    // item and input query cursor rectangles match
    QCOMPARE(cursorRectFromItem, cursorRectFromQuery.toRect());

    // item cursor rectangle and positionToRectangle calculations match
    QCOMPARE(cursorRectFromItem, cursorRectFromPositionToRectangle.toRect());

    // item-canvas transform and input item transform match
#ifdef Q_OS_MAC
    QEXPECT_FAIL("","QTBUG-22966", Abort);
#endif
    QCOMPARE(QQuickItemPrivate::get(textInput)->itemToCanvasTransform(), qApp->inputPanel()->inputItemTransform());

    // input panel cursorRectangle property and tranformed item cursor rectangle match
    QRectF sceneCursorRect = QQuickItemPrivate::get(textInput)->itemToCanvasTransform().mapRect(cursorRectFromItem);
    QCOMPARE(sceneCursorRect, qApp->inputPanel()->cursorRectangle());

    delete canvas;
}

void tst_qquicktextinput::tripleClickSelectsAll()
{
    QString qmlfile = testFile("positionAt.qml");
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
    QTest::qWait(qApp->styleHints()->mouseDoubleClickInterval() + 1);
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


void tst_qquicktextinput::getText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("inputMask");
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("end");
    QTest::addColumn<QString>("expectedText");

    QTest::newRow("all plain text")
            << standard.at(0)
            << QString()
            << 0 << standard.at(0).length()
            << standard.at(0);

    QTest::newRow("plain text sub string")
            << standard.at(0)
            << QString()
            << 0 << 12
            << standard.at(0).mid(0, 12);

    QTest::newRow("plain text sub string reversed")
            << standard.at(0)
            << QString()
            << 12 << 0
            << standard.at(0).mid(0, 12);

    QTest::newRow("plain text cropped beginning")
            << standard.at(0)
            << QString()
            << -3 << 4
            << standard.at(0).mid(0, 4);

    QTest::newRow("plain text cropped end")
            << standard.at(0)
            << QString()
            << 23 << standard.at(0).length() + 8
            << standard.at(0).mid(23);

    QTest::newRow("plain text cropped beginning and end")
            << standard.at(0)
            << QString()
            << -9 << standard.at(0).length() + 4
            << standard.at(0);
}

void tst_qquicktextinput::getText()
{
    QFETCH(QString, text);
    QFETCH(QString, inputMask);
    QFETCH(int, start);
    QFETCH(int, end);
    QFETCH(QString, expectedText);

    QString componentStr = "import QtQuick 2.0\nTextInput { text: \"" + text + "\"; inputMask: \"" + inputMask + "\" }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    QCOMPARE(textInput->getText(start, end), expectedText);
}

void tst_qquicktextinput::insert_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("inputMask");
    QTest::addColumn<int>("selectionStart");
    QTest::addColumn<int>("selectionEnd");
    QTest::addColumn<int>("insertPosition");
    QTest::addColumn<QString>("insertText");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<int>("expectedSelectionStart");
    QTest::addColumn<int>("expectedSelectionEnd");
    QTest::addColumn<int>("expectedCursorPosition");
    QTest::addColumn<bool>("selectionChanged");
    QTest::addColumn<bool>("cursorPositionChanged");

    QTest::newRow("at cursor position (beginning)")
            << standard.at(0)
            << QString()
            << 0 << 0 << 0
            << QString("Hello")
            << QString("Hello") + standard.at(0)
            << 5 << 5 << 5
            << false << true;

    QTest::newRow("at cursor position (end)")
            << standard.at(0)
            << QString()
            << standard.at(0).length() << standard.at(0).length() << standard.at(0).length()
            << QString("Hello")
            << standard.at(0) + QString("Hello")
            << standard.at(0).length() + 5 << standard.at(0).length() + 5 << standard.at(0).length() + 5
            << false << true;

    QTest::newRow("at cursor position (middle)")
            << standard.at(0)
            << QString()
            << 18 << 18 << 18
            << QString("Hello")
            << standard.at(0).mid(0, 18) + QString("Hello") + standard.at(0).mid(18)
            << 23 << 23 << 23
            << false << true;

    QTest::newRow("after cursor position (beginning)")
            << standard.at(0)
            << QString()
            << 0 << 0 << 18
            << QString("Hello")
            << standard.at(0).mid(0, 18) + QString("Hello") + standard.at(0).mid(18)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("before cursor position (end)")
            << standard.at(0)
            << QString()
            << standard.at(0).length() << standard.at(0).length() << 18
            << QString("Hello")
            << standard.at(0).mid(0, 18) + QString("Hello") + standard.at(0).mid(18)
            << standard.at(0).length() + 5 << standard.at(0).length() + 5 << standard.at(0).length() + 5
            << false << true;

    QTest::newRow("before cursor position (middle)")
            << standard.at(0)
            << QString()
            << 18 << 18 << 0
            << QString("Hello")
            << QString("Hello") + standard.at(0)
            << 23 << 23 << 23
            << false << true;

    QTest::newRow("after cursor position (middle)")
            << standard.at(0)
            << QString()
            << 18 << 18 << standard.at(0).length()
            << QString("Hello")
            << standard.at(0) + QString("Hello")
            << 18 << 18 << 18
            << false << false;

    QTest::newRow("before selection")
            << standard.at(0)
            << QString()
            << 14 << 19 << 0
            << QString("Hello")
            << QString("Hello") + standard.at(0)
            << 19 << 24 << 24
            << false << true;

    QTest::newRow("before reversed selection")
            << standard.at(0)
            << QString()
            << 19 << 14 << 0
            << QString("Hello")
            << QString("Hello") + standard.at(0)
            << 19 << 24 << 19
            << false << true;

    QTest::newRow("after selection")
            << standard.at(0)
            << QString()
            << 14 << 19 << standard.at(0).length()
            << QString("Hello")
            << standard.at(0) + QString("Hello")
            << 14 << 19 << 19
            << false << false;

    QTest::newRow("after reversed selection")
            << standard.at(0)
            << QString()
            << 19 << 14 << standard.at(0).length()
            << QString("Hello")
            << standard.at(0) + QString("Hello")
            << 14 << 19 << 14
            << false << false;

    QTest::newRow("into selection")
            << standard.at(0)
            << QString()
            << 14 << 19 << 18
            << QString("Hello")
            << standard.at(0).mid(0, 18) + QString("Hello") + standard.at(0).mid(18)
            << 14 << 24 << 24
            << true << true;

    QTest::newRow("into reversed selection")
            << standard.at(0)
            << QString()
            << 19 << 14 << 18
            << QString("Hello")
            << standard.at(0).mid(0, 18) + QString("Hello") + standard.at(0).mid(18)
            << 14 << 24 << 14
            << true << false;

    QTest::newRow("rich text into plain text")
            << standard.at(0)
            << QString()
            << 0 << 0 << 0
            << QString("<b>Hello</b>")
            << QString("<b>Hello</b>") + standard.at(0)
            << 12 << 12 << 12
            << false << true;

    QTest::newRow("before start")
            << standard.at(0)
            << QString()
            << 0 << 0 << -3
            << QString("Hello")
            << standard.at(0)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("past end")
            << standard.at(0)
            << QString()
            << 0 << 0 << standard.at(0).length() + 3
            << QString("Hello")
            << standard.at(0)
            << 0 << 0 << 0
            << false << false;

    const QString inputMask = "009.009.009.009";
    const QString ip = "192.168.2.14";

    QTest::newRow("mask: at cursor position (beginning)")
            << ip
            << inputMask
            << 0 << 0 << 0
            << QString("125")
            << QString("125.168.2.14")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("mask: at cursor position (end)")
            << ip
            << inputMask
            << inputMask.length() << inputMask.length() << inputMask.length()
            << QString("8")
            << ip
            << inputMask.length() << inputMask.length() << inputMask.length()
            << false << false;

    QTest::newRow("mask: at cursor position (middle)")
            << ip
            << inputMask
            << 6 << 6 << 6
            << QString("75.2")
            << QString("192.167.5.24")
            << 6 << 6 << 6
            << false << false;

    QTest::newRow("mask: after cursor position (beginning)")
            << ip
            << inputMask
            << 0 << 0 << 6
            << QString("75.2")
            << QString("192.167.5.24")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("mask: before cursor position (end)")
            << ip
            << inputMask
            << inputMask.length() << inputMask.length() << 6
            << QString("75.2")
            << QString("192.167.5.24")
            << inputMask.length() << inputMask.length() << inputMask.length()
            << false << false;

    QTest::newRow("mask: before cursor position (middle)")
            << ip
            << inputMask
            << 6 << 6 << 0
            << QString("125")
            << QString("125.168.2.14")
            << 6 << 6 << 6
            << false << false;

    QTest::newRow("mask: after cursor position (middle)")
            << ip
            << inputMask
            << 6 << 6 << 13
            << QString("8")
            << "192.168.2.18"
            << 6 << 6 << 6
            << false << false;

    QTest::newRow("mask: before selection")
            << ip
            << inputMask
            << 6 << 8 << 0
            << QString("125")
            << QString("125.168.2.14")
            << 6 << 8 << 8
            << false << false;

    QTest::newRow("mask: before reversed selection")
            << ip
            << inputMask
            << 8 << 6 << 0
            << QString("125")
            << QString("125.168.2.14")
            << 6 << 8 << 6
            << false << false;

    QTest::newRow("mask: after selection")
            << ip
            << inputMask
            << 6 << 8 << 13
            << QString("8")
            << "192.168.2.18"
            << 6 << 8 << 8
            << false << false;

    QTest::newRow("mask: after reversed selection")
            << ip
            << inputMask
            << 8 << 6 << 13
            << QString("8")
            << "192.168.2.18"
            << 6 << 8 << 6
            << false << false;

    QTest::newRow("mask: into selection")
            << ip
            << inputMask
            << 5 << 8 << 6
            << QString("75.2")
            << QString("192.167.5.24")
            << 5 << 8 << 8
            << true << false;

    QTest::newRow("mask: into reversed selection")
            << ip
            << inputMask
            << 8 << 5 << 6
            << QString("75.2")
            << QString("192.167.5.24")
            << 5 << 8 << 5
            << true << false;

    QTest::newRow("mask: before start")
            << ip
            << inputMask
            << 0 << 0 << -3
            << QString("4")
            << ip
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("mask: past end")
            << ip
            << inputMask
            << 0 << 0 << ip.length() + 3
            << QString("4")
            << ip
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("mask: invalid characters")
            << ip
            << inputMask
            << 0 << 0 << 0
            << QString("abc")
            << QString("192.168.2.14")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("mask: mixed validity")
            << ip
            << inputMask
            << 0 << 0 << 0
            << QString("a1b2c5")
            << QString("125.168.2.14")
            << 0 << 0 << 0
            << false << false;
}

void tst_qquicktextinput::insert()
{
    QFETCH(QString, text);
    QFETCH(QString, inputMask);
    QFETCH(int, selectionStart);
    QFETCH(int, selectionEnd);
    QFETCH(int, insertPosition);
    QFETCH(QString, insertText);
    QFETCH(QString, expectedText);
    QFETCH(int, expectedSelectionStart);
    QFETCH(int, expectedSelectionEnd);
    QFETCH(int, expectedCursorPosition);
    QFETCH(bool, selectionChanged);
    QFETCH(bool, cursorPositionChanged);

    QString componentStr = "import QtQuick 2.0\nTextInput { text: \"" + text + "\"; inputMask: \"" + inputMask + "\" }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    textInput->select(selectionStart, selectionEnd);

    QSignalSpy selectionSpy(textInput, SIGNAL(selectedTextChanged()));
    QSignalSpy selectionStartSpy(textInput, SIGNAL(selectionStartChanged()));
    QSignalSpy selectionEndSpy(textInput, SIGNAL(selectionEndChanged()));
    QSignalSpy textSpy(textInput, SIGNAL(textChanged()));
    QSignalSpy cursorPositionSpy(textInput, SIGNAL(cursorPositionChanged()));

    textInput->insert(insertPosition, insertText);

    QCOMPARE(textInput->text(), expectedText);
    QCOMPARE(textInput->length(), inputMask.isEmpty() ? expectedText.length() : inputMask.length());

    QCOMPARE(textInput->selectionStart(), expectedSelectionStart);
    QCOMPARE(textInput->selectionEnd(), expectedSelectionEnd);
    QCOMPARE(textInput->cursorPosition(), expectedCursorPosition);

    if (selectionStart > selectionEnd)
        qSwap(selectionStart, selectionEnd);

    QCOMPARE(selectionSpy.count() > 0, selectionChanged);
    QCOMPARE(selectionStartSpy.count() > 0, selectionStart != expectedSelectionStart);
    QCOMPARE(selectionEndSpy.count() > 0, selectionEnd != expectedSelectionEnd);
    QCOMPARE(textSpy.count() > 0, text != expectedText);
    QCOMPARE(cursorPositionSpy.count() > 0, cursorPositionChanged);
}

void tst_qquicktextinput::remove_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("inputMask");
    QTest::addColumn<int>("selectionStart");
    QTest::addColumn<int>("selectionEnd");
    QTest::addColumn<int>("removeStart");
    QTest::addColumn<int>("removeEnd");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<int>("expectedSelectionStart");
    QTest::addColumn<int>("expectedSelectionEnd");
    QTest::addColumn<int>("expectedCursorPosition");
    QTest::addColumn<bool>("selectionChanged");
    QTest::addColumn<bool>("cursorPositionChanged");

    QTest::newRow("from cursor position (beginning)")
            << standard.at(0)
            << QString()
            << 0 << 0
            << 0 << 5
            << standard.at(0).mid(5)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("to cursor position (beginning)")
            << standard.at(0)
            << QString()
            << 0 << 0
            << 5 << 0
            << standard.at(0).mid(5)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("to cursor position (end)")
            << standard.at(0)
            << QString()
            << standard.at(0).length() << standard.at(0).length()
            << standard.at(0).length() << standard.at(0).length() - 5
            << standard.at(0).mid(0, standard.at(0).length() - 5)
            << standard.at(0).length() - 5 << standard.at(0).length() - 5 << standard.at(0).length() - 5
            << false << true;

    QTest::newRow("to cursor position (end)")
            << standard.at(0)
            << QString()
            << standard.at(0).length() << standard.at(0).length()
            << standard.at(0).length() - 5 << standard.at(0).length()
            << standard.at(0).mid(0, standard.at(0).length() - 5)
            << standard.at(0).length() - 5 << standard.at(0).length() - 5 << standard.at(0).length() - 5
            << false << true;

    QTest::newRow("from cursor position (middle)")
            << standard.at(0)
            << QString()
            << 18 << 18
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 18 << 18 << 18
            << false << false;

    QTest::newRow("to cursor position (middle)")
            << standard.at(0)
            << QString()
            << 23 << 23
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 18 << 18 << 18
            << false << true;

    QTest::newRow("after cursor position (beginning)")
            << standard.at(0)
            << QString()
            << 0 << 0
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("before cursor position (end)")
            << standard.at(0)
            << QString()
            << standard.at(0).length() << standard.at(0).length()
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << standard.at(0).length() - 5 << standard.at(0).length() - 5 << standard.at(0).length() - 5
            << false << true;

    QTest::newRow("before cursor position (middle)")
            << standard.at(0)
            << QString()
            << 23 << 23
            << 0 << 5
            << standard.at(0).mid(5)
            << 18 << 18 << 18
            << false << true;

    QTest::newRow("after cursor position (middle)")
            << standard.at(0)
            << QString()
            << 18 << 18
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 18 << 18 << 18
            << false << false;

    QTest::newRow("before selection")
            << standard.at(0)
            << QString()
            << 14 << 19
            << 0 << 5
            << standard.at(0).mid(5)
            << 9 << 14 << 14
            << false << true;

    QTest::newRow("before reversed selection")
            << standard.at(0)
            << QString()
            << 19 << 14
            << 0 << 5
            << standard.at(0).mid(5)
            << 9 << 14 << 9
            << false << true;

    QTest::newRow("after selection")
            << standard.at(0)
            << QString()
            << 14 << 19
            << standard.at(0).length() - 5 << standard.at(0).length()
            << standard.at(0).mid(0, standard.at(0).length() - 5)
            << 14 << 19 << 19
            << false << false;

    QTest::newRow("after reversed selection")
            << standard.at(0)
            << QString()
            << 19 << 14
            << standard.at(0).length() - 5 << standard.at(0).length()
            << standard.at(0).mid(0, standard.at(0).length() - 5)
            << 14 << 19 << 14
            << false << false;

    QTest::newRow("from selection")
            << standard.at(0)
            << QString()
            << 14 << 24
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 14 << 19 << 19
            << true << true;

    QTest::newRow("from reversed selection")
            << standard.at(0)
            << QString()
            << 24 << 14
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 14 << 19 << 14
            << true << false;

    QTest::newRow("cropped beginning")
            << standard.at(0)
            << QString()
            << 0 << 0
            << -3 << 4
            << standard.at(0).mid(4)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("cropped end")
            << standard.at(0)
            << QString()
            << 0 << 0
            << 23 << standard.at(0).length() + 8
            << standard.at(0).mid(0, 23)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("cropped beginning and end")
            << standard.at(0)
            << QString()
            << 0 << 0
            << -9 << standard.at(0).length() + 4
            << QString()
            << 0 << 0 << 0
            << false << false;

    const QString inputMask = "009.009.009.009";
    const QString ip = "192.168.2.14";

    QTest::newRow("mask: from cursor position")
            << ip
            << inputMask
            << 6 << 6
            << 6 << 9
            << QString("192.16..14")
            << 6 << 6 << 6
            << false << false;

    QTest::newRow("mask: to cursor position")
            << ip
            << inputMask
            << 6 << 6
            << 2 << 6
            << QString("19.8.2.14")
            << 6 << 6 << 6
            << false << false;

    QTest::newRow("mask: before cursor position")
            << ip
            << inputMask
            << 6 << 6
            << 0 << 2
            << QString("2.168.2.14")
            << 6 << 6 << 6
            << false << false;

    QTest::newRow("mask: after cursor position")
            << ip
            << inputMask
            << 6 << 6
            << 12 << 16
            << QString("192.168.2.")
            << 6 << 6 << 6
            << false << false;

    QTest::newRow("mask: before selection")
            << ip
            << inputMask
            << 6 << 8
            << 0 << 2
            << QString("2.168.2.14")
            << 6 << 8 << 8
            << false << false;

    QTest::newRow("mask: before reversed selection")
            << ip
            << inputMask
            << 8 << 6
            << 0 << 2
            << QString("2.168.2.14")
            << 6 << 8 << 6
            << false << false;

    QTest::newRow("mask: after selection")
            << ip
            << inputMask
            << 6 << 8
            << 12 << 16
            << QString("192.168.2.")
            << 6 << 8 << 8
            << false << false;

    QTest::newRow("mask: after reversed selection")
            << ip
            << inputMask
            << 8 << 6
            << 12 << 16
            << QString("192.168.2.")
            << 6 << 8 << 6
            << false << false;

    QTest::newRow("mask: from selection")
            << ip
            << inputMask
            << 6 << 13
            << 8 << 10
            << QString("192.168..14")
            << 6 << 13 << 13
            << true << false;

    QTest::newRow("mask: from reversed selection")
            << ip
            << inputMask
            << 13 << 6
            << 8 << 10
            << QString("192.168..14")
            << 6 << 13 << 6
            << true << false;

    QTest::newRow("mask: cropped beginning")
            << ip
            << inputMask
            << 0 << 0
            << -3 << 4
            << QString(".168.2.14")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("mask: cropped end")
            << ip
            << inputMask
            << 0 << 0
            << 13 << 28
            << QString("192.168.2.1")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("mask: cropped beginning and end")
            << ip
            << inputMask
            << 0 << 0
            << -9 << 28
            << QString("...")
            << 0 << 0 << 0
            << false << false;
}

void tst_qquicktextinput::remove()
{
    QFETCH(QString, text);
    QFETCH(QString, inputMask);
    QFETCH(int, selectionStart);
    QFETCH(int, selectionEnd);
    QFETCH(int, removeStart);
    QFETCH(int, removeEnd);
    QFETCH(QString, expectedText);
    QFETCH(int, expectedSelectionStart);
    QFETCH(int, expectedSelectionEnd);
    QFETCH(int, expectedCursorPosition);
    QFETCH(bool, selectionChanged);
    QFETCH(bool, cursorPositionChanged);

    QString componentStr = "import QtQuick 2.0\nTextInput { text: \"" + text + "\"; inputMask: \"" + inputMask + "\" }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    textInput->select(selectionStart, selectionEnd);

    QSignalSpy selectionSpy(textInput, SIGNAL(selectedTextChanged()));
    QSignalSpy selectionStartSpy(textInput, SIGNAL(selectionStartChanged()));
    QSignalSpy selectionEndSpy(textInput, SIGNAL(selectionEndChanged()));
    QSignalSpy textSpy(textInput, SIGNAL(textChanged()));
    QSignalSpy cursorPositionSpy(textInput, SIGNAL(cursorPositionChanged()));

    textInput->remove(removeStart, removeEnd);

    QCOMPARE(textInput->text(), expectedText);
    QCOMPARE(textInput->length(), inputMask.isEmpty() ? expectedText.length() : inputMask.length());

    if (selectionStart > selectionEnd)  //
        qSwap(selectionStart, selectionEnd);

    QCOMPARE(textInput->selectionStart(), expectedSelectionStart);
    QCOMPARE(textInput->selectionEnd(), expectedSelectionEnd);
    QCOMPARE(textInput->cursorPosition(), expectedCursorPosition);

    QCOMPARE(selectionSpy.count() > 0, selectionChanged);
    QCOMPARE(selectionStartSpy.count() > 0, selectionStart != expectedSelectionStart);
    QCOMPARE(selectionEndSpy.count() > 0, selectionEnd != expectedSelectionEnd);
    QCOMPARE(textSpy.count() > 0, text != expectedText);

    if (cursorPositionChanged)  //
        QVERIFY(cursorPositionSpy.count() > 0);
}

void tst_qquicktextinput::keySequence_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QKeySequence>("sequence");
    QTest::addColumn<int>("selectionStart");
    QTest::addColumn<int>("selectionEnd");
    QTest::addColumn<int>("cursorPosition");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<QString>("selectedText");

    // standard[0] == "the [4]quick [10]brown [16]fox [20]jumped [27]over [32]the [36]lazy [41]dog"

    QTest::newRow("select all")
            << standard.at(0) << QKeySequence(QKeySequence::SelectAll) << 0 << 0
            << 44 << standard.at(0) << standard.at(0);
    QTest::newRow("select end of line")
            << standard.at(0) << QKeySequence(QKeySequence::SelectEndOfLine) << 5 << 5
            << 44 << standard.at(0) << standard.at(0).mid(5);
    QTest::newRow("select end of document") // ### Not handled.
            << standard.at(0) << QKeySequence(QKeySequence::SelectEndOfDocument) << 3 << 3
            << 3 << standard.at(0) << QString();
    QTest::newRow("select end of block")
            << standard.at(0) << QKeySequence(QKeySequence::SelectEndOfBlock) << 18 << 18
            << 44 << standard.at(0) << standard.at(0).mid(18);
    QTest::newRow("delete end of line")
            << standard.at(0) << QKeySequence(QKeySequence::DeleteEndOfLine) << 24 << 24
            << 24 << standard.at(0).mid(0, 24) << QString();
    QTest::newRow("move to start of line")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToStartOfLine) << 31 << 31
            << 0 << standard.at(0) << QString();
    QTest::newRow("move to start of block")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToStartOfBlock) << 25 << 25
            << 0 << standard.at(0) << QString();
    QTest::newRow("move to next char")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToNextChar) << 12 << 12
            << 13 << standard.at(0) << QString();
    QTest::newRow("move to previous char")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToPreviousChar) << 3 << 3
            << 2 << standard.at(0) << QString();
    QTest::newRow("select next char")
            << standard.at(0) << QKeySequence(QKeySequence::SelectNextChar) << 23 << 23
            << 24 << standard.at(0) << standard.at(0).mid(23, 1);
    QTest::newRow("select previous char")
            << standard.at(0) << QKeySequence(QKeySequence::SelectPreviousChar) << 19 << 19
            << 18 << standard.at(0) << standard.at(0).mid(18, 1);
    QTest::newRow("move to next word")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToNextWord) << 7 << 7
            << 10 << standard.at(0) << QString();
    QTest::newRow("move to previous word")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToPreviousWord) << 7 << 7
            << 4 << standard.at(0) << QString();
    QTest::newRow("select previous word")
            << standard.at(0) << QKeySequence(QKeySequence::SelectPreviousWord) << 11 << 11
            << 10 << standard.at(0) << standard.at(0).mid(10, 1);
    QTest::newRow("delete (selection)")
            << standard.at(0) << QKeySequence(QKeySequence::Delete) << 12 << 15
            << 12 << (standard.at(0).mid(0, 12) + standard.at(0).mid(15)) << QString();
    QTest::newRow("delete (no selection)")
            << standard.at(0) << QKeySequence(QKeySequence::Delete) << 15 << 15
            << 15 << (standard.at(0).mid(0, 15) + standard.at(0).mid(16)) << QString();
    QTest::newRow("delete end of word")
            << standard.at(0) << QKeySequence(QKeySequence::DeleteEndOfWord) << 24 << 24
            << 24 << (standard.at(0).mid(0, 24) + standard.at(0).mid(27)) << QString();
    QTest::newRow("delete start of word")
            << standard.at(0) << QKeySequence(QKeySequence::DeleteStartOfWord) << 7 << 7
            << 4 << (standard.at(0).mid(0, 4) + standard.at(0).mid(7)) << QString();
}

void tst_qquicktextinput::keySequence()
{
    QFETCH(QString, text);
    QFETCH(QKeySequence, sequence);
    QFETCH(int, selectionStart);
    QFETCH(int, selectionEnd);
    QFETCH(int, cursorPosition);
    QFETCH(QString, expectedText);
    QFETCH(QString, selectedText);

    if (sequence.isEmpty()) {
        QSKIP("Key sequence is undefined");
    }

    QString componentStr = "import QtQuick 2.0\nTextInput { focus: true; text: \"" + text + "\" }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    QQuickCanvas canvas;
    textInput->setParentItem(canvas.rootItem());
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(QGuiApplication::activeWindow(), &canvas);

    textInput->select(selectionStart, selectionEnd);

    simulateKeys(&canvas, sequence);

    QCOMPARE(textInput->cursorPosition(), cursorPosition);
    QCOMPARE(textInput->text(), expectedText);
    QCOMPARE(textInput->selectedText(), selectedText);
}

#define NORMAL 0
#define REPLACE_UNTIL_END 1

void tst_qquicktextinput::undo_data()
{
    QTest::addColumn<QStringList>("insertString");
    QTest::addColumn<IntList>("insertIndex");
    QTest::addColumn<IntList>("insertMode");
    QTest::addColumn<QStringList>("expectedString");
    QTest::addColumn<bool>("use_keys");

    for (int i=0; i<2; i++) {
        QString keys_str = "keyboard";
        bool use_keys = true;
        if (i==0) {
            keys_str = "insert";
            use_keys = false;
        }

        {
            IntList insertIndex;
            IntList insertMode;
            QStringList insertString;
            QStringList expectedString;

            insertIndex << -1;
            insertMode << NORMAL;
            insertString << "1";

            insertIndex << -1;
            insertMode << NORMAL;
            insertString << "5";

            insertIndex << 1;
            insertMode << NORMAL;
            insertString << "3";

            insertIndex << 1;
            insertMode << NORMAL;
            insertString << "2";

            insertIndex << 3;
            insertMode << NORMAL;
            insertString << "4";

            expectedString << "12345";
            expectedString << "1235";
            expectedString << "135";
            expectedString << "15";
            expectedString << "";

            QTest::newRow(QString(keys_str + "_numbers").toLatin1()) <<
                insertString <<
                insertIndex <<
                insertMode <<
                expectedString <<
                bool(use_keys);
        }
        {
            IntList insertIndex;
            IntList insertMode;
            QStringList insertString;
            QStringList expectedString;

            insertIndex << -1;
            insertMode << NORMAL;
            insertString << "World"; // World

            insertIndex << 0;
            insertMode << NORMAL;
            insertString << "Hello"; // HelloWorld

            insertIndex << 0;
            insertMode << NORMAL;
            insertString << "Well"; // WellHelloWorld

            insertIndex << 9;
            insertMode << NORMAL;
            insertString << "There"; // WellHelloThereWorld;

            expectedString << "WellHelloThereWorld";
            expectedString << "WellHelloWorld";
            expectedString << "HelloWorld";
            expectedString << "World";
            expectedString << "";

            QTest::newRow(QString(keys_str + "_helloworld").toLatin1()) <<
                insertString <<
                insertIndex <<
                insertMode <<
                expectedString <<
                bool(use_keys);
        }
        {
            IntList insertIndex;
            IntList insertMode;
            QStringList insertString;
            QStringList expectedString;

            insertIndex << -1;
            insertMode << NORMAL;
            insertString << "Ensuring";

            insertIndex << -1;
            insertMode << NORMAL;
            insertString << " instan";

            insertIndex << 9;
            insertMode << NORMAL;
            insertString << "an ";

            insertIndex << 10;
            insertMode << REPLACE_UNTIL_END;
            insertString << " unique instance.";

            expectedString << "Ensuring a unique instance.";
            expectedString << "Ensuring an instan";
            expectedString << "Ensuring instan";
            expectedString << "";

            QTest::newRow(QString(keys_str + "_patterns").toLatin1()) <<
                insertString <<
                insertIndex <<
                insertMode <<
                expectedString <<
                bool(use_keys);
        }
    }
}

void tst_qquicktextinput::undo()
{
    QFETCH(QStringList, insertString);
    QFETCH(IntList, insertIndex);
    QFETCH(IntList, insertMode);
    QFETCH(QStringList, expectedString);

    QString componentStr = "import QtQuick 2.0\nTextInput { focus: true }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    QQuickCanvas canvas;
    textInput->setParentItem(canvas.rootItem());
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(QGuiApplication::activeWindow(), &canvas);

    int i;

// STEP 1: First build up an undo history by inserting or typing some strings...
    for (i = 0; i < insertString.size(); ++i) {
        if (insertIndex[i] > -1)
            textInput->setCursorPosition(insertIndex[i]);

 // experimental stuff
        if (insertMode[i] == REPLACE_UNTIL_END) {
            textInput->select(insertIndex[i], insertIndex[i] + 8);

            // This is what I actually want...
            // QTest::keyClick(testWidget, Qt::Key_End, Qt::ShiftModifier);
        }

        for (int j = 0; j < insertString.at(i).length(); j++)
            QTest::keyClick(&canvas, insertString.at(i).at(j).toLatin1());
    }

// STEP 2: Next call undo several times and see if we can restore to the previous state
    for (i = 0; i < expectedString.size() - 1; ++i) {
        QCOMPARE(textInput->text(), expectedString[i]);
        simulateKeys(&canvas, QKeySequence::Undo);
    }

// STEP 3: Verify that we have undone everything
    QVERIFY(textInput->text().isEmpty());
}

void tst_qquicktextinput::redo_data()
{
    QTest::addColumn<QStringList>("insertString");
    QTest::addColumn<IntList>("insertIndex");
    QTest::addColumn<QStringList>("expectedString");

    {
        IntList insertIndex;
        QStringList insertString;
        QStringList expectedString;

        insertIndex << -1;
        insertString << "World"; // World
        insertIndex << 0;
        insertString << "Hello"; // HelloWorld
        insertIndex << 0;
        insertString << "Well"; // WellHelloWorld
        insertIndex << 9;
        insertString << "There"; // WellHelloThereWorld;

        expectedString << "World";
        expectedString << "HelloWorld";
        expectedString << "WellHelloWorld";
        expectedString << "WellHelloThereWorld";

        QTest::newRow("Inserts and setting cursor") << insertString << insertIndex << expectedString;
    }
}

void tst_qquicktextinput::redo()
{
    QFETCH(QStringList, insertString);
    QFETCH(IntList, insertIndex);
    QFETCH(QStringList, expectedString);

    QString componentStr = "import QtQuick 2.0\nTextInput { focus: true }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    QQuickCanvas canvas;
    textInput->setParentItem(canvas.rootItem());
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(QGuiApplication::activeWindow(), &canvas);

    int i;
    // inserts the diff strings at diff positions
    for (i = 0; i < insertString.size(); ++i) {
        if (insertIndex[i] > -1)
            textInput->setCursorPosition(insertIndex[i]);
        for (int j = 0; j < insertString.at(i).length(); j++)
            QTest::keyClick(&canvas, insertString.at(i).at(j).toLatin1());
    }

    // undo everything
    while (!textInput->text().isEmpty())
        simulateKeys(&canvas, QKeySequence::Undo);

    for (i = 0; i < expectedString.size(); ++i) {
        simulateKeys(&canvas, QKeySequence::Redo);
        QCOMPARE(textInput->text() , expectedString[i]);
    }
}

void tst_qquicktextinput::undo_keypressevents_data()
{
    QTest::addColumn<KeyList>("keys");
    QTest::addColumn<QStringList>("expectedString");

    {
        KeyList keys;
        QStringList expectedString;

        keys << "AFRAID"
                << QKeySequence::MoveToStartOfLine
                << "VERY"
                << Qt::Key_Left
                << Qt::Key_Left
                << Qt::Key_Left
                << Qt::Key_Left
                << "BE"
                << QKeySequence::MoveToEndOfLine
                << "!";

        expectedString << "BEVERYAFRAID!";
        expectedString << "BEVERYAFRAID";
        expectedString << "VERYAFRAID";
        expectedString << "AFRAID";

        QTest::newRow("Inserts and moving cursor") << keys << expectedString;
    } {
        KeyList keys;
        QStringList expectedString;

        // inserting '1234'
        keys << "1234" << QKeySequence::MoveToStartOfLine
                // skipping '12'
                << Qt::Key_Right << Qt::Key_Right
                // selecting '34'
                << (Qt::Key_Right | Qt::ShiftModifier) << (Qt::Key_Right | Qt::ShiftModifier)
                // deleting '34'
                << Qt::Key_Delete;

        expectedString << "12";
        expectedString << "1234";

        QTest::newRow("Inserts,moving,selection and delete") << keys << expectedString;
    } {
        KeyList keys;
        QStringList expectedString;

        // inserting 'AB12'
        keys << "AB12"
                << QKeySequence::MoveToStartOfLine
                // selecting 'AB'
                << (Qt::Key_Right | Qt::ShiftModifier) << (Qt::Key_Right | Qt::ShiftModifier)
                << Qt::Key_Delete
                << QKeySequence::Undo
                << Qt::Key_Right
#ifdef Q_OS_WIN //Mac(?) has a specialcase to handle jumping to the end of a selection
                << Qt::Key_Left
#endif
                << (Qt::Key_Right | Qt::ShiftModifier) << (Qt::Key_Right | Qt::ShiftModifier)
                << Qt::Key_Delete;

        expectedString << "AB";
        expectedString << "AB12";

        QTest::newRow("Inserts,moving,selection, delete and undo") << keys << expectedString;
    } {
        KeyList keys;
        QStringList expectedString;

        // inserting 'ABCD'
        keys << "abcd"
                //move left two
                << Qt::Key_Left << Qt::Key_Left
                // inserting '1234'
                << "1234"
                // selecting '1234'
                << (Qt::Key_Left | Qt::ShiftModifier) << (Qt::Key_Left | Qt::ShiftModifier) << (Qt::Key_Left | Qt::ShiftModifier) << (Qt::Key_Left | Qt::ShiftModifier)
                // overwriting '1234' with '5'
                << "5"
                // undoing deletion of 'AB'
                << QKeySequence::Undo
                // overwriting '1234' with '6'
                << "6";

        expectedString << "ab6cd";
        // for versions previous to 3.2 we overwrite needed two undo operations
        expectedString << "ab1234cd";
        expectedString << "abcd";

        QTest::newRow("Inserts,moving,selection and undo, removing selection") << keys << expectedString;
    } {
        KeyList keys;
        QStringList expectedString;

        // inserting 'ABC'
        keys << "ABC"
                // removes 'C'
                << Qt::Key_Backspace;

        expectedString << "AB";
        expectedString << "ABC";

        QTest::newRow("Inserts,backspace") << keys << expectedString;
    } {
        KeyList keys;
        QStringList expectedString;

        keys << "ABC"
                // removes 'C'
                << Qt::Key_Backspace
                // inserting 'Z'
                << "Z";

        expectedString << "ABZ";
        expectedString << "AB";
        expectedString << "ABC";

        QTest::newRow("Inserts,backspace,inserts") << keys << expectedString;
    } {
        KeyList keys;
        QStringList expectedString;

        // inserting '123'
        keys << "123" << QKeySequence::MoveToStartOfLine
            // selecting '123'
             << QKeySequence::SelectEndOfLine
            // overwriting '123' with 'ABC'
             << "ABC";

        expectedString << "ABC";
        // for versions previous to 3.2 we overwrite needed two undo operations
        expectedString << "123";

        QTest::newRow("Inserts,moving,selection and overwriting") << keys << expectedString;
    }
}

void tst_qquicktextinput::undo_keypressevents()
{
    QFETCH(KeyList, keys);
    QFETCH(QStringList, expectedString);

    QString componentStr = "import QtQuick 2.0\nTextInput { focus: true }";
    QDeclarativeComponent textInputComponent(&engine);
    textInputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextInput *textInput = qobject_cast<QQuickTextInput*>(textInputComponent.create());
    QVERIFY(textInput != 0);

    QQuickCanvas canvas;
    textInput->setParentItem(canvas.rootItem());
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(QGuiApplication::activeWindow(), &canvas);

    simulateKeys(&canvas, keys);

    for (int i = 0; i < expectedString.size(); ++i) {
        QCOMPARE(textInput->text() , expectedString[i]);
        simulateKeys(&canvas, QKeySequence::Undo);
    }
    QVERIFY(textInput->text().isEmpty());
}

void tst_qquicktextinput::QTBUG_19956()
{
    QFETCH(QString, url);

    QQuickView canvas(testFileUrl(url));
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
    QUrl url = testFileUrl("qtbug-19956regexp.qml");

    QString warning = url.toString() + ":11: Unable to assign [undefined] to QRegExp";
    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning));

    QQuickView canvas(url);
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


void tst_qquicktextinput::negativeDimensions()
{
    // Verify this doesn't assert during initialization.
    QDeclarativeComponent component(&engine, testFileUrl("negativeDimensions.qml"));
    QScopedPointer<QObject> o(component.create());
    QVERIFY(o);
    QQuickTextInput *input = o->findChild<QQuickTextInput *>("input");
    QVERIFY(input);
    QCOMPARE(input->width(), qreal(-1));
    QCOMPARE(input->height(), qreal(-1));
}

QTEST_MAIN(tst_qquicktextinput)

#include "tst_qquicktextinput.moc"
