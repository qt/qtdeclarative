// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtGui>
#include <QtQuick>

#include "beziercurve.h"

//! [1]
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QQuickView view;
    QSurfaceFormat format = view.format();
    format.setSamples(16);
    view.setFormat(format);
    view.setSource(QUrl("qrc:///scenegraph/customgeometry/main.qml"));
    view.show();

    return app.exec();
}
//! [1]
