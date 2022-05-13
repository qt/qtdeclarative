// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QtQuick/QQuickView>

#include "spinner.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QQuickView view;
    view.setSource(QUrl("qrc:///scenegraph/threadedanimation/main.qml"));
    view.show();

    return QGuiApplication::exec();
}
