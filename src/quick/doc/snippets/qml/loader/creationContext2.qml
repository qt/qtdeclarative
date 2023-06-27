// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
                sourceComponent: Component {
                    Text { text: index }    //okay
                }
            }
        }
//![0]
    }
}

