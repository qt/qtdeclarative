import qqmllanguage.Test
import QtQml

VariantAssociationProvider {
    property string mapEmail: ""
    property string hashEmail: ""

    Component.onCompleted: {
        let map = variantMap;
        let mapReference = map;

        map.email = "map@map.map";

        let hash = variantHash;
        let hashReference = hash;

        hash.email = "hash@hash.hash";

        mapEmail = mapReference.email;
        hashEmail = hashReference.email;
    }
}
