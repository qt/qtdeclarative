// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

ListView {
    id: root
    width: 360
    height: 360
    cacheBuffer: 100000
    model: ListModel {
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
        ListElement { component: "asyncloadercurrentindex.qml" }
    }

    interactive: false
    orientation: ListView.Horizontal

    currentIndex: 0

    delegate: Loader {
        width: root.width
        height: root.height

        source: component
        asynchronous: true
    }
}
