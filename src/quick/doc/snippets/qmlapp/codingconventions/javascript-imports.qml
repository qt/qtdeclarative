// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

//![0]
import "myscript.js" as Script

Rectangle { color: "blue"; width: Script.calculateWidth(parent) }
//![0]
