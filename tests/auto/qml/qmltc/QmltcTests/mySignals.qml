// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QmltcTests
import QtQuick


TypeWithSignal {
    property int primitive
    property font gadget
    property QtObject object

    // value types by value
    onSignalWithPrimitive: (x) => { primitive = x; }
    onSignalWithGadget: (x) => { gadget = x; }

    // value types by const reference
    onSignalWithConstReferenceToGadget: (x) => { gadget = x; }

    // object by pointers
    onSignalWithPointer: (x) => { object = x; }
    onSignalWithPointerToConst: (x) => { object = x; }
    onSignalWithPointerToConst2: (x) => { object = x; }
    onSignalWithConstPointer: (x) => { object = x; }
    onSignalWithConstPointerToConst: (x) => { object = x; }
    onSignalWithConstPointerToConst2: (x) => { object = x; }
}
