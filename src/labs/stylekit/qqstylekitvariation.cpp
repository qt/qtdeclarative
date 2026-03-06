// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitvariation_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype StyleVariation
    \inqmlmodule Qt.labs.StyleKit
    \inherits AbstractStylableControls
    \brief Defines alternative styling for specific controls.


    A StyleVariation lets you define alternative styling that can be applied
    to specific controls in the application (\e{instance variations}),
    or to all descendants of a particular parent control type
    (\e{type variations}). For example, you can give controls in a sidebar a
    compact appearance, or make buttons inside all \l {ToolBar}{toolbars} look
    different from buttons elsewhere.

    \section2 Instance Variations

    Instance variations are activated by setting the \l {StyleVariation::}{variations}
    attached property on any item in the application — all StyleKit controls that are
    children or descendants of that item will receive the alternative styling. Variations
    can also be set at multiple levels of the hierarchy simultaneously; when they conflict,
    the variation closest to the control takes precedence.

    Instance variations are defined in the \l Style or \l Theme that they affect:

    \snippet InstanceVariationSnippets.qml instance variations in style

    And they are applied from the application using the
    \l {StyleVariation::}{variations} attached property:

    \snippet InstanceVariationSnippets.qml apply instance variation

    Variation names referred to from the application that are not defined in the active
    style (or theme) are silently ignored — they act as hints that the style author can
    choose to implement or leave unhandled.

    \section2 Type Variations

    Type variations are assigned to the \l {ControlStyle::}{variations} property of a
    specific \l ControlStyle, without requiring a \l name. Unlike instance variations —
    which require the application to explicitly opt in — type variations are applied
    automatically to all controls of a given type whenever they are descendants of the
    specified parent control type.

    The following snippet shows how to make all \l {Button}{Buttons} inside a \l Frame look
    distinct from buttons elsewhere:

    \snippet TypeVariationSnippets.qml frame with variation

    Because \l {AbstractStylableControls::}{groupBox} falls back to
    \l {AbstractStylableControls::}{frame} in the style hierarchy, type variations
    set on \c frame are automatically inherited by \c groupBox as well. To opt out,
    reset the variations for the subtype.

    \section2 Propagation order

    A StyleVariation is local to the \l Style or \l Theme where it is defined. A Theme
    cannot shadow an entire StyleVariation defined in the Style — only individual
    properties within it can be overridden, just like any other style property.

    For example, if both the \l Style and the active \l Theme define \c {frame.variations},
    both type variation arrays take effect for a \l Frame. The resolution order for
    individual properties is: the Theme's variation properties take precedence over the
    Theme's direct properties, which take precedence over the Style's variation properties,
    which take precedence over the Style's direct properties.

    The same applies to instance variations. A StyleVariation defined in a \l Theme takes
    precedence over the Theme's direct properties. If a StyleVariation with the same
    \l name is also defined in the \l Style, both take effect, following the same
    resolution order as described above.

    In the following snippet, a button with \c {StyleVariation.variations: ["alert"]}
    will be red in the light theme and cyan in the dark theme, with a 4-pixel border in
    both. Because the dark theme overrides \c {button.background.radius} to be \c 6,
    that property takes precedence over the \c {background.radius} set in the Style's
    variation. As a result, the button's radius ends up being \c 6 in the dark theme, but \c 0
    in the light theme:

    \snippet VariationPropagation.qml propagation

    \labs

    \sa Style, Theme, {ControlStyle::variations}{ControlStyle.variations},
        {StyleVariation::variations}{StyleVariation.variations}
*/

/*!
    \qmlproperty string StyleVariation::name

    The name of this variation.

    The name identifies this variation when used as an
    \l {Instance Variations}{instance variation}.

    \sa {StyleVariation::variations}{variations} {Instance Variations}
*/

/*!
    \qmlattachedproperty list<string> StyleVariation::variations

    This property holds the list of \l{StyleVariation}{instance variations}
    to activate for a \l Control, and its descendant controls.

    If multiple variations in the list sets the same property, the first
    one in the list takes precedence.

    If variations are set on both a parent item and a descendant control, both
    sets apply, with the control's own variations taking precedence
    over those inherited from the parent.

    \snippet InstanceVariationSnippets.qml apply instance variation

    \sa name
*/

/*!
    \qmlattachedproperty int StyleVariation::controlType

    This property identifies the \l {StyleReader::controlType}{control type} of
    the item it is attached to.

    StyleKit uses it to resolve \l {Type Variations}{type variations} for descendant
    controls — if a parent item's \c controlType matches a control type that has
    \l {ControlStyle::}{variations} defined in the \l Style, those variations apply
    to all descendant controls.

    Built-in controls set this property automatically. Custom controls
    must set it explicitly to participate in type variation resolution.

    \sa {ControlStyle::variations}{ControlStyle.variations},
        {StyleReader::controlType}{StyleReader.controlType}
*/

int QQStyleKitVariation::s_typeVariationCount = 0;
int QQStyleKitVariationAttached::s_instanceVariationCount = 0;

QQStyleKitVariation::QQStyleKitVariation(QObject *parent)
    : QQStyleKitControls(parent)
{
    /* As an optimization, keep track of how many type variations that are defined
     * inside the style. That way we can skip looking for them later if s_typeVariationCount == 0. */
    ++s_typeVariationCount;
}

QQStyleKitVariationAttached *QQStyleKitVariation::qmlAttachedProperties(QObject *object)
{
    return new QQStyleKitVariationAttached(object);
}

QString QQStyleKitVariation::name() const
{
    return m_name;
}

void QQStyleKitVariation::setName(const QString &name)
{
    if (m_name == name)
        return;

    m_name = name;
    emit nameChanged();
}

QQStyleKitVariationAttached::QQStyleKitVariationAttached(QObject *parent)
    : QObject(parent)
{
}

QStringList QQStyleKitVariationAttached::variations() const
{
    return m_variations;
}

void QQStyleKitVariationAttached::setVariations(const QStringList &variations)
{
    if (m_variations == variations)
        return;

    /* As an optimization, we count the number of instance variations set from the application.
     * That way, if s_instanceVariationCount == 1, for example, and we found a variation while
     * resolving the effective variations for a specific QQStyleReader, we can stop the search. */
    s_instanceVariationCount++;

    m_variations = variations;
    emit variationsChanged();
}

QQStyleKitExtendableControlType QQStyleKitVariationAttached::controlType()
{
    return m_controlType;
}

void QQStyleKitVariationAttached::setControlType(QQStyleKitExtendableControlType type)
{
    if (m_controlType == type)
        return;

    m_controlType = type;
    emit controlTypeChanged();
}

QT_END_NAMESPACE

#include "moc_qqstylekitvariation_p.cpp"
