%modules = ( # path to module name map
    "QtQml" => "$basedir/src/qml",
    "QtQuick" => "$basedir/src/quick",
    "QtQuickWidgets" => "$basedir/src/quickwidgets",
    "QtQuickParticles" => "$basedir/src/particles",
    "QtQuickTest" => "$basedir/src/qmltest",
    "QtQmlDevTools" => "$basedir/src/qmldevtools",
    "QtPacketProtocol" => "$basedir/src/plugins/qmltooling/packetprotocol",
    "QtQmlDebug" => "$basedir/src/qmldebug",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
    "QtQmlDevTools" => "../qml/parser;../qml/jsruntime;../qml/qml/ftw;../qml/compiler;../qml/memory;.",
);
%deprecatedheaders = (
);
# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#   - an empty string to use the same branch under test (dependencies will become "refs/heads/master" if we are in the master branch)
#
%dependencies = (
        "qtbase" => "",
        "qtxmlpatterns" => "",
);
