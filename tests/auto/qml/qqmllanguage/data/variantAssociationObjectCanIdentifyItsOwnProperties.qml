import qqmllanguage.Test
import QtQml

VariantAssociationProvider {
    property bool variantMapHadPropertyFromParent: true
    property bool variantMapHadMissingOwnProperty: false
    property bool variantMapHadOwnProperty: false

    property bool variantHashHadPropertyFromParent: true
    property bool variantHashHadMissingOwnProperty: false
    property bool variantHashHadOwnProperty: false

    Component.onCompleted: {
        variantMapHadPropertyFromParent = variantMap.hasOwnProperty("constructor")
        variantHashHadPropertyFromParent = variantMap.hasOwnProperty("constructor")

        variantMapHadMissingOwnProperty = variantMap.hasOwnProperty("email")
        variantMap.email = "foo"
        variantMapHadOwnProperty = variantMap.hasOwnProperty("email")

        variantHashHadMissingOwnProperty = variantHash.hasOwnProperty("email")
        variantHash.email = "foo"
        variantHashHadOwnProperty = variantHash.hasOwnProperty("email")
    }
}
