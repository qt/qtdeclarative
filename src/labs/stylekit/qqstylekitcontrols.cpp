// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitcontrols_p.h"
#include "qqstylekitcontrol_p.h"
#include "qqstylekitvariation_p.h"
#include "qqstylekitcustomcontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype AbstractStylableControls
    \inqmlmodule Qt.labs.StyleKit
    \brief Abstract base type containing the control types that can be styled.

    AbstractControls is an abstract base type. It contains a \l ControlStyle for
    each control type that can be styled by a \l Style, \l Theme, or \l StyleVariation.

    The control types form a hierarchy where properties set on a base type
    propagate down to the more specific ones. For example, assigning a radius
    of four to \c abstractButton.background.radius will cause all button
    types, such as \l button, \l checkBox, and \l radioButton, to get a radius
    of four.

    The snippets on this page illustrate a few of the key properties that can be
    used to style each control type, but they are by no means exhaustive. Many other properties
    are available, as documented in the \l ControlStyle and \l DelegateStyle documentation.
    In practice, some of them can also be omitted because the
    \l {Style::fallbackStyle}{fallback style} already supplies sensible defaults.
    This means that if you simply remove a property from one of the snippets, it
    might not actually affect its appearance, since it will just be read from the
    fallback style instead.

    \labs
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::control

    Grouped property for styling all controls.

    \c control is the base type in the control hierarchy, and properties set here
    serve as defaults for all other control types. For example, setting
    \c{control.implicitWidth: 200} makes \e all controls 200 pixels wide,
    including buttons, scroll indicators and every other control.
    It also overrides any values inherited from a higher level in the style
    hierarchy (the \l Style for a \l Theme, or the
    \l {Style::fallbackStyle}{fallbackStyle} for a \l Style), \e including hover
    effects and other state-based behavior. Use \c{control} sparingly, and
    prefer more specific base types like \l abstractButton or \l pane when
    possible.

    \snippet ControlsSnippets.qml control
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::abstractButton

    Grouped property for styling all button-like controls, including
    \l [QtQuickControls]{Button}, \l [QtQuickControls]{CheckBox},
    \l [QtQuickControls]{RadioButton}, and \l [QtQuickControls]{Switch}.
    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml abstractButton

    \sa {qtlabsstylekit-controls.html}{Control Types}
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::applicationWindow

    Grouped property for styling \l [QtQuickControls] ApplicationWindow.

    Unset properties fall back to \l control.

    Use \c applicationWindow to set the background color of the window:

    \snippet ControlsSnippets.qml applicationWindow

    \note The application needs to use ApplicationWindow, not Window, for this to take effect.
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::button

    Grouped property for styling \l [QtQuickControls]{Button}.

    Unset properties fall back to \l abstractButton.

    \snippet ControlsSnippets.qml button
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::checkBox

    Grouped property for styling \l [QtQuickControls]{CheckBox}.

    Unset properties fall back to \l abstractButton.

    \snippet ControlsSnippets.qml checkBox
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::comboBox

    Grouped property for styling \l [QtQuickControls]{ComboBox}.

    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml comboBox
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::flatButton

    Grouped property for styling flat buttons (buttons with no visible
    background in their normal state). The styling will take effect for
    a \l [QtQuickControls]{Button} if \l [QtQuickControls]{Button::flat}{Button.flat} is set to \c true.

    Unset properties fall back to \l abstractButton.

    \snippet ControlsSnippets.qml flatButton
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::frame

    Grouped property for styling \l [QtQuickControls]{Frame}.

    Unset properties fall back to \l pane.

    \snippet ControlsSnippets.qml frame
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::groupBox

    Grouped property for styling \l [QtQuickControls]{GroupBox}.

    Unset properties fall back to \l frame.

    \snippet ControlsSnippets.qml groupBox
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::itemDelegate

    Grouped property for styling \l [QtQuickControls]{ItemDelegate}.

    Unset properties fall back to \l control.

    \note In Qt Quick Controls, \l [QtQuickControls]{ItemDelegate} inherits from
    \l [QtQuickControls]{AbstractButton}. In StyleKit, however, \c itemDelegate
    falls back to \l control rather than \l abstractButton, since delegates are
    typically \e styled very differently from buttons (flat, no borders or drop shadows, etc.).

    \snippet ControlsSnippets.qml itemDelegate
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::label

    Grouped property for styling \l [QtQuickControls]{Label}.

    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml label
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::page

    Grouped property for styling \l [QtQuickControls]{Page}.

    Unset properties fall back to \l pane.

    \snippet ControlsSnippets.qml page
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::pane

    Grouped property for styling \l [QtQuickControls]{Pane}.

    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml pane
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::popup

    Grouped property for styling \l [QtQuickControls]{Popup}.

    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml popup
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::progressBar

    Grouped property for styling \l [QtQuickControls]{ProgressBar}.
    For a progress bar, the groove is styled through the indicator, while the progress
    track is styled through the indicator's foreground.

    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml progressBar

    StyleKit doesn't provide a dedicated property to style the indeterminate animation
    of a progress bar. To change the animation, you need to implement a custom indicator
    foreground \l {DelegateStyle::}{delegate} instead:

    \snippet ControlsSnippets.qml progressBar indeterminate
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::radioButton

    Grouped property for styling \l [QtQuickControls]{RadioButton}.

    Unset properties fall back to \l abstractButton.

    \snippet ControlsSnippets.qml radioButton
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::scrollBar

    Grouped property for styling \l [QtQuickControls]{ScrollBar}.
    For a scroll bar, the groove is styled through the indicator, while the handle is
    styled through the indicator's foreground.

    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml scrollBar
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::scrollIndicator

    Grouped property for styling \l [QtQuickControls]{ScrollIndicator}.

    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml scrollIndicator
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::scrollView

    Grouped property for styling \l [QtQuickControls]{ScrollView}.

    ScrollView itself has no visual delegates to style. Its scroll bars can be
    styled separately through the \l scrollBar property. But you can use
    \l {ControlStateStyle::padding}{padding} to control the space between
    the scroll bars and the content area.

    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml scrollView
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::slider

    Grouped property for styling \l [QtQuickControls]{Slider}.
    For a slider bar, the groove is styled through the indicator, while the progress
    track is styled through the indicator's foreground.
    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml slider
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::spinBox

    Grouped property for styling \l [QtQuickControls]{SpinBox}.

    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml spinBox

    \note It's currently only possible to position the up and down buttons to
    be on the left or right side of the control, but not on top of each other.
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::switchControl

    Grouped property for styling \l [QtQuickControls]{Switch}.

    Unset properties fall back to \l abstractButton.

    \snippet ControlsSnippets.qml switchControl
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::tabBar

    Grouped property for styling \l [QtQuickControls]{TabBar}.

    Unset properties fall back to \l pane.

    \snippet ControlsSnippets.qml tabBar
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::tabButton

    Grouped property for styling \l [QtQuickControls]{TabButton}.

    Unset properties fall back to \l abstractButton.

    \snippet ControlsSnippets.qml tabButton
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::textArea

    Grouped property for styling \l [QtQuickControls]{TextArea}.

    Unset properties fall back to \l textInput.

    \snippet ControlsSnippets.qml textArea
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::textField

    Grouped property for styling \l [QtQuickControls]{TextField}.

    Unset properties fall back to \l textInput.

    \snippet ControlsSnippets.qml textField
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::textInput

    Grouped property for styling all text input controls, including
    \l [QtQuickControls]{TextField} and \l [QtQuickControls]{TextArea}.

    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml textInput
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::toolBar

    Grouped property for styling \l [QtQuickControls]{ToolBar}.

    Unset properties fall back to \l pane.

    \snippet ControlsSnippets.qml toolBar
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::toolButton

    Grouped property for styling \l [QtQuickControls]{ToolButton}.

    Unset properties fall back to \l abstractButton.

    \snippet ControlsSnippets.qml toolButton
*/

/*!
    \qmlproperty ControlStyle AbstractStylableControls::toolSeparator

    Grouped property for styling \l [QtQuickControls]{ToolSeparator}.

    Unset properties fall back to \l control.

    \snippet ControlsSnippets.qml toolSeparator
*/

using namespace Qt::StringLiterals;

QQStyleKitControls::QQStyleKitControls(QObject *parent)
    : QObject(parent)
{
}

QQmlListProperty<QObject> QQStyleKitControls::data()
{
    return QQmlListProperty<QObject>(this, &m_data);
}

const QList<QObject *> QQStyleKitControls::children() const
{
    return m_data;
}

/* Lazy-create the controls that the style is actually using, when accessed
 * them from the style/application (e.g from Style or Theme). We don't lazy
 * create any controls while resolving style properties, as undefined controls would
 * anyway not contain any property overrides. The properties have setters too, to
 * allow the style/application to share custom ControlStyle the classical
 * way, e.g button: ControlStyle { id: button }. */
QQStyleKitControl* QQStyleKitControls::getControl(QQStyleKitExtendableControlType controlType) const
{
    if (!m_controls.contains(controlType))
        return nullptr;
    return m_controls[controlType];
}


QList<QQStyleKitVariation *> QQStyleKitControls::variations() const
{
    QList<QQStyleKitVariation *> list;
    for (auto *obj : children()) {
        if (auto *variation = qobject_cast<QQStyleKitVariation *>(obj))
            list.append(variation);
    }
    return list;
}

#define IMPLEMENT_ACCESSORS(NAME, TYPE) \
QQStyleKitControl *QQStyleKitControls::NAME() const \
{ \
    if (!m_controls.contains(TYPE)) { \
        auto *self = const_cast<QQStyleKitControls *>(this); \
        auto *control = new QQStyleKitControl(self); \
        self->m_controls.insert(TYPE, control); \
    } \
    return m_controls[TYPE]; \
} \
void QQStyleKitControls::set_ ## NAME(QQStyleKitControl *control) \
{ \
    m_controls.insert(TYPE, control); \
}


IMPLEMENT_ACCESSORS(abstractButton, QQStyleKitReader::ControlType::AbstractButton)
IMPLEMENT_ACCESSORS(applicationWindow, QQStyleKitReader::ControlType::ApplicationWindow)
IMPLEMENT_ACCESSORS(control, QQStyleKitReader::ControlType::Control)
IMPLEMENT_ACCESSORS(button, QQStyleKitReader::ControlType::Button)
IMPLEMENT_ACCESSORS(flatButton, QQStyleKitReader::ControlType::FlatButton)
IMPLEMENT_ACCESSORS(checkBox, QQStyleKitReader::ControlType::CheckBox)
IMPLEMENT_ACCESSORS(comboBox, QQStyleKitReader::ControlType::ComboBox)
IMPLEMENT_ACCESSORS(progressBar, QQStyleKitReader::ControlType::ProgressBar)
IMPLEMENT_ACCESSORS(scrollBar, QQStyleKitReader::ControlType::ScrollBar)
IMPLEMENT_ACCESSORS(scrollIndicator, QQStyleKitReader::ControlType::ScrollIndicator)
IMPLEMENT_ACCESSORS(scrollView, QQStyleKitReader::ControlType::ScrollView)
IMPLEMENT_ACCESSORS(slider, QQStyleKitReader::ControlType::Slider)
IMPLEMENT_ACCESSORS(spinBox, QQStyleKitReader::ControlType::SpinBox)
IMPLEMENT_ACCESSORS(switchControl, QQStyleKitReader::ControlType::SwitchControl)
IMPLEMENT_ACCESSORS(tabBar, QQStyleKitReader::ControlType::TabBar)
IMPLEMENT_ACCESSORS(tabButton, QQStyleKitReader::ControlType::TabButton)
IMPLEMENT_ACCESSORS(textField, QQStyleKitReader::ControlType::TextField)
IMPLEMENT_ACCESSORS(textInput, QQStyleKitReader::ControlType::TextInput)
IMPLEMENT_ACCESSORS(toolBar, QQStyleKitReader::ControlType::ToolBar)
IMPLEMENT_ACCESSORS(toolButton, QQStyleKitReader::ControlType::ToolButton)
IMPLEMENT_ACCESSORS(toolSeparator, QQStyleKitReader::ControlType::ToolSeparator)
IMPLEMENT_ACCESSORS(radioButton, QQStyleKitReader::ControlType::RadioButton)
IMPLEMENT_ACCESSORS(itemDelegate, QQStyleKitReader::ControlType::ItemDelegate)
IMPLEMENT_ACCESSORS(popup, QQStyleKitReader::ControlType::Popup)
IMPLEMENT_ACCESSORS(pane, QQStyleKitReader::ControlType::Pane)
IMPLEMENT_ACCESSORS(page, QQStyleKitReader::ControlType::Page)
IMPLEMENT_ACCESSORS(frame, QQStyleKitReader::ControlType::Frame)
IMPLEMENT_ACCESSORS(label, QQStyleKitReader::ControlType::Label)
IMPLEMENT_ACCESSORS(groupBox, QQStyleKitReader::ControlType::GroupBox)
IMPLEMENT_ACCESSORS(textArea, QQStyleKitReader::ControlType::TextArea)

#undef IMPLEMENT_ACCESSORS

void QQStyleKitControls::componentComplete()
{
    for (auto *obj : children()) {
        if (auto *customControl = qobject_cast<QQStyleKitCustomControl *>(obj)) {
            const QQStyleKitExtendableControlType type = customControl->controlType();
            const QQStyleKitExtendableControlType reserved
                = QQStyleKitExtendableControlType(QQStyleKitReader::ControlType::Unspecified);
            if (type >= reserved)
                qmlWarning(this) << "CustomControls must use a controlType less than " << reserved;
            if (m_controls.contains(type))
                qmlWarning(this) << "CustomControl registered more than once: " << type;
            m_controls.insert(type, customControl);
        }
    }
}

QT_END_NAMESPACE

#include "moc_qqstylekitcontrols_p.cpp"
