// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Frame {
    id: container

    property var controlMetaObject

    ColumnLayout {
        id: exampleLayout
        anchors.centerIn: parent

        Label {
            text: !exampleLoader.active ? qsTr("Show example") : qsTr("Hide example")

            Layout.alignment: Qt.AlignHCenter

            MouseArea {
                anchors.fill: parent
                onClicked: exampleLoader.active = !exampleLoader.active
            }
        }

        Loader {
            id: exampleLoader
            active: false
            sourceComponent: controlMetaObject ? controlMetaObject.exampleComponent : null

            Layout.preferredHeight: active ? item.implicitHeight : 0
        }
    }
}
