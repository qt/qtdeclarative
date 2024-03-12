// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// This JavaScript file is a single, small, imported script.
// It imports other small scripts which are deeply nested.
// It is imported by another script.

.import "msdsi12.js" as Msdsi12

function testFunc(seedValue) {
    var retn = 0.115;
    retn *= Msdsi12.testFunc(seedValue + retn);
    return retn;
}
