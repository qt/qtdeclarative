// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <qtest.h>
#include <QtTest/QSignalSpy>
#include <QtQuickTestUtils/private/testhttpserver_p.h>
#include <math.h>
#include <QFile>
#include <QtQuick/QQuickTextDocument>
#include <QtQuickTest/QtQuickTest>
#include <QTextDocument>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlexpression.h>
#include <QtQml/qqmlcomponent.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/private/qguiapplication_p.h>
#include <private/qquickflickable_p.h>
#include <private/qquickrectangle_p.h>
#include <private/qquicktextedit_p.h>
#include <private/qquicktextedit_p_p.h>
#include <private/qquicktext_p.h>
#include <private/qquicktextdocument_p.h>
#include <QFontMetrics>
#include <QtQuick/QQuickView>
#include <QDir>
#include <QRegularExpression>
#include <QInputMethod>
#include <QClipboard>
#include <QMimeData>
#include <private/qquicktextcontrol_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickTestUtils/private/platformquirks_p.h>
#include <QtQuickTestUtils/private/platforminputcontext_p.h>
#include <private/qinputmethod_p.h>
#include <QtGui/qstylehints.h>
#include <qmath.h>

Q_DECLARE_METATYPE(QQuickTextEdit::SelectionMode)
Q_DECLARE_METATYPE(Qt::Key)
DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

static bool isPlatformWayland()
{
    return !QGuiApplication::platformName().compare(QLatin1String("wayland"), Qt::CaseInsensitive);
}

typedef QPair<int, QChar> Key;

class tst_qquicktextedit : public QQmlDataTest

{
    Q_OBJECT
public:
    tst_qquicktextedit();

private slots:
    void initTestCase() override;

    void cleanup();
    void text();
    void width();
    void wrap();
    void textFormat();
    void lineCount_data();
    void lineCount();

    // ### these tests may be trivial
    void hAlign();
    void hAlign_RightToLeft();
    void hAlignVisual();
    void vAlign();
    void font();
    void color();
    void textMargin();
    void persistentSelection();
    void selectionOnFocusOut();
    void focusOnPress();
    void selection();
    void overwriteMode();
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
    void mouseSelectionMode_accessors();
    void selectByMouse();
    void selectByKeyboard();
#if QT_CONFIG(shortcut)
    void keyboardSelection_data();
    void keyboardSelection();
#endif
    void renderType();
    void inputMethodHints();

    void positionAt_data();
    void positionAt();

    void linkHover();
    void linkInteraction();

    void cursorDelegate_data();
    void cursorDelegate();
    void remoteCursorDelegate();
    void cursorVisible();
    void delegateLoading_data();
    void delegateLoading();
    void cursorDelegateHeight();
    void navigation();
    void readOnly();
#if QT_CONFIG(clipboard)
    void copyAndPaste();
    void canPaste();
    void canPasteEmpty();
    void middleClickPaste();
#endif
    void textInput();
    void inputMethodUpdate();
    void openInputPanel();
    void geometrySignals();
#if QT_CONFIG(clipboard)
    void pastingRichText_QTBUG_14003();
#endif
    void implicitSize_data();
    void implicitSize();
    void implicitSize_QTBUG_63153();
    void contentSize();
    void boundingRect();
    void clipRect();
    void implicitSizeBinding_data();
    void implicitSizeBinding();
    void largeTextObservesViewport_data();
    void largeTextObservesViewport();
    void largeTextSelection();
    void renderingAroundSelection();

    void signal_editingfinished();

    void preeditCursorRectangle();
    void inputMethodComposing();
    void cursorRectangleSize_data();
    void cursorRectangleSize();

    void getText_data();
    void getText();
    void getFormattedText_data();
    void getFormattedText();
    void append_data();
    void append();
    void insert_data();
    void insert();
    void remove_data();
    void remove();
#if QT_CONFIG(shortcut)
    void keySequence_data();
    void keySequence();
#endif

    void undo_data();
    void undo();
    void redo_data();
    void redo();
#if QT_CONFIG(shortcut)
    void undo_keypressevents_data();
    void undo_keypressevents();
#endif
    void clear();

    void baseUrl();
    void embeddedImages();
    void embeddedImages_data();

    void emptytags_QTBUG_22058();
    void cursorRectangle_QTBUG_38947();
    void textCached_QTBUG_41583();
    void doubleSelect_QTBUG_38704();

    void padding();
    void paddingAndWrap();
    void QTBUG_51115_readOnlyResetsSelection();
    void keys_shortcutoverride();

    void transparentSelectionColor();

    void inFlickableMouse_data();
    void inFlickableMouse();
    void inFlickableTouch_data();
    void inFlickableTouch();

    void keyEventPropagation();

    void markdown();
#if QT_CONFIG(clipboard)
    void pasteHtmlIntoMarkdown();
#endif

    void touchscreenDoesNotSelect_data();
    void touchscreenDoesNotSelect();
    void touchscreenSetsFocusAndMovesCursor();

    void longPressInputMethod();

    void rtlAlignmentInColumnLayout_QTBUG_112858();

    void resizeTextEditPolish();
private:
    void simulateKeys(QWindow *window, const QList<Key> &keys);
#if QT_CONFIG(shortcut)
    void simulateKeys(QWindow *window, const QKeySequence &sequence);
#endif

    void simulateKey(QWindow *, int key, Qt::KeyboardModifiers modifiers = {});
    bool isMainFontFixed();
    static bool hasWindowActivation();

    QStringList standard;
    QStringList richText;

    QStringList hAlignmentStrings;
    QStringList vAlignmentStrings;

    QList<Qt::Alignment> vAlignments;
    QList<Qt::Alignment> hAlignments;

    QStringList colorStrings;

    QQmlEngine engine;

    QPointingDevice *touchDevice = QTest::createTouchDevice();
};

typedef QList<int> IntList;
Q_DECLARE_METATYPE(IntList)

typedef QList<Key> KeyList;
Q_DECLARE_METATYPE(KeyList)

Q_DECLARE_METATYPE(QQuickTextEdit::HAlignment)
Q_DECLARE_METATYPE(QQuickTextEdit::VAlignment)
Q_DECLARE_METATYPE(QQuickTextEdit::TextFormat)

void tst_qquicktextedit::simulateKeys(QWindow *window, const QList<Key> &keys)
{
    for (int i = 0; i < keys.size(); ++i) {
        const int key = keys.at(i).first;
        const int modifiers = key & Qt::KeyboardModifierMask;
        const QString text = !keys.at(i).second.isNull() ? QString(keys.at(i).second) : QString();

        QKeyEvent press(QEvent::KeyPress, Qt::Key(key), Qt::KeyboardModifiers(modifiers), text);
        QKeyEvent release(QEvent::KeyRelease, Qt::Key(key), Qt::KeyboardModifiers(modifiers), text);

        QGuiApplication::sendEvent(window, &press);
        QGuiApplication::sendEvent(window, &release);
    }
}

#if QT_CONFIG(shortcut)

void tst_qquicktextedit::simulateKeys(QWindow *window, const QKeySequence &sequence)
{
    for (int i = 0; i < sequence.count(); ++i)
        QTest::keyClick(window, sequence[i].key(), sequence[i].keyboardModifiers());
}

QList<Key> &operator <<(QList<Key> &keys, const QKeySequence &sequence)
{
    for (int i = 0; i < sequence.count(); ++i)
        keys << Key(sequence[i].toCombined(), QChar());
    return keys;
}

#endif // QT_CONFIG(shortcut)

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
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
    qRegisterMetaType<QQuickTextEdit::TextFormat>();
    qRegisterMetaType<QQuickTextEdit::SelectionMode>();

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

class NodeCheckerTextEdit : public QQuickTextEdit
{
public:
    NodeCheckerTextEdit(QQuickItem *parent = nullptr) : QQuickTextEdit(parent) {}

    void populateLinePositions(QSGNode *node)
    {
        sortedLinePositions.clear();
        lastLinePosition = 0;
        QSGNode *ch = node->firstChild();
        while (ch != node->lastChild()) {
            QCOMPARE(ch->type(), QSGNode::TransformNodeType);
            QSGTransformNode *tn = static_cast<QSGTransformNode *>(ch);
            int y = 0;
            if (!tn->matrix().isIdentity())
                y = tn->matrix().column(3).y();
            if (tn->childCount() == 0) {
                // A TransformNode with no children is a waste of memory.
                // So far, QQuickTextEdit still creates a couple of extras.
                qCDebug(lcTests) << "ignoring leaf TransformNode" << tn << "@ y" << y;
            } else {
                qCDebug(lcTests) << "child" << tn << "@ y" << y << "has children" << tn->childCount();
                sortedLinePositions.append(y);
                lastLinePosition = qMax(lastLinePosition, y);
            }
            ch = ch->nextSibling();
        }
        std::sort(sortedLinePositions.begin(), sortedLinePositions.end());
    }

    QSGNode *updatePaintNode(QSGNode *node, UpdatePaintNodeData *data) override
    {
       QSGNode *ret = QQuickTextEdit::updatePaintNode(node, data);
       qCDebug(lcTests) << "updated root node" << ret;
       populateLinePositions(ret);
       return ret;
    }

    QList<int> sortedLinePositions;
    int lastLinePosition;
};

void tst_qquicktextedit::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlRegisterType<NodeCheckerTextEdit>("Qt.test", 1, 0, "NodeCheckerTextEdit");
}

void tst_qquicktextedit::cleanup()
{
    // ensure not even skipped tests with custom input context leave it dangling
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = nullptr;
}

void tst_qquicktextedit::text()
{
    {
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData("import QtQuick 2.0\nTextEdit {  text: \"\"  }", QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->text(), QString(""));
        QCOMPARE(textEditObject->length(), 0);
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"" + standard.at(i) + "\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->text(), standard.at(i));
        QCOMPARE(textEditObject->length(), standard.at(i).size());
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"" + richText.at(i) + "\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());

        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);

        QString expected = richText.at(i);
        expected.replace(QRegularExpression("\\\\(.)"),"\\1");
        QCOMPARE(textEditObject->text(), expected);
        QCOMPARE(textEditObject->length(), expected.size());
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.RichText; text: \"" + standard.at(i) + "\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);

        QString actual = textEditObject->text();
        QString expected = standard.at(i);
        actual.remove("\n");
        actual.remove(QRegularExpression(".*<body[^>]*>"));
        actual.remove(QRegularExpression("(<[^>]*>)+"));
        expected.remove("\n");
        QCOMPARE(actual.simplified(), expected);
        QCOMPARE(textEditObject->length(), expected.size());
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.RichText; text: \"" + richText.at(i) + "\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QString actual = textEditObject->text();
        QString expected = richText.at(i);
        actual.remove("\n");
        actual.replace(QRegularExpression(".*<body[^>]*>"),"");
        actual.replace(QRegularExpression("(<[^>]*>)+"),"<>");
        expected.replace(QRegularExpression("(<[^>]*>)+"),"<>");
        QCOMPARE(actual.simplified(),expected.simplified());

        expected.replace("<>", " ");
        QCOMPARE(textEditObject->length(), expected.simplified().size());
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.AutoText; text: \"" + standard.at(i) + "\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->text(), standard.at(i));
        QCOMPARE(textEditObject->length(), standard.at(i).size());
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.AutoText; text: \"" + richText.at(i) + "\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QString actual = textEditObject->text();
        QString expected = richText.at(i);
        actual.remove("\n");
        actual.replace(QRegularExpression(".*<body[^>]*>"),"");
        actual.replace(QRegularExpression("(<[^>]*>)+"),"<>");
        expected.replace(QRegularExpression("(<[^>]*>)+"),"<>");
        QCOMPARE(actual.simplified(),expected.simplified());

        expected.replace("<>", " ");
        QCOMPARE(textEditObject->length(), expected.simplified().size());
    }
}

void tst_qquicktextedit::width()
{
    // uses Font metrics to find the width for standard and document to find the width for rich
    {
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData("import QtQuick 2.0\nTextEdit {  text: \"\" }", QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->width(), 0.0);
    }

    bool requiresUnhintedMetrics = !qmlDisableDistanceField();

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"" + standard.at(i) + "\" }";
        QQmlComponent texteditComponent(&engine);
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

        qreal metricWidth = layout.boundingRect().width();

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->width(), metricWidth);
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QTextDocument document;
        document.setHtml(richText.at(i));
        document.setDocumentMargin(0);
        if (requiresUnhintedMetrics)
            document.setUseDesignMetrics(true);

        qreal documentWidth = document.idealWidth();

        QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.RichText; text: \"" + richText.at(i) + "\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->width(), documentWidth);
    }
}

void tst_qquicktextedit::wrap()
{
    // for specified width and wrap set true
    {
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData("import QtQuick 2.0\nTextEdit {  text: \"\"; wrapMode: TextEdit.WordWrap; width: 300 }", QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->width(), 300.);
    }

    for (int i = 0; i < standard.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  wrapMode: TextEdit.WordWrap; width: 300; text: \"" + standard.at(i) + "\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->width(), 300.);
    }

    for (int i = 0; i < richText.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  wrapMode: TextEdit.WordWrap; width: 300; text: \"" + richText.at(i) + "\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->width(), 300.);
    }
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\n TextEdit {}", QUrl());
        QScopedPointer<QObject> object(component.create());
        QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(object.data());
        QVERIFY(edit);

        QSignalSpy spy(edit, SIGNAL(wrapModeChanged()));

        QCOMPARE(edit->wrapMode(), QQuickTextEdit::NoWrap);

        edit->setWrapMode(QQuickTextEdit::Wrap);
        QCOMPARE(edit->wrapMode(), QQuickTextEdit::Wrap);
        QCOMPARE(spy.size(), 1);

        edit->setWrapMode(QQuickTextEdit::Wrap);
        QCOMPARE(spy.size(), 1);

        edit->setWrapMode(QQuickTextEdit::NoWrap);
        QCOMPARE(edit->wrapMode(), QQuickTextEdit::NoWrap);
        QCOMPARE(spy.size(), 2);
    }

}

void tst_qquicktextedit::textFormat()
{
    {
        QQmlComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nTextEdit { text: \"Hello\"; textFormat: Text.RichText }", QUrl::fromLocalFile(""));
        QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit*>(textComponent.create());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->textFormat(), QQuickTextEdit::RichText);
    }
    {
        QQmlComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nTextEdit { text: \"<b>Hello</b>\"; textFormat: Text.PlainText }", QUrl::fromLocalFile(""));
        QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit*>(textComponent.create());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->textFormat(), QQuickTextEdit::PlainText);
    }
    {
        QQmlComponent textComponent(&engine);
        textComponent.setData("import QtQuick 2.0\nTextEdit { text: \"_Hello_\"; textFormat: Text.MarkdownText }", QUrl::fromLocalFile(""));
        QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit*>(textComponent.create());

        QVERIFY(textObject != nullptr);
        QCOMPARE(textObject->textFormat(), QQuickTextEdit::MarkdownText);
        QVERIFY(textObject->textDocument());
        auto doc = textObject->textDocument()->textDocument();
        QVERIFY(doc);
        QTextCursor cursor(doc);
        QVERIFY(cursor.charFormat().fontUnderline());
    }
    {
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\n TextEdit {}", QUrl());
        QScopedPointer<QObject> object(component.create());
        QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(object.data());
        QVERIFY(edit);

        QSignalSpy spy(edit, &QQuickTextEdit::textFormatChanged);

        QCOMPARE(edit->textFormat(), QQuickTextEdit::PlainText);

        edit->setTextFormat(QQuickTextEdit::RichText);
        QCOMPARE(edit->textFormat(), QQuickTextEdit::RichText);
        QCOMPARE(spy.size(), 1);

        edit->setTextFormat(QQuickTextEdit::RichText);
        QCOMPARE(spy.size(), 1);

        edit->setTextFormat(QQuickTextEdit::PlainText);
        QCOMPARE(edit->textFormat(), QQuickTextEdit::PlainText);
        QCOMPARE(spy.size(), 2);

        edit->setTextFormat(QQuickTextEdit::MarkdownText);
        QCOMPARE(edit->textFormat(), QQuickTextEdit::MarkdownText);
        QCOMPARE(spy.size(), 3);
    }
}

static int calcLineCount(QTextDocument* doc)
{
    int subLines = 0;
    for (QTextBlock it = doc->begin(); it != doc->end(); it = it.next()) {
        QTextLayout *layout = it.layout();
        if (!layout)
            continue;
        subLines += layout->lineCount()-1;
    }
    return doc->lineCount() + subLines;
}

void tst_qquicktextedit::lineCount_data()
{
    QTest::addColumn<QStringList>("texts");
    QTest::newRow("plaintext") << standard;
    QTest::newRow("richtext") << richText;
}

void tst_qquicktextedit::lineCount()
{
    QFETCH(const QStringList, texts);

    for (const QString& text : texts) {
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.0\nTextEdit { }", QUrl());

        QQuickTextEdit *textedit = qobject_cast<QQuickTextEdit*>(component.create());
        QVERIFY(textedit);

        QTextDocument *doc = QQuickTextEditPrivate::get(textedit)->document;
        QVERIFY(doc);

        textedit->setText(text);

        textedit->setWidth(100.0);
        QCOMPARE(textedit->lineCount(), calcLineCount(doc));

        textedit->setWrapMode(QQuickTextEdit::Wrap);
        QCOMPARE(textedit->lineCount(), calcLineCount(doc));

        textedit->setWidth(50.0);
        QCOMPARE(textedit->lineCount(), calcLineCount(doc));

        textedit->setWrapMode(QQuickTextEdit::NoWrap);
        QCOMPARE(textedit->lineCount(), calcLineCount(doc));
    }
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
            QQmlComponent texteditComponent(&engine);
            texteditComponent.setData(componentStr.toLatin1(), QUrl());
            QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

            QVERIFY(textEditObject != nullptr);
            QCOMPARE((int)textEditObject->hAlign(), (int)hAlignments.at(j));
        }
    }

    for (int i = 0; i < richText.size(); i++)
    {
        for (int j=0; j < hAlignmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nTextEdit {  horizontalAlignment: \"" + hAlignmentStrings.at(j) + "\"; text: \"" + richText.at(i) + "\" }";
            QQmlComponent texteditComponent(&engine);
            texteditComponent.setData(componentStr.toLatin1(), QUrl());
            QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

            QVERIFY(textEditObject != nullptr);
            QCOMPARE((int)textEditObject->hAlign(), (int)hAlignments.at(j));
        }
    }

}

void tst_qquicktextedit::hAlign_RightToLeft()
{
    PlatformInputContext platformInputContext;
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = &platformInputContext;

    QQuickView window(testFileUrl("horizontalAlignment_RightToLeft.qml"));
    QQuickTextEdit *textEdit = window.rootObject()->findChild<QQuickTextEdit*>("text");
    QVERIFY(textEdit != nullptr);
    window.showNormal();

    const QString rtlText = textEdit->text();

    // implicit alignment should follow the reading direction of text
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > window.width()/2);

    // explicitly left aligned
    textEdit->setHAlign(QQuickTextEdit::AlignLeft);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignLeft);
    QVERIFY(textEdit->positionToRectangle(0).x() < window.width()/2);

    // explicitly right aligned
    textEdit->setHAlign(QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > window.width()/2);

    QString textString = textEdit->text();
    textEdit->setText(QString("<i>") + textString + QString("</i>"));
    textEdit->resetHAlign();

    // implicitly aligned rich text should follow the reading direction of RTL text
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->effectiveHAlign(), textEdit->hAlign());
    QVERIFY(textEdit->positionToRectangle(0).x() > window.width()/2);

    // explicitly left aligned rich text
    textEdit->setHAlign(QQuickTextEdit::AlignLeft);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignLeft);
    QCOMPARE(textEdit->effectiveHAlign(), textEdit->hAlign());
    QVERIFY(textEdit->positionToRectangle(0).x() < window.width()/2);

    // explicitly right aligned rich text
    textEdit->setHAlign(QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->effectiveHAlign(), textEdit->hAlign());
    QVERIFY(textEdit->positionToRectangle(0).x() > window.width()/2);

    textEdit->setText(textString);

    // explicitly center aligned
    textEdit->setHAlign(QQuickTextEdit::AlignHCenter);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignHCenter);
    QVERIFY(textEdit->positionToRectangle(0).x() > window.width()/2);

    // reseted alignment should go back to following the text reading direction
    textEdit->resetHAlign();
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > window.width()/2);

    // mirror the text item
    QQuickItemPrivate::get(textEdit)->setLayoutMirror(true);

    // mirrored implicit alignment should continue to follow the reading direction of the text
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->effectiveHAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > window.width()/2);

    // mirrored explicitly right aligned behaves as left aligned
    textEdit->setHAlign(QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->effectiveHAlign(), QQuickTextEdit::AlignLeft);
    QVERIFY(textEdit->positionToRectangle(0).x() < window.width()/2);

    // mirrored explicitly left aligned behaves as right aligned
    textEdit->setHAlign(QQuickTextEdit::AlignLeft);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignLeft);
    QCOMPARE(textEdit->effectiveHAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > window.width()/2);

    // disable mirroring
    QQuickItemPrivate::get(textEdit)->setLayoutMirror(false);
    textEdit->resetHAlign();

    // English text should be implicitly left aligned
    textEdit->setText("Hello world!");
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignLeft);
    QVERIFY(textEdit->positionToRectangle(0).x() < window.width()/2);

    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QVERIFY(textEdit->hasActiveFocus());

    textEdit->setText(QString());
    { QInputMethodEvent ev(rtlText, QList<QInputMethodEvent::Attribute>()); QGuiApplication::sendEvent(textEdit, &ev); }
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    { QInputMethodEvent ev("Hello world!", QList<QInputMethodEvent::Attribute>()); QGuiApplication::sendEvent(textEdit, &ev); }
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignLeft);

    // Clear pre-edit text.  TextEdit should maybe do this itself on setText, but that may be
    // redundant as an actual input method may take care of it.
    { QInputMethodEvent ev; QGuiApplication::sendEvent(textEdit, &ev); }

    // empty text with implicit alignment follows the system locale-based
    // keyboard input direction from qApp->inputMethod()->inputDirection
    textEdit->setText("");
    platformInputContext.setInputDirection(Qt::LeftToRight);
    QCOMPARE(qApp->inputMethod()->inputDirection(), Qt::LeftToRight);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignLeft);
    QVERIFY(textEdit->positionToRectangle(0).x() < window.width()/2);

    QSignalSpy cursorRectangleSpy(textEdit, SIGNAL(cursorRectangleChanged()));

    platformInputContext.setInputDirection(Qt::RightToLeft);
    QCOMPARE(cursorRectangleSpy.size(), 1);
    QCOMPARE(qApp->inputMethod()->inputDirection(), Qt::RightToLeft);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > window.width()/2);

    // neutral text follows also input method direction
    textEdit->resetHAlign();
    textEdit->setText(" ()((=<>");
    platformInputContext.setInputDirection(Qt::LeftToRight);
    QCOMPARE(textEdit->effectiveHAlign(), QQuickTextEdit::AlignLeft);
    QVERIFY(textEdit->cursorRectangle().left() < window.width()/2);
    platformInputContext.setInputDirection(Qt::RightToLeft);
    QCOMPARE(textEdit->effectiveHAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->cursorRectangle().left() > window.width()/2);

    // set input direction while having content
    platformInputContext.setInputDirection(Qt::LeftToRight);
    textEdit->setText("a");
    textEdit->setCursorPosition(1);
    platformInputContext.setInputDirection(Qt::RightToLeft);
    QTest::keyClick(&window, Qt::Key_Backspace);
    QVERIFY(textEdit->text().isEmpty());
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->cursorRectangle().left() > window.width()/2);

    // input direction changed while not having focus
    platformInputContext.setInputDirection(Qt::LeftToRight);
    textEdit->setFocus(false);
    platformInputContext.setInputDirection(Qt::RightToLeft);
    textEdit->setFocus(true);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->cursorRectangle().left() > window.width()/2);

    textEdit->setHAlign(QQuickTextEdit::AlignRight);
    QCOMPARE(textEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(textEdit->positionToRectangle(0).x() > window.width()/2);

    // make sure editor doesn't rely on input for updating size
    QQuickTextEdit *emptyEdit = window.rootObject()->findChild<QQuickTextEdit*>("emptyTextEdit");
    QVERIFY(emptyEdit != nullptr);
    platformInputContext.setInputDirection(Qt::RightToLeft);
    emptyEdit->setFocus(true);
    QCOMPARE(emptyEdit->hAlign(), QQuickTextEdit::AlignRight);
    QVERIFY(emptyEdit->cursorRectangle().left() > window.width()/2);
}


static int numberOfNonWhitePixels(int fromX, int toX, const QImage &image)
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

void tst_qquicktextedit::hAlignVisual()
{
    QQuickView view(testFileUrl("hAlignVisual.qml"));
    view.setFlags(view.flags() | Qt::WindowStaysOnTopHint); // Prevent being obscured by other windows.
    view.showNormal();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QQuickText *text = view.rootObject()->findChild<QQuickText*>("textItem");
    QVERIFY(text != nullptr);

    // Try to check whether alignment works by checking the number of black
    // pixels in the thirds of the grabbed image.
    const int windowWidth = view.width();
    const int textWidth = qCeil(text->implicitWidth());
    QVERIFY2(textWidth < windowWidth, "System font too large.");
    const int sectionWidth = textWidth / 3;
    const int centeredSection1 = (windowWidth - textWidth) / 2;
    const int centeredSection2 = centeredSection1 + sectionWidth;
    const int centeredSection3 = centeredSection2 + sectionWidth;
    const int centeredSection3End = centeredSection3 + sectionWidth;

    {
        if (QGuiApplication::platformName() == QLatin1String("minimal"))
            QSKIP("Skipping due to grabWindow not functional on minimal platforms");

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
        QImage image = view.grabWindow();
        const int left = numberOfNonWhitePixels(centeredSection1, centeredSection2, image);
        const int mid = numberOfNonWhitePixels(centeredSection2, centeredSection3, image);
        const int right = numberOfNonWhitePixels(centeredSection3, centeredSection3End, image);
        QVERIFY2(left < mid, msgNotLessThan(left, mid).constData());
        QVERIFY2(mid < right, msgNotLessThan(mid, right).constData());
    }

    text->setWidth(200);

    {
        // Left Align
        QImage image = view.grabWindow();
        int x = qCeil(text->implicitWidth() * view.devicePixelRatio());
        int left = numberOfNonWhitePixels(0, x, image);
        int right = numberOfNonWhitePixels(x, image.width() - x, image);
        QVERIFY2(left > 0, msgNotGreaterThan(left, 0).constData());
        QCOMPARE(right, 0);
    }
    {
        // HCenter Align
        text->setHAlign(QQuickText::AlignHCenter);
        QImage image = view.grabWindow();
        int x1 = qFloor(image.width() - text->implicitWidth() * view.devicePixelRatio()) / 2;
        int x2 = image.width() - x1;
        int left = numberOfNonWhitePixels(0, x1, image);
        int mid = numberOfNonWhitePixels(x1, x2 - x1, image);
        int right = numberOfNonWhitePixels(x2, image.width() - x2, image);
        QCOMPARE(left, 0);
        QVERIFY2(mid > 0, msgNotGreaterThan(left, 0).constData());
        QCOMPARE(right, 0);
    }
    {
        // Right Align
        text->setHAlign(QQuickText::AlignRight);
        QImage image = view.grabWindow();
        int x = image.width() - qCeil(text->implicitWidth() * view.devicePixelRatio());
        int left = numberOfNonWhitePixels(0, x, image);
        int right = numberOfNonWhitePixels(x, image.width() - x, image);
        QCOMPARE(left, 0);
        QVERIFY2(right > 0, msgNotGreaterThan(left, 0).constData());
    }
}

void tst_qquicktextedit::vAlign()
{
    //test one align each, and then test if two align fails.

    for (int i = 0; i < standard.size(); i++)
    {
        for (int j=0; j < vAlignmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nTextEdit {  verticalAlignment: \"" + vAlignmentStrings.at(j) + "\"; text: \"" + standard.at(i) + "\" }";
            QQmlComponent texteditComponent(&engine);
            texteditComponent.setData(componentStr.toLatin1(), QUrl());
            QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

            QVERIFY(textEditObject != nullptr);
            QCOMPARE((int)textEditObject->vAlign(), (int)vAlignments.at(j));
        }
    }

    for (int i = 0; i < richText.size(); i++)
    {
        for (int j=0; j < vAlignmentStrings.size(); j++)
        {
            QString componentStr = "import QtQuick 2.0\nTextEdit {  verticalAlignment: \"" + vAlignmentStrings.at(j) + "\"; text: \"" + richText.at(i) + "\" }";
            QQmlComponent texteditComponent(&engine);
            texteditComponent.setData(componentStr.toLatin1(), QUrl());
            QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

            QVERIFY(textEditObject != nullptr);
            QCOMPARE((int)textEditObject->vAlign(), (int)vAlignments.at(j));
        }
    }

    QQmlComponent texteditComponent(&engine);
    texteditComponent.setData(
                "import QtQuick 2.0\n"
                "TextEdit { width: 100; height: 100; text: \"Hello World\" }", QUrl());
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

    QVERIFY(textEditObject != nullptr);

    QCOMPARE(textEditObject->vAlign(), QQuickTextEdit::AlignTop);
    QVERIFY(textEditObject->cursorRectangle().bottom() < 50);
    QVERIFY(textEditObject->positionToRectangle(0).bottom() < 50);

    // bottom aligned
    textEditObject->setVAlign(QQuickTextEdit::AlignBottom);
    QCOMPARE(textEditObject->vAlign(), QQuickTextEdit::AlignBottom);
    QVERIFY(textEditObject->cursorRectangle().top() > 50);
    QVERIFY(textEditObject->positionToRectangle(0).top() > 50);

    // explicitly center aligned
    textEditObject->setVAlign(QQuickTextEdit::AlignVCenter);
    QCOMPARE(textEditObject->vAlign(), QQuickTextEdit::AlignVCenter);
    QVERIFY(textEditObject->cursorRectangle().top() < 50);
    QVERIFY(textEditObject->cursorRectangle().bottom() > 50);
    QVERIFY(textEditObject->positionToRectangle(0).top() < 50);
    QVERIFY(textEditObject->positionToRectangle(0).bottom() > 50);

    // Test vertical alignment after resizing the height.
    textEditObject->setHeight(textEditObject->height() - 20);
    QVERIFY(textEditObject->cursorRectangle().top() < 40);
    QVERIFY(textEditObject->cursorRectangle().bottom() > 40);
    QVERIFY(textEditObject->positionToRectangle(0).top() < 40);
    QVERIFY(textEditObject->positionToRectangle(0).bottom() > 40);

    textEditObject->setHeight(textEditObject->height() + 40);
    QVERIFY(textEditObject->cursorRectangle().top() < 60);
    QVERIFY(textEditObject->cursorRectangle().bottom() > 60);
    QVERIFY(textEditObject->positionToRectangle(0).top() < 60);
    QVERIFY(textEditObject->positionToRectangle(0).bottom() > 60);
}

void tst_qquicktextedit::font()
{
    //test size, then bold, then italic, then family
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  font.pointSize: 40; text: \"Hello World\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->font().pointSize(), 40);
        QCOMPARE(textEditObject->font().bold(), false);
        QCOMPARE(textEditObject->font().italic(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  font.bold: true; text: \"Hello World\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->font().bold(), true);
        QCOMPARE(textEditObject->font().italic(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  font.italic: true; text: \"Hello World\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->font().italic(), true);
        QCOMPARE(textEditObject->font().bold(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  font.family: \"Helvetica\"; text: \"Hello World\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->font().family(), QString("Helvetica"));
        QCOMPARE(textEditObject->font().bold(), false);
        QCOMPARE(textEditObject->font().italic(), false);
    }

    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  font.family: \"\"; text: \"Hello World\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->font().family(), QString(""));
    }
}

void tst_qquicktextedit::color()
{
    //test initial color
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"Hello World\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject);
        QCOMPARE(textEditObject->color(), QColor("black"));
        QCOMPARE(textEditObject->selectionColor(), QColor::fromRgba(0xFF000080));
        QCOMPARE(textEditObject->selectedTextColor(), QColor("white"));

        QSignalSpy colorSpy(textEditObject, SIGNAL(colorChanged(QColor)));
        QSignalSpy selectionColorSpy(textEditObject, SIGNAL(selectionColorChanged(QColor)));
        QSignalSpy selectedTextColorSpy(textEditObject, SIGNAL(selectedTextColorChanged(QColor)));

        textEditObject->setColor(QColor("white"));
        QCOMPARE(textEditObject->color(), QColor("white"));
        QCOMPARE(colorSpy.size(), 1);

        textEditObject->setSelectionColor(QColor("black"));
        QCOMPARE(textEditObject->selectionColor(), QColor("black"));
        QCOMPARE(selectionColorSpy.size(), 1);

        textEditObject->setSelectedTextColor(QColor("blue"));
        QCOMPARE(textEditObject->selectedTextColor(), QColor("blue"));
        QCOMPARE(selectedTextColorSpy.size(), 1);

        textEditObject->setColor(QColor("white"));
        QCOMPARE(colorSpy.size(), 1);

        textEditObject->setSelectionColor(QColor("black"));
        QCOMPARE(selectionColorSpy.size(), 1);

        textEditObject->setSelectedTextColor(QColor("blue"));
        QCOMPARE(selectedTextColorSpy.size(), 1);

        textEditObject->setColor(QColor("black"));
        QCOMPARE(textEditObject->color(), QColor("black"));
        QCOMPARE(colorSpy.size(), 2);

        textEditObject->setSelectionColor(QColor("blue"));
        QCOMPARE(textEditObject->selectionColor(), QColor("blue"));
        QCOMPARE(selectionColorSpy.size(), 2);

        textEditObject->setSelectedTextColor(QColor("white"));
        QCOMPARE(textEditObject->selectedTextColor(), QColor("white"));
        QCOMPARE(selectedTextColorSpy.size(), 2);
    }

    //test normal
    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  color: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        //qDebug() << "textEditObject: " << textEditObject->color() << "vs. " << QColor(colorStrings.at(i));
        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->color(), QColor(colorStrings.at(i)));
    }

    //test selection
    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  selectionColor: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->selectionColor(), QColor(colorStrings.at(i)));
    }

    //test selected text
    for (int i = 0; i < colorStrings.size(); i++)
    {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  selectedTextColor: \"" + colorStrings.at(i) + "\"; text: \"Hello World\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->selectedTextColor(), QColor(colorStrings.at(i)));
    }

    {
        QString colorStr = "#AA001234";
        QColor testColor("#001234");
        testColor.setAlpha(170);

        QString componentStr = "import QtQuick 2.0\nTextEdit {  color: \"" + colorStr + "\"; text: \"Hello World\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());

        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->color(), testColor);
    }
}

void tst_qquicktextedit::textMargin()
{
    for (qreal i=0; i<=10; i+=0.3) {
        QString componentStr = "import QtQuick 2.0\nTextEdit {  textMargin: " + QString::number(i) + "; text: \"Hello World\" }";
        QQmlComponent texteditComponent(&engine);
        texteditComponent.setData(componentStr.toLatin1(), QUrl());
        QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
        QVERIFY(textEditObject != nullptr);
        QCOMPARE(textEditObject->textMargin(), i);
    }
}

void tst_qquicktextedit::persistentSelection()
{
    QQuickView window(testFileUrl("persistentSelection.qml"));
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(edit);
    QVERIFY(edit->hasActiveFocus());

    QSignalSpy spy(edit, SIGNAL(persistentSelectionChanged(bool)));

    QCOMPARE(edit->persistentSelection(), false);

    edit->setPersistentSelection(false);
    QCOMPARE(edit->persistentSelection(), false);
    QCOMPARE(spy.size(), 0);

    edit->select(1, 4);
    QCOMPARE(edit->property("selected").toString(), QLatin1String("ell"));

    edit->setFocus(false);
    QCOMPARE(edit->property("selected").toString(), QString());

    edit->setFocus(true);
    QCOMPARE(edit->property("selected").toString(), QString());

    edit->setPersistentSelection(true);
    QCOMPARE(edit->persistentSelection(), true);
    QCOMPARE(spy.size(), 1);

    edit->select(1, 4);
    QCOMPARE(edit->property("selected").toString(), QLatin1String("ell"));

    edit->setFocus(false);
    QCOMPARE(edit->property("selected").toString(), QLatin1String("ell"));

    edit->setFocus(true);
    QCOMPARE(edit->property("selected").toString(), QLatin1String("ell"));

    // QTBUG-50587 (persistentSelection with readOnly)
    edit->setReadOnly(true);

    edit->setPersistentSelection(false);
    QCOMPARE(edit->persistentSelection(), false);
    QCOMPARE(spy.size(), 2);

    edit->select(1, 4);
    QCOMPARE(edit->property("selected").toString(), QLatin1String("ell"));

    edit->setFocus(false);
    QCOMPARE(edit->property("selected").toString(), QString());

    edit->setFocus(true);
    QCOMPARE(edit->property("selected").toString(), QString());

    edit->setPersistentSelection(true);
    QCOMPARE(edit->persistentSelection(), true);
    QCOMPARE(spy.size(), 3);

    edit->select(1, 4);
    QCOMPARE(edit->property("selected").toString(), QLatin1String("ell"));

    edit->setFocus(false);
    QCOMPARE(edit->property("selected").toString(), QLatin1String("ell"));

    edit->setFocus(true);
    QCOMPARE(edit->property("selected").toString(), QLatin1String("ell"));
}

void tst_qquicktextedit::selectionOnFocusOut()
{
    QQuickView window(testFileUrl("focusOutSelection.qml"));
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QPoint p1(25, 35);
    QPoint p2(25, 85);

    QQuickTextEdit *edit1 = window.rootObject()->findChild<QQuickTextEdit*>("text1");
    QQuickTextEdit *edit2 = window.rootObject()->findChild<QQuickTextEdit*>("text2");

    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QVERIFY(edit1->hasActiveFocus());
    QVERIFY(!edit2->hasActiveFocus());

    edit1->selectAll();
    QCOMPARE(edit1->selectedText(), QLatin1String("text 1"));

    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, p2);

    QCOMPARE(edit1->selectedText(), QLatin1String(""));
    QVERIFY(!edit1->hasActiveFocus());
    QVERIFY(edit2->hasActiveFocus());

    edit2->selectAll();
    QCOMPARE(edit2->selectedText(), QLatin1String("text 2"));


    edit2->setFocus(false, Qt::ActiveWindowFocusReason);
    QVERIFY(!edit2->hasActiveFocus());
    QCOMPARE(edit2->selectedText(), QLatin1String("text 2"));

    edit2->setFocus(true);
    QVERIFY(edit2->hasActiveFocus());

    edit2->setFocus(false, Qt::PopupFocusReason);
    QVERIFY(!edit2->hasActiveFocus());
    QCOMPARE(edit2->selectedText(), QLatin1String("text 2"));
}

void tst_qquicktextedit::focusOnPress()
{
    QString componentStr =
            "import QtQuick 2.0\n"
            "TextEdit {\n"
                "property bool selectOnFocus: false\n"
                "width: 100; height: 50\n"
                "activeFocusOnPress: true\n"
                "text: \"Hello World\"\n"
                "onFocusChanged: { if (focus && selectOnFocus) selectAll() }"
            " }";
    QQmlComponent texteditComponent(&engine);
    texteditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
    QVERIFY(textEditObject != nullptr);
    QCOMPARE(textEditObject->focusOnPress(), true);
    QCOMPARE(textEditObject->hasFocus(), false);

    QSignalSpy focusSpy(textEditObject, SIGNAL(focusChanged(bool)));
    QSignalSpy activeFocusSpy(textEditObject, SIGNAL(focusChanged(bool)));
    QSignalSpy activeFocusOnPressSpy(textEditObject, SIGNAL(activeFocusOnPressChanged(bool)));

    textEditObject->setFocusOnPress(true);
    QCOMPARE(textEditObject->focusOnPress(), true);
    QCOMPARE(activeFocusOnPressSpy.size(), 0);

    QQuickWindow window;
    window.resize(100, 50);
    textEditObject->setParentItem(window.contentItem());
    window.showNormal();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QCOMPARE(textEditObject->hasFocus(), false);
    QCOMPARE(textEditObject->hasActiveFocus(), false);

    QPoint centerPoint(window.width()/2, window.height()/2);
    Qt::KeyboardModifiers noModifiers;
    QTest::mousePress(&window, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QCOMPARE(textEditObject->hasFocus(), true);
    QCOMPARE(textEditObject->hasActiveFocus(), true);
    QCOMPARE(focusSpy.size(), 1);
    QCOMPARE(activeFocusSpy.size(), 1);
    QCOMPARE(textEditObject->selectedText(), QString());
    QTest::mouseRelease(&window, Qt::LeftButton, noModifiers, centerPoint);

    textEditObject->setFocusOnPress(false);
    QCOMPARE(textEditObject->focusOnPress(), false);
    QCOMPARE(activeFocusOnPressSpy.size(), 1);

    textEditObject->setFocus(false);
    QCOMPARE(textEditObject->hasFocus(), false);
    QCOMPARE(textEditObject->hasActiveFocus(), false);
    QCOMPARE(focusSpy.size(), 2);
    QCOMPARE(activeFocusSpy.size(), 2);

    // Wait for double click timeout to expire before clicking again.
    QTest::qWait(400);
    QTest::mousePress(&window, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QCOMPARE(textEditObject->hasFocus(), false);
    QCOMPARE(textEditObject->hasActiveFocus(), false);
    QCOMPARE(focusSpy.size(), 2);
    QCOMPARE(activeFocusSpy.size(), 2);
    QTest::mouseRelease(&window, Qt::LeftButton, noModifiers, centerPoint);

    textEditObject->setFocusOnPress(true);
    QCOMPARE(textEditObject->focusOnPress(), true);
    QCOMPARE(activeFocusOnPressSpy.size(), 2);

    // Test a selection made in the on(Active)FocusChanged handler isn't overwritten.
    textEditObject->setProperty("selectOnFocus", true);

    QTest::qWait(400);
    QTest::mousePress(&window, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QCOMPARE(textEditObject->hasFocus(), true);
    QCOMPARE(textEditObject->hasActiveFocus(), true);
    QCOMPARE(focusSpy.size(), 3);
    QCOMPARE(activeFocusSpy.size(), 3);
    QCOMPARE(textEditObject->selectedText(), textEditObject->text());
    QTest::mouseRelease(&window, Qt::LeftButton, noModifiers, centerPoint);
}

void tst_qquicktextedit::selection()
{
    QString testStr = standard[0];//TODO: What should happen for multiline/rich text?
    QString componentStr = "import QtQuick 2.0\nTextEdit {  text: \""+ testStr +"\"; }";
    QQmlComponent texteditComponent(&engine);
    texteditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
    QVERIFY(textEditObject != nullptr);


    //Test selection follows cursor
    for (int i=0; i<= testStr.size(); i++) {
        textEditObject->setCursorPosition(i);
        QCOMPARE(textEditObject->cursorPosition(), i);
        QCOMPARE(textEditObject->selectionStart(), i);
        QCOMPARE(textEditObject->selectionEnd(), i);
        QVERIFY(textEditObject->selectedText().isNull());
    }

    textEditObject->setCursorPosition(0);
    QCOMPARE(textEditObject->cursorPosition(), 0);
    QCOMPARE(textEditObject->selectionStart(), 0);
    QCOMPARE(textEditObject->selectionEnd(), 0);
    QVERIFY(textEditObject->selectedText().isNull());

    // Verify invalid positions are ignored.
    textEditObject->setCursorPosition(-1);
    QCOMPARE(textEditObject->cursorPosition(), 0);
    QCOMPARE(textEditObject->selectionStart(), 0);
    QCOMPARE(textEditObject->selectionEnd(), 0);
    QVERIFY(textEditObject->selectedText().isNull());

    textEditObject->setCursorPosition(textEditObject->text().size()+1);
    QCOMPARE(textEditObject->cursorPosition(), 0);
    QCOMPARE(textEditObject->selectionStart(), 0);
    QCOMPARE(textEditObject->selectionEnd(), 0);
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
    QCOMPARE(textEditObject->cursorPosition(), 0);
    QCOMPARE(textEditObject->selectionStart(), 0);
    QCOMPARE(textEditObject->selectionEnd(), 0);
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
    QCOMPARE(textEditObject->selectedText().size(), 10);
    textEditObject->select(-10,0);
    QCOMPARE(textEditObject->selectedText().size(), 10);
    textEditObject->select(100,101);
    QCOMPARE(textEditObject->selectedText().size(), 10);
    textEditObject->select(0,-10);
    QCOMPARE(textEditObject->selectedText().size(), 10);
    textEditObject->select(0,100);
    QCOMPARE(textEditObject->selectedText().size(), 10);

    textEditObject->deselect();
    QVERIFY(textEditObject->selectedText().isNull());
    textEditObject->select(0,10);
    QCOMPARE(textEditObject->selectedText().size(), 10);
    textEditObject->deselect();
    QVERIFY(textEditObject->selectedText().isNull());
}

void tst_qquicktextedit::overwriteMode()
{
    QString componentStr = "import QtQuick 2.0\nTextEdit { focus: true; }";
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    QSignalSpy spy(textEdit, SIGNAL(overwriteModeChanged(bool)));

    QQuickWindow window;
    textEdit->setParentItem(window.contentItem());
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QVERIFY(textEdit->hasActiveFocus());

    textEdit->setOverwriteMode(true);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(true, textEdit->overwriteMode());
    textEdit->setOverwriteMode(false);
    QCOMPARE(spy.size(), 2);
    QCOMPARE(false, textEdit->overwriteMode());

    QVERIFY(!textEdit->overwriteMode());
    QString insertString = "Some first text";
    for (int j = 0; j < insertString.size(); j++)
        QTest::keyClick(&window, insertString.at(j).toLatin1());

    QCOMPARE(textEdit->text(), QString("Some first text"));

    textEdit->setOverwriteMode(true);
    QCOMPARE(spy.size(), 3);
    textEdit->setCursorPosition(5);

    insertString = "shiny";
    for (int j = 0; j < insertString.size(); j++)
        QTest::keyClick(&window, insertString.at(j).toLatin1());
    QCOMPARE(textEdit->text(), QString("Some shiny text"));

    textEdit->setCursorPosition(textEdit->text().size());
    QTest::keyClick(&window, Qt::Key_Enter);

    textEdit->setOverwriteMode(false);
    QCOMPARE(spy.size(), 4);

    insertString = "Second paragraph";

    for (int j = 0; j < insertString.size(); j++)
        QTest::keyClick(&window, insertString.at(j).toLatin1());
    QCOMPARE(textEdit->lineCount(), 2);

    textEdit->setCursorPosition(15);

    QCOMPARE(textEdit->cursorPosition(), 15);

    textEdit->setOverwriteMode(true);
    QCOMPARE(spy.size(), 5);

    insertString = " blah";
    for (int j = 0; j < insertString.size(); j++)
        QTest::keyClick(&window, insertString.at(j).toLatin1());
    QCOMPARE(textEdit->lineCount(), 2);

    QCOMPARE(textEdit->text(), QString("Some shiny text blah\nSecond paragraph"));
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

    const char16_t arabic_str[] = { 0x0638, 0x0643, 0x0646, 0x0647, 0x0633, 0x0638, 0x0643, 0x0646, 0x0647, 0x0633, 0x0647};
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
    QCOMPARE(textEdit.isRightToLeft(text.size()-2, text.size()-1), text.mid(text.size()-2, text.size()-1).isRightToLeft());
    QCOMPARE(textEdit.isRightToLeft(text.size()/2, text.size()/2 + 1), text.mid(text.size()/2, text.size()/2 + 1).isRightToLeft());
    QCOMPARE(textEdit.isRightToLeft(0,text.size()/4), text.mid(0,text.size()/4).isRightToLeft());
    QCOMPARE(textEdit.isRightToLeft(text.size()/4,3*text.size()/4), text.mid(text.size()/4,3*text.size()/4).isRightToLeft());
    if (text.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML TextEdit: isRightToLeft(start, end) called with the end property being smaller than the start.");
    QCOMPARE(textEdit.isRightToLeft(3*text.size()/4,text.size()-1), text.mid(3*text.size()/4,text.size()-1).isRightToLeft());

    // then test that the feature actually works
    QCOMPARE(textEdit.isRightToLeft(0,0), emptyString);
    QCOMPARE(textEdit.isRightToLeft(0,1), firstCharacter);
    QCOMPARE(textEdit.isRightToLeft(text.size()-2, text.size()-1), lastCharacter);
    QCOMPARE(textEdit.isRightToLeft(text.size()/2, text.size()/2 + 1), middleCharacter);
    QCOMPARE(textEdit.isRightToLeft(0,text.size()/4), startString);
    QCOMPARE(textEdit.isRightToLeft(text.size()/4,3*text.size()/4), midString);
    if (text.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML TextEdit: isRightToLeft(start, end) called with the end property being smaller than the start.");
    QCOMPARE(textEdit.isRightToLeft(3*text.size()/4,text.size()-1), endString);
}

void tst_qquicktextedit::keySelection()
{
    QQuickView window(testFileUrl("navigation.qml"));
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QVERIFY(window.rootObject() != nullptr);

    QQuickTextEdit *input = qobject_cast<QQuickTextEdit *>(qvariant_cast<QObject *>(window.rootObject()->property("myInput")));

    QVERIFY(input != nullptr);
    QVERIFY(input->hasActiveFocus());

    QSignalSpy spy(input, SIGNAL(selectedTextChanged()));

    simulateKey(&window, Qt::Key_Right, Qt::ShiftModifier);
    QVERIFY(input->hasActiveFocus());
    QCOMPARE(input->selectedText(), QString("a"));
    QCOMPARE(spy.size(), 1);
    simulateKey(&window, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus());
    QCOMPARE(input->selectedText(), QString());
    QCOMPARE(spy.size(), 2);
    simulateKey(&window, Qt::Key_Right);
    QVERIFY(!input->hasActiveFocus());
    QCOMPARE(input->selectedText(), QString());
    QCOMPARE(spy.size(), 2);

    simulateKey(&window, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus());
    QCOMPARE(spy.size(), 2);
    simulateKey(&window, Qt::Key_Left, Qt::ShiftModifier);
    QVERIFY(input->hasActiveFocus());
    QCOMPARE(input->selectedText(), QString("a"));
    QCOMPARE(spy.size(), 3);
    simulateKey(&window, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus());
    QCOMPARE(input->selectedText(), QString());
    QCOMPARE(spy.size(), 4);
    simulateKey(&window, Qt::Key_Left);
    QVERIFY(!input->hasActiveFocus());
    QCOMPARE(input->selectedText(), QString());
    QCOMPARE(spy.size(), 4);
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
    QQmlComponent textinputComponent(&engine);
    textinputComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *texteditObject = qobject_cast<QQuickTextEdit*>(textinputComponent.create());
    QVERIFY(texteditObject != nullptr);

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
    QQmlComponent texteditComponent(&engine);
    texteditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *texteditObject = qobject_cast<QQuickTextEdit*>(texteditComponent.create());
    QVERIFY(texteditObject != nullptr);

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
    QTest::addColumn<bool>("focus");
    QTest::addColumn<bool>("focusOnPress");
    QTest::addColumn<int>("clicks");

    // import installed
    QTest::newRow("on") << testFile("mouseselection_true.qml") << 4 << 9 << "45678" << true << true << 1;
    QTest::newRow("off") << testFile("mouseselection_false.qml") << 4 << 9 << QString() << true << true << 1;
    QTest::newRow("default") << testFile("mouseselectionmode_default.qml") << 4 << 9 << "45678" << true << true << 1;
    QTest::newRow("off word selection") << testFile("mouseselection_false_words.qml") << 4 << 9 << QString() << true << true << 1;
    QTest::newRow("on word selection (4,9)") << testFile("mouseselection_true_words.qml") << 4 << 9 << "0123456789" << true << true << 1;

    QTest::newRow("on unfocused") << testFile("mouseselection_true.qml") << 4 << 9 << "45678" << false << false << 1;
    QTest::newRow("on word selection (4,9) unfocused") << testFile("mouseselection_true_words.qml") << 4 << 9 << "0123456789" << false << false << 1;

    QTest::newRow("on focus on press") << testFile("mouseselection_true.qml") << 4 << 9 << "45678" << false << true << 1;
    QTest::newRow("on word selection (4,9) focus on press") << testFile("mouseselection_true_words.qml") << 4 << 9 << "0123456789" << false << true << 1;

    QTest::newRow("on word selection (2,13)") << testFile("mouseselection_true_words.qml") << 2 << 13 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 1;
    QTest::newRow("on word selection (2,30)") << testFile("mouseselection_true_words.qml") << 2 << 30 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 1;
    QTest::newRow("on word selection (9,13)") << testFile("mouseselection_true_words.qml") << 9 << 13 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 1;
    QTest::newRow("on word selection (9,30)") << testFile("mouseselection_true_words.qml") << 9 << 30 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 1;
    QTest::newRow("on word selection (13,2)") << testFile("mouseselection_true_words.qml") << 13 << 2 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 1;
    QTest::newRow("on word selection (20,2)") << testFile("mouseselection_true_words.qml") << 20 << 2 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 1;
    QTest::newRow("on word selection (12,9)") << testFile("mouseselection_true_words.qml") << 12 << 9 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 1;
    QTest::newRow("on word selection (30,9)") << testFile("mouseselection_true_words.qml") << 30 << 9 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 1;

    QTest::newRow("on double click (4,9)") << testFile("mouseselection_true.qml") << 4 << 9 << "0123456789" << true << true << 2;
    QTest::newRow("on double click (2,13)") << testFile("mouseselection_true.qml") << 2 << 13 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 2;
    QTest::newRow("on double click (2,30)") << testFile("mouseselection_true.qml") << 2 << 30 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 2;
    QTest::newRow("on double click (9,13)") << testFile("mouseselection_true.qml") << 9 << 13 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 2;
    QTest::newRow("on double click (9,30)") << testFile("mouseselection_true.qml") << 9 << 30 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 2;
    QTest::newRow("on double click (13,2)") << testFile("mouseselection_true.qml") << 13 << 2 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 2;
    QTest::newRow("on double click (20,2)") << testFile("mouseselection_true.qml") << 20 << 2 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 2;
    QTest::newRow("on double click (12,9)") << testFile("mouseselection_true.qml") << 12 << 9 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 2;
    QTest::newRow("on double click (30,9)") << testFile("mouseselection_true.qml") << 30 << 9 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ" << true << true << 2;

    QTest::newRow("on triple click (4,9)") << testFile("mouseselection_true.qml") << 4 << 9 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n" << true << true << 3;
    QTest::newRow("on triple click (2,13)") << testFile("mouseselection_true.qml") << 2 << 13 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n" << true << true << 3;
    QTest::newRow("on triple click (2,30)") << testFile("mouseselection_true.qml") << 2 << 30 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n" << true << true << 3;
    QTest::newRow("on triple click (9,13)") << testFile("mouseselection_true.qml") << 9 << 13 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n" << true << true << 3;
    QTest::newRow("on triple click (9,30)") << testFile("mouseselection_true.qml") << 9 << 30 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n" << true << true << 3;
    QTest::newRow("on triple click (13,2)") << testFile("mouseselection_true.qml") << 13 << 2 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n" << true << true << 3;
    QTest::newRow("on triple click (20,2)") << testFile("mouseselection_true.qml") << 20 << 2 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n" << true << true << 3;
    QTest::newRow("on triple click (12,9)") << testFile("mouseselection_true.qml") << 12 << 9 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n" << true << true << 3;
    QTest::newRow("on triple click (30,9)") << testFile("mouseselection_true.qml") << 30 << 9 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n" << true << true << 3;

    QTest::newRow("on triple click (2,40)") << testFile("mouseselection_true.qml") << 2 << 40 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n9876543210\n" << true << true << 3;
    QTest::newRow("on triple click (2,50)") << testFile("mouseselection_true.qml") << 2 << 50 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n9876543210\n\nZXYWVUTSRQPON MLKJIHGFEDCBA" << true << true << 3;
    QTest::newRow("on triple click (25,40)") << testFile("mouseselection_true.qml") << 25 << 40 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n9876543210\n" << true << true << 3;
    QTest::newRow("on triple click (25,50)") << testFile("mouseselection_true.qml") << 25 << 50 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n9876543210\n\nZXYWVUTSRQPON MLKJIHGFEDCBA" << true << true << 3;
    QTest::newRow("on triple click (40,25)") << testFile("mouseselection_true.qml") << 40 << 25 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n9876543210\n" << true << true << 3;
    QTest::newRow("on triple click (40,50)") << testFile("mouseselection_true.qml") << 40 << 50 << "9876543210\n\nZXYWVUTSRQPON MLKJIHGFEDCBA" << true << true << 3;
    QTest::newRow("on triple click (50,25)") << testFile("mouseselection_true.qml") << 50 << 25 << "0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n9876543210\n\nZXYWVUTSRQPON MLKJIHGFEDCBA" << true << true << 3;
    QTest::newRow("on triple click (50,40)") << testFile("mouseselection_true.qml") << 50 << 40 << "9876543210\n\nZXYWVUTSRQPON MLKJIHGFEDCBA" << true << true << 3;

    QTest::newRow("on tr align") << testFile("mouseselection_align_tr.qml") << 4 << 9 << "45678" << true << true << 1;
    QTest::newRow("on center align") << testFile("mouseselection_align_center.qml") << 4 << 9 << "45678" << true << true << 1;
    QTest::newRow("on bl align") << testFile("mouseselection_align_bl.qml") << 4 << 9 << "45678" << true << true << 1;
}

void tst_qquicktextedit::mouseSelection()
{
    QFETCH(QString, qmlfile);
    QFETCH(int, from);
    QFETCH(int, to);
    QFETCH(QString, selectedText);
    QFETCH(bool, focus);
    QFETCH(bool, focusOnPress);
    QFETCH(int, clicks);

    QQuickView window(QUrl::fromLocalFile(qmlfile));

    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QVERIFY(window.rootObject() != nullptr);
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textEditObject != nullptr);

    textEditObject->setFocus(focus);
    textEditObject->setFocusOnPress(focusOnPress);

    // Avoid that the last click from the previous test data and the first click in the
    // current test data happens so close in time that they are interpreted as a double click.
    static const int moreThanDoubleClickInterval = QGuiApplication::styleHints()->mouseDoubleClickInterval() + 1;

    // press-and-drag-and-release from x1 to x2
    QPoint p1 = textEditObject->positionToRectangle(from).center().toPoint();
    QPoint p2 = textEditObject->positionToRectangle(to).center().toPoint();
    if (clicks == 2)
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, p1, moreThanDoubleClickInterval);
    else if (clicks == 3)
        QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, p1, moreThanDoubleClickInterval);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QTest::mouseMove(&window, p2);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p2);
    QTRY_COMPARE(textEditObject->selectedText(), selectedText);

    // Clicking and shift to clicking between the same points should select the same text.
    textEditObject->setCursorPosition(0);
    if (clicks > 1)
        QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, p1, 10);
    if (clicks != 2)
        QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, p1, 10);
    QTest::mouseClick(&window, Qt::LeftButton, Qt::ShiftModifier, p2);
    QTRY_COMPARE(textEditObject->selectedText(), selectedText);
}

void tst_qquicktextedit::dragMouseSelection()
{
    QString qmlfile = testFile("mouseselection_true.qml");

    QQuickView window(QUrl::fromLocalFile(qmlfile));

    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QVERIFY(window.rootObject() != nullptr);
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textEditObject != nullptr);

    // press-and-drag-and-release from x1 to x2
    int x1 = 10;
    int x2 = 70;
    int y = QFontMetrics(textEditObject->font()).height() / 2;
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x1,y));
    QTest::mouseMove(&window, QPoint(x2, y));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x2,y));
    QTest::qWait(300);
    QString str1;
    QTRY_VERIFY((str1 = textEditObject->selectedText()).size() > 3);

    // press and drag the current selection.
    x1 = 40;
    x2 = 100;
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x1,y));
    QTest::mouseMove(&window, QPoint(x2, y));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x2,y));
    QTest::qWait(300);
    QString str2;
    QTRY_VERIFY((str2 = textEditObject->selectedText()).size() > 3);

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

    QQuickView window(QUrl::fromLocalFile(qmlfile));

    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QVERIFY(window.rootObject() != nullptr);
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textEditObject != nullptr);
    textEditObject->setSelectByMouse(true);

    // press-and-drag-and-release from x1 to x2
    int x1 = 10;
    int x2 = 70;
    int y = textEditObject->height()/2;
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x1,y));
    QTest::mouseMove(&window, QPoint(x2, y));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(x2,y));
    QString str = textEditObject->selectedText();
    if (selectWords) {
        QTRY_COMPARE(textEditObject->selectedText(), text);
    } else {
        QTRY_VERIFY(textEditObject->selectedText().size() > 3);
        QVERIFY(str != text);
    }
}

void tst_qquicktextedit::mouseSelectionMode_accessors()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n TextEdit {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(object.data());
    QVERIFY(edit);

    QSignalSpy spy(edit, &QQuickTextEdit::mouseSelectionModeChanged);

    QCOMPARE(edit->mouseSelectionMode(), QQuickTextEdit::SelectCharacters);

    edit->setMouseSelectionMode(QQuickTextEdit::SelectWords);
    QCOMPARE(edit->mouseSelectionMode(), QQuickTextEdit::SelectWords);
    QCOMPARE(spy.size(), 1);

    edit->setMouseSelectionMode(QQuickTextEdit::SelectWords);
    QCOMPARE(spy.size(), 1);

    edit->setMouseSelectionMode(QQuickTextEdit::SelectCharacters);
    QCOMPARE(edit->mouseSelectionMode(), QQuickTextEdit::SelectCharacters);
    QCOMPARE(spy.size(), 2);
}

void tst_qquicktextedit::selectByMouse()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n TextEdit {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(object.data());
    QVERIFY(edit);

    QSignalSpy spy(edit, SIGNAL(selectByMouseChanged(bool)));

    QCOMPARE(edit->selectByMouse(), false);

    edit->setSelectByMouse(true);
    QCOMPARE(edit->selectByMouse(), true);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), true);

    edit->setSelectByMouse(true);
    QCOMPARE(spy.size(), 1);

    edit->setSelectByMouse(false);
    QCOMPARE(edit->selectByMouse(), false);
    QCOMPARE(spy.size(), 2);
    QCOMPARE(spy.at(1).at(0).toBool(), false);
}

void tst_qquicktextedit::selectByKeyboard()
{
    QQmlComponent oldComponent(&engine);
    oldComponent.setData("import QtQuick 2.0\n TextEdit { selectByKeyboard: true }", QUrl());
    QVERIFY(!oldComponent.create());

    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.1\n TextEdit { }", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(object.data());
    QVERIFY(edit);

    QSignalSpy spy(edit, SIGNAL(selectByKeyboardChanged(bool)));

    QCOMPARE(edit->isReadOnly(), false);
    QCOMPARE(edit->selectByKeyboard(), true);

    edit->setReadOnly(true);
    QCOMPARE(edit->selectByKeyboard(), false);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.at(0).at(0).toBool(), false);

    edit->setSelectByKeyboard(true);
    QCOMPARE(edit->selectByKeyboard(), true);
    QCOMPARE(spy.size(), 2);
    QCOMPARE(spy.at(1).at(0).toBool(), true);

    edit->setReadOnly(false);
    QCOMPARE(edit->selectByKeyboard(), true);
    QCOMPARE(spy.size(), 2);

    edit->setSelectByKeyboard(false);
    QCOMPARE(edit->selectByKeyboard(), false);
    QCOMPARE(spy.size(), 3);
    QCOMPARE(spy.at(2).at(0).toBool(), false);
}

#if QT_CONFIG(shortcut)

Q_DECLARE_METATYPE(QKeySequence::StandardKey)

void tst_qquicktextedit::keyboardSelection_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("readOnly");
    QTest::addColumn<bool>("selectByKeyboard");
    QTest::addColumn<int>("cursorPosition");
    QTest::addColumn<QKeySequence::StandardKey>("standardKey");
    QTest::addColumn<QString>("selectedText");

    QTest::newRow("editable - select first char")
            << QStringLiteral("editable - select first char") << false << true << 0 << QKeySequence::SelectNextChar << QStringLiteral("e");
    QTest::newRow("editable - select first word")
            << QStringLiteral("editable - select first char") << false << true << 0 << QKeySequence::SelectNextWord << QStringLiteral("editable ");

    QTest::newRow("editable - cannot select first char")
            << QStringLiteral("editable - cannot select first char") << false << false << 0 << QKeySequence::SelectNextChar << QStringLiteral("");
    QTest::newRow("editable - cannot select first word")
            << QStringLiteral("editable - cannot select first word") << false << false << 0 << QKeySequence::SelectNextWord << QStringLiteral("");

    QTest::newRow("editable - select last char")
            << QStringLiteral("editable - select last char") << false << true << 27 << QKeySequence::SelectPreviousChar << QStringLiteral("r");
    QTest::newRow("editable - select last word")
            << QStringLiteral("editable - select last word") << false << true << 27 << QKeySequence::SelectPreviousWord << QStringLiteral("word");

    QTest::newRow("editable - cannot select last char")
            << QStringLiteral("editable - cannot select last char") << false << false << 35 << QKeySequence::SelectPreviousChar << QStringLiteral("");
    QTest::newRow("editable - cannot select last word")
            << QStringLiteral("editable - cannot select last word") << false << false << 35 << QKeySequence::SelectPreviousWord << QStringLiteral("");

    QTest::newRow("read-only - cannot select first char")
            << QStringLiteral("read-only - cannot select first char") << true << false << 0 << QKeySequence::SelectNextChar << QStringLiteral("");
    QTest::newRow("read-only - cannot select first word")
            << QStringLiteral("read-only - cannot select first word") << true << false << 0 << QKeySequence::SelectNextWord << QStringLiteral("");

    QTest::newRow("read-only - cannot select last char")
            << QStringLiteral("read-only - cannot select last char") << true << false << 35 << QKeySequence::SelectPreviousChar << QStringLiteral("");
    QTest::newRow("read-only - cannot select last word")
            << QStringLiteral("read-only - cannot select last word") << true << false << 35 << QKeySequence::SelectPreviousWord << QStringLiteral("");

    QTest::newRow("read-only - select first char")
            << QStringLiteral("read-only - select first char") << true << true << 0 << QKeySequence::SelectNextChar << QStringLiteral("r");
    QTest::newRow("read-only - select first word")
            << QStringLiteral("read-only - select first word") << true << true << 0 << QKeySequence::SelectNextWord << QStringLiteral("read");

    QTest::newRow("read-only - select last char")
            << QStringLiteral("read-only - select last char") << true << true << 28 << QKeySequence::SelectPreviousChar << QStringLiteral("r");
    QTest::newRow("read-only - select last word")
            << QStringLiteral("read-only - select last word") << true << true << 28 << QKeySequence::SelectPreviousWord << QStringLiteral("word");
}

void tst_qquicktextedit::keyboardSelection()
{
    QFETCH(QString, text);
    QFETCH(bool, readOnly);
    QFETCH(bool, selectByKeyboard);
    QFETCH(int, cursorPosition);
    QFETCH(QKeySequence::StandardKey, standardKey);
    QFETCH(QString, selectedText);

    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.1\n TextEdit { focus: true }", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(object.data());
    QVERIFY(edit);

    edit->setText(text);
    edit->setSelectByKeyboard(selectByKeyboard);
    edit->setReadOnly(readOnly);
    edit->setCursorPosition(cursorPosition);

    QQuickWindow window;
    edit->setParentItem(window.contentItem());
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QVERIFY(edit->hasActiveFocus());

    simulateKeys(&window, standardKey);

    QCOMPARE(edit->selectedText(), selectedText);
}

#endif // QT_CONFIG(shortcut)

void tst_qquicktextedit::renderType()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n TextEdit {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(object.data());
    QVERIFY(edit);

    QSignalSpy spy(edit, SIGNAL(renderTypeChanged()));

    QCOMPARE(edit->renderType(), QQuickTextEdit::QtRendering);

    edit->setRenderType(QQuickTextEdit::NativeRendering);
    QCOMPARE(edit->renderType(), QQuickTextEdit::NativeRendering);
    QCOMPARE(spy.size(), 1);

    edit->setRenderType(QQuickTextEdit::NativeRendering);
    QCOMPARE(spy.size(), 1);

    edit->setRenderType(QQuickTextEdit::QtRendering);
    QCOMPARE(edit->renderType(), QQuickTextEdit::QtRendering);
    QCOMPARE(spy.size(), 2);
}

void tst_qquicktextedit::inputMethodHints()
{
    QQuickView window(testFileUrl("inputmethodhints.qml"));
    window.show();
    window.requestActivate();

    QVERIFY(window.rootObject() != nullptr);
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textEditObject != nullptr);
    QVERIFY(textEditObject->inputMethodHints() & Qt::ImhNoPredictiveText);
    QSignalSpy inputMethodHintSpy(textEditObject, SIGNAL(inputMethodHintsChanged()));
    textEditObject->setInputMethodHints(Qt::ImhUppercaseOnly);
    QVERIFY(textEditObject->inputMethodHints() & Qt::ImhUppercaseOnly);
    QCOMPARE(inputMethodHintSpy.size(), 1);
    textEditObject->setInputMethodHints(Qt::ImhUppercaseOnly);
    QCOMPARE(inputMethodHintSpy.size(), 1);

    QQuickTextEdit plainTextEdit;
    QCOMPARE(plainTextEdit.inputMethodHints(), Qt::ImhNone);
}

void tst_qquicktextedit::positionAt_data()
{
    QTest::addColumn<QQuickTextEdit::HAlignment>("horizontalAlignment");
    QTest::addColumn<QQuickTextEdit::VAlignment>("verticalAlignment");

    QTest::newRow("top-left") << QQuickTextEdit::AlignLeft << QQuickTextEdit::AlignTop;
    QTest::newRow("bottom-left") << QQuickTextEdit::AlignLeft << QQuickTextEdit::AlignBottom;
    QTest::newRow("center-left") << QQuickTextEdit::AlignLeft << QQuickTextEdit::AlignVCenter;

    QTest::newRow("top-right") << QQuickTextEdit::AlignRight << QQuickTextEdit::AlignTop;
    QTest::newRow("top-center") << QQuickTextEdit::AlignHCenter << QQuickTextEdit::AlignTop;

    QTest::newRow("center") << QQuickTextEdit::AlignHCenter << QQuickTextEdit::AlignVCenter;
}

void tst_qquicktextedit::positionAt()
{
    QFETCH(QQuickTextEdit::HAlignment, horizontalAlignment);
    QFETCH(QQuickTextEdit::VAlignment, verticalAlignment);

    QQuickView window(testFileUrl("positionAt.qml"));
    QVERIFY(window.rootObject() != nullptr);
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QQuickTextEdit *texteditObject = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(texteditObject != nullptr);
    texteditObject->setHAlign(horizontalAlignment);
    texteditObject->setVAlign(verticalAlignment);

    QTextLayout layout(texteditObject->text().replace(QLatin1Char('\n'), QChar::LineSeparator));
    layout.setFont(texteditObject->font());

    if (!qmlDisableDistanceField()) {
        QTextOption option;
        option.setUseDesignMetrics(true);
        layout.setTextOption(option);
    }

    layout.beginLayout();
    QTextLine line = layout.createLine();
    line.setLineWidth(texteditObject->width());
    QTextLine secondLine = layout.createLine();
    secondLine.setLineWidth(texteditObject->width());
    layout.endLayout();

    qreal y0 = 0;
    qreal y1 = 0;

    switch (verticalAlignment) {
    case QQuickTextEdit::AlignTop:
        y0 = line.height() / 2;
        y1 = line.height() * 3 / 2;
        break;
    case QQuickTextEdit::AlignVCenter:
        y0 = (texteditObject->height() - line.height()) / 2;
        y1 = (texteditObject->height() + line.height()) / 2;
        break;
    case QQuickTextEdit::AlignBottom:
        y0 = texteditObject->height() - line.height() * 3 / 2;
        y1 = texteditObject->height() - line.height() / 2;
        break;
    }

    qreal xoff = 0;
    switch (horizontalAlignment) {
    case QQuickTextEdit::AlignLeft:
        break;
    case QQuickTextEdit::AlignHCenter:
        xoff = (texteditObject->width() - secondLine.naturalTextWidth()) / 2;
        break;
    case QQuickTextEdit::AlignRight:
        xoff = texteditObject->width() - secondLine.naturalTextWidth();
        break;
    default:
        break;
    }
    int pos = texteditObject->positionAt(texteditObject->width()/2, y0);

    int widthBegin = floor(xoff + line.cursorToX(pos - 1));
    int widthEnd = ceil(xoff + line.cursorToX(pos + 1));

    QVERIFY(widthBegin <= texteditObject->width() / 2);
    QVERIFY(widthEnd >= texteditObject->width() / 2);

    const qreal x0 = texteditObject->positionToRectangle(pos).x();
    const qreal x1 = texteditObject->positionToRectangle(pos + 1).x();

    QString preeditText = texteditObject->text().mid(0, pos);
    texteditObject->setText(texteditObject->text().mid(pos));
    texteditObject->setCursorPosition(0);

    QInputMethodEvent inputEvent(preeditText, QList<QInputMethodEvent::Attribute>());
    QGuiApplication::sendEvent(texteditObject, &inputEvent);

    // Check all points within the preedit text return the same position.
    QCOMPARE(texteditObject->positionAt(0, y0), 0);
    QCOMPARE(texteditObject->positionAt(x0 / 2, y0), 0);
    QCOMPARE(texteditObject->positionAt(x0, y0), 0);

    // Verify positioning returns to normal after the preedit text.
    QCOMPARE(texteditObject->positionAt(x1, y0), 1);
    QCOMPARE(texteditObject->positionToRectangle(1).x(), x1);

    QVERIFY(texteditObject->positionAt(x0 / 2, y1) > 0);
}

#if QT_CONFIG(cursor)
void tst_qquicktextedit::linkHover()
{
    if (isPlatformWayland())
         QSKIP("Wayland: QCursor::setPos() doesn't work.");

    QQuickView window(testFileUrl("linkInteraction.qml"));
    window.setFlag(Qt::FramelessWindowHint);
    QVERIFY(window.rootObject() != nullptr);
    QQuickVisualTestUtils::centerOnScreen(&window);
    QQuickVisualTestUtils::moveMouseAway(&window);
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QQuickTextEdit *texteditObject = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(texteditObject != nullptr);

    QSignalSpy hover(texteditObject, SIGNAL(linkHovered(QString)));

    const QString link("http://example.com/");
    const QPoint linkPos = window.mapToGlobal(texteditObject->positionToRectangle(7).center().toPoint());
    const QPoint textPos = window.mapToGlobal(texteditObject->positionToRectangle(2).center().toPoint());

    QCursor::setPos(linkPos);
    QTRY_COMPARE(hover.size(), 1);
    QCOMPARE(window.cursor().shape(), Qt::PointingHandCursor);
    QCOMPARE(hover.last()[0].toString(), link);

    QCursor::setPos(textPos);
    QTRY_COMPARE(hover.size(), 2);
    QCOMPARE(window.cursor().shape(), Qt::IBeamCursor);
    QCOMPARE(hover.last()[0].toString(), QString());

    QCursor::setPos(linkPos);
    QTRY_COMPARE(hover.size(), 3);
    QCOMPARE(window.cursor().shape(), Qt::PointingHandCursor);
    QCOMPARE(hover.last()[0].toString(), link);

    QCursor::setPos(textPos);
    QTRY_COMPARE(hover.size(), 4);
    QCOMPARE(window.cursor().shape(), Qt::IBeamCursor);
    QCOMPARE(hover.last()[0].toString(), QString());
}
#endif

void tst_qquicktextedit::linkInteraction()
{
    QQuickView window(testFileUrl("linkInteraction.qml"));
    QVERIFY(window.rootObject() != nullptr);
    QQuickVisualTestUtils::centerOnScreen(&window);
    QQuickVisualTestUtils::moveMouseAway(&window);
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QQuickTextEdit *texteditObject = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(texteditObject != nullptr);

    QSignalSpy spy(texteditObject, SIGNAL(linkActivated(QString)));
    QSignalSpy hover(texteditObject, SIGNAL(linkHovered(QString)));

    const QString link("http://example.com/");

    const QPointF linkPos = texteditObject->positionToRectangle(7).center();
    const QPointF textPos = texteditObject->positionToRectangle(2).center();

    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, linkPos.toPoint());
    QTRY_COMPARE(spy.size(), 1);
    QTRY_COMPARE(hover.size(), 1);
    QCOMPARE(spy.last()[0].toString(), link);
    QCOMPARE(hover.last()[0].toString(), link);
    QCOMPARE(texteditObject->hoveredLink(), link);
    QCOMPARE(texteditObject->linkAt(linkPos.x(), linkPos.y()), link);

    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, textPos.toPoint());
    QTRY_COMPARE(spy.size(), 1);
    QTRY_COMPARE(hover.size(), 2);
    QCOMPARE(hover.last()[0].toString(), QString());
    QCOMPARE(texteditObject->hoveredLink(), QString());
    QCOMPARE(texteditObject->linkAt(textPos.x(), textPos.y()), QString());

    texteditObject->setReadOnly(true);

    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, linkPos.toPoint());
    QTRY_COMPARE(spy.size(), 2);
    QTRY_COMPARE(hover.size(), 3);
    QCOMPARE(spy.last()[0].toString(), link);
    QCOMPARE(hover.last()[0].toString(), link);
    QCOMPARE(texteditObject->hoveredLink(), link);
    QCOMPARE(texteditObject->linkAt(linkPos.x(), linkPos.y()), link);

    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, textPos.toPoint());
    QTRY_COMPARE(spy.size(), 2);
    QTRY_COMPARE(hover.size(), 4);
    QCOMPARE(hover.last()[0].toString(), QString());
    QCOMPARE(texteditObject->hoveredLink(), QString());
    QCOMPARE(texteditObject->linkAt(textPos.x(), textPos.y()), QString());
}

void tst_qquicktextedit::cursorDelegate_data()
{
    QTest::addColumn<QUrl>("source");
    QTest::newRow("out of line") << testFileUrl("cursorTest.qml");
    QTest::newRow("in line") << testFileUrl("cursorTestInline.qml");
    QTest::newRow("external") << testFileUrl("cursorTestExternal.qml");
}

void tst_qquicktextedit::cursorDelegate()
{
    QFETCH(QUrl, source);
    QQuickView view(source);
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QQuickTextEdit *textEditObject = view.rootObject()->findChild<QQuickTextEdit*>("textEditObject");
    QVERIFY(textEditObject != nullptr);
    // Delegate creation is deferred until focus in or cursor visibility is forced.
    QVERIFY(!textEditObject->findChild<QQuickItem*>("cursorInstance"));
    QVERIFY(!textEditObject->isCursorVisible());
    //Test Delegate gets created
    textEditObject->setFocus(true);
    QVERIFY(textEditObject->isCursorVisible());
    QQuickItem* delegateObject = textEditObject->findChild<QQuickItem*>("cursorInstance");
    QVERIFY(delegateObject);
    QCOMPARE(delegateObject->property("localProperty").toString(), QString("Hello"));
    //Test Delegate gets moved
    for (int i=0; i<= textEditObject->text().size(); i++) {
        textEditObject->setCursorPosition(i);
        QCOMPARE(textEditObject->cursorRectangle().x(), delegateObject->x());
        QCOMPARE(textEditObject->cursorRectangle().y(), delegateObject->y());
    }

    // Test delegate gets moved on mouse press.
    textEditObject->setSelectByMouse(true);
    textEditObject->setCursorPosition(0);
    const QPoint point1 = textEditObject->positionToRectangle(5).center().toPoint();
    QTest::qWait(400);  //ensure this isn't treated as a double-click
    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, point1);
    QTest::qWait(50);
    QTRY_VERIFY(textEditObject->cursorPosition() != 0);
    QCOMPARE(textEditObject->cursorRectangle().x(), delegateObject->x());
    QCOMPARE(textEditObject->cursorRectangle().y(), delegateObject->y());

    // Test delegate gets moved on mouse drag
    textEditObject->setCursorPosition(0);
    const QPoint point2 = textEditObject->positionToRectangle(10).center().toPoint();
    QTest::qWait(400);  //ensure this isn't treated as a double-click
    QTest::mousePress(&view, Qt::LeftButton, Qt::NoModifier, point1);
    QMouseEvent mv(QEvent::MouseMove, point2, view.mapToGlobal(point2),
                   Qt::LeftButton, Qt::LeftButton,Qt::NoModifier);
    QGuiApplication::sendEvent(&view, &mv);
    QTest::mouseRelease(&view, Qt::LeftButton, Qt::NoModifier, point2);
    QTest::qWait(50);
    QTRY_COMPARE(textEditObject->cursorRectangle().x(), delegateObject->x());
    QCOMPARE(textEditObject->cursorRectangle().y(), delegateObject->y());

    textEditObject->setReadOnly(true);
    textEditObject->setCursorPosition(0);
    QTest::qWait(400);  //ensure this isn't treated as a double-click
    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, textEditObject->positionToRectangle(5).center().toPoint());
    QTest::qWait(50);
    QTRY_VERIFY(textEditObject->cursorPosition() != 0);
    QCOMPARE(textEditObject->cursorRectangle().x(), delegateObject->x());
    QCOMPARE(textEditObject->cursorRectangle().y(), delegateObject->y());

    textEditObject->setCursorPosition(0);
    QTest::qWait(400);  //ensure this isn't treated as a double-click
    QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier, textEditObject->positionToRectangle(5).center().toPoint());
    QTest::qWait(50);
    QTRY_VERIFY(textEditObject->cursorPosition() != 0);
    QCOMPARE(textEditObject->cursorRectangle().x(), delegateObject->x());
    QCOMPARE(textEditObject->cursorRectangle().y(), delegateObject->y());

    textEditObject->setCursorPosition(0);
    QCOMPARE(textEditObject->cursorRectangle().x(), delegateObject->x());
    QCOMPARE(textEditObject->cursorRectangle().y(), delegateObject->y());

    textEditObject->setReadOnly(false);

    // Delegate moved when text is entered
    textEditObject->setText(QString());
    for (int i = 0; i < 20; ++i) {
        QTest::keyClick(&view, Qt::Key_A);
        QCOMPARE(textEditObject->cursorRectangle().x(), delegateObject->x());
        QCOMPARE(textEditObject->cursorRectangle().y(), delegateObject->y());
    }

    // Delegate moved when text is entered by im.
    textEditObject->setText(QString());
    for (int i = 0; i < 20; ++i) {
        QInputMethodEvent event;
        event.setCommitString("a");
        QGuiApplication::sendEvent(textEditObject, &event);
        QCOMPARE(textEditObject->cursorRectangle().x(), delegateObject->x());
        QCOMPARE(textEditObject->cursorRectangle().y(), delegateObject->y());
    }
    // Delegate moved when text is removed by im.
    for (int i = 19; i > 1; --i) {
        QInputMethodEvent event;
        event.setCommitString(QString(), -1, 1);
        QGuiApplication::sendEvent(textEditObject, &event);
        QCOMPARE(textEditObject->cursorRectangle().x(), delegateObject->x());
        QCOMPARE(textEditObject->cursorRectangle().y(), delegateObject->y());
    }
    {   // Delegate moved the text is changed in place by im.
        QInputMethodEvent event;
        event.setCommitString("i", -1, 1);
        QGuiApplication::sendEvent(textEditObject, &event);
        QCOMPARE(textEditObject->cursorRectangle().x(), delegateObject->x());
        QCOMPARE(textEditObject->cursorRectangle().y(), delegateObject->y());
    }

    //Test Delegate gets deleted
    textEditObject->setCursorDelegate(nullptr);
    QVERIFY(!textEditObject->findChild<QQuickItem*>("cursorInstance"));
}

void tst_qquicktextedit::remoteCursorDelegate()
{
    ThreadedTestHTTPServer server(dataDirectory(), TestHTTPServer::Delay);

    QQuickView view;

    QQmlComponent component(view.engine(), server.url("/RemoteCursor.qml"));

    view.rootContext()->setContextProperty("contextDelegate", &component);
    view.setSource(testFileUrl("cursorTestRemote.qml"));
    view.showNormal();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QQuickTextEdit *textEditObject = view.rootObject()->findChild<QQuickTextEdit*>("textEditObject");
    QVERIFY(textEditObject != nullptr);

    // Delegate is created on demand, and so won't be available immediately.  Focus in or
    // setCursorVisible(true) will trigger creation.
    QTRY_VERIFY(!textEditObject->findChild<QQuickItem*>("cursorInstance"));
    QVERIFY(!textEditObject->isCursorVisible());

    textEditObject->setFocus(true);
    QVERIFY(textEditObject->isCursorVisible());

    // Wait for component to load.
    QTRY_COMPARE(component.status(), QQmlComponent::Ready);
    QVERIFY(textEditObject->findChild<QQuickItem*>("cursorInstance"));
}

void tst_qquicktextedit::cursorVisible()
{
    QQuickTextEdit edit;
    edit.componentComplete();
    QSignalSpy spy(&edit, SIGNAL(cursorVisibleChanged(bool)));

    QQuickView view(testFileUrl("cursorVisible.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(&view, qGuiApp->focusWindow());

    QCOMPARE(edit.isCursorVisible(), false);

    edit.setCursorVisible(true);
    QCOMPARE(edit.isCursorVisible(), true);
    QCOMPARE(spy.size(), 1);

    edit.setCursorVisible(false);
    QCOMPARE(edit.isCursorVisible(), false);
    QCOMPARE(spy.size(), 2);

    edit.setFocus(true);
    QCOMPARE(edit.isCursorVisible(), false);
    QCOMPARE(spy.size(), 2);

    edit.setParentItem(view.rootObject());
    QCOMPARE(edit.isCursorVisible(), true);
    QCOMPARE(spy.size(), 3);

    edit.setFocus(false);
    QCOMPARE(edit.isCursorVisible(), false);
    QCOMPARE(spy.size(), 4);

    edit.setFocus(true);
    QCOMPARE(edit.isCursorVisible(), true);
    QCOMPARE(spy.size(), 5);

    QWindow alternateView;
    alternateView.show();
    alternateView.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&alternateView));

    QCOMPARE(edit.isCursorVisible(), false);
    QCOMPARE(spy.size(), 6);

    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QCOMPARE(edit.isCursorVisible(), true);
    QCOMPARE(spy.size(), 7);

    {   // Cursor attribute with 0 length hides cursor.
        QInputMethodEvent ev(QString(), QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 0, 0, QVariant()));
        QCoreApplication::sendEvent(&edit, &ev);
    }
    QCOMPARE(edit.isCursorVisible(), false);
    QCOMPARE(spy.size(), 8);

    {   // Cursor attribute with non zero length shows cursor.
        QInputMethodEvent ev(QString(), QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 0, 1, QVariant()));
        QCoreApplication::sendEvent(&edit, &ev);
    }
    QCOMPARE(edit.isCursorVisible(), true);
    QCOMPARE(spy.size(), 9);


    {   // If the cursor is hidden by the input method and the text is changed it should be visible again.
        QInputMethodEvent ev(QString(), QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 0, 0, QVariant()));
        QCoreApplication::sendEvent(&edit, &ev);
    }
    QCOMPARE(edit.isCursorVisible(), false);
    QCOMPARE(spy.size(), 10);

    edit.setText("something");
    QCOMPARE(edit.isCursorVisible(), true);
    QCOMPARE(spy.size(), 11);

    {   // If the cursor is hidden by the input method and the cursor position is changed it should be visible again.
        QInputMethodEvent ev(QString(), QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 0, 0, QVariant()));
        QCoreApplication::sendEvent(&edit, &ev);
    }
    QCOMPARE(edit.isCursorVisible(), false);
    QCOMPARE(spy.size(), 12);

    edit.setCursorPosition(5);
    QCOMPARE(edit.isCursorVisible(), true);
    QCOMPARE(spy.size(), 13);
}

void tst_qquicktextedit::delegateLoading_data()
{
    QTest::addColumn<QString>("qmlfile");
    QTest::addColumn<QString>("error");

    // import installed
    QTest::newRow("pass") << "cursorHttpTestPass.qml" << "";
    QTest::newRow("fail1") << "cursorHttpTestFail1.qml" << "{{ServerBaseUrl}}/FailItem.qml: Remote host closed the connection";
    QTest::newRow("fail2") << "cursorHttpTestFail2.qml" << "{{ServerBaseUrl}}/ErrItem.qml:4:5: Fungus is not a type";
}

void tst_qquicktextedit::delegateLoading()
{
    QFETCH(QString, qmlfile);
    QFETCH(QString, error);

    QHash<QString, TestHTTPServer::Mode> dirs;
    dirs[testFile("httpfail")] = TestHTTPServer::Disconnect;
    dirs[testFile("httpslow")] = TestHTTPServer::Delay;
    dirs[testFile("http")] = TestHTTPServer::Normal;
    ThreadedTestHTTPServer server(dirs);

    error.replace(QStringLiteral("{{ServerBaseUrl}}"), server.baseUrl().toString());

    if (!error.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, error.toUtf8());
    QQuickView view(server.url(qmlfile));
    view.show();
    view.requestActivate();

    if (!error.isEmpty()) {
        QTRY_VERIFY(view.status()==QQuickView::Error);
        QTRY_VERIFY(!view.rootObject()); // there is fail item inside this test
    } else {
        QTRY_VERIFY(view.rootObject());//Wait for loading to finish.
        QQuickTextEdit *textEditObject = view.rootObject()->findChild<QQuickTextEdit*>("textEditObject");
        //    view.rootObject()->dumpObjectTree();
        QVERIFY(textEditObject != nullptr);
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

void tst_qquicktextedit::cursorDelegateHeight()
{
    QQuickView view(testFileUrl("cursorHeight.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QQuickTextEdit *textEditObject = view.rootObject()->findChild<QQuickTextEdit*>("textEditObject");
    QVERIFY(textEditObject);
    // Delegate creation is deferred until focus in or cursor visibility is forced.
    QVERIFY(!textEditObject->findChild<QQuickItem*>("cursorInstance"));
    QVERIFY(!textEditObject->isCursorVisible());

    // Test that the delegate gets created.
    textEditObject->setFocus(true);
    QVERIFY(textEditObject->isCursorVisible());
    QQuickItem* delegateObject = textEditObject->findChild<QQuickItem*>("cursorInstance");
    QVERIFY(delegateObject);

    const int largerHeight = textEditObject->cursorRectangle().height();

    textEditObject->setCursorPosition(0);
    QCOMPARE(delegateObject->x(), textEditObject->cursorRectangle().x());
    QCOMPARE(delegateObject->y(), textEditObject->cursorRectangle().y());
    QCOMPARE(delegateObject->height(), textEditObject->cursorRectangle().height());

    // Move the cursor to the next line, which has a smaller font.
    textEditObject->setCursorPosition(5);
    QCOMPARE(delegateObject->x(), textEditObject->cursorRectangle().x());
    QCOMPARE(delegateObject->y(), textEditObject->cursorRectangle().y());
    QVERIFY(textEditObject->cursorRectangle().height() < largerHeight);
    QCOMPARE(delegateObject->height(), textEditObject->cursorRectangle().height());

    // Test that the delegate gets deleted
    textEditObject->setCursorDelegate(nullptr);
    QVERIFY(!textEditObject->findChild<QQuickItem*>("cursorInstance"));
}

/*
TextEdit element should only handle left/right keys until the cursor reaches
the extent of the text, then they should ignore the keys.
*/
void tst_qquicktextedit::navigation()
{
    QQuickView window(testFileUrl("navigation.qml"));
    window.show();
    window.requestActivate();

    QVERIFY(window.rootObject() != nullptr);

    QQuickTextEdit *input = qobject_cast<QQuickTextEdit *>(qvariant_cast<QObject *>(window.rootObject()->property("myInput")));

    QVERIFY(input != nullptr);
    QTRY_VERIFY(input->hasActiveFocus());
    simulateKey(&window, Qt::Key_Left);
    QVERIFY(!input->hasActiveFocus());
    simulateKey(&window, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus());
    simulateKey(&window, Qt::Key_Right);
    QVERIFY(input->hasActiveFocus());
    simulateKey(&window, Qt::Key_Right);
    QVERIFY(!input->hasActiveFocus());
    simulateKey(&window, Qt::Key_Left);
    QVERIFY(input->hasActiveFocus());

    // Test left and right navigation works if the TextEdit is empty (QTBUG-25447).
    input->setText(QString());
    QCOMPARE(input->cursorPosition(), 0);
    simulateKey(&window, Qt::Key_Right);
    QCOMPARE(input->hasActiveFocus(), false);
    simulateKey(&window, Qt::Key_Left);
    QCOMPARE(input->hasActiveFocus(), true);
    simulateKey(&window, Qt::Key_Left);
    QCOMPARE(input->hasActiveFocus(), false);
}

#if QT_CONFIG(clipboard)
void tst_qquicktextedit::copyAndPaste()
{
    if (!PlatformQuirks::isClipboardAvailable())
        QSKIP("This machine doesn't support the clipboard");

    QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"Hello world!\" }";
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    // copy and paste
    QCOMPARE(textEdit->text().size(), 12);
    textEdit->select(0, textEdit->text().size());;
    textEdit->copy();
    QCOMPARE(textEdit->selectedText(), QString("Hello world!"));
    QCOMPARE(textEdit->selectedText().size(), 12);
    textEdit->setCursorPosition(0);
    QVERIFY(textEdit->canPaste());
    textEdit->paste();
    QCOMPARE(textEdit->text(), QString("Hello world!Hello world!"));
    QCOMPARE(textEdit->text().size(), 24);

    // canPaste
    QVERIFY(textEdit->canPaste());
    textEdit->setReadOnly(true);
    QVERIFY(!textEdit->canPaste());
    textEdit->paste();
    QCOMPARE(textEdit->text(), QString("Hello world!Hello world!"));
    QCOMPARE(textEdit->text().size(), 24);
    textEdit->setReadOnly(false);
    QVERIFY(textEdit->canPaste());

    // QTBUG-12339
    // test that document and internal text attribute are in sync
    QQuickItemPrivate* pri = QQuickItemPrivate::get(textEdit);
    QQuickTextEditPrivate *editPrivate = static_cast<QQuickTextEditPrivate*>(pri);
    QCOMPARE(textEdit->text(), editPrivate->text);

    // cut: no selection
    textEdit->cut();
    QCOMPARE(textEdit->text(), QString("Hello world!Hello world!"));

    // select word
    textEdit->setCursorPosition(0);
    textEdit->selectWord();
    QCOMPARE(textEdit->selectedText(), QString("Hello"));

    // cut: read only.
    textEdit->setReadOnly(true);
    textEdit->cut();
    QCOMPARE(textEdit->text(), QString("Hello world!Hello world!"));
    textEdit->setReadOnly(false);

    // select all and cut
    textEdit->selectAll();
    textEdit->cut();
    QCOMPARE(textEdit->text().size(), 0);
    textEdit->paste();
    QCOMPARE(textEdit->text(), QString("Hello world!Hello world!"));
    QCOMPARE(textEdit->text().size(), 24);

    // Copy first word.
    textEdit->setCursorPosition(0);
    textEdit->selectWord();
    textEdit->copy();
    // copy: no selection, previous copy retained;
    textEdit->setCursorPosition(0);
    QCOMPARE(textEdit->selectedText(), QString());
    textEdit->copy();
    textEdit->setText(QString());
    textEdit->paste();
    QCOMPARE(textEdit->text(), QString("Hello"));
}
#endif

#if QT_CONFIG(clipboard)
void tst_qquicktextedit::canPaste()
{
    QGuiApplication::clipboard()->setText("Some text");

    QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"Hello world!\" }";
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    // check initial value - QTBUG-17765
    QTextDocument document;
    QQuickTextControl tc(&document);
    QCOMPARE(textEdit->canPaste(), tc.canPaste());
}
#endif

#if QT_CONFIG(clipboard)
void tst_qquicktextedit::canPasteEmpty()
{
    QGuiApplication::clipboard()->clear();

    QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"Hello world!\" }";
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    // check initial value - QTBUG-17765
    QTextDocument document;
    QQuickTextControl tc(&document);
    QCOMPARE(textEdit->canPaste(), tc.canPaste());
}
#endif

#if QT_CONFIG(clipboard)
void tst_qquicktextedit::middleClickPaste()
{
    if (!PlatformQuirks::isClipboardAvailable())
        QSKIP("This machine doesn't support the clipboard");

    QQuickView window(testFileUrl("mouseselection_true.qml"));

    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QVERIFY(window.rootObject() != nullptr);
    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textEditObject != nullptr);

    textEditObject->setFocus(true);

    QString originalText = textEditObject->text();
    QString selectedText = "234567";

    // press-and-drag-and-release from x1 to x2
    const QPoint p1 = textEditObject->positionToRectangle(2).center().toPoint();
    const QPoint p2 = textEditObject->positionToRectangle(8).center().toPoint();
    const QPoint p3 = textEditObject->positionToRectangle(1).center().toPoint();
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QTest::mouseMove(&window, p2);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p2);
    QTRY_COMPARE(textEditObject->selectedText(), selectedText);

    // Middle click pastes the selected text, assuming the platform supports it.
    QTest::mouseClick(&window, Qt::MiddleButton, Qt::NoModifier, p3);

    if (QGuiApplication::clipboard()->supportsSelection())
        QCOMPARE(textEditObject->text().mid(1, selectedText.size()), selectedText);
    else
        QCOMPARE(textEditObject->text(), originalText);
}
#endif

void tst_qquicktextedit::readOnly()
{
    QQuickView window(testFileUrl("readOnly.qml"));
    window.show();
    window.requestActivate();

    QVERIFY(window.rootObject() != nullptr);

    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(qvariant_cast<QObject *>(window.rootObject()->property("myInput")));

    QVERIFY(edit != nullptr);
    QTRY_VERIFY(edit->hasActiveFocus());
    QVERIFY(edit->isReadOnly());
    QString initial = edit->text();
    for (int k=Qt::Key_0; k<=Qt::Key_Z; k++)
        simulateKey(&window, k);
    simulateKey(&window, Qt::Key_Return);
    simulateKey(&window, Qt::Key_Space);
    simulateKey(&window, Qt::Key_Escape);
    QCOMPARE(edit->text(), initial);

    edit->setCursorPosition(3);
    edit->setReadOnly(false);
    QCOMPARE(edit->isReadOnly(), false);
    QCOMPARE(edit->cursorPosition(), edit->text().size());
}

void tst_qquicktextedit::inFlickableMouse_data()
{
    QTest::addColumn<bool>("readonly");
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<int>("expectFlickingAfter");
    QTest::newRow("editable") << false << true << 3;
    QTest::newRow("readonly") << true << true << 3;
    QTest::newRow("disabled") << false << false << 3;
}

void tst_qquicktextedit::inFlickableMouse()
{
    QFETCH(bool, readonly);
    QFETCH(bool, enabled);
    QFETCH(int, expectFlickingAfter);

    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QQuickView view(testFileUrl("inFlickable.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QQuickFlickable *flick = qobject_cast<QQuickFlickable *>(view.rootObject());
    QVERIFY(flick);
    QQuickTextEdit *edit = flick->findChild<QQuickTextEdit*>("text");
    QVERIFY(edit);
    edit->setReadOnly(readonly);
    edit->setEnabled(enabled);

    // flick with mouse
    QPoint p(10, 100);
    QTest::mousePress(&view, Qt::LeftButton, {}, p);
    QObject *pressGrabber = QPointingDevicePrivate::get(QPointingDevice::primaryPointingDevice())->firstPointExclusiveGrabber();
    // even if TextEdit is readonly, it still grabs on press.  But not if it's disabled.
    if (enabled)
        QCOMPARE(pressGrabber, edit);
    else
        QCOMPARE(pressGrabber, flick);
    int i = 0;
    // after a couple of events, Flickable steals the grab and starts moving
    for (; i < 4 && !flick->isMoving(); ++i) {
        p -= QPoint(0, dragThreshold);
        QTest::mouseMove(&view, p);
    }
    QCOMPARE(flick->isMoving(), bool(expectFlickingAfter));
    if (expectFlickingAfter) {
        qCDebug(lcTests) << "flickable started moving after" << i << "moves, when we got to" << p;
        QCOMPARE(i, expectFlickingAfter);
    }
    QTest::mouseRelease(&view, Qt::LeftButton, {}, p);
}

void tst_qquicktextedit::inFlickableTouch_data()
{
    QTest::addColumn<bool>("readonly");
    QTest::addColumn<bool>("enabled");
    QTest::addColumn<int>("expectFlickingAfter");
    QTest::newRow("editable") << false << true << 3;
    QTest::newRow("readonly") << true << true << 3;
    QTest::newRow("disabled") << false << false << 3;
}

void tst_qquicktextedit::inFlickableTouch()
{
    QFETCH(bool, readonly);
    QFETCH(bool, enabled);
    QFETCH(int, expectFlickingAfter);

    const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
    QQuickView view(testFileUrl("inFlickable.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QQuickFlickable *flick = qobject_cast<QQuickFlickable *>(view.rootObject());
    QVERIFY(flick);
    QQuickTextEdit *edit = flick->findChild<QQuickTextEdit*>("text");
    QVERIFY(edit);
    edit->setReadOnly(readonly);
    edit->setEnabled(enabled);

    // flick with touch
    QPoint p(10, 100);
    QTest::touchEvent(&view, touchDevice).press(1, p, &view);
    QQuickTouchUtils::flush(&view);
    QObject *pressGrabber = QPointingDevicePrivate::get(touchDevice)->firstPointExclusiveGrabber();
    // even if TextEdit is readonly, it still grabs on press.  But not if it's disabled.
    if (enabled)
        QCOMPARE(pressGrabber, edit);
    else
        QCOMPARE(pressGrabber, flick);
    int i = 0;
    // after a couple of events, Flickable steals the grab and starts moving
    for (; i < 4 && !flick->isMoving(); ++i) {
        p -= QPoint(0, dragThreshold);
        QTest::touchEvent(&view, touchDevice).move(1, p, &view);
        QQuickTouchUtils::flush(&view);
    }
    QCOMPARE(flick->isMoving(), bool(expectFlickingAfter));
    if (expectFlickingAfter) {
        qCDebug(lcTests) << "flickable started moving after" << i << "moves, when we got to" << p;
        QCOMPARE(i, expectFlickingAfter);
    }
    QTest::touchEvent(&view, touchDevice).release(1, p, &view);
}

void tst_qquicktextedit::simulateKey(QWindow *view, int key, Qt::KeyboardModifiers modifiers)
{
    QKeyEvent press(QKeyEvent::KeyPress, key, modifiers);
    QKeyEvent release(QKeyEvent::KeyRelease, key, modifiers);

    QGuiApplication::sendEvent(view, &press);
    QGuiApplication::sendEvent(view, &release);
}

bool tst_qquicktextedit::isMainFontFixed()
{
    bool ret = QFontInfo(QGuiApplication::font()).fixedPitch();
    if (ret) {
        qCWarning(lcTests) << "QFontDatabase::GeneralFont is monospaced: markdown writing is likely to use too many backticks"
                           << QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    }
    return ret;
}

bool tst_qquicktextedit::hasWindowActivation()
{
    return (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::WindowActivation));
}

void tst_qquicktextedit::textInput()
{
    QQuickView view(testFileUrl("inputMethodEvent.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(view.rootObject());
    QVERIFY(edit);
    QVERIFY(edit->hasActiveFocus());

    // test that input method event is committed and change signal is emitted
    QSignalSpy spy(edit, SIGNAL(textChanged()));
    QInputMethodEvent event;
    event.setCommitString( "Hello world!", 0, 0);
    QGuiApplication::sendEvent(edit, &event);
    QCOMPARE(edit->text(), QString("Hello world!"));
    QCOMPARE(spy.size(), 1);

    // QTBUG-12339
    // test that document and internal text attribute are in sync
    QQuickTextEditPrivate *editPrivate = static_cast<QQuickTextEditPrivate*>(QQuickItemPrivate::get(edit));
    QCOMPARE(editPrivate->text, QString("Hello world!"));

    QInputMethodQueryEvent queryEvent(Qt::ImEnabled);
    QGuiApplication::sendEvent(edit, &queryEvent);
    QCOMPARE(queryEvent.value(Qt::ImEnabled).toBool(), true);

    edit->setReadOnly(true);
    QGuiApplication::sendEvent(edit, &queryEvent);
    QCOMPARE(queryEvent.value(Qt::ImEnabled).toBool(), false);

    edit->setReadOnly(false);

    QInputMethodEvent preeditEvent("PREEDIT", QList<QInputMethodEvent::Attribute>());
    QGuiApplication::sendEvent(edit, &preeditEvent);
    QCOMPARE(edit->text(), QString("Hello world!"));
    QCOMPARE(edit->preeditText(), QString("PREEDIT"));

    QInputMethodEvent preeditEvent2("PREEDIT2", QList<QInputMethodEvent::Attribute>());
    QGuiApplication::sendEvent(edit, &preeditEvent2);
    QCOMPARE(edit->text(), QString("Hello world!"));
    QCOMPARE(edit->preeditText(), QString("PREEDIT2"));

    QInputMethodEvent preeditEvent3("", QList<QInputMethodEvent::Attribute>());
    QGuiApplication::sendEvent(edit, &preeditEvent3);
    QCOMPARE(edit->text(), QString("Hello world!"));
    QCOMPARE(edit->preeditText(), QString(""));
}

void tst_qquicktextedit::inputMethodUpdate()
{
    PlatformInputContext platformInputContext;
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = &platformInputContext;

    QQuickView view(testFileUrl("inputMethodEvent.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(view.rootObject());
    QVERIFY(edit);
    QVERIFY(edit->hasActiveFocus());

    // text change even without cursor position change needs to trigger update
    edit->setText("test");
    platformInputContext.clear();
    edit->setText("xxxx");
    QVERIFY(platformInputContext.m_updateCallCount > 0);

    // input method event replacing text
    platformInputContext.clear();
    {
        QInputMethodEvent inputMethodEvent;
        inputMethodEvent.setCommitString("y", -1, 1);
        QGuiApplication::sendEvent(edit, &inputMethodEvent);
    }
    QVERIFY(platformInputContext.m_updateCallCount > 0);

    // input method changing selection
    platformInputContext.clear();
    {
        QList<QInputMethodEvent::Attribute> attributes;
        attributes << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 0, 2, QVariant());
        QInputMethodEvent inputMethodEvent("", attributes);
        QGuiApplication::sendEvent(edit, &inputMethodEvent);
    }
    QVERIFY(edit->selectionStart() != edit->selectionEnd());
    QVERIFY(platformInputContext.m_updateCallCount > 0);

    // programmatical selections trigger update
    platformInputContext.clear();
    edit->selectAll();
    QVERIFY(platformInputContext.m_updateCallCount > 0);

    // font changes
    platformInputContext.clear();
    QFont font = edit->font();
    font.setBold(!font.bold());
    edit->setFont(font);
    QVERIFY(platformInputContext.m_updateCallCount > 0);

    // normal input
    platformInputContext.clear();
    {
        QInputMethodEvent inputMethodEvent;
        inputMethodEvent.setCommitString("y");
        QGuiApplication::sendEvent(edit, &inputMethodEvent);
    }
    QVERIFY(platformInputContext.m_updateCallCount > 0);

    // changing cursor position
    platformInputContext.clear();
    edit->setCursorPosition(0);
    QVERIFY(platformInputContext.m_updateCallCount > 0);

    // continuing with selection
    platformInputContext.clear();
    edit->moveCursorSelection(1);
    QVERIFY(platformInputContext.m_updateCallCount > 0);

    // read only disabled input method
    platformInputContext.clear();
    edit->setReadOnly(true);
    QVERIFY(platformInputContext.m_updateCallCount > 0);
    edit->setReadOnly(false);

    // no updates while no focus
    edit->setFocus(false);
    platformInputContext.clear();
    edit->setText("Foo");
    QCOMPARE(platformInputContext.m_updateCallCount, 0);
    edit->setCursorPosition(1);
    QCOMPARE(platformInputContext.m_updateCallCount, 0);
    edit->selectAll();
    QCOMPARE(platformInputContext.m_updateCallCount, 0);
    edit->setReadOnly(true);
    QCOMPARE(platformInputContext.m_updateCallCount, 0);
}

void tst_qquicktextedit::openInputPanel()
{
    PlatformInputContext platformInputContext;
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = &platformInputContext;

    QQuickView view(testFileUrl("openInputPanel.qml"));
    view.showNormal();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(view.rootObject());
    QVERIFY(edit);

    // check default values
    QVERIFY(edit->focusOnPress());
    QVERIFY(!edit->hasActiveFocus());
    QVERIFY(qApp->focusObject() != edit);

    QCOMPARE(qApp->inputMethod()->isVisible(), false);

    // input panel should open on focus
    QPoint centerPoint(view.width()/2, view.height()/2);
    Qt::KeyboardModifiers noModifiers = Qt::NoModifier;
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QVERIFY(edit->hasActiveFocus());
    QCOMPARE(qApp->focusObject(), edit);
    QCOMPARE(qApp->inputMethod()->isVisible(), true);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);

    // input panel should be re-opened when pressing already focused TextEdit
    qApp->inputMethod()->hide();
    QCOMPARE(qApp->inputMethod()->isVisible(), false);
    QVERIFY(edit->hasActiveFocus());
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QCOMPARE(qApp->inputMethod()->isVisible(), true);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);

    // input panel should stay visible if focus is lost to another text editor
    QSignalSpy inputPanelVisibilitySpy(qApp->inputMethod(), SIGNAL(visibleChanged()));
    QQuickTextEdit anotherEdit;
    anotherEdit.setParentItem(view.rootObject());
    anotherEdit.setFocus(true);
    QCOMPARE(qApp->inputMethod()->isVisible(), true);
    QCOMPARE(qApp->focusObject(), qobject_cast<QObject*>(&anotherEdit));
    QCOMPARE(inputPanelVisibilitySpy.size(), 0);

    anotherEdit.setFocus(false);
    QVERIFY(qApp->focusObject() != &anotherEdit);
    QCOMPARE(view.activeFocusItem(), view.contentItem());
    anotherEdit.setFocus(true);

    qApp->inputMethod()->hide();

    // input panel should not be opened if TextEdit is read only
    edit->setReadOnly(true);
    edit->setFocus(true);
    QCOMPARE(qApp->inputMethod()->isVisible(), false);
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);
    QGuiApplication::processEvents();
    QCOMPARE(qApp->inputMethod()->isVisible(), false);

    // input panel should not be opened if focusOnPress is set to false
    edit->setFocusOnPress(false);
    edit->setFocus(false);
    edit->setFocus(true);
    QCOMPARE(qApp->inputMethod()->isVisible(), false);
    QTest::mousePress(&view, Qt::LeftButton, noModifiers, centerPoint);
    QTest::mouseRelease(&view, Qt::LeftButton, noModifiers, centerPoint);
    QCOMPARE(qApp->inputMethod()->isVisible(), false);

    inputMethodPrivate->testContext = nullptr;
}

void tst_qquicktextedit::geometrySignals()
{
    QQmlComponent component(&engine, testFileUrl("geometrySignals.qml"));
    QObject *o = component.create();
    QVERIFY(o);
    QCOMPARE(o->property("bindingWidth").toInt(), 400);
    QCOMPARE(o->property("bindingHeight").toInt(), 500);
    delete o;
}

#if QT_CONFIG(clipboard)
void tst_qquicktextedit::pastingRichText_QTBUG_14003()
{
    QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.PlainText }";
    QQmlComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickTextEdit *obj = qobject_cast<QQuickTextEdit*>(component.create());

    QTRY_VERIFY(obj != nullptr);
    QTRY_COMPARE(obj->textFormat(), QQuickTextEdit::PlainText);

    QMimeData *mData = new QMimeData;
    mData->setHtml("<font color=\"red\">Hello</font>");
    QGuiApplication::clipboard()->setMimeData(mData);

    obj->paste();
    QTRY_COMPARE(obj->text(), QString());
    QTRY_COMPARE(obj->textFormat(), QQuickTextEdit::PlainText);
}
#endif

void tst_qquicktextedit::implicitSize_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("wrap");
    QTest::addColumn<QString>("format");
    QTest::newRow("plain") << "The quick red fox jumped over the lazy brown dog" << "TextEdit.NoWrap" << "TextEdit.PlainText";
    QTest::newRow("richtext") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "TextEdit.NoWrap" << "TextEdit.RichText";
    QTest::newRow("plain_wrap") << "The quick red fox jumped over the lazy brown dog" << "TextEdit.Wrap" << "TextEdit.PlainText";
    QTest::newRow("richtext_wrap") << "<b>The quick red fox jumped over the lazy brown dog</b>" << "TextEdit.Wrap" << "TextEdit.RichText";
}

void tst_qquicktextedit::implicitSize()
{
    QFETCH(QString, text);
    QFETCH(QString, wrap);
    QFETCH(QString, format);
    QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"" + text + "\"; width: 50; wrapMode: " + wrap + "; textFormat: " + format + " }";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit*>(textComponent.create());

    QVERIFY(textObject->width() < textObject->implicitWidth());
    QCOMPARE(textObject->height(), textObject->implicitHeight());

    textObject->resetWidth();
    QCOMPARE(textObject->width(), textObject->implicitWidth());
    QCOMPARE(textObject->height(), textObject->implicitHeight());
}

void tst_qquicktextedit::implicitSize_QTBUG_63153()
{
    QString componentStr = "import QtQuick 2.0\nTextEdit { }";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit*>(textComponent.create());
    textObject->setText("short");
    qreal shortImplicitWidth = textObject->implicitWidth();
    textObject->setText("in contrast to short this is long");
    QVERIFY2(shortImplicitWidth < textObject->implicitWidth(), qPrintable(QString("%1 < %2").arg(textObject->implicitWidth()).arg(shortImplicitWidth)));
}

void tst_qquicktextedit::contentSize()
{
    QString componentStr = "import QtQuick 2.0\nTextEdit { width: 75; height: 16; font.pixelSize: 10 }";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit *>(object.data());

    QSignalSpy spy(textObject, SIGNAL(contentSizeChanged()));

    textObject->setText("The quick red fox jumped over the lazy brown dog");

    QVERIFY(textObject->contentWidth() > textObject->width());
    QVERIFY(textObject->contentHeight() < textObject->height());
    QCOMPARE(spy.size(), 1);

    textObject->setWrapMode(QQuickTextEdit::WordWrap);
    QVERIFY(textObject->contentWidth() <= textObject->width());
    QVERIFY(textObject->contentHeight() > textObject->height());
    QCOMPARE(spy.size(), 2);

    textObject->setText("The quickredfoxjumpedoverthe lazy brown dog");

    QVERIFY(textObject->contentWidth() > textObject->width());
    QVERIFY(textObject->contentHeight() > textObject->height());
    QCOMPARE(spy.size(), 3);
}

void tst_qquicktextedit::implicitSizeBinding_data()
{
    implicitSize_data();
}

void tst_qquicktextedit::implicitSizeBinding()
{
    QFETCH(QString, text);
    QFETCH(QString, wrap);
    QFETCH(QString, format);
    QString componentStr = "import QtQuick 2.0\nTextEdit { text: \"" + text + "\"; width: implicitWidth; height: implicitHeight; wrapMode: " + wrap + "; textFormat: " + format + " }";
    QQmlComponent textComponent(&engine);
    textComponent.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(textComponent.create());
    QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit *>(object.data());

    QCOMPARE(textObject->width(), textObject->implicitWidth());
    QCOMPARE(textObject->height(), textObject->implicitHeight());

    textObject->resetWidth();
    QCOMPARE(textObject->width(), textObject->implicitWidth());
    QCOMPARE(textObject->height(), textObject->implicitHeight());

    textObject->resetHeight();
    QCOMPARE(textObject->width(), textObject->implicitWidth());
    QCOMPARE(textObject->height(), textObject->implicitHeight());
}

void tst_qquicktextedit::largeTextObservesViewport_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QQuickTextEdit::TextFormat>("textFormat");
    QTest::addColumn<bool>("parentIsViewport");
    QTest::addColumn<int>("cursorPos");
    QTest::addColumn<int>("scrollDelta");   // non-zero to move TextEdit in viewport

    QTest::addColumn<int>("expectedBlockTolerance");
    QTest::addColumn<int>("expectedBlocksAboveViewport");
    QTest::addColumn<int>("expectedBlocksPastViewport");
    QTest::addColumn<int>("expectedRenderedRegionMin");
    QTest::addColumn<int>("expectedRenderedRegionMax");

    QString text;
    {
        QStringList lines;
        // "line 100" is 8 characters; many lines are longer, some are shorter
        // so we populate 1250 lines, 11389 characters
        const int lineCount = QQuickTextEditPrivate::largeTextSizeThreshold / 8;
        lines.reserve(lineCount);
        for (int i = 0; i < lineCount; ++i)
            lines << QLatin1String("line ") + QString::number(i);
        text = lines.join('\n');
    }
    Q_ASSERT(text.size() > QQuickTextEditPrivate::largeTextSizeThreshold);

    // by default, the root item acts as the viewport:
    // QQuickTextEdit doesn't populate lines of text beyond the bottom of the window
    // cursor position 1000 is on line 121
    QTest::newRow("default plain text") << text << QQuickTextEdit::PlainText << false << 1000 << 2
                                        << 3 << 118 << 144 << 2150 << 3000;
    // make the rectangle into a viewport item, and move the text upwards:
    // QQuickTextEdit doesn't populate lines of text beyond the bottom of the viewport rectangle
    QTest::newRow("clipped plain text") << text << QQuickTextEdit::PlainText << true << 1000 << 0
                                        << 3 << 123 << 137 << 2200 << 3000;

    // scroll backwards
    QTest::newRow("scroll backwards in plain text") << text << QQuickTextEdit::PlainText << true << 1000 << 600
                                                    << 3 << 91 << 108 << 1475 << 2300;

    {
        QStringList lines;
        // "line 100" is 8 characters; many lines are longer, some are shorter
        // so we populate 1250 lines, 11389 characters
        const int lineCount = QQuickTextEditPrivate::largeTextSizeThreshold / 8;
        lines.reserve(lineCount);
        // add a table (of contents, perhaps): ensure that doesn't get included in renderedRegion after we've scrolled past it
        lines << QLatin1String("<table border='1'><tr><td>Chapter 1<td></tr><tr><td>Chapter 2</td></tr><tr><td>etc</td></tr></table>");
        for (int i = 0; i < lineCount; ++i) {
            if (i > 0 && i % 50 == 0)
                // chapter heading with floating image: ensure that doesn't get included in renderedRegion after we've scrolled past it
                lines << QLatin1String("<img style='float:left;' src='http/exists.png' height='32'/><h1>chapter ") +
                         QString::number(i / 50) + QLatin1String("</h1>");
            lines << QLatin1String("<p>line ") + QString::number(i) + QLatin1String("</p>");
        }
        text = lines.join('\n');
    }
    Q_ASSERT(text.size() > QQuickTextEditPrivate::largeTextSizeThreshold);

    // by default, the root item acts as the viewport:
    // QQuickTextEdit doesn't populate blocks beyond the bottom of the window
    QTest::newRow("default styled text") << text << QQuickTextEdit::RichText << false << 1000 << 0
                                         << 3 << 122 << 139 << 3600 << 4500;
    // make the rectangle into a viewport item, and move the text upwards:
    // QQuickTextEdit doesn't populate blocks that don't intersect the viewport rectangle
    QTest::newRow("clipped styled text") << text << QQuickTextEdit::RichText << true << 1000 << 0
                                         << 3 << 127 << 136 << 3700 << 4360;
    // get the "chapter 2" heading into the viewport
    QTest::newRow("heading visible") << text << QQuickTextEdit::RichText << true << 800 << 0
                                     << 3 << 105 << 113 << 3050 << 3600;
    // get the "chapter 2" heading into the viewport, and then scroll backwards
    QTest::newRow("scroll backwards") << text << QQuickTextEdit::RichText << true << 800 << 20
                                     << 3 << 104 << 113 << 3000 << 3600;
    // get the "chapter 2" heading into the viewport, and then scroll forwards
    QTest::newRow("scroll forwards") << text << QQuickTextEdit::RichText << true << 800 << -50
                                     << 3 << 106 << 115 << 3000 << 3670;
}

void tst_qquicktextedit::largeTextObservesViewport()
{
    if ((QGuiApplication::platformName() == QLatin1String("offscreen"))
        || (QGuiApplication::platformName() == QLatin1String("minimal")))
        QSKIP("Skipping due to grabWindow not functional on offscreen/minimal platforms");
    QFETCH(QString, text);
    QFETCH(QQuickTextEdit::TextFormat, textFormat);
    QFETCH(bool, parentIsViewport);
    QFETCH(int, cursorPos);
    QFETCH(int, scrollDelta);
    QFETCH(int, expectedBlockTolerance);
    QFETCH(int, expectedBlocksAboveViewport);
    QFETCH(int, expectedBlocksPastViewport);
    QFETCH(int, expectedRenderedRegionMin);
    QFETCH(int, expectedRenderedRegionMax);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("viewport.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QQuickTextEdit *textItem = window.rootObject()->findChild<QQuickTextEdit*>();
    QVERIFY(textItem);
    QQuickItem *viewportItem = textItem->parentItem();
    QQuickTextEditPrivate *textPriv = QQuickTextEditPrivate::get(textItem);

    viewportItem->setFlag(QQuickItem::ItemIsViewport, parentIsViewport);
    textItem->setTextFormat(textFormat);
    textItem->setText(text);
    textItem->setFocus(true);
    if (lcTests().isDebugEnabled())
        QTest::qWait(1000);
    textItem->setCursorPosition(cursorPos);
    auto cursorRect = textItem->cursorRectangle();
    textItem->setY(-cursorRect.top());
    if (lcTests().isDebugEnabled())
        QTest::qWait(500);
    if (scrollDelta) {
        textItem->setY(textItem->y() + scrollDelta);
        if (lcTests().isDebugEnabled())
            QTest::qWait(500);
    }
    qCDebug(lcTests) << "text size" << textItem->text().size() << "lines" << textItem->lineCount() << "font" << textItem->font();
    Q_ASSERT(textItem->text().size() > QQuickTextEditPrivate::largeTextSizeThreshold);
    QVERIFY(textItem->flags().testFlag(QQuickItem::ItemObservesViewport)); // large text sets this flag automatically
    QCOMPARE(textItem->viewportItem(), parentIsViewport ? viewportItem : viewportItem->parentItem());
    QTRY_COMPARE_GT(textPriv->firstBlockInViewport, 0); // wait for rendering
    qCDebug(lcTests) << "first block rendered" << textPriv->firstBlockInViewport
                     << "expected" << expectedBlocksAboveViewport
                     << "first block past viewport" << textPriv->firstBlockPastViewport
                     << "expected" << expectedBlocksPastViewport
                     << "region" << textPriv->renderedRegion << "bottom" << textPriv->renderedRegion.bottom()
                     << "expected range" << expectedRenderedRegionMin << expectedRenderedRegionMax;
    if (scrollDelta >= 0) { // unfortunately firstBlockInViewport isn't always reliable after scrolling
        QTRY_IMPL((qAbs(textPriv->firstBlockInViewport - expectedBlocksAboveViewport) <= expectedBlockTolerance), 5000);
    }
    QVERIFY2((qAbs(textPriv->firstBlockInViewport - expectedBlocksAboveViewport) <= expectedBlockTolerance),
             qPrintable(QString::fromLatin1("Expected first block in viewport %1 to be near %2 (tolerance: %3)")
                        .arg(textPriv->firstBlockInViewport).arg(expectedBlocksAboveViewport).arg(expectedBlockTolerance)));
    QVERIFY2((qAbs(textPriv->firstBlockPastViewport - expectedBlocksPastViewport) <= expectedBlockTolerance),
             qPrintable(QString::fromLatin1("Expected first block past viewport %1 to be near %2 (tolerance: %3)")
                        .arg(textPriv->firstBlockPastViewport).arg(expectedBlocksPastViewport).arg(expectedBlockTolerance)));
    QCOMPARE_GT(textPriv->renderedRegion.top(), expectedRenderedRegionMin);
    QCOMPARE_LT(textPriv->renderedRegion.bottom(), expectedRenderedRegionMax);
    QVERIFY(textPriv->cursorItem);
    qCDebug(lcTests) << "cursor rect" << textItem->cursorRectangle() << "visible?" << textPriv->cursorItem->isVisible();
    QCOMPARE(textPriv->cursorItem->isVisible(), textPriv->renderedRegion.intersects(textItem->cursorRectangle()));
}

void tst_qquicktextedit::largeTextSelection()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("qtConfigureHelp.qml")));
    NodeCheckerTextEdit *textItem = qmlobject_cast<NodeCheckerTextEdit *>(window.rootObject());
    QVERIFY(textItem);
    QTRY_VERIFY(textItem->sortedLinePositions.size() > 0);
    const auto sortedLinePositions = textItem->sortedLinePositions;

    QQuickTextEditPrivate *textPriv = QQuickTextEditPrivate::get(textItem);
    QSignalSpy renderSpy(&window, &QQuickWindow::afterRendering);
    if (lcTests().isDebugEnabled())
        QTest::qWait(500); // for visual check; not needed in CI

    const int renderCount = renderSpy.size();
    textItem->setCursorPosition(200);
    textItem->moveCursorSelection(220);
    QTRY_COMPARE_GT(renderSpy.size(), renderCount);

    if (lcTests().isDebugEnabled())
        QTest::qWait(500); // for visual check; not needed in CI

    qCDebug(lcTests) << "TextEdit's nodes" << textPriv->textNodeMap;
    qCDebug(lcTests) << "font" << textItem->font() << "line positions"
                     << textItem->sortedLinePositions << "expected" << sortedLinePositions;

    const bool eachTextNodeRenderedOnlyOnce = [textItem]() -> bool {
        for (auto i = 1; i < textItem->sortedLinePositions.count(); ++i)
            if (textItem->sortedLinePositions[i - 1] == textItem->sortedLinePositions[i])
                return false;
        return true;
    }();
    QVERIFY(eachTextNodeRenderedOnlyOnce);
}

void tst_qquicktextedit::renderingAroundSelection()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("threeLines.qml")));
    NodeCheckerTextEdit *textItem = qmlobject_cast<NodeCheckerTextEdit*>(window.rootObject());
    QVERIFY(textItem);
    QTRY_VERIFY(textItem->sortedLinePositions.size() > 0);
    const auto sortedLinePositions = textItem->sortedLinePositions;
    const int lastLinePosition = textItem->lastLinePosition;
    QQuickTextEditPrivate *textPriv = QQuickTextEditPrivate::get(textItem);
    QSignalSpy renderSpy(&window, &QQuickWindow::afterRendering);

    if (lcTests().isDebugEnabled())
        QTest::qWait(500); // for visual check; not needed in CI

    const int renderCount = renderSpy.size();
    QPoint p1 = textItem->mapToScene(textItem->positionToRectangle(8).center()).toPoint();
    QPoint p2 = textItem->mapToScene(textItem->positionToRectangle(10).center()).toPoint();
    qCDebug(lcTests) << "drag from" << p1 << "to" << p2;
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p1);
    QTest::mouseMove(&window, p2);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p2);
    // ensure that QQuickTextEdit::updatePaintNode() has a chance to run
    QTRY_COMPARE_GT(renderSpy.size(), renderCount);

    if (lcTests().isDebugEnabled())
        QTest::qWait(500); // for visual check; not needed in CI

    qCDebug(lcTests) << "TextEdit's nodes" << textPriv->textNodeMap;
    qCDebug(lcTests) << "font" << textItem->font() << "line positions" << textItem->sortedLinePositions << "should be" << sortedLinePositions;
    QCOMPARE(textItem->lastLinePosition, lastLinePosition);
    QTRY_COMPARE(textItem->sortedLinePositions, sortedLinePositions);
}

void tst_qquicktextedit::signal_editingfinished()
{
    QQuickView *window = new QQuickView(nullptr);
    window->setBaseSize(QSize(800,600));

    window->setSource(testFileUrl("signal_editingfinished.qml"));
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));
    QCOMPARE(QGuiApplication::focusWindow(), window);

    QVERIFY(window->rootObject() != nullptr);

    QQuickTextEdit *input1 = qobject_cast<QQuickTextEdit *>(qvariant_cast<QObject *>(window->rootObject()->property("input1")));
    QVERIFY(input1);
    QQuickTextEdit *input2 = qobject_cast<QQuickTextEdit *>(qvariant_cast<QObject *>(window->rootObject()->property("input2")));
    QVERIFY(input2);

    QSignalSpy editingFinished1Spy(input1, SIGNAL(editingFinished()));

    input1->setFocus(true);
    QTRY_VERIFY(input1->hasActiveFocus());
    QTRY_VERIFY(!input2->hasActiveFocus());

    {
        QKeyEvent key(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());
        QTRY_COMPARE(editingFinished1Spy.size(), 1);

        QTRY_VERIFY(!input1->hasActiveFocus());
        QTRY_VERIFY(input2->hasActiveFocus());
    }

    QSignalSpy editingFinished2Spy(input2, SIGNAL(editingFinished()));

    input2->setFocus(true);
    QTRY_VERIFY(!input1->hasActiveFocus());
    QTRY_VERIFY(input2->hasActiveFocus());

    {
        QKeyEvent key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::ShiftModifier, "", false, 1);
        QGuiApplication::sendEvent(window, &key);
        QVERIFY(key.isAccepted());
        QTRY_COMPARE(editingFinished2Spy.size(), 1);

        QTRY_VERIFY(input1->hasActiveFocus());
        QTRY_VERIFY(!input2->hasActiveFocus());
    }
}

void tst_qquicktextedit::clipRect()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n TextEdit {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(object.data());
    QVERIFY(edit);

    QCOMPARE(edit->clipRect().x(), qreal(0));
    QCOMPARE(edit->clipRect().y(), qreal(0));

    QCOMPARE(edit->clipRect().width(), edit->width() + edit->cursorRectangle().width());
    QCOMPARE(edit->clipRect().height(), edit->height());

    edit->setText("Hello World");
    QCOMPARE(edit->clipRect().x(), qreal(0));
    QCOMPARE(edit->clipRect().y(), qreal(0));
    // XXX: TextEdit allows an extra 3 pixels boundary for the cursor beyond it's width for non
    // empty text. TextInput doesn't.
    QCOMPARE(edit->clipRect().width(), edit->width() + edit->cursorRectangle().width() + 3);
    QCOMPARE(edit->clipRect().height(), edit->height());

    // clip rect shouldn't exceed the size of the item, expect for the cursor width;
    edit->setWidth(edit->width() / 2);
    QCOMPARE(edit->clipRect().x(), qreal(0));
    QCOMPARE(edit->clipRect().y(), qreal(0));
    QCOMPARE(edit->clipRect().width(), edit->width() + edit->cursorRectangle().width() + 3);
    QCOMPARE(edit->clipRect().height(), edit->height());

    edit->setHeight(edit->height() * 2);
    QCOMPARE(edit->clipRect().x(), qreal(0));
    QCOMPARE(edit->clipRect().y(), qreal(0));
    QCOMPARE(edit->clipRect().width(), edit->width() + edit->cursorRectangle().width() + 3);
    QCOMPARE(edit->clipRect().height(), edit->height());

    QQmlComponent cursorComponent(&engine);
    cursorComponent.setData("import QtQuick 2.0\nRectangle { height: 20; width: 8 }", QUrl());

    edit->setCursorDelegate(&cursorComponent);
    edit->setCursorVisible(true);

    // If a cursor delegate is used it's size should determine the excess width.
    QCOMPARE(edit->clipRect().x(), qreal(0));
    QCOMPARE(edit->clipRect().y(), qreal(0));
    QCOMPARE(edit->clipRect().width(), edit->width() + 8 + 3);
    QCOMPARE(edit->clipRect().height(), edit->height());

    // Alignment and wrapping don't affect the clip rect.
    edit->setHAlign(QQuickTextEdit::AlignRight);
    QCOMPARE(edit->clipRect().x(), qreal(0));
    QCOMPARE(edit->clipRect().y(), qreal(0));
    QCOMPARE(edit->clipRect().width(), edit->width() + 8 + 3);
    QCOMPARE(edit->clipRect().height(), edit->height());

    edit->setWrapMode(QQuickTextEdit::Wrap);
    QCOMPARE(edit->clipRect().x(), qreal(0));
    QCOMPARE(edit->clipRect().y(), qreal(0));
    QCOMPARE(edit->clipRect().width(), edit->width() + 8 + 3);
    QCOMPARE(edit->clipRect().height(), edit->height());

    edit->setVAlign(QQuickTextEdit::AlignBottom);
    QCOMPARE(edit->clipRect().x(), qreal(0));
    QCOMPARE(edit->clipRect().y(), qreal(0));
    QCOMPARE(edit->clipRect().width(), edit->width() + 8 + 3);
    QCOMPARE(edit->clipRect().height(), edit->height());
}

void tst_qquicktextedit::boundingRect()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n TextEdit {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(object.data());
    QVERIFY(edit);

    QTextLayout layout;
    layout.setFont(edit->font());

    if (!qmlDisableDistanceField()) {
        QTextOption option;
        option.setUseDesignMetrics(true);
        layout.setTextOption(option);
    }
    layout.beginLayout();
    QTextLine line = layout.createLine();
    layout.endLayout();

    QCOMPARE(edit->boundingRect().x(), qreal(0));
    QCOMPARE(edit->boundingRect().y(), qreal(0));
    QCOMPARE(edit->boundingRect().width(), edit->cursorRectangle().width());
    QCOMPARE(edit->boundingRect().height(), line.height());

    edit->setText("Hello World");

    layout.setText(edit->text());
    layout.beginLayout();
    line = layout.createLine();
    layout.endLayout();

    QCOMPARE(edit->boundingRect().x(), qreal(0));
    QCOMPARE(edit->boundingRect().y(), qreal(0));
    QCOMPARE(edit->boundingRect().width(), line.naturalTextWidth() + edit->cursorRectangle().width() + 3);

    QFontMetricsF fontMetrics(QGuiApplication::font());
    qreal leading = fontMetrics.leading();
    qreal ascent = fontMetrics.ascent();
    qreal descent = fontMetrics.descent();

    bool leadingOverflow = qCeil(ascent + descent) < qCeil(ascent + descent + leading);
    if (leadingOverflow)
        QEXPECT_FAIL("", "See QTBUG-82954", Continue);
    QCOMPARE(edit->boundingRect().height(), line.height());

    // the size of the bounding rect shouldn't be bounded by the size of item.
    edit->setWidth(edit->width() / 2);
    QCOMPARE(edit->boundingRect().x(), qreal(0));
    QCOMPARE(edit->boundingRect().y(), qreal(0));
    QCOMPARE(edit->boundingRect().width(), line.naturalTextWidth() + edit->cursorRectangle().width() + 3);

    if (leadingOverflow)
        QEXPECT_FAIL("", "See QTBUG-82954", Continue);
    QCOMPARE(edit->boundingRect().height(), line.height());

    edit->setHeight(edit->height() * 2);
    QCOMPARE(edit->boundingRect().x(), qreal(0));
    QCOMPARE(edit->boundingRect().y(), qreal(0));
    QCOMPARE(edit->boundingRect().width(), line.naturalTextWidth() + edit->cursorRectangle().width() + 3);

    if (leadingOverflow)
        QEXPECT_FAIL("", "See QTBUG-82954", Continue);
    QCOMPARE(edit->boundingRect().height(), line.height());

    QQmlComponent cursorComponent(&engine);
    cursorComponent.setData("import QtQuick 2.0\nRectangle { height: 20; width: 8 }", QUrl());

    edit->setCursorDelegate(&cursorComponent);
    edit->setCursorVisible(true);

    // Don't include the size of a cursor delegate as it has its own bounding rect.
    QCOMPARE(edit->boundingRect().x(), qreal(0));
    QCOMPARE(edit->boundingRect().y(), qreal(0));
    QCOMPARE(edit->boundingRect().width(), line.naturalTextWidth());

    if (leadingOverflow)
        QEXPECT_FAIL("", "See QTBUG-82954", Continue);
    QCOMPARE(edit->boundingRect().height(), line.height());

    edit->setHAlign(QQuickTextEdit::AlignRight);
    QCOMPARE(edit->boundingRect().x(), edit->width() - line.naturalTextWidth());
    QCOMPARE(edit->boundingRect().y(), qreal(0));
    QCOMPARE(edit->boundingRect().width(), line.naturalTextWidth());

    if (leadingOverflow)
        QEXPECT_FAIL("", "See QTBUG-82954", Continue);
    QCOMPARE(edit->boundingRect().height(), line.height());

    edit->setWrapMode(QQuickTextEdit::Wrap);
    QCOMPARE(edit->boundingRect().right(), edit->width());
    QCOMPARE(edit->boundingRect().y(), qreal(0));
    QVERIFY(edit->boundingRect().width() < line.naturalTextWidth());
    QVERIFY(edit->boundingRect().height() > line.height());

    edit->setVAlign(QQuickTextEdit::AlignBottom);
    QCOMPARE(edit->boundingRect().right(), edit->width());
    QCOMPARE(edit->boundingRect().bottom(), edit->height());
    QVERIFY(edit->boundingRect().width() < line.naturalTextWidth());
    QVERIFY(edit->boundingRect().height() > line.height());
}

void tst_qquicktextedit::preeditCursorRectangle()
{
    QString preeditText = "super";

    QQuickView view(testFileUrl("inputMethodEvent.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(view.rootObject());
    QVERIFY(edit);

    QQuickItem *cursor = edit->findChild<QQuickItem *>("cursor");
    QVERIFY(cursor);

    QSignalSpy editSpy(edit, SIGNAL(cursorRectangleChanged()));
    QSignalSpy panelSpy(qGuiApp->inputMethod(), SIGNAL(cursorRectangleChanged()));

    QRectF currentRect;

    QCOMPARE(QGuiApplication::focusObject(), static_cast<QObject *>(edit));
    QInputMethodQueryEvent query(Qt::ImCursorRectangle);
    QCoreApplication::sendEvent(edit, &query);
    QRectF previousRect = query.value(Qt::ImCursorRectangle).toRectF();

    // Verify that the micro focus rect is positioned the same for position 0 as
    // it would be if there was no preedit text.
    QInputMethodEvent imEvent(preeditText, QList<QInputMethodEvent::Attribute>()
            << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 0, preeditText.size(), QVariant()));
    QCoreApplication::sendEvent(edit, &imEvent);
    QCoreApplication::sendEvent(edit, &query);
    currentRect = query.value(Qt::ImCursorRectangle).toRectF();
    QCOMPARE(edit->cursorRectangle(), currentRect);
    QCOMPARE(cursor->position(), currentRect.topLeft());
    QCOMPARE(currentRect, previousRect);

    // Verify that the micro focus rect moves to the left as the cursor position
    // is incremented.
    editSpy.clear();
    panelSpy.clear();
    for (int i = 1; i <= 5; ++i) {
        QInputMethodEvent imEvent(preeditText, QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, i, preeditText.size(), QVariant()));
        QCoreApplication::sendEvent(edit, &imEvent);
        QCoreApplication::sendEvent(edit, &query);
        currentRect = query.value(Qt::ImCursorRectangle).toRectF();
        QCOMPARE(edit->cursorRectangle(), currentRect);
        QCOMPARE(cursor->position(), currentRect.topLeft());
        QVERIFY(previousRect.left() < currentRect.left());
        QCOMPARE(editSpy.size(), 1); editSpy.clear();
        QCOMPARE(panelSpy.size(), 1); panelSpy.clear();
        previousRect = currentRect;
    }

    // Verify that if the cursor rectangle is updated if the pre-edit text changes
    // but the (non-zero) cursor position is the same.
    editSpy.clear();
    panelSpy.clear();
    {   QInputMethodEvent imEvent("wwwww", QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 5, 1, QVariant()));
        QCoreApplication::sendEvent(edit, &imEvent); }
    QCoreApplication::sendEvent(edit, &query);
    currentRect = query.value(Qt::ImCursorRectangle).toRectF();
    QCOMPARE(edit->cursorRectangle(), currentRect);
    QCOMPARE(cursor->position(), currentRect.topLeft());
    QCOMPARE(editSpy.size(), 1);
    QCOMPARE(panelSpy.size(), 1);

    // Verify that if there is no preedit cursor then the micro focus rect is the
    // same as it would be if it were positioned at the end of the preedit text.
    editSpy.clear();
    panelSpy.clear();
    {   QInputMethodEvent imEvent(preeditText, QList<QInputMethodEvent::Attribute>());
        QCoreApplication::sendEvent(edit, &imEvent); }
    QCoreApplication::sendEvent(edit, &query);
    currentRect = query.value(Qt::ImCursorRectangle).toRectF();
    QCOMPARE(edit->cursorRectangle(), currentRect);
    QCOMPARE(cursor->position(), currentRect.topLeft());
    QCOMPARE(currentRect, previousRect);
    QCOMPARE(editSpy.size(), 1);
    QCOMPARE(panelSpy.size(), 1);
}

void tst_qquicktextedit::inputMethodComposing()
{
    QString text = "supercalifragisiticexpialidocious!";

    QQuickView view(testFileUrl("inputContext.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));

    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(view.rootObject());
    QVERIFY(edit);
    QCOMPARE(QGuiApplication::focusObject(), static_cast<QObject *>(edit));

    QSignalSpy spy(edit, SIGNAL(inputMethodComposingChanged()));
    edit->setCursorPosition(12);

    QCOMPARE(edit->isInputMethodComposing(), false);

    {
        QInputMethodEvent event(text.mid(3), QList<QInputMethodEvent::Attribute>());
        QGuiApplication::sendEvent(edit, &event);
    }

    QCOMPARE(edit->isInputMethodComposing(), true);
    QCOMPARE(spy.size(), 1);

    {
        QInputMethodEvent event(text.mid(12), QList<QInputMethodEvent::Attribute>());
        QGuiApplication::sendEvent(edit, &event);
    }
    QCOMPARE(spy.size(), 1);

    {
        QInputMethodEvent event;
        QGuiApplication::sendEvent(edit, &event);
    }
    QCOMPARE(edit->isInputMethodComposing(), false);
    QCOMPARE(spy.size(), 2);

    // Changing the text while not composing doesn't alter the composing state.
    edit->setText(text.mid(0, 16));
    QCOMPARE(edit->isInputMethodComposing(), false);
    QCOMPARE(spy.size(), 2);

    {
        QInputMethodEvent event(text.mid(16), QList<QInputMethodEvent::Attribute>());
        QGuiApplication::sendEvent(edit, &event);
    }
    QCOMPARE(edit->isInputMethodComposing(), true);
    QCOMPARE(spy.size(), 3);

    // Changing the text while composing cancels composition.
    edit->setText(text.mid(0, 12));
    QCOMPARE(edit->isInputMethodComposing(), false);
    QCOMPARE(spy.size(), 4);

    {   // Preedit cursor positioned outside (empty) preedit; composing.
        QInputMethodEvent event(QString(), QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, -2, 1, QVariant()));
        QGuiApplication::sendEvent(edit, &event);
    }
    QCOMPARE(edit->isInputMethodComposing(), true);
    QCOMPARE(spy.size(), 5);

    {   // Cursor hidden; composing
        QInputMethodEvent event(QString(), QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 0, 0, QVariant()));
        QGuiApplication::sendEvent(edit, &event);
    }
    QCOMPARE(edit->isInputMethodComposing(), true);
    QCOMPARE(spy.size(), 5);

    {   // Default cursor attributes; composing.
        QInputMethodEvent event(QString(), QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 0, 1, QVariant()));
        QGuiApplication::sendEvent(edit, &event);
    }
    QCOMPARE(edit->isInputMethodComposing(), true);
    QCOMPARE(spy.size(), 5);

    {   // Selections are persisted: not composing
        QInputMethodEvent event(QString(), QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 2, 4, QVariant()));
        QGuiApplication::sendEvent(edit, &event);
    }
    QCOMPARE(edit->isInputMethodComposing(), false);
    QCOMPARE(spy.size(), 6);

    edit->setCursorPosition(0);

    {   // Formatting applied; composing.
        QTextCharFormat format;
        format.setUnderlineStyle(QTextCharFormat::SingleUnderline);
        QInputMethodEvent event(QString(), QList<QInputMethodEvent::Attribute>()
                << QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat, 2, 4, format));
        QGuiApplication::sendEvent(edit, &event);
    }
    QCOMPARE(edit->isInputMethodComposing(), true);
    QCOMPARE(spy.size(), 7);

    {
        QInputMethodEvent event;
        QGuiApplication::sendEvent(edit, &event);
    }
    QCOMPARE(edit->isInputMethodComposing(), false);
    QCOMPARE(spy.size(), 8);
}

void tst_qquicktextedit::cursorRectangleSize_data()
{
    QTest::addColumn<bool>("useCursorDelegate");

    QTest::newRow("default cursor") << false;
    QTest::newRow("custom cursor delegate") << true;
}

void tst_qquicktextedit::cursorRectangleSize()
{
    QFETCH(bool, useCursorDelegate);

    QQuickView *window = new QQuickView(testFileUrl("positionAt.qml"));
    QVERIFY(window->rootObject() != nullptr);
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit *>(window->rootObject());

    QQmlComponent cursorDelegate(window->engine());
    if (useCursorDelegate) {
        cursorDelegate.setData("import QtQuick 2.0\nRectangle { width:10; height:10; }", QUrl());
        textEdit->setCursorDelegate(&cursorDelegate);
    }

    // make sure cursor rectangle is not at (0,0)
    textEdit->setX(10);
    textEdit->setY(10);
    textEdit->setCursorPosition(3);
    QVERIFY(textEdit != nullptr);
    textEdit->setFocus(true);
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(window));

    QInputMethodQueryEvent event(Qt::ImCursorRectangle);
    qApp->sendEvent(textEdit, &event);
    QRectF cursorRectFromQuery = event.value(Qt::ImCursorRectangle).toRectF();

    QRectF cursorRectFromItem = textEdit->cursorRectangle();
    QRectF cursorRectFromPositionToRectangle = textEdit->positionToRectangle(textEdit->cursorPosition());

    QVERIFY(cursorRectFromItem.isValid());
    QVERIFY(cursorRectFromQuery.isValid());
    QVERIFY(cursorRectFromPositionToRectangle.isValid());

    // item and input query cursor rectangles match
    QCOMPARE(cursorRectFromItem, cursorRectFromQuery);

    // item cursor rectangle and positionToRectangle calculations match
    QCOMPARE(cursorRectFromItem, cursorRectFromPositionToRectangle);

    // item-window transform and input item transform match
    QCOMPARE(QQuickItemPrivate::get(textEdit)->itemToWindowTransform(), qApp->inputMethod()->inputItemTransform());

    // input panel cursorRectangle property and tranformed item cursor rectangle match
    QRectF sceneCursorRect = QQuickItemPrivate::get(textEdit)->itemToWindowTransform().mapRect(cursorRectFromItem);
    QCOMPARE(sceneCursorRect, qApp->inputMethod()->cursorRectangle());

    delete window;
}

void tst_qquicktextedit::getText_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("start");
    QTest::addColumn<int>("end");
    QTest::addColumn<QString>("expectedText");

    const QString richBoldText = QStringLiteral("This is some <b>bold</b> text");
    const QString plainBoldText = QStringLiteral("This is some bold text");
    const QString richBoldTextLB = QStringLiteral("This is some<br/><b>bold</b> text");
    const QString plainBoldTextLB = QString(QStringLiteral("This is some\nbold text")).replace(QLatin1Char('\n'), QChar(QChar::LineSeparator));

    QTest::newRow("all plain text")
            << standard.at(0)
            << 0 << standard.at(0).size()
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
            << 23 << standard.at(0).size() + 8
            << standard.at(0).mid(23);

    QTest::newRow("plain text cropped beginning and end")
            << standard.at(0)
            << -9 << standard.at(0).size() + 4
            << standard.at(0);

    QTest::newRow("all rich text")
            << richBoldText
            << 0 << plainBoldText.size()
            << plainBoldText;

    QTest::newRow("rich text sub string")
            << richBoldText
            << 14 << 21
            << plainBoldText.mid(14, 7);

    // Line break.
    QTest::newRow("all plain text (line break)")
            << standard.at(1)
            << 0 << standard.at(1).size()
            << standard.at(1);

    QTest::newRow("plain text sub string (line break)")
            << standard.at(1)
            << 0 << 12
            << standard.at(1).mid(0, 12);

    QTest::newRow("plain text sub string reversed (line break)")
            << standard.at(1)
            << 12 << 0
            << standard.at(1).mid(0, 12);

    QTest::newRow("plain text cropped beginning (line break)")
            << standard.at(1)
            << -3 << 4
            << standard.at(1).mid(0, 4);

    QTest::newRow("plain text cropped end (line break)")
            << standard.at(1)
            << 23 << standard.at(1).size() + 8
            << standard.at(1).mid(23);

    QTest::newRow("plain text cropped beginning and end (line break)")
            << standard.at(1)
            << -9 << standard.at(1).size() + 4
            << standard.at(1);

    QTest::newRow("all rich text (line break)")
            << richBoldTextLB
            << 0 << plainBoldTextLB.size()
            << plainBoldTextLB;

    QTest::newRow("rich text sub string (line break)")
            << richBoldTextLB
            << 14 << 21
            << plainBoldTextLB.mid(14, 7);
}

void tst_qquicktextedit::getText()
{
    QFETCH(QString, text);
    QFETCH(int, start);
    QFETCH(int, end);
    QFETCH(QString, expectedText);

    QString componentStr = "import QtQuick 2.0\nTextEdit { textFormat: TextEdit.AutoText; text: \"" + text + "\" }";
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

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
            << 0 << standard.at(0).size()
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
            << 23 << standard.at(0).size() + 8
            << standard.at(0).mid(23);

    QTest::newRow("plain text cropped beginning and end")
            << standard.at(0)
            << QQuickTextEdit::PlainText
            << -9 << standard.at(0).size() + 4
            << standard.at(0);

    QTest::newRow("all rich (Auto) text")
            << richBoldText
            << QQuickTextEdit::AutoText
            << 0 << plainBoldText.size()
            << QString("This is some \\<.*\\>bold\\</.*\\> text");

    QTest::newRow("all rich (Rich) text")
            << richBoldText
            << QQuickTextEdit::RichText
            << 0 << plainBoldText.size()
            << QString("This is some \\<.*\\>bold\\</.*\\> text");

    QTest::newRow("all rich (Plain) text")
            << richBoldText
            << QQuickTextEdit::PlainText
            << 0 << richBoldText.size()
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
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    textEdit->setTextFormat(textFormat);
    textEdit->setText(text);

    if (textFormat == QQuickTextEdit::RichText
            || (textFormat == QQuickTextEdit::AutoText && Qt::mightBeRichText(text))) {
        QVERIFY(textEdit->getFormattedText(start, end).contains(QRegularExpression(expectedText)));
    } else {
        QCOMPARE(textEdit->getFormattedText(start, end), expectedText);
    }
}

void tst_qquicktextedit::append_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QQuickTextEdit::TextFormat>("textFormat");
    QTest::addColumn<int>("selectionStart");
    QTest::addColumn<int>("selectionEnd");
    QTest::addColumn<QString>("appendText");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<int>("expectedSelectionStart");
    QTest::addColumn<int>("expectedSelectionEnd");
    QTest::addColumn<int>("expectedCursorPosition");
    QTest::addColumn<bool>("selectionChanged");
    QTest::addColumn<bool>("cursorPositionChanged");

    QTest::newRow("cursor kept intact (beginning)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0
            << QString("Hello")
            << standard.at(0) + QString("\nHello")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("cursor kept intact (middle)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 18 << 18
            << QString("Hello")
            << standard.at(0) + QString("\nHello")
            << 18 << 18 << 18
            << false << false;

    QTest::newRow("cursor follows (end)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << standard.at(0).size() << standard.at(0).size()
            << QString("Hello")
            << standard.at(0) + QString("\nHello")
            << standard.at(0).size() + 6 << standard.at(0).size() + 6 << standard.at(0).size() + 6
            << false << true;

    QTest::newRow("selection kept intact (beginning)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 18
            << QString("Hello")
            << standard.at(0) + QString("\nHello")
            << 0 << 18 << 18
            << false << false;

    QTest::newRow("selection kept intact (middle)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 14 << 18
            << QString("Hello")
            << standard.at(0) + QString("\nHello")
            << 14 << 18 << 18
            << false << false;

    QTest::newRow("selection kept intact, cursor follows (end)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 18 << standard.at(0).size()
            << QString("Hello")
            << standard.at(0) + QString("\nHello")
            << 18 << standard.at(0).size() + 6 << standard.at(0).size() + 6
            << true << true;

    QTest::newRow("reversed selection kept intact")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 18 << 14
            << QString("Hello")
            << standard.at(0) + QString("\nHello")
            << 14 << 18 << 14
            << false << false;

    QTest::newRow("rich text into plain text")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0
            << QString("<b>Hello</b>")
            << standard.at(0) + QString("\n<b>Hello</b>")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("rich text into rich text")
            << standard.at(0) << QQuickTextEdit::RichText
            << 0 << 0
            << QString("<b>Hello</b>")
            << standard.at(0) + QChar(QChar::ParagraphSeparator) + QString("Hello")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("rich text into auto text")
            << standard.at(0) << QQuickTextEdit::AutoText
            << 0 << 0
            << QString("<b>Hello</b>")
            << standard.at(0) + QString("\nHello")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("markdown into markdown")
            << QString("**Hello**") << QQuickTextEdit::MarkdownText
            << 0 << 0
            << QString(" *world*")
            << QString("Hello\u2029world")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("rich text into markdown")
            << QString("**Hello**") << QQuickTextEdit::MarkdownText
            << 0 << 0
            << QString(" <i>world</i>")
            << QString("Hello\u2029world")
            << 0 << 0 << 0
            << false << false;
}

void tst_qquicktextedit::append()
{
    QFETCH(QString, text);
    QFETCH(QQuickTextEdit::TextFormat, textFormat);
    QFETCH(int, selectionStart);
    QFETCH(int, selectionEnd);
    QFETCH(QString, appendText);
    QFETCH(QString, expectedText);
    QFETCH(int, expectedSelectionStart);
    QFETCH(int, expectedSelectionEnd);
    QFETCH(int, expectedCursorPosition);
    QFETCH(bool, selectionChanged);
    QFETCH(bool, cursorPositionChanged);

    QString componentStr = "import QtQuick 2.2\nTextEdit { text: \"" + text + "\" }";
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    textEdit->setTextFormat(textFormat);
    textEdit->select(selectionStart, selectionEnd);

    QSignalSpy selectionSpy(textEdit, SIGNAL(selectedTextChanged()));
    QSignalSpy selectionStartSpy(textEdit, SIGNAL(selectionStartChanged()));
    QSignalSpy selectionEndSpy(textEdit, SIGNAL(selectionEndChanged()));
    QSignalSpy textSpy(textEdit, SIGNAL(textChanged()));
    QSignalSpy cursorPositionSpy(textEdit, SIGNAL(cursorPositionChanged()));

    textEdit->append(appendText);

    if (textFormat == QQuickTextEdit::RichText || textFormat == QQuickTextEdit::MarkdownText ||
            (textFormat == QQuickTextEdit::AutoText &&
             (Qt::mightBeRichText(text) || Qt::mightBeRichText(appendText)))) {
        QCOMPARE(textEdit->getText(0, expectedText.size()), expectedText);
    } else {
        QCOMPARE(textEdit->text(), expectedText);

    }
    QCOMPARE(textEdit->length(), expectedText.size());

    QCOMPARE(textEdit->selectionStart(), expectedSelectionStart);
    QCOMPARE(textEdit->selectionEnd(), expectedSelectionEnd);
    QCOMPARE(textEdit->cursorPosition(), expectedCursorPosition);

    if (selectionStart > selectionEnd)
        qSwap(selectionStart, selectionEnd);

    QCOMPARE(selectionSpy.size() > 0, selectionChanged);
    QCOMPARE(selectionStartSpy.size() > 0, selectionStart != expectedSelectionStart);
    QCOMPARE(selectionEndSpy.size() > 0, selectionEnd != expectedSelectionEnd);
    QCOMPARE(textSpy.size() > 0, text != expectedText);
    QCOMPARE(cursorPositionSpy.size() > 0, cursorPositionChanged);
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
            << standard.at(0).size() << standard.at(0).size() << standard.at(0).size()
            << QString("Hello")
            << standard.at(0) + QString("Hello")
            << standard.at(0).size() + 5 << standard.at(0).size() + 5 << standard.at(0).size() + 5
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
            << standard.at(0).size() << standard.at(0).size() << 18
            << QString("Hello")
            << standard.at(0).mid(0, 18) + QString("Hello") + standard.at(0).mid(18)
            << standard.at(0).size() + 5 << standard.at(0).size() + 5 << standard.at(0).size() + 5
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
            << 18 << 18 << standard.at(0).size()
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
            << true << true;

    QTest::newRow("before reversed selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 19 << 14 << 0
            << QString("Hello")
            << QString("Hello") + standard.at(0)
            << 19 << 24 << 19
            << true << true;

    QTest::newRow("after selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 14 << 19 << standard.at(0).size()
            << QString("Hello")
            << standard.at(0) + QString("Hello")
            << 14 << 19 << 19
            << false << false;

    QTest::newRow("after reversed selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 19 << 14 << standard.at(0).size()
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
            << 0 << 0 << standard.at(0).size() + 3
            << QString("Hello")
            << standard.at(0)
            << 0 << 0 << 0
            << false << false;

    const QString markdownBaseString("# Hello\nWorld\n");
    QTest::newRow("markdown into markdown at end")
            << markdownBaseString << QQuickTextEdit::MarkdownText
            << 0 << 0 << 11
            << QString("\n## Other\ntext")
            << QString("Hello\u2029World\u2029Other\u2029text")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("markdown into markdown in the middle")
            << markdownBaseString << QQuickTextEdit::MarkdownText
            << 0 << 0 << 6
            << QString("## Other\ntext\n")
            << QString("Hello\u2029Other\u2029text\u2029World")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("markdown into markdown in the middle no newlines")
            << markdownBaseString << QQuickTextEdit::MarkdownText
            << 0 << 0 << 6
            << QString("## Other\ntext")
            << QString("Hello\u2029Other\u2029textWorld")
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("markdown with bold span into markdown")
            << QString("# Heading\n text") << QQuickTextEdit::MarkdownText
            << 0 << 0 << 8
            << QString("*Body*")
            << QString("Heading\u2029Bodytext")
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
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    textEdit->setTextFormat(textFormat);
    textEdit->select(selectionStart, selectionEnd);

    QSignalSpy selectionSpy(textEdit, SIGNAL(selectedTextChanged()));
    QSignalSpy selectionStartSpy(textEdit, SIGNAL(selectionStartChanged()));
    QSignalSpy selectionEndSpy(textEdit, SIGNAL(selectionEndChanged()));
    QSignalSpy textSpy(textEdit, SIGNAL(textChanged()));
    QSignalSpy cursorPositionSpy(textEdit, SIGNAL(cursorPositionChanged()));

    textEdit->insert(insertPosition, insertText);

    if (textFormat == QQuickTextEdit::RichText || textFormat == QQuickTextEdit::MarkdownText ||
            (textFormat == QQuickTextEdit::AutoText &&
                (Qt::mightBeRichText(text) || Qt::mightBeRichText(insertText)))) {
        QCOMPARE(textEdit->getText(0, expectedText.size()), expectedText);
        qCDebug(lcTests) << "with formatting:" << textEdit->getFormattedText(0, 100);
    } else {
        QCOMPARE(textEdit->text(), expectedText);
    }
    QCOMPARE(textEdit->length(), expectedText.size());

    QCOMPARE(textEdit->selectionStart(), expectedSelectionStart);
    QCOMPARE(textEdit->selectionEnd(), expectedSelectionEnd);
    QCOMPARE(textEdit->cursorPosition(), expectedCursorPosition);

    if (selectionStart > selectionEnd)
        qSwap(selectionStart, selectionEnd);

    QCOMPARE(selectionSpy.size() > 0, selectionChanged);
    QCOMPARE(selectionStartSpy.size() > 0, selectionStart != expectedSelectionStart);
    QCOMPARE(selectionEndSpy.size() > 0, selectionEnd != expectedSelectionEnd);
    QCOMPARE(textSpy.size() > 0, text != expectedText);
    QCOMPARE(cursorPositionSpy.size() > 0, cursorPositionChanged);
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
            << standard.at(0).size() << standard.at(0).size()
            << standard.at(0).size() << standard.at(0).size() - 5
            << standard.at(0).mid(0, standard.at(0).size() - 5)
            << standard.at(0).size() - 5 << standard.at(0).size() - 5 << standard.at(0).size() - 5
            << false << true;

    QTest::newRow("to cursor position (end)")
            << standard.at(0) << QQuickTextEdit::PlainText
            << standard.at(0).size() << standard.at(0).size()
            << standard.at(0).size() - 5 << standard.at(0).size()
            << standard.at(0).mid(0, standard.at(0).size() - 5)
            << standard.at(0).size() - 5 << standard.at(0).size() - 5 << standard.at(0).size() - 5
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
            << standard.at(0).size() << standard.at(0).size()
            << 18 << 23
            << standard.at(0).mid(0, 18) + standard.at(0).mid(23)
            << standard.at(0).size() - 5 << standard.at(0).size() - 5 << standard.at(0).size() - 5
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
            << true << true;

    QTest::newRow("before reversed selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 19 << 14
            << 0 << 5
            << standard.at(0).mid(5)
            << 9 << 14 << 9
            << true << true;

    QTest::newRow("after selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 14 << 19
            << standard.at(0).size() - 5 << standard.at(0).size()
            << standard.at(0).mid(0, standard.at(0).size() - 5)
            << 14 << 19 << 19
            << false << false;

    QTest::newRow("after reversed selection")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 19 << 14
            << standard.at(0).size() - 5 << standard.at(0).size()
            << standard.at(0).mid(0, standard.at(0).size() - 5)
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
            << 23 << standard.at(0).size() + 8
            << standard.at(0).mid(0, 23)
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("plain text cropped beginning and end")
            << standard.at(0) << QQuickTextEdit::PlainText
            << 0 << 0
            << -9 << standard.at(0).size() + 4
            << QString()
            << 0 << 0 << 0
            << false << false;

    QTest::newRow("all rich text")
            << richBoldText << QQuickTextEdit::RichText
            << 0 << 0
            << 0 << plainBoldText.size()
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
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    textEdit->setTextFormat(textFormat);
    textEdit->select(selectionStart, selectionEnd);

    QSignalSpy selectionSpy(textEdit, SIGNAL(selectedTextChanged()));
    QSignalSpy selectionStartSpy(textEdit, SIGNAL(selectionStartChanged()));
    QSignalSpy selectionEndSpy(textEdit, SIGNAL(selectionEndChanged()));
    QSignalSpy textSpy(textEdit, SIGNAL(textChanged()));
    QSignalSpy cursorPositionSpy(textEdit, SIGNAL(cursorPositionChanged()));

    textEdit->remove(removeStart, removeEnd);

    if (textFormat == QQuickTextEdit::RichText
            || (textFormat == QQuickTextEdit::AutoText && Qt::mightBeRichText(text))) {
        QCOMPARE(textEdit->getText(0, expectedText.size()), expectedText);
    } else {
        QCOMPARE(textEdit->text(), expectedText);
    }
    QCOMPARE(textEdit->length(), expectedText.size());

    if (selectionStart > selectionEnd)  //
        qSwap(selectionStart, selectionEnd);

    QCOMPARE(textEdit->selectionStart(), expectedSelectionStart);
    QCOMPARE(textEdit->selectionEnd(), expectedSelectionEnd);
    QCOMPARE(textEdit->cursorPosition(), expectedCursorPosition);

    QCOMPARE(selectionSpy.size() > 0, selectionChanged);
    QCOMPARE(selectionStartSpy.size() > 0, selectionStart != expectedSelectionStart);
    QCOMPARE(selectionEndSpy.size() > 0, selectionEnd != expectedSelectionEnd);
    QCOMPARE(textSpy.size() > 0, text != expectedText);


    if (cursorPositionChanged)  //
        QVERIFY(cursorPositionSpy.size() > 0);
}

#if QT_CONFIG(shortcut)

void tst_qquicktextedit::keySequence_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QKeySequence>("sequence");
    QTest::addColumn<int>("selectionStart");
    QTest::addColumn<int>("selectionEnd");
    QTest::addColumn<int>("cursorPosition");
    QTest::addColumn<QString>("expectedText");
    QTest::addColumn<QString>("selectedText");
    QTest::addColumn<Qt::Key>("layoutDirection");

    // standard[0] == "the [4]quick [10]brown [16]fox [20]jumped [27]over [32]the [36]lazy [41]dog"

    QTest::newRow("select all")
            << standard.at(0) << QKeySequence(QKeySequence::SelectAll) << 0 << 0
            << 44 << standard.at(0) << standard.at(0)
            << Qt::Key_Direction_L;
    QTest::newRow("select start of line")
            << standard.at(0) << QKeySequence(QKeySequence::SelectStartOfLine) << 5 << 5
            << 0 << standard.at(0) << standard.at(0).mid(0, 5)
            << Qt::Key_Direction_L;
    QTest::newRow("select start of block")
            << standard.at(0) << QKeySequence(QKeySequence::SelectStartOfBlock) << 5 << 5
            << 0 << standard.at(0) << standard.at(0).mid(0, 5)
            << Qt::Key_Direction_L;
    QTest::newRow("select end of line")
            << standard.at(0) << QKeySequence(QKeySequence::SelectEndOfLine) << 5 << 5
            << 44 << standard.at(0) << standard.at(0).mid(5)
            << Qt::Key_Direction_L;
    QTest::newRow("select end of document")
            << standard.at(0) << QKeySequence(QKeySequence::SelectEndOfDocument) << 3 << 3
            << 44 << standard.at(0) << standard.at(0).mid(3)
            << Qt::Key_Direction_L;
    QTest::newRow("select end of block")
            << standard.at(0) << QKeySequence(QKeySequence::SelectEndOfBlock) << 18 << 18
            << 44 << standard.at(0) << standard.at(0).mid(18)
            << Qt::Key_Direction_L;
    QTest::newRow("delete end of line")
            << standard.at(0) << QKeySequence(QKeySequence::DeleteEndOfLine) << 24 << 24
            << 24 << standard.at(0).mid(0, 24) << QString()
            << Qt::Key_Direction_L;
    QTest::newRow("move to start of line")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToStartOfLine) << 31 << 31
            << 0 << standard.at(0) << QString()
            << Qt::Key_Direction_L;
    QTest::newRow("move to start of block")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToStartOfBlock) << 25 << 25
            << 0 << standard.at(0) << QString()
            << Qt::Key_Direction_L;
    QTest::newRow("move to next char")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToNextChar) << 12 << 12
            << 13 << standard.at(0) << QString()
            << Qt::Key_Direction_L;
    QTest::newRow("move to previous char (ltr)")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToPreviousChar) << 3 << 3
            << 2 << standard.at(0) << QString()
            << Qt::Key_Direction_L;
    QTest::newRow("move to previous char (rtl)")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToPreviousChar) << 3 << 3
            << 4 << standard.at(0) << QString()
            << Qt::Key_Direction_R;
    QTest::newRow("move to previous char with selection")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToPreviousChar) << 3 << 7
            << 3 << standard.at(0) << QString()
            << Qt::Key_Direction_L;
    QTest::newRow("select next char (ltr)")
            << standard.at(0) << QKeySequence(QKeySequence::SelectNextChar) << 23 << 23
            << 24 << standard.at(0) << standard.at(0).mid(23, 1)
            << Qt::Key_Direction_L;
    QTest::newRow("select next char (rtl)")
            << standard.at(0) << QKeySequence(QKeySequence::SelectNextChar) << 23 << 23
            << 22 << standard.at(0) << standard.at(0).mid(22, 1)
            << Qt::Key_Direction_R;
    QTest::newRow("select previous char (ltr)")
            << standard.at(0) << QKeySequence(QKeySequence::SelectPreviousChar) << 19 << 19
            << 18 << standard.at(0) << standard.at(0).mid(18, 1)
            << Qt::Key_Direction_L;
    QTest::newRow("select previous char (rtl)")
            << standard.at(0) << QKeySequence(QKeySequence::SelectPreviousChar) << 19 << 19
            << 20 << standard.at(0) << standard.at(0).mid(19, 1)
            << Qt::Key_Direction_R;
    QTest::newRow("move to next word (ltr)")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToNextWord) << 7 << 7
            << 10 << standard.at(0) << QString()
            << Qt::Key_Direction_L;
    QTest::newRow("move to next word (rtl)")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToNextWord) << 7 << 7
            << 4 << standard.at(0) << QString()
            << Qt::Key_Direction_R;
    QTest::newRow("move to previous word (ltr)")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToPreviousWord) << 7 << 7
            << 4 << standard.at(0) << QString()
            << Qt::Key_Direction_L;
    QTest::newRow("move to previous word (rlt)")
            << standard.at(0) << QKeySequence(QKeySequence::MoveToPreviousWord) << 7 << 7
            << 10 << standard.at(0) << QString()
            << Qt::Key_Direction_R;
    QTest::newRow("select next word")
            << standard.at(0) << QKeySequence(QKeySequence::SelectNextWord) << 11 << 11
            << 16 << standard.at(0) << standard.at(0).mid(11, 5)
            << Qt::Key_Direction_L;
    QTest::newRow("select previous word")
            << standard.at(0) << QKeySequence(QKeySequence::SelectPreviousWord) << 11 << 11
            << 10 << standard.at(0) << standard.at(0).mid(10, 1)
            << Qt::Key_Direction_L;
    QTest::newRow("delete (selection)")
            << standard.at(0) << QKeySequence(QKeySequence::Delete) << 12 << 15
            << 12 << (standard.at(0).mid(0, 12) + standard.at(0).mid(15)) << QString()
            << Qt::Key_Direction_L;
    QTest::newRow("delete (no selection)")
            << standard.at(0) << QKeySequence(QKeySequence::Delete) << 15 << 15
            << 15 << (standard.at(0).mid(0, 15) + standard.at(0).mid(16)) << QString()
            << Qt::Key_Direction_L;
    QTest::newRow("delete end of word")
            << standard.at(0) << QKeySequence(QKeySequence::DeleteEndOfWord) << 24 << 24
            << 24 << (standard.at(0).mid(0, 24) + standard.at(0).mid(27)) << QString()
            << Qt::Key_Direction_L;
    QTest::newRow("delete start of word")
            << standard.at(0) << QKeySequence(QKeySequence::DeleteStartOfWord) << 7 << 7
            << 4 << (standard.at(0).mid(0, 4) + standard.at(0).mid(7)) << QString()
            << Qt::Key_Direction_L;
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
    QFETCH(Qt::Key, layoutDirection);

    if (sequence.isEmpty()) {
        QSKIP("Key sequence is undefined");
    }

    QString componentStr = "import QtQuick 2.0\nTextEdit { focus: true; text: \"" + text + "\" }";
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    QQuickWindow window;
    textEdit->setParentItem(window.contentItem());
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QVERIFY(textEdit->hasActiveFocus());

    simulateKey(&window, layoutDirection);

    textEdit->select(selectionStart, selectionEnd);

    simulateKeys(&window, sequence);

    QCOMPARE(textEdit->cursorPosition(), cursorPosition);
    QCOMPARE(textEdit->text(), expectedText);
    QCOMPARE(textEdit->selectedText(), selectedText);
}

#endif // QT_CONFIG(shortcut)

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
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    QQuickWindow window;
    textEdit->setParentItem(window.contentItem());
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QVERIFY(textEdit->hasActiveFocus());
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

        for (int j = 0; j < insertString.at(i).size(); j++)
            QTest::keyClick(&window, insertString.at(i).at(j).toLatin1());
    }

    QCOMPARE(spy.size(), 1);

// STEP 2: Next call undo several times and see if we can restore to the previous state
    for (i = 0; i < expectedString.size() - 1; ++i) {
        QCOMPARE(textEdit->text(), expectedString[i]);
        QVERIFY(textEdit->canUndo());
        textEdit->undo();
    }

// STEP 3: Verify that we have undone everything
    QVERIFY(textEdit->text().isEmpty());
    QVERIFY(!textEdit->canUndo());
    QCOMPARE(spy.size(), 2);
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
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    QQuickWindow window;
    textEdit->setParentItem(window.contentItem());
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QVERIFY(textEdit->hasActiveFocus());

    QVERIFY(!textEdit->canUndo());
    QVERIFY(!textEdit->canRedo());

    QSignalSpy spy(textEdit, SIGNAL(canRedoChanged()));

    int i;
    // inserts the diff strings at diff positions
    for (i = 0; i < insertString.size(); ++i) {
        if (insertIndex[i] > -1)
            textEdit->setCursorPosition(insertIndex[i]);
        for (int j = 0; j < insertString.at(i).size(); j++)
            QTest::keyClick(&window, insertString.at(i).at(j).toLatin1());
        QVERIFY(textEdit->canUndo());
        QVERIFY(!textEdit->canRedo());
    }

    QCOMPARE(spy.size(), 0);

    // undo everything
    while (!textEdit->text().isEmpty()) {
        QVERIFY(textEdit->canUndo());
        textEdit->undo();
        QVERIFY(textEdit->canRedo());
    }

    QCOMPARE(spy.size(), 1);

    for (i = 0; i < expectedString.size(); ++i) {
        QVERIFY(textEdit->canRedo());
        textEdit->redo();
        QCOMPARE(textEdit->text() , expectedString[i]);
        QVERIFY(textEdit->canUndo());
    }
    QVERIFY(!textEdit->canRedo());
    QCOMPARE(spy.size(), 2);
}

#if QT_CONFIG(shortcut)

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
        expectedString << "123";

        QTest::newRow("Inserts,moving,selection and overwriting") << keys << expectedString;
    }

    bool canCopyPaste = PlatformQuirks::isClipboardAvailable();

    if (canCopyPaste) {
        KeyList keys;
        keys    << "123"
                << QKeySequence(QKeySequence::SelectStartOfLine)
                << QKeySequence(QKeySequence::Cut)
                << "ABC"
                << QKeySequence(QKeySequence::Paste);
        QStringList expectedString = QStringList()
                << "ABC123"
                << "ABC"
                << ""
                << "123";
        QTest::newRow("Cut,paste") << keys << expectedString;
    }
    if (canCopyPaste) {
        KeyList keys;
        keys    << "123"
                << QKeySequence(QKeySequence::SelectStartOfLine)
                << QKeySequence(QKeySequence::Copy)
                << "ABC"
                << QKeySequence(QKeySequence::Paste);
        QStringList expectedString = QStringList()
                << "ABC123"
                << "ABC"
                << "123";
        QTest::newRow("Copy,paste") << keys << expectedString;
    }
}

void tst_qquicktextedit::undo_keypressevents()
{
    QFETCH(KeyList, keys);
    QFETCH(QStringList, expectedString);

    QString componentStr = "import QtQuick 2.0\nTextEdit { focus: true }";
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    QQuickWindow window;
    textEdit->setParentItem(window.contentItem());
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QVERIFY(textEdit->hasActiveFocus());

    simulateKeys(&window, keys);

    for (int i = 0; i < expectedString.size(); ++i) {
        QCOMPARE(textEdit->text() , expectedString[i]);
        textEdit->undo();
    }
    QVERIFY(textEdit->text().isEmpty());
}

#endif // QT_CONFIG(shortcut)

void tst_qquicktextedit::clear()
{
    QString componentStr = "import QtQuick 2.0\nTextEdit { focus: true }";
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    QQuickWindow window;
    textEdit->setParentItem(window.contentItem());
    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QVERIFY(textEdit->hasActiveFocus());

    QSignalSpy spy(textEdit, SIGNAL(canUndoChanged()));

    textEdit->setText("I am Legend");
    QCOMPARE(textEdit->text(), QString("I am Legend"));
    textEdit->clear();
    QVERIFY(textEdit->text().isEmpty());

    QCOMPARE(spy.size(), 1);

    // checks that clears can be undone
    textEdit->undo();
    QVERIFY(!textEdit->canUndo());
    QCOMPARE(spy.size(), 2);
    QCOMPARE(textEdit->text(), QString("I am Legend"));

    textEdit->setCursorPosition(4);
    QInputMethodEvent preeditEvent("PREEDIT", QList<QInputMethodEvent::Attribute>());
    QGuiApplication::sendEvent(textEdit, &preeditEvent);
    QCOMPARE(textEdit->text(), QString("I am Legend"));
    QCOMPARE(textEdit->preeditText(), QString("PREEDIT"));

    textEdit->clear();
    QVERIFY(textEdit->text().isEmpty());

    QCOMPARE(spy.size(), 3);

    // checks that clears can be undone
    textEdit->undo();
    QVERIFY(!textEdit->canUndo());
    QCOMPARE(spy.size(), 4);
    QCOMPARE(textEdit->text(), QString("I am Legend"));

    textEdit->setText(QString("<i>I am Legend</i>"));
    QCOMPARE(textEdit->text(), QString("<i>I am Legend</i>"));
    textEdit->clear();
    QVERIFY(textEdit->text().isEmpty());

    QCOMPARE(spy.size(), 5);

    // checks that clears can be undone
    textEdit->undo();
    QCOMPARE(spy.size(), 6);
    QCOMPARE(textEdit->text(), QString("<i>I am Legend</i>"));
}

void tst_qquicktextedit::baseUrl()
{
    QUrl localUrl("file:///tests/text.qml");
    QUrl remoteUrl("http://www.qt-project.org/test.qml");

    QQmlComponent textComponent(&engine);
    textComponent.setData("import QtQuick 2.0\n TextEdit {}", localUrl);
    QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit *>(textComponent.create());

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

void tst_qquicktextedit::embeddedImages_data()
{
    QTest::addColumn<QUrl>("qmlfile");
    QTest::addColumn<QString>("error");
    QTest::newRow("local") << testFileUrl("embeddedImagesLocal.qml") << "";
    QTest::newRow("local-error") << testFileUrl("embeddedImagesLocalError.qml")
        << testFileUrl("embeddedImagesLocalError.qml").toString()+":3:1: QML TextEdit: Cannot open: " + testFileUrl("http/notexists.png").toString();
    QTest::newRow("local") << testFileUrl("embeddedImagesLocalRelative.qml") << "";
    QTest::newRow("remote") << testFileUrl("embeddedImagesRemote.qml") << "";
    QTest::newRow("remote-error") << testFileUrl("embeddedImagesRemoteError.qml")
        << testFileUrl("embeddedImagesRemoteError.qml").toString()+":3:1: QML TextEdit: Error transferring {{ServerBaseUrl}}/notexists.png - server replied: Not found";
    QTest::newRow("remote") << testFileUrl("embeddedImagesRemoteRelative.qml") << "";
}

void tst_qquicktextedit::embeddedImages()
{
    QFETCH(QUrl, qmlfile);
    QFETCH(QString, error);

    TestHTTPServer server;
    QVERIFY2(server.listen(), qPrintable(server.errorString()));
    server.serveDirectory(testFile("http"));

    error.replace(QStringLiteral("{{ServerBaseUrl}}"), server.baseUrl().toString());

    if (!error.isEmpty())
        QTest::ignoreMessage(QtWarningMsg, error.toLatin1());

    QQmlComponent textComponent(&engine, qmlfile);
    QQuickTextEdit *textObject = qobject_cast<QQuickTextEdit*>(textComponent.beginCreate(engine.rootContext()));
    QVERIFY(textObject != nullptr);

    const int baseUrlPropertyIndex = textObject->metaObject()->indexOfProperty("serverBaseUrl");
    if (baseUrlPropertyIndex != -1) {
        QMetaProperty prop = textObject->metaObject()->property(baseUrlPropertyIndex);
        QVERIFY(prop.write(textObject, server.baseUrl().toString()));
    }

    textComponent.completeCreate();

    QTRY_COMPARE(QQuickTextEditPrivate::get(textObject)->document->resourcesLoading(), 0);

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

void tst_qquicktextedit::emptytags_QTBUG_22058()
{
    QQuickView window(testFileUrl("qtbug-22058.qml"));
    QVERIFY(window.rootObject() != nullptr);

    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    QQuickTextEdit *input = qobject_cast<QQuickTextEdit *>(qvariant_cast<QObject *>(window.rootObject()->property("inputField")));
    QVERIFY(input->hasActiveFocus());

    QInputMethodEvent event("", QList<QInputMethodEvent::Attribute>());
    event.setCommitString("<b>Bold<");
    QGuiApplication::sendEvent(input, &event);
    QCOMPARE(input->text(), QString("<b>Bold<"));
    event.setCommitString(">");
    QGuiApplication::sendEvent(input, &event);
    QCOMPARE(input->text(), QString("<b>Bold<>"));
}

void tst_qquicktextedit::cursorRectangle_QTBUG_38947()
{
    QQuickView window(testFileUrl("qtbug-38947.qml"));

    window.show();
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    QQuickTextEdit *edit = window.rootObject()->findChild<QQuickTextEdit *>("textedit");
    QVERIFY(edit);

    QPoint from = edit->positionToRectangle(0).center().toPoint();
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, from);

    QSignalSpy spy(edit, SIGNAL(cursorRectangleChanged()));
    QVERIFY(spy.isValid());

    for (int i = 1; i < edit->length() - 1; ++i) {
        QRectF rect = edit->positionToRectangle(i);
        QTest::mouseMove(&window, rect.center().toPoint());
        QCOMPARE(edit->cursorRectangle(), rect);
        QCOMPARE(spy.size(), i);
    }

    QPoint to = edit->positionToRectangle(edit->length() - 1).center().toPoint();
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, to);
}

void tst_qquicktextedit::textCached_QTBUG_41583()
{
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\nTextEdit { property int margin: 10; text: \"TextEdit\"; textMargin: margin; property bool empty: !text; }", QUrl());
    QQuickTextEdit *textedit = qobject_cast<QQuickTextEdit*>(component.create());
    QVERIFY(textedit);
    QCOMPARE(textedit->textMargin(), qreal(10.0));
    QCOMPARE(textedit->text(), QString("TextEdit"));
    QVERIFY(!textedit->property("empty").toBool());
}

void tst_qquicktextedit::doubleSelect_QTBUG_38704()
{
    QString componentStr = "import QtQuick 2.2\nTextEdit { text: \"TextEdit\" }";
    QQmlComponent textEditComponent(&engine);
    textEditComponent.setData(componentStr.toLatin1(), QUrl());
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit*>(textEditComponent.create());
    QVERIFY(textEdit != nullptr);

    QSignalSpy selectionSpy(textEdit, SIGNAL(selectedTextChanged()));

    textEdit->select(0,1); //Select some text initially
    QCOMPARE(selectionSpy.size(), 1);
    textEdit->select(0,1); //No change to selection start/end
    QCOMPARE(selectionSpy.size(), 1);
    textEdit->select(0,2); //Change selection end
    QCOMPARE(selectionSpy.size(), 2);
    textEdit->select(1,2); //Change selection start
    QCOMPARE(selectionSpy.size(), 3);
}

void tst_qquicktextedit::padding()
{
    QScopedPointer<QQuickView> window(new QQuickView);
    window->setSource(testFileUrl("padding.qml"));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QQuickItem *root = window->rootObject();
    QVERIFY(root);
    QQuickTextEdit *obj = qobject_cast<QQuickTextEdit*>(root);
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

    delete root;
}

void tst_qquicktextedit::paddingAndWrap()
{
    // Check that the document ends up with the correct width if
    // we set left and right padding after component completed.
    QScopedPointer<QQuickView> window(new QQuickView);
    window->setSource(testFileUrl("wordwrap.qml"));
    QTRY_COMPARE(window->status(), QQuickView::Ready);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));
    QQuickItem *root = window->rootObject();
    QVERIFY(root);
    QQuickTextEdit *obj = qobject_cast<QQuickTextEdit *>(root);
    QVERIFY(obj != nullptr);
    QTextDocument *doc = QQuickTextEditPrivate::get(obj)->document;

    QCOMPARE(doc->textWidth(), obj->width());
    obj->setLeftPadding(10);
    obj->setRightPadding(10);
    QCOMPARE(doc->textWidth(), obj->width() - obj->leftPadding() - obj->rightPadding());
    obj->setLeftPadding(0);
    obj->setRightPadding(0);
    QCOMPARE(doc->textWidth(), obj->width());
}

void tst_qquicktextedit::QTBUG_51115_readOnlyResetsSelection()
{
    QQuickView view;
    view.setSource(testFileUrl("qtbug51115.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QQuickTextEdit *obj = qobject_cast<QQuickTextEdit*>(view.rootObject());

    QCOMPARE(obj->selectedText(), QString());
}

void tst_qquicktextedit::keys_shortcutoverride()
{
    // Tests that QML TextEdit receives Keys.onShortcutOverride  (QTBUG-68711)
    QQuickView view;
    view.setSource(testFileUrl("keys_shortcutoverride.qml"));
    view.show();
    view.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&view));
    QObject *root = view.rootObject();
    QVERIFY(root);

    QQuickTextEdit *textEdit = root->findChild<QQuickTextEdit*>();
    QVERIFY(textEdit);
    QQuickRectangle *rectangle = root->findChild<QQuickRectangle*>(QLatin1String("rectangle"));
    QVERIFY(rectangle);

    // Precondition: check if its not already changed
    QCOMPARE(root->property("who").value<QString>(), QLatin1String("nobody"));

    // send Key_Escape to the Rectangle
    QVERIFY(rectangle->hasActiveFocus());
    QTest::keyPress(&view, Qt::Key_Escape);
    QCOMPARE(root->property("who").value<QString>(), QLatin1String("Rectangle"));

    // send Key_Escape to TextEdit
    textEdit->setFocus(true);
    QTest::keyPress(&view, Qt::Key_Escape);
    QCOMPARE(root->property("who").value<QString>(), QLatin1String("TextEdit"));
}

void tst_qquicktextedit::transparentSelectionColor()
{
    if (QGuiApplication::platformName() == QLatin1String("minimal"))
        QSKIP("Skipping due to grabWindow not functional on minimal platforms");

    QQuickView view;
    view.setSource(testFileUrl("transparentSelectionColor.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QObject *root = view.rootObject();
    QVERIFY(root);

    QQuickTextEdit *textEdit = root->findChild<QQuickTextEdit *>();
    QVERIFY(textEdit);
    textEdit->selectAll();

    QImage img = view.grabWindow();
    QCOMPARE(img.isNull(), false);

    QColor color = img.pixelColor(int(textEdit->width() / 2), int(textEdit->height()) / 2);
    QVERIFY(color.red() > 250);
    QVERIFY(color.blue() < 10);
    QVERIFY(color.green() < 10);
}

void tst_qquicktextedit::keyEventPropagation()
{
    QQuickView view;
    view.setSource(testFileUrl("keyEventPropagation.qml"));
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));
    QObject *root = view.rootObject();
    QVERIFY(root);

    QSignalSpy downSpy(root, SIGNAL(keyDown(int)));
    QSignalSpy upSpy(root, SIGNAL(keyUp(int)));

    QQuickTextEdit *textEdit = root->findChild<QQuickTextEdit *>();
    QVERIFY(textEdit->hasActiveFocus());
    simulateKey(&view, Qt::Key_Back);
    QCOMPARE(downSpy.size(), 1);
    QCOMPARE(upSpy.size(), 1);
    auto downKey = downSpy.takeFirst();
    auto upKey = upSpy.takeFirst();
    QCOMPARE(downKey.at(0).toInt(), Qt::Key_Back);
    QCOMPARE(upKey.at(0).toInt(), Qt::Key_Back);

    simulateKey(&view, Qt::Key_Shift);
    QCOMPARE(downSpy.size(), 1);
    QCOMPARE(upSpy.size(), 1);
    downKey = downSpy.takeFirst();
    upKey = upSpy.takeFirst();
    QCOMPARE(downKey.at(0).toInt(), Qt::Key_Shift);
    QCOMPARE(upKey.at(0).toInt(), Qt::Key_Shift);

    simulateKey(&view, Qt::Key_A);
    QCOMPARE(downSpy.size(), 0);
    QCOMPARE(upSpy.size(), 0);

    simulateKey(&view, Qt::Key_Right);
    QCOMPARE(downSpy.size(), 0);
    QCOMPARE(upSpy.size(), 1);
    upKey = upSpy.takeFirst();
    QCOMPARE(upKey.at(0).toInt(), Qt::Key_Right);
}

void tst_qquicktextedit::markdown()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("markdown.qml")));
    QQuickTextEdit *te = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(te);
    QVERIFY(te->textDocument());
    auto doc = te->textDocument()->textDocument();
    QVERIFY(doc);
    const QString mdSource("# Heading\n\nBody\n\n");

    // Component.onCompleted has pre-populated a string in italics
    QCOMPARE(te->text(), "*whee*\n\n");
    QCOMPARE(te->getText(0, 100), "whee");
    QCOMPARE(te->getFormattedText(0, 100), "*whee*\n\n");
    QVERIFY(QTextCursor(doc).charFormat().font().italic());

    if (isMainFontFixed())
        QSKIP("fixed-pitch main font (QTBUG-103484)");
    te->clear();
    te->insert(0, mdSource);
    QCOMPARE(te->text(), mdSource);
    QCOMPARE(te->getText(0, 12), "Heading\u2029Body");
    QCOMPARE(te->getFormattedText(0, 12), "# Heading\n\nBody\n\n");
    QCOMPARE(QTextCursor(doc).blockFormat().headingLevel(), 1);

    te->selectAll();
    QCOMPARE(te->selectedText(), "Heading\u2029Body");

    te->clear();
    te->setText(mdSource);
    QCOMPARE(te->text(), mdSource);
    QCOMPARE(te->getText(0, 12), "Heading\u2029Body");
    QCOMPARE(te->getFormattedText(0, 12), "# Heading\n\nBody\n\n");
    QCOMPARE(QTextCursor(doc).blockFormat().headingLevel(), 1);

    te->insert(12, "_text_");
    QCOMPARE(te->text(), "# Heading\n\nBody_text_\n\n");
    QCOMPARE(te->getText(0, 100), "Heading\u2029Bodytext");
    QCOMPARE(te->getFormattedText(0, 100), "# Heading\n\nBody_text_\n\n");
    QTextCursor cursor(doc);
    cursor.movePosition(QTextCursor::End);
    QVERIFY(cursor.charFormat().fontUnderline());
}

#if QT_CONFIG(clipboard)
void tst_qquicktextedit::pasteHtmlIntoMarkdown()
{
    if (isMainFontFixed())
        QSKIP("fixed-pitch main font (QTBUG-103484)");
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("markdown.qml")));
    QQuickTextEdit *te = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(te);
    QVERIFY(te->textDocument());
    auto doc = te->textDocument()->textDocument();
    QVERIFY(doc);

    QMimeData *mData = new QMimeData; // not a leak: if it's stack-allocated, we get a double free
    mData->setHtml("<b>Hello <i>world</i></b>");
    QGuiApplication::clipboard()->setMimeData(mData);

    te->selectAll();
    te->paste();
    QTRY_COMPARE(te->text(), "**Hello *world***\n\n");
    QCOMPARE(te->getFormattedText(0, 100), "**Hello *world***\n\n");
    QCOMPARE(te->textFormat(), QQuickTextEdit::MarkdownText);
    QCOMPARE(te->getText(0, 100), "Hello world");

    QGuiApplication::clipboard()->clear();
    te->selectAll();
    te->copy();
    QTRY_VERIFY(QGuiApplication::clipboard()->mimeData()->hasHtml());
    QVERIFY(QGuiApplication::clipboard()->mimeData()->hasText());
    const auto *md = QGuiApplication::clipboard()->mimeData();
    qCDebug(lcTests) << "mime types available" << md->formats();
    qCDebug(lcTests) << "HTML" << md->html();
    // QTextDocumentFragment::toHtml() is subject to change, so we don't QCOMPARE this verbose HTML
    QVERIFY(md->html().toLatin1().startsWith('<'));
}
#endif

void tst_qquicktextedit::touchscreenDoesNotSelect_data()
{
    QTest::addColumn<QUrl>("src");
    QTest::addColumn<bool>("mouseOnly");
    QTest::newRow("new") << testFileUrl("mouseselectionmode_default.qml") << true;
#if QT_VERSION < QT_VERSION_CHECK(7, 0, 0)
    QTest::newRow("old") << testFileUrl("mouseselection_old_default.qml") << false;
#endif
}

void tst_qquicktextedit::touchscreenDoesNotSelect()
{
    QFETCH(QUrl, src);
    QFETCH(bool, mouseOnly);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, src));

    QQuickTextEdit *textEditObject = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textEditObject != nullptr);
    QCOMPARE(textEditObject->selectByMouse(), mouseOnly);
    textEditObject->setSelectByMouse(true); // enable selection with pre-6.4 import version

    // press-drag-and-release from x1 to x2
    int x1 = 10;
    int x2 = 70;
    int y = QFontMetrics(textEditObject->font()).height() / 2;
    QTest::touchEvent(&window, touchDevice).press(0, QPoint(x1,y), &window);
    QTest::touchEvent(&window, touchDevice).move(0, QPoint(x2,y), &window);
    QTest::touchEvent(&window, touchDevice).release(0, QPoint(x2,y), &window);
    QQuickTouchUtils::flush(&window);
    // if the import version is old enough, fall back to old behavior: touch swipe _does_ select text
    QCOMPARE(textEditObject->selectedText().isEmpty(), mouseOnly);
}

void tst_qquicktextedit::touchscreenSetsFocusAndMovesCursor()
{
    if (!hasWindowActivation())
        QSKIP("Window activation is not supported");
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("twoInAColumn.qml")));
    window.requestActivate();
    QVERIFY(QTest::qWaitForWindowActive(&window));

    QQuickTextEdit *top = window.rootObject()->findChild<QQuickTextEdit*>("top");
    QVERIFY(top);
    QQuickTextEdit *bottom = window.rootObject()->findChild<QQuickTextEdit*>("bottom");
    QVERIFY(bottom);
    const auto len = bottom->text().size();

    // tap the bottom field
    QPoint p1 = bottom->mapToScene({6, 6}).toPoint();
    QTest::touchEvent(&window, touchDevice).press(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(qApp->focusObject(), bottom);
    // text cursor is at 0 by default, on press
    QCOMPARE(bottom->cursorPosition(), 0);
    // so typing a character prepends it
    QVERIFY(!bottom->text().startsWith('q'));
    QTest::keyClick(&window, Qt::Key_Q);
    QVERIFY(bottom->text().startsWith('q'));
    QCOMPARE(bottom->text().size(), len + 1);
    QTest::touchEvent(&window, touchDevice).release(0, p1, &window);
    QQuickTouchUtils::flush(&window);
    // the cursor gets moved on release, as long as TextInput's grab wasn't stolen (e.g. by Flickable)
    QVERIFY(bottom->cursorPosition() < 5);

    // press-drag-and-release from p1 to p2 on the top field
    p1 = top->mapToScene({6, 6}).toPoint();
    QPoint p2 = top->mapToScene({76, 6}).toPoint();
    QTest::touchEvent(&window, touchDevice).press(0, p1, &window);
    QTest::touchEvent(&window, touchDevice).move(0, p2, &window);
    QTest::touchEvent(&window, touchDevice).release(0, p2, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(qApp->focusObject(), top);
    QVERIFY(top->selectedText().isEmpty());
}

void tst_qquicktextedit::longPressInputMethod() // QTBUG-115004
{
    QQuickView window;
    window.setMinimumWidth(200);
    window.setMinimumHeight(100);
    QVERIFY(QQuickTest::showView(window, testFileUrl("positionAt.qml")));
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(edit);

    // Realistically there are touch events. But QQuickTextEdit doesn't handle them yet;
    // so we only test the synth-mouse events for now.
    QPoint pos = edit->positionToRectangle(20).center().toPoint(); // in the word "pi|ece"
    QTest::mousePress(&window, Qt::LeftButton, {}, pos);

    // Simulate input method events as seen on Android during long-press
    {
        QInputMethodEvent imEvent({}, QList<QInputMethodEvent::Attribute>()
                                          << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 20, 0, {}));
        QCoreApplication::sendEvent(edit, &imEvent);
    }
    {
        QInputMethodEvent imEvent({}, QList<QInputMethodEvent::Attribute>()
                                          << QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 0, 0, {})
                                          << QInputMethodEvent::Attribute(QInputMethodEvent::Selection, 13, -5, {}));
        QCoreApplication::sendEvent(edit, &imEvent);
    }

    // Release later => long press
    QTest::mouseRelease(&window, Qt::LeftButton, {}, pos, 1500);

    QTRY_COMPARE(edit->selectedText(), "piece");
}

void tst_qquicktextedit::rtlAlignmentInColumnLayout_QTBUG_112858()
{
    QQuickView window(testFileUrl("qtbug-112858.qml"));
    QVERIFY(window.rootObject() != nullptr);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickTextEdit *edit = window.rootObject()->findChild<QQuickTextEdit *>();
    QVERIFY(edit != nullptr);

    const auto text = edit->text();
    const auto lines = text.split("\n");
    QCOMPARE(lines.size(), edit->lineCount());

    int currentLineStartPos = 0;
    QRectF firstLineStartPosRect;

    // check that all lines are aligned, for RTL text it means that they have the same pos at the right
    for (int i = 0; i < lines.size(); ++i) {
        const auto lineStartPosRect = edit->positionToRectangle(currentLineStartPos);
        QVERIFY(lineStartPosRect.isValid());

        if (i == 0)
            firstLineStartPosRect = lineStartPosRect;
        else
            QCOMPARE(lineStartPosRect.right(), firstLineStartPosRect.right());

        currentLineStartPos += lines.at(i).size() + 1;
    }
}

void tst_qquicktextedit::resizeTextEditPolish()
{
    QQuickView window(testFileUrl("resizeTextEditPolish.qml"));
    QVERIFY(window.rootObject() != nullptr);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *edit = window.rootObject()->findChild<QQuickTextEdit *>();
    QVERIFY(edit != nullptr);
    QCOMPARE(edit->lineCount(), 1);

    QSignalSpy spy(edit, SIGNAL(lineCountChanged()));

    // Resize item and check for item polished
    auto *item = edit->parentItem();
    item->setWidth(item->width() - (item->width() / 2));

    QVERIFY(QQuickTest::qIsPolishScheduled(edit));
    QVERIFY(QQuickTest::qWaitForPolish(edit));

    QTRY_COMPARE(spy.size(), 1);
    QVERIFY(edit->lineCount() > 1);
    QCOMPARE(edit->state(), QString("multi-line"));
    auto *editPriv = QQuickTextEditPrivate::get(edit);
    QCOMPARE(editPriv->xoff, 0);
    QCOMPARE(editPriv->yoff, 0);
}

QTEST_MAIN(tst_qquicktextedit)

#include "tst_qquicktextedit.moc"
