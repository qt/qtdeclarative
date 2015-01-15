import Test 1.0

ItemModelsTest {
    property var itemSelection
    property int count
    property bool contains: false

    function range(top, bottom, left, right, parent) {
        if (parent === undefined)
            parent = invalidModelIndex()
        var topLeft = model.index(top, left, parent)
        var bottomRight = model.index(bottom, right, parent)
        return createItemSelectionRange(topLeft, bottomRight)
    }

    onModelChanged: {
        itemSelection = createItemSelection()
        itemSelection.prepend(range(0, 0, 0, 5))
        itemSelection.append(range(0, 5, 0, 0))
        for (var i = 0; i < 3; i++)
            itemSelection.insert(i, range(i, i + 1, i + 2, i + 3))

        var itemSelection2 = createItemSelection()
        for (i = 3; i < 6; i++)
            itemSelection2.select(model.index(i, i + 1), model.index(i + 2, i + 3))

        itemSelection.merge(itemSelection2, 2 /*ItemSelectionModel.Select*/)

        count = itemSelection.length
        contains = itemSelection.contains(model.index(0, 0))

        itemSelection.removeAt(3)
        itemSelection.removeFirst()
        itemSelection.removeLast()
    }
}
