pragma Strict
import QtQml

LoggingCategory {
    id: self
    name: "foobar"
    Component.onCompleted: {
        console.debug("b", 4.55);
        console.log("b", 4.55);
        console.info("b", 4.55);
        console.warn("b", 4.55);
        console.error("b", 4.55);
        console.debug(self, "b", 4.55);
        console.log(self, "b", 4.55);
        console.info(self, "b", 4.55);
        console.warn(self, "b", 4.55);
        console.error(self, "b", 4.55);
        console.debug(Component, "b", 4.55);
        console.log(Component, "b", 4.55);
        console.info(Component, "b", 4.55);
        console.warn(Component, "b", 4.55);
        console.error(Component, "b", 4.55);

        console.log("a", undefined, "b", false, null, 7);
        console.log();
        console.log(4)
        console.log(self);
        console.log(Component);
    }
}
