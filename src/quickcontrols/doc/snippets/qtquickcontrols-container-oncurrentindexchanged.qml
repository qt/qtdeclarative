// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick.Controls

Container {
    //! [1]
    onCurrentIndexChanged: {
        print("currentIndex changed to", currentIndex)
        // ...
    }
    //! [1]
}
