import QtQuick 2.0
import Qt.test 1.0

Item {
    MiscTypeTest {
        id: mtt
    }

    function test_invalid_url_equal()
    {
        return mtt.invalidUrl() == mtt.invalidUrl();
    }

    function test_invalid_url_refequal()
    {
        return mtt.invalidUrl() === mtt.invalidUrl();
    }

    function test_valid_url_equal()
    {
        return mtt.validUrl() == mtt.validUrl();
    }

    function test_valid_url_refequal()
    {
        return mtt.validUrl() === mtt.validUrl();
    }
}
