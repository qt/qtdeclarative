// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    readonly property int test: 34
    signal pressed()
    function f() {
        let sum = 0, sum2 = 0
        for(let i = 1; i < 42; i = i + 2) {
            sum = test + i
            {
                let sum = 42; // another unrelated sum
            }
        }
        // signal and property changed
        testChanged();
        pressed();
    }

    // attached
    Keys.onPressed: {
    }

    // propertychanged handler
    onTestChanged: {
        f(); // method identifier
    }

    // signal handler
    onPressed: {}

    enum K { Plus}
    property int tt: Identifiers.Plus // component and enum value
    property int tt2: Identifiers.K.Plus

}
