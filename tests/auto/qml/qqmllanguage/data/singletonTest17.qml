// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import org.qtproject.Test 1.0

Item {
    id: test

    property int value1: RegisteredSingleton.testProp1;
    property string value2: "Test value: " + RegisteredSingleton.testProp3;
}
