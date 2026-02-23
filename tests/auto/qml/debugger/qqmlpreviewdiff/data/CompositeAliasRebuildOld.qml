// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

// The composite type with aliases IS the document root. Changing a property
// on it triggers a rebuild of the root object, which is also a composite
// type instance. The composite levels loop then tries to repopulate bindings
// in the base type's context, where aliases reference internal IDs.

import QtQuick

CompositeBaseWithAliases {
    width: 400
    height: 400
    header.color: "blue"
}
