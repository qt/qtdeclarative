import qqmllanguage.Test
import QtQml

ReadCounter {
    Component.onCompleted: {
        let rootReference;
        let firstSubpart;
        let secondSubpart;

        rootReference = inner, firstSubpart = rootReference.firstDate,
        secondSubpart = rootReference.secondDate, firstSubpart.getSeconds(),
        secondSubpart.getSeconds(), firstSubpart.getSeconds(),
        secondSubpart.getSeconds(), firstSubpart.setTime(2992),
        firstSubpart.getSeconds(), secondSubpart.getSeconds();
    }
}
