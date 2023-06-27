// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    width: 200; height: 250

    //! [model]
    ListModel { id: fruitModel }
    //! [model]

    //! [view]
    ListView {
        anchors.fill: parent
        model: fruitModel
        delegate: Row {
            id: delegate
            required property string name
            required property real cost

            Text { text: "Fruit: " + delegate.name }
            Text { text: "Cost: $" + delegate.cost }
        }
    }
    //! [view]

    //! [mouse area]
    MouseArea {
        anchors.fill: parent
        onClicked: fruitModel.append({"cost": 5.95, "name":"Pizza"})
    }
    //! [mouse area]
}
