import qqmllanguage.Test
import QtQml

ReadCounter {
    property int accessesCount: 0

    function randomBetween(min, max) {
        const minceil = Math.ceil(min);
        const maxfloor = Math.floor(max);
        return Math.floor(Math.random() * (maxfloor - minceil) + minceil);
    }

    Component.onCompleted: {
        let reference = stringList;
        let accesses = randomBetween(4, 100);
        accessesCount = accesses;

        while (accesses > 0) {
            reference.length;
            stringListChanged();
            --accesses;
        }
    }
}
