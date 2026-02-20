import Test 1.0

AliasToGroupedPropertyComponent {
    id: root

    // Alias into a grouped property without any grouped property binding.
    // 'grouped' is a MyGroupedObject* with a 'value' property, but no
    // 'grouped.value: ...' binding exists in this document. The alias
    // should still resolve via the property type's metaObject.
    property alias groupedValue: root.grouped.value

    // Alias through an alias property (QTBUG-130605 pattern):
    // 'groupedObj' is an alias to 'grouped' defined in the component.
    // No binding for 'groupedObj' exists in this document either.
    property alias throughAliasValue: root.groupedObj.value
}
