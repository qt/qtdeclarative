// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qprocess.h>
#include <QtCore/qloggingcategory.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

Q_LOGGING_CATEGORY(lcQml, "qt.qml.tests");

class tst_qml : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qml() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void nonWindow();
    void itemAndWindowGeometry_data();
    void itemAndWindowGeometry();

private:
    QString qmlPath;
};

void tst_qml::initTestCase()
{
    QQmlDataTest::initTestCase();
    qmlPath = QLibraryInfo::path(QLibraryInfo::BinariesPath);

#if defined(Q_OS_WIN)
    qmlPath += QLatin1String("/qml.exe");
#else
    qmlPath += QLatin1String("/qml");
#endif

    QVERIFY(QFileInfo(qmlPath).exists());
}

void tst_qml::nonWindow()
{
    QProcess qml;
    qml.start(qmlPath, { testFile("nonWindow.qml") });
    QVERIFY(qml.waitForFinished());
    QCOMPARE(qml.exitCode(), 0); // Should not exit with code 2
}

void tst_qml::itemAndWindowGeometry_data()
{
    QTest::addColumn<QString>("config");
    QTest::addColumn<QString>("geometry");
    QTest::addColumn<QString>("qmlfile");
    QTest::addColumn<QSize>("expectedWindowSize");
    QTest::addColumn<QSize>("expectedContentSize");

    const QString none; // empty string

    auto sizeOrInvalid = [](int w, int h) {
        static const bool wm = QGuiApplicationPrivate::platformIntegration()->
                hasCapability(QPlatformIntegration::WindowManagement);
        return wm ? QSize(w, h) : QSize();
    };

    QTest::newRow("default: unsized")
            << none << none << "unsizedItem.qml"
            << QSize() << QSize(); // default size depends on window system
    QTest::newRow("default: unsized with geometry")
            << none << "100x100+50+50" << "unsizedItem.qml"
            << sizeOrInvalid(100, 100) << sizeOrInvalid(100, 100);
    QTest::newRow("resizeToItem: unsized")
            << "resizeToItem" << none << "unsizedItem.qml"
            << QSize() << QSize(0, 0);
    QTest::newRow("resizeToItem: unsized with geometry")
            << "resizeToItem" << "100x100+50+50" << "unsizedItem.qml"
            << sizeOrInvalid(100, 100) << QSize(0, 0);

    QTest::newRow("default: sized")
            << none << none << "sizedItem.qml"
            << QSize() << QSize();
    QTest::newRow("default: sized with geometry")
            << none << "100x100+50+50" << "sizedItem.qml"
            << sizeOrInvalid(100, 100) << sizeOrInvalid(100, 100);
    QTest::newRow("resizeToItem: sized")
            << "resizeToItem" << none << "sizedItem.qml"
            << sizeOrInvalid(200, 150) << sizeOrInvalid(200, 150);
    QTest::newRow("resizeToItem: sized with geometry")
            << "resizeToItem" << "320x240+50+50" << "sizedItem.qml"
            << sizeOrInvalid(320, 240) << QSize(200, 150);

    QTest::newRow("default: resizing")
            << none << none << "resizeItem.qml"
            << QSize() << QSize();
    QTest::newRow("default: resizing with geometry")
            << none << "100x100+50+50" << "resizeItem.qml"
            << sizeOrInvalid(100, 100) << sizeOrInvalid(100, 100);
    QTest::newRow("resizeToItem: resizing")
            << "resizeToItem" << none << "resizeItem.qml"
            << sizeOrInvalid(100, 50) << sizeOrInvalid(100, 50);
    QTest::newRow("resizeToItem: resizing with geometry")
            << "resizeToItem" << "320x240+50+50" << "resizeItem.qml"
            << sizeOrInvalid(100, 50) << sizeOrInvalid(100, 50);
}

/*
    - A root Item will get put into a Window depending on config (implementations in
      tools/qml/ResizeItemToWindow.qml and ResizeWindowToItem.qml).
    - The window system will enforce a minimum size.
    - In the default configuration, the root Item should then get resized to fit
      (QTBUG-114068 / QTBUG-116753).
    - In resizeToItem configuration, if the item width/height are not set, the window would
      try to be 0x0, but the window system won't allow it.
    - This also tests the `--qwindowgeometry` argument: with the default config, the
      item should be resized to fit, but not with `-c resizeToItem`.
*/
void tst_qml::itemAndWindowGeometry()
{
#ifdef Q_OS_WIN
    QSKIP("console.info does not go to stderr on Windows.");
#endif

    QFETCH(QString, config);
    QFETCH(QString, geometry);
    QFETCH(QString, qmlfile);
    QFETCH(QSize, expectedWindowSize);
    QFETCH(QSize, expectedContentSize);

    QStringList args;
    if (!config.isEmpty())
        args << "-c" << config;
    if (!geometry.isEmpty())
        args << "--qwindowgeometry" << geometry;
    args << testFile(qmlfile);
    QProcess qml;
    qml.start(qmlPath, args);
    QVERIFY(qml.waitForFinished());
    QCOMPARE(qml.exitStatus(), QProcess::NormalExit);
    const QByteArray output = qml.readAllStandardError();
    const auto sizeLineIndex = output.lastIndexOf("window");
    QCOMPARE_GE(sizeLineIndex, 0);
    const auto newlineIndex = output.indexOf('\n', sizeLineIndex);
    QCOMPARE_GT(newlineIndex, sizeLineIndex);
    // expect a line like "window 120 120 content 120 120"
    const auto sizes = output.sliced(sizeLineIndex, newlineIndex - sizeLineIndex).split(' ');
    QCOMPARE_GE(sizes.size(), 6);
    QCOMPARE(sizes[0], "window");
    QCOMPARE(sizes[3], "content");
    const QSize windowSize(sizes[1].toInt(), sizes[2].toInt());
    const QSize contentSize(sizes[4].toInt(), sizes[5].toInt());
    qCDebug(lcQml) << sizes
                   << "window" << windowSize << "expect" << expectedWindowSize
                   << "content" << contentSize << "expect" << expectedContentSize;
    QVERIFY(!windowSize.isEmpty());
    if (config != "resizeToItem") {
        // default config:
        // ResizeItemToWindow.qml should have resized the item to its window
        QCOMPARE(contentSize, windowSize);
    }
    // windowSize can be off-by-one on hidpi (e.g. QT_SCALE_FACTOR=2 on xcb);
    // perhaps that's a bug somewhere, but so far we aren't testing hidpi on CI
    if (expectedWindowSize.isValid())
        QCOMPARE(windowSize, expectedWindowSize);
    if (expectedContentSize.isValid())
        QCOMPARE(contentSize, expectedContentSize);
}

QTEST_MAIN(tst_qml)

#include <tst_qml.moc>
