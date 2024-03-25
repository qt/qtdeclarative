// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QmltcTests 1.0

TypeWithRequiredProperties {
    id: self
    required property int primitiveType
    required property list<int> valueList
    required property list<Item> objectList

    property int propertyThatWillBeMarkedRequired

    // This is already bound so it should not appear as part of the
    // bundle.
    inheritedRequiredPropertyThatWillBeBound : 10

    // This should be ignored as it alias a required property we are
    // already going to consider. It should thus not appear as part of
    // the bundle.
    property alias aliasToRequiredProperty : self.primitiveType

    required propertyThatWillBeMarkedRequired
    required nonRequiredInheritedPropertyThatWillBeMarkedRequired

    property alias aliasToRequiredInner: inner.requiredInner

    // This should be ignored as the underlying property is already bound.
    property alias aliasToRequiredBoundedInner: inner.requiredBoundedInner

    property alias aliasToInnerThatWillBeMarkedRequired: inner.nonRequiredInner
    required aliasToInnerThatWillBeMarkedRequired

    property int notRequired
    property alias requiredAliasToUnrequiredProperty : self.notRequired
    required requiredAliasToUnrequiredProperty

    // When we have an alias to a required property in the same scope
    // we exclude the alias in favor of setting the property directly.
    // See for example aliasToRequiredProperty in this file.
    //
    // The following alias should instead be picked up, as it point to
    // an inner scope.
    // Nonetheless, an initial implementation had a bug that would
    // discard the alias as long as a property with the same name as
    // the target was present in the same scope.
    //
    // The following alias tests this initially failing case.
    property alias aliasToPropertyThatShadows: inner.primitiveType

    property Item children : Item {
        id: inner
        required property int requiredInner
        property int nonRequiredInner
        required property int requiredBoundedInner
        requiredBoundedInner: 43

        required property int primitiveType
    }
}
