// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QStringList>

#include <qqmlengine.h>
#include <qqmlcontext.h>
#include <qqml.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>


/*
   This example illustrates exposing a QStringList as a
   model in QML
*/

int main(int argc, char ** argv)
{
    QGuiApplication app(argc, argv);

//![0]
    QStringList dataList = {
        "Item 1",
        "Item 2",
        "Item 3",
        "Item 4"
    };

    QQuickView view;
    view.setInitialProperties({{ "model", QVariant::fromValue(dataList) }});
//![0]

    view.setSource(QUrl("qrc:/qt/qml/stringlistmodel/view.qml"));
    view.show();

    return app.exec();
}

