// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import shared as Examples

Item {
    width: 500; height: 500
    Examples.LauncherList {
        id: launcherList
        anchors.fill: parent
        Component.onCompleted: {
            addExample("📚 Stacking", "Window stacking order", Qt.resolvedUrl("examples/stacking.qml"));
            addExample("✂️ Clipping", "Ancestor clipping", Qt.resolvedUrl("examples/clipping.qml"));
            addExample("↘️ Transform", "Transforms", Qt.resolvedUrl("examples/transform.qml"));
            addExample("🗓️ Widget", "A calendar widget", Qt.resolvedUrl("examples/widget.qml"));
            addExample("🕸️ Web", "A native web view", Qt.resolvedUrl("examples/webview.qml"));
            addExample("🗺️ Map", "A native map view", Qt.resolvedUrl("examples/map.qml"));
            addExample("📽️ Video", "A native video player", Qt.resolvedUrl("examples/video.qml"));
            if (haveQtMultimedia)
                addExample("💡 HDR", "A HDR enabled video player", Qt.resolvedUrl("examples/hdr.qml"));
        }
    }
}
