import qqmllanguage.Test
import QtQml

ReadCounter {
    function randomBetween(min, max) {
        const minceil = Math.ceil(min);
        const maxfloor = Math.floor(max);
        return Math.floor(Math.random() * (maxfloor - minceil) + minceil);
    }

    Component.onCompleted: {
        let reference = stringList;
        var accesses = randomBetween(4, 100);
        while (accesses > 0) {
            reference.length;
            --accesses;
        }
    }
}
