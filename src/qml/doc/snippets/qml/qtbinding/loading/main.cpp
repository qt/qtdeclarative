// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#include <QGuiApplication>
#include <QtQml>
#include <QtQuick>

static void withComponent()
{
//![QQmlComponent-a]
// Using QQmlComponent
QQmlEngine engine;
QQmlComponent component(&engine,
        QUrl::fromLocalFile("MyItem.qml"));
QObject *object = component.create();
//![QQmlComponent-a]
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

//![QQuickView]
// Using QQuickView
QQuickView view;
view.setSource(QUrl::fromLocalFile("MyItem.qml"));
view.show();
QObject *object = view.rootObject();
//![QQuickView]

//![properties]
object->setProperty("width", 500);
QQmlProperty(object, "width").write(500);
//![properties]

//![cast]
QQuickItem *item = qobject_cast<QQuickItem*>(object);
item->setWidth(500);
//![cast]

//![findChild]
QObject *rect = object->findChild<QObject*>("rect");
if (rect)
    rect->setProperty("color", "red");
//![findChild]

//![QQmlComponent-b]
delete object;
//![QQmlComponent-b]

withComponent();

    return app.exec();
}

