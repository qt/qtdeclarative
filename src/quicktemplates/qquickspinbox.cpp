// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquickspinbox_p.h"

#include <private/qquickcontrol_p_p.h>
#include <private/qquickindicatorbutton_p.h>
#include <private/qquicktextinput_p.h>

#include <private/qqmlengine_p.h>

#include <QtQml/qqmlinfo.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SpinBox
    \inherits Control
//!     \nativetype QQuickSpinBox
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-input
    \ingroup qtquickcontrols-focusscopes
    \brief Allows the user to select from a set of preset values.

    \image qtquickcontrols-spinbox.png
           {Spin box with numeric value and buttons}

    SpinBox allows the user to choose an integer value by clicking the up
    or down indicator buttons, or by pressing up or down on the keyboard.
    Optionally, SpinBox can be also made \l editable, so the user can enter
    a text value in the input field.

    By default, SpinBox provides discrete values in the range of \c [0-99]
    with a \l stepSize of \c 1.

    \snippet qtquickcontrols-spinbox.qml 1

    \section2 Custom Values

    \image qtquickcontrols-spinbox-textual.png
           {Spin box displaying textual values}

    Even though SpinBox works on integer values, it can be customized to
    accept arbitrary input values. The following snippet demonstrates how
    \l validator, \l textFromValue and \l valueFromText can be used to
    customize the default behavior.

    \snippet qtquickcontrols-spinbox-textual.qml 1

    A prefix and suffix can be added using regular expressions:

    \snippet qtquickcontrols-spinbox-prefix.qml 1

    \sa Tumbler, {Customizing SpinBox}, {Focus Management in Qt Quick Controls}
*/

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \include qquickspinbox.qdocinc {valueModified} {SpinBox}
*/

class QQuickSpinBoxPrivate
    : public QQuickAbstractSpinBox<QQuickSpinBox, int>::QQuickAbstractSpinBoxPrivate
{
    Q_DECLARE_PUBLIC(QQuickSpinBox)

public:
    QQuickSpinBoxPrivate();

    bool setValue(int newValue, bool wrap, ValueStatus modified) override;

    void updateValue() override;
    void updateDisplayText() override;

    void contentItemTextChanged();

    QString evaluateTextFromValue(int val) const;
    int evaluateValueFromText(const QString &text) const;

    bool live = false;

private:
    friend class QQuickAbstractSpinBox<QQuickSpinBox, int>::QQuickAbstractSpinBoxPrivate;
};

QQuickSpinBoxPrivate::QQuickSpinBoxPrivate()
    : QQuickAbstractSpinBox<QQuickSpinBox, int>::QQuickAbstractSpinBoxPrivate()
{
    from = 0;
    to = 99;
}

// modified indicates if the value was modified by the user and not programatically
// this is then passed on to updateDisplayText to indicate that the user has modified
// the value so it may need to trigger an update of the contentItem's text too

bool QQuickSpinBoxPrivate::setValue(int newValue, bool allowWrap, ValueStatus modified)
{
    Q_Q(QQuickSpinBox);
    int correctedValue = newValue;
    if (q->isComponentComplete())
         correctedValue = boundValue(newValue, allowWrap);

    if (modified == ValueStatus::Unmodified && newValue == correctedValue && newValue == value)
        return false;

    const bool emitSignals = (value != correctedValue);
    value = correctedValue;

    updateDisplayText();
    updateUpEnabled();
    updateDownEnabled();

    // Only emit the signals if the corrected value is not the same as the
    // original value to avoid unnecessary updates
    if (emitSignals) {
        emit q->valueChanged();
        if (modified == ValueStatus::Modified)
            emit q->valueModified();
    }
    return true;
}

void QQuickSpinBoxPrivate::updateValue()
{
    if (!contentItem)
        return;

    const QVariant text = contentItem->property("text");
    if (text.isValid())
        setValue(evaluateValueFromText(text.toString()), false /* wrap */, ValueStatus::Modified);
}

void QQuickSpinBoxPrivate::updateDisplayText()
{
    setDisplayText(evaluateTextFromValue(value));
}

void QQuickSpinBoxPrivate::contentItemTextChanged()
{
    Q_Q(QQuickSpinBox);

    QQuickTextInput *inputTextItem = qobject_cast<QQuickTextInput *>(q->contentItem());
    if (!inputTextItem)
        return;
    QString text = inputTextItem->text();
#if QT_CONFIG(validator)
    if (validator && live)
        validator->fixup(text);
#endif

    if (live) {
        const int enteredVal = evaluateValueFromText(text);
        const int correctedValue = boundValue(enteredVal, false);
        if (correctedValue == enteredVal && correctedValue != value) {
            // If live is true and the text is valid change the value
            // setValue will set the displayText for us.
            q->setValue(correctedValue);
            return;
        }
    }
    // If live is false or the value is not valid, just set the displayText
    setDisplayText(text);
}

QString QQuickSpinBoxPrivate::evaluateTextFromValue(int val) const
{
    Q_Q(const QQuickSpinBox);

    QString text;
    QQmlEngine *engine = qmlEngine(q);
    if (engine && textFromValue.isCallable()) {
        QJSValue loc;
#if QT_CONFIG(qml_locale)
        loc = QJSValuePrivate::fromReturnedValue(
                engine->handle()->fromData(QMetaType::fromType<QLocale>(), &locale));
#endif
        text = textFromValue.call(QJSValueList() << val << loc).toString();
    } else {
        text = locale.toString(val);
    }
    return text;
}

int QQuickSpinBoxPrivate::evaluateValueFromText(const QString &text) const
{
    Q_Q(const QQuickSpinBox);
    int value;
    QQmlEngine *engine = qmlEngine(q);
    if (engine && valueFromText.isCallable()) {
        QJSValue loc;
#if QT_CONFIG(qml_locale)
        loc = QJSValuePrivate::fromReturnedValue(
                engine->handle()->fromData(QMetaType::fromType<QLocale>(), &locale));
#endif
        value = valueFromText.call(QJSValueList() << text << loc).toInt();
    } else {
        value = locale.toInt(text);
    }
    return value;
}

QQuickSpinBox::QQuickSpinBox(QQuickItem *parent)
    : QQuickControl(*(new QQuickSpinBoxPrivate), parent),
      QQuickAbstractSpinBox<QQuickSpinBox, int>()
{
}

QQuickSpinBox::~QQuickSpinBox()
{

}

/*!
    \include qquickspinbox.qdocinc {from} {SpinBox} {0}
*/

void QQuickSpinBox::setFrom(int from)
{
    Q_D(QQuickSpinBox);
    if (d->from == from)
        return;

    d->from = from;
    emit fromChanged();
    if (isComponentComplete()) {
        if (!d->setValue(d->value,
                         /* allowWrap = */ false, QQuickSpinBoxPrivate::ValueStatus::Unmodified)) {
            d->updateUpEnabled();
            d->updateDownEnabled();
        }
    }
}

/*!
    \include qquickspinbox.qdocinc {to} {SpinBox} {99}
*/

void QQuickSpinBox::setTo(int to)
{
    Q_D(QQuickSpinBox);
    if (d->to == to)
        return;

    d->to = to;
    emit toChanged();
    if (isComponentComplete()) {
        if (!d->setValue(d->value,
                         /* allowWrap = */ false, QQuickSpinBoxPrivate::ValueStatus::Unmodified)) {
            d->updateUpEnabled();
            d->updateDownEnabled();
        }
    }
}

/*!
    \include qquickspinbox.qdocinc {value-prop} {int} {SpinBox} {0}
*/

void QQuickSpinBox::setValue(int value)
{
    Q_D(QQuickSpinBox);
    d->setValue(value, false /* wrap */, QQuickSpinBoxPrivate::ValueStatus::Unmodified);
}

/*!
    \include qquickspinbox.qdocinc {stepSize} {SpinBox} {1}
*/

void QQuickSpinBox::setStepSize(int step)
{
    Q_D(QQuickSpinBox);
    if (d->stepSize == step)
        return;

    d->stepSize = step;
    emit stepSizeChanged();
}

/*!
    \include qquickspinbox.qdocinc {editable} {SpinBox}
*/

/*!
    \qmlproperty bool QtQuick.Controls::SpinBox::live
    \since 6.6

    This property holds whether the \l value is updated when the user edits the
    \l displayText. The default value is \c false. If this property is \c true and
    the value entered by the user is valid and within the bounds of the spinbox
    [\l from, \l to], the value of the SpinBox will be set. If this property is
    \c false or the value entered by the user is outside the boundaries, the
    value will not be updated until the enter or return keys are pressed, or the
    input field loses focus.

    \sa editable, displayText
*/
bool QQuickSpinBox::isLive() const
{
    Q_D(const QQuickSpinBox);
    return d->live;
}

void QQuickSpinBox::setLive(bool live)
{
    Q_D(QQuickSpinBox);
    if (d->live == live)
        return;

    d->live = live;

    //make sure to update the value when changing to live
    if (live)
        d->contentItemTextChanged();

    emit liveChanged();
}

#if QT_CONFIG(validator)
/*!
    \qmlproperty Validator QtQuick.Controls::SpinBox::validator

    This property holds the input text validator for editable spinboxes. By
    default, SpinBox uses \l IntValidator to accept input of integer numbers.

    \code
    SpinBox {
        id: control
        validator: IntValidator {
            locale: control.locale.name
            bottom: Math.min(control.from, control.to)
            top: Math.max(control.from, control.to)
        }
    }
    \endcode

    \sa editable, textFromValue, valueFromText, {Control::locale}{locale},
        {Validating Input Text}
*/
#endif

/*!
    \qmlproperty function QtQuick.Controls::SpinBox::textFromValue

    This property holds a callback function that is called whenever
    an integer value needs to be converted to display text.

    The default function can be overridden to display custom text for a given
    value. This applies to both editable and non-editable spinboxes;
    for example, when using the up and down buttons or a mouse wheel to
    increment and decrement the value, the new value is converted to display
    text using this function.

    The callback function signature is \c {string function(value, locale)}.
    The function can have one or two arguments, where the first argument
    is the value to be converted, and the optional second argument is the
    locale that should be used for the conversion, if applicable.

    The default implementation does the conversion using
    \l {string Number::toLocaleString(locale, format, precision)}{Number.toLocaleString}():

    \code
    textFromValue: function(value, locale) { return Number(value).toLocaleString(locale, 'f', 0); }
    \endcode

    \note When applying a custom \c textFromValue implementation for editable
    spinboxes, a matching \l valueFromText implementation must be provided
    to be able to convert the custom text back to an integer value.

    \sa valueFromText, validator, {Control::locale}{locale}
*/
QJSValue QQuickSpinBox::textFromValue() const
{
    Q_D(const QQuickSpinBox);
    if (!d->textFromValue.isCallable()) {
        QQmlEngine *engine = qmlEngine(this);
        if (engine)
            d->textFromValue = engine->evaluate(QStringLiteral("(function(value, locale) { return Number(value).toLocaleString(locale, 'f', 0); })"));
    }
    return d->textFromValue;
}

/*!
    \qmlproperty function QtQuick.Controls::SpinBox::valueFromText

    This property holds a callback function that is called whenever
    input text needs to be converted to an integer value.

    This function only needs to be overridden when \l textFromValue
    is overridden for an editable spinbox.

    The callback function signature is \c {int function(text, locale)}.
    The function can have one or two arguments, where the first argument
    is the text to be converted, and the optional second argument is the
    locale that should be used for the conversion, if applicable.

    The default implementation does the conversion using \l {QtQml::Locale}{Number.fromLocaleString()}:

    \code
    valueFromText: function(text, locale) { return Number.fromLocaleString(locale, text); }
    \endcode

    \note When applying a custom \l textFromValue implementation for editable
    spinboxes, a matching \c valueFromText implementation must be provided
    to be able to convert the custom text back to an integer value.

    \sa textFromValue, validator, {Control::locale}{locale}
*/
QJSValue QQuickSpinBox::valueFromText() const
{
    Q_D(const QQuickSpinBox);
    if (!d->valueFromText.isCallable()) {
        QQmlEngine *engine = qmlEngine(this);
        if (engine)
            d->valueFromText = engine->evaluate(QStringLiteral("(function(text, locale) { return Number.fromLocaleString(locale, text); })"));
    }
    return d->valueFromText;
}

/*!
    \include qquickindicatorbutton.qdocinc {properties} {SpinBox} {up}

    The \c up.hovered property was introduced in \l{QtQuick.Controls} 2.1,
    and the \c up.implicitIndicatorWidth and \c up.implicitIndicatorHeight
    properties were introduced in \l{QtQuick.Controls} 2.5.

    \sa increase()
*/

/*!
    \include qquickindicatorbutton.qdocinc {properties} {SpinBox} {down}

    The \c down.hovered property was introduced in \l{QtQuick.Controls} 2.1,
    and the \c down.implicitIndicatorWidth and \c down.implicitIndicatorHeight
    properties were introduced in \l{QtQuick.Controls} 2.5.

    \sa decrease()
*/

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \include qquickspinbox.qdocinc {inputMethodHints} {SpinBox}
*/

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \include qquickspinbox.qdocinc {inputMethodComposing} {SpinBox}
*/

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \include qquickspinbox.qdocinc {wrap} {SpinBox}
*/

void QQuickSpinBox::setWrap(bool wrap)
{
    Q_D(QQuickSpinBox);
    if (d->wrap == wrap)
        return;

    d->wrap = wrap;
    if (d->value == d->from || d->value == d->to) {
        d->updateUpEnabled();
        d->updateDownEnabled();
    }
    emit wrapChanged();
}

/*!
    \since QtQuick.Controls 2.4 (Qt 5.11)
    \qmlproperty string QtQuick.Controls::SpinBox::displayText
    \readonly

    This property holds the textual value of the spinbox.

    The value of the property is based on \l textFromValue and \l {Control::}
    {locale}, and equal to:
    \badcode
    var text = spinBox.textFromValue(spinBox.value, spinBox.locale)
    \endcode

    \sa textFromValue
*/

/*!
    \include qquickspinbox.qdocinc {increase} {SpinBox}
*/

/*!
    \include qquickspinbox.qdocinc {decrease} {SpinBox}
*/

void QQuickSpinBox::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickSpinBox);
    if (QQuickTextInput *oldInput = qobject_cast<QQuickTextInput *>(oldItem)) {
        disconnect(oldInput, &QQuickTextInput::inputMethodComposingChanged, this, &QQuickSpinBox::inputMethodComposingChanged);
        QObjectPrivate::disconnect(oldInput, &QQuickTextInput::textChanged, d, &QQuickSpinBoxPrivate::contentItemTextChanged);
    }

    if (newItem) {
        newItem->setActiveFocusOnTab(true);
        if (d->activeFocus)
            newItem->forceActiveFocus(static_cast<Qt::FocusReason>(d->focusReason));
#if QT_CONFIG(cursor)
        if (d->editable)
            newItem->setCursor(Qt::IBeamCursor);
#endif

        if (QQuickTextInput *newInput = qobject_cast<QQuickTextInput *>(newItem)) {
            connect(newInput, &QQuickTextInput::inputMethodComposingChanged, this, &QQuickSpinBox::inputMethodComposingChanged);
            QObjectPrivate::connect(newInput, &QQuickTextInput::textChanged, d, &QQuickSpinBoxPrivate::contentItemTextChanged);
        }
    }
}

QQuickControlPrivate *QQuickSpinBox::d_base_func()
{
    Q_D(QQuickSpinBox);
    return d;
}
const QQuickControlPrivate *QQuickSpinBox::d_base_func() const
{
    Q_D(const QQuickSpinBox);
    return d;
}

QT_END_NAMESPACE

#include "moc_qquickspinbox_p.cpp"
