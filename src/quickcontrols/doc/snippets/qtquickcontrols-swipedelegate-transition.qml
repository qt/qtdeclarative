// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

//! [1]
SwipeDelegate {
    swipe.transition: Transition {
        SmoothedAnimation { velocity: 3; easing.type: Easing.InOutCubic }
    }
}
//! [1]
