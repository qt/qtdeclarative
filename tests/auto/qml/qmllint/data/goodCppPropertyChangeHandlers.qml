import PropertyChangeHandlers 1.0
import QtQuick
Item {
    TypeWithProperties {
        onAChanged: { console.log("OK"); }
        onBChanged: { console.log("OK"); }
        onCChanged: { console.log("OK"); }
        onDChanged: { console.log("OK"); }
        onEChanged: { console.log("OK"); }
    }

    TypeWithProperties {
        onCWeirdSignal: { console.log("OK"); }
        onDSignal: { console.log("OK"); }
    }

    TypeWithProperties {
        onCWeirdSignal: function(value) { console.log("OK?", value); }
        onDSignal: function(value, str) { console.log("OK?", value, str); }
    }

    TypeWithProperties {
        onCChanged: function(value) { console.log("OK?", value); }
        onDChanged: function(value, str) { console.log("OK?", value, str); }
    }
}
