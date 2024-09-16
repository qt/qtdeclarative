// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.5

Rectangle {
    width: 300
    height: 300

    property string activatedShortcut
    property string ambiguousShortcut

    property alias shortcuts: repeater.model

    Repeater {
        id: repeater

        Item {
            Shortcut {
                sequence: modelData.sequence
                enabled: modelData.enabled
                autoRepeat: modelData.autoRepeat
                context: modelData.context
                onActivated: activatedShortcut = sequence
                onActivatedAmbiguously: ambiguousShortcut = sequence
            }
        }
    }
}
