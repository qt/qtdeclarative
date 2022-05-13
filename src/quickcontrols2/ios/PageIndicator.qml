// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import QtQuick.Controls.impl
import QtQuick.Controls.iOS

T.PageIndicator {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    delegate: Image {
        source: control.IOS.url + "pageindicator-delegate"
        ImageSelector on source {
            states: [
                {"light": control.IOS.theme === IOS.Light},
                {"dark": control.IOS.theme === IOS.Dark},
                {"current": index === control.currentIndex},
            ]
        }
        opacity: control.enabled ? 1 : 0.5
    }

    contentItem: Row {
        spacing: 10

        Repeater {
            model: control.count
            delegate: control.delegate
        }
    }
}
