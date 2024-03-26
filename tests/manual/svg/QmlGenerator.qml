// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import SvgImageTest

Item {
    id: item
    width: childrenRect.width * (SvgManager.scale / 10.0)
    height: childrenRect.height * (SvgManager.scale / 10.0)
    scale: SvgManager.scale / 10
    transformOrigin: Item.TopLeft

    property var dynamicObject: null
    Connections {
        target: SvgManager
        function onCurrentSourceChanged() {
            if (dynamicObject)
                dynamicObject.destroy()

            var s = SvgManager.qmlSource

            dynamicObject = Qt.createQmlObject(s, item, "dummy.qml")
        }
    }
}
