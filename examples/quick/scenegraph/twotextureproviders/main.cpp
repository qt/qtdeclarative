// Copyright (C) 2014 Gunnar Sletta <gunnar@sletta.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>

#include <QtQuick/QQuickView>

#include "xorblender.h"

int main(int argc, char **argv)
{
    qputenv("QSG_RHI", "1"); // ### Qt 6 remove, this will be the default anyway

    QGuiApplication app(argc, argv);

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:///scenegraph/twotextureproviders/main.qml"));
    view.show();

    return QGuiApplication::exec();
}
