// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/qguiapplication.h>
#include <QtQuick/QQuickView>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName("layoutdirection-manual-test");
    QCoreApplication::setOrganizationName("QtProject");
    QQuickView view;
    view.setSource(QUrl(QStringLiteral("qrc:/qt/qml/layoutdirection/layoutdirection.qml")));
    view.show();

    return app.exec();
}
