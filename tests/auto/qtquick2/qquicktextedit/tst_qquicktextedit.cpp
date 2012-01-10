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
#include "../../shared/testhttpserver.h"
#include <math.h>
#include <QFile>
#include <QTextDocument>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qdeclarativeexpression.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtGui/qguiapplication.h>
#include <private/qquicktextedit_p.h>
#include <private/qquicktextedit_p_p.h>
#include <QFontMetrics>
#include <QtQuick/QQuickView>
#include <QDir>
#include <QStyle>
#include <QInputPanel>
#include <QClipboard>
#include <QMimeData>
#include <private/qquicktextcontrol_p.h>
#include "../../shared/util.h"
#include "../../shared/platforminputcontext.h"
#include <private/qinputpanel_p.h>

#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif


Q_DECLARE_METATYPE(QQuickTextEdit::SelectionMode)
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

typedef QPair<int, QChar> Key;

class tst_qquicktextedit : public QDeclarativeDataTest

{
    Q_OBJECT
public:
    tst_qquicktextedit();

private slots:
    void text();
    void width();
    void wrap();
    void textFormat();
    void alignments();
    void alignments_data();

    // ### these tests may be trivial
    void hAlign();
    void hAlign_RightToLeft();
    void vAlign();
    void font();
    void color();
    void textMargin();
    void persistentSelection();
    void focusOnPress();
    void selection();
    void isRightToLeft_data();
    void isRightToLeft();
    void keySelection();
    void moveCursorSelection_data();
    void moveCursorSelection();
    void moveCursorSelectionSequence_data();
    void moveCursorSelectionSequence();
    void mouseSelection_data();
    void mouseSelection();
    void mouseSelectionMode_data();
    void mouseSelectionMode();
    void dragMouseSelection();
    void inputMethodHints();

    void positionAt();

    void linkActivated();

    void cursorDelegate();
    void cursorVisible();
    void delegateLoading_data();
    void delegateLoading();
    void navigation();
    void readOnly();
    void copyAndPaste();
    void canPaste();
    void canPasteEmpty();
    void textInput();
    void openInputPanel();
    void geometrySignals();
    void pastingRichText_QTBUG_14003();
    void implicitSize_data();
    void implicitSize();
    void testQtQuick11Attributes();
    void testQtQuick11Attributes_data();

    void preeditCursorRectangle();
    void inputMethodComposing();
    void cursorRectangleSize();

    void getText_data();
    void getText();
    void getFormattedText_data();
    void getFormattedText();
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

    void emptytags_QTBUG_22058();

private:
    void simulateKeys(QWindow *window, const QList<Key> &keys);
    void simulateKeys(QWindow *window, const QKeySequence &sequence);

    void simulateKey(QQuickView *, int key, Qt::KeyboardModifiers modifiers = 0);

    QStringList standard;
    QStringList richText;

    QStringList hAlignmentStrings;
    QStringList vAlignmentStrings;

    QList<Qt::Alignment> vAlignments;
    QList<Qt::Alignment> hAlignments;

    QStringList colorStrings;

    QDeclarativeEngine engine;
};

typedef QList<int> IntList;
Q_DECLARE_METATYPE(IntList)

typedef QList<Key> KeyList;
Q_DECLARE_METATYPE(KeyList)

Q_DECLARE_METATYPE(QQuickTextEdit::TextFormat)

void tst_qquicktextedit::simulateKeys(QWindow *window, const QList<Key> &keys)
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

void tst_qquicktextedit::simulateKeys(QWindow *window, const QKeySequence &sequence)
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

tst_qquicktextedit::tst_qquicktextedit()
{
    standard << "the quick brown fox jumped over the lazy dog"
             << "the quick brown fox\n jumped over the lazy dog"
             << "Hello, world!"
             << "!dlrow ,olleH";

    richText << "<i>the <b>quick</b> brown <a href=\\\"http://www.google.com\\\">fox</a> jumped over the <b>lazy</b> dog</i>"
             << "<i>the <b>quick</b> brown <a href=\\\"http://www.google.com\\\">fox</a><br>jumped over the <b>lazy</b> dog</i>";

    hAlignmentStrings << "AlignLeft"
                      << "AlignRight"
                      << "AlignHCenter";

    vAlignmentStrings << "AlignTop"
                      << "AlignBottom"
                      << "AlignVCenter";

    hAlignments << Qt::AlignLeft
                << Qt::AlignRight
                << Qt::AlignHCenter;

    vAlignments << Qt::AlignTop
                << Qt::AlignBottom
                << Qt::AlignVCenter;

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

void tst_qquicktextedit::text()
{
    {
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData("import QtQuick 2.0\nTextEdit {  text: \"\"  }", QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->text(), QString(""));
        QCOMPARE(textEditObject->length(), 0);
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->text(), standard.at(i));
        QCOMPARE(textEditObject->length(), standard.at(i).length());
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"" + richText.at(i) + "\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());

        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);

        QString expected = richText.at(i);
        expected.replace(QRegExp("\\\\(.)"),"\\1");
        QCOMPARE(textEditObject->text(), expected);
        QCOMPARE(textEditObject->length(), expected.length());
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.RichText; text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);

        QString actual = textEditObject->text();
        QString expected = standard.at(i);
        actual.remove(QRegExp(".*<body[^>]*>"));
        actual.remove(QRegExp("(<[^>]*>)+"));
        expected.remove("\n");
        QCOMPARE(actual.simplified(), expected);
        QCOMPARE(textEditObject->length(), expected.length());
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.RichText; text: \"" + richText.at(i) + "\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QString actual = textEditObject->text();
        QString expected = richText.at(i);
        actual.replace(QRegExp(".*<body[^>]*>"),"");
        actual.replace(QRegExp("(<[^>]*>)+"),"<>");
        expected.replace(QRegExp("(<[^>]*>)+"),"<>");
        QCOMPARE(actual.simplified(),expected.simplified());

        expected.replace("<>", " ");
        QCOMPARE(textEditObject->length(), expected.simplified().length());
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.AutoText; text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->text(), standard.at(i));
        QCOMPARE(textEditObject->length(), standard.at(i).length());
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.AutoText; text: \"" + richText.at(i) + "\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QString actual = textEditObject->text();
        QString expected = richText.at(i);
        actual.replace(QRegExp(".*<body[^>]*>"),"");
        actual.replace(QRegExp("(<[^>]*>)+"),"<>");
        expected.replace(QRegExp("(<[^>]*>)+"),"<>");
        QCOMPARE(actual.simplified(),expected.simplified());

        expected.replace("<>", " ");
        QCOMPARE(textEditObject->length(), expected.simplified().length());
    }
}

void tst_qquicktextedit::width()
{
    // uses Font metrics to find the width for standard and document to find the width for rich
    {
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData("import QtQuick 2.0\nTextEdit {  text: \"\" }", QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->width(), 0.0);
    }

    bool requiresUnhintedMetrics = !qmlDisableDistanceField();

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QString s = standard.at(i);
        s.replace(QLatin1Char('\n'), QChar::LineSeparator);

        QTextLayout layout(s);
        layout.setFont(textEditObject->font());
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

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->width(), qreal(metricWidth));
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QTextDocument document;
        document.setHtml(richText.at(i));
        document.setDocumentMargin(0);
        if (requiresUnhintedMetrics)
            document.setUseDesignMetrics(true);

        int documentWidth = ceil(document.idealWidth());

        QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.RichText; text: \"" + richText.at(i) + "\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->width(), qreal(documentWidth));
    }
}

void tst_qquicktextedit::wrap()
{
    // for specified width and wrap set true
    {
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData("import QtQuick 2.0\nTextEdit {  text: \"\"; wrapMode: TextEdit.WordWrap; width: 300 }", QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->width(), 300.);
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  wrapMode: TextEdit.WordWrap; width: 300; text: \"" + standard.at(i) + "\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->width(), 300.);
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  wrapMode: TextEdit.WordWrap; width: 300; text: \"" + richText.at(i) + "\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->width(), 300.);
    }

}

void tst_qquicktextedit::textFormat()
{
    {
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nTextEdit { text: \"Hello\"; textFormat: Text.RichText }", QUrl::fromLocalFile(""));
        QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit*>(textComponent.create());

        QVERIFY(textObject != 0);
        QVERIFY(textObject->textFormat() == QQuickTextEdit::RichText);
    }
    {
        QDeclarativeComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nTextEdit { text: \"<b>Hello</b>\"; textFormat: Text.PlainText }", QUrl::fromLocalFile(""));
        QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit*>(textComponent.create());

        QVERIFY(textObject != 0);
        QVERIFY(textObject->textFormat() == QQuickTextEdit::PlainText);
    }
}

void tst_qquicktextedit::alignments_data()
{
    QTest::addColumn<int>("hAlign");
    QTest::addColumn<int>("vAlign");
    QTest::addColumn<QString>("expectfile");

    QTest::newRow("LT") << int(Qt::AlignLeft) << int(Qt::AlignTop) << "alignments_lt";
    QTest::newRow("RT") << int(Qt::AlignRight) << int(Qt::AlignTop) << "alignments_rt";
    QTest::newRow("CT") << int(Qt::AlignHCenter) << int(Qt::AlignTop) << "alignments_ct";

    QTest::newRow("LB") << int(Qt::AlignLeft) << int(Qt::AlignBottom) << "alignments_lb";
    QTest::newRow("RB") << int(Qt::AlignRight) << int(Qt::AlignBottom) << "alignments_rb";
    QTest::newRow("CB") << int(Qt::AlignHCenter) << int(Qt::AlignBottom) << "alignments_cb";

    QTest::newRow("LC") << int(Qt::AlignLeft) << int(Qt::AlignVCenter) << "alignments_lc";
    QTest::newRow("RC") << int(Qt::AlignRight) << int(Qt::AlignVCenter) << "alignments_rc";
    QTest::newRow("CC") << int(Qt::AlignHCenter) << int(Qt::AlignVCenter) << "alignments_cc";
}


void tst_qquicktextedit::alignments()
{
    QSKIP("Image comparison of text is almost guaranteed to fail during development");

    QFETCH(int, hAlign);
    QFETCH(int, vAlign);
    QFETCH(QString, expectfile);

    QQuickView canvas(testFileUrl("alignments.qml"));

    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());

    QObject *ob = canvas.rootObject();
    QVERIFY(ob != 0);
    ob->setProperty("horizontalAlignment",hAlign);
    ob->setProperty("verticalAlignment",vAlign);
    QTRY_COMPARE(ob->property("running").toBool(),false);
    QImage actual = canvas.grabFrameBuffer();

    expectfile = createExpectedFileIfNotFound(expectfile, actual);

    QImage expect(expectfile);

    QCOMPARE(actual,expect);
}


//the alignment tests may be trivial o.oa
void tst_qquicktextedit::hAlign()
{
    //test one align each, and then test if two align fails.

    for (int i = 0; i < standard.size(); i++)
    {
        for (int j=0; j < hAlignmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nTextEdit {  horizontalAlignment: \"" + hAlignmentStrings.at(j) + "\"; text: \"" + standard.at(i) + "\" }";
            QDeclarativeComponent texteditComponent(&engine);
            texteditComponent.setData(componentStr.toLatin1(), QUrl());
            QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

            QVERIFY(textEditObject != 0);
            QCOMPARE((int)textEditObject->hAlign(), (int)hAlignments.at(j));
        }
    }

    for (int i = 0; i < richText.size(); i++)
    {
        for (int j=0; j < hAlignmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nTextEdit {  horizontalAlignment: \"" + hAlignmentStrings.at(j) + "\"; text: \"" + richText.at(i) + "\" }";
            QDeclarativeComponent texteditComponent(&engine);
            texteditComponent.setData(componentStr.toLatin1(), QUrl());
            QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

            QVERIFY(textEditObject != 0);
            QCOMPARE((int)textEditObject->hAlign(), (int)hAlignments.at(j));
        }
    }

}

void tst_qquicktextedit::hAlign_RightToLeft()
{
    QQuickView canvas(testFileUrl("horizontalAlignment_RightToLeft.qml"));
    QQuickTextEdit *textEdit = canvas.rootObject()->findChild<QQuickTextEdit*>("text");
    QVERIFY(textEdit != 0);
    canvas.show();

    const QString rtlText = textEdit->text();

    // implicit alignment should follow the reading direction of text
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > canvas.width()/2);

    // explicitly left aligned
    textEdit->setHAlign(QQuickTextEdit::AlignLeft);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignLeft);
    QVERIFY(textEdit->positionToRectangle(0).x() < canvas.width()/2);

    // explicitly right aligned
    textEdit->setHAlign(QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > canvas.width()/2);

    QString textString = textEdit->text();
    textEdit->setText(QString("<i>") + textString + QString("</i>"));
    textEdit->resetHAlign();

    // implicitly aligned rich text should follow the reading direction of RTL text
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->effectiveHAlign(), textEdit->hAlign());
    QVERIFY(textEdit->positionToRectangle(0).x() > canvas.width()/2);

    // explicitly left aligned rich text
    textEdit->setHAlign(QQuickTextEdit::AlignLeft);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignLeft);
    QCOMPARE(textEdit->effectiveHAlign(), textEdit->hAlign());
    QVERIFY(textEdit->positionToRectangle(0).x() < canvas.width()/2);

    // explicitly right aligned rich text
    textEdit->setHAlign(QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->effectiveHAlign(), textEdit->hAlign());
    QVERIFY(textEdit->positionToRectangle(0).x() > canvas.width()/2);

    textEdit->setText(textString);

    // explicitly center aligned
    textEdit->setHAlign(QQuickTextEdit::AlignHCenter);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignHCenter);
    QVERIFY(textEdit->positionToRectangle(0).x() > canvas.width()/2);

    // reseted alignment should go back to following the text reading direction
    textEdit->resetHAlign();
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > canvas.width()/2);

    // mirror the text item
    QQuickItemPrivate::get(textEdit)->setLayoutMirror(true);

    // mirrored implicit alignment should continue to follow the reading direction of the text
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->effectiveHAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > canvas.width()/2);

    // mirrored explicitly right aligned behaves as left aligned
    textEdit->setHAlign(QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->effectiveHAlign(), QQuickTextEdit::AlignLeft);
    QVERIFY(textEdit->positionToRectangle(0).x() < canvas.width()/2);

    // mirrored explicitly left aligned behaves as right aligned
    textEdit->setHAlign(QQuickTextEdit::AlignLeft);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignLeft);
    QCOMPARE(textEdit->effectiveHAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > canvas.width()/2);

    // disable mirroring
    QQuickItemPrivate::get(textEdit)->setLayoutMirror(false);
    textEdit->resetHAlign();

    // English text should be implicitly left aligned
    textEdit->setText("Hello world!");
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignLeft);
    QVERIFY(textEdit->positionToRectangle(0).x() < canvas.width()/2);

    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());

    textEdit->setText(QString());
    { QInputMethodEvent ev(rtlText, QList<QInputMethodEvent::Attribute>()); QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &ev); }
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    { QInputMethodEvent ev("Hello world!", QList<QInputMethodEvent::Attribute>()); QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &ev); }
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignLeft);

    // Clear pre-edit text.  TextEdit should maybe do this itself on setText, but that may be
    // redundant as an actual input method may take care of it.
    { QInputMethodEvent ev; QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &ev); }

    // empty text with implicit alignment follows the system locale-based
    // keyboard input direction from QGuiApplication::keyboardInputDirection
    textEdit->setText("");
    QCOMPARE(textEdit->hAlign(), QGuiApplication::keyboardInputDirection() == Qt::LeftToRight ?
                                  QQuickTextEdit::AlignLeft : QQuickTextEdit::AlignRight);
    if (QGuiApplication::keyboardInputDirection() == Qt::LeftToRight)
        QVERIFY(textEdit->positionToRectangle(0).x() < canvas.width()/2);
    else
        QVERIFY(textEdit->positionToRectangle(0).x() > canvas.width()/2);
    textEdit->setHAlign(QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > canvas.width()/2);

    // alignment of TextEdit with no text set to it
    QString componentStr = "import QtQuick 2.0\nTextEdit {}";
    QDeclarativeComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit*>(textComponent.create());
    QCOMPARE(textObject->hAlign(), QGuiApplication::keyboardInputDirection() == Qt::LeftToRight ?
                                  QQuickTextEdit::AlignLeft : QQuickTextEdit::AlignRight);
    delete textObject;
}

void tst_qquicktextedit::vAlign()
{
    //test one align each, and then test if two align fails.

    for (int i = 0; i < standard.size(); i++)
    {
        for (int j=0; j < vAlignmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nTextEdit {  verticalAlignment: \"" + vAlignmentStrings.at(j) + "\"; text: \"" + standard.at(i) + "\" }";
            QDeclarativeComponent texteditComponent(&engine);
            texteditComponent.setData(componentStr.toLatin1(), QUrl());
            QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

            QVERIFY(textEditObject != 0);
            QCOMPARE((int)textEditObject->vAlign(), (int)vAlignments.at(j));
        }
    }

    for (int i = 0; i < richText.size(); i++)
    {
        for (int j=0; j < vAlignmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nTextEdit {  verticalAlignment: \"" + vAlignmentStrings.at(j) + "\"; text: \"" + richText.at(i) + "\" }";
            QDeclarativeComponent texteditComponent(&engine);
            texteditComponent.setData(componentStr.toLatin1(), QUrl());
            QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

            QVERIFY(textEditObject != 0);
            QCOMPARE((int)textEditObject->vAlign(), (int)vAlignments.at(j));
        }
    }

}

void tst_qquicktextedit::font()
{
    //test size, then bold, then italic, then family
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  font.pointSize: 40; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->font().pointSize(), 40);
        QCOMPARE(textEditObject->font().bold(), false);
        QCOMPARE(textEditObject->font().italic(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  font.bold: true; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->font().bold(), true);
        QCOMPARE(textEditObject->font().italic(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  font.italic: true; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->font().italic(), true);
        QCOMPARE(textEditObject->font().bold(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  font.family: \"Helvetica\"; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->font().family(), QString("Helvetica"));
        QCOMPARE(textEditObject->font().bold(), false);
        QCOMPARE(textEditObject->font().italic(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  font.family: \"\"; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->font().family(), QString(""));
    }
}

void tst_qquicktextedit::color()
{
    //test initial color
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QQuickTextEditPrivate *textEditPrivate = static_cast<QQuickTextEditPrivate*>(QQuickItemPrivate::get(textEditObject));

        QVERIFY(textEditObject);
        QVERIFY(textEditPrivate);
        QVERIFY(textEditPrivate->control);

        QPalette pal = textEditPrivate->control->palette();
        QCOMPARE(textEditPrivate->color, QColor("black"));
        QCOMPARE(textEditPrivate->color, pal.color(QPalette::Text));
    }
    //test normal
    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  color: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        //qDebug() << "textEditObject: " << textEditObject->color() << "vs. " << QColor(colorStrings.at(i));
        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->color(), QColor(colorStrings.at(i)));
    }

    //test selection
    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  selectionColor: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->selectionColor(), QColor(colorStrings.at(i)));
    }

    //test selected text
    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  selectedTextColor: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->selectedTextColor(), QColor(colorStrings.at(i)));
    }

    {
        QString colorStr = "#AA001234";
        QColor testColor("#001234");
        testColor.setAlpha(170);

        QString componentStr = "import QtQuick 2.0\nTextEdit {  color: \"" + colorStr + "\"; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->color(), testColor);
    }
}

void tst_qquicktextedit::textMargin()
{
    for (qreal i=0; i<=10; i+=0.3) {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  textMargin: " + QString::number(i) + "; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->textMargin(), i);
    }
}

void tst_qquicktextedit::persistentSelection()
{
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  persistentSelection: true; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->persistentSelection(), true);
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  persistentSelection: false; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->persistentSelection(), false);
    }
}

void tst_qquicktextedit::focusOnPress()
{
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  activeFocusOnPress: true; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->focusOnPress(), true);
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  activeFocusOnPress: false; text: \"Hello World\" }";
        QDeclarativeComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        QVERIFY(textEditObject != 0);
        QCOMPARE(textEditObject->focusOnPress(), false);
    }
}

void tst_qquicktextedit::selection()
{
    QString testStr = standard[0];//TODO: What should happen for multiline/rich text?
    QString componentStr = "import QtQuick 2.0\nTextEdit {  text: \""+ testStr +"\"; }";
    QDeclarativeComponent texteditComponent(&engine);
    texteditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
    QVERIFY(textEditObject != 0);


    //Test selection follows cursor
    for (int i=0; i<= testStr.size(); i++) {
        textEditObject->setCursorPosition(i);
        QCOMPARE(textEditObject->cursorPosition(), i);
        QCOMPARE(textEditObject->selectionStart(), i);
        QCOMPARE(textEditObject->selectionEnd(), i);
        QVERIFY(textEditObject->selectedText().isNull());
    }

    textEditObject->setCursorPosition(0);
    QVERIFY(textEditObject->cursorPosition() == 0);
    QVERIFY(textEditObject->selectionStart() == 0);
    QVERIFY(textEditObject->selectionEnd() == 0);
    QVERIFY(textEditObject->selectedText().isNull());

    // Verify invalid positions are ignored.
    textEditObject->setCursorPosition(-1);
    QVERIFY(textEditObject->cursorPosition() == 0);
    QVERIFY(textEditObject->selectionStart() == 0);
    QVERIFY(textEditObject->selectionEnd() == 0);
    QVERIFY(textEditObject->selectedText().isNull());

    textEditObject->setCursorPosition(textEditObject->text().count()+1);
    QVERIFY(textEditObject->cursorPosition() == 0);
    QVERIFY(textEditObject->selectionStart() == 0);
    QVERIFY(textEditObject->selectionEnd() == 0);
    QVERIFY(textEditObject->selectedText().isNull());

    //Test selection
    for (int i=0; i<= testStr.size(); i++) {
        textEditObject->select(0,i);
        QCOMPARE(testStr.mid(0,i), textEditObject->selectedText());
    }
    for (int i=0; i<= testStr.size(); i++) {
        textEditObject->select(i,testStr.size());
        QCOMPARE(testStr.mid(i,testStr.size()-i), textEditObject->selectedText());
    }

    textEditObject->setCursorPosition(0);
    QVERIFY(textEditObject->cursorPosition() == 0);
    QVERIFY(textEditObject->selectionStart() == 0);
    QVERIFY(textEditObject->selectionEnd() == 0);
    QVERIFY(textEditObject->selectedText().isNull());

    //Test Error Ignoring behaviour
    textEditObject->setCursorPosition(0);
    QVERIFY(textEditObject->selectedText().isNull());
    textEditObject->select(-10,0);
    QVERIFY(textEditObject->selectedText().isNull());
    textEditObject->select(100,101);
    QVERIFY(textEditObject->selectedText().isNull());
    textEditObject->select(0,-10);
    QVERIFY(textEditObject->selectedText().isNull());
    textEditObject->select(0,100);
    QVERIFY(textEditObject->selectedText().isNull());
    textEditObject->select(0,10);
    QVERIFY(textEditObject->selectedText().size() == 10);
    textEditObject->select(-10,0);
    QVERIFY(textEditObject->selectedText().size() == 10);
    textEditObject->select(100,101);
    QVERIFY(textEditObject->selectedText().size() == 10);
    textEditObject->select(0,-10);
    QVERIFY(textEditObject->selectedText().size() == 10);
    textEditObject->select(0,100);
    QVERIFY(textEditObject->selectedText().size() == 10);

    textEditObject->deselect();
    QVERIFY(textEditObject->selectedText().isNull());
    textEditObject->select(0,10);
    QVERIFY(textEditObject->selectedText().size() == 10);
    textEditObject->deselect();
    QVERIFY(textEditObject->selectedText().isNull());
}

void tst_qquicktextedit::isRightToLeft_data()
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

void tst_qquicktextedit::isRightToLeft()
{
    QFETCH(QString, text);
    QFETCH(bool, emptyString);
    QFETCH(bool, firstCharacter);
    QFETCH(bool, lastCharacter);
    QFETCH(bool, middleCharacter);
    QFETCH(bool, startString);
    QFETCH(bool, midString);
    QFETCH(bool, endString);

    QQuickTextEdit textEdit;
    textEdit.setText(text);

    // first test that the right string is delivered to the QString::isRightToLeft()
    QCOMPARE(textEdit.isRightToLeft(0,0), text.mid(0,0).isRightToLeft());
    QCOMPARE(textEdit.isRightToLeft(0,1), text.mid(0,1).isRightToLeft());
    QCOMPARE(textEdit.isRightToLeft(text.count()-2, text.count()-1), text.mid(text.count()-2, text.count()-1).isRightToLeft());
    QCOMPARE(textEdit.isRightToLeft(text.count()/2, text.count()/2 + 1), text.mid(text.count()/2, text.count()/2 + 1).isRightToLeft());
    QCOMPARE(textEdit.isRightToLeft(0,text.count()/4), text.mid(0,text.count()/4).isRightToLeft());
    QCOMPARE(textEdit.isRightToLeft(text.count()/4,3*text.count()/4), text.mid(text.count()/4,3*text.count()/4).isRightToLeft());
    if (text.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML TextEdit: isRightToLeft(start, end) called with the end property being smaller than the start.");
    QCOMPARE(textEdit.isRightToLeft(3*text.count()/4,text.count()-1), text.mid(3*text.count()/4,text.count()-1).isRightToLeft());

    // then test that the feature actually works
    QCOMPARE(textEdit.isRightToLeft(0,0), emptyString);
    QCOMPARE(textEdit.isRightToLeft(0,1), firstCharacter);
    QCOMPARE(textEdit.isRightToLeft(text.count()-2, text.count()-1), lastCharacter);
    QCOMPARE(textEdit.isRightToLeft(text.count()/2, text.count()/2 + 1), middleCharacter);
    QCOMPARE(textEdit.isRightToLeft(0,text.count()/4), startString);
    QCOMPARE(textEdit.isRightToLeft(text.count()/4,3*text.count()/4), midString);
    if (text.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML TextEdit: isRightToLeft(start, end) called with the end property being smaller than the start.");
    QCOMPARE(textEdit.isRightToLeft(3*text.count()/4,text.count()-1), endString);
}

void tst_qquicktextedit::keySelection()
{
    QQuickView canvas(testFileUrl("navigation.qml"));
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());
    canvas.requestActivateWindow();

    QVERIFY(canvas.rootObject() != 0);

    QQuickTextEdit *input = qobject_cast<QQuickTextEdit *>(qvariant_cast<QObject *>(canvas.rootObject()->property("myInput")));

    QVERIFY(input != 0);
    QTRY_VERIFY(input->hasActiveFocus() == true);

    QSignalSpy spy(input, SIGNAL(selectionChanged()));

    simulateKey(&canvas, Qt::Key_Right, Qt::ShiftModifier);
    QVERIFY(input->hasActiveFocus() == true);
    QCOMPARE(input->selectedText(), QString("a"));
    QCOMPARE(spy.count(), 1);
    simulateKey(&canvas, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus() == true);
    QCOMPARE(input->selectedText(), QString());
    QCOMPARE(spy.count(), 2);
    simulateKey(&canvas, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus() == false);
    QCOMPARE(input->selectedText(), QString());
    QCOMPARE(spy.count(), 2);

    simulateKey(&canvas, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus() == true);
    QCOMPARE(spy.count(), 2);
    simulateKey(&canvas, Qt::Key_Left, Qt::ShiftModifier);
    QVERIFY(input->hasActiveFocus() == true);
    QCOMPARE(input->selectedText(), QString("a"));
    QCOMPARE(spy.count(), 3);
    simulateKey(&canvas, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus() == true);
    QCOMPARE(input->selectedText(), QString());
    QCOMPARE(spy.count(), 4);
    simulateKey(&canvas, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus() == false);
    QCOMPARE(input->selectedText(), QString());
    QCOMPARE(spy.count(), 4);
}

void tst_qquicktextedit::moveCursorSelection_data()
{
    QTest::addColumn<QString>("testStr");
    QTest::addColumn<int>("cursorPosition");
    QTest::addColumn<int>("movePosition");
    QTest::addColumn<QQuickTextEdit::SelectionMode>("mode");
    QTest::addColumn<int>("selectionStart");
    QTest::addColumn<int>("selectionEnd");
    QTest::addColumn<bool>("reversible");

    QTest::newRow("(t)he|characters")
            << standard[0] << 0 << 1 << QQuickTextEdit::SelectCharacters << 0 << 1 << true;
    QTest::newRow("do(g)|characters")
            << standard[0] << 43 << 44 << QQuickTextEdit::SelectCharacters << 43 << 44 << true;
    QTest::newRow("jum(p)ed|characters")
            << standard[0] << 23 << 24 << QQuickTextEdit::SelectCharacters << 23 << 24 << true;
    QTest::newRow("jumped( )over|characters")
            << standard[0] << 26 << 27 << QQuickTextEdit::SelectCharacters << 26 << 27 << true;
    QTest::newRow("(the )|characters")
            << standard[0] << 0 << 4 << QQuickTextEdit::SelectCharacters << 0 << 4 << true;
    QTest::newRow("( dog)|characters")
            << standard[0] << 40 << 44 << QQuickTextEdit::SelectCharacters << 40 << 44 << true;
    QTest::newRow("( jumped )|characters")
            << standard[0] << 19 << 27 << QQuickTextEdit::SelectCharacters << 19 << 27 << true;
    QTest::newRow("th(e qu)ick|characters")
            << standard[0] << 2 << 6 << QQuickTextEdit::SelectCharacters << 2 << 6 << true;
    QTest::newRow("la(zy d)og|characters")
            << standard[0] << 38 << 42 << QQuickTextEdit::SelectCharacters << 38 << 42 << true;
    QTest::newRow("jum(ped ov)er|characters")
            << standard[0] << 23 << 29 << QQuickTextEdit::SelectCharacters << 23 << 29 << true;
    QTest::newRow("()the|characters")
            << standard[0] << 0 << 0 << QQuickTextEdit::SelectCharacters << 0 << 0 << true;
    QTest::newRow("dog()|characters")
            << standard[0] << 44 << 44 << QQuickTextEdit::SelectCharacters << 44 << 44 << true;
    QTest::newRow("jum()ped|characters")
            << standard[0] << 23 << 23 << QQuickTextEdit::SelectCharacters << 23 << 23 << true;

    QTest::newRow("<(t)he>|words")
            << standard[0] << 0 << 1 << QQuickTextEdit::SelectWords << 0 << 3 << true;
    QTest::newRow("<do(g)>|words")
            << standard[0] << 43 << 44 << QQuickTextEdit::SelectWords << 41 << 44 << true;
    QTest::newRow("<jum(p)ed>|words")
            << standard[0] << 23 << 24 << QQuickTextEdit::SelectWords << 20 << 26 << true;
    QTest::newRow("<jumped( )>over|words")
            << standard[0] << 26 << 27 << QQuickTextEdit::SelectWords << 20 << 27 << false;
    QTest::newRow("jumped<( )over>|words,reversed")
            << standard[0] << 27 << 26 << QQuickTextEdit::SelectWords << 26 << 31 << false;
    QTest::newRow("<(the )>quick|words")
            << standard[0] << 0 << 4 << QQuickTextEdit::SelectWords << 0 << 4 << false;
    QTest::newRow("<(the )quick>|words,reversed")
            << standard[0] << 4 << 0 << QQuickTextEdit::SelectWords << 0 << 9 << false;
    QTest::newRow("<lazy( dog)>|words")
            << standard[0] << 40 << 44 << QQuickTextEdit::SelectWords << 36 << 44 << false;
    QTest::newRow("lazy<( dog)>|words,reversed")
            << standard[0] << 44 << 40 << QQuickTextEdit::SelectWords << 40 << 44 << false;
    QTest::newRow("<fox( jumped )>over|words")
            << standard[0] << 19 << 27 << QQuickTextEdit::SelectWords << 16 << 27 << false;
    QTest::newRow("fox<( jumped )over>|words,reversed")
            << standard[0] << 27 << 19 << QQuickTextEdit::SelectWords << 19 << 31 << false;
    QTest::newRow("<th(e qu)ick>|words")
            << standard[0] << 2 << 6 << QQuickTextEdit::SelectWords << 0 << 9 << true;
    QTest::newRow("<la(zy d)og|words>")
            << standard[0] << 38 << 42 << QQuickTextEdit::SelectWords << 36 << 44 << true;
    QTest::newRow("<jum(ped ov)er>|words")
            << standard[0] << 23 << 29 << QQuickTextEdit::SelectWords << 20 << 31 << true;
    QTest::newRow("<()>the|words")
            << standard[0] << 0 << 0 << QQuickTextEdit::SelectWords << 0 << 0 << true;
    QTest::newRow("dog<()>|words")
            << standard[0] << 44 << 44 << QQuickTextEdit::SelectWords << 44 << 44 << true;
    QTest::newRow("jum<()>ped|words")
            << standard[0] << 23 << 23 << QQuickTextEdit::SelectWords << 23 << 23 << true;

    QTest::newRow("Hello<(,)> |words")
            << standard[2] << 5 << 6 << QQuickTextEdit::SelectWords << 5 << 6 << true;
    QTest::newRow("Hello<(, )>world|words")
            << standard[2] << 5 << 7 << QQuickTextEdit::SelectWords << 5 << 7 << false;
    QTest::newRow("Hello<(, )world>|words,reversed")
            << standard[2] << 7 << 5 << QQuickTextEdit::SelectWords << 5 << 12 << false;
    QTest::newRow("<Hel(lo, )>world|words")
            << standard[2] << 3 << 7 << QQuickTextEdit::SelectWords << 0 << 7 << false;
    QTest::newRow("<Hel(lo, )world>|words,reversed")
            << standard[2] << 7 << 3 << QQuickTextEdit::SelectWords << 0 << 12 << false;
    QTest::newRow("<Hel(lo)>,|words")
            << standard[2] << 3 << 5 << QQuickTextEdit::SelectWords << 0 << 5 << true;
    QTest::newRow("Hello<()>,|words")
            << standard[2] << 5 << 5 << QQuickTextEdit::SelectWords << 5 << 5 << true;
    QTest::newRow("Hello,<()>|words")
            << standard[2] << 6 << 6 << QQuickTextEdit::SelectWords << 6 << 6 << true;
    QTest::newRow("Hello<,( )>world|words")
            << standard[2] << 6 << 7 << QQuickTextEdit::SelectWords << 5 << 7 << false;
    QTest::newRow("Hello,<( )world>|words,reversed")
            << standard[2] << 7 << 6 << QQuickTextEdit::SelectWords << 6 << 12 << false;
    QTest::newRow("Hello<,( world)>|words")
            << standard[2] << 6 << 12 << QQuickTextEdit::SelectWords << 5 << 12 << false;
    QTest::newRow("Hello,<( world)>|words,reversed")
            << standard[2] << 12 << 6 << QQuickTextEdit::SelectWords << 6 << 12 << false;
    QTest::newRow("Hello<,( world!)>|words")
            << standard[2] << 6 << 13 << QQuickTextEdit::SelectWords << 5 << 13 << false;
    QTest::newRow("Hello,<( world!)>|words,reversed")
            << standard[2] << 13 << 6 << QQuickTextEdit::SelectWords << 6 << 13 << false;
    QTest::newRow("Hello<(, world!)>|words")
            << standard[2] << 5 << 13 << QQuickTextEdit::SelectWords << 5 << 13 << true;
    QTest::newRow("world<(!)>|words")
            << standard[2] << 12 << 13 << QQuickTextEdit::SelectWords << 12 << 13 << true;
    QTest::newRow("world!<()>)|words")
            << standard[2] << 13 << 13 << QQuickTextEdit::SelectWords << 13 << 13 << true;
    QTest::newRow("world<()>!)|words")
            << standard[2] << 12 << 12 << QQuickTextEdit::SelectWords << 12 << 12 << true;

    QTest::newRow("<(,)>olleH |words")
            << standard[3] << 7 << 8 << QQuickTextEdit::SelectWords << 7 << 8 << true;
    QTest::newRow("<dlrow( ,)>olleH|words")
            << standard[3] << 6 << 8 << QQuickTextEdit::SelectWords << 1 << 8 << false;
    QTest::newRow("dlrow<( ,)>olleH|words,reversed")
            << standard[3] << 8 << 6 << QQuickTextEdit::SelectWords << 6 << 8 << false;
    QTest::newRow("<dlrow( ,ol)leH>|words")
            << standard[3] << 6 << 10 << QQuickTextEdit::SelectWords << 1 << 13 << false;
    QTest::newRow("dlrow<( ,ol)leH>|words,reversed")
            << standard[3] << 10 << 6 << QQuickTextEdit::SelectWords << 6 << 13 << false;
    QTest::newRow(",<(ol)leH>,|words")
            << standard[3] << 8 << 10 << QQuickTextEdit::SelectWords << 8 << 13 << true;
    QTest::newRow(",<()>olleH|words")
            << standard[3] << 8 << 8 << QQuickTextEdit::SelectWords << 8 << 8 << true;
    QTest::newRow("<()>,olleH|words")
            << standard[3] << 7 << 7 << QQuickTextEdit::SelectWords << 7 << 7 << true;
    QTest::newRow("<dlrow( )>,olleH|words")
            << standard[3] << 6 << 7 << QQuickTextEdit::SelectWords << 1 << 7 << false;
    QTest::newRow("dlrow<( ),>olleH|words,reversed")
            << standard[3] << 7 << 6 << QQuickTextEdit::SelectWords << 6 << 8 << false;
    QTest::newRow("<(dlrow )>,olleH|words")
            << standard[3] << 1 << 7 << QQuickTextEdit::SelectWords << 1 << 7 << false;
    QTest::newRow("<(dlrow ),>olleH|words,reversed")
            << standard[3] << 7 << 1 << QQuickTextEdit::SelectWords << 1 << 8 << false;
    QTest::newRow("<(!dlrow )>,olleH|words")
            << standard[3] << 0 << 7 << QQuickTextEdit::SelectWords << 0 << 7 << false;
    QTest::newRow("<(!dlrow ),>olleH|words,reversed")
            << standard[3] << 7 << 0 << QQuickTextEdit::SelectWords << 0 << 8 << false;
    QTest::newRow("(!dlrow ,)olleH|words")
            << standard[3] << 0 << 8 << QQuickTextEdit::SelectWords << 0 << 8 << true;
    QTest::newRow("<(!)>dlrow|words")
            << standard[3] << 0 << 1 << QQuickTextEdit::SelectWords << 0 << 1 << true;
    QTest::newRow("<()>!dlrow|words")
            << standard[3] << 0 << 0 << QQuickTextEdit::SelectWords << 0 << 0 << true;
    QTest::newRow("!<()>dlrow|words")
            << standard[3] << 1 << 1 << QQuickTextEdit::SelectWords << 1 << 1 << true;
}

void tst_qquicktextedit::moveCursorSelection()
{
    QFETCH(QString, testStr);
    QFETCH(int, cursorPosition);
    QFETCH(int, movePosition);
    QFETCH(QQuickTextEdit::SelectionMode, mode);
    QFETCH(int, selectionStart);
    QFETCH(int, selectionEnd);
    QFETCH(bool, reversible);

    QString componentStr = "import QtQuick 2.0\nTextEdit {  text: \""+ testStr +"\"; }";
    QDeclarativeComponent textinputComponent(&engine);
    textinputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *texteditObject = qobject_cast<QQuickTextEdit*>(textinputComponent.create());
    QVERIFY(texteditObject != 0);

    texteditObject->setCursorPosition(cursorPosition);
    texteditObject->moveCursorSelection(movePosition, mode);

    QCOMPARE(texteditObject->selectedText(), testStr.mid(selectionStart, selectionEnd - selectionStart));
    QCOMPARE(texteditObject->selectionStart(), selectionStart);
    QCOMPARE(texteditObject->selectionEnd(), selectionEnd);

    if (reversible) {
        texteditObject->setCursorPosition(movePosition);
        texteditObject->moveCursorSelection(cursorPosition, mode);

        QCOMPARE(texteditObject->selectedText(), testStr.mid(selectionStart, selectionEnd - selectionStart));
        QCOMPARE(texteditObject->selectionStart(), selectionStart);
        QCOMPARE(texteditObject->selectionEnd(), selectionEnd);
    }
}

void tst_qquicktextedit::moveCursorSelectionSequence_data()
{
    QTest::addColumn<QString>("testStr");
    QTest::addColumn<int>("cursorPosition");
    QTest::addColumn<int>("movePosition1");
    QTest::addColumn<int>("movePosition2");
    QTest::addColumn<int>("selection1Start");
    QTest::addColumn<int>("selection1End");
    QTest::addColumn<int>("selection2Start");
    QTest::addColumn<int>("selection2End");

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
    QTest::newRow("the quick<(^ {^bro)wn>} fox|rtl")
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
}

void tst_qquicktextedit::moveCursorSelectionSequence()
{
    QFETCH(QString, testStr);
    QFETCH(int, cursorPosition);
    QFETCH(int, movePosition1);
    QFETCH(int, movePosition2);
    QFETCH(int, selection1Start);
    QFETCH(int, selection1End);
    QFETCH(int, selection2Start);
    QFETCH(int, selection2End);

    QString componentStr = "import QtQuick 2.0\nTextEdit {  text: \""+ testStr +"\"; }";
    QDeclarativeComponent texteditComponent(&engine);
    texteditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *texteditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
    QVERIFY(texteditObject != 0);

    texteditObject->setCursorPosition(cursorPosition);

    texteditObject->moveCursorSelection(movePosition1, QQuickTextEdit::SelectWords);
    QCOMPARE(texteditObject->selectedText(), testStr.mid(selection1Start, selection1End - selection1Start));
    QCOMPARE(texteditObject->selectionStart(), selection1Start);
    QCOMPARE(texteditObject->selectionEnd(), selection1End);

    texteditObject->moveCursorSelection(movePosition2, QQuickTextEdit::SelectWords);
    QCOMPARE(texteditObject->selectedText(), testStr.mid(selection2Start, selection2End - selection2Start));
    QCOMPARE(texteditObject->selectionStart(), selection2Start);
    QCOMPARE(texteditObject->selectionEnd(), selection2End);
}


void tst_qquicktextedit::mouseSelection_data()
{
    QTest::addColumn<QString>("qmlfile");
    QTest::addColumn<int>("from");
    QTest::addColumn<int>("to");
    QTest::addColumn<QString>("selectedText");

    // import installed
    QTest::newRow("on") << testFile("mouseselection_true.qml") << 4 << 9 << "45678";
    QTest::newRow("off") << testFile("mouseselection_false.qml") << 4 << 9 << QString();
    QTest::newRow("default") << testFile("mouseselection_default.qml") << 4 << 9 << QString();
    QTest::newRow("off word selection") << testFile("mouseselection_false_words.qml") << 4 << 9 << QString();
    QTest::newRow("on word selection (4,9)") << testFile("mouseselection_true_words.qml") << 4 << 9 << "0123456789";
    QTest::newRow("on word selection (2,13)") << testFile("mouseselection_true_words.qml") << 2 << 13 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QTest::newRow("on word selection (2,30)") << testFile("mouseselection_true_words.qml") << 2 << 30 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QTest::newRow("on word selection (9,13)") << testFile("mouseselection_true_words.qml") << 9 << 13 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QTest::newRow("on word selection (9,30)") << testFile("mouseselection_true_words.qml") << 9 << 30 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QTest::newRow("on word selection (13,2)") << testFile("mouseselection_true_words.qml") << 13 << 2 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QTest::newRow("on word selection (20,2)") << testFile("mouseselection_true_words.qml") << 20 << 2 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QTest::newRow("on word selection (12,9)") << testFile("mouseselection_true_words.qml") << 12 << 9 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    QTest::newRow("on word selection (30,9)") << testFile("mouseselection_true_words.qml") << 30 << 9 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
}

void tst_qquicktextedit::mouseSelection()
{
    QFETCH(QString, qmlfile);
    QFETCH(int, from);
    QFETCH(int, to);
    QFETCH(QString, selectedText);

    QQuickView canvas(QUrl::fromLocalFile(qmlfile));

    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());

    QVERIFY(canvas.rootObject() != 0);
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit *>(canvas.rootObject());
    QVERIFY(textEditObject != 0);

    // press-and-drag-and-release from x1 to x2
    QPoint p1 = textEditObject->positionToRectangle(from).center().toPoint();
    QPoint p2 = textEditObject->positionToRectangle(to).center().toPoint();
    QTest::mousePress(&canvas, Qt::LeftButton, 0, p1);
    QTest::mouseMove(&canvas, p2);
    QTest::mouseRelease(&canvas, Qt::LeftButton, 0, p2);
    QTest::qWait(50);
    QTRY_COMPARE(textEditObject->selectedText(), selectedText);

    // Clicking and shift to clicking between the same points should select the same text.
    textEditObject->setCursorPosition(0);
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::NoModifier, p1);
    QTest::mouseClick(&canvas, Qt::LeftButton, Qt::ShiftModifier, p2);
    QTest::qWait(50);
    QTRY_COMPARE(textEditObject->selectedText(), selectedText);
}

void tst_qquicktextedit::dragMouseSelection()
{
    QString qmlfile = testFile("mouseselection_true.qml");

    QQuickView canvas(QUrl::fromLocalFile(qmlfile));

    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(&canvas, qGuiApp->focusWindow());

    QVERIFY(canvas.rootObject() != 0);
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit *>(canvas.rootObject());
    QVERIFY(textEditObject != 0);

    // press-and-drag-and-release from x1 to x2
    int x1 = 10;
    int x2 = 70;
    int y = textEditObject->height()/2;
    QTest::mousePress(&canvas, Qt::LeftButton, 0, QPoint(x1,y));
    QTest::mouseMove(&canvas, QPoint(x2, y));
    QTest::mouseRelease(&canvas, Qt::LeftButton, 0, QPoint(x2,y));
    QTest::qWait(300);
    QString str1;
    QTRY_VERIFY((str1 = textEditObject->selectedText()).length() > 3);

    // press and drag the current selection.
    x1 = 40;
    x2 = 100;
    QTest::mousePress(&canvas, Qt::LeftButton, 0, QPoint(x1,y));
    QTest::mouseMove(&canvas, QPoint(x2, y));
    QTest::mouseRelease(&canvas, Qt::LeftButton, 0, QPoint(x2,y));
    QTest::qWait(300);
    QString str2;
    QTRY_VERIFY((str2 = textEditObject->selectedText()).length() > 3);

    QVERIFY(str1 != str2); // Verify the second press and drag is a new selection and not the first moved.
}

void tst_qquicktextedit::mouseSelectionMode_data()
{
    QTest::addColumn<QString>("qmlfile");
    QTest::addColumn<bool>("selectWords");

    // import installed
    QTest::newRow("SelectWords") << testFile("mouseselectionmode_words.qml") << true;
    QTest::newRow("SelectCharacters") << testFile("mouseselectionmode_characters.qml") << false;
    QTest::newRow("default") << testFile("mouseselectionmode_default.qml") << false;
}

void tst_qquicktextedit::mouseSelectionMode()
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
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit *>(canvas.rootObject());
    QVERIFY(textEditObject != 0);

    // press-and-drag-and-release from x1 to x2
    int x1 = 10;
    int x2 = 70;
    int y = textEditObject->height()/2;
    QTest::mousePress(&canvas, Qt::LeftButton, 0, QPoint(x1,y));
    QTest::mouseMove(&canvas, QPoint(x2, y));
    //QTest::mouseMove(canvas, QPoint(x2,y)); // doesn't work
//    QMouseEvent mv(QEvent::MouseMove, QPoint(x2,y), Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
//    QGuiApplication::sendEvent(&canvas, &mv);
    QTest::mouseRelease(&canvas, Qt::LeftButton, 0, QPoint(x2,y));
    QString str = textEditObject->selectedText();
    if (selectWords) {
        QTRY_COMPARE(textEditObject->selectedText(), text);
    } else {
        QTRY_VERIFY(textEditObject->selectedText().length() > 3);
        QVERIFY(str != text);
    }
}

void tst_qquicktextedit::inputMethodHints()
{
    QQuickView canvas(testFileUrl("inputmethodhints.qml"));
    canvas.show();
    canvas.requestActivateWindow();

    QVERIFY(canvas.rootObject() != 0);
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit *>(canvas.rootObject());
    QVERIFY(textEditObject != 0);
    QVERIFY(textEditObject->inputMethodHints() & Qt::ImhNoPredictiveText);
    textEditObject->setInputMethodHints(Qt::ImhUppercaseOnly);
    QVERIFY(textEditObject->inputMethodHints() & Qt::ImhUppercaseOnly);
}

void tst_qquicktextedit::positionAt()
{
    QQuickView canvas(testFileUrl("positionAt.qml"));
    QVERIFY(canvas.rootObject() != 0);
    canvas.show();
    canvas.requestActivateWindow();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);

    QQuickTextEdit *texteditObject = qobject_cast<QQuickTextEdit *>(canvas.rootObject());
    QVERIFY(texteditObject != 0);

    QTextLayout layout(texteditObject->text());
    layout.setFont(texteditObject->font());

    if (!qmlDisableDistanceField()) {
        QTextOption option;
        option.setUseDesignMetrics(true);
        layout.setTextOption(option);
    }

    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    const int y0 = line.height() / 2;
    const int y1 = line.height() * 3 / 2;

    int pos = texteditObject->positionAt(texteditObject->width()/2, y0);

    int widthBegin = floor(line.cursorToX(pos - 1));
    int widthEnd = ceil(line.cursorToX(pos + 1));

    QVERIFY(widthBegin <= texteditObject->width() / 2);
    QVERIFY(widthEnd >= texteditObject->width() / 2);

    const qreal x0 = texteditObject->positionToRectangle(pos).x();
    const qreal x1 = texteditObject->positionToRectangle(pos + 1).x();

    QString preeditText = texteditObject->text().mid(0, pos);
    texteditObject->setText(texteditObject->text().mid(pos));
    texteditObject->setCursorPosition(0);

    QInputMethodEvent inputEvent(preeditText, QList<QInputMethodEvent::Attribute>());
    QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &inputEvent);

    // Check all points within the preedit text return the same position.
    QCOMPARE(texteditObject->positionAt(0, y0), 0);
    QCOMPARE(texteditObject->positionAt(x0 / 2, y0), 0);
    QCOMPARE(texteditObject->positionAt(x0, y0), 0);

    // Verify positioning returns to normal after the preedit text.
    QCOMPARE(texteditObject->positionAt(x1, y0), 1);
    QCOMPARE(texteditObject->positionToRectangle(1).x(), x1);

    QVERIFY(texteditObject->positionAt(x0 / 2, y1) > 0);
}

void tst_qquicktextedit::linkActivated()
{
    QQuickView canvas(testFileUrl("linkActivated.qml"));
    QVERIFY(canvas.rootObject() != 0);
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);

    QQuickTextEdit *texteditObject = qobject_cast<QQuickTextEdit *>(canvas.rootObject());
    QVERIFY(texteditObject != 0);

    QSignalSpy spy(texteditObject, SIGNAL(linkActivated(QString)));

    const QString link("http://example.com/");

    const QPointF linkPos = texteditObject->positionToRectangle(7).center();
    const QPointF textPos = texteditObject->positionToRectangle(2).center();

    QTest::mouseClick(&canvas, Qt::LeftButton, 0, linkPos.toPoint());
    QTRY_COMPARE(spy.count(), 1);
    QCOMPARE(spy.last()[0].toString(), link);

    QTest::mouseClick(&canvas, Qt::LeftButton, 0, textPos.toPoint());
    QTest::qWait(50);
    QCOMPARE(spy.count(), 1);

    texteditObject->setReadOnly(true);

    QTest::mouseClick(&canvas, Qt::LeftButton, 0, linkPos.toPoint());
    QTRY_COMPARE(spy.count(), 2);
    QCOMPARE(spy.last()[0].toString(), link);

    QTest::mouseClick(&canvas, Qt::LeftButton, 0, textPos.toPoint());
    QTest::qWait(50);
    QCOMPARE(spy.count(), 2);
}

void tst_qquicktextedit::cursorDelegate()
{
    QQuickView view(testFileUrl("cursorTest.qml"));
    view.show();
    view.requestActivateWindow();
    QQuickTextEdit *textEditObject = view.rootObject()->findChild<QQuickTextEdit*>("textEditObject");
    QVERIFY(textEditObject != 0);
    QVERIFY(textEditObject->findChild<QQuickItem*>("cursorInstance"));
    //Test Delegate gets created
    textEditObject->setFocus(true);
    QQuickItem* delegateObject = textEditObject->findChild<QQuickItem*>("cursorInstance");
    QVERIFY(delegateObject);
    QCOMPARE(delegateObject->property("localProperty").toString(), QString("Hello"));
    //Test Delegate gets moved
    for (int i=0; i<= textEditObject->text().length(); i++) {
        textEditObject->setCursorPosition(i);
        QCOMPARE(textEditObject->cursorRectangle().x(), qRound(delegateObject->x()));
        QCOMPARE(textEditObject->cursorRectangle().y(), qRound(delegateObject->y()));
    }
    // Clear preedit text;
    QInputMethodEvent event;
    QGuiApplication::sendEvent(&view, &event);


    // Test delegate gets moved on mouse press.
    textEditObject->setSelectByMouse(true);
    textEditObject->setCursorPosition(0);
    const QPoint point1 = textEditObject->positionToRectangle(5).center().toPoint();
    QTest::mouseClick(&view, Qt::LeftButton, 0, point1);
    QTest::qWait(50);
    QTRY_VERIFY(textEditObject->cursorPosition() != 0);
    QCOMPARE(textEditObject->cursorRectangle().x(), qRound(delegateObject->x()));
    QCOMPARE(textEditObject->cursorRectangle().y(), qRound(delegateObject->y()));

    // Test delegate gets moved on mouse drag
    textEditObject->setCursorPosition(0);
    const QPoint point2 = textEditObject->positionToRectangle(10).center().toPoint();
    QTest::mousePress(&view, Qt::LeftButton, 0, point1);
    QMouseEvent mv(QEvent::MouseMove, point2, Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
    QGuiApplication::sendEvent(&view, &mv);
    QTest::mouseRelease(&view, Qt::LeftButton, 0, point2);
    QTest::qWait(50);
    QTRY_COMPARE(textEditObject->cursorRectangle().x(), qRound(delegateObject->x()));
    QCOMPARE(textEditObject->cursorRectangle().y(), qRound(delegateObject->y()));

    textEditObject->setReadOnly(true);
    textEditObject->setCursorPosition(0);
    QTest::mouseClick(&view, Qt::LeftButton, 0, textEditObject->positionToRectangle(5).center().toPoint());
    QTest::qWait(50);
    QTRY_VERIFY(textEditObject->cursorPosition() != 0);
    QCOMPARE(textEditObject->cursorRectangle().x(), qRound(delegateObject->x()));
    QCOMPARE(textEditObject->cursorRectangle().y(), qRound(delegateObject->y()));

    textEditObject->setCursorPosition(0);
    QTest::mouseClick(&view, Qt::LeftButton, 0, textEditObject->positionToRectangle(5).center().toPoint());
    QTest::qWait(50);
    QTRY_VERIFY(textEditObject->cursorPosition() != 0);
    QCOMPARE(textEditObject->cursorRectangle().x(), qRound(delegateObject->x()));
    QCOMPARE(textEditObject->cursorRectangle().y(), qRound(delegateObject->y()));

    textEditObject->setCursorPosition(0);
    QCOMPARE(textEditObject->cursorRectangle().x(), qRound(delegateObject->x()));
    QCOMPARE(textEditObject->cursorRectangle().y(), qRound(delegateObject->y()));
    //Test Delegate gets deleted
    textEditObject->setCursorDelegate(0);
    QVERIFY(!textEditObject->findChild<QQuickItem*>("cursorInstance"));
}

void tst_qquicktextedit::cursorVisible()
{
    QQuickView view(testFileUrl("cursorVisible.qml"));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());

    QQuickTextEdit edit;
    QSignalSpy spy(&edit, SIGNAL(cursorVisibleChanged(bool)));

    QCOMPARE(edit.isCursorVisible(), false);

    edit.setCursorVisible(true);
    QCOMPARE(edit.isCursorVisible(), true);
    QCOMPARE(spy.count(), 1);

    edit.setCursorVisible(false);
    QCOMPARE(edit.isCursorVisible(), false);
    QCOMPARE(spy.count(), 2);

    edit.setFocus(true);
    QCOMPARE(edit.isCursorVisible(), false);
    QCOMPARE(spy.count(), 2);

    edit.setParentItem(view.rootObject());
    QCOMPARE(edit.isCursorVisible(), true);
    QCOMPARE(spy.count(), 3);

    edit.setFocus(false);
    QCOMPARE(edit.isCursorVisible(), false);
    QCOMPARE(spy.count(), 4);

    edit.setFocus(true);
    QCOMPARE(edit.isCursorVisible(), true);
    QCOMPARE(spy.count(), 5);

    QQuickView alternateView;
    alternateView.show();
    alternateView.requestActivateWindow();
    QTest::qWaitForWindowShown(&alternateView);

    QCOMPARE(edit.isCursorVisible(), false);
    QCOMPARE(spy.count(), 6);

    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QCOMPARE(edit.isCursorVisible(), true);
    QCOMPARE(spy.count(), 7);
}

void tst_qquicktextedit::delegateLoading_data()
{
    QTest::addColumn<QString>("qmlfile");
    QTest::addColumn<QString>("error");

    // import installed
    QTest::newRow("pass") << "cursorHttpTestPass.qml" << "";
    QTest::newRow("fail1") << "cursorHttpTestFail1.qml" << "http://localhost:42332/FailItem.qml: Remote host closed the connection ";
    QTest::newRow("fail2") << "cursorHttpTestFail2.qml" << "http://localhost:42332/ErrItem.qml:4:5: Fungus is not a type ";
}

void tst_qquicktextedit::delegateLoading()
{
#ifdef Q_OS_MAC
    QSKIP("Test crashes during canvas tear down. QTBUG-23010");
#endif
    QFETCH(QString, qmlfile);
    QFETCH(QString, error);

    TestHTTPServer server(42332);
    server.serveDirectory(testFile("httpfail"), TestHTTPServer::Disconnect);
    server.serveDirectory(testFile("httpslow"), TestHTTPServer::Delay);
    server.serveDirectory(testFile("http"));

    QQuickView view(QUrl(QLatin1String("http://localhost:42332/") + qmlfile));
    view.show();
    view.requestActivateWindow();

    if (!error.isEmpty()) {
        QTest::ignoreMessage(QtWarningMsg, error.toUtf8());
        QTRY_VERIFY(view.status()==QQuickView::Error);
        QTRY_VERIFY(!view.rootObject()); // there is fail item inside this test
    } else {
        QTRY_VERIFY(view.rootObject());//Wait for loading to finish.
        QQuickTextEdit *textEditObject = view.rootObject()->findChild<QQuickTextEdit*>("textEditObject");
        //    view.rootObject()->dumpObjectTree();
        QVERIFY(textEditObject != 0);
        textEditObject->setFocus(true);
        QQuickItem *delegate;
        delegate = view.rootObject()->findChild<QQuickItem*>("delegateOkay");
        QVERIFY(delegate);
        delegate = view.rootObject()->findChild<QQuickItem*>("delegateSlow");
        QVERIFY(delegate);

        delete delegate;
    }


    //A test should be added here with a component which is ready but component.create() returns null
    //Not sure how to accomplish this with QQuickTextEdits cursor delegate
    //###This was only needed for code coverage, and could be a case of overzealous defensive programming
    //delegate = view.rootObject()->findChild<QQuickItem*>("delegateErrorB");
    //QVERIFY(!delegate);
}

/*
TextEdit element should only handle left/right keys until the cursor reaches
the extent of the text, then they should ignore the keys.
*/
void tst_qquicktextedit::navigation()
{
    QQuickView canvas(testFileUrl("navigation.qml"));
    canvas.show();
    canvas.requestActivateWindow();

    QVERIFY(canvas.rootObject() != 0);

    QQuickItem *input = qobject_cast<QQuickItem *>(qvariant_cast<QObject *>(canvas.rootObject()->property("myInput")));

    QVERIFY(input != 0);
    QTRY_VERIFY(input->hasActiveFocus() == true);
    simulateKey(&canvas, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus() == false);
    simulateKey(&canvas, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus() == true);
    simulateKey(&canvas, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus() == true);
    simulateKey(&canvas, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus() == false);
    simulateKey(&canvas, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus() == true);
}

void tst_qquicktextedit::copyAndPaste() {
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

    QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"Hello world!\" }";
    QDeclarativeComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != 0);

    // copy and paste
    QCOMPARE(textEdit->text().length(), 12);
    textEdit->select(0, textEdit->text().length());;
    textEdit->copy();
    QCOMPARE(textEdit->selectedText(), QString("Hello world!"));
    QCOMPARE(textEdit->selectedText().length(), 12);
    textEdit->setCursorPosition(0);
    QVERIFY(textEdit->canPaste());
    textEdit->paste();
    QCOMPARE(textEdit->text(), QString("Hello world!Hello world!"));
    QCOMPARE(textEdit->text().length(), 24);

    // canPaste
    QVERIFY(textEdit->canPaste());
    textEdit->setReadOnly(true);
    QVERIFY(!textEdit->canPaste());
    textEdit->setReadOnly(false);
    QVERIFY(textEdit->canPaste());

    // QTBUG-12339
    // test that document and internal text attribute are in sync
    QQuickItemPrivate* pri = QQuickItemPrivate::get(textEdit);
    QQuickTextEditPrivate *editPrivate = static_cast<QQuickTextEditPrivate*>(pri);
    QCOMPARE(textEdit->text(), editPrivate->text);

    // select word
    textEdit->setCursorPosition(0);
    textEdit->selectWord();
    QCOMPARE(textEdit->selectedText(), QString("Hello"));

    // select all and cut
    textEdit->selectAll();
    textEdit->cut();
    QCOMPARE(textEdit->text().length(), 0);
    textEdit->paste();
    QCOMPARE(textEdit->text(), QString("Hello world!Hello world!"));
    QCOMPARE(textEdit->text().length(), 24);
#endif
}

void tst_qquicktextedit::canPaste() {
#ifndef QT_NO_CLIPBOARD

    QGuiApplication::clipboard()->setText("Some text");

    QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"Hello world!\" }";
    QDeclarativeComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != 0);

    // check initial value - QTBUG-17765
    QQuickTextControl tc(0);
    QCOMPARE(textEdit->canPaste(), tc.canPaste());

#endif
}

void tst_qquicktextedit::canPasteEmpty() {
#ifndef QT_NO_CLIPBOARD

    QGuiApplication::clipboard()->clear();

    QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"Hello world!\" }";
    QDeclarativeComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != 0);

    // check initial value - QTBUG-17765
    QQuickTextControl tc(0);
    QCOMPARE(textEdit->canPaste(), tc.canPaste());

#endif
}

void tst_qquicktextedit::readOnly()
{
    QQuickView canvas(testFileUrl("readOnly.qml"));
    canvas.show();
    canvas.requestActivateWindow();

    QVERIFY(canvas.rootObject() != 0);

    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(qvariant_cast<QObject *>(canvas.rootObject()->property("myInput")));

    QVERIFY(edit != 0);
    QTRY_VERIFY(edit->hasActiveFocus() == true);
    QVERIFY(edit->isReadOnly() == true);
    QString initial = edit->text();
    for (int k=Qt::Key_0; k<=Qt::Key_Z; k++)
        simulateKey(&canvas, k);
    simulateKey(&canvas, Qt::Key_Return);
    simulateKey(&canvas, Qt::Key_Space);
    simulateKey(&canvas, Qt::Key_Escape);
    QCOMPARE(edit->text(), initial);

    edit->setCursorPosition(3);
    edit->setReadOnly(false);
    QCOMPARE(edit->isReadOnly(), false);
    QCOMPARE(edit->cursorPosition(), edit->text().length());
}

void tst_qquicktextedit::simulateKey(QQuickView *view, int key, Qt::KeyboardModifiers modifiers)
{
    QKeyEvent press(QKeyEvent::KeyPress, key, modifiers);
    QKeyEvent release(QKeyEvent::KeyRelease, key, modifiers);

    QGuiApplication::sendEvent(view, &press);
    QGuiApplication::sendEvent(view, &release);
}

void tst_qquicktextedit::textInput()
{
    QQuickView view(testFileUrl("inputMethodEvent.qml"));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(view.rootObject());
    QVERIFY(edit);
    QVERIFY(edit->hasActiveFocus() == true);

    // test that input method event is committed and change signal is emitted
    QSignalSpy spy(edit, SIGNAL(textChanged(QString)));
    QInputMethodEvent event;
    event.setCommitString( "Hello world!", 0, 0);
    QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &event);
    QCOMPARE(edit->text(), QString("Hello world!"));
    QCOMPARE(spy.count(), 1);

    // QTBUG-12339
    // test that document and internal text attribute are in sync
    QQuickTextEditPrivate *editPrivate = static_cast<QQuickTextEditPrivate*>(QQuickItemPrivate::get(edit));
    QCOMPARE(editPrivate->text, QString("Hello world!"));

    // test that tentative commit is included in text property
    edit->setText("");
    spy.clear();
    QList<QInputMethodEvent::Attribute> attributes;
    QInputMethodEvent event2("preedit", attributes);
    event2.setTentativeCommitString("string");
    QGuiApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &event2);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(edit->text(), QString("string"));
}

void tst_qquicktextedit::openInputPanel()
{
    PlatformInputContext platformInputContext;
    QInputPanelPrivate *inputPanelPrivate = QInputPanelPrivate::get(qApp->inputPanel());
    inputPanelPrivate->testContext = &platformInputContext;

    QQuickView view(testFileUrl("openInputPanel.qml"));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());

    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(view.rootObject());
    QVERIFY(edit);

    // check default values
    QVERIFY(edit->focusOnPress());
    QVERIFY(!edit->hasActiveFocus());
    qDebug() << &edit << qApp->inputPanel()->inputItem();
    QCOMPARE(qApp->inputPanel()->inputItem(), static_cast<QObject*>(0));

    QCOMPARE(qApp->inputPanel()->visible(), false);

    // input panel should open on focus
    QPoint centerPoint(view.width()/2, view.height()/2);
    Qt::KeyboardModifiers noModifiers = 0;
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QVERIFY(edit->hasActiveFocus());
    QCOMPARE(qApp->inputPanel()->inputItem(), edit);
    QCOMPARE(qApp->inputPanel()->visible(), true);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);

    // input panel should be re-opened when pressing already focused TextEdit
    qApp->inputPanel()->hide();
    QCOMPARE(qApp->inputPanel()->visible(), false);
    QVERIFY(edit->hasActiveFocus());
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QCOMPARE(qApp->inputPanel()->visible(), true);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);

    // input panel should stay visible if focus is lost to another text editor
    QSignalSpy inputPanelVisibilitySpy(qApp->inputPanel(), SIGNAL(visibleChanged()));
    QQuickTextEdit anotherEdit;
    anotherEdit.setParentItem(view.rootObject());
    anotherEdit.setFocus(true);
    QCOMPARE(qApp->inputPanel()->visible(), true);
    QCOMPARE(qApp->inputPanel()->inputItem(), qobject_cast<QObject*>(&anotherEdit));
    QCOMPARE(inputPanelVisibilitySpy.count(), 0);

    anotherEdit.setFocus(false);
    QCOMPARE(qApp->inputPanel()->inputItem(), static_cast<QObject*>(0));
    QCOMPARE(view.activeFocusItem(), view.rootItem());
    anotherEdit.setFocus(true);

    // input item should be null if focus is lost to an item that doesn't accept inputs
    QQuickItem item;
    item.setParentItem(view.rootObject());
    item.setFocus(true);
    QCOMPARE(qApp->inputPanel()->inputItem(), static_cast<QObject*>(0));
    QCOMPARE(view.activeFocusItem(), &item);

    qApp->inputPanel()->hide();

    // input panel should not be opened if TextEdit is read only
    edit->setReadOnly(true);
    edit->setFocus(true);
    QCOMPARE(qApp->inputPanel()->visible(), false);
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QCOMPARE(qApp->inputPanel()->visible(), false);

    // input panel should not be opened if focusOnPress is set to false
    edit->setFocusOnPress(false);
    edit->setFocus(false);
    edit->setFocus(true);
    QCOMPARE(qApp->inputPanel()->visible(), false);
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);
    QCOMPARE(qApp->inputPanel()->visible(), false);

    // input panel should open when openSoftwareInputPanel is called
    edit->openSoftwareInputPanel();
    QCOMPARE(qApp->inputPanel()->visible(), true);

    // input panel should close when closeSoftwareInputPanel is called
    edit->closeSoftwareInputPanel();
    QCOMPARE(qApp->inputPanel()->visible(), false);

    inputPanelPrivate->testContext = 0;
}

void tst_qquicktextedit::geometrySignals()
{
    QDeclarativeComponent component(&engine, testFileUrl("geometrySignals.qml"));
    QObject *o = component.create();
    QVERIFY(o);
    QCOMPARE(o->property("bindingWidth").toInt(), 400);
    QCOMPARE(o->property("bindingHeight").toInt(), 500);
    delete o;
}

void tst_qquicktextedit::pastingRichText_QTBUG_14003()
{
#ifndef QT_NO_CLIPBOARD
    QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.PlainText }";
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickTextEdit *obj = qobject_cast<QQuickTextEdit*>(component.create());

    QTRY_VERIFY(obj != 0);
    QTRY_VERIFY(obj->textFormat() == QQuickTextEdit::PlainText);

    QMimeData *mData = new QMimeData;
    mData->setHtml("<font color=\"red\">Hello</font>");
    QGuiApplication::clipboard()->setMimeData(mData);

    obj->paste();
    QTRY_VERIFY(obj->text() == "");
    QTRY_VERIFY(obj->textFormat() == QQuickTextEdit::PlainText);
#endif
}

void tst_qquicktextedit::implicitSize_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("wrap");
    QTest::newRow("plain") << "The quick red fox jumped over the lazy brown dog" << "TextEdit.NoWrap";
    QTest::newRow("richtext") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "TextEdit.NoWrap";
    QTest::newRow("plain_wrap") << "The quick red fox jumped over the lazy brown dog" << "TextEdit.Wrap";
    QTest::newRow("richtext_wrap") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "TextEdit.Wrap";
}

void tst_qquicktextedit::implicitSize()
{
    QFETCH(QString, text);
    QFETCH(QString, wrap);
    QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"" + text + "\"; width: 50; wrapMode: " + wrap + " }";
    QDeclarativeComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit*>(textComponent.create());

    QVERIFY(textObject->width() < textObject->implicitWidth());
    QVERIFY(textObject->height() == textObject->implicitHeight());

    textObject->resetWidth();
    QVERIFY(textObject->width() == textObject->implicitWidth());
    QVERIFY(textObject->height() == textObject->implicitHeight());
}

void tst_qquicktextedit::testQtQuick11Attributes()
{
    QFETCH(QString, code);
    QFETCH(QString, warning);
    QFETCH(QString, error);

    QDeclarativeEngine engine;
    QObject *obj;

    QDeclarativeComponent valid(&engine);
    valid.setData("import QtQuick 2.0; TextEdit { " + code.toUtf8() + " }", QUrl(""));
    obj = valid.create();
    QVERIFY(obj);
    QVERIFY(valid.errorString().isEmpty());
    delete obj;

    QDeclarativeComponent invalid(&engine);
    invalid.setData("import QtQuick 1.0; TextEdit { " + code.toUtf8() + " }", QUrl(""));
    QTest::ignoreMessage(QtWarningMsg, warning.toUtf8());
    obj = invalid.create();
    QCOMPARE(invalid.errorString(), error);
    delete obj;
}

void tst_qquicktextedit::testQtQuick11Attributes_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<QString>("warning");
    QTest::addColumn<QString>("error");

    QTest::newRow("canPaste") << "property bool foo: canPaste"
        << "<Unknown File>:1: ReferenceError: Can't find variable: canPaste"
        << "";

    QTest::newRow("lineCount") << "property int foo: lineCount"
        << "<Unknown File>:1: ReferenceError: Can't find variable: lineCount"
        << "";

    QTest::newRow("moveCursorSelection") << "Component.onCompleted: moveCursorSelection(0, TextEdit.SelectCharacters)"
        << "<Unknown File>:1: ReferenceError: Can't find variable: moveCursorSelection"
        << "";

    QTest::newRow("deselect") << "Component.onCompleted: deselect()"
        << "<Unknown File>:1: ReferenceError: Can't find variable: deselect"
        << "";

    QTest::newRow("onLinkActivated") << "onLinkActivated: {}"
        << "QDeclarativeComponent: Component is not ready"
        << ":1 \"TextEdit.onLinkActivated\" is not available in QtQuick 1.0.\n";
}

void tst_qquicktextedit::preeditCursorRectangle()
{
    QString preeditText = "super";

    QQuickView view(testFileUrl("inputMethodEvent.qml"));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);

    QTRY_COMPARE(&view, qGuiApp->focusWindow());
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(view.rootObject());
    QVERIFY(edit);

    QSignalSpy editSpy(edit, SIGNAL(cursorRectangleChanged()));
    QSignalSpy panelSpy(qGuiApp->inputPanel(), SIGNAL(cursorRectangleChanged()));

    QRect currentRect;

    QInputMethodQueryEvent query(Qt::ImCursorRectangle);
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &query);
    QRect previousRect = query.value(Qt::ImCursorRectangle).toRect();

    // Verify that the micro focus rect is positioned the same for position 0 as
    // it would be if there was no preedit text.
    QInputMethodEvent imEvent(preeditText, QList<QInputMethodEvent::Attribute>()
            << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 0, preeditText.length(), QVariant()));
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &imEvent);
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &query);
    currentRect = query.value(Qt::ImCursorRectangle).toRect();
    QCOMPARE(currentRect, previousRect);
    QCOMPARE(editSpy.count(), 0);
    QCOMPARE(panelSpy.count(), 0);

    // Verify that the micro focus rect moves to the left as the cursor position
    // is incremented.
    for (int i = 1; i <= 5; ++i) {
        QInputMethodEvent imEvent(preeditText, QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, i, preeditText.length(), QVariant()));
        QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &imEvent);
        QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &query);
        currentRect = query.value(Qt::ImCursorRectangle).toRect();
        QVERIFY(previousRect.left() < currentRect.left());
        QVERIFY(editSpy.count() > 0); editSpy.clear();
        QVERIFY(panelSpy.count() > 0); panelSpy.clear();
        previousRect = currentRect;
    }

    // Verify that if there is no preedit cursor then the micro focus rect is the
    // same as it would be if it were positioned at the end of the preedit text.
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &imEvent);
    editSpy.clear();
    panelSpy.clear();
    {   QInputMethodEvent imEvent(preeditText, QList<QInputMethodEvent::Attribute>());
        QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &imEvent); }
    QCoreApplication::sendEvent(qGuiApp->inputPanel()->inputItem(), &query);
    currentRect = query.value(Qt::ImCursorRectangle).toRect();
    QCOMPARE(currentRect, previousRect);
    QVERIFY(editSpy.count() > 0);
    QVERIFY(panelSpy.count() > 0);
}

void tst_qquicktextedit::inputMethodComposing()
{
    QString text = "supercalifragisiticexpialidocious!";

    QQuickView view(testFileUrl("inputContext.qml"));
    view.show();
    view.requestActivateWindow();
    QTest::qWaitForWindowShown(&view);
    QTRY_COMPARE(&view, qGuiApp->focusWindow());
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(view.rootObject());
    QVERIFY(edit);
    QSignalSpy spy(edit, SIGNAL(inputMethodComposingChanged()));
    edit->setCursorPosition(12);

    QCOMPARE(edit->isInputMethodComposing(), false);

    {
        QInputMethodEvent event(text.mid(3), QList<QInputMethodEvent::Attribute>());
        QGuiApplication::sendEvent(edit, &event);
    }

    QCOMPARE(edit->isInputMethodComposing(), true);
    QCOMPARE(spy.count(), 1);

    {
        QInputMethodEvent event(text.mid(12), QList<QInputMethodEvent::Attribute>());
        QGuiApplication::sendEvent(edit, &event);
    }
    QCOMPARE(spy.count(), 1);

    {
        QInputMethodEvent event;
        QGuiApplication::sendEvent(edit, &event);
    }
    QCOMPARE(edit->isInputMethodComposing(), false);
    QCOMPARE(spy.count(), 2);
}

void tst_qquicktextedit::cursorRectangleSize()
{
    QQuickView *canvas = new QQuickView(testFileUrl("positionAt.qml"));
    QVERIFY(canvas->rootObject() != 0);
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit *>(canvas->rootObject());

    // make sure cursor rectangle is not at (0,0)
    textEdit->setX(10);
    textEdit->setY(10);
    textEdit->setCursorPosition(3);
    QVERIFY(textEdit != 0);
    textEdit->setFocus(true);
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);

    QInputMethodQueryEvent event(Qt::ImCursorRectangle);
    qApp->sendEvent(qApp->inputPanel()->inputItem(), &event);
    QRectF cursorRectFromQuery = event.value(Qt::ImCursorRectangle).toRectF();

    QRect cursorRectFromItem = textEdit->cursorRectangle();
    QRectF cursorRectFromPositionToRectangle = textEdit->positionToRectangle(textEdit->cursorPosition());

    // item and input query cursor rectangles match
    QCOMPARE(cursorRectFromItem, cursorRectFromQuery.toRect());

    // item cursor rectangle and positionToRectangle calculations match
    QCOMPARE(cursorRectFromItem, cursorRectFromPositionToRectangle.toRect());

    // item-canvas transform and input item transform match
    QCOMPARE(QQuickItemPrivate::get(textEdit)->itemToCanvasTransform(), qApp->inputPanel()->inputItemTransform());

    // input panel cursorRectangle property and tranformed item cursor rectangle match
    QRectF sceneCursorRect = QQuickItemPrivate::get(textEdit)->itemToCanvasTransform().mapRect(cursorRectFromItem);
    QCOMPARE(sceneCursorRect, qApp->inputPanel()->cursorRectangle());

    delete canvas;
}

void tst_qquicktextedit::getText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("end");
    QTest::addColumn<QString>("expectedText");

    const QString richBoldText = QStringLiteral("This is some <b>bold</b> text");
    const QString plainBoldText = QStringLiteral("This is some bold text");

    QTest::newRow("all plain text")
            << standard.at(0)
            << 0 << standard.at(0).length()
            << standard.at(0);

    QTest::newRow("plain text sub string")
            << standard.at(0)
            << 0 << 12
            << standard.at(0).mid(0, 12);

    QTest::newRow("plain text sub string reversed")
            << standard.at(0)
            << 12 << 0
            << standard.at(0).mid(0, 12);

    QTest::newRow("plain text cropped beginning")
            << standard.at(0)
            << -3 << 4
            << standard.at(0).mid(0, 4);

    QTest::newRow("plain text cropped end")
            << standard.at(0)
            << 23 << standard.at(0).length() + 8
            << standard.at(0).mid(23);

    QTest::newRow("plain text cropped beginning and end")
            << standard.at(0)
            << -9 << standard.at(0).length() + 4
            << standard.at(0);

    QTest::newRow("all rich text")
            << richBoldText
            << 0 << plainBoldText.length()
            << plainBoldText;

    QTest::newRow("rich text sub string")
            << richBoldText
            << 14 << 21
            << plainBoldText.mid(14, 7);
}

void tst_qquicktextedit::getText()
{
    QFETCH(QString, text);
    QFETCH(int, start);
    QFETCH(int, end);
    QFETCH(QString, expectedText);

    QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.AutoText; text: \"" + text + "\" }";
    QDeclarativeComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != 0);

    QCOMPARE(textEdit->getText(start, end), expectedText);
}

void tst_qquicktextedit::getFormattedText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QQuickTextEdit::TextFormat>("textFormat");
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("end");
    QTest::addColumn<QString>("expectedText");

    const QString richBoldText = QStringLiteral("This is some <b>bold</b> text");
    const QString plainBoldText = QStringLiteral("This is some bold text");

    QTest::newRow("all plain text")
            << standard.at(0)
            << QQuickTextEdit::PlainText
            << 0 << standard.at(0).length()
            << standard.at(0);

    QTest::newRow("plain text sub string")
            << standard.at(0)
            << QQuickTextEdit::PlainText
            << 0 << 12
            << standard.at(0).mid(0, 12);

    QTest::newRow("plain text sub string reversed")
            << standard.at(0)
            << QQuickTextEdit::PlainText
            << 12 << 0
            << standard.at(0).mid(0, 12);

    QTest::newRow("plain text cropped beginning")
            << standard.at(0)
            << QQuickTextEdit::PlainText
            << -3 << 4
            << standard.at(0).mid(0, 4);

    QTest::newRow("plain text cropped end")
            << standard.at(0)
            << QQuickTextEdit::PlainText
            << 23 << standard.at(0).length() + 8
            << standard.at(0).mid(23);

    QTest::newRow("plain text cropped beginning and end")
            << standard.at(0)
            << QQuickTextEdit::PlainText
            << -9 << standard.at(0).length() + 4
            << standard.at(0);

    QTest::newRow("all rich (Auto) text")
            << richBoldText
            << QQuickTextEdit::AutoText
            << 0 << plainBoldText.length()
            << QString("This is some \\<.*\\>bold\\</.*\\> text");

    QTest::newRow("all rich (Rich) text")
            << richBoldText
            << QQuickTextEdit::RichText
            << 0 << plainBoldText.length()
            << QString("This is some \\<.*\\>bold\\</.*\\> text");

    QTest::newRow("all rich (Plain) text")
            << richBoldText
            << QQuickTextEdit::PlainText
            << 0 << richBoldText.length()
            << richBoldText;

    QTest::newRow("rich (Auto) text sub string")
            << richBoldText
            << QQuickTextEdit::AutoText
            << 14 << 21
            << QString("\\<.*\\>old\\</.*\\> tex");

    QTest::newRow("rich (Rich) text sub string")
            << richBoldText
            << QQuickTextEdit::RichText
            << 14 << 21
            << QString("\\<.*\\>old\\</.*\\> tex");

    QTest::newRow("rich (Plain) text sub string")
            << richBoldText
            << QQuickTextEdit::PlainText
            << 17 << 27
            << richBoldText.mid(17, 10);
}

void tst_qquicktextedit::getFormattedText()
{
    QFETCH(QString, text);
    QFETCH(QQuickTextEdit::TextFormat, textFormat);
    QFETCH(int, start);
    QFETCH(int, end);
    QFETCH(QString, expectedText);

    QString componentStr = "import QtQuick 2.0\nTextEdit {}";
    QDeclarativeComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != 0);

    textEdit->setTextFormat(textFormat);
    textEdit->setText(text);

    if (textFormat == QQuickTextEdit::RichText
            || (textFormat == QQuickTextEdit::AutoText && Qt::mightBeRichText(text))) {
        QVERIFY(textEdit->getFormattedText(start, end).contains(QRegExp(expectedText)));
    } else {
        QCOMPARE(textEdit->getFormattedText(start, end), expectedText);
    }
}

void tst_qquicktextedit::insert_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QQuickTextEdit::TextFormat>("textFormat");
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
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0 << 0
            << QString("Hello")
            << QString("Hello") + standard.at(0)
            << 5 << 5 << 5
            << false << true;

    QTest::newRow("at cursor position (end)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << standard.at(0).length() << standard.at(0).length() << standard.at(0).length()
            << QString("Hello")
            << standard.at(0) + QString("Hello")
            << standard.at(0).length() + 5 << standard.at(0).length() + 5 << standard.at(0).length() + 5
            << false << true;

    QTest::newRow("at cursor position (middle)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 18 << 18 << 18
            << QString("Hello")
            << standard.at(0).mid(0, 18) + QString("Hello") + standard.at(0).mid(18)
            << 23 << 23 << 23
            << false << true;

    QTest::newRow("after cursor position (beginning)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0 << 18
            << QString("Hello")
            << standard.at(0).mid(0, 18) + QString("Hello") + standard.at(0).mid(18)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("before cursor position (end)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << standard.at(0).length() << standard.at(0).length() << 18
            << QString("Hello")
            << standard.at(0).mid(0, 18) + QString("Hello") + standard.at(0).mid(18)
            << standard.at(0).length() + 5 << standard.at(0).length() + 5 << standard.at(0).length() + 5
            << false << true;

    QTest::newRow("before cursor position (middle)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 18 << 18 << 0
            << QString("Hello")
            << QString("Hello") + standard.at(0)
            << 23 << 23 << 23
            << false << true;

    QTest::newRow("after cursor position (middle)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 18 << 18 << standard.at(0).length()
            << QString("Hello")
            << standard.at(0) + QString("Hello")
            << 18 << 18 << 18
            << false << false;

    QTest::newRow("before selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 14 << 19 << 0
            << QString("Hello")
            << QString("Hello") + standard.at(0)
            << 19 << 24 << 24
            << false << true;

    QTest::newRow("before reversed selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 19 << 14 << 0
            << QString("Hello")
            << QString("Hello") + standard.at(0)
            << 19 << 24 << 19
            << false << true;

    QTest::newRow("after selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 14 << 19 << standard.at(0).length()
            << QString("Hello")
            << standard.at(0) + QString("Hello")
            << 14 << 19 << 19
            << false << false;

    QTest::newRow("after reversed selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 19 << 14 << standard.at(0).length()
            << QString("Hello")
            << standard.at(0) + QString("Hello")
            << 14 << 19 << 14
            << false << false;

    QTest::newRow("into selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 14 << 19 << 18
            << QString("Hello")
            << standard.at(0).mid(0, 18) + QString("Hello") + standard.at(0).mid(18)
            << 14 << 24 << 24
            << true << true;

    QTest::newRow("into reversed selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 19 << 14 << 18
            << QString("Hello")
            << standard.at(0).mid(0, 18) + QString("Hello") + standard.at(0).mid(18)
            << 14 << 24 << 14
            << true << false;

    QTest::newRow("rich text into plain text")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0 << 0
            << QString("<b>Hello</b>")
            << QString("<b>Hello</b>") + standard.at(0)
            << 12 << 12 << 12
            << false << true;

    QTest::newRow("rich text into rich text")
            << standard.at(0) << QQuickTextEdit::RichText
            << 0 << 0 << 0
            << QString("<b>Hello</b>")
            << QString("Hello") + standard.at(0)
            << 5 << 5 << 5
            << false << true;

    QTest::newRow("rich text into auto text")
            << standard.at(0) << QQuickTextEdit::AutoText
            << 0 << 0 << 0
            << QString("<b>Hello</b>")
            << QString("Hello") + standard.at(0)
            << 5 << 5 << 5
            << false << true;

    QTest::newRow("before start")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0 << -3
            << QString("Hello")
            << standard.at(0)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("past end")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0 << standard.at(0).length() + 3
            << QString("Hello")
            << standard.at(0)
            << 0 << 0 << 0
            << false << false;
}

void tst_qquicktextedit::insert()
{
    QFETCH(QString, text);
    QFETCH(QQuickTextEdit::TextFormat, textFormat);
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

    QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"" + text + "\" }";
    QDeclarativeComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != 0);

    textEdit->setTextFormat(textFormat);
    textEdit->select(selectionStart, selectionEnd);

    QSignalSpy selectionSpy(textEdit, SIGNAL(selectionChanged()));
    QSignalSpy selectionStartSpy(textEdit, SIGNAL(selectionStartChanged()));
    QSignalSpy selectionEndSpy(textEdit, SIGNAL(selectionEndChanged()));
    QSignalSpy textSpy(textEdit, SIGNAL(textChanged(QString)));
    QSignalSpy cursorPositionSpy(textEdit, SIGNAL(cursorPositionChanged()));

    textEdit->insert(insertPosition, insertText);

    if (textFormat == QQuickTextEdit::RichText || (textFormat == QQuickTextEdit::AutoText && (
            Qt::mightBeRichText(text) || Qt::mightBeRichText(insertText)))) {
        QCOMPARE(textEdit->getText(0, expectedText.length()), expectedText);
    } else {
        QCOMPARE(textEdit->text(), expectedText);

    }
    QCOMPARE(textEdit->length(), expectedText.length());

    QCOMPARE(textEdit->selectionStart(), expectedSelectionStart);
    QCOMPARE(textEdit->selectionEnd(), expectedSelectionEnd);
    QCOMPARE(textEdit->cursorPosition(), expectedCursorPosition);

    if (selectionStart > selectionEnd)
        qSwap(selectionStart, selectionEnd);

    QEXPECT_FAIL("into selection", "selectionChanged signal isn't emitted on edits within selection", Continue);
    QEXPECT_FAIL("into reversed selection", "selectionChanged signal isn't emitted on edits within selection", Continue);
    QCOMPARE(selectionSpy.count() > 0, selectionChanged);
    QCOMPARE(selectionStartSpy.count() > 0, selectionStart != expectedSelectionStart);
    QEXPECT_FAIL("into reversed selection", "selectionEndChanged signal not emitted", Continue);
    QCOMPARE(selectionEndSpy.count() > 0, selectionEnd != expectedSelectionEnd);
    QCOMPARE(textSpy.count() > 0, text != expectedText);
    QCOMPARE(cursorPositionSpy.count() > 0, cursorPositionChanged);
}

void tst_qquicktextedit::remove_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QQuickTextEdit::TextFormat>("textFormat");
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

    const QString richBoldText = QStringLiteral("This is some <b>bold</b> text");
    const QString plainBoldText = QStringLiteral("This is some bold text");

    QTest::newRow("from cursor position (beginning)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0
            << 0 << 5
            << standard.at(0).mid(5)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("to cursor position (beginning)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0
            << 5 << 0
            << standard.at(0).mid(5)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("to cursor position (end)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << standard.at(0).length() << standard.at(0).length()
            << standard.at(0).length() << standard.at(0).length() - 5
            << standard.at(0).mid(0, standard.at(0).length() - 5)
            << standard.at(0).length() - 5 << standard.at(0).length() - 5 << standard.at(0).length() - 5
            << false << true;

    QTest::newRow("to cursor position (end)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << standard.at(0).length() << standard.at(0).length()
            << standard.at(0).length() - 5 << standard.at(0).length()
            << standard.at(0).mid(0, standard.at(0).length() - 5)
            << standard.at(0).length() - 5 << standard.at(0).length() - 5 << standard.at(0).length() - 5
            << false << true;

    QTest::newRow("from cursor position (middle)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 18 << 18
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 18 << 18 << 18
            << false << false;

    QTest::newRow("to cursor position (middle)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 23 << 23
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 18 << 18 << 18
            << false << true;

    QTest::newRow("after cursor position (beginning)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("before cursor position (end)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << standard.at(0).length() << standard.at(0).length()
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << standard.at(0).length() - 5 << standard.at(0).length() - 5 << standard.at(0).length() - 5
            << false << true;

    QTest::newRow("before cursor position (middle)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 23 << 23
            << 0 << 5
            << standard.at(0).mid(5)
            << 18 << 18 << 18
            << false << true;

    QTest::newRow("after cursor position (middle)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 18 << 18
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 18 << 18 << 18
            << false << false;

    QTest::newRow("before selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 14 << 19
            << 0 << 5
            << standard.at(0).mid(5)
            << 9 << 14 << 14
            << false << true;

    QTest::newRow("before reversed selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 19 << 14
            << 0 << 5
            << standard.at(0).mid(5)
            << 9 << 14 << 9
            << false << true;

    QTest::newRow("after selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 14 << 19
            << standard.at(0).length() - 5 << standard.at(0).length()
            << standard.at(0).mid(0, standard.at(0).length() - 5)
            << 14 << 19 << 19
            << false << false;

    QTest::newRow("after reversed selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 19 << 14
            << standard.at(0).length() - 5 << standard.at(0).length()
            << standard.at(0).mid(0, standard.at(0).length() - 5)
            << 14 << 19 << 14
            << false << false;

    QTest::newRow("from selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 14 << 24
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 14 << 19 << 19
            << true << true;

    QTest::newRow("from reversed selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 24 << 14
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << 14 << 19 << 14
            << true << false;

    QTest::newRow("plain text cropped beginning")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0
            << -3 << 4
            << standard.at(0).mid(4)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("plain text cropped end")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0
            << 23 << standard.at(0).length() + 8
            << standard.at(0).mid(0, 23)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("plain text cropped beginning and end")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0
            << -9 << standard.at(0).length() + 4
            << QString()
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("all rich text")
            << richBoldText << QQuickTextEdit::RichText
            << 0 << 0
            << 0 << plainBoldText.length()
            << QString()
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("rick text sub string")
            << richBoldText << QQuickTextEdit::RichText
            << 0 << 0
            << 14 << 21
            << plainBoldText.mid(0, 14) + plainBoldText.mid(21)
            << 0 << 0 << 0
            << false << false;
}

void tst_qquicktextedit::remove()
{
    QFETCH(QString, text);
    QFETCH(QQuickTextEdit::TextFormat, textFormat);
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

    QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"" + text + "\" }";
    QDeclarativeComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != 0);

    textEdit->setTextFormat(textFormat);
    textEdit->select(selectionStart, selectionEnd);

    QSignalSpy selectionSpy(textEdit, SIGNAL(selectionChanged()));
    QSignalSpy selectionStartSpy(textEdit, SIGNAL(selectionStartChanged()));
    QSignalSpy selectionEndSpy(textEdit, SIGNAL(selectionEndChanged()));
    QSignalSpy textSpy(textEdit, SIGNAL(textChanged(QString)));
    QSignalSpy cursorPositionSpy(textEdit, SIGNAL(cursorPositionChanged()));

    textEdit->remove(removeStart, removeEnd);

    if (textFormat == QQuickTextEdit::RichText
            || (textFormat == QQuickTextEdit::AutoText && Qt::mightBeRichText(text))) {
        QCOMPARE(textEdit->getText(0, expectedText.length()), expectedText);
    } else {
        QCOMPARE(textEdit->text(), expectedText);
    }
    QCOMPARE(textEdit->length(), expectedText.length());

    if (selectionStart > selectionEnd)  //
        qSwap(selectionStart, selectionEnd);

    QCOMPARE(textEdit->selectionStart(), expectedSelectionStart);
    QCOMPARE(textEdit->selectionEnd(), expectedSelectionEnd);
    QCOMPARE(textEdit->cursorPosition(), expectedCursorPosition);

    QEXPECT_FAIL("from selection", "selectionChanged signal isn't emitted on edits within selection", Continue);
    QEXPECT_FAIL("from reversed selection", "selectionChanged signal isn't emitted on edits within selection", Continue);
    QCOMPARE(selectionSpy.count() > 0, selectionChanged);
    QCOMPARE(selectionStartSpy.count() > 0, selectionStart != expectedSelectionStart);
    QEXPECT_FAIL("from reversed selection", "selectionEndChanged signal not emitted", Continue);
    QCOMPARE(selectionEndSpy.count() > 0, selectionEnd != expectedSelectionEnd);
    QCOMPARE(textSpy.count() > 0, text != expectedText);


    if (cursorPositionChanged)  //
        QVERIFY(cursorPositionSpy.count() > 0);
}


void tst_qquicktextedit::keySequence_data()
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
    QTest::newRow("select end of document")
            << standard.at(0) << QKeySequence(QKeySequence::SelectEndOfDocument) << 3 << 3
            << 44 << standard.at(0) << standard.at(0).mid(3);
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

void tst_qquicktextedit::keySequence()
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

    QString componentStr = "import QtQuick 2.0\nTextEdit { focus: true; text: \"" + text + "\" }";
    QDeclarativeComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != 0);

    QQuickCanvas canvas;
    textEdit->setParentItem(canvas.rootItem());
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(QGuiApplication::activeWindow(), &canvas);

    textEdit->select(selectionStart, selectionEnd);

    simulateKeys(&canvas, sequence);

    QCOMPARE(textEdit->cursorPosition(), cursorPosition);
    QCOMPARE(textEdit->text(), expectedText);
    QCOMPARE(textEdit->selectedText(), selectedText);
}

#define NORMAL 0
#define REPLACE_UNTIL_END 1

void tst_qquicktextedit::undo_data()
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
            expectedString << "Ensuring a ";    // ### Not present in TextInput.
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

void tst_qquicktextedit::undo()
{
    QFETCH(QStringList, insertString);
    QFETCH(IntList, insertIndex);
    QFETCH(IntList, insertMode);
    QFETCH(QStringList, expectedString);

    QString componentStr = "import QtQuick 2.0\nTextEdit { focus: true }";
    QDeclarativeComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != 0);

    QQuickCanvas canvas;
    textEdit->setParentItem(canvas.rootItem());
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(QGuiApplication::activeWindow(), &canvas);

    QVERIFY(!textEdit->canUndo());

    QSignalSpy spy(textEdit, SIGNAL(canUndoChanged()));

    int i;

// STEP 1: First build up an undo history by inserting or typing some strings...
    for (i = 0; i < insertString.size(); ++i) {
        if (insertIndex[i] > -1)
            textEdit->setCursorPosition(insertIndex[i]);

 // experimental stuff
        if (insertMode[i] == REPLACE_UNTIL_END) {
            textEdit->select(insertIndex[i], insertIndex[i] + 8);

            // This is what I actually want...
            // QTest::keyClick(testWidget, Qt::Key_End, Qt::ShiftModifier);
        }

        for (int j = 0; j < insertString.at(i).length(); j++)
            QTest::keyClick(&canvas, insertString.at(i).at(j).toLatin1());
    }

    QCOMPARE(spy.count(), 1);

// STEP 2: Next call undo several times and see if we can restore to the previous state
    for (i = 0; i < expectedString.size() - 1; ++i) {
        QCOMPARE(textEdit->text(), expectedString[i]);
        QVERIFY(textEdit->canUndo());
        textEdit->undo();
    }

// STEP 3: Verify that we have undone everything
    QVERIFY(textEdit->text().isEmpty());
    QVERIFY(!textEdit->canUndo());
    QCOMPARE(spy.count(), 2);
}

void tst_qquicktextedit::redo_data()
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

void tst_qquicktextedit::redo()
{
    QFETCH(QStringList, insertString);
    QFETCH(IntList, insertIndex);
    QFETCH(QStringList, expectedString);

    QString componentStr = "import QtQuick 2.0\nTextEdit { focus: true }";
    QDeclarativeComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != 0);

    QQuickCanvas canvas;
    textEdit->setParentItem(canvas.rootItem());
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(QGuiApplication::activeWindow(), &canvas);

    QVERIFY(!textEdit->canUndo());
    QVERIFY(!textEdit->canRedo());

    QSignalSpy spy(textEdit, SIGNAL(canRedoChanged()));

    int i;
    // inserts the diff strings at diff positions
    for (i = 0; i < insertString.size(); ++i) {
        if (insertIndex[i] > -1)
            textEdit->setCursorPosition(insertIndex[i]);
        for (int j = 0; j < insertString.at(i).length(); j++)
            QTest::keyClick(&canvas, insertString.at(i).at(j).toLatin1());
        QVERIFY(textEdit->canUndo());
        QVERIFY(!textEdit->canRedo());
    }

    QCOMPARE(spy.count(), 0);

    // undo everything
    while (!textEdit->text().isEmpty()) {
        QVERIFY(textEdit->canUndo());
        textEdit->undo();
        QVERIFY(textEdit->canRedo());
    }

    QCOMPARE(spy.count(), 1);

    for (i = 0; i < expectedString.size(); ++i) {
        QVERIFY(textEdit->canRedo());
        textEdit->redo();
        QCOMPARE(textEdit->text() , expectedString[i]);
        QVERIFY(textEdit->canUndo());
    }
    QVERIFY(!textEdit->canRedo());
    QCOMPARE(spy.count(), 2);
}

void tst_qquicktextedit::undo_keypressevents_data()
{
    QTest::addColumn<KeyList>("keys");
    QTest::addColumn<QStringList>("expectedString");

    {
        KeyList keys;
        QStringList expectedString;

        keys << "AFRAID"
                << Qt::Key_Home
                << "VERY"
                << Qt::Key_Left
                << Qt::Key_Left
                << Qt::Key_Left
                << Qt::Key_Left
                << "BE"
                << Qt::Key_End
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
        keys << "1234" << Qt::Key_Home
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
                << Qt::Key_Home
                // selecting 'AB'
                << (Qt::Key_Right | Qt::ShiftModifier) << (Qt::Key_Right | Qt::ShiftModifier)
                << Qt::Key_Delete
                << QKeySequence::Undo
                // ### Text is selected in text input
//                << Qt::Key_Right
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
                // ### Text is selected in text input
                << (Qt::Key_Left | Qt::ShiftModifier) << (Qt::Key_Left | Qt::ShiftModifier) << (Qt::Key_Left | Qt::ShiftModifier) << (Qt::Key_Left | Qt::ShiftModifier)
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
        keys << "123" << Qt::Key_Home
            // selecting '123'
             << (Qt::Key_End | Qt::ShiftModifier)
            // overwriting '123' with 'ABC'
             << "ABC";

        expectedString << "ABC";
        // ### One operation in TextInput.
        expectedString << "A";
        expectedString << "123";

        QTest::newRow("Inserts,moving,selection and overwriting") << keys << expectedString;
    }
}

void tst_qquicktextedit::undo_keypressevents()
{
    QFETCH(KeyList, keys);
    QFETCH(QStringList, expectedString);

    QString componentStr = "import QtQuick 2.0\nTextEdit { focus: true }";
    QDeclarativeComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != 0);

    QQuickCanvas canvas;
    textEdit->setParentItem(canvas.rootItem());
    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QTRY_COMPARE(QGuiApplication::activeWindow(), &canvas);

    simulateKeys(&canvas, keys);

    for (int i = 0; i < expectedString.size(); ++i) {
        QCOMPARE(textEdit->text() , expectedString[i]);
        textEdit->undo();
    }
    QVERIFY(textEdit->text().isEmpty());
}

void tst_qquicktextedit::emptytags_QTBUG_22058()
{
    QQuickView canvas(testFileUrl("qtbug-22058.qml"));
    QVERIFY(canvas.rootObject() != 0);

    canvas.show();
    canvas.requestActivateWindow();
    QTest::qWaitForWindowShown(&canvas);
    QQuickTextEdit *input = qobject_cast<QQuickTextEdit *>(qvariant_cast<QObject *>(canvas.rootObject()->property("inputField")));
    QVERIFY(input->hasActiveFocus());

    QInputMethodEvent event("", QList<QInputMethodEvent::Attribute>());
    event.setCommitString("<b>Bold<");
    QGuiApplication::sendEvent(input, &event);
    QCOMPARE(input->text(), QString("<b>Bold<"));
    event.setCommitString(">");
    QGuiApplication::sendEvent(input, &event);
    QCOMPARE(input->text(), QString("<b>Bold<>"));
}

QTEST_MAIN(tst_qquicktextedit)

#include "tst_qquicktextedit.moc"
