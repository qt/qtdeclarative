// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import Qt.test 1.0

Item {
    id: arbitraryVariantConsumer
    property MyArbitraryVariantProvider a: MyArbitraryVariantProvider { id: arbitraryVariantProvider }
    property variant someVariant;
    property int sideEffect: 10

    function callCppFunction() {
        // in this case, we call a const CPP function with variant return value and integer arguments.
        arbitraryVariantConsumer.sideEffect = arbitraryVariantConsumer.sideEffect + 3;
        someVariant = arbitraryVariantProvider.possibleVariant(sideEffect, sideEffect * 2, sideEffect * 5);
    }
}
