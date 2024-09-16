// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: 400
    height: 400

    ListView {
        anchors.fill: parent
        model: 5
//![0]
        delegate: Component {
            Loader {
                source: "MyComponent.qml" //okay
            }
        }
//![0]
    }
}

