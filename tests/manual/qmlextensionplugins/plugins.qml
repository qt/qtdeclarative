// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
//![0]
import TimeExample // import types from the plugin

Clock { // this class is defined in QML (imports/TimeExample/Clock.qml)

    Time { // this class is defined in C++ (plugin.cpp)
        id: time
    }

    hours: time.hour
    minutes: time.minute

}
//![0]
