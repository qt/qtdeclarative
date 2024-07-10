// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtTest/QtTest>
#include <QtQuick/QQuickView>
#include <QtQuick/QQuickTextDocument>
#include <QtQuick/QQuickItem>
#include <QtQuick/private/qquicktextedit_p.h>
#include <QtQuick/private/qquicktextdocument_p.h>
#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>
#include <QtGui/QTextDocumentWriter>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuickTest/QtQuickTest>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquicktextdocument : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquicktextdocument();

private slots:
    void textDocumentWriter();
    void textDocumentWithImage();
    void changeCharFormatInRange_data();
    void changeCharFormatInRange();
};

QString text = QStringLiteral("foo bar");

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

void tst_qquicktextdocument::textDocumentWithImage()
{
    QQuickTextDocumentWithImageResources document(nullptr);
    QImage image(1, 1, QImage::Format_Mono);
    image.fill(1);

    QString name = "image";
    document.addResource(QTextDocument::ImageResource, name, image);
    QTextImageFormat format;
    format.setName(name);
    QCOMPARE(image, document.image(format));
    QCOMPARE(image, document.resource(QTextDocument::ImageResource, name).value<QImage>());
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
    QQuickView window(testFileUrl("text.qml"));
    window.showNormal();
    QQuickTextEdit *textEdit = qobject_cast<QQuickTextEdit *>(window.rootObject());
    QVERIFY(textEdit);
    QVERIFY(textEdit->textDocument());

    auto *doc = textEdit->textDocument()->textDocument();
    QVERIFY(doc);

    QSignalSpy contentSpy(doc, &QTextDocument::contentsChanged);
    const auto data = QStringLiteral("Format String");
    doc->setPlainText(data);
    const auto block = doc->findBlockByNumber(0);

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

QTEST_MAIN(tst_qquicktextdocument)

#include "tst_qquicktextdocument.moc"
