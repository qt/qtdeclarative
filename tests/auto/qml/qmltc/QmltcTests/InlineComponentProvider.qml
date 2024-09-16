// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    component IC1 : Item {
        property string objName: "IC1"
    }

    component IC2: QtObject {
        property string objName: "IC2"
    }

    enum MyEnum { Jumps, Over, The, Lazy, Dog }

    Item {
        component IC3: Item {
            property string objName: "IC3"
        }
    }


}
