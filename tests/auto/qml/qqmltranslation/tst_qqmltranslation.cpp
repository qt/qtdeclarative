/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QTranslator>
#include "../../shared/util.h"

class tst_qqmltranslation : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmltranslation() {}

private slots:
    void translation();
    void idTranslation();
    void translationInQrc();
};

void tst_qqmltranslation::translation()
{
    QTranslator translator;
    translator.load(QLatin1String("qml_fr"), dataDirectory());
    QCoreApplication::installTranslator(&translator);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("translation.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("basic").toString(), QLatin1String("bonjour"));
    QCOMPARE(object->property("basic2").toString(), QLatin1String("au revoir"));
    QCOMPARE(object->property("basic3").toString(), QLatin1String("bonjour"));
    QCOMPARE(object->property("disambiguation").toString(), QLatin1String("salut"));
    QCOMPARE(object->property("disambiguation2").toString(), QString::fromUtf8("\xc3\xa0 plus tard"));
    QCOMPARE(object->property("disambiguation3").toString(), QLatin1String("salut"));
    QCOMPARE(object->property("noop").toString(), QLatin1String("bonjour"));
    QCOMPARE(object->property("noop2").toString(), QLatin1String("au revoir"));
    QCOMPARE(object->property("singular").toString(), QLatin1String("1 canard"));
    QCOMPARE(object->property("singular2").toString(), QLatin1String("1 canard"));
    QCOMPARE(object->property("plural").toString(), QLatin1String("2 canards"));
    QCOMPARE(object->property("plural2").toString(), QLatin1String("2 canards"));

    QCoreApplication::removeTranslator(&translator);
    delete object;
}

void tst_qqmltranslation::idTranslation()
{
    QTranslator translator;
    translator.load(QLatin1String("qmlid_fr"), dataDirectory());
    QCoreApplication::installTranslator(&translator);

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("idtranslation.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("idTranslation").toString(), QLatin1String("bonjour tout le monde"));
    QCOMPARE(object->property("idTranslation2").toString(), QLatin1String("bonjour tout le monde"));
    QCOMPARE(object->property("idTranslation3").toString(), QLatin1String("bonjour tout le monde"));

    QCoreApplication::removeTranslator(&translator);
    delete object;
}

void tst_qqmltranslation::translationInQrc()
{
    QTranslator translator;
    translator.load(":/qml_fr.qm");
    QCoreApplication::installTranslator(&translator);

    QQmlEngine engine;
    QQmlComponent component(&engine, QUrl("qrc:/translation.qml"));
    QObject *object = component.create();
    QVERIFY(object != 0);

    QCOMPARE(object->property("basic").toString(), QLatin1String("bonjour"));
    QCOMPARE(object->property("basic2").toString(), QLatin1String("au revoir"));
    QCOMPARE(object->property("basic3").toString(), QLatin1String("bonjour"));
    QCOMPARE(object->property("disambiguation").toString(), QLatin1String("salut"));
    QCOMPARE(object->property("disambiguation2").toString(), QString::fromUtf8("\xc3\xa0 plus tard"));
    QCOMPARE(object->property("disambiguation3").toString(), QLatin1String("salut"));
    QCOMPARE(object->property("noop").toString(), QLatin1String("bonjour"));
    QCOMPARE(object->property("noop2").toString(), QLatin1String("au revoir"));
    QCOMPARE(object->property("singular").toString(), QLatin1String("1 canard"));
    QCOMPARE(object->property("singular2").toString(), QLatin1String("1 canard"));
    QCOMPARE(object->property("plural").toString(), QLatin1String("2 canards"));
    QCOMPARE(object->property("plural2").toString(), QLatin1String("2 canards"));

    QCoreApplication::removeTranslator(&translator);
    delete object;
}

QTEST_MAIN(tst_qqmltranslation)

#include "tst_qqmltranslation.moc"
