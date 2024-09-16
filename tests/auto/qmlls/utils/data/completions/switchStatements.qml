// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    property int myProperty;
    function g(x) { return x + 1; }
    function f(x) {
        switch(x) {

        case 1:
            return 0;
        case g(x) + 3: {
            return 1;
        }
        }

        switch(x) {
        case 42:
            myProperty = x + f(x)
            myProperty = myProperty * 0.33
        default:
            return 123456
        case 666:
            for(;;) {
                g(x)
            }
            myProperty = "hello"
        }
        switch(x) {
        default:
            break;
        }
    }

}
