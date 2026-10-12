import qqmllanguage.Test
import QtQml
import "singleton"

QtObject {
    property variant singletonInstance: ReadCounterSingleton
    property int finalQualityLevel: ValueTypeWithEnum1.LowQuality

    function randomBetween(min, max) {
        const minceil = Math.ceil(min);
        const maxfloor = Math.floor(max);
        return Math.floor(Math.random() * (maxfloor - minceil) + minceil);
    }

    Component.onCompleted: {
        // 2 accesses
        ReadCounterSingleton.valueType.quality = ValueTypeWithEnum1.LowQuality;

        let counter = "e"
        let prop = "valueTyp" + counter
        // 0 accesses. The data will be obtained at the first occasion.
        let singletonReference = ReadCounterSingleton[prop];

        let accesses = randomBetween(4, 100);

        while (accesses > 0) {
            // 2 accesses on the first read. No accesses after as
            // there is no change.
            singletonReference.quality;
            --accesses;
        }

        // 2 accesses
        ReadCounterSingleton.valueType.quality = ValueTypeWithEnum1.VeryHighQuality;
        // 2 accesses
        finalQualityLevel = singletonReference.quality;
    }
}
