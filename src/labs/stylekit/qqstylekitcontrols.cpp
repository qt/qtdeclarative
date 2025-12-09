// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitcontrols_p.h"
#include "qqstylekitcontrol_p.h"
#include "qqstylekitvariation_p.h"
#include "qqstylekitcustomcontrol_p.h"

QT_BEGIN_NAMESPACE

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
 * allow the style/application to share custom StyleKitControls the classical
 * way, e.g button: StyleKitControl { id: button }. */
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
