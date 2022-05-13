// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

ApplicationWindow {
    objectName: "appWin"
    width: 400
    height: 400

    property alias mainItem: mainItem

    font.family: "Arial"

    T.Control {
        id: mainItem
        objectName: "mainItem"
        anchors.fill: parent
        property alias item_2: _item_2;
        property alias item_3: _item_3;
        property alias item_4: _item_4;
        property alias item_5: _item_5;
        property alias item_6: _item_6;
        T.Control {
            id: _item_2
            objectName: "_item_2"
            T.Control {
                id: _item_3
                objectName: "_item_3"
            }
        }
        T.TextArea {
            id: _item_4
            objectName: "_item_4"
            text: "Text Area"
        }
        T.TextField {
            id: _item_5
            objectName: "_item_5"
            text: "Text Field"
        }
        T.Label {
            id: _item_6
            objectName: "_item_6"
            text: "Label"
        }
    }
}
