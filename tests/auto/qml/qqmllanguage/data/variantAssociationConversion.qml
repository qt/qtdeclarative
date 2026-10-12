import qqmllanguage.Test
import QtQml

VariantAssociationProvider {
    property string foo: "bar"
    property list<int> baz: [10, 20, 30]
    variantMap: {"first": foo, "second": baz, "third": Component}
    variantHash: {"first": foo, "second": baz, "third": Component}
}
