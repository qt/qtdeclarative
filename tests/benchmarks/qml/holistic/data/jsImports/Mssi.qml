// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0

import "mssi.js" as MssiJs

Item {
    id: testQtObject
    property int importedScriptFunctionValue: MssiJs.testFunc(20)
}
