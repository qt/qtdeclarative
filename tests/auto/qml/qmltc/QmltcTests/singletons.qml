// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QmltcTests

Item {
    property int cppSingleton1: SingletonType.mySingletonValue
    property int cppSingleton2: SingletonType.mySingletonValue

    property int qmlSingleton1: SingletonThing.integerProperty
    property int qmlSingleton2: SingletonThing.integerProperty

    property int cppEnum: SingletonType.Enum // should be 1
    property int qmlEnum: SingletonThing.Special // should be 1

    function writeSingletonType() {
        SingletonType.mySingletonValue = 100
    }

    function writeSingletonThing() {
        SingletonThing.integerProperty = 100
    }
}
