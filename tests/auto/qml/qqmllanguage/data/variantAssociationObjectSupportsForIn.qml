import qqmllanguage.Test
import QtQml

VariantAssociationProvider {
    property list<string> variantMapKeys: []
    property list<string> variantHashKeys: []

    Component.onCompleted: {
        variantMap.foo = "bar";
        variantMap.email = "foo@bar.com";
        variantMap[1] = 1;

        for (var element in variantMap) {
            variantMapKeys.push(element);
        }

        variantHash.foo = "bar";
        variantHash.email = "foo@bar.com";
        variantHash[1] = 1;

        for (var element in variantHash) {
            variantHashKeys.push(element);
        }
    }
}
