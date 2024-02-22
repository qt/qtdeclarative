// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Templates as T

ApplicationWindow {
    objectName: "appWin"
    width: 400
    height: 400

    property alias mainItem: mainItem

    T.Control {
        id: mainItem
        objectName: "mainItem"
        anchors.fill: parent
        property alias item_2: _item_2;
        property alias item_3: _item_3;
        T.Control {
            id: _item_2
            objectName: "_item_2"
            T.Control {
                id: _item_3
                objectName: "_item_3"
            }
        }
    }
}
