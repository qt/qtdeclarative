// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

//![0]
// myscript.js
function getRandom(previousValue) {
    return Math.floor(previousValue + Math.random() * 90) % 360;
}
//![0]
