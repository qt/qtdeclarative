// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// Simulates ChoosingCoffeeForm.ui.qml: a form with multiple CoffeeCard
// children (composite types) that have IDs and are exposed as aliases.

import QtQuick
import QtQuick.Layouts

Item {
    id: root
    property alias cappuccinoButton: cappuccino.button
    property alias latteButton: latte.button
    property alias espressoButton: espresso.button
    property alias macchiatoButton: macchiato.button
    property alias cards: cards
    property alias cappuccino: cappuccino
    property alias macchiato: macchiato
    property alias espresso: espresso
    property alias latte: latte

    GridLayout {
        id: cards
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        rowSpacing: 20
        columnSpacing: 20
        CoffeeCard {
            id: cappuccino
            coffeeName: "Cappuccino"
            ingredients: "Milk, Espresso, Foam"
            time: 2
        }
        CoffeeCard {
            id: latte
            coffeeName: "Latte"
            ingredients: "Coffee, Foam"
            time: 3
        }
        CoffeeCard {
            id: espresso
            coffeeName: "Espresso"
            ingredients: "Milk, Espresso"
            time: 2
        }
        CoffeeCard {
            id: macchiato
            coffeeName: "Macchiato"
            ingredients: "Milk foam, Espresso"
            time: 4
        }
    }
}
