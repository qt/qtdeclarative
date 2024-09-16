// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml 2.0
import QtQuick 2.0
import QtQuick.Window 2.0

import Test 2.0
import "."

QtObject {
    //Doesn't create items, just checks that the types are accessible
    property TestType tt
    property TestTypeCpp ttc
    property Window wi
    property Item it
}
