// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

import QtQuick
import QtQuick.Templates as T
import Qt.labs.StyleKit
import Qt.labs.StyleKit.impl

T.RangeSlider {
    id: control

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            first.implicitHandleWidth + leftPadding + rightPadding,
                            second.implicitHandleWidth + leftPadding + rightPadding,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             first.implicitHandleHeight + topPadding + bottomPadding,
                             second.implicitHandleHeight + topPadding + bottomPadding,
                             implicitContentHeight + topPadding + bottomPadding)

    leftPadding: styleReaderFirst.leftPadding
    topPadding: styleReaderFirst.topPadding
    rightPadding: styleReaderFirst.rightPadding
    bottomPadding: styleReaderFirst.bottomPadding

    leftInset: styleReaderFirst.background.leftMargin
    topInset: styleReaderFirst.background.topMargin
    rightInset: styleReaderFirst.background.rightMargin
    bottomInset: styleReaderFirst.background.bottomMargin

    spacing: styleReaderFirst.spacing

    states: [
        /* The delegate logic is moved out of the delegate, to relieve the style
         * developer from having to re-invent it if he changes the delegate. To
         * disable it, 'states' can be set to an empty array from the outside. */
        State {
            when: horizontal
            PropertyChanges  {
                // The width of the handle is fixed. But its margins are used
                // to describe the area within the content area that the handle
                // is allowed to move.
                control.first.handle.x: leftPadding + styleReaderFirst.handle.leftMargin
                   + (first.visualPosition * (availableWidth - styleReaderFirst.handle.leftMargin
                         - styleReaderFirst.handle.rightMargin - first.handle.width))
                // The height of the handle is fixed. But the margins are used to
                // shift its position up or down from the center of the content area.
                control.first.handle.y: topPadding
                    + styleReaderFirst.handle.topMargin - styleReaderFirst.handle.bottomMargin
                    + (availableHeight - first.handle.height) / 2

                control.second.handle.x: leftPadding + styleReaderSecond.handle.leftMargin
                   + (second.visualPosition * (availableWidth - styleReaderSecond.handle.leftMargin
                         - styleReaderSecond.handle.rightMargin - second.handle.width))

                control.second.handle.y: topPadding
                    + styleReaderSecond.handle.topMargin - styleReaderSecond.handle.bottomMargin
                    + (availableHeight - second.handle.height) / 2
            }
        },
        State {
            // Note: we deliberatly flip margins (but not padding) in vertical
            // mode since a vertical slider is logically a flipped horizontal slider.
            when: control.vertical
            PropertyChanges  {
                control.first.handle.x: leftPadding
                   + styleReaderFirst.handle.topMargin - styleReaderFirst.handle.bottomMargin
                   + (availableWidth + first.handle.height) / 2
                control.first.handle.y: topPadding + styleReaderFirst.handle.leftMargin
                   + (first.visualPosition * (availableHeight - styleReaderFirst.handle.leftMargin
                         - styleReaderFirst.handle.rightMargin - first.handle.width))
                control.first.handle.rotation: 90
                control.first.handle.transformOrigin: Item.TopLeft

                control.second.handle.x: leftPadding
                   + styleReaderSecond.handle.topMargin - styleReaderSecond.handle.bottomMargin
                   + (availableWidth + second.handle.height) / 2
                control.second.handle.y: topPadding + styleReaderSecond.handle.leftMargin
                   + (second.visualPosition * (availableHeight - styleReaderSecond.handle.leftMargin
                         - styleReaderSecond.handle.rightMargin - second.handle.width))
                control.second.handle.rotation: 90
                control.second.handle.transformOrigin: Item.TopLeft
            }
        }
    ]

    readonly property StyleReader styleReader: styleReaderFirst

    StyleVariation.controlType: styleReaderFirst.controlType
    StyleReader {
        id: styleReaderFirst
        controlType: StyleReader.Slider
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.first.hovered
        pressed: control.first.pressed
        palette: control.palette
        vertical: !control.horizontal
    }

    StyleReader {
        id: styleReaderSecond
        controlType: StyleReader.Slider
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.second.hovered
        pressed: control.second.pressed
        palette: control.palette
        vertical: !control.horizontal
    }

    StyleReader {
        id: styleReaderIndicator
        controlType: StyleReader.Slider
        enabled: control.enabled
        focused: control.activeFocus
        hovered: control.hovered || control.first.hovered || control.second.hovered
        pressed: control.first.pressed || control.second.pressed
        palette: control.palette
        vertical: !control.horizontal
    }

    first.handle: HandleDelegate {
        quickControl: control
        handleStyle: styleReaderFirst.handle.first
    }

    second.handle: HandleDelegate {
        quickControl: control
        handleStyle: styleReaderSecond.handle.second
    }

    background: BackgroundAndIndicatorDelegate {
        quickControl: control
        indicatorStyle: styleReaderIndicator.indicator
        backgroundStyle: styleReaderIndicator.background
        indicator.firstProgress: control.first.position
        indicator.secondProgress: control.second.position
        vertical: control.vertical
    }
}
