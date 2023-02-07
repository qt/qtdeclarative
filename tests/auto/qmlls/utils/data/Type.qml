// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.15

BaseType {
    id: derived
    property int inTypeDotQml

    component MyInlineComponent: BaseType {
        id: derivedInIC
        property int inMyInlineComponent
    }

    property BaseType inlineType: BaseType {
         id: derivedInline
    }

    property MyInlineComponent icType: MyInlineComponent {
        id:derivedInIcInline
    }

    property var icType2: MyBaseInlineComponent {
        id:derivedInIcInline2
    }

    property var nestedIcType: MyNestedInlineComponent {
        id:derivedInIcInline3
    }
}
