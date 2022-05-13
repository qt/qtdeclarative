// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.11

ScrollView {
    clip: true

    function inputAsRow() {
        return {
            checked: checkedCheckBox.checked,
            amount: amountSpinBox.value,
            fruitType: fruitTypeTextField.text,
            fruitName: fruitNameTextField.text,
            fruitPrice: parseFloat(fruitPriceTextField.text)
        }
    }

    default property alias content: gridLayout.children

    GridLayout {
        id: gridLayout
        columns: 2

        Label {
            text: "checked"
        }
        CheckBox {
            id: checkedCheckBox
        }

        Label {
            text: "amount"
        }
        SpinBox {
            id: amountSpinBox
            value: 1
        }

        Label {
            text: "fruitType"
        }
        TextField {
            id: fruitTypeTextField
            text: "Pear"
        }

        Label {
            text: "fruitName"
        }
        TextField {
            id: fruitNameTextField
            text: "Williams"
        }

        Label {
            text: "fruitPrice"
        }
        TextField {
            id: fruitPriceTextField
            text: "1.50"
        }
    }
}
