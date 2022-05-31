// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QGuiApplication>
#include <QList>

#include <qqmlengine.h>
#include <qqmlcontext.h>
#include <qqml.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>

#include "dataobject.h"

/*
   This example illustrates exposing a QList<QObject*> as a
   model in QML
*/

//![0]
int main(int argc, char ** argv)
{
    QGuiApplication app(argc, argv);

    const QStringList colorList = {"red",
                                   "green",
                                   "blue",
                                   "yellow"};

    const QStringList moduleList = {"Core", "GUI", "Multimedia", "Multimedia Widgets", "Network",
                                    "QML", "Quick", "Quick Controls", "Quick Dialogs",
                                    "Quick Layouts", "Quick Test", "SQL", "Widgets", "3D",
                                    "Android Extras", "Bluetooth", "Concurrent", "D-Bus",
                                    "Gamepad", "Graphical Effects", "Help", "Image Formats",
                                    "Location", "Mac Extras", "NFC", "OpenGL", "Platform Headers",
                                    "Positioning", "Print Support", "Purchasing", "Quick Extras",
                                    "Quick Timeline", "Quick Widgets", "Remote Objects", "Script",
                                    "SCXML", "Script Tools", "Sensors", "Serial Bus",
                                    "Serial Port", "Speech", "SVG", "UI Tools", "WebEngine",
                                    "WebSockets", "WebView", "Windows Extras", "XML",
                                    "XML Patterns", "Charts", "Network Authorization",
                                    "Virtual Keyboard", "Quick 3D", "Quick WebGL"};

    QList<QObject *> dataList;
    for (const QString &module : moduleList)
        dataList.append(new DataObject("Qt " + module, colorList.at(rand() % colorList.length())));

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setInitialProperties({{ "model", QVariant::fromValue(dataList) }});
//![0]

    view.setSource(QUrl("qrc:/qt/qml/objectlistmodel/view.qml"));
    view.show();

    return app.exec();
}
