// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitpropertyresolver_p.h"
#include "qqstylekitcontrol_p.h"
#include "qqstylekitcontrols_p.h"
#include "qqstylekitvariation_p.h"
#include "qqstylekitstyle_p.h"
#include "qqstylekittheme_p.h"
#include "qqstylekitdebug_p.h"

#include <QtQuickTemplates2/private/qquickcontrol_p.h>
#include <QtCore/QScopedValueRollback>

QT_BEGIN_NAMESPACE

bool QQStyleKitPropertyResolver::s_styleWarningsIssued = false;
bool QQStyleKitPropertyResolver::s_isReadingProperty = false;
QQSK::State QQStyleKitPropertyResolver::s_cachedState = QQSK::StateFlag::Unspecified;
QVarLengthArray<QQSK::StateFlag, 10> QQStyleKitPropertyResolver::s_cachedStateList;

const QList<QQStyleKitExtendableControlType> QQStyleKitPropertyResolver::baseTypesForType(
    QQStyleKitExtendableControlType exactType)
{
    /* By default, the base types should mirror the class hierarchy in Qt Quick Controls.
     * However, to make it possible to style a base type without having to "undo" it again
     * in a sub type, we choose to diverge in som cases:
     *
     * ItemDelegate — Normally used as a menu item in a ComboBox or as an item in a ListView.
     *     Although it behaves similarly to a button, it is typically styled very differently
     *     (e.g., without borders, drop shadows, gradients, etc.). For that reason, it falls
     *     back to Control rather than AbstractButton.
     *
     * TabBar — In Qt Quick Controls, ToolBar inherits Pane, while TabBar inherits Container.
     *     Since it is desirable for a TabBar to share styling characteristics (such as
     *     background color) with ToolBar and Pane, we let it fall back to Pane instead of
     *     Control.
     */
    switch (exactType) {
    case QQStyleKitReader::ApplicationWindow: {
        static QList<QQStyleKitExtendableControlType> t =
            { QQStyleKitReader::ApplicationWindow };
        return t; }
    case QQStyleKitReader::Button:
    case QQStyleKitReader::FlatButton:
    case QQStyleKitReader::ToolButton:
    case QQStyleKitReader::TabButton:
    case QQStyleKitReader::RadioButton:
    case QQStyleKitReader::CheckBox:
    case QQStyleKitReader::SwitchControl: {
        static QList<QQStyleKitExtendableControlType> t =
            { QQStyleKitReader::AbstractButton, QQStyleKitReader::Control };
        return t; }
    case QQStyleKitReader::Menu:
    case QQStyleKitReader::Dialog: {
        static QList<QQStyleKitExtendableControlType> t =
            { QQStyleKitReader::Popup, QQStyleKitReader::Control };
        return t; }
    case QQStyleKitReader::Page:
    case QQStyleKitReader::Frame:
    case QQStyleKitReader::TabBar:
    case QQStyleKitReader::ToolBar: {
        static QList<QQStyleKitExtendableControlType> t =
            { QQStyleKitReader::Pane, QQStyleKitReader::Control };
        return t; }
    case QQStyleKitReader::GroupBox: {
        static QList<QQStyleKitExtendableControlType> t =
            { QQStyleKitReader::Frame, QQStyleKitReader::Pane, QQStyleKitReader::Control };
        return t;
    }
    case QQStyleKitReader::TextField:
    case QQStyleKitReader::TextArea: {
        static QList<QQStyleKitExtendableControlType> t =
            { QQStyleKitReader::TextInput, QQStyleKitReader::Control };
        return t; }
    default: {
        static QList<QQStyleKitExtendableControlType> t =
            { QQStyleKitReader::Control };
        return t; }
    }

    Q_UNREACHABLE();
    return {};
}

void QQStyleKitPropertyResolver::cacheReaderState(QQSK::State state)
{
    Q_ASSERT(state != QQSK::StateFlag::Unspecified);
    if (state == s_cachedState)
        return;

    s_cachedState = state;

    /* Note: The order in which we add the states below matters.
     * The reason is that the s_cachedStateList that we build is used by QQStyleKitPropertyResolver
     * later to generate all the different state combinations that should be tested when
     * searching for a property. And the states added first to the list will "win" if the
     * same property is set in several of the states. */
    s_cachedStateList.clear();
    if (state.testFlag(QQSK::StateFlag::Pressed))
        s_cachedStateList.append(QQSK::StateFlag::Pressed);
    if (state.testFlag(QQSK::StateFlag::Hovered))
        s_cachedStateList.append(QQSK::StateFlag::Hovered);
    if (state.testFlag(QQSK::StateFlag::Highlighted))
        s_cachedStateList.append(QQSK::StateFlag::Highlighted);
    if (state.testFlag(QQSK::StateFlag::Focused))
        s_cachedStateList.append(QQSK::StateFlag::Focused);
    if (state.testFlag(QQSK::StateFlag::Checked))
        s_cachedStateList.append(QQSK::StateFlag::Checked);
    if (state.testFlag(QQSK::StateFlag::Vertical))
        s_cachedStateList.append(QQSK::StateFlag::Vertical);
    if (state.testFlag(QQSK::StateFlag::Disabled))
        s_cachedStateList.append(QQSK::StateFlag::Disabled);
}

void QQStyleKitPropertyResolver::addVariationToReader(
    QQStyleKitReader *styleReader,
    QQStyleKitStyleAndThemeBase *styleOrTheme,
    QQStyleKitVariation *variation)
{
    /* Add the variation to the StyleReader's list of effective variations.
     * A variation may appear more than once in a type variation list or in
     * an instance variation list, but it only needs to be added once - the
     * first in the list will shadow the other ones anyway. */
    if (!styleReader->m_effectiveVariations.contains(variation))
        styleReader->m_effectiveVariations.append(variation);

    /* We also record in which Style or Theme the variation was found, so that
     * it only takes effect for that specific Style or Theme during property
     * propagation. Note that the same type variation can be used from both the
     * Style and the Theme (its ID can be added to several variation lists). */
    if (!variation->m_usageContext.contains(styleOrTheme))
        variation->m_usageContext.append(styleOrTheme);
}

void QQStyleKitPropertyResolver::addTypeVariationsToReader(
    QQStyleKitReader *styleReader,
    QQStyleKitStyleAndThemeBase *styleOrTheme,
    const AttachedVariationList &attachedVariations)
{
    const QQStyleKitExtendableControlType styleReaderType = styleReader->controlType();
    const auto styleReaderBaseType = baseTypesForType(styleReaderType);

    static PropertyPathIds ids;
    if (ids.property.property() == QQSK::Property::NoProperty) {
        /* ids is made static, since the 'variations' path will be the same for all
         * StyleKitControls. Also, since subtypes are only possible for delegates,
         * and 'variations' is a control property, we can exclude subtypes. */
        ids.property = styleReader->propertyPathId(QQSK::Property::Variations, PropertyPathId::Flag::ExcludeSubtype);
        ids.alternative = styleReader->propertyPathId(QQSK::Property::NoProperty, PropertyPathId::Flag::ExcludeSubtype);
        ids.subTypeProperty = PropertyPathId();
        ids.subTypeAlternative = PropertyPathId();
    }

    for (const QQStyleKitVariationAttached *attached : attachedVariations) {
        const auto parentType = attached->controlType();
        const auto parentBaseTypes = baseTypesForType(parentType);

        /* Search for the 'variations' property set on the parentType, or any of its base
         * types, using normal propagation and fallback logic. Unlike when resolving other style
         * properties, we limit the search to the style or theme we're processing, since it matters which
         * style or theme a variation belongs when they're used to resolve other style properties later. */
        const QVariant typeVariationsVariant = readPropertyInRelevantControls(styleOrTheme, ids, parentType, parentBaseTypes);
        if (!typeVariationsVariant.isValid())
            continue;

        const auto typeVariations = *qvariant_cast<QList<QQStyleKitVariation *> *>(typeVariationsVariant);

        for (QQStyleKitVariation *variation : typeVariations) {
            /* Inside each type variation, check if the control type that styleReader represents has
             * been defined. If so, it means that the variation _might_ affect it, and should therefore
             * be added to the style readers list of effective variations. */
            if (!variation) {
                /* The variation will be nullptr if non-QQStyleKitVariation elements
                 * are added to the 'variations' list from QML (such as strings). */
                 continue;
            }

            if (variation->getControl(styleReaderType)) {
                addVariationToReader(styleReader, styleOrTheme, variation);
            } else {
                for (int type : styleReaderBaseType) {
                    if (variation->getControl(type))
                        addVariationToReader(styleReader, styleOrTheme, variation);
                }
            }
        }
    }
}

void QQStyleKitPropertyResolver::addInstanceVariationsToReader(
    QQStyleKitReader *styleReader,
    QQStyleKitStyleAndThemeBase *styleOrTheme,
    const AttachedVariationList &attachedVariations)
{
    /* Add the variations set from the application to the list of effective variations
     * in the styleReader. But, to speed up property look-up later on, we only add the
     * variations that has the potential to affect the control type, or its base types,
     * that the styleReader represents. The variations that are closest to styleReader
     * in the hierarchy will be added first and take precendence over the ones added last. */
    const QQStyleKitExtendableControlType styleReaderType = styleReader->controlType();
    const auto styleReaderBaseTypes = baseTypesForType(styleReaderType);

    for (const QQStyleKitVariationAttached *attached : attachedVariations) {
        for (const QString &instanceVariationName : attached->variations()) {
            for (QQStyleKitVariation *variation : styleOrTheme->m_styleVariations)  {
                if (variation->name() != instanceVariationName)
                    continue;

                /* Invariant: we found a variation in the given Style or a Theme with a name that matches
                 * a name in the attached variation list. Check if the found variation contains the
                 * type, or the subtypes, of the style reader. If not, it doesn't affect it and can
                 * therefore be skipped. */
                if (variation->getControl(styleReaderType)) {
                    addVariationToReader(styleReader, styleOrTheme, variation);
                } else {
                    for (int baseType : styleReaderBaseTypes) {
                        if (variation->getControl(baseType))
                            addVariationToReader(styleReader, styleOrTheme, variation);
                    }
                }
            }
        }
    }
}

void QQStyleKitPropertyResolver::rebuildVariationsForReader(
    QQStyleKitReader *styleReader, QQStyleKitStyle *style)
{
    /* Traverse up the parent chain of \a styleReader, and for each parent, look for an
     * instance of QQStyleKitVariationAttached. And for each attached object, check if it
     * has variations that can potentially affect the style reader. If so, add the
     * variations to the style readers list of effective variations.
     * A QQStyleKitVariationAttached can specify both Instance Variations and Type Variations.
     * The former should affect all descendant StyleKitReaders of the parent, while the
     * latter should only affect descendant StyleKitReaders of a specific type. */
    Q_ASSERT(styleReader->m_effectiveVariationsDirty);
    styleReader->m_effectiveVariationsDirty = false;
    styleReader->m_effectiveVariations.clear();

    if (!style->m_hasVariations)
        return;

    /* Walk up the parent chain and collect all attached StyleVariation objects that
     * may affect this StyleReader. Their variation lists affect instance variations,
     * and their control type may affect type variations. */
    AttachedVariationList attachedVariations;
    for (QObject *current = styleReader; current; current = current->parent()) {
        if (const QObject *attachedObject = qmlAttachedPropertiesObject<QQStyleKitVariation>(current, false)) {
            const auto *attached = static_cast<const QQStyleKitVariationAttached *>(attachedObject);
            attachedVariations.append(attached);
        }
    }

    if (attachedVariations.isEmpty())
        return;

    for (QQStyleKitStyle *current = style; current; current = current->fallbackStyle()) {
        if (QQStyleKitTheme *theme = current->theme()) {
            addInstanceVariationsToReader(styleReader, theme, attachedVariations);
            addTypeVariationsToReader(styleReader, theme, attachedVariations);
        }
        addInstanceVariationsToReader(styleReader, current, attachedVariations);
        addTypeVariationsToReader(styleReader, current, attachedVariations);
    }
}

template <class T>
QVariant QQStyleKitPropertyResolver::readPropertyInStorageForState(
    const PropertyPathId main, const PropertyPathId alternative,
    const T *storageProvider, QQSK::State state)
{
    /* If either the main property or its alternative is set in the storage,
     * we’ve found the best match and can return the value to the application.
     * The reason we support an alternative property is that some properties can be
     * specified in more than one way. For example, 'topLeftRadius' can be set either
     * directly ('topLeftRadius', the main property) or indirectly via 'radius'
     * (the alternative). Whichever one is encountered first in the propagation chain
     * takes precedence.
     * This means that when resolving 'topLeftRadius' for a Button, if 'radius' is set
     * on the Button and 'topLeftRadius' is set on AbstractButton, then 'radius' will
     * override 'topLeftRadius' and be used as the final value. */
    Q_ASSERT(qlonglong(state) <= qlonglong(QQSK::StateFlag::MAX_STATE));

    const PropertyStorageId propertyKey = main.storageId(state);

    if (Q_UNLIKELY(QQStyleKitDebug::enabled()))
        QQStyleKitDebug::trace(main, storageProvider, state, propertyKey);

    const QVariant propertyValue = storageProvider->readStyleProperty(propertyKey);
    if (propertyValue.isValid()) {
        if (Q_UNLIKELY(QQStyleKitDebug::enabled()))
            QQStyleKitDebug::notifyPropertyRead(main, storageProvider, state, propertyValue);
        return propertyValue;
    }

    const PropertyStorageId altPropertyKey = alternative.storageId(state);

    if (Q_UNLIKELY(QQStyleKitDebug::enabled()))
        QQStyleKitDebug::trace(alternative, storageProvider, state, altPropertyKey);

    const QVariant altValue = storageProvider->readStyleProperty(altPropertyKey);
    if (altValue.isValid()) {
        if (Q_UNLIKELY(QQStyleKitDebug::enabled()))
            QQStyleKitDebug::notifyPropertyRead(main, storageProvider, state, altValue);
        return altValue;
    }

    return {};
}

template <class INDICES_CONTAINER>
QVariant QQStyleKitPropertyResolver::readPropertyInControlForStates(
    const PropertyPathId main, const PropertyPathId alternative,
    const QQStyleKitControl *control, INDICES_CONTAINER &stateListIndices,
    int startIndex, int recursionLevel)
{
    for (int i = startIndex; i < s_cachedStateList.length(); ++i) {
        /* stateListIndices is a helper list to track which index in the state list
         * each recursion level is currently processing. The first recursion level
         * will iterate through all of the states. The second recursion level will
         * only the iterate through the states that comes after the state at the
         * previous recursion. And so on recursively. The end result will be a list of
         * state combinations (depth first) where no state is repeated more than
         * once. And for each combination, we check if the storage has a value assigned
         * for the given property for the given state combination. */
        stateListIndices[recursionLevel] = i;
        const QQSK::StateFlag stateFlag = s_cachedStateList[i];

        /* Optimization: check if the control stores values for any properties for
         * the state we're processing. Otherwise, skip the state. */
        if (!control->m_writtenStates.testFlag(stateFlag))
            continue;

        /* Optimization: check if the style/theme/variation stores a value for the
         * property in the state we're processing. Otherwise, skip the state. */
        const QQStyleKitControls *controls = control->controls();
        const QQSK::State statesAffectingProperty = controls->m_writtenPropertyPaths[main.pathId()];
        if (!statesAffectingProperty.testFlag(stateFlag)) {
            if (alternative.property() == QQSK::Property::NoProperty) {
                continue;
            } else {
                const QQSK::State statesAffectingAlternative = controls->m_writtenPropertyPaths[alternative.pathId()];
                if (!statesAffectingAlternative.testFlag(stateFlag))
                    continue;
            }
        }

        if (recursionLevel < s_cachedStateList.length() - 1) {
            // Continue the recursion towards the longest possible nested state
            const QVariant value = readPropertyInControlForStates(
                main, alternative, control, stateListIndices, i + 1, recursionLevel + 1);
            if (value.isValid())
                return value;
        }

        // Check the current combination
        QQSK::State storageState = QQSK::StateFlag::Unspecified;
        for (int j = 0; j <= recursionLevel; ++j)
            storageState.setFlag(s_cachedStateList[stateListIndices[j]]);
        const QVariant value = readPropertyInStorageForState(main, alternative, control, storageState);
        if (value.isValid())
            return value;
    }

    return {};
}

QVariant QQStyleKitPropertyResolver::readPropertyInControl(
    const PropertyPathIds &ids, const QQStyleKitControl *control)
{
    /* Find the most specific state combination (based on the state of the reader) that
     * has a value set for the property in the contol. In case several state combinations
     * could be found, the order of the states in the stateList decides the priority.
     * If we're reading a property in a subtype, try all state combinations in the subtype
     * first, before trying all the state combinations in the super type. */
    QVarLengthArray<int, 10> stateListIndices(s_cachedStateList.length());

    if (ids.subTypeProperty.property() != QQSK::Property::NoProperty) {
        if (s_cachedState != QQSK::StateFlag::Normal) {
            QVariant value = readPropertyInControlForStates(
                ids.subTypeProperty, ids.subTypeAlternative, control, stateListIndices, 0, 0);
            if (value.isValid())
                return value;
        }

        if (control->m_writtenStates.testFlag(QQSK::StateFlag::Normal)) {
            const QVariant value = readPropertyInStorageForState(
                ids.subTypeProperty, ids.subTypeAlternative, control, QQSK::StateFlag::Normal);
            if (value.isValid())
                return value;
        }
    }

    if (s_cachedState != QQSK::StateFlag::Normal) {
        const QVariant value = readPropertyInControlForStates(
            ids.property, ids.alternative, control, stateListIndices, 0, 0);
        if (value.isValid())
            return value;
    }

    /* The normal state is the propagation fall back for all state combinations.
     * If the normal state has the property set, it'll return a valid QVariant,
     * which will cause the propagation to stop. Otherwise we'll return an invalid
     * variant which will cause the search to continue. */
    if (control->m_writtenStates.testFlag(QQSK::StateFlag::Normal))
        return readPropertyInStorageForState(ids.property, ids.alternative, control, QQSK::StateFlag::Normal);

    return {};
}

QVariant QQStyleKitPropertyResolver::readPropertyInRelevantControls(
    const QQStyleKitControls *controls, const PropertyPathIds &ids,
    const QQStyleKitExtendableControlType exactType,
    const QList<QQStyleKitExtendableControlType> baseTypes)
{
    if (!controls)
        return {};

    /* Optimization: check if the style/theme/variation stores a value for
     * the property, regardless of state. Otherwise we can just return. */
    while (true) {
        const auto writtenProperties = controls->m_writtenPropertyPaths;
        if (writtenProperties.contains(ids.property.pathId()))
            break;
        const bool hasAlternative = ids.alternative.property() != QQSK::Property::NoProperty;
        if (hasAlternative && writtenProperties.contains(ids.alternative.pathId()))
            break;
        if (ids.subTypeProperty.property() == QQSK::Property::NoProperty)
            return {};
        if (writtenProperties.contains(ids.subTypeProperty.pathId()))
            break;
        if (hasAlternative && writtenProperties.contains(ids.subTypeAlternative.pathId()))
            break;
        return {};
    }

    if (const QQStyleKitControl *control = controls->getControl(exactType)) {
        const QVariant value = readPropertyInControl(ids, control);
        if (value.isValid())
            return value;
    }

    for (const int type : baseTypes) {
        if (const QQStyleKitControl *control = controls->getControl(type)) {
            const QVariant value = readPropertyInControl(ids, control);
            if (value.isValid())
                return value;
        }
    }

    return {};
}

QVariant QQStyleKitPropertyResolver::readPropertyInVariations(
    const QList<QPointer<QQStyleKitVariation>> &variations,
    const QQStyleKitStyleAndThemeBase *styleOrTheme,
    const PropertyPathIds &ids,
    const QQStyleKitExtendableControlType exactType,
    const QList<QQStyleKitExtendableControlType> baseTypes)
{
    bool foundAtLeastOneVariation = false;
    for (const QPointer<QQStyleKitVariation> &variation : variations) {
        if (!variation)
            continue;
        if (!variation->m_usageContext.contains(styleOrTheme)) {
            if (foundAtLeastOneVariation) {
                /* Optimization: We have found at least one effective variation in the list, but this
                 * one has a different usage context. This means that the remaining variations will also
                 * have a different usage context, since the variations were added to the list sorted on context.
                 * So we can end the iteration. */
                break;
            }
            /* The variations list is sorted on usage context. And the first variation in the list with
             * the correct usage context has yet to be found. So continue to the next variation. */
            continue;
        }
        foundAtLeastOneVariation = true;
        const QVariant value = readPropertyInRelevantControls(variation, ids, exactType, baseTypes);
        if (value.isValid())
            return value;
    }
    return {};
}

QVariant QQStyleKitPropertyResolver::readPropertyInStyle(
    QQStyleKitStyle *style, const PropertyPathIds &ids, QQStyleKitReader *styleReader)
{
    /* Sync the palette of the style with the palette of the current reader. Note
     * that this can cause palette bindings in the style to change, which will
     * result in calls to writeStyleProperty(). */
    style->syncFromQPalette(styleReader->effectivePalette());

    /* Cache the state of the style reader to avoid rebuilding the same helper
     * structures on subsequent reads. In practice, a single style reader
     * typically processes many properties in sequence rather than just one. */
    cacheReaderState(styleReader->controlState());

    if (styleReader->m_effectiveVariationsDirty)
        rebuildVariationsForReader(styleReader, style);

    const QQStyleKitExtendableControlType exactType = styleReader->controlType();
    const QList<QQStyleKitExtendableControlType> baseTypes = baseTypesForType(exactType);

    QVariant value;

    while (true) {
        value = readPropertyInVariations(styleReader->m_effectiveVariations, style->theme(), ids, exactType, baseTypes);
        if (value.isValid())
            break;

        value = readPropertyInRelevantControls(style->theme(), ids, exactType, baseTypes);
        if (value.isValid())
            break;

        value = readPropertyInVariations(styleReader->m_effectiveVariations, style, ids, exactType, baseTypes);
        if (value.isValid())
            break;

        value = readPropertyInRelevantControls(style, ids, exactType, baseTypes);
        if (value.isValid())
            break;

        if (auto *fallbackStyle = style->fallbackStyle()) {
            /* Recurse into the fallback style, and search for the property there. If not
             * found, and the fallback style has a fallback style, the recursion continues. */
            value = readPropertyInStyle(fallbackStyle, ids, styleReader);
            if (value.isValid())
                break;
        }

        break;
    }

    if (Q_UNLIKELY(QQStyleKitDebug::enabled())) {
        if (!value.isValid())
            QQStyleKitDebug::notifyPropertyNotResolved(ids.property);
    }

    return value;
}

QVariant QQStyleKitPropertyResolver::readStyleProperty(
    const QQStyleKitPropertyGroup *group,
    const QQSK::Property property,
    const QQSK::Property alternative)
{
    const QQStyleKitControlProperties *controlProperties = group->controlProperties();
    const QQSK::PropertyPathFlags pathFlags = group->pathFlags();
    const QQSK::Subclass subclass = controlProperties->subclass();

    if (subclass != QQSK::Subclass::QQStyleKitReader) {
        /* Due to propagation, we must know the reader’s state in order to resolve the value
         * of a given property. For example, the background delegate’s color may follow a
         * completely different lookup path—and therefore produce a different result—depending
         * on whether the reader is a Slider or a Button, and whether it is hovered, pressed, etc.
         * For this reason, when a property is not read via a QQStyleKitReader, propagation and
         * fall back logic cannot be supported.
         * However, to still allow static (i.e., non-propagating) bindings between properties
         * _inside_ a Style, we fall back to return the value set directly on the accessed control. */
        Q_ASSERT(subclass == QQSK::Subclass::QQStyleKitState);
        const QQStyleKitControlState *controlState = controlProperties->asQQStyleKitState();
        const QQStyleKitControl *control = controlState->control();
        const PropertyPathId propertyPathId = group->propertyPathId(property, PropertyPathId::Flag::IncludeSubtype);
        const PropertyStorageId key = propertyPathId.storageId(controlState->nestedState());
        return control->readStyleProperty(key);
    }

    QQStyleKitStyle *style = controlProperties->style();
    if (!style) {
        if (!s_styleWarningsIssued) {
            s_styleWarningsIssued = true;
            qmlWarning(group) << "style properties cannot be read: No StyleKit style has been set!";
        }
        return {};
    }

    if (!style->loaded()) {
        // Optimization: Skip reads until both the style and the theme is ready
        return {};
    }

    QQStyleKitDebug::groupBeingRead = group;
    QQStyleKitReader *styleReader = controlProperties->asQQStyleKitReader();

    if (s_isReadingProperty) {
        if (!s_styleWarningsIssued) {
            s_styleWarningsIssued = true;
            qmlWarning(styleReader) << "The style property '" << property << "' was read "
                                    << "before finishing the read of another style property. "
                                    << "This is likely to cause a style glitch.";
        }
    }
    QScopedValueRollback rollback(s_isReadingProperty, true);

    PropertyPathIds ids;
    ids.property = group->propertyPathId(property, PropertyPathId::Flag::ExcludeSubtype);
    ids.alternative = group->propertyPathId(alternative, PropertyPathId::Flag::ExcludeSubtype);
    const bool insideSubType = pathFlags &
        (QQSK::PropertyPathFlag::DelegateSubtype1 | QQSK::PropertyPathFlag::DelegateSubtype2);

    if (insideSubType) {
        ids.subTypeProperty = group->propertyPathId(property, PropertyPathId::Flag::IncludeSubtype);
        ids.subTypeAlternative = group->propertyPathId(alternative, PropertyPathId::Flag::IncludeSubtype);
    } else {
        ids.subTypeProperty = PropertyPathId();
        ids.subTypeAlternative = PropertyPathId();
    }

    if (!pathFlags.testFlag(QQSK::PropertyPathFlag::Global)) {
        /* A style reader can have a storage that contains local property overrides (that is,
         * interpolated values from an ongoing transition). When searching for a property, we
         * therefore need to check this storage first. The exception is if the property was read
         * inside the 'global' group, which means that we should read the values directly
         * from the style. */
        if (insideSubType) {
            const QVariant value = readPropertyInStorageForState(
                ids.subTypeProperty, ids.subTypeAlternative, styleReader, QQSK::StateFlag::Normal);
            if (value.isValid())
                return value;
        }
        const QVariant value = readPropertyInStorageForState(
            ids.property, ids.alternative, styleReader, QQSK::StateFlag::Normal);
        if (value.isValid())
            return value;
    }

    return readPropertyInStyle(style, ids, styleReader);
}

bool QQStyleKitPropertyResolver::writeStyleProperty(
    const QQStyleKitPropertyGroup *group,
    const QQSK::Property property,
    const QVariant &value)
{
    // While readStyleProperty() takes propagation into account, writeStyleProperty() doesn't.
    // Instead it writes \a value directly to the storage that the group belongs to.
    Q_ASSERT(group);
    const QQStyleKitControlProperties *controlProperties = group->controlProperties();
    const QQSK::PropertyPathFlags pathFlags = group->pathFlags();
    const QQSK::Subclass subclass = controlProperties->subclass();
    const PropertyPathId propertyPathId = group->propertyPathId(property, PropertyPathId::Flag::IncludeSubtype);

    if (pathFlags.testFlag(QQSK::PropertyPathFlag::Global)) {
        qmlWarning(controlProperties) << "Properties inside 'global' are read-only!";
        return false;
    }

    if (subclass == QQSK::Subclass::QQStyleKitReader) {
        // This is a write to a StyleKitReader, probably from an ongoing transition
        QQStyleKitReader *reader = controlProperties->asQQStyleKitReader();
        const PropertyStorageId key = propertyPathId.storageId(QQSK::StateFlag::Normal);
        const QVariant currentValue = reader->readStyleProperty(key);
        const bool valueChanged = currentValue != value;
        if (valueChanged) {
            reader->writeStyleProperty(key, value);
            QQStyleKitDebug::notifyPropertyWrite(group, property, reader, QQSK::StateFlag::Normal, key, value);
        }
        return valueChanged;
    }

    if (subclass == QQSK::Subclass::QQStyleKitState) {
        // This is a write to a control inside the StyleKit style
        const QQStyleKitControlState *controlState = controlProperties->asQQStyleKitState();
        QQStyleKitControl *control = controlState->control();
        const QQSK::State nestedState = controlState->nestedState();
        const PropertyStorageId key = propertyPathId.storageId(nestedState);
        const QVariant currentValue = control->readStyleProperty(key);
        const bool valueChanged = currentValue != value;
        if (valueChanged) {
            /* Optimization: Allow a control to track which states it stores property values for.
             * When later reading a property via propagation, we can skip that control entirely
             * if it has no stored values corresponding to the state requested by the StyleKitReader. */
            control->m_writtenStates |= nestedState;
            /* Optimization: Track which properties a style, theme, or variation defines
             * values for, and for which states. When reading a property later, we can skip
             * the style/theme/variation entirely if it has no stored values for it, or if
             * none match the state requested by the StyleKitReader. */
            QQStyleKitControls *controls = control->controls();
            const QQSK::State alreadyWrittenStates = controls->m_writtenPropertyPaths[propertyPathId.pathId()];
            controls->m_writtenPropertyPaths[propertyPathId.pathId()] = alreadyWrittenStates | nestedState;

            control->writeStyleProperty(key, value);
            QQStyleKitDebug::notifyPropertyWrite(group, property, control, nestedState, key, value);
        }
        return valueChanged;
    }

    Q_UNREACHABLE();
    return false;
}

bool QQStyleKitPropertyResolver::hasLocalStyleProperty(
    const QQStyleKitPropertyGroup *group,
    const QQSK::Property property)
{
    Q_ASSERT(group);
    const QQStyleKitControlProperties *controlProperties = group->controlProperties();
    const QQSK::PropertyPathFlags pathFlags = group->pathFlags();
    const PropertyPathId propertyPathId = group->propertyPathId(property, PropertyPathId::Flag::IncludeSubtype);
    const QQSK::Subclass subclass = controlProperties->subclass();

    if (pathFlags.testFlag(QQSK::PropertyPathFlag::Global))
        return false;

    if (subclass == QQSK::Subclass::QQStyleKitReader) {
        const PropertyStorageId key = propertyPathId.storageId(QQSK::StateFlag::Normal);
        return controlProperties->asQQStyleKitReader()->readStyleProperty(key).isValid();
    }

    if (subclass == QQSK::Subclass::QQStyleKitState) {
        const QQStyleKitControlState *controlState = controlProperties->asQQStyleKitState();
        const QQStyleKitControl *control = controlState->control();
        const PropertyStorageId key = propertyPathId.storageId(controlState->nestedState());
        return control->readStyleProperty(key).isValid();
    }

    Q_UNREACHABLE();
    return false;
}

QT_END_NAMESPACE

#include "moc_qqstylekitpropertyresolver_p.cpp"
