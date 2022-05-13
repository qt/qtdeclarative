// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml 2.0
import QtQuick 2.0
import QtQuick.Window 2.0
import QtQuick.LocalStorage 2.0
import Test 2.0
import TestPlugin 1.0
import "."

QtObject {
    property TestType tt //No object, although it should be properly parented if there were one
    property TestTypeCpp tt2 //No object, although it should be properly parented if there were one
    property TestTypePlugin tt3
    property Item it
}
