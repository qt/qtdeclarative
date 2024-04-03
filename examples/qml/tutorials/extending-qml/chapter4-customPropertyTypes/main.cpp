// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include "piechart.h"
#include "pieslice.h"

#include <QtQuick/QQuickView>
#include <QGuiApplication>

//![0]
int main(int argc, char *argv[])
{
//![0]
    QGuiApplication app(argc, argv);

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.loadFromModule("Charts", "App");
    view.show();
    return QGuiApplication::exec();

//![2]
}
//![2]
