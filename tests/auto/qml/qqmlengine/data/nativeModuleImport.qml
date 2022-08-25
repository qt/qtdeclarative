// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml

import "info.mjs" as Info
import "nativeModuleImport.mjs" as Test

QtObject {
    property string a: Test.func()
    property string b: Test.getName()
    property string c: Info.name
    property string d: Test.name
    property string e: Test.foo
}
