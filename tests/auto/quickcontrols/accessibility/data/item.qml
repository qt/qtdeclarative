// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
Item {
    Accessible.role: Accessible.Client
    Item {
        id: item1
        property double stepSize: 0
        property double from: 0
        property double to: 100
        property double value: 25
        Accessible.role: Accessible.Slider
        Accessible.onDecreaseAction: { value -= stepSize; }
        Accessible.onIncreaseAction: { value += stepSize; }
    }
    Item {
        id: item2
        property double stepSize: 5
        property double from: 0
        property double to: 100
        property double value: 25
        Accessible.role: Accessible.Slider
        Accessible.onDecreaseAction: { value -= stepSize; }
        Accessible.onIncreaseAction: { value += stepSize; }
    }
    Item {
        id: item3
        property double stepSize: 0
        property double minimumValue: 0
        property double maximumValue: 100
        property double from: -1000
        property double to: 1000
        property double value: 25
        Accessible.role: Accessible.Slider
        Accessible.onDecreaseAction: { value -= stepSize; }
        Accessible.onIncreaseAction: { value += stepSize; }
    }
    Item {
        id: item4
        property double stepSize: 5
        property double minimumValue: 0
        property double maximumValue: 100
        property double from: -1000
        property double to: 1000
        property double value: 25
        Accessible.role: Accessible.Slider
        Accessible.onDecreaseAction: { value -= stepSize; }
        Accessible.onIncreaseAction: { value += stepSize; }
    }
}
