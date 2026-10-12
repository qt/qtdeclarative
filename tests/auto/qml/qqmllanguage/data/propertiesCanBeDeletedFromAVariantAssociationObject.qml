import qqmllanguage.Test
import QtQml

VariantAssociationProvider {
    property string variantMapEmailBeforeDelete: ""
    property string variantHashEmailBeforeDelete: ""

    property bool mapDeleteReturnedTrue: false
    property bool hashDeleteReturnedTrue: false

    property bool deleteOfNonExistingThrew: false

    Component.onCompleted: {
        variantMap.email = "foo@bar.com"
        variantMapEmailBeforeDelete = variantMap.email
        mapDeleteReturnedTrue = delete variantMap.email

        variantHash.email = "foo@bar.com"
        variantHashEmailBeforeDelete = variantHash.email
        hashDeleteReturnedTrue = delete variantHash.email

        try {
            delete variantMap.nonexisting
            delete variantHash.nonexisting
        } catch (_) {
            deleteOfNonExistingThrew = true;
        }
    }
}
