// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.NativeStyle as NativeStyle

NativeStyle.DefaultSlider {
    id: control
    readonly property Item __focusFrameTarget: handle
    readonly property Item __focusFrameStyleItem: handle
    font.pixelSize: background.styleFont(control).pixelSize

    background: NativeStyle.Slider {
        control: control
        subControl: NativeStyle.Slider.Groove | NativeStyle.Slider.Handle
        // We normally cannot use a nine patch image for the
        // groove if we draw tickmarks (since then the scaling
        // would scale the tickmarks too). The groove might
        // also use a different background color before, and
        // after, the handle.
        useNinePatchImage: false
    }

    handle: NativeStyle.Slider {
        // The handle is hidden, since it will be drawn as a part
        // of the background. But will still needs it to be here so
        // that we can place the focus rect correctly.
        visible: false

        control: control
        subControl: NativeStyle.Slider.Handle
        x: control.leftPadding + (control.horizontal ? control.visualPosition * (control.availableWidth - width) : (control.availableWidth - width) / 2)
        y: control.topPadding + (control.horizontal ? (control.availableHeight - height) / 2 : control.visualPosition * (control.availableHeight - height))
        useNinePatchImage: false
    }
}
