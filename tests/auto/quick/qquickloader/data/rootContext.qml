// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

Rectangle {
    width: 360
    height: 360

    property alias loader: loader

    Loader {
        id: loader
    }

    property Component component: Item {
        property bool trigger: false
        onTriggerChanged: {
            objectInRootContext.doIt() // make sure we can resolve objectInRootContext
            loader.active = false
            objectInRootContext.doIt() // make sure we can STILL resolve objectInRootContext
            anotherProperty = true // see if we can trigger subsequent signal handlers (we shouldn't)
        }
        property bool anotherProperty: false
        onAnotherPropertyChanged: {
            // this should never be executed
            objectInRootContext.doIt()
        }
    }
}
