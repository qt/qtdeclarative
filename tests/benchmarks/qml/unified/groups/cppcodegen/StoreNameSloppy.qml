// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma Strict
import QtQml

QtObject {
    property int input: 10
    property int output: 10
    onInputChanged: output = input
}
