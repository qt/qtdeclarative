// This file exists for the sole purpose for qmlimportscanner to find
// which modules it needs to extract for deployment.
// Otherwise, it fails to find the imports that are expressed in the
// C++ code belonging to the test.

import QtQml 2.0
import QtQuick 2.0

QtObject { }    // This is needed in order to keep importscanner happy
