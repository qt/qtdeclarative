// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    property alias textArea: textArea
    property alias scrollBar: scrollBar

    Flickable {
        anchors.fill: parent
        TextArea.flickable: TextArea {
            id: textArea
        }
        ScrollBar.vertical: ScrollBar {
            id: scrollBar
            // Need to explicitly set this to account for platforms like Android,
            // where the UiEffects style hint does not include HoverEffect, and
            // hence QQuickControlPrivate::calcHoverEnabled() would otherwise return false.
            hoverEnabled: true
        }
    }
}
