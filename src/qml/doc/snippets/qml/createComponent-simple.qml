// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick

Item {
    id: container
    width: 300; height: 300

    function loadButton() {
        var component = Qt.createComponent("Button.qml");
        if (component.status == Component.Ready) {
            var button = component.createObject(container);
            button.color = "red";
        }
    }

    Component.onCompleted: loadButton()
}
//![0]
