// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Item {
    Item {
        property string objName: "Child1"
    }

    Repeater {
        model: 4

        Item {
            property string objName: "Child" + (index + 1)
        }
    }

    Item {
        property string objName: "Child6"
    }
}
