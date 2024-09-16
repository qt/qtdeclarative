// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtTest/QtTest>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickTextDocument>
#include <QtQuick/QQuickView>
#include <QtQuick/private/qquicktextdocument_p.h>
#include <QtQuick/private/qquicktextedit_p.h>
#include <QtQuick/private/qquicktextedit_p_p.h>
#include <QtCore/QStringConverter>
#include <QtGui/QTextBlock>
#include <QtGui/QTextDocument>
#include <QtGui/QTextDocumentWriter>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlFile>
#include <QtQuickTest/QtQuickTest>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

using namespace Qt::StringLiterals;

class tst_qquicktextdocument : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquicktextdocument();

private:
    QPair<int, int> fragmentsAndItalics(const QTextDocument *doc);
    bool isMainFontFixed();

private slots:
    void textDocumentWriter();
    void customDocument();
    void replaceDocument();
    void sourceAndSave_data();
    void sourceAndSave();
    void loadErrorNoSuchFile();
    void loadErrorPermissionDenied();
    void overrideTextFormat_data();
    void overrideTextFormat();
    void changeCharFormatInRange_data();
    void changeCharFormatInRange();
    void independentDocumentsSameSource_data();
    void independentDocumentsSameSource();
};

QString text = QStringLiteral("foo bar");

// similar to TestDocument in tst_qtextdocument.cpp
class FakeImageDocument : public QTextDocument
{
public:
    inline FakeImageDocument(const QUrl &testUrl, const QString &testString)
        : url(testUrl), string(testString), resourceLoaded(false) {}

    bool hasResourceCached();

protected:
    virtual QVariant loadResource(int type, const QUrl &name) override;

private:
    QUrl url;
    QString string;
    bool resourceLoaded;
};

bool FakeImageDocument::hasResourceCached()
{
    return resourceLoaded;
}

QVariant FakeImageDocument::loadResource(int type, const QUrl &name)
{
    qCDebug(lcTests) << type << name << ": expecting" << int(QTextDocument::ImageResource) << url;
    if (type == QTextDocument::ImageResource && name == url) {
        resourceLoaded = true;
        return string;
    }
    return QTextDocument::loadResource(type, name);
}

tst_qquicktextdocument::tst_qquicktextdocument()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

/*! \internal
    Returns {fragmentCount, italicFragmentIndex}. If no italic fragment is found,
    italicFragmentIndex is -1.
*/
QPair<int, int> tst_qquicktextdocument::fragmentsAndItalics(const QTextDocument *doc)
{
    int fragmentCount = 0;
    int italicFragment = -1;
    for (QTextBlock::iterator it = doc->firstBlock().begin(); !(it.atEnd()); ++it) {
        QTextFragment currentFragment = it.fragment();
        if (currentFragment.charFormat().fontItalic())
            italicFragment = fragmentCount;
        ++fragmentCount;
        qCDebug(lcTests) << (currentFragment.charFormat().fontItalic() ? "italic" : "roman") << currentFragment.text();
    }
    return {fragmentCount, italicFragment};
}

bool tst_qquicktextdocument::isMainFontFixed()
{
    bool ret = QFontInfo(QGuiApplication::font()).fixedPitch();
    if (ret) {
        qCWarning(lcTests) << "QFontDatabase::GeneralFont is monospaced: markdown writing is likely to use too many backticks"
                           << QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    }
    return ret;
}

void tst_qquicktextdocument::textDocumentWriter()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("text.qml"));
    QObject* o = c.create();
    QVERIFY(o);
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit*>(o);
    QVERIFY(edit);

    QQuickTextDocument* quickDocument = qobject_cast<QQuickTextDocument*>(edit->property("textDocument").value<QObject*>());
    QVERIFY(quickDocument->textDocument() != nullptr);

    QBuffer output;
    output.open(QBuffer::ReadWrite);
    QVERIFY(output.buffer().isEmpty());

    edit->setProperty("text", QVariant(text));
    QTextDocumentWriter writer(&output, "plaintext");
    QVERIFY(writer.write(quickDocument->textDocument()));
    QCOMPARE(output.buffer(), text.toLatin1());
    delete o;
}

/*! \internal
    Verify that it's OK to replace the default QTextDocument that TextEdit creates
    with a user-created QTextDocument.

    Also verify that the user can still override QTextDocument::loadResource().
    QTextDocument::loadResource() can call its QObject parent's loadResource();
    the default QTextDocument's parent is the QQuickTextEdit, which provides an
    implementation of QQuickTextEdit::loadResource(), which uses QQuickPixmap
    to load and cache images. This will be bypassed if the user overrides
    loadResource() to do something different.
*/
void tst_qquicktextdocument::customDocument()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("text.qml"));
    QScopedPointer<QQuickTextEdit> textEdit(qobject_cast<QQuickTextEdit*>(c.create()));
    QCOMPARE(textEdit.isNull(), false);
    auto *textEditPriv = QQuickTextEditPrivate::get(textEdit.get());
    QVERIFY(textEditPriv->ownsDocument);

    QQuickTextDocument *quickDocument = textEdit->property("textDocument").value<QQuickTextDocument*>();
    QVERIFY(quickDocument);
    QPointer<QTextDocument> defaultDocument(quickDocument->textDocument());
    const QString imageUrl = "https://www.qt.io/hubfs/Qt-logo-neon-small.png";
    const QString fakeImageData = "foo!";

    FakeImageDocument fdoc(QUrl(imageUrl), fakeImageData);
    quickDocument->setTextDocument(&fdoc);
    QVERIFY(defaultDocument.isNull()); // deleted because of being replaced (don't leak)
    QCOMPARE(textEditPriv->ownsDocument, false);

    // QQuickTextEdit::setText() -> QQuickTextControl::setHtml() ->
    //   QQuickTextControlPrivate::setContent() -> fdoc->setHtml()
    //   and eventually fdoc->loadResource() which substitutes a string for the requested image
    textEdit->setTextFormat(QQuickTextEdit::RichText);
    textEdit->setText("an image: <img src='" + imageUrl + "'/>");
    QCOMPARE(fdoc.hasResourceCached(), true);

    auto firstBlock = fdoc.firstBlock();
    // check that image loading has been bypassed by FakeImageDocument
    bool foundImage = false;
    int fragmentCount = 0;
    for (QTextBlock::iterator it = firstBlock.begin(); !(it.atEnd()); ++it) {
        QTextFragment currentFragment = it.fragment();
        QVERIFY(currentFragment.isValid());
        ++fragmentCount;
        const QString imageName = currentFragment.charFormat().stringProperty(QTextFormat::ImageName);
        if (!imageName.isEmpty()) {
            QCOMPARE(imageName, imageUrl);
            foundImage = true;
            QCOMPARE(fdoc.resource(QTextDocument::ImageResource, imageUrl).toString(), fakeImageData);
        }
    }
    QVERIFY(foundImage);
    QCOMPARE(fragmentCount, 2);
}

/*! \internal
    Verify that it's OK to replace the default QTextDocument that TextEdit creates
    with a user-created QTextDocument that has different text in it, and that
    interactive editing continues to function afterwards, independently of
    the previous document.
*/
void tst_qquicktextdocument::replaceDocument() // QTBUG-126267
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("initialText.qml")));
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textEdit);
    auto *textEditPriv = QQuickTextEditPrivate::get(textEdit);
    QVERIFY(textEditPriv->ownsDocument);
    QQuickTextDocument *quickDocument = textEdit->property("textDocument").value<QQuickTextDocument*>();
    QVERIFY(quickDocument);
    QPointer<QTextDocument> defaultDocument(quickDocument->textDocument());

    QTextDocument replacementDoc;
    const QString replacementText("Hello World");
    {
        QTextCursor cursor(&replacementDoc);
        cursor.insertText(replacementText);
    }
    QCOMPARE(textEdit->text(), "Hello Qt");
    QSignalSpy renderSpy(&window, &QQuickWindow::afterRendering);
    quickDocument->setTextDocument(&replacementDoc);
    QVERIFY(defaultDocument.isNull()); // deleted because of being replaced (don't leak)
    QCOMPARE(textEditPriv->ownsDocument, false);
    QCOMPARE(textEdit->text(), replacementText);
    QCOMPARE(replacementDoc.toPlainText(), replacementText);
    QTRY_COMPARE_GT(renderSpy.size(), 0);

    QCOMPARE(window.activeFocusItem(), textEdit);
    QTest::keyEvent(QTest::KeyAction::Click, &window, Qt::Key_End);
    QTest::keyEvent(QTest::KeyAction::Click, &window, '!');
    QCOMPARE(textEdit->text(), replacementText + '!');
}

void tst_qquicktextdocument::sourceAndSave_data()
{
    QTest::addColumn<QQuickTextEdit::TextFormat>("textFormat");
    QTest::addColumn<QString>("source");
    QTest::addColumn<std::optional<QStringConverter::Encoding>>("expectedEncoding");
    QTest::addColumn<Qt::TextFormat>("expectedTextFormat");
    QTest::addColumn<int>("minCharCount");
    QTest::addColumn<QString>("expectedPlainText");

    const std::optional<QStringConverter::Encoding> nullEnc;

    QTest::newRow("plain") << QQuickTextEdit::PlainText << "hello.txt"
        << nullEnc << Qt::PlainText << 15 << u"Γειά σου Κόσμε!"_s;
    QTest::newRow("markdown") << QQuickTextEdit::MarkdownText << "hello.md"
        << nullEnc << Qt::MarkdownText << 15 << u"Γειά σου Κόσμε!"_s;
    QTest::newRow("html") << QQuickTextEdit::RichText << "hello.html"
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8)
        << Qt::RichText << 15 << u"Γειά σου Κόσμε!"_s;
    QTest::newRow("html-utf16be") << QQuickTextEdit::AutoText << "hello-utf16be.html"
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf16BE)
        << Qt::RichText << 15 << u"Γειά σου Κόσμε!"_s;
}

void tst_qquicktextdocument::sourceAndSave()
{
    QFETCH(QQuickTextEdit::TextFormat, textFormat);
    QFETCH(QString, source);
    QFETCH(std::optional<QStringConverter::Encoding>, expectedEncoding);
    QFETCH(Qt::TextFormat, expectedTextFormat);
    QFETCH(int, minCharCount);
    QFETCH(QString, expectedPlainText);

    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("text.qml"));
    QScopedPointer<QQuickTextEdit> textEdit(qobject_cast<QQuickTextEdit*>(c.create()));
    QCOMPARE(textEdit.isNull(), false);
    QQuickTextDocument *qqdoc = textEdit->property("textDocument").value<QQuickTextDocument*>();
    QVERIFY(qqdoc);
    const QQmlContext *ctxt = e.rootContext();
    // text.qml has text: "" but that's not a real change; QQuickTextEdit::setText() returns early
    // QQuickTextEditPrivate::init() also modifies defaults and then resets the modified state
    QCOMPARE(qqdoc->isModified(), false);
    // no stray signals should be visible to QML during init
    QCOMPARE(textEdit->property("modifiedChangeCount").toInt(), 0);
    QCOMPARE(textEdit->property("sourceChangeCount").toInt(), 0);
    QTextDocument *doc = qqdoc->textDocument();
    QVERIFY(doc);
    QSignalSpy sourceChangedSpy(qqdoc, &QQuickTextDocument::sourceChanged);
    QSignalSpy modifiedChangedSpy(qqdoc, &QQuickTextDocument::modifiedChanged);
    QSignalSpy statusChangedSpy(qqdoc, &QQuickTextDocument::statusChanged);

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QFile sf(QQmlFile::urlToLocalFileOrQrc(ctxt->resolvedUrl(testFileUrl(source))));
    qCDebug(lcTests) << source << "orig ->" << sf.fileName();
    QVERIFY(sf.exists());
    QString tmpPath = tmpDir.filePath(source);
    QVERIFY(sf.copy(tmpPath));
    qCDebug(lcTests) << source << "copy ->" << tmpDir.path() << ":" << tmpPath;

    QCOMPARE(statusChangedSpy.size(), 0);
    QCOMPARE(qqdoc->status(), QQuickTextDocument::Status::Null);
    textEdit->setTextFormat(textFormat);
    qqdoc->setProperty("source", QUrl::fromLocalFile(tmpPath));
    QCOMPARE(sourceChangedSpy.size(), 1);
    QCOMPARE(textEdit->property("sourceChangeCount").toInt(), 1);
    QCOMPARE(statusChangedSpy.size(), 2); // Loading, then Loaded
    QCOMPARE(qqdoc->status(), QQuickTextDocument::Status::Loaded);
    QVERIFY(qqdoc->errorString().isEmpty());
    const auto *qqdp = QQuickTextDocumentPrivate::get(qqdoc);
    QCOMPARE(qqdp->detectedFormat, expectedTextFormat);
    QCOMPARE_GE(doc->characterCount(), minCharCount);
    QCOMPARE(doc->toPlainText().trimmed(), expectedPlainText);
    QCOMPARE(qqdp->encoding, expectedEncoding);

    textEdit->setText("hello");
    QCOMPARE(qqdoc->isModified(), false);
    QCOMPARE_GE(textEdit->property("modifiedChangeCount").toInt(), 1);
    QCOMPARE(textEdit->property("modifiedChangeCount").toInt(), modifiedChangedSpy.size());
    modifiedChangedSpy.clear();
    textEdit->insert(5, "!");
    QCOMPARE_GE(modifiedChangedSpy.size(), 1);
    QCOMPARE(qqdoc->isModified(), true);

    qqdoc->save();
    QCOMPARE(statusChangedSpy.size(), 4); // Saving, then Saved
    QCOMPARE(qqdoc->status(), QQuickTextDocument::Status::Saved);
    QVERIFY(qqdoc->errorString().isEmpty());
    QFile tf(tmpPath);
    QVERIFY(tf.open(QIODeviceBase::ReadOnly));
    auto readBack = tf.readAll();
    if (expectedTextFormat == Qt::RichText) {
        QStringDecoder dec(*expectedEncoding);
        const QString decStr = dec(readBack);
        QVERIFY(decStr.contains("hello!</p>"));
    } else {
        QVERIFY(readBack.contains("hello!"));
    }
    QCOMPARE(textEdit->property("sourceChangeCount").toInt(), sourceChangedSpy.size());
}

void tst_qquicktextdocument::loadErrorNoSuchFile()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("text.qml"));
    QScopedPointer<QQuickTextEdit> textEdit(qobject_cast<QQuickTextEdit*>(c.create()));
    QCOMPARE(textEdit.isNull(), false);
    QQuickTextDocument *qqdoc = textEdit->property("textDocument").value<QQuickTextDocument*>();
    QVERIFY(qqdoc);
    QCOMPARE(textEdit->property("sourceChangeCount").toInt(), 0);
    QTextDocument *doc = qqdoc->textDocument();
    QVERIFY(doc);
    QSignalSpy sourceChangedSpy(qqdoc, &QQuickTextDocument::sourceChanged);
    QSignalSpy statusChangedSpy(qqdoc, &QQuickTextDocument::statusChanged);

    QCOMPARE(statusChangedSpy.size(), 0);
    QCOMPARE(qqdoc->status(), QQuickTextDocument::Status::Null);
    const QRegularExpression err(".*does not exist");
    QTest::ignoreMessage(QtWarningMsg, err);
    qqdoc->setProperty("source", testFileUrl("nosuchfile.md"));
    QCOMPARE(sourceChangedSpy.size(), 1);
    QCOMPARE(textEdit->property("sourceChangeCount").toInt(), 1);
    qCDebug(lcTests) << "status history" << textEdit->property("statusHistory").toList();
    QCOMPARE(statusChangedSpy.size(), 1);
    QCOMPARE(qqdoc->status(), QQuickTextDocument::Status::ReadError);
    QVERIFY(qqdoc->errorString().contains(err));
}

void tst_qquicktextdocument::loadErrorPermissionDenied()
{
#ifdef Q_OS_UNIX
    if (geteuid() == 0)
        QSKIP("Permission will not be denied with root privileges.");
#endif
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("text.qml"));
    QScopedPointer<QQuickTextEdit> textEdit(qobject_cast<QQuickTextEdit*>(c.create()));
    QCOMPARE(textEdit.isNull(), false);
    QQuickTextDocument *qqdoc = textEdit->property("textDocument").value<QQuickTextDocument*>();
    QVERIFY(qqdoc);
    const QQmlContext *ctxt = e.rootContext();
    QCOMPARE(textEdit->property("sourceChangeCount").toInt(), 0);
    QTextDocument *doc = qqdoc->textDocument();
    QVERIFY(doc);
    QSignalSpy sourceChangedSpy(qqdoc, &QQuickTextDocument::sourceChanged);
    QSignalSpy statusChangedSpy(qqdoc, &QQuickTextDocument::statusChanged);

    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const QString source("hello.md");
    QFile sf(QQmlFile::urlToLocalFileOrQrc(ctxt->resolvedUrl(testFileUrl(source))));
    qCDebug(lcTests) << source << "orig ->" << sf.fileName();
    QVERIFY(sf.exists());
    QString tmpPath = tmpDir.filePath(source);
    QVERIFY(sf.copy(tmpPath));
    qCDebug(lcTests) << source << "copy ->" << tmpDir.path() << ":" << tmpPath;
    if (!QFile::setPermissions(tmpPath, QFileDevice::Permissions{})) // no permissions at all
        QSKIP("Failed to change permissions of temporary file: cannot continue.");

    QCOMPARE(statusChangedSpy.size(), 0);
    QCOMPARE(qqdoc->status(), QQuickTextDocument::Status::Null);
    const QRegularExpression err(".*Failed to read: Permission denied");
    QTest::ignoreMessage(QtWarningMsg, err);
    qqdoc->setProperty("source", QUrl::fromLocalFile(tmpPath));
    QCOMPARE(sourceChangedSpy.size(), 1);
    QCOMPARE(textEdit->property("sourceChangeCount").toInt(), 1);
    qCDebug(lcTests) << "status history" << textEdit->property("statusHistory").toList();
    QCOMPARE(statusChangedSpy.size(), 1);
    QCOMPARE(qqdoc->status(), QQuickTextDocument::Status::ReadError);
    QVERIFY(qqdoc->errorString().contains(err));
}

void tst_qquicktextdocument::overrideTextFormat_data()
{
    QTest::addColumn<QUrl>("qmlfile");
    QTest::addColumn<QQuickTextEdit::TextFormat>("initialFormat");
    QTest::addColumn<QUrl>("source");
    QTest::addColumn<int>("expectedInitialFragmentCount");
    QTest::addColumn<int>("expectedInitialItalicFragment");
    // first part of TextEdit.text after loading
    QTest::addColumn<QString>("expectedTextPrefix");

    QTest::addColumn<QQuickTextEdit::TextFormat>("replacementFormat");
    QTest::addColumn<int>("expectedFragmentCount");
    QTest::addColumn<int>("expectedItalicFragment");
    // first part of TextEdit.text after switching to replacementFormat
    QTest::addColumn<QString>("expectedReplacementPrefix");
    QTest::addColumn<int>("expectedTextChangedSignalsAfterReplacement");

    QTest::addColumn<QQuickTextEdit::TextFormat>("finalFormat");
    QTest::addColumn<int>("expectedFinalFragmentCount");
    QTest::addColumn<int>("expectedFinalItalicFragment");
    // first part of TextEdit.text after switching to finalFormat
    QTest::addColumn<QString>("expectedFinalPrefix");

    QTest::newRow("load md, switch to plain, back to md")
            << testFileUrl("text.qml") << QQuickTextEdit::MarkdownText << testFileUrl("hello.md")
            << 3 << 1 << u"Γειά σου *Κόσμε*!"_s
            << QQuickTextEdit::PlainText << 1 << -1 << u"Γειά σου *Κόσμε*!"_s << 2
            << QQuickTextEdit::MarkdownText << 3 << 1 << u"Γειά σου *Κόσμε*!"_s;
    QTest::newRow("load md, switch to plain, then auto")
            << testFileUrl("text.qml") << QQuickTextEdit::MarkdownText << testFileUrl("hello.md")
            << 3 << 1 << u"Γειά σου *Κόσμε*!"_s
            << QQuickTextEdit::PlainText << 1 << -1 << u"Γειά σου *Κόσμε*!"_s << 2
            << QQuickTextEdit::AutoText << 3 << 1 << u"Γειά σου *Κόσμε*!"_s;
    QTest::newRow("load md, switch to html, then plain")
            << testFileUrl("text.qml") << QQuickTextEdit::MarkdownText << testFileUrl("hello.md")
            << 3 << 1 << u"Γειά σου *Κόσμε*!"_s
            << QQuickTextEdit::RichText << 3 << 1 << u"<!DOCTYPE HTML"_s << 2
            << QQuickTextEdit::PlainText << 1 << -1 << u"<!DOCTYPE HTML"_s;
    QTest::newRow("load md as plain text, switch to md, back to plain")
            << testFileUrl("text.qml") << QQuickTextEdit::PlainText << testFileUrl("hello.md")
            << 1 << -1 << u"Γειά σου *Κόσμε*!"_s
            << QQuickTextEdit::MarkdownText << 3 << 1 << u"Γειά σου *Κόσμε*!"_s << 2
            << QQuickTextEdit::PlainText << 1 << -1 << u"Γειά σου *Κόσμε*!"_s;
    QTest::newRow("load md as autotext, switch to plain, back to auto")
            << testFileUrl("text.qml") << QQuickTextEdit::AutoText << testFileUrl("hello.md")
            << 3 << 1 << u"Γειά σου *Κόσμε*!"_s
            << QQuickTextEdit::PlainText << 1 << -1 << u"Γειά σου *Κόσμε*!"_s << 2
            << QQuickTextEdit::AutoText << 3 << 1 << u"Γειά σου *Κόσμε*!"_s;
    QTest::newRow("load md as autotext, switch to md, then plain")
            << testFileUrl("text.qml") << QQuickTextEdit::AutoText << testFileUrl("hello.md")
            << 3 << 1 << u"Γειά σου *Κόσμε*!"_s
            << QQuickTextEdit::MarkdownText << 3 << 1 << u"Γειά σου *Κόσμε*!"_s
            // going from AutoText to a matching explicit format does not cause extra textChanged()
            << 1
            << QQuickTextEdit::PlainText << 1 << -1 << u"Γειά σου *Κόσμε*!"_s;
    QTest::newRow("load md as autotext, switch to html, then plain")
            << testFileUrl("text.qml") << QQuickTextEdit::AutoText << testFileUrl("hello.md")
            << 3 << 1 << u"Γειά σου *Κόσμε*!"_s
            << QQuickTextEdit::RichText << 3 << 1 << u"<!DOCTYPE HTML"_s << 2
            << QQuickTextEdit::PlainText << 1 << -1 << u"<!DOCTYPE HTML"_s;

    QTest::newRow("load html, switch to plain, back to rich")
            << testFileUrl("text.qml") << QQuickTextEdit::RichText << testFileUrl("hello.html")
            << 3 << 1 << u"<!DOCTYPE HTML"_s
            << QQuickTextEdit::PlainText << 1 << -1 << u"<!DOCTYPE HTML"_s << 2
            << QQuickTextEdit::RichText << 3 << 1 << u"<!DOCTYPE HTML"_s;
    QTest::newRow("load html as plain text, switch to html, back to plain")
            << testFileUrl("text.qml") << QQuickTextEdit::PlainText << testFileUrl("hello.html")
            << 1 << -1 << u"Γειά σου <i>Κόσμε</i>!"_s
            << QQuickTextEdit::RichText << 3 << 1 << u"<!DOCTYPE HTML"_s << 2
            << QQuickTextEdit::PlainText << 1 << -1 << u"<!DOCTYPE HTML"_s;
    QTest::newRow("load html as autotext, switch to plain, back to auto")
            << testFileUrl("text.qml") << QQuickTextEdit::AutoText << testFileUrl("hello.html")
            << 3 << 1 << u"<!DOCTYPE HTML"_s
            << QQuickTextEdit::PlainText << 1 << -1 << u"<!DOCTYPE HTML"_s << 2
            << QQuickTextEdit::AutoText << 3 << 1 << u"<!DOCTYPE HTML"_s;
    QTest::newRow("load html as autotext, switch to html, then plain")
            << testFileUrl("text.qml") << QQuickTextEdit::AutoText << testFileUrl("hello.html")
            << 3 << 1 << u"<!DOCTYPE HTML"_s
            << QQuickTextEdit::RichText << 3 << 1 << u"<!DOCTYPE HTML"_s
            // going from AutoText to a matching explicit format does not cause extra textChanged()
            << 1
            << QQuickTextEdit::PlainText << 1 << -1 << u"<!DOCTYPE HTML"_s;
    QTest::newRow("load html as autotext, switch to markdown, then plain")
            << testFileUrl("text.qml") << QQuickTextEdit::AutoText << testFileUrl("hello.html")
            << 3 << 1 << u"<!DOCTYPE HTML"_s
            << QQuickTextEdit::MarkdownText << 3 << 1 << u"Γειά σου *Κόσμε*!"_s << 2
            << QQuickTextEdit::PlainText << 1 << -1 << u"Γειά σου *Κόσμε*!"_s;
}

void tst_qquicktextdocument::overrideTextFormat() // QTBUG-120772
{
    if (isMainFontFixed())
        QSKIP("fixed-pitch main font (QTBUG-103484)");

    QFETCH(QUrl, qmlfile);
    QFETCH(QQuickTextEdit::TextFormat, initialFormat);
    QFETCH(QUrl, source);
    QFETCH(int, expectedInitialFragmentCount);
    QFETCH(int, expectedInitialItalicFragment);
    QFETCH(QString, expectedTextPrefix);

    QFETCH(QQuickTextEdit::TextFormat, replacementFormat);
    QFETCH(int, expectedFragmentCount);
    QFETCH(int, expectedItalicFragment);
    QFETCH(QString, expectedReplacementPrefix);
    QFETCH(int, expectedTextChangedSignalsAfterReplacement);

    QFETCH(QQuickTextEdit::TextFormat, finalFormat);
    QFETCH(int, expectedFinalFragmentCount);
    QFETCH(int, expectedFinalItalicFragment);
    QFETCH(QString, expectedFinalPrefix);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, qmlfile));
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textEdit);
    QQuickTextDocument *qqdoc = textEdit->property("textDocument").value<QQuickTextDocument*>();
    QVERIFY(qqdoc);
    QTextDocument *doc = qqdoc->textDocument();
    QVERIFY(doc);

    textEdit->setTextFormat(initialFormat);
    QCOMPARE(qqdoc->isModified(), false);
    QCOMPARE(textEdit->property("sourceChangeCount").toInt(), 0);
    QSignalSpy sourceChangedSpy(qqdoc, &QQuickTextDocument::sourceChanged);
    QSignalSpy textChangedSpy(textEdit, &QQuickTextEdit::textChanged);

    qqdoc->setProperty("source", source);
    QCOMPARE(sourceChangedSpy.size(), 1);
    QCOMPARE(textEdit->property("sourceChangeCount").toInt(), 1);
    QCOMPARE_GE(textChangedSpy.size(), 1);
    auto fragCountAndItalic = fragmentsAndItalics(doc);
    QCOMPARE(fragCountAndItalic.first, expectedInitialFragmentCount);
    QCOMPARE(fragCountAndItalic.second, expectedInitialItalicFragment);
    QString textPropValue = textEdit->text();
    qCDebug(lcTests) << "expect text()" << textPropValue.first(qMin(20, textPropValue.size() - 1))
                     << "to start with" << expectedTextPrefix;
    QVERIFY(textPropValue.startsWith(expectedTextPrefix));

    textEdit->setTextFormat(replacementFormat);
    QCOMPARE(qqdoc->isModified(), false);
    QCOMPARE(sourceChangedSpy.size(), 1);
    QCOMPARE_GE(textChangedSpy.size(), expectedTextChangedSignalsAfterReplacement);
    fragCountAndItalic = fragmentsAndItalics(doc);
    QCOMPARE(fragCountAndItalic.first, expectedFragmentCount);
    QCOMPARE(fragCountAndItalic.second, expectedItalicFragment);
    textPropValue = textEdit->text();
    qCDebug(lcTests) << "expect text()" << textPropValue.first(qMin(20, textPropValue.size() - 1))
                     << "to start with" << expectedReplacementPrefix;
    QVERIFY(textPropValue.startsWith(expectedReplacementPrefix));

    textEdit->setTextFormat(finalFormat);
    QCOMPARE(qqdoc->isModified(), false);
    QCOMPARE(sourceChangedSpy.size(), 1);
    QCOMPARE_GE(textChangedSpy.size(), expectedTextChangedSignalsAfterReplacement + 1);
    fragCountAndItalic = fragmentsAndItalics(doc);
    QCOMPARE(fragCountAndItalic.first, expectedFinalFragmentCount);
    QCOMPARE(fragCountAndItalic.second, expectedFinalItalicFragment);
    textPropValue = textEdit->text();
    qCDebug(lcTests) << "expect text()" << textPropValue.first(qMin(20, textPropValue.size() - 1))
                     << "to start with" << expectedFinalPrefix;
    QVERIFY(textPropValue.startsWith(expectedFinalPrefix));
}

void tst_qquicktextdocument::changeCharFormatInRange_data()
{
    QTest::addColumn<bool>("editBlock");

    QTest::newRow("begin/end") << true;
    QTest::newRow("no edit block") << false; // QTBUG-126886 : don't crash
}

void tst_qquicktextdocument::changeCharFormatInRange()
{
    QFETCH(bool, editBlock);
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("initialText.qml")));
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textEdit);
    QVERIFY(textEdit->textDocument());
    auto *doc = textEdit->textDocument()->textDocument();
    QVERIFY(doc);

    QSignalSpy contentSpy(doc, &QTextDocument::contentsChanged);
    const auto data = QStringLiteral("Format String");
    doc->setPlainText(data);
    auto block = doc->findBlockByNumber(0);

    auto formatText = [block, data] {
         QTextLayout::FormatRange formatText;
         formatText.start = 0;
         formatText.length = data.size();
         formatText.format.setForeground(Qt::green);
         block.layout()->setFormats({formatText});
    };

    // change the char format of this block, and verify visual effect
    if (editBlock) {
        QTextCursor cursor(doc);
        cursor.beginEditBlock();
        formatText();
        cursor.endEditBlock();
    } else {
        formatText();
    }

    QVERIFY(QQuickTest::qWaitForPolish(textEdit));
    QCOMPARE(contentSpy.size(), editBlock ? 2 : 1);
}

void tst_qquicktextdocument::independentDocumentsSameSource_data()
{
    QTest::addColumn<QUrl>("qmlfile");

    QTest::newRow("textFormat above source") << testFileUrl("sideBySideIndependent.qml");
    QTest::newRow("source above textFormat") << testFileUrl("sideBySideIndependentReverse.qml");
}

// ensure that two TextEdits' textFormat properties take effect, regardless of qml init order
void tst_qquicktextdocument::independentDocumentsSameSource() // QTBUG-120772
{
    QFETCH(QUrl, qmlfile);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, qmlfile));
    QQuickTextEdit *textEditPlain = window.rootObject()->findChild<QQuickTextEdit *>("plain");
    QVERIFY(textEditPlain);
    QQuickTextEdit *textEditMarkdown = window.rootObject()->findChild<QQuickTextEdit *>("markdown");
    QVERIFY(textEditMarkdown);

    auto fragCountAndItalic = fragmentsAndItalics(textEditPlain->textDocument()->textDocument());
    QCOMPARE(fragCountAndItalic.first, 1);
    QCOMPARE(fragCountAndItalic.second, -1);

    fragCountAndItalic = fragmentsAndItalics(textEditMarkdown->textDocument()->textDocument());
    QCOMPARE(fragCountAndItalic.first, 3);
    QCOMPARE(fragCountAndItalic.second, 1);
}

QTEST_MAIN(tst_qquicktextdocument)

#include "tst_qquicktextdocument.moc"
