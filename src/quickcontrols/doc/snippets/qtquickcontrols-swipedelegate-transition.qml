// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GFDL-1.3-no-invariants-only

import QtQuick
import QtQuick.Controls

//! [1]
SwipeDelegate {
    swipe.transition: Transition {
        SmoothedAnimation { velocity: 3; easing.type: Easing.InOutCubic }
    }
}
//! [1]
