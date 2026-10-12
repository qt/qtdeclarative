import qqmllanguage.Test
import QtQml

VariantAssociationProvider {
    property list<var> otherList: [1, 2, 3]
    property list<var> l: []

    Component.onCompleted: {
        l[0] = variantHash;

        variantMap.theList = l;

        variantMap.theList[0].email = "hash@hash.hash";
        variantMap.theList[0].foo = otherList;
        variantMap.theList[0].foo[1] = 200;
    }
}
