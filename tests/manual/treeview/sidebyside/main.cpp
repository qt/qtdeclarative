/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtQuick/qquickview.h>
#include <QtQuick/qquickwindow.h>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQml/qqmlcontext.h>

#include <QtQuick/private/qquicktreeview_p.h>
#include <QtQuick/private/qquicktreeview_p_p.h>

#include <QtGui/qguiapplication.h>

#ifdef QT_WIDGETS_LIB
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qtreeview.h>
#endif

#include "testmodel.h"

int main(int c, char **args) {
#ifdef QT_WIDGETS_LIB
    QApplication app(c, args);
#else
    QGuiApplication app(c, args);
#endif

    QQmlApplicationEngine engine("qrc:data/treeview.qml");
    QQuickWindow *window = static_cast<QQuickWindow *>(engine.rootObjects().at(0));
    auto treeView = window->property("treeView").value<QQuickTreeView *>();
    treeView->expand(0);

#ifdef QT_WIDGETS_LIB
    // Show widget version of the treeview as well
    QTreeView treeViewWidget;
    treeViewWidget.setModel(treeView->model().value<TestModel *>());
    treeViewWidget.setExpanded(treeViewWidget.model()->index(0, 0), true);
    treeViewWidget.resize(640, 480);
    treeViewWidget.show();
    treeViewWidget.raise();
#endif

    return app.exec();
}
