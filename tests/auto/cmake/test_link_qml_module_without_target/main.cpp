// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QDirIterator>
#include <QtGui/QGuiApplication>
#include <QtTest/QtTest>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQuick/QQuickView>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

// Manually import the qml plugins so that the qml modules and their resources
// are initialized.
Q_IMPORT_QML_PLUGIN(BasePlugin)
Q_IMPORT_QML_PLUGIN(DerivedPlugin)

class TestLinkStaticQmlModule : public QObject
{
    Q_OBJECT
private slots:
    void canRun();
};

void TestLinkStaticQmlModule::canRun()
{
    // Show all the non-Qt loaded resources.
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const auto path = it.next();
        if (!path.startsWith(u":/qt-project.org"_qs)) {
            qDebug() << path;
        }
    }

    QQuickView view;
    QVERIFY(QFile::exists(":/qt/qml/Base/qml/Red.qml"));
    QVERIFY(QFile::exists(":/qt/qml/Derived/qml/Blue.qml"));
    QVERIFY(QQuickTest::showView(view, QUrl(u"qrc:/qt/qml/Derived/qml/main.qml"_qs)));
}

QTEST_MAIN(TestLinkStaticQmlModule)
#include "main.moc"
