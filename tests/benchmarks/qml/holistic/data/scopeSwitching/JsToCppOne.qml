// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import Qt.test 1.0

Item {
    id: arbitraryVariantConsumer
    property MyArbitraryVariantProvider a: MyArbitraryVariantProvider { id: arbitraryVariantProvider }
    property int sideEffect: 10

    function callCppFunction() {
        // in this case, we call a const CPP function with no return value and no arguments.
        arbitraryVariantConsumer.sideEffect = arbitraryVariantConsumer.sideEffect + 2;
        arbitraryVariantProvider.doNothing();
    }
}
