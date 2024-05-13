// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//! [file]
import QtQuick
import QtQuick.Controls.Basic

StackView {
    id: control

    popEnter: Transition {
        XAnimator {
            from: (control.mirrored ? -1 : 1) * -control.width
            to: 0
            duration: 400
            easing.type: Easing.OutCubic
        }
    }

    popExit: Transition {
        XAnimator {
            from: 0
            to: (control.mirrored ? -1 : 1) * control.width
            duration: 400
            easing.type: Easing.OutCubic
        }
    }
}
//! [file]
