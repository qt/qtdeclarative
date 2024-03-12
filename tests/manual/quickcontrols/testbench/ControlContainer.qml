// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: container

    implicitWidth: delegate.implicitWidth
    implicitHeight: delegate.implicitHeight

    property var controlMetaObject
    property var states

    Loader {
        id: delegate
        sourceComponent: controlMetaObject ? controlMetaObject.component : null

        function is(state) {
            return container.states.indexOf(state) !== -1
        }
    }
}
