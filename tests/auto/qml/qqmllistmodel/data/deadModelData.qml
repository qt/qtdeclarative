import QtQml

QtObject {
    id: root
    objectName: "root"

    function swapCorpses() {
        const lhsData = getModelData(lhsButtonListModel);
        const rhsData = getModelData(rhsButtonListModel);

        lhsButtonListModel.clear();
        rhsButtonListModel.clear();

        addToModel(lhsButtonListModel, rhsData);
        addToModel(rhsButtonListModel, lhsData);
    }

    property ListModel l1: ListModel {
        id: lhsButtonListModel
    }

    property ListModel l2: ListModel {
        id: rhsButtonListModel
    }

    property ListModel l3: ListModel {
        id: simpleList
        ListElement { name: "a" }
    }

    Component.onCompleted: {
        // set up lhs and rhsButtonListModel for C++
        lhsButtonListModel.append({ "ident": 1,  "buttonText": "B 1"});
        lhsButtonListModel.append({ "ident": 2,  "buttonText": "B 2"});
        lhsButtonListModel.append({ "ident": 3,  "buttonText": "B 3"});

        rhsButtonListModel.append({ "ident": 4,  "buttonText": "B 4"});
        rhsButtonListModel.append({ "ident": 5,  "buttonText": "B 5"});
        rhsButtonListModel.append({ "ident": 6,  "buttonText": "B 6"});

        // check that reads and writes to deleted entries do not cause crashes
        const item = simpleList.get(0)
        simpleList.remove(0)
        item.name = "b" // has no effect
        if (item.name === undefined)
            root.objectName = "works"
    }

    function getModelData(model) {
        var dataList = []
        for (var i = 0; i < model.count; ++i)
            dataList.push(model.get(i));

        return dataList;
    }

    function addToModel(model, buttonData) {
        for (var i = 0; i < buttonData.length; ++i)
            model.append(buttonData[i]);
    }
}
