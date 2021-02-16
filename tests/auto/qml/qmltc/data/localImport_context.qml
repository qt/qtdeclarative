LocallyImported_context {
    id: bar
    property int p1: 41
    count: p1 + 1

    function localGetMagicValue() {
        // call base type's method
        return getMagicValue();
    }
}
