// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
ListView {
    width: 400; height: 400
    model: 5
    delegate: myItem.mycomponent    //will create green Rectangles

    MyItem { id: myItem }
}
//![0]
