// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Layouts

Item {
    width: 400
    height: 400

    GridLayout {
        columns: 4

        CalculatorButton { text: "7"; objectName: "d7" }
        CalculatorButton { text: "8"; objectName: "d8" }
        CalculatorButton { text: "9"; objectName: "d9" }
        CalculatorButton { text: "4"; objectName: "d4" }

        // Sets a property on the aliased deferred content from the outside.
        CalculatorButton { text: "0"; objectName: "ext"; label.color: "cyan" }
    }
}
