import QtQuick
import QmltcTests 1.0
Item {
    id: root
    property alias extCountAlias: withExtension.count
    property alias extDerivedCountAlias: withExtensionDerived.count
    property alias extDerivedStrAlias: withExtensionDerived.str
    property alias extNamespaceCountAlias: withExtensionNamespace.count

    Text {
        id: text
        font.letterSpacing: 13
        property bool shouldBeVisible: true
    }

    TypeWithExtension {
        id: withExtension
        count: -10
        property bool shouldBeVisible: true
    }

    TypeWithExtensionDerived {
        id: withExtensionDerived
        str: "hooray"
        count: -10
        property bool shouldBeVisible: true
    }

    TypeWithExtensionNamespace {
        id: withExtensionNamespace
        count: -10
        property bool shouldBeVisible: true
    }

    TypeWithBaseTypeExtension {
        id: withBaseTypeExtension
        str: "hooray"
        count: -10
        property bool shouldBeVisible: true
    }

    QmlTypeWithExtension {
        id: qmlWithExtension
        count: -10
        property bool shouldBeVisible: true
    }

    QmlTypeWithBaseTypeExtension {
        id: qmlWithBaseTypeExtension
        str: "hooray"
        count: -10
        property bool shouldBeVisible: true
    }

    // script bindings on properties:

    TypeWithExtensionDerived {
        id: withExtensionDerivedScript
        str: "hoo" + "ray"
        count: -10 + 0
    }

    TypeWithExtensionNamespace {
        id: withExtensionNamespaceScript
        count: -10 + 0
    }
}
