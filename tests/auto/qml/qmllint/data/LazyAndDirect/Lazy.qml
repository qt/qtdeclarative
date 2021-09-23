pragma Singleton
import QtQml

QtObject {
    property Direct direct

    function setDirect(newDirect : Direct) { direct = newDirect }
}
