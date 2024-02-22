// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0

import "pragmaBmOne.js" as PragmaBmOneJs
import "pragmaBmTwo.js" as PragmaBmTwoJs

Item {
    id: testQtObject

    // value = 20 + 2 + 9 + (nbr times shared testFunc has been called previously == 0)
    property int importedScriptFunctionValueOne: PragmaBmOneJs.testFuncOne(20)

    // value = 20 + 3 + 9 + (nbr times shared testFunc has been called previously == 1)
    property int importedScriptFunctionValueTwo: PragmaBmTwoJs.testFuncTwo(20)
}
