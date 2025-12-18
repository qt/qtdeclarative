// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtTest/QSignalSpy>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/qquickitem.h>
#include <QtQuickShapes/private/qquickshape_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

class tst_QQuickVectorImage : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickVectorImage();

private slots:
    void parseFiles_data();
    void parseFiles();
    void parseBrokenFile();
    void asyncShapes_data();
    void asyncShapes();
};

tst_QQuickVectorImage::tst_QQuickVectorImage()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickVectorImage::parseFiles_data()
{
    QTest::addColumn<QString>("fileName");

    QDir dir(QStringLiteral(":/svgs"));
    const QFileInfoList infos = dir.entryInfoList({ QStringLiteral("*.svg") });

    QVERIFY(!infos.isEmpty());
    for (const QFileInfo &info : infos) {
        QString fileName = info.fileName();
        QByteArray rowName = fileName.toUtf8();

        QTest::newRow(rowName) << fileName;
    }
}

void tst_QQuickVectorImage::parseFiles()
{
    QFETCH(QString, fileName);

    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("fileName"), QStringLiteral("qrc:/svgs/%1").arg(fileName));

    QQmlComponent c(&engine, testFileUrl("vectorimage.qml"));
    QQuickItem *item = qobject_cast<QQuickItem *>(c.create());
    auto cleanup = qScopeGuard([&item] {
        delete item;
        item = nullptr;
    });

    QVERIFY(item != nullptr);
    QVERIFY(!item->childItems().isEmpty());
    QVERIFY(!item->childItems().first()->size().isNull());
}

void tst_QQuickVectorImage::parseBrokenFile()
{
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("fileName"), testFileUrl("svg/broken.svg"));

    QQmlComponent c(&engine, testFileUrl("vectorimage.qml"));
    QQuickItem *item = qobject_cast<QQuickItem *>(c.create());
    auto cleanup = qScopeGuard([&item] {
        delete item;
        item = nullptr;
    });

    QVERIFY(item != nullptr);
    QVERIFY(!item->childItems().isEmpty());
    QVERIFY(item->childItems().first()->size().isNull());
}

void tst_QQuickVectorImage::asyncShapes_data()
{
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<bool>("isAsync");

    QTest::newRow("sync") << "vectorimage.qml" << false;
    QTest::newRow("async") << "vectorimage-async.qml" << true;
}

void tst_QQuickVectorImage::asyncShapes()
{
    QFETCH(QString, testFile);
    QFETCH(bool, isAsync);

    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("fileName"), QStringLiteral("qrc:/svgs/gradientxform.svg"));

    QQmlComponent c(&engine, testFileUrl(testFile));
    QQuickItem *item = qobject_cast<QQuickItem *>(c.create());
    auto cleanup = qScopeGuard([&item] {
        delete item;
        item = nullptr;
    });

    QVERIFY(item != nullptr);
    QVERIFY(!item->childItems().isEmpty());
    QVERIFY(!item->childItems().first()->size().isNull());

    const QList<const QQuickShape *> shapes = item->findChildren<const QQuickShape *>();
    QVERIFY(!shapes.isEmpty());
    for (const QQuickShape *shape : shapes)
        QCOMPARE(shape->asynchronous(), isAsync);
}

QTEST_MAIN(tst_QQuickVectorImage)

#include "tst_qquickvectorimage.moc"
