import QtQml 2.0

QtObject {
    Component.onCompleted: {
        var list = [[1,2],[3,4],[5,6]];

        for (const [x,y] of list)
            console.log("X: "+x+"; Y: "+y);
        for (let [x,y] of list)
            console.log("X: "+x+"; Y: "+y);
    }
}
