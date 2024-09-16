// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

import "moduleBm.js" as ModuleBmJs

Item {
    id: testQtObject
    property int importedScriptFunctionValue: ModuleBmJs.testFunc(20)
}
