// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Item {
    property Component mycomponent: comp1

    QtObject {
        id: internalSettings
        property color color: "green"
    }

    Component {
        id: comp1
        Rectangle { color: internalSettings.color; width: 400; height: 50 }
    }
}
//![0]
