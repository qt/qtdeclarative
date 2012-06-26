%modules = ( # path to module name map
    "QtQml" => "$basedir/src/qml",
    "QtQuick" => "$basedir/src/quick",
    "QtQuickParticles" => "$basedir/src/particles",
    "QtQuickTest" => "$basedir/src/qmltest",
    "QtQmlDevTools" => "$basedir/src/qmldevtools",
);
%moduleheaders = ( # restrict the module headers to those found in relative path
    "QtQmlDevTools" => "../qml/qml/parser",
);
%classnames = (
    "qtqmlversion.h" => "QtQmlVersion",
);
%mastercontent = (
    "gui" => "#include <QtGui/QtGui>\n",
    "network" => "#include <QtNetwork/QtNetwork>\n",
    "testlib" => "#include <QtTest/QtTest>\n",
    "qml" => "#include <QtQml/QtQml>\n",
    "quick" => "#include <QtQuick/QtQuick>\n",
);
%modulepris = (
    "QtQml" => "$basedir/modules/qt_qml.pri",
    "QtQuick" => "$basedir/modules/qt_quick.pri",
    "QtQuickParticles" => "$basedir/modules/qt_quickparticles.pri",
    "QtQuickTest" => "$basedir/modules/qt_qmltest.pri",
    "QtQmlDevTools" => "$basedir/modules/qt_qmldevtools.pri",
);
%deprecatedheaders = (
    "QtQml" => {
        "QQmlImageProvider" => "QtQuick/QQuickImageProvider",
        "qqmlimageprovider.h" => "QtQuick/qquickimageprovider.h",
    },
);
# Module dependencies.
# Every module that is required to build this module should have one entry.
# Each of the module version specifiers can take one of the following values:
#   - A specific Git revision.
#   - any git symbolic ref resolvable from the module's repository (e.g. "refs/heads/master" to track master branch)
#
%dependencies = (
        "qtbase" => "799f0841a6f22ccaa03f3673ba91ad7b40f20612",
        "qtxmlpatterns" => "refs/heads/master",
        "qtjsbackend" => "refs/heads/master",
);
