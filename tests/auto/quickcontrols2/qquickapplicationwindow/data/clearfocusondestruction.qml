// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtGraphicalEffects

ApplicationWindow {
    width: 200
    height: 200
    visible: true

    property alias textfield: textfield

    /*
     * The code below is the simplest way we can trigger that the signal
     * activeFocusItemChanged() is emitted during destruction of the
     * ApplicationWindow. This caused a crash in QQuickApplicationWindow.
     */
    FastBlur {
        id: fastBlur
        anchors.fill: parent
        radius: 30
        source: ShaderEffectSource {
            id: effectsource
            sourceItem: textfield
            sourceRect: Qt.rect( 0, 0, fastBlur.width, fastBlur.height )
        }
    }

    TextField {
        id: textfield
        anchors.bottom: parent.bottom
        focus: true
    }
}
