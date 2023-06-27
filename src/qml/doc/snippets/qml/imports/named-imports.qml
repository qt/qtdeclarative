// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [imports]
import QtQuick as QtLibrary
import "../MyComponents" as MyComponents
import com.nokia.qml.mymodule 1.0 as MyModule
//! [imports]

Item {
    //! [imported items]
    QtLibrary.Rectangle {
        // ...
    }

    MyComponents.Slider {
        // ...
    }

    MyModule.SomeComponent {
        // ...
    }
    //! [imported items]
}
