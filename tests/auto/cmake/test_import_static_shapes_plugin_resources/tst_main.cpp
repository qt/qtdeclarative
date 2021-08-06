/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
