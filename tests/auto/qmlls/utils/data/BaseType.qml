// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15

Item {
    property string inBaseTypeDotQml: "Hello from BaseType!"

    component MyBaseInlineComponent: Item {
        id: baseIC
    }

    Item {
        id: child

        Item {
            id: nestedChild

            component MyNestedInlineComponent: Item {
                property string inMyNestedInlineComponent: "world"
            }
        }
    }
    property int helloProperty: 123
    function helloFunction() {
        return helloProperty
    }
}
