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

void QQStyleKitPropertyResolver::addTypeVariationsToReader(
    QQStyleKitReader *styleReader,
    const QQStyleKitExtendableControlType parentType,
    const QQStyleKitStyle *style)
{
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

    const auto parentBaseTypes = baseTypesForType(parentType);
    const QVariant inStyleVariationsVar = readPropertyInStyle(ids, parentType, parentBaseTypes, style);
    if (!inStyleVariationsVar.isValid())
        return;

    /* Inside each Type Variation, check if the control type that styleReader represents has
     * been defined. If so, it means that the variation might affect it, and should therefore be
     * added to the style readers list of effective variations. */
    const QQStyleKitExtendableControlType styleReaderType = styleReader->type();
    const auto styleReaderBaseType = baseTypesForType(styleReaderType);

    const auto inStyleVariations = *qvariant_cast<QList<QQStyleKitVariation *> *>(inStyleVariationsVar);
    for (auto *variation : inStyleVariations) {
        if (!variation) {
            // This happens if the user adds non QQStyleKitVariation elements to the QML array
            continue;
        }
        /* Note: when we read the variations property from the style, it returns the varations
         * set in the most specific storage/state/control (because of propagation). And those
         * are the only ones that will take effect. This means that even if there are variations
         * in the fallback style for the requested type (control) that overrides some properties,
         * and the style/theme has variations that overrides something else, the variations in
         * the fallback style will, in that case, not be used. The properties not set in the
         * effective variation will instead propagate back to be read from the type in the theme
         * or the style. This approach is easier to understand and work with, since the propagation
         * always flow in one direction, and doesn't jump back and forth between variations, styles
         * and themes. */
        if (variation->getControl(styleReaderType)) {
            styleReader->m_effectiveInStyleVariations.append(variation);
        } else {
            for (int type : styleReaderBaseType) {
                if (variation->getControl(type)) {
                    styleReader->m_effectiveInStyleVariations.append(variation);
                    break;
                }
            }
        }
    }
}

void QQStyleKitPropertyResolver::addInstanceVariationsToReader(
    QQStyleKitReader *styleReader, const QStringList &inAppVariationNames,
    const QVarLengthArray<const QQStyleKitControls *, 6> &stylesAndThemes)
{
    /* Add the variations set from the application to the list of effective variations
     * in the styleReader. But, to speed up property look-up later on, we only add the
     * variations that has the potential to affect the control type, or its base types,
     * that the styleReader represents. The variations that are closest to styleReader
     * in the hierarchy will be added first and take precendence over the ones added last. */
    const QQStyleKitExtendableControlType styleReaderType = styleReader->type();
    const auto styleReaderBaseTypes = baseTypesForType(styleReaderType);

    for (const QString &attachedVariationName : inAppVariationNames) {
        for (const QQStyleKitControls *styleOrTheme : stylesAndThemes) {
            const QList<QQStyleKitVariation *> variationsInStyleOrTheme = styleOrTheme->variations();
            for (QQStyleKitVariation *variationInStyleOrTheme : variationsInStyleOrTheme)  {
                if (variationInStyleOrTheme->name() != attachedVariationName)
                    continue;
                /* Invariant: we found a variation in a Style or a Theme with a name that matches
                 * a name in the attached variation list. Check if the found variation contains the
                 * type, or the subtypes, of the style reader. If not, it doesn't affect it and can
                 * therefore be skipped. */
                if (variationInStyleOrTheme->getControl(styleReaderType)) {
                    styleReader->m_effectiveInAppVariations.append(variationInStyleOrTheme);
                } else {
                    for (int baseType : styleReaderBaseTypes) {
                        if (variationInStyleOrTheme->getControl(baseType)) {
                            styleReader->m_effectiveInAppVariations.append(variationInStyleOrTheme);
                            break;
                        }
                    }
                }
            }
        }
    }
}

void QQStyleKitPropertyResolver::rebuildVariationsForReader(
    QQStyleKitReader *styleReader, const QQStyleKitStyle *style)
{
    /* Traverse up the parent chain of \a styleReader, and for each parent, look for an
     * instance of QQStyleKitControlAttached. And for each attached object, check if it
     * has variations that can potentially affect the style reader. If so, add the
     * variations to the style readers list of effective variations.
     * A QQStyleKitControlAttached can specify both Instance Variations and Type Variations.
     * The former should affect all descendant StyleKitReaders of the parent, while the
     * latter should only affect descendant StyleKitReaders of a specific type. */
    Q_ASSERT(styleReader->m_effectiveVariationsDirty);
    styleReader->m_effectiveVariationsDirty = false;
    styleReader->m_effectiveInAppVariations.clear();
    styleReader->m_effectiveInStyleVariations.clear();

    const bool hasInstanceVariations = QQStyleKitControlAttached::s_variationCount > 0;
    const bool styleHasVariations = QQStyleKitVariation::s_variationCount > 0;
    if (!styleHasVariations) {
        /* No variations are defined in the current style or theme. In that case
         * it doesn't matter if the application has variations set - they
         * will anyway not map to any variations in the style. */
        return;
    }

    /* We need to search through all variations defined in either the style or the theme,
     * including the ones in the fallback styles, since the variation property propagates,
     * like all the other style properties. */
    QVarLengthArray<const QQStyleKitControls *, 6> stylesAndThemes;
    const QQStyleKitStyle *styleOrFallbackStyle = style;
    while (styleOrFallbackStyle) {
        if (const auto *theme = styleOrFallbackStyle->theme())
            stylesAndThemes.append(theme);
        stylesAndThemes.append(styleOrFallbackStyle);
        styleOrFallbackStyle = styleOrFallbackStyle->fallbackStyle();
    }

    QObject *parentObj = styleReader;
    while (parentObj) {
        QObject *attachedObject = qmlAttachedPropertiesObject<QQStyleKitControl>(parentObj, false);
        if (attachedObject) {
            auto *controlAtt = static_cast<QQStyleKitControlAttached *>(attachedObject);
            if (hasInstanceVariations)
                addInstanceVariationsToReader(styleReader, controlAtt->variations(), stylesAndThemes);
            if (styleHasVariations)
                addTypeVariationsToReader(styleReader, controlAtt->controlType(), style);
        }

        parentObj = parentObj->parent();
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

QVariant QQStyleKitPropertyResolver::readPropertyInStyle(
    const PropertyPathIds &ids,
    const QQStyleKitExtendableControlType exactType,
    const QList<QQStyleKitExtendableControlType> baseTypes,
    const QQStyleKitStyle *style)
{
    QVariant value;

    while (true) {
        value = readPropertyInRelevantControls(style->theme(), ids, exactType, baseTypes);
        if (value.isValid())
            break;
        value = readPropertyInRelevantControls(style, ids, exactType, baseTypes);
        if (value.isValid())
            break;

        if (auto *fallbackStyle = style->fallbackStyle()) {
            /* Recurse into the fallback style, and search for the property there. If not
             * found, and the fallback style has a fallback style, the recursion continues. */
            fallbackStyle->setPalette(style->palette());
            value = readPropertyInStyle(ids, exactType, baseTypes, fallbackStyle);
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

QVariant QQStyleKitPropertyResolver::readProperty(
    const PropertyPathIds &ids, QQStyleKitReader *styleReader, QQStyleKitStyle *style)
{
    if (styleReader->m_effectiveVariationsDirty)
        rebuildVariationsForReader(styleReader, style);

    /* Sync the palette of the style with the palette of the current reader. Note
     * that this can cause palette bindings in the style to change, which will
     * result in calls to writeStyleProperty(). */
    style->setPalette(styleReader->palette());

    const QQStyleKitExtendableControlType exactType = styleReader->type();
    const QList<QQStyleKitExtendableControlType> baseTypes = baseTypesForType(exactType);

    QVariant value;

    while (true) {
        for (const QPointer<QQStyleKitVariation> &variation : std::as_const(styleReader->m_effectiveInAppVariations)) {
            if (!variation)
                continue;
            value = readPropertyInRelevantControls(variation, ids, exactType, baseTypes);
            if (value.isValid())
                break;
        }
        if (value.isValid())
            break;

        for (const QPointer<QQStyleKitVariation> &variation : std::as_const(styleReader->m_effectiveInStyleVariations)) {
            if (!variation)
                continue;
            value = readPropertyInRelevantControls(variation, ids, exactType, baseTypes);
            if (value.isValid())
                break;
        }
        if (value.isValid())
            break;

        value = readPropertyInStyle(ids, exactType, baseTypes, style);
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

    if (subclass == QQSK::Subclass::QQStyleKitState) {
        /* Due to propagation, we must know the reader’s state in order to resolve the value
         * of a given property. For example, the background delegate’s color may follow a
         * completely different lookup path—and therefore produce a different result—depending
         * on whether the reader is a Slider or a Button, and whether it is hovered, pressed, etc.
         * For this reason, when a property is accessed directly from a control (or one of its
         * states) within the StyleKit, full propagation cannot be supported: the necessary
         * reader context is unavailable.
         * However, to still allow static (i.e., non-propagating) bindings between properties
         * inside a Style, we fall back to simply returning the value defined on the accessed
         * control itself. */
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

    if (subclass == QQSK::Subclass::QQStyleKitReader) {
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

        style->setPalette(styleReader->palette());
        cacheReaderState(styleReader->controlState());
        const QVariant value = readProperty(ids, styleReader, style);
        return value;
    }

    Q_UNREACHABLE();
    return {};
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
            /* Optimization: Track which properties a control’s subclass (style, theme, or
             * variation) defines values for, and for which states. When reading a property
             * later, we can skip the style/theme/variation entirely if it has no stored
             * values for it, or if none match the state requested by the StyleKitReader. */
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
