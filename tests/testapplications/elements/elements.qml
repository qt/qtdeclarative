// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import "content/elements.js" as Elements
import "content"

Item {
    id: elementsapp; height: 640; width: 360

    property string qmlfiletoload: ""
    property string helptext: ""

    GradientElement { anchors.fill: parent }

    GridViewElement { height: parent.height * .95; width: parent.width * .95; anchors.centerIn: parent; }

    HelpDesk { width: parent.width; height: 200; anchors { bottom: parent.bottom; right: parent.right; bottomMargin: 3; rightMargin: 3 } }

    // Start or remove an .qml when the qmlfiletoload property changes
    onQmlfiletoloadChanged: {
        if (qmlfiletoload == "") {
            Elements.removeApp();
        } else {
            Elements.setapp(qmlfiletoload,elementsapp);
        }
    }

    // Set the qmlfiletoload property with a script function
    function runapp(qmlfile) {
        console.log("Starting ",qmlfile);
        qmlfiletoload = qmlfile;
    }
}
