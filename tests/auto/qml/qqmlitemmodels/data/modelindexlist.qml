import Test 1.0

ItemModelsTest {
    property var modelIndexList
    property int count

    onModelChanged: {
        modelIndexList = createModelIndexList()
        modelIndexList.prepend(model.index(0, 0))
        modelIndexList.append(model.index(1, 1))
        for (var i = 0; i < 3; i++)
            modelIndexList.insert(i, model.index(2 + i, 2 + i))

        count = modelIndexList.length
        modelIndex = modelIndexList.at(0)

        modelIndexList.removeAt(3)
        modelIndexList.removeFirst()
        modelIndexList.removeLast()
    }
}
