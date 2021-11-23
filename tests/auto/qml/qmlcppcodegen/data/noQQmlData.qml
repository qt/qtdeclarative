import QtQml
import TestTypes

BirthdayParty {
    id: party
    property string inDaHouse: " in da house!"
    property string n: host.name + inDaHouse
    function burn() { host = mrBurns.createObject(); }
    property Component mrBurns: Person { name: "Mr Burns" }
}
