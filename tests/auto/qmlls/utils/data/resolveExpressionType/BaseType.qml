// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    function helloMethod() {}
    property int helloProperty
    signal helloSignal
    enum HelloEnum { HelloEnumValue1, HelloEnumValue2, HelloEnumValue3 }
    property font helloFont
}
