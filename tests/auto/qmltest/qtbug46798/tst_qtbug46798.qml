// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick 2.2
import QtQml.Models 2.3
import QtTest 1.1

Item {
    id: top

    ListView {
        id: listViewWithObjectModel // QTBUG-46798
        anchors.fill: parent
        model: ObjectModel {
            id: objectModel
            Rectangle { width: 160; height: 160; color: "red" }
            Rectangle { width: 160; height: 160; color: "green" }
            Rectangle { width: 160; height: 160; color: "blue" }
        }
    }

    TestCase {
        name: "QTBUG-46798"
        when: windowShown

        function test_bug46798() {
            var item = objectModel.get(0)
            if (item) {
                objectModel.remove(0)
                item.destroy()
            }
        }
    }
}
