// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.Slider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitHandleWidth + leftPadding + rightPadding,
                            implicitContentWidth + leftPadding + rightPadding)

    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitHandleHeight + topPadding + bottomPadding,
                             implicitContentHeight + topPadding + bottomPadding)

    leftPadding: styleReader.leftPadding
    topPadding: styleReader.topPadding
    rightPadding: styleReader.rightPadding
    bottomPadding: styleReader.bottomPadding

    leftInset: styleReader.background.leftMargin
    topInset: styleReader.background.topMargin
    rightInset: styleReader.background.rightMargin
    bottomInset: styleReader.background.bottomMargin

    spacing: styleReader.spacing

    states: [
        /* The delegate logic is moved out of the delegate, to relieve the style
         * developer from having to re-invent it if he changes the delegate. To
         * disable it, 'states' can be set to an empty array from the outside. */
        State {
            when: horizontal
            PropertyChanges  {
                // The width of the handle is fixed. But its margins are is used
                // to describe the area within the content area that the handle
                // is allowed to move.
                control.handle.x: leftPadding + styleReader.handle.leftMargin
                   + (visualPosition * (availableWidth - styleReader.handle.leftMargin
                         - styleReader.handle.rightMargin - handle.width))
                // The height of the handle is fixed. But the margins are used to
                // shift its position up or down from the center of the content area.
                control.handle.y: topPadding
                    + styleReader.handle.topMargin - styleReader.handle.bottomMargin
                    + (availableHeight - control.handle.height) / 2
            }
        },
        State {
            when: control.vertical
            PropertyChanges  {
                control.handle.x: control.leftPadding
                   + styleReader.handle.leftMargin - styleReader.handle.rightMargin
                   + (availableWidth - handle.width) / 2
                control.handle.y: control.topPadding + styleReader.handle.topMargin
                   + (visualPosition * (availableHeight - styleReader.handle.topMargin
                         - styleReader.handle.bottomMargin - handle.height))
            }
        }
    ]

    StyleVariation.controlType: styleReader.controlType
    StyleReader {
        id: styleReader
        controlType: StyleReader.Slider
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered
        pressed: control.pressed
        palette: control.palette
        vertical: !control.horizontal
    }

    handle: HandleDelegate {
        quickControl: control
        handleStyle: styleReader.handle
    }

    background: BackgroundAndIndicatorDelegate {
        quickControl: control
        indicatorStyle: styleReader.indicator
        backgroundStyle: styleReader.background
        indicator.secondProgress: control.position
    }
}
