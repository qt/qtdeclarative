// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQuick/qquickview.h>
#include <QtQuick/qquickwindow.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQml/qqmlcontext.h>

#include <QtQuick/private/qquicktreeview_p.h>
#include <QtQuick/private/qquicktreeview_p_p.h>

#include <QtGui/qguiapplication.h>

#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qapplication.h>
#include <QtGui/qfilesystemmodel.h>
#endif

#include "testmodel.h"

int main(int c, char **args) {
#ifdef QT_WIDGETS_LIB
    QApplication app(c, args);
#else
    QGuiApplication app(c, args);
#endif

    QFileSystemModel model;
    model.setIconProvider(nullptr); // save time: we don't need icons
    model.setRootPath("/");

    QQmlApplicationEngine engine("qrc:data/treeview.qml");
    engine.rootContext()->setContextProperty("fileSystemModel", &model);

    QQuickWindow *window = static_cast<QQuickWindow *>(engine.rootObjects().at(0));
    auto treeView = window->property("treeView").value<QQuickTreeView *>();
    treeView->expand(0);

    return app.exec();
}
