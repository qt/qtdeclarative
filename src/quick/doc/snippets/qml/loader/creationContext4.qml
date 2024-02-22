// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
Item {
    width: 400
    height: 400

    Component {
        id: myComponent
        Text { text: modelIndex }    //okay
    }

    ListView {
        anchors.fill: parent
        model: 5
        delegate: Component {
            Loader {
                property int modelIndex: index
                sourceComponent: myComponent
            }
        }
    }
}
//![0]
