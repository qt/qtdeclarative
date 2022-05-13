// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Item {
    width: 320
    height: 480

    ListView {
        anchors.fill: parent
        model: simpleModel
        delegate: Text {
            text: name
        }
    }

    ListModel {
        id: simpleModel
        ListElement {
            name: "first"
        }
        ListElement {
            name: "second"
        }
        ListElement {
            name: "third"
        }
    }
}
