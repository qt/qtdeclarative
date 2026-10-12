import qqmllanguage.Test
import QtQml

VariantAssociationProvider {
    property var varPropertyMap;
    property var varPropertyHash;

    Component.onCompleted: {
        variantMap.email = "something";
        varPropertyMap = variantMap;
        variantMap.email = "map@map.map";

        variantHash.email = "something";
        varPropertyHash = variantHash;
        variantHash.email = "hash@hash.hash";
    }
}
