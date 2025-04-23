// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/qtest.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qprocess.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QTemporaryFile>

class tst_qml : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qml() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void nonWindow();
    void extraPositionalArguments();

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

void tst_qml::extraPositionalArguments()
{
    QProcess qml;
    QStringList args;

    QTemporaryFile f;
    QVERIFY(f.open());
    QVERIFY(f.write(R"(import QtQml

QtObject {
    Component.onCompleted: {
        let s = ""
        for (let i = 2; i < Qt.application.arguments.length; ++i)
            s += Qt.application.arguments[i].substring(1)
        console.log(s)
        Qt.quit()
    }
}
)"));
    f.flush();

    args << f.fileName();
    args << "--";
    for (char c = 'a'; c <= 'z'; ++c)
        args << u'-' + QString(c);

    qml.start(qmlPath, args);
    QVERIFY(qml.waitForFinished());
    QVERIFY(qml.exitStatus() == QProcess::NormalExit && qml.exitCode() == 0);
    QVERIFY(qml.readAllStandardError().contains("abcdefghijklmnopqrstuvwxyz"));
}

QTEST_MAIN(tst_qml)

#include <tst_qml.moc>
