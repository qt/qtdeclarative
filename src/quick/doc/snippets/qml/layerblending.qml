// Copyright (C) 2014 Gunnar Sletta <gunnar@sletta.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: 300
    height: 200

    Row {
        height: 100
        anchors.centerIn: parent
        spacing: 50

//! [non-layered]
Item {
    id: nonLayered

    opacity: 0.5

    width: 100
    height: 100

    Rectangle { width: 80; height: 80; border.width: 1 }
    Rectangle { x: 20; y: 20; width: 80; height: 80; border.width: 1 }
}
//! [non-layered]

//! [layered]
Item {
    id: layered

    opacity: 0.5

    layer.enabled: true

    width: 100
    height: 100

    Rectangle { width: 80; height: 80; border.width: 1 }
    Rectangle { x: 20; y: 20; width: 80; height: 80; border.width: 1 }
}
//! [layered]

    }
}
