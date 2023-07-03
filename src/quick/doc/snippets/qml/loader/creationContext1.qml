// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick

//![0]
Item {
    width: 400
    height: 400

    Component {
        id: myComponent
        Text { text: index }    //fails
    }

    ListView {
        anchors.fill: parent
        model: 5
        delegate: Component {
            id: delegateComponent
            Loader {
                sourceComponent: myComponent
            }
        }
    }
}
//![0]
