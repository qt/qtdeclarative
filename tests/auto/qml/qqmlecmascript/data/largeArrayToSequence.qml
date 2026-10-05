import QtQml

QtObject {
    property list<int> values
    property int count: -1

    Component.onCompleted: {
        let array = [];
        for (let i = 0; i < 600000; i++)
            array.push(i);

        values = array;
        count = values.length;
    }
}
