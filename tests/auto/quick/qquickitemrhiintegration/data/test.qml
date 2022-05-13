// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import TestQquickitemrhiintegration

Item {
    width: 640
    height: 480

    Text {
        id: apiInfo
        color: "black"
        font.pixelSize: 16
        property int api: GraphicsInfo.api
        text: {
            if (GraphicsInfo.api === GraphicsInfo.OpenGLRhi)
                "OpenGL on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.Direct3D11Rhi)
                "D3D11 on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.VulkanRhi)
                "Vulkan on QRhi";
            else if (GraphicsInfo.api === GraphicsInfo.MetalRhi)
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
    }
}
