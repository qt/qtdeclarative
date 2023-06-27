// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
//![0]
TextInput {
    id: hexNumber
    validator: RegularExpressionValidator { regularExpression: /[0-9A-F]+/ }
}
//![0]
