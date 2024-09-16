// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGuiApplication>
#include <QStringList>

#include <qqmlengine.h>
#include <qqmlcontext.h>
#include <qqml.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickview.h>

#include "greeter.h"
#include "hello-swift.h"

int main(int argc, char ** argv)
{
    QGuiApplication app(argc, argv);

    QMetaType::registerConverter<std::string, QString>(&QString::fromStdString);

    Greeter greeter;

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.engine()->rootContext()->setContextProperty("greeter", &greeter);

    view.setSource(QUrl("qrc:/qt/qml/helloswift/main.qml"));
    view.show();

    return app.exec();
}

#include "moc_greeter.cpp"
