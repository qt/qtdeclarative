// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQml
import QmltcQProcessTests

TypeWithVersionedAlias {
    id: self
    property alias unexistingProperty: self.notExisting
    property alias existingProperty: self.existing
}
