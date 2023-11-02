// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: container

    implicitWidth: delegate.implicitWidth
    implicitHeight: delegate.implicitHeight

    required property var controlMetaObject
    required property var states
    required property int index

    Loader {
        id: delegate
        sourceComponent: controlMetaObject ? controlMetaObject.component : null

        readonly property alias index: container.index

        function is(state) {
            return container.states.indexOf(state) !== -1
        }

        function anyStateContains(state) {
            return container.states.some(s => s.indexOf(state) !== -1)
        }
    }
}
