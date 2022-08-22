// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQml
import QmltcTests

TypeWithManyProperties {
    id: self

    property alias hasAllAttributesAlias: self.hasAllAttributes
    //hasAllAttributes: "The string."
    hasAllAttributesAlias: "The string." // when this is missing then qmltc does not emit any error...
    property alias hasAllAttributes2Alias: self.hasAllAttributes2

    property alias readOnlyAlias: self.readOnly
    property alias readAndWriteMemberAlias: self.readAndWriteMember
    property alias resettableAlias: self.resettable
    property alias unresettableAlias: self.unresettable
    property alias notifiableAlias: self.notifiable
    property alias notifiableMemberAlias: self.notifiableMember
    property alias latestReadAndWriteAlias: self.latestReadAndWrite

    // aliases cannot be readonly, they inherit it from the property pointed to
    // readonly default property alias readOnlyAlias2: self.readAndWriteMember // not valid

    default property alias defaultAlias: self.readAndWriteMember

    // cannot be compiled by qmlcachegen it seems: required property cannot have initializer
    // required property alias requiredAlias: self.hasAllAttributes

    function assignUndefinedToResettableAlias() {
        resettableAlias = undefined
    }
    function assignUndefinedToUnresettableAlias() {
        unresettableAlias = undefined
    }
}
