// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/qguiapplication.h>
#include <QtQuick/QQuickView>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setApplicationName("threadedlistmodel-manual-test");
    QCoreApplication::setOrganizationName("QtProject");

    QQuickView view;
    view.setSource(QUrl(QStringLiteral("qrc:/qt/qml/threadedlistmodel/timedisplay.qml")));
    view.show();

    return app.exec();
}
