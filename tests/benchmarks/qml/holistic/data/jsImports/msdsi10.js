// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// This JavaScript file is a single, small, imported script.
// It imports other small scripts which are deeply nested.
// It is imported by another script.

.import "msdsi11.js" as Msdsi11

function testFunc(seedValue) {
    var retn = 0.105;
    retn *= Msdsi11.testFunc(seedValue + retn);
    return retn;
}
