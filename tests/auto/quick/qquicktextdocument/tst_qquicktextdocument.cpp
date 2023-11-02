// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtTest/QtTest>
#include <QtQuick/QQuickTextDocument>
#include <QtQuick/QQuickItem>
#include <QtQuick/private/qquicktextedit_p.h>
#include <QtQuick/private/qquicktextedit_p_p.h>
#include <QtGui/QTextBlock>
#include <QtGui/QTextDocument>
#include <QtGui/QTextDocumentWriter>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuickTestUtils/private/qmlutils_p.h>

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

class tst_qquicktextdocument : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquicktextdocument();

private slots:
    void textDocumentWriter();
    void customDocument();
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

QTEST_MAIN(tst_qquicktextdocument)

#include "tst_qquicktextdocument.moc"
