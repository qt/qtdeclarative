// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    id: root

    property alias game: theGame
    property alias loader: theLoader

    Item {
        id: theGame

        property bool isReady: false

        onStateChanged: {
            if (state == "invalid") {
                // The Loader's active property is bound to isReady, so none of its bindings
                // should be updated when isReady becomes false
                isReady = false;

                player.destroy();
                player = null;
            } else if (state == "running") {
                player = Qt.createQmlObject("import QtQuick 2.0; Item { property color color: 'black' }", root);

                isReady = true;
            }
        }

        property Item player
    }

    Loader {
        id: theLoader
        active: theGame.isReady
        sourceComponent: Rectangle {
            width: 400
            height: 400
            color: game.player.color

            property var game: theGame
        }
    }
}
