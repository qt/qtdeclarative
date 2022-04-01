pragma Strict

// Import the latest version of Ambiguous, which is the C++ type added in version 1.2, not this one.
import TestTypes as Test

// Not an inheritance cycle, and also not an ambiguous type.
Test.Ambiguous {
    property string i: objectName + 2
}
