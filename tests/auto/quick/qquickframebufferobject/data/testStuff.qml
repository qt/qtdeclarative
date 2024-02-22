// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import FBOItem 1.0

Item {
    width: 400
    height: 400

    FBOItem {
        objectName: "fbo"
        color: "red"
        width: 100
        height: 100
    }

}
