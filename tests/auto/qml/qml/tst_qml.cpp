// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qprocess.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qml : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qml() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void nonWindow();

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

QTEST_MAIN(tst_qml)

#include <tst_qml.moc>
