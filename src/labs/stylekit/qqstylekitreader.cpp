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

using namespace Qt::StringLiterals;

static const QString kAlternate1 = "A1"_L1;
static const QString kAlternate2 = "A2"_L1;

static QFont resolvedFontWithOverrides(const QQStyleKitReader *reader, const QFont &baseFont)
{
    Q_ASSERT(reader);
    QFont font = baseFont;
    const QQStyleKitTextProperties *textProps = reader->global()->text();
    if (!textProps)
        return font;
    if (textProps->isDefined(QQSK::Property::Bold))
        font.setBold(textProps->styleProperty<bool>(QQSK::Property::Bold));
    if (textProps->isDefined(QQSK::Property::Italic))
        font.setItalic(textProps->styleProperty<bool>(QQSK::Property::Italic));
    if (textProps->isDefined(QQSK::Property::PointSize))
        font.setPointSizeF(textProps->styleProperty<qreal>(QQSK::Property::PointSize));
    return font;
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

    setFont(resolvedFontWithOverrides(this, style->fontForReader(this)));
}

void QQStyleKitReader::resetAll()
{
    for (QQStyleKitReader *reader : s_allReaders) {
        reader->m_effectiveVariationsDirty = true;
        reader->clearLocalStorage();
        reader->emitChangedForAllStyleProperties();
        reader->updateFontFromTheme();
    }
}

void QQStyleKitReader::updateFontFromTheme()
{
    const QQStyleKitStyle *style = QQStyleKitStyle::current();
    if (!style || !style->loaded())
        return;

    setFont(resolvedFontWithOverrides(this, style->fontForReader(this)));
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

QQStyleKitExtendableControlType QQStyleKitReader::type() const
{
    return m_type;
}

void QQStyleKitReader::setType(QQStyleKitExtendableControlType type)
{
    if (m_type == type)
        return;

    m_type = type;
    populateLocalStorage();
    emit typeChanged();
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
    return &const_cast<QQStyleKitReader *>(this)->m_palette;
}

void QQStyleKitReader::setPalette(QQuickPalette *palette)
{
    if (palette && m_palette.toQPalette() == palette->toQPalette())
        return;

    m_palette.reset();
    if (palette)
        m_palette.inheritPalette(palette->toQPalette());
    emit paletteChanged();

    const auto *stylePtr = style();
    if (!stylePtr || !stylePtr->loaded())
        return;

    clearLocalStorage();
    emitChangedForAllStyleProperties();
}

QFont QQStyleKitReader::font() const
{
    return m_font;
}

void QQStyleKitReader::setFont(const QFont &font)
{
    if (m_font == font)
        return;

    m_font = font;
    emit fontChanged();
}

QQStyleKitControlProperties *QQStyleKitReader::global() const
{
    return &const_cast<QQStyleKitReader *>(this)->m_global;
}

QT_END_NAMESPACE

#include "moc_qqstylekitreader_p.cpp"
