// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Simulates CoffeeCard: a composite child type used inside the form.

import QtQuick

Item {
    id: root
    property string coffeeName: ""
    property string ingredients: ""
    property int time: 0
    property alias button: btn

    width: 200
    height: 100

    Text {
        id: nameText
        text: root.coffeeName
    }
    Text {
        id: ingredientsText
        text: root.ingredients
        anchors.top: nameText.bottom
    }
    Item {
        id: btn
        objectName: "button"
        anchors.top: ingredientsText.bottom
    }
}
