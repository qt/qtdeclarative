import QtQml 2.15

ListModel {
    id: listModel

    Component.onCompleted: {
        const rawObj = { innerList: [1, 2, 3] }
        listModel.append(rawObj)
        const convertedObj = listModel.get(0)

        console.log(rawObj.innerList)
        console.log(Object.values(rawObj.innerList))

        console.log(convertedObj.innerList)
        console.log(Object.values(convertedObj.innerList))
    }
}
