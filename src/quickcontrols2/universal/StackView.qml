/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.Universal

T.StackView {
    id: control

    popEnter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.InQuint }
            NumberAnimation { property: "x"; from: (control.mirrored ? -0.3 : 0.3) * -control.width; to: 0; duration: 400; easing.type: Easing.OutCubic }
        }
    }

    popExit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 200; easing.type: Easing.OutQuint }
    }

    pushEnter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.InQuint }
            NumberAnimation { property: "x"; from: (control.mirrored ? -0.3 : 0.3) * control.width; to: 0; duration: 400; easing.type: Easing.OutCubic }
        }
    }

    pushExit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 200; easing.type: Easing.OutQuint }
    }

    replaceEnter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200; easing.type: Easing.InQuint }
            NumberAnimation { property: "x"; from: (control.mirrored ? -0.3 : 0.3) * control.width; to: 0; duration: 400; easing.type: Easing.OutCubic }
        }
    }

    replaceExit: Transition {
        NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 200; easing.type: Easing.OutQuint }
    }
}
