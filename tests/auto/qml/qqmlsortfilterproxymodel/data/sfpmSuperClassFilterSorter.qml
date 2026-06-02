// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQml

QtObject {
    id: root

    // FunctionSorter
    component SorterRoleData: QtObject { property string display }
    component BaseFunctionSorter: FunctionSorter {
        function compare(lhsData: SorterRoleData, rhsData: SorterRoleData) : int {
            return (lhsData.display < rhsData.display) ? -1 :
                   (lhsData.display > rhsData.display) ? 1 : 0
        }
    }
    component DerivedFunctionSorter: BaseFunctionSorter {}
    property DerivedFunctionSorter derivedSorter: DerivedFunctionSorter {}

    component OverrideFunctionSorter: BaseFunctionSorter {
        function compare(lhsData: SorterRoleData, rhsData: SorterRoleData) : int {
            return (lhsData.display > rhsData.display) ? -1 :
                   (lhsData.display < rhsData.display) ? 1 : 0
        }
    }
    property OverrideFunctionSorter overrideSorter: OverrideFunctionSorter {}

    // FunctionFilter
    component BaseFunctionFilter: FunctionFilter {
        function filter(display: string) : bool {
            return display == "cherry"
        }
    }
    component DerivedFunctionFilter: BaseFunctionFilter {}
    property DerivedFunctionFilter derivedFilter: DerivedFunctionFilter {}

    component OverrideFunctionFilter: BaseFunctionFilter {
        function filter(display: string) : bool {
            return display != "cherry"
        }
    }
    property OverrideFunctionFilter overrideFilter: OverrideFunctionFilter {}

    property SortFilterProxyModel sfpmProxyModel: SortFilterProxyModel {}
    property SortFilterProxyModel sfpmOverrideProxyModel: SortFilterProxyModel {}
}

