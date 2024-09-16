// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml
import Qt.test

QtObject {
    property listPropertyAssignment_Gadget gadget
    property ListPropertyAssignment_Object object: ListPropertyAssignment_Object { }

    Component.onCompleted: {
        gadget.gadgetStringList = ["Element1", "Element2", "Element3"]
        gadget.gadgetVariantList = [1, "foo", null, true]
        object.qobjectStringList = ["Element1", "Element2", "Element3"]

        gadget.gadgetStringList[0] = "Completely new Element"
        gadget.gadgetVariantList[0] = "Completely new Element"
        object.qobjectStringList[0] = "Completely new Element"
    }
}
