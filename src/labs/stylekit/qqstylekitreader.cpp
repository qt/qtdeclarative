// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuick/private/qquickpalette_p.h>
#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickstategroup_p.h>
#include <QtQuick/private/qquickpropertychanges_p.h>

#include "qqstylekit_p.h"
#include "qqstylekitreader_p.h"
#include "qqstylekitpropertyresolver_p.h"
#include "qqstylekitstyle_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype StyleReader
    \inqmlmodule Qt.labs.StyleKit
    \inherits ControlStyleProperties
    \brief Reads properties from the active Style for a specific control.

    StyleReader is the bridge between a control in \l {Qt Quick Controls}
    and the \l {StyleKit::style}{active style}. It exposes all the style
    properties — \l {ControlStyleProperties::background}{backgrounds},
    \l {ControlStyleProperties::indicator}{indicators},
    \l {ControlStyleProperties::handle}{handles},
    \l {ControlStyleProperties::text}{text},
    \l {ControlStyleProperties::padding}{padding},
    and more — that a control, and its delegates, should bind to.
    All built-in StyleKit controls use a StyleReader internally for this purpose.

    The state properties — \l hovered, \l pressed, \l focused, \l checked,
    \l highlighted, etc. — tell StyleReader which state the control is in.
    It uses these to resolve and read the correct property values from
    the \l Style, taking \l {Theme}{Themes}, \l {StyleVariation}{StyleVariations},
    \l {AbstractStylableControls}{fallback types}, and property propagation
    into account.

    When implementing a \l CustomControl, you can follow the same approach as
    the built-in controls:

    \snippet StyleReaderSnippets.qml custom control

    \note The style properties inherited from \l ControlStyleProperties do not map
    directly to the \l Style. Instead, they reflect a cache of potentially interpolated
    values during an active \l {ControlStyle::}{transition}. Writing to them will \e not
    update the corresponding properties in the \l Style and may interfere with ongoing
    transitions. Write directly to the \l Style when you need to change a style property.

    \labs

    \sa CustomControl, Style, {StyleVariation::controlType}{StyleVariation.controlType}
*/

/*!
    \qmlproperty enumeration StyleReader::controlType

    Identifies which control type in the \l Style this reader reads
    properties from. This can either be set to one of the predefined values
    below, or to a \l {CustomControl::controlType}{custom control type} defined in the
    \l Style.

    \value StyleReader.Button         \l {Button}
    \value StyleReader.CheckBox       \l {CheckBox}
    \value StyleReader.ComboBox       \l {ComboBox}
    \value StyleReader.Frame          \l {Frame}
    \value StyleReader.GroupBox       \l {GroupBox}
    \value StyleReader.ItemDelegate   \l {ItemDelegate}
    \value StyleReader.Label          \l {Label}
    \value StyleReader.Menu           \l {Menu}
    \value StyleReader.Dialog         \l {Dialog}
    \value StyleReader.Page           \l {Page}
    \value StyleReader.Pane           \l {Pane}
    \value StyleReader.Popup          \l {Popup}
    \value StyleReader.ProgressBar    \l {ProgressBar}
    \value StyleReader.RadioButton    \l {RadioButton}
    \value StyleReader.ScrollBar      \l {ScrollBar}
    \value StyleReader.ScrollView     \l {ScrollView}
    \value StyleReader.SearchField    \l {SearchField}
    \value StyleReader.Slider         \l {Slider}
    \value StyleReader.SpinBox        \l {SpinBox}
    \value StyleReader.Switch         \l {Switch}
    \value StyleReader.TabBar         \l {TabBar}
    \value StyleReader.TabButton      \l {TabButton}
    \value StyleReader.TextArea       \l {TextArea}
    \value StyleReader.TextField      \l {TextField}
    \value StyleReader.ToolBar        \l {ToolBar}
    \value StyleReader.ToolButton     \l {ToolButton}
    \value StyleReader.ToolSeparator  \l {ToolSeparator}

    \sa {StyleVariation::controlType}{StyleVariation.controlType}
*/

/*!
    \qmlproperty bool StyleReader::enabled

    Whether the control is enabled.

    Bind this to the control's \l {Item::enabled}{enabled} property.

    The default value is \c true.

    \sa {ControlStyleState::enabled}{ControlStyleState.disabled}
*/

/*!
    \qmlproperty bool StyleReader::hovered

    Whether the control is hovered.

    Bind this to the control's \l {Control::hovered}{hovered} property.

    The default value is \c false.

    \sa {ControlStateStyle::hovered}{ControlStateStyle.hovered}
*/

/*!
    \qmlproperty bool StyleReader::pressed

    Whether the control is pressed.

    Bind this to the control's \l {AbstractButton::pressed}{pressed} property.

    The default value is \c false.

    \sa {ControlStateStyle::pressed}{ControlStateStyle.pressed}
*/

/*!
    \qmlproperty bool StyleReader::focused

    Whether the control has active focus.

    Bind this to the control's \l {Item::activeFocus}{activeFocus} property.

    The default value is \c false.

    \sa {ControlStateStyle::focused}{ControlStateStyle.focused}
*/

/*!
    \qmlproperty bool StyleReader::checked

    Whether the control is checked.

    Bind this to the control's \l {AbstractButton::checked}{checked} property.

    The default value is \c false.

    \sa {ControlStateStyle::checked}{ControlStateStyle.checked}
*/

/*!
    \qmlproperty bool StyleReader::highlighted

    Whether the control is highlighted.

    Bind this to the control's \l {Button::highlighted}{highlighted} property.

    The default value is \c false.

    \sa {ControlStateStyle::highlighted}{ControlStateStyle.highlighted}
*/

/*!
    \qmlproperty bool StyleReader::vertical

    Whether the control is oriented vertically.

    Bind this to the control's \l {Slider::orientation}{orientation}.

    The default value is \c false.

    \sa {ControlStateStyle::vertical}{ControlStateStyle.vertical}
*/

/*!
    \qmlproperty palette StyleReader::palette

    The palette of the control. StyleKit uses this to resolve color properties
    that bind the \l {Style::}{palette} in the \l Style.

    Bind this to the control's \l {Control::palette}{palette} property.

    \sa {Style::palette}{Style.palette}
*/

/*!
    \qmlproperty font StyleReader::font
    \readonly

    The effective font for this control type, as defined by the
    \l {AbstractStyle::fonts}{style}. This also takes into account any font
    overrides set in the \l {ControlStyle::}{text} properties of the style.

    Bind the control's \l {Control::font}{font} property to this property.

    \note Unlike properties such as \l hovered, \l pressed and \l palette — which
    is forwarded from the control to the StyleReader — \l font is an output. Bind the
    control's \l {Control::font}{font} property to this value, not the other way around.
*/

/*!
    \qmlproperty ControlProperties StyleReader::global
    \readonly

    Provides direct access to the style properties, bypassing any ongoing
    \l {ControlStyle::}{transition}.

    While a state transition is ongoing, style properties read from a StyleReader
    may return interpolated values. By prepending \c global to the property path,
    you bypass the transition and get the end-state values immediately.

    For example, when transitioning from \l hovered to \l pressed, \c {background.color}
    may return an interpolated value between the \l {DelegateStyle::}{color} in the
    \l {ControlStateStyle::}{hovered} state and the color in the
    \l {ControlStateStyle::}{pressed} state. \c {global.background.color}, on the
    other hand, returns the color in the pressed state directly.

    \sa {StyleKit::transitionsEnabled}{transitionsEnabled}
*/

using namespace Qt::StringLiterals;

static const QString kAlternate1 = "A1"_L1;
static const QString kAlternate2 = "A2"_L1;

static quint64 textFontOverridesSignature(const QQStyleKitTextProperties *t)
{
    if (!t)
        return 0;

    quint64 sig = 0;

    // bit 0: bold defined, bit 1: bold value
    if (t->isDefined(QQSK::Property::Bold)) {
        sig |= (quint64(1) << 0);
        if (t->styleProperty<bool>(QQSK::Property::Bold))
            sig |= (quint64(1) << 1);
    }
    // bit 2: italic defined, bit 3: italic value
    if (t->isDefined(QQSK::Property::Italic)) {
        sig |= (quint64(1) << 2);
        if (t->styleProperty<bool>(QQSK::Property::Italic))
            sig |= (quint64(1) << 3);
    }
    // bit 4: pointSize defined, bits 5..: quantized pointSize
    if (t->isDefined(QQSK::Property::PointSize)) {
        sig |= (quint64(1) << 4);
        const qreal ps = t->styleProperty<qreal>(QQSK::Property::PointSize);
        // 64-bit signature: 5 bits used, 59 bits available for pointSize
        constexpr int payloadBits = 64 - 5;
        const qint64 maxQ = (quint64(1) << payloadBits) - 1;
        const quint64 q = quint64(qBound<qint64>(0, qRound64(ps * 64.0), maxQ));
        sig |= (q << 5);
    }
    return sig;
}

QList<QQStyleKitReader *> QQStyleKitReader::s_allReaders;
QMap<QString, QQmlComponent *> QQStyleKitReader::s_propertyChangesComponents;



QQStyleKitReader::QQStyleKitReader(QObject *parent)
    : QQStyleKitControlProperties(QQSK::PropertyGroup::Control, parent)
    , m_dontEmitChangedSignals(false)
    , m_effectiveVariationsDirty(true)
    , m_global(QQStyleKitControlProperties(QQSK::PropertyGroup::GlobalFlag, this))
{
    s_allReaders.append(this);
}

QQStyleKitReader::~QQStyleKitReader()
{
    s_allReaders.removeOne(this);
}

QQuickStateGroup *QQStyleKitReader::stateGroup()
{
    if (m_stateGroup)
        return m_stateGroup;

    /* Lazy create a StyleKitReaderStateGroup as soon as we have delegates
     * that needs to be "tracked". That is, the user of this StyleKitReader
     * has read one or more properties, so we need to check and emit changes for
     * those properties whenever our state changes. */
    const auto *stylePtr = style();
    Q_ASSERT(stylePtr);

    m_stateGroup = new QQuickStateGroup(this);

    // Add two states that we can alternate between
    auto statesProp = m_stateGroup->statesProperty();
    QQuickState *alternate1 = new QQuickState(m_stateGroup);
    QQuickState *alternate2 = new QQuickState(m_stateGroup);
    alternate1->setName(kAlternate1);
    alternate2->setName(kAlternate2);
    m_stateGroup->statesProperty().append(&statesProp, alternate1);
    m_stateGroup->statesProperty().append(&statesProp, alternate2);

    QQmlComponent *controlComp = createControlChangesComponent();
    instantiatePropertyChanges(controlComp);

    return m_stateGroup;
}

QQmlComponent *QQStyleKitReader::createControlChangesComponent() const
{
    static const QLatin1String propertyName("control"_L1);
    if (s_propertyChangesComponents.contains(propertyName))
        return s_propertyChangesComponents.value(propertyName);

    const QString qmlControlCode = QString::fromUtf8(R"(
    import QtQuick
    PropertyChanges {
        spacing: global.spacing
        padding: global.padding
        leftPadding: global.leftPadding
        rightPadding: global.rightPadding
        topPadding: global.topPadding
        bottomPadding: global.bottomPadding
        text.color: global.text.color
        text.alignment: global.text.alignment
        text.bold: global.text.bold
        text.italic: global.text.italic
        text.pointSize: global.text.pointSize
        text.padding: global.text.padding
        text.leftPadding: global.text.leftPadding
        text.rightPadding: global.text.rightPadding
        text.topPadding: global.text.topPadding
        text.bottomPadding: global.text.bottomPadding
    }
    )");

    // TODO: cache propertyName to component!
    QQmlComponent *component = new QQmlComponent(qmlEngine(style()));
    component->setData(qmlControlCode.toUtf8(), QUrl());
    Q_ASSERT_X(!component->isError(), __FUNCTION__, component->errorString().toUtf8().constData());
    s_propertyChangesComponents.insert(propertyName, component);
    return component;
}

QQmlComponent *QQStyleKitReader::createDelegateChangesComponent(const QString &delegateName) const
{
    if (s_propertyChangesComponents.contains(delegateName))
        return s_propertyChangesComponents.value(delegateName);

    static const QString qmlTemplateCode = QString::fromUtf8(R"(
    import QtQuick
    PropertyChanges { $ {
        implicitWidth: global.$.implicitWidth
        implicitHeight: global.$.implicitHeight
        visible: global.$.visible
        color: global.$.color
        gradient: global.$.gradient
        radius: global.$.radius
        topLeftRadius: global.$.topLeftRadius
        topRightRadius: global.$.topRightRadius
        bottomLeftRadius: global.$.bottomLeftRadius
        bottomRightRadius: global.$.bottomRightRadius
        margins: global.$.margins
        alignment: global.$.alignment
        leftMargin: global.$.leftMargin
        rightMargin: global.$.rightMargin
        topMargin: global.$.topMargin
        bottomMargin: global.$.bottomMargin
        scale: global.$.scale
        rotation: global.$.rotation
        opacity: global.$.opacity
        border.color: global.$.border.color
        border.width: global.$.border.width
        shadow.color: global.$.shadow.color
        shadow.scale: global.$.shadow.scale
        shadow.blur: global.$.shadow.blur
        shadow.visible: global.$.shadow.visible
        shadow.opacity: global.$.shadow.opacity
        shadow.verticalOffset: global.$.shadow.verticalOffset
        shadow.horizontalOffset: global.$.shadow.horizontalOffset
        shadow.delegate: global.$.shadow.delegate
        image.source: global.$.image.source
        image.color: global.$.image.color
        image.fillMode: global.$.image.fillMode
        delegate: global.$.delegate
        data: global.$.data
    }}
    )");

    QString substitutedCode = qmlTemplateCode;
    substitutedCode.replace('$'_L1, delegateName);
    QQmlComponent *component = new QQmlComponent(qmlEngine(style()));
    component->setData(substitutedCode.toUtf8(), QUrl());
    Q_ASSERT_X(!component->isError(), __FUNCTION__, component->errorString().toUtf8().constData());
    s_propertyChangesComponents.insert(delegateName, component);
    return component;
}

void QQStyleKitReader::instantiatePropertyChanges(QQmlComponent *comp)
{
    QObject *obj = comp->create(qmlContext(this));
    auto *propertyChanges = qobject_cast<QQuickPropertyChanges *>(obj);
    Q_ASSERT(propertyChanges);

    // setter for the "target" property is called setObject
    propertyChanges->setObject(this);
    /* set "explicit" to true, meaning that the StyleProperties shouldn't
     * create bindings, but do one-off assignments. Bindings are not needed
     * here since it's the state changes of this StyleKitReader that
     * drives the property changes. The properties cannot change outside of
     * a state change (or, if they do, it will trigger a full update equal
     * to a theme change) */
    propertyChanges->setIsExplicit(true);
    /* We don't need to ever restore the properties back to default, since
     * the group state will never be reset back to an empty string. This will
     * hopefully avoid the generation of restore structures inside the StateGroup. */
    propertyChanges->setRestoreEntryValues(false);

    /* Add the new PropertyChanges object to both states, A1 and A2 */
    for (QQuickState *state : stateGroup()->states()) {
        auto changesProp = state->changes();
        changesProp.append(&changesProp, propertyChanges);
    }
}

void QQStyleKitReader::maybeTrackDelegates()
{
    forEachUsedDelegate(
        [this](QQStyleKitDelegateProperties *delegate, QQSK::Delegate type, const QString &delegatePath){
            if (m_trackedDelegates.testFlag(type)) {
                // We're already tracking the delegate. So nothing needs to be done.
                return;
            }
            if (!delegate->visible()) {
                /* As an optimization, if the delegate is hidden, we don't track it. Most
                 * controls set background.visible to false, for example. If this, for
                 * whatever reason, is not wanted, set opacity to 0 instead. */
                return;
            }
            /* Invariant: The application has read one or more properties for the given delegate
             * from the Style, but we don't yet have a PropertyChanges object that can track
             * changes to it (and run transitions). So we create one now. By lazy creating them this
             * way, we avoid creating PropertyChanges for all the different delegates that a control
             * _may_ have. Instead, we only track changes for the delegates it actually uses. */
            m_trackedDelegates.setFlag(type);
            QQmlComponent *comp = createDelegateChangesComponent(delegatePath);
            instantiatePropertyChanges(comp);
        });
}

void QQStyleKitReader::updateControl()
{
    const QQStyleKitStyle *style = QQStyleKitStyle::current();
    if (!style || !style->loaded())
        return;

    /* Alternate between two states to trigger a state change. The state group
     * will, upon changing state, take care of reading the updated property values,
     * compare them against the current ones in the local storage, and emit changes
     * (possibly using a transition) if changed. Since the new state might change the
     * transition, we need to update it first before we do the state change, so that
     * it takes effect.
     * If we have skipped tracking some delegates because they are hidden, we need to
     * check again if this is still the case for the current state. Otherwise, we now
     * need to track them. Untracked delegates are not backed by PropertyChanges objects,
     * and hence, will not update when we do a state swap below.
     * Note that the first time this function is called after start-up, none of the
     * delegates are yet tracked, and therefore will be created now. */

    maybeTrackDelegates();

    auto transitionProp = stateGroup()->transitionsProperty();
    const int transitionCountInStateGroup = transitionProp.count(&transitionProp);
    const bool enabled = QQStyleKit::qmlAttachedProperties()->transitionsEnabled();
    QQuickTransition *transitionInStyle = enabled ? transition() : nullptr;
    QQuickTransition *transitionInStateGroup =
        transitionCountInStateGroup > 0 ? transitionProp.at(&transitionProp, 0) : nullptr;
    if (transitionInStyle != transitionInStateGroup) {
        transitionProp.clear(&transitionProp);
        if (transitionInStyle)
            transitionProp.append(&transitionProp, transitionInStyle);
    }

    switch (m_alternateState) {
    case AlternateState::Alternate1:
        m_alternateState = AlternateState::Alternate2;
        stateGroup()->setState(kAlternate2);
        break;
    case AlternateState::Alternate2:
        m_alternateState = AlternateState::Alternate1;
        stateGroup()->setState(kAlternate1);
        break;
    default:
        Q_UNREACHABLE();
    }

    auto textOverrideSig = textFontOverridesSignature(global()->text());
    if (m_lastTextFontOverridesSignature != textOverrideSig)
        m_fontDirty = true;
    m_lastTextFontOverridesSignature = textOverrideSig;
    rebuildEffectiveFont();
}

void QQStyleKitReader::resetAll()
{
    for (QQStyleKitReader *reader : s_allReaders) {
        reader->m_effectiveVariationsDirty = true;
        reader->m_fontDirty = true;
        reader->clearLocalStorage();
        reader->rebuildEffectivePalette();
        reader->rebuildEffectiveFont();
        reader->emitChangedForAllStyleProperties(EmitFlag::AllProperties);
    }
}

void QQStyleKitReader::populateLocalStorage()
{
    if (!m_storage.isEmpty())
        return;
    const auto *stylePtr = style();
    if (!stylePtr || !stylePtr->loaded())
        return;

    /* The local storage is empty, which is typically the case after an
     * operation that should perform without a transition, such as a theme
     * change or a change to a property value in the Style itself.
     * Doing a transition in that case is unwanted and slow (since the
     * operation typically affect all controls), so we short-cut the process by
     * emitting changed signals directly for all the properties instead.
     * Since that will render the local storage out-of-sync, we clear it at the
     * same time to signal that it's 'dirty'. Which is why we need to sync it back
     * up again now.
     * Syncing the local storage before changing state (even if the values that
     * end up in the storage should be exactly the same as those already
     * showing), has the upshot that we can compare after the state change which
     * properties has changed, which will limit the amount of changed signals we
     * then need to emit. */
    m_dontEmitChangedSignals = true;
    updateControl();
    m_dontEmitChangedSignals = false;
}

void QQStyleKitReader::clearLocalStorage()
{
    /* Clear all the local property overrides that has been set on this reader. Such
     * overrides are typically interpolated values set by a transition during a state
     * change. By clearing them, the controls will end up reading the property values
     * directly from the Style instead. */
    m_storage.clear();
}

QQSK::State QQStyleKitReader::controlState() const
{
    QQSK::State effectiveState = m_state;

    if (!enabled()) {
        // Some states are not valid if the control is disabled
        effectiveState &= ~(QQSK::StateFlag::Pressed |
                          QQSK::StateFlag::Hovered |
                          QQSK::StateFlag::Highlighted |
                          QQSK::StateFlag::Focused |
                          QQSK::StateFlag::Hovered);
    }

    if (effectiveState == QQSK::StateFlag::Unspecified)
        effectiveState.setFlag(QQSK::StateFlag::Normal);

    return effectiveState;
}

QVariant QQStyleKitReader::readStyleProperty(PropertyStorageId key) const
{
    return m_storage.value(key);
}

void QQStyleKitReader::writeStyleProperty(PropertyStorageId key, const QVariant &value)
{
    m_storage.insert(key, value);
}

bool QQStyleKitReader::dontEmitChangedSignals() const
{
    return m_dontEmitChangedSignals;
}

QQStyleKitExtendableControlType QQStyleKitReader::controlType() const
{
    return m_type;
}

void QQStyleKitReader::setControlType(QQStyleKitExtendableControlType type)
{
    if (m_type == type)
        return;

    m_type = type;
    populateLocalStorage();
    emit controlTypeChanged();
    updateControl();
}

#ifdef QT_DEBUG
QQStyleKitReader::ControlType QQStyleKitReader::typeAsControlType() const
{
    /* Note: m_type is of type int to support extending the list
     * of possible types from the Style itself. This function
     * is here to for debugging purposes */
    return ControlType(m_type);
}
#endif

bool QQStyleKitReader::hovered() const
{
    return m_state.testFlag(QQSK::StateFlag::Hovered);
}

void QQStyleKitReader::setHovered(bool hovered)
{
    if (hovered == QQStyleKitReader::hovered())
        return;

    populateLocalStorage();
    m_state.setFlag(QQSK::StateFlag::Hovered, hovered);
    emit hoveredChanged();
    updateControl();
}

bool QQStyleKitReader::enabled() const
{
    return !m_state.testFlag(QQSK::StateFlag::Disabled);
}

void QQStyleKitReader::setEnabled(bool enabled)
{
    if (enabled == QQStyleKitReader::enabled())
        return;

    populateLocalStorage();
    m_state.setFlag(QQSK::StateFlag::Disabled, !enabled);
    emit enabledChanged();
    updateControl();
}

bool QQStyleKitReader::focused() const
{
    return m_state.testFlag(QQSK::StateFlag::Focused);
}

void QQStyleKitReader::setFocused(bool focused)
{
    if (focused == QQStyleKitReader::focused())
        return;

    populateLocalStorage();
    m_state.setFlag(QQSK::StateFlag::Focused, focused);
    emit focusedChanged();
    updateControl();
}

bool QQStyleKitReader::checked() const
{
    return m_state.testFlag(QQSK::StateFlag::Checked);
}

void QQStyleKitReader::setChecked(bool checked)
{
    if (checked == QQStyleKitReader::checked())
        return;

    populateLocalStorage();
    m_state.setFlag(QQSK::StateFlag::Checked, checked);
    emit checkedChanged();
    updateControl();
}

bool QQStyleKitReader::pressed() const
{
    return m_state.testFlag(QQSK::StateFlag::Pressed);
}

void QQStyleKitReader::setPressed(bool pressed)
{
    if (pressed == QQStyleKitReader::pressed())
        return;

    populateLocalStorage();
    m_state.setFlag(QQSK::StateFlag::Pressed, pressed);
    emit pressedChanged();
    updateControl();
}

bool QQStyleKitReader::vertical() const
{
    return m_state.testFlag(QQSK::StateFlag::Vertical);
}

void QQStyleKitReader::setVertical(bool vertical)
{
    if (vertical == QQStyleKitReader::vertical())
        return;

    populateLocalStorage();
    m_state.setFlag(QQSK::StateFlag::Vertical, vertical);
    emit verticalChanged();
    updateControl();
}

bool QQStyleKitReader::highlighted() const
{
    return m_state.testFlag(QQSK::StateFlag::Highlighted);
}

void QQStyleKitReader::setHighlighted(bool highlighted)
{
    if (highlighted == QQStyleKitReader::highlighted())
        return;

    populateLocalStorage();
    m_state.setFlag(QQSK::StateFlag::Highlighted, highlighted);
    emit highlightedChanged();
    updateControl();
}

QQuickPalette *QQStyleKitReader::palette() const
{
    return m_palette.data();
}

void QQStyleKitReader::setPalette(QQuickPalette *palette)
{
    if (m_palette == palette)
        return;

    if (m_palette)
        QObject::disconnect(m_palette, nullptr, this, nullptr);

    m_palette = palette;
    emit paletteChanged();

    if (m_palette) {
        // changed signal will be triggered when any role changes
        QObject::connect(m_palette, &QQuickPalette::changed,
                         this, &QQStyleKitReader::onPaletteChanged);
    }

    onPaletteChanged();
}

QPalette QQStyleKitReader::effectivePalette() const
{
    return m_effectivePalette;
}

void QQStyleKitReader::onPaletteChanged()
{
    const QQStyleKitStyle *style = QQStyleKitStyle::current();
    if (!style || !style->loaded())
        return;

    if (rebuildEffectivePalette()) {
        clearLocalStorage();
        emitChangedForAllStyleProperties(EmitFlag::Colors);
    }
}

bool QQStyleKitReader::rebuildEffectivePalette()
{
    auto mergedPalette = style()->paletteForControlType(this->controlType());
    const auto stylePaletteResolveMask = mergedPalette.resolveMask();
    if (m_palette) {
        // The control palette takes precedence over the style palette
        const auto controlPalette = m_palette->toQPalette();
        mergedPalette = controlPalette.resolve(mergedPalette);
        // Explicitly set the resolve mask to make sure it is not lost during the resolve operation
        // when the control palette has a resolveMask of 0
        mergedPalette.setResolveMask(stylePaletteResolveMask | controlPalette.resolveMask());
    }
    if (m_effectivePalette == mergedPalette)
        return false;

    m_effectivePalette = mergedPalette;
    return true;
}

QFont QQStyleKitReader::font() const
{
    return m_font;
}

bool QQStyleKitReader::rebuildEffectiveFont()
{
    const QQStyleKitStyle *style = QQStyleKitStyle::current();
    if (!style || !style->loaded())
        return false;

    if (!m_fontDirty)
        return false;

    QFont font = style->fontForControlType(controlType());
    const QQStyleKitTextProperties *textProps = global()->text();
    if (textProps) {
        if (textProps->isDefined(QQSK::Property::Bold))
            font.setBold(textProps->bold());
        if (textProps->isDefined(QQSK::Property::Italic))
            font.setItalic(textProps->italic());
        if (textProps->isDefined(QQSK::Property::PointSize))
            font.setPointSizeF(textProps->pointSize());
    }

    if (m_font == font)
        return false;

    m_font = font;
    emit fontChanged();
    return true;
}

QQStyleKitControlProperties *QQStyleKitReader::global() const
{
    return &const_cast<QQStyleKitReader *>(this)->m_global;
}

QT_END_NAMESPACE

#include "moc_qqstylekitreader_p.cpp"
