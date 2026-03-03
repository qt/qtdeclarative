// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitcontrol_p.h"
#include "qqstylekitcontrols_p.h"
#include "qqstylekitpropertyresolver_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype ControlStyle
    \inqmlmodule Qt.labs.StyleKit
    \inherits ControlStateStyle
    \brief Defines the style for a control in the \c normal state

    A ControlStyle describes how a \l Control should be styled. Its API
    largely mirrors that of a Qt Quick Control: it provides grouped
    properties for delegates such as
    \l {ControlState::background}{background},
    \l {ControlState::indicator}{indicator},
    \l {ControlState::handle}{handle}, and
    \l {ControlState::text}{text}, along with layout
    properties such as
    \l {ControlState::padding}{padding} and
    \l {ControlState::spacing}{spacing}.
    If you are familiar with the API of a \l Control in Qt Quick Controls,
    you should find the ControlStyle API easy to follow.

    ControlStyle inherits \l ControlStateStyle because it represents the
    \e normal state: properties set directly on a ControlStyle describe
    how the control looks when no other state is active. State-specific
    overrides are set through nested states, such as \l {ControlStateStyle::}{hovered}
    \l {ControlStateStyle::}{pressed}, and \l {ControlStateStyle::}{checked}.

    \l {AbstractStylableControls}{Each stylable control} in a \l Style, \l Theme, or \l StyleVariation is a ControlStyle.
    For example, in the snippet below, \l {AbstractStylableControls::}{control},
    \l {AbstractStylableControls::}{button} and \l {AbstractStylableControls::}{radioButton}
    are all ControlStyles:

    \snippet ControlStyleSnippets.qml ControlStyle

    \sa {AbstractStylableControls}{All stylable controls}, Style, Theme,
        StyleVariation, ControlStateStyle, DelegateStyle
*/

/*!
    \qmlproperty list<StyleVariation> ControlStyle::variations

    A list of \l {StyleVariation}{type variations} for this control type.

    A type variation provides alternate styling for controls that are
    children (or descendants) of this control type. For example, you
    can use it to style all \l {Button}{buttons} inside a \l {Frame}{frame}
    differently from buttons elsewhere:

    \snippet TypeVariationSnippets.qml frame with variation

    Unlike instance variations — which are applied to specific control
    instances from the application via the
    \l {StyleVariation.variations} attached property — type
    variations are applied to \e{all} instances of a control type from
    the \l Style, without requiring the application to opt in.

    \sa StyleVariation
*/

QQStyleKitControl::QQStyleKitControl(QObject *parent)
    : QQStyleKitControlState(parent)
{
}

QQmlListProperty<QQStyleKitVariation> QQStyleKitControl::variations()
{
    const bool isWrite = subclass() == QQSK::Subclass::QQStyleKitState;
    if (isWrite) {
        if (!QQStyleKitPropertyResolver::hasLocalStyleProperty(this, QQSK::Property::Variations)) {
            /* We need to handle both the setter and the getter in the getter, since QML doesn't
             * use a separate setter for QQmlListProperty. This means that we need to initialize
             * the storage with an empty QList, since we need to be prepared for the QML engine
             * inserting elements into it behind our back. A negative side-effect of this logic
             * is that a simple read of the property for a QQStyleKitState will also accidentally
             * populate the local storage of that QQStylKitState, and thereby affect propagation.
             * Note: since Variations is not a part of QQStyleKitProperties, it's not possible,
             * and there is also no point, in emitting a changed signal globally, since a
             * QQuickStyleKitReader inherits QQStyleKitProperties and not QQStyleKitControl,
             * and as such, doesn't have a 'variations' property. */
            auto *newList = new QList<QQStyleKitVariation *>();
            setStyleProperty(QQSK::Property::Variations, newList);
        }
    }

    /* Read the property, taking propagation into account. Note that since QQmlListProperty takes
     * a pointer to a QList as argument, we need to store the list as a pointer as well */
    const QVariant variant = QQStyleKitPropertyResolver::readStyleProperty(this, QQSK::Property::Variations);
    if (!variant.isValid()) {
        // Return a read-only list. Trying to change this list from the app has no effect.
        static auto *emptyList = new QList<QQStyleKitVariation *>();
        return QQmlListProperty<QQStyleKitVariation>(this, emptyList);
    }

    const auto value = qvariant_cast<QList<QQStyleKitVariation *> *>(variant);
    return QQmlListProperty<QQStyleKitVariation>(this, value);
}

QVariant QQStyleKitControl::readStyleProperty(PropertyStorageId key) const
{
    return m_storage.value(key);
}

void QQStyleKitControl::writeStyleProperty(PropertyStorageId key, const QVariant &value)
{
    m_storage.insert(key, value);
}

QQStyleKitControls *QQStyleKitControl::controls() const
{
    Q_ASSERT(qobject_cast<QQStyleKitControls *>(parent()));
    return static_cast<QQStyleKitControls *>(parent());
}

QQStyleKitExtendableControlType QQStyleKitControl::controlType() const
{
    const auto &controlsMap = controls()->m_controls;
    Q_ASSERT(std::find(controlsMap.begin(), controlsMap.end(), this) != controlsMap.end());
    return controlsMap.key(const_cast<QQStyleKitControl *>(this));
}

QT_END_NAMESPACE

#include "moc_qqstylekitcontrol_p.cpp"
