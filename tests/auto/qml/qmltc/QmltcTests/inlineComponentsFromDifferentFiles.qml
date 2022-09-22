import QtQuick
import QmltcTests as MyModule

Item {
    property MyModule.InlineComponentProvider.IC1 fromModule1
    fromModule1: MyModule.InlineComponentProvider.IC1 {}

    property var fromModule2: MyModule.InlineComponentProvider.IC1 {}

    property InlineComponentProvider.IC1 fromOtherFile1: InlineComponentProvider.IC1 {}
    property InlineComponentProvider.IC2 fromOtherFile2: InlineComponentProvider.IC2 {}
    property InlineComponentProvider.IC3 fromOtherFile3: InlineComponentProvider.IC3 {}

    property InlineComponentReexporter.IC100 reExported: InlineComponentReexporter.IC100 {}

    property var looksLikeEnumIsEnum: InlineComponentProvider.Dog
    property var looksLikeEnumIsInlineComponent: InlineComponentProvider.IC1 {}
}
