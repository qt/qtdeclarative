// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import Testqquickrhiitem

Item {
    width: 640
    height: 480

    Text {
        id: apiInfo
        color: "black"
        font.pixelSize: 16
        property int api: GraphicsInfo.api
        text: {
            if (GraphicsInfo.api === GraphicsInfo.OpenGL)
                "OpenGL on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.Direct3D11)
                "D3D11 on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.Direct3D12)
                "D3D12 on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.Vulkan)
                "Vulkan on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.Metal)
                "Metal on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.Null)
                "Null on QRhi";
            else
                "Unknown API";
        }
    }

    TestRhiItem {
        anchors.centerIn: parent
        width: 400
        height: 400
        color: "red"
        objectName: "rhiitem"
    }
}
