// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick.Controls

Container {
    //! [1]
    onCurrentIndexChanged: {
        print("currentIndex changed to", currentIndex)
        // ...
    }
    //! [1]
}
