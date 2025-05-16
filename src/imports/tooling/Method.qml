// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

import QML

Member {
    default property list<Parameter> parameters
    property string type
    property int revision: 0
    property bool isConstructor: false
    property bool isList: false
    property bool isPointer: false
    property bool isJavaScriptFunction: false
    property bool isCloned: false
    property bool isConstant: false
}
