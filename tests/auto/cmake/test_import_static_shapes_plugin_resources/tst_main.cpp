// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtCore/QtGlobal>
#include <QtCore/QScopeGuard>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickView>
#include <QtTest/QTest>

#include <QtQuickTestUtils/private/viewtestutils_p.h>

static bool gotShaderErrorMessage = false;
QtMessageHandler oldHandler = nullptr;

void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // If QuickShapesPrivate's resource object files that contain shaders are not linked to the
    // the final executable, QRhiImplementation::sanityCheckGraphicsPipeline will issue a warning
    // that we intercept.
    if (type == QtWarningMsg && msg == QLatin1String("Empty shader passed to graphics pipeline")) {
        gotShaderErrorMessage = true;
    }

    if (oldHandler)
        oldHandler(type, context, msg);
}

class tstImportStaticShapesPluginResources : public QObject
{
    Q_OBJECT
private slots:
    void cleanup();
    void loadApp();
};

void tstImportStaticShapesPluginResources::cleanup()
{
    if (oldHandler) {
        qInstallMessageHandler(oldHandler);
        oldHandler = nullptr;
    }
}

void tstImportStaticShapesPluginResources::loadApp()
{
    gotShaderErrorMessage = false;
    oldHandler = qInstallMessageHandler(messageHandler);

    QQuickView view;
    QVERIFY(QQuickTest::showView(view, QUrl("qrc:///app.qml")));
    QCOMPARE(gotShaderErrorMessage, false);
}

QTEST_MAIN(tstImportStaticShapesPluginResources)
#include "tst_main.moc"
