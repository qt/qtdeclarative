// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick 2.2
import Qt.labs.settings 1.0

Item {
    id: root

    Item {
        id: sibling1
        property string prop1: "value1"
    }

    Settings {
        property alias alias1: sibling1.prop1
        property alias alias2: sibling2.prop2
    }

    Item {
        id: sibling2
        property string prop2: "value2"
    }
}
