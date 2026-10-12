import qqmllanguage.Test
import QtQml

ReadCounter {
    Component.onCompleted: {
        let reference = stringList;

        for (var i = 0; i < 100; ++i) {
            reference.push("foo");
        }

        JSON.stringify(reference);
    }
}
