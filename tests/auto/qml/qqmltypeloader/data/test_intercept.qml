// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

ListView {
    width: 400
    height: 500
    model: 2

    id: test
    property int created: 0
    property int loaded: 0

    delegate: Loader {
        width: ListView.view.width
        height: 100
        asynchronous: true
        source: index == 0 ? "Intercept.qml" : "GenericView.qml"

        onLoaded: {
            test.loaded++
        }
        Component.onCompleted: {
            test.created++
        }
    }
}
