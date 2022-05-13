// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <private/qquickimage_p.h>
#include <QQmlApplicationEngine>

class tst_sharedimage : public QObject
{
    Q_OBJECT
public:
    tst_sharedimage()
    {
    }

private slots:
    void initTestCase();
    void compareToPlainLoad_data();
    void compareToPlainLoad();
};

void tst_sharedimage::initTestCase()
{
#if !QT_CONFIG(systemsemaphore)
    QSKIP("Shared image not supported");
#endif

#ifdef Q_OS_ANDROID
    /*
     * On Android, images are usually in resources, not in
     * files on the file system.
     */
    QSKIP("Shared image not useful on Android, skipping test");
#endif
}

void tst_sharedimage::compareToPlainLoad_data()
{
    QString imagePath = QFINDTESTDATA("data/yellow.png");
    if (imagePath.startsWith(QLatin1Char('/')))
        imagePath.remove(0, 1);
    QString plainImage("Image { source: \"file:///%1\"; cache: false; %2 }");
    QString sharedImage("Image { source: \"image://shared/%1\"; cache: false; %2 }");
    QString script("import QtQuick 2.0\nimport Qt.labs.sharedimage 1.0\n%1\n");

    QString plain = script.arg(plainImage).arg(imagePath);
    QString shared = script.arg(sharedImage).arg(imagePath);

    QTest::addColumn<QByteArray>("plainScript");
    QTest::addColumn<QByteArray>("sharedScript");

    QString opts = QStringLiteral("asynchronous: false;");
    QTest::newRow("sync") << plain.arg(opts).toLatin1() << shared.arg(opts).toLatin1();

    opts = QStringLiteral("asynchronous: true");
    QTest::newRow("async") << plain.arg(opts).toLatin1() << shared.arg(opts).toLatin1();

    opts = QStringLiteral("sourceSize: Qt.size(50, 50)");
    QTest::newRow("scaled, stretch") << plain.arg(opts).toLatin1() << shared.arg(opts).toLatin1();

    opts = QStringLiteral("sourceSize: Qt.size(50, 50); fillMode: Image.PreserveAspectFit");
    QTest::newRow("scaled, aspectfit") << plain.arg(opts).toLatin1() << shared.arg(opts).toLatin1();
}

void tst_sharedimage::compareToPlainLoad()
{
    QFETCH(QByteArray, plainScript);
    QFETCH(QByteArray, sharedScript);

    QImage images[2];
    for (int i = 0; i < 2; i++) {
        QQmlApplicationEngine engine;
        engine.loadData(i ? sharedScript : plainScript);
        QVERIFY(engine.rootObjects().size());
        QQuickImage *obj = qobject_cast<QQuickImage*>(engine.rootObjects().at(0));
        QVERIFY(obj != nullptr);
        QTRY_VERIFY(!obj->image().isNull());
        images[i] = obj->image();
    }

    QCOMPARE(images[1], images[0].convertToFormat(images[1].format()));
}

QTEST_MAIN(tst_sharedimage)

#include "tst_sharedimage.moc"
