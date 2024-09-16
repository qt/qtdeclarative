// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import TimeExample2 // import types from the plugin
import BasicExtension

Clock { // this class is defined in QML (Clock.qml)
    property Time time: Time {} // this class is defined in C++ (plugin.cpp)

    hours: time.hour
    minutes: time.minute
    property Extension extension // from BasicExtension
    property More more: More {}
    property string fromESModule: ESModule.eee()
    property string fromJSFile: Less.bar()
}
