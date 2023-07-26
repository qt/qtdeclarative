import QtQml 2.15

QtObject {
    id: root
    Component.onCompleted: {
        function WithPrototype(refMsgSeqNr) {
            this.init(refMsgSeqNr)
        };

        WithPrototype.prototype = {
            init: function(refMsgSeqNr) {
                this.testObj = {
                    has: function(a) { return a === refMsgSeqNr }
                }

                this.protocolSubTypeID = 2
                this.messageControl = 0
                this.referredMsgSequenceNumber = refMsgSeqNr
            }
        };

        let comp = Qt.createComponent("dynamic.qml");
        let inst1 = comp.createObject(root, { testObj: new Set(), });
        let inst2 = comp.createObject(root, new WithPrototype(1));

        objectName = inst1.use() + " - " + inst2.use();
    }
}
