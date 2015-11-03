/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
#include "../../shared/util.h"

class tst_qquicktextdocument : public QQmlDataTest
{
    Q_OBJECT
private slots:
    void textDocumentWriter();
    void textDocumentWithImage();
};

QString text = QStringLiteral("foo bar");

void tst_qquicktextdocument::textDocumentWriter()
{
    QQmlEngine e;
    QQmlComponent c(&e, testFileUrl("text.qml"));
    QObject* o = c.create();
    QVERIFY(o);
    QQuickTextEdit *edit = qobject_cast<QQuickTextEdit*>(o);
    QVERIFY(edit);

    QQuickTextDocument* quickDocument = qobject_cast<QQuickTextDocument*>(edit->property("textDocument").value<QObject*>());
    QVERIFY(quickDocument->textDocument() != 0);

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
    QQuickTextDocumentWithImageResources document(0);
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
