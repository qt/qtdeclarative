// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    property color rectColor: "red"

    Rectangle {
        property int d: 100
        id: square
        width: d
        height: d
        anchors.centerIn: parent
        color: root.rectColor
        NumberAnimation on rotation { from: 0; to: 360; duration: 2000; loops: Animation.Infinite; }
    }

    Text {
        id: text
        anchors.centerIn: parent
        property string api
        Connections {
            target: text.GraphicsInfo
            function onApiChanged() {
                var api = text.GraphicsInfo.api;
                if (api === GraphicsInfo.Software)
                    text.api = "Software";
                else if (api === GraphicsInfo.OpenGL)
                    text.api = "OpenGL on QRhi";
                else if (api === GraphicsInfo.Direct3D11)
                    text.api = "D3D11 on QRhi";
                else if (api === GraphicsInfo.Direct3D12)
                    text.api = "D3D12 on QRhi";
                else if (api === GraphicsInfo.Vulkan)
                    text.api = "Vulkan on QRhi";
                else if (api === GraphicsInfo.Metal)
                    text.api = "Metal on QRhi";
                else if (api === GraphicsInfo.Null)
                    text.api = "Null on QRhi";
                else
                    text.api = "Unknown API";
            }
        }
        text: "Qt Quick running in a widget\nGraphicsInfo.api says: " + api
    }

    function performLayerBasedGrab(fn) {
        root.grabToImage(function(result) {
            result.saveToFile(fn);
        });
    }
}
