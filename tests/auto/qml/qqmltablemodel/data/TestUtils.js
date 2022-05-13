// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

function testModelRoleDataProvider(index, role, cellData) {
    switch (role) {
    case "display":
        switch (index.column) {
        case 0:
            return cellData.name
        case 1:
            return cellData.age
        }
        break
    case "name":
        return cellData.name
    case "age":
        return cellData.age
    }
    return cellData
}
