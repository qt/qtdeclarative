// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtTest/QtTest>
#include <QtQuick/QQuickTextDocument>
#include <QtQuick/QQuickItem>
#include <QtQuick/private/qquicktextedit_p.h>
#include <QtQuick/private/qquicktextdocument_p.h>
#include <QtGui/QTextDocument>
#include <QtGui/QTextDocumentWriter>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquicktextdocument : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquicktextdocument();

private slots:
    void textDocumentWriter();
    void textDocumentWithImage();
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

QTEST_MAIN(tst_qquicktextdocument)

#include "tst_qquicktextdocument.moc"
