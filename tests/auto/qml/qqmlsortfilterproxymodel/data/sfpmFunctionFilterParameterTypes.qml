// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

QtObject {
    property SortFilterProxyModel hashModel: SortFilterProxyModel {
        filters: FunctionFilter {
            property int searchValue: 42
            function filter(hashData: var) : bool {
                return hashData.testValue === searchValue
            }
        }
    }

    property SortFilterProxyModel mapModel: SortFilterProxyModel {
        filters: FunctionFilter {
            property int searchValue: 42
            function filter(mapData: var) : bool {
                return mapData.testValue === searchValue
            }
        }
    }

    property SortFilterProxyModel listModel: SortFilterProxyModel {
        filters: FunctionFilter {
            property int searchValue: 42
            function filter(listData: var) : bool {
                return listData.length === 2 && listData[0] === searchValue
            }
        }
    }

    property SortFilterProxyModel nestedModel: SortFilterProxyModel {
        filters: FunctionFilter {
            property int searchValue: 42
            function filter(nestedData: var) : bool {
                return nestedData.items[0].testValue === searchValue
            }
        }
    }

    property SortFilterProxyModel varIntModel: SortFilterProxyModel {
        filters: FunctionFilter {
            property int searchValue: 42
            function filter(intData: var) : bool {
                return intData === searchValue
            }
        }
    }

    property SortFilterProxyModel intModel: SortFilterProxyModel {
        filters: FunctionFilter {
            property int searchValue: 42
            function filter(intData: int) : bool {
                return intData === searchValue
            }
        }
    }
}
