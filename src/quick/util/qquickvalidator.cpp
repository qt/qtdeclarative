// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickvalidator_p.h"

QT_BEGIN_NAMESPACE

#if QT_CONFIG(validator)

/*!
    \qmltype IntValidator
    \nativetype QIntValidator
    \inqmlmodule QtQuick
    \ingroup qtquick-text-utility
    \ingroup qtquick-text-validators
    \brief Defines a validator for integer values.

    The IntValidator type provides a validator for integer values.

    If no \l locale is set IntValidator uses the \l {QLocale::setDefault()}{default locale} to
    interpret the number and will accept locale specific digits, group separators, and positive
    and negative signs.  In addition, IntValidator is always guaranteed to accept a number
    formatted according to the "C" locale.

    The following example shows a TextInput object with an IntValidator to check
    that the user has input an integer within the specified range, updating the
    text color to highlight invalid input:

    \snippet qml/intvalidator.qml 0

    \sa DoubleValidator, RegularExpressionValidator, {Validating Input Text}
*/

QQuickIntValidator::QQuickIntValidator(QObject *parent)
    : QIntValidator(parent)
{
}

/*!
    \qmlproperty string QtQuick::IntValidator::locale

    This property holds the name of the locale used to interpret the number.

    \sa {QtQml::Qt::locale()}{Qt.locale()}
*/

QString QQuickIntValidator::localeName() const
{
    return locale().name();
}

void QQuickIntValidator::setLocaleName(const QString &name)
{
    if (locale().name() != name) {
        setLocale(QLocale(name));
        emit localeNameChanged();
    }
}

void QQuickIntValidator::resetLocaleName()
{
    QLocale defaultLocale;
    if (locale() != defaultLocale) {
        setLocale(defaultLocale);
        emit localeNameChanged();
    }
}

/*!
    \qmlproperty int QtQuick::IntValidator::top

    This property holds the validator's highest acceptable value.
    By default, this property's value is derived from the highest signed integer available (typically 2147483647).
*/
/*!
    \qmlproperty int QtQuick::IntValidator::bottom

    This property holds the validator's lowest acceptable value.
    By default, this property's value is derived from the lowest signed integer available (typically -2147483647).
*/

/*!
    \qmltype DoubleValidator
    \nativetype QDoubleValidator
    \inqmlmodule QtQuick
    \ingroup qtquick-text-utility
    \ingroup qtquick-text-validators
    \brief Defines a validator for non-integer numbers.

    The DoubleValidator type provides a validator for non-integer numbers.

    \list
    \li Accepted Input: Input is accepted if it contains a double that is within
        the valid range and is in the correct format.
    \li Accepted but Invalid Input: Input is accepted but considered invalid if
        it contains a double that is outside the valid range or is in the wrong
        format (for example, too many digits after the decimal point or empty).
    \li Rejected Input: Input is rejected if it is not a double.
    \endlist

    \note If the valid range consists of only positive doubles (for example,
    0.0 to 100.0) and the input is a negative double, it is rejected. If
    \l notation is set to \c DoubleValidator.StandardNotation and the input
    contains more digits before the decimal point than a double in the valid
    range may have, it is also rejected. If \l notation is
    \c DoubleValidator.ScientificNotation and the input is not in the valid
    range, it is accepted but invalid. The value may become valid by changing
    the exponent.

    The following example shows a TextInput object with a DoubleValidator to
    check that the user has input a double within the specified range,
    updating the text color to highlight invalid input:

    \snippet qml/doublevalidator.qml 0

    \sa IntValidator, RegularExpressionValidator, {Validating Input Text}
*/

QQuickDoubleValidator::QQuickDoubleValidator(QObject *parent)
    : QDoubleValidator(parent)
{
}

/*!
    \qmlproperty string QtQuick::DoubleValidator::locale

    This property holds the name of the locale used to interpret the number.

    \sa {QtQml::Qt::locale()}{Qt.locale()}
*/

QString QQuickDoubleValidator::localeName() const
{
    return locale().name();
}

void QQuickDoubleValidator::setLocaleName(const QString &name)
{
    if (locale().name() != name) {
        setLocale(QLocale(name));
        emit localeNameChanged();
    }
}

void QQuickDoubleValidator::resetLocaleName()
{
    QLocale defaultLocale;
    if (locale() != defaultLocale) {
        setLocale(defaultLocale);
        emit localeNameChanged();
    }
}

/*!
    \qmlproperty real QtQuick::DoubleValidator::top

    This property holds the validator's maximum acceptable value.
    By default, this property contains a value of infinity.
*/
/*!
    \qmlproperty real QtQuick::DoubleValidator::bottom

    This property holds the validator's minimum acceptable value.
    By default, this property contains a value of -infinity.
*/
/*!
    \qmlproperty int QtQuick::DoubleValidator::decimals

    This property holds the validator's maximum number of digits after the decimal point.
    By default, this property contains a value of 1000.
*/
/*!
    \qmlproperty enumeration QtQuick::DoubleValidator::notation
    This property holds the notation of how a string can describe a number.

    The possible values for this property are:

    \value DoubleValidator.StandardNotation     only decimal numbers with optional sign (e.g. \c -0.015)
    \value DoubleValidator.ScientificNotation   (default) the written number may have an exponent part (e.g. \c 1.5E-2)
*/

/*!
    \qmltype RegularExpressionValidator
    \nativetype QRegularExpressionValidator
    \inqmlmodule QtQuick
    \ingroup qtquick-text-utility
    \ingroup qtquick-text-validators
    \brief Provides a string validator.
    \since 5.14

    The RegularExpressionValidator type provides a validator, that counts as valid any string which
    matches a specified regular expression.

    \sa IntValidator, DoubleValidator, {Validating Input Text}
*/
/*!
   \qmlproperty regularExpression QtQuick::RegularExpressionValidator::regularExpression

   This property holds the regular expression used for validation.

   Note that this property should be a regular expression in JS syntax, e.g /a/ for the regular
   expression matching "a".

   By default, this property contains a regular expression with the pattern \c{.*} that matches any
   string.

   Below you can find an example of a \l TextInput object with a RegularExpressionValidator
   specified:

   \snippet qml/regularexpression.qml 0

   Some more examples of regular expressions:

   \list
   \li A list of numbers with one to three positions separated by a comma:
       \badcode
       /\d{1,3}(?:,\d{1,3})+$/
       \endcode

   \li An amount consisting of up to 3 numbers before the decimal point, and
       1 to 2 after the decimal point:
       \badcode
       /(\d{1,3})([.,]\d{1,2})?$/
       \endcode
   \endlist
*/

#endif // validator

QT_END_NAMESPACE

#include "moc_qquickvalidator_p.cpp"
