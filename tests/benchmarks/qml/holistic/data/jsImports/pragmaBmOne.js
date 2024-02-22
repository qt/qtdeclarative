// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// This JavaScript file is a single, large, imported script.
// It imports a shared library script.
.import "pragmaLib.js" as PragmaLibJs

function testFuncOne(seedValue) {
    var retn = seedValue + 2;
    retn += PragmaLibJs.testFunc();
    return retn;
}
