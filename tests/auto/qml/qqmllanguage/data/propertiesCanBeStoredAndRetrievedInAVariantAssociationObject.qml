import qqmllanguage.Test
import QtQml

VariantAssociationProvider {
    property string variantMapSymbolProperty: ""
    property int variantMapArrayIndexProperty: 0

    property string variantHashSymbolProperty: ""
    property int variantHashArrayIndexProperty: 0

    Component.onCompleted: {
        variantMap.email = "foo@bar.com";
        variantMap[1] = 1;

        variantMapSymbolProperty = variantMap.email;
        variantMapArrayIndexProperty = variantMap[1];

        variantHash.email = "foo@bar.com";
        variantHash[1] = 1;

        variantHashSymbolProperty = variantHash.email;
        variantHashArrayIndexProperty = variantHash[1];
    }
}
