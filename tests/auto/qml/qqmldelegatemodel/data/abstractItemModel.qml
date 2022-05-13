// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml.Models 2.15
import QtQuick 2.15

import Test 1.0

DelegateModel {
    model: AbstractItemModel {}
    delegate: Item {}
}
