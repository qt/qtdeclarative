// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtTest/QtTest>
#include <QtQuick/QQuickTextDocument>
#include <QtQuick/QQuickItem>
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
#include <QtQuickTestUtils/private/qmlutils_p.h>

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

using namespace Qt::StringLiterals;

class tst_qquicktextdocument : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquicktextdocument();

private slots:
    void textDocumentWriter();
    void customDocument();
    void sourceAndSave_data();
    void sourceAndSave();
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

void tst_qquicktextdocument::sourceAndSave_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<std::optional<QStringConverter::Encoding>>("expectedEncoding");
    QTest::addColumn<QString>("expectedMimeType");
    QTest::addColumn<int>("minCharCount");
    QTest::addColumn<QString>("expectedPlainText");

    const std::optional<QStringConverter::Encoding> nullEnc;

    QTest::newRow("plain") << "hello.txt"
        << nullEnc << "text/plain" << 15 << u"Γειά σου Κόσμε!"_s;
    QTest::newRow("markdown") << "hello.md"
        << nullEnc << "text/markdown" << 15 << u"Γειά σου Κόσμε!"_s;
    QTest::newRow("html") << "hello.html"
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf8)
        << "text/html" << 15 << u"Γειά σου Κόσμε!"_s;
    QTest::newRow("html-utf16be") << "hello-utf16be.html"
        << std::optional<QStringConverter::Encoding>(QStringConverter::Utf16BE)
        << "text/html" << 15 << u"Γειά σου Κόσμε!"_s;
}

void tst_qquicktextdocument::sourceAndSave()
{
    QFETCH(QString, source);
    QFETCH(std::optional<QStringConverter::Encoding>, expectedEncoding);
    QFETCH(QString, expectedMimeType);
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
    qqdoc->setProperty("source", QUrl::fromLocalFile(tmpPath));
    QCOMPARE(sourceChangedSpy.size(), 1);
    QCOMPARE(textEdit->property("sourceChangeCount").toInt(), 1);
    QCOMPARE(statusChangedSpy.size(), 2); // Loading, then Loaded
    QCOMPARE(qqdoc->status(), QQuickTextDocument::Status::Loaded);
    const auto *qqdp = QQuickTextDocumentPrivate::get(qqdoc);
    QVERIFY(qqdp->mimeType.inherits(expectedMimeType));
    const bool expectHtml = (expectedMimeType == "text/html");
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
    QCOMPARE(statusChangedSpy.size(), 4); // Saving, then SaveDone
    QCOMPARE(qqdoc->status(), QQuickTextDocument::Status::SaveDone);
    QFile tf(tmpPath);
    QVERIFY(tf.open(QIODeviceBase::ReadOnly));
    auto readBack = tf.readAll();
    if (expectHtml) {
        QStringDecoder dec(*expectedEncoding);
        const QString decStr = dec(readBack);
        QVERIFY(decStr.contains("hello!</p>"));
    } else {
        QVERIFY(readBack.contains("hello!"));
    }
    QCOMPARE(textEdit->property("sourceChangeCount").toInt(), sourceChangedSpy.size());
}

QTEST_MAIN(tst_qquicktextdocument)

#include "tst_qquicktextdocument.moc"
