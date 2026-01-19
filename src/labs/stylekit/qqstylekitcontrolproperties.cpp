// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitcontrol_p.h"
#include "qqstylekitstyle_p.h"
#include "qqstylekittheme_p.h"
#include "qqstylekitcontrolproperties_p.h"
#include "qqstylekitpropertyresolver_p.h"

QT_BEGIN_NAMESPACE

// ************* QQStyleKitPropertyGroup ****************

QHash<PropertyPathId_t, QString> QQStyleKitPropertyGroup::s_pathStrings;

QQStyleKitPropertyGroup::QQStyleKitPropertyGroup(QQSK::PropertyGroup, QObject *parent)
    : QObject(parent)
{
}

PropertyPathId QQStyleKitPropertyGroup::propertyPathId(QQSK::Property property, PropertyPathId::Flag flag) const
{
    if (flag == PropertyPathId::Flag::IncludeSubtype) {
        if (m_pathFlags.testFlag(QQSK::PropertyPathFlag::DelegateSubtype1))
            return PropertyPathId(property, m_groupSpace.start, QQSK::PropertyGroup::DelegateSubtype1);
        else if (m_pathFlags.testFlag(QQSK::PropertyPathFlag::DelegateSubtype2))
            return PropertyPathId(property, m_groupSpace.start, QQSK::PropertyGroup::DelegateSubtype2);
    }
    return PropertyPathId(property, m_groupSpace.start, QQSK::PropertyGroup::DelegateSubtype0);
}

QString QQStyleKitPropertyGroup::pathToString() const
{
    /* Start from the root of the path and build the path down to this group. This
     * mirrors how the groups were originally created and avoids rounding issues
     * that can arise if attempting to reconstruct the path “backwards”.
     * Note: For each group, m_groupSpace.start is stored relative to the root,
     * while m_groupSpace.size is relative to the parent group. However, when
     * calculating the group index, the group-space start must be computed
     * relative to the parent group.
     * We cache the requested paths, as the same paths are typically requested
     * repeatedly. The number of possible paths (and thus leaf groups) is well below
     * 100, and in practice the cache usually ends up with fewer than 20 entries. */
    if (s_pathStrings.contains(m_groupSpace.start))
        return s_pathStrings[m_groupSpace.start];

    constexpr PropertyPathId_t rootGroupsSize = nestedGroupsStartSize / nestedGroupCount;
    const auto metaEnum = QMetaEnum::fromType<QQSK::PropertyGroup>();

    PropertyPathId_t nestedGroupStart = m_groupSpace.start;
    PropertyPathId_t nestedGroupSize = rootGroupsSize;
    PropertyPathId_t nestedGroupIndex = nestedGroupStart / nestedGroupSize;
    auto groupType = QQSK::PropertyGroup(nestedGroupIndex);
    if (groupType == QQSK::PropertyGroup::Control)
        return {};

    QString groupName = QString::fromLatin1(metaEnum.valueToKey(static_cast<int>(groupType)));
    groupName[0] = groupName[0].toLower();
    QString pathString = groupName;

    while (true) {
        nestedGroupStart -= nestedGroupIndex * nestedGroupSize;
        nestedGroupSize /= nestedGroupCount;
        nestedGroupIndex = nestedGroupStart / nestedGroupSize;
        groupType = QQSK::PropertyGroup(nestedGroupIndex);
        if (groupType == QQSK::PropertyGroup::Control)
            break;

        QString groupName = QString::fromLatin1(metaEnum.valueToKey(static_cast<int>(groupType)));
        groupName[0] = groupName[0].toLower();
        pathString += '.'_L1 + groupName;
    }

    s_pathStrings.insert(m_groupSpace.start, pathString);
    return pathString;
}

QQStyleKitControlProperties *QQStyleKitPropertyGroup::controlProperties() const
{
    if (isControlProperties()) {
        Q_ASSERT(qobject_cast<const QQStyleKitControlProperties *>(this));
        auto *self = const_cast<QQStyleKitPropertyGroup *>(this);
        return static_cast<QQStyleKitControlProperties *>(self);
    }
    Q_ASSERT(qobject_cast<const QQStyleKitControlProperties *>(parent()));
    return static_cast<QQStyleKitControlProperties *>(parent());
}

template<typename T>
T *QQStyleKitPropertyGroup::lazyCreateGroup(T * const &ptr, QQSK::PropertyGroup group) const
{
    T *nestedGroup = QQSK::lazyCreate(ptr, controlProperties(), group);

    // Nested groups inherit path flags from their parents
    nestedGroup->m_pathFlags = m_pathFlags;

    if (group == QQSK::PropertyGroup::DelegateSubtype1) {
        /* Subtypes, like states, are not part of a property's path ID—they belong to the
         * storage ID instead. They are therefore prefixed later, during lookup, when
         * propagation determines which value to read.
         * For now, we simply record which subtype this group (and any nested groups) is
         * associated with. The subtype will then be taken into account later when reading
         * properties from the group. Setting aside space for the sub types was already
         * taken care of during the construction of the root QQStyleKitControlProperties. */
        nestedGroup->m_pathFlags.setFlag(QQSK::PropertyPathFlag::DelegateSubtype1);
        nestedGroup->m_groupSpace = m_groupSpace;
    } else if (group == QQSK::PropertyGroup::DelegateSubtype2) {
        nestedGroup->m_pathFlags.setFlag(QQSK::PropertyPathFlag::DelegateSubtype2);
        nestedGroup->m_groupSpace = m_groupSpace;
    } else {
        /* Calculate the available property ID space for the nested group. This is done by
         * dividing the available space inside _this_ group on the number of potential groups
         * that _this_ group can potentially contain. */
        const PropertyPathId_t nestedGroupIndex = PropertyPathId_t(group);
        const PropertyPathId_t nestedGroupSize = m_groupSpace.size / nestedGroupCount;
        nestedGroup->m_groupSpace.size = nestedGroupSize;
        nestedGroup->m_groupSpace.start = m_groupSpace.start + (nestedGroupIndex * nestedGroupSize);
        /* Ensure that we haven’t exhausted the available PropertyPathId space. There must be
         * enough room remaining to assign IDs for all properties defined in QQSK::Property.
         * If this assertion triggers, consider switching to a wider PropertyPathId_t type or
         * optimizing how the space is allocated. For example, certain nested paths (such as
         * control.handle.indicator) can never occur, yet we currently reserve INNER_GROUP_COUNT
         * for every nesting level, which is wasteful. */
        Q_ASSERT(nestedGroupSize >= PropertyPathId_t(QQSK::Property::COUNT));
    }
    return nestedGroup;
}

/* This macro will check if the caller has the same group path as \a GROUP_PATH.
 * This is needed since a QQSK::Property (e.g Color) can sometimes be a
 * property in several different subclasses of QQStyleKitPropertyGroup.
 * For example, both control.background.color and control.indicator.color has a
 * color property. But the group path differs, so they are in reality two completely
 * different properties. And in that case, when the former changes value, we want to
 * emit changes globally only to that property, and not the latter.
 * The caller of this macro will therefore need to go through all the usages of its
 * subclass in the API, to figure out which group itself is an instance of. For the
 * one that is a match, the macro will go through all readers and emit the same
 * signal for them. */
#define CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(CONTROL_PROPERTIES, GROUP_PATH) \
if (this == CONTROL_PROPERTIES -> GROUP_PATH ) { \
    for (QQStyleKitReader *reader : QQStyleKitReader::s_allReaders) { \
        const auto baseTypes = QQStyleKitPropertyResolver::baseTypesForType(reader->type()); \
        if (reader->type() != controlType && !baseTypes.contains(controlType)) \
            continue; \
        reader->clearLocalStorage(); \
        ((reader-> GROUP_PATH ->*changedSignals)(), ...); \
    } \
    return; \
}

template<typename SUBCLASS>
void QQStyleKitPropertyGroup::handleStylePropertyChanged(void (SUBCLASS::*changedSignal)()) {
    handleStylePropertiesChanged<SUBCLASS>(changedSignal);
}

template <typename SUBCLASS, typename... CHANGED_SIGNALS>
void QQStyleKitPropertyGroup::handleStylePropertiesChanged(CHANGED_SIGNALS... changedSignals)
{
    /* This function will check which subclass of QQStyleKitProperties this
     * group is (nested) inside. Based on that, it will decide if the signals
     * should be emitted locally or not, and if the changed properties affects
     * all existing QQStyleKitReaders, and therefore will need to be
     * emitted 'globally'. Note that it only makes sense to call this function
     * for changed properties that are available from a QQStyleKitReader.
     * Properities only available from e.g QQStyleKitControl (such as
     * variations), are anyway not readable from a QQStyleKitReader. */
    static_assert(std::is_base_of<QQStyleKitPropertyGroup, SUBCLASS>::value,
                  "SUBCLASS must inherit QQStyleKitPropertyGroup");

    auto *group = static_cast<SUBCLASS *>(this);
    const QQSK::Subclass objectWrittenTo = controlProperties()->subclass();

    if (objectWrittenTo == QQSK::Subclass::QQStyleKitState) {
        ((group->*changedSignals)(), ...);

        if (shouldEmitGlobally()) {
            const QQStyleKitControl *control = controlProperties()->asQQStyleKitState()->control();
            const QQStyleKitExtendableControlType type = control->controlType();
            group->emitGlobally(type, changedSignals...);
        }
        return;
    }

    if (objectWrittenTo == QQSK::Subclass::QQStyleKitReader) {
        /* Unless the StyleReader has told us not to emit any signals (because it's only
         * syncing it's own local storage with old values before starting a transition), we
         * emit the signal like normal. This will cause the control to repaint (perhaps
         * using a transition). */
        if (shouldEmitLocally())
            ((group->*changedSignals)(), ...);
        return;
    }

    Q_UNREACHABLE();
}

void QQStyleKitPropertyGroup::emitChangedForAllStylePropertiesRecursive(EmitFlags emitFlags)
{
    /* This function will emit changed signals for all style properties in the
     * StyleKit API (for a single QQStyleKitReader), which is needed after
     * doing a style-, or theme change. */
    const int startIndex = QQStyleKitPropertyGroup::staticMetaObject.propertyOffset();
    const QMetaObject* meta = metaObject();
    for (int i = startIndex; i < meta->propertyCount(); ++i) {
        const QMetaProperty prop = meta->property(i);
        const QMetaObject* propMetaObject = QMetaType::fromName(prop.typeName()).metaObject();
        if (propMetaObject) {
            if (propMetaObject->inherits(&QQStyleKitDelegateProperties::staticMetaObject)) {
                /* Skip recursing into QQStyleKitDelegateProperties, because those are lazy
                 * created when read, and reading them from here would accidentally
                 * create them. */
                continue;
            }
            if (propMetaObject->inherits(&QQStyleKitPropertyGroup::staticMetaObject)) {
                // The property is of type QQStyleKitPropertyGroup, so recurse into it
                QObject *childObj = qvariant_cast<QObject *>(property(prop.name()));
                if (auto *child = qobject_cast<QQStyleKitPropertyGroup *>(childObj))
                    child->emitChangedForAllStylePropertiesRecursive(emitFlags);
                continue;
            }
        }

        if (!emitFlags.testFlag(EmitFlag::AllProperties)) {
            // Only emit for color properties when the Colors flag is set
            if (emitFlags.testFlag(EmitFlag::Colors)) {
                if (prop.metaType() != QMetaType::fromType<QColor>())
                    continue;
            }
        }

        // Emit the changed signal for the property
        Q_ASSERT(prop.hasNotifySignal());
        QMetaMethod notify = prop.notifySignal();
        notify.invoke(this, Qt::DirectConnection);
    }
}

bool QQStyleKitPropertyGroup::shouldEmitLocally()
{
    return !controlProperties()->asQQStyleKitReader()->dontEmitChangedSignals();
}

bool QQStyleKitPropertyGroup::shouldEmitGlobally()
{
    QQStyleKitStyle *parentStyle = controlProperties()->style();
    if (!parentStyle)
        return false;

    if (parentStyle->loaded() && !parentStyle->m_isUpdatingPalette) {
        /* When a property has changed in the 'global' QQStyleKitStyle itself, it can
        * potentially affect all control instances. We therefore need to go through all
        * QQStyleKitReaders and inform that their own local property that matches the
        * 'global' property needs to be re-read. We emit the signals directly, omitting any
        * applied transitions in the QQStyleKitReaders, to optimize for speed. The exception
        * is if we're just updating the palette in the Style to match the palette in the current
        * control / QQStyleKitReader. Such a change will only affect a single control. */
        return parentStyle == QQStyleKitStyle::current();
    }
    return false;
}

// ************* QQStyleKitImageProperties ****************

QQStyleKitImageProperties::QQStyleKitImageProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent)
    : QQStyleKitPropertyGroup(group, parent)
{
}

template <typename... CHANGED_SIGNALS>
void QQStyleKitImageProperties::emitGlobally(
    QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const
{
    // Go through all instances of QQStyleKitImageProperties
    const QQStyleKitControlProperties *cp = controlProperties();
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, background()->image());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, handle()->image());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, indicator()->image());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, indicator()->foreground()->image());
}

QUrl QQStyleKitImageProperties::source() const
{
    return styleProperty<QUrl>(QQSK::Property::Source);
}

void QQStyleKitImageProperties::setSource(const QUrl &source)
{
    if (setStyleProperty(QQSK::Property::Source, source))
        handleStylePropertyChanged(&QQStyleKitImageProperties::sourceChanged);
}

QColor QQStyleKitImageProperties::color() const
{
    return styleProperty<QColor>(QQSK::Property::Color);
}

void QQStyleKitImageProperties::setColor(const QColor &color)
{
    if (setStyleProperty(QQSK::Property::Color, color))
        handleStylePropertyChanged(&QQStyleKitImageProperties::colorChanged);
}

QQuickImage::FillMode QQStyleKitImageProperties::fillMode() const
{
    return styleProperty<QQuickImage::FillMode>(QQSK::Property::FillMode);
}

void QQStyleKitImageProperties::setFillMode(QQuickImage::FillMode fillMode)
{
    if (setStyleProperty(QQSK::Property::FillMode, fillMode))
        handleStylePropertyChanged(&QQStyleKitImageProperties::fillModeChanged);
}

// ************* QQStyleKitBorderProperties ****************

QQStyleKitBorderProperties::QQStyleKitBorderProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent)
    : QQStyleKitPropertyGroup(group, parent)
{
}

template <typename... CHANGED_SIGNALS>
void QQStyleKitBorderProperties::emitGlobally(
    QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const
{
    // Go through all instances of QQStyleKitBorderProperties
    const QQStyleKitControlProperties *cp = controlProperties();
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, background()->border());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, handle()->border());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, indicator()->border());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, indicator()->foreground()->border());
}

qreal QQStyleKitBorderProperties::width() const
{
    return styleProperty<qreal>(QQSK::Property::Width);
}

void QQStyleKitBorderProperties::setWidth(qreal width)
{
    if (setStyleProperty(QQSK::Property::Width, width))
        handleStylePropertyChanged(&QQStyleKitBorderProperties::widthChanged);
}

QColor QQStyleKitBorderProperties::color() const
{
    return styleProperty<QColor>(QQSK::Property::Color, Qt::transparent);
}

void QQStyleKitBorderProperties::setColor(const QColor &color)
{
    if (setStyleProperty(QQSK::Property::Color, color))
        handleStylePropertyChanged(&QQStyleKitBorderProperties::colorChanged);
}

// ************* QQStyleKitShadowProperties ****************

QQStyleKitShadowProperties::QQStyleKitShadowProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent)
    : QQStyleKitPropertyGroup(group, parent)
{
}

template <typename... CHANGED_SIGNALS>
void QQStyleKitShadowProperties::emitGlobally(
    QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const
{
    // Go through all instances of QQStyleKitShadowProperties
    const QQStyleKitControlProperties *cp = controlProperties();
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, background()->shadow());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, handle()->shadow());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, indicator()->shadow());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, indicator()->foreground()->shadow());
}

QColor QQStyleKitShadowProperties::color() const
{
    return styleProperty<QColor>(QQSK::Property::Color, Qt::transparent);
}

void QQStyleKitShadowProperties::setColor(QColor color)
{
    if (setStyleProperty(QQSK::Property::Color, color))
        handleStylePropertyChanged(&QQStyleKitShadowProperties::colorChanged);
}

qreal QQStyleKitShadowProperties::opacity() const
{
    return styleProperty<qreal>(QQSK::Property::Opacity, 1.0);
}

void QQStyleKitShadowProperties::setOpacity(qreal opacity)
{
    if (setStyleProperty(QQSK::Property::Opacity, opacity))
        handleStylePropertyChanged(&QQStyleKitShadowProperties::opacityChanged);
}

qreal QQStyleKitShadowProperties::scale() const
{
    return styleProperty<qreal>(QQSK::Property::Scale, 1.0);
}

void QQStyleKitShadowProperties::setScale(qreal scale)
{
    if (setStyleProperty(QQSK::Property::Scale, scale))
        handleStylePropertyChanged(&QQStyleKitShadowProperties::scaleChanged);
}

qreal QQStyleKitShadowProperties::verticalOffset() const
{
    return styleProperty<qreal>(QQSK::Property::VOffset);
}

void QQStyleKitShadowProperties::setVerticalOffset(qreal verticalOffset)
{
    if (setStyleProperty(QQSK::Property::VOffset, verticalOffset))
        handleStylePropertyChanged(&QQStyleKitShadowProperties::verticalOffsetChanged);
}

qreal QQStyleKitShadowProperties::horizontalOffset() const
{
    return styleProperty<qreal>(QQSK::Property::HOffset);
}

void QQStyleKitShadowProperties::setHorizontalOffset(qreal horizontalOffset)
{
    if (setStyleProperty(QQSK::Property::HOffset, horizontalOffset))
        handleStylePropertyChanged(&QQStyleKitShadowProperties::horizontalOffsetChanged);
}

qreal QQStyleKitShadowProperties::blur() const
{
    return styleProperty<qreal>(QQSK::Property::Blur, 10.0);
}

void QQStyleKitShadowProperties::setBlur(qreal blur)
{
    if (setStyleProperty(QQSK::Property::Blur, blur))
        handleStylePropertyChanged(&QQStyleKitShadowProperties::blurChanged);
}

bool QQStyleKitShadowProperties::visible() const
{
    return styleProperty<bool>(QQSK::Property::Visible, true);
}

void QQStyleKitShadowProperties::setVisible(bool visible)
{
    if (setStyleProperty(QQSK::Property::Visible, visible))
        handleStylePropertyChanged(&QQStyleKitShadowProperties::visibleChanged);
}

QQmlComponent *QQStyleKitShadowProperties::delegate() const
{
    return styleProperty<QQmlComponent *>(QQSK::Property::Delegate);
}

void QQStyleKitShadowProperties::setDelegate(QQmlComponent *delegate)
{
    if (setStyleProperty(QQSK::Property::Delegate, delegate))
        handleStylePropertyChanged(&QQStyleKitShadowProperties::delegateChanged);
}

// ************* QQStyleKitDelegateProperties ****************

QQStyleKitDelegateProperties::QQStyleKitDelegateProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent)
    : QQStyleKitPropertyGroup(group, parent)
{
}

template <typename... CHANGED_SIGNALS>
void QQStyleKitDelegateProperties::emitGlobally(
    QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const
{
    // Go through all instances of QQStyleKitDelegateProperties
    const QQStyleKitControlProperties *cp = controlProperties();
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, background());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, handle());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, indicator());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, indicator()->foreground());
}

qreal QQStyleKitDelegateProperties::radius() const
{
    return styleProperty<qreal>(QQSK::Property::Radius);
}

void QQStyleKitDelegateProperties::setRadius(qreal radius)
{
    if (setStyleProperty(QQSK::Property::Radius, radius))
        handleStylePropertiesChanged<QQStyleKitDelegateProperties>(
            &QQStyleKitDelegateProperties::radiusChanged,
            &QQStyleKitDelegateProperties::topLeftRadiusChanged,
            &QQStyleKitDelegateProperties::topRightRadiusChanged,
            &QQStyleKitDelegateProperties::bottomLeftRadiusChanged,
            &QQStyleKitDelegateProperties::bottomRightRadiusChanged);
}

qreal QQStyleKitDelegateProperties::topLeftRadius() const
{
    return styleProperty<qreal>(QQSK::Property::TopLeftRadius, QQSK::Property::Radius);
}

void QQStyleKitDelegateProperties::setTopLeftRadius(qreal radius)
{
    if (setStyleProperty(QQSK::Property::TopLeftRadius, radius))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::topLeftRadiusChanged);
}

qreal QQStyleKitDelegateProperties::topRightRadius() const
{
    return styleProperty<qreal>(QQSK::Property::TopRightRadius, QQSK::Property::Radius);
}

void QQStyleKitDelegateProperties::setTopRightRadius(qreal radius)
{
    if (setStyleProperty(QQSK::Property::TopRightRadius, radius))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::topRightRadiusChanged);
}

qreal QQStyleKitDelegateProperties::bottomLeftRadius() const
{
    return styleProperty<qreal>(QQSK::Property::BottomLeftRadius, QQSK::Property::Radius);
}

void QQStyleKitDelegateProperties::setBottomLeftRadius(qreal radius)
{
    if (setStyleProperty(QQSK::Property::BottomLeftRadius, radius))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::bottomLeftRadiusChanged);
}

qreal QQStyleKitDelegateProperties::bottomRightRadius() const
{
    return styleProperty<qreal>(QQSK::Property::BottomRightRadius, QQSK::Property::Radius);
}

void QQStyleKitDelegateProperties::setBottomRightRadius(qreal radius)
{
    if (setStyleProperty(QQSK::Property::BottomRightRadius, radius))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::bottomRightRadiusChanged);
}

qreal QQStyleKitDelegateProperties::scale() const
{
    return styleProperty<qreal>(QQSK::Property::Scale, 1.0);
}

void QQStyleKitDelegateProperties::setScale(qreal scale)
{
    if (setStyleProperty(QQSK::Property::Scale, scale))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::scaleChanged);
}

qreal QQStyleKitDelegateProperties::rotation() const
{
    return styleProperty<qreal>(QQSK::Property::Rotation);
}

void QQStyleKitDelegateProperties::setRotation(qreal rotation)
{
    if (setStyleProperty(QQSK::Property::Rotation, rotation))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::rotationChanged);
}

qreal QQStyleKitDelegateProperties::implicitWidth() const
{
    return styleProperty<qreal>(QQSK::Property::ImplicitWidth);
}

void QQStyleKitDelegateProperties::setImplicitWidth(qreal width)
{
    if (setStyleProperty(QQSK::Property::ImplicitWidth, width))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::implicitWidthChanged);
}

qreal QQStyleKitDelegateProperties::implicitHeight() const
{
    return styleProperty<qreal>(QQSK::Property::ImplicitHeight);
}

void QQStyleKitDelegateProperties::setImplicitHeight(qreal height)
{
    if (setStyleProperty(QQSK::Property::ImplicitHeight, height))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::implicitHeightChanged);
}

qreal QQStyleKitDelegateProperties::minimumWidth() const
{
    return styleProperty<qreal>(QQSK::Property::MinimumWidth);
}

void QQStyleKitDelegateProperties::setMinimumWidth(qreal width)
{
    if (setStyleProperty(QQSK::Property::MinimumWidth, width))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::minimumWidthChanged);
}

qreal QQStyleKitDelegateProperties::margins() const
{
    return styleProperty<qreal>(QQSK::Property::Margins);
}

void QQStyleKitDelegateProperties::setMargins(qreal margins)
{
    if (setStyleProperty(QQSK::Property::Margins, margins))
        handleStylePropertiesChanged<QQStyleKitDelegateProperties>(
            &QQStyleKitDelegateProperties::marginsChanged,
            &QQStyleKitDelegateProperties::leftMarginChanged,
            &QQStyleKitDelegateProperties::rightMarginChanged,
            &QQStyleKitDelegateProperties::topMarginChanged,
            &QQStyleKitDelegateProperties::bottomMarginChanged);
}

qreal QQStyleKitDelegateProperties::leftMargin() const
{
    return styleProperty<qreal>(QQSK::Property::LeftMargin, QQSK::Property::Margins);
}

void QQStyleKitDelegateProperties::setLeftMargin(qreal margin)
{
    if (setStyleProperty(QQSK::Property::LeftMargin, margin))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::leftMarginChanged);
}

qreal QQStyleKitDelegateProperties::rightMargin() const
{
    return styleProperty<qreal>(QQSK::Property::RightMargin, QQSK::Property::Margins);
}

void QQStyleKitDelegateProperties::setRightMargin(qreal margin)
{
    if (setStyleProperty(QQSK::Property::RightMargin, margin))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::rightMarginChanged);
}

qreal QQStyleKitDelegateProperties::topMargin() const
{
    return styleProperty<qreal>(QQSK::Property::TopMargin, QQSK::Property::Margins);
}

void QQStyleKitDelegateProperties::setTopMargin(qreal margin)
{
    if (setStyleProperty(QQSK::Property::TopMargin, margin))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::topMarginChanged);
}

qreal QQStyleKitDelegateProperties::bottomMargin() const
{
    return styleProperty<qreal>(QQSK::Property::BottomMargin, QQSK::Property::Margins);
}

void QQStyleKitDelegateProperties::setBottomMargin(qreal margin)
{
    if (setStyleProperty(QQSK::Property::BottomMargin, margin))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::bottomMarginChanged);
}

Qt::Alignment QQStyleKitDelegateProperties::alignment() const
{
    return styleProperty<Qt::Alignment>(QQSK::Property::Alignment, Qt::AlignLeft | Qt::AlignVCenter);
}

void QQStyleKitDelegateProperties::setAlignment(Qt::Alignment alignment)
{
    if (setStyleProperty(QQSK::Property::Alignment, alignment))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::alignmentChanged);
}

qreal QQStyleKitDelegateProperties::opacity() const
{
    return styleProperty<qreal>(QQSK::Property::Opacity, 1.0);
}

void QQStyleKitDelegateProperties::setOpacity(qreal opacity)
{
    if (setStyleProperty(QQSK::Property::Opacity, opacity))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::opacityChanged);
}

QColor QQStyleKitDelegateProperties::color() const
{
    return styleProperty<QColor>(QQSK::Property::Color, Qt::transparent);
}

void QQStyleKitDelegateProperties::setColor(const QColor &color)
{
    if (setStyleProperty(QQSK::Property::Color, color))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::colorChanged);
}

bool QQStyleKitDelegateProperties::visible() const
{
    return styleProperty<bool>(QQSK::Property::Visible, true);
}

void QQStyleKitDelegateProperties::setVisible(bool visible)
{
    if (setStyleProperty(QQSK::Property::Visible, visible))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::visibleChanged);
}

bool QQStyleKitDelegateProperties::clip() const
{
    return styleProperty<bool>(QQSK::Property::Clip, false);
}

void QQStyleKitDelegateProperties::setClip(bool clip)
{
    if (setStyleProperty(QQSK::Property::Clip, clip))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::clipChanged);
}

QQuickGradient *QQStyleKitDelegateProperties::gradient() const
{
    return styleProperty<QQuickGradient *>(QQSK::Property::Gradient);
}

void QQStyleKitDelegateProperties::setGradient(QQuickGradient *gradient)
{
    if (setStyleProperty(QQSK::Property::Gradient, gradient))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::gradientChanged);
}

QObject *QQStyleKitDelegateProperties::data() const
{
    return styleProperty<QObject *>(QQSK::Property::Data);
}

void QQStyleKitDelegateProperties::setData(QObject *data)
{
    if (setStyleProperty(QQSK::Property::Data, data))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::dataChanged);
}

QQmlComponent *QQStyleKitDelegateProperties::delegate() const
{
    return styleProperty<QQmlComponent *>(QQSK::Property::Delegate);
}

void QQStyleKitDelegateProperties::setDelegate(QQmlComponent *delegate)
{
    if (setStyleProperty(QQSK::Property::Delegate, delegate))
        handleStylePropertyChanged(&QQStyleKitDelegateProperties::delegateChanged);
}

QQStyleKitBorderProperties *QQStyleKitDelegateProperties::border() const
{
    return lazyCreateGroup(m_border, QQSK::PropertyGroup::Border);
}

QQStyleKitShadowProperties *QQStyleKitDelegateProperties::shadow() const
{
    return lazyCreateGroup(m_shadow, QQSK::PropertyGroup::Shadow);
}

QQStyleKitImageProperties *QQStyleKitDelegateProperties::image() const
{
    return lazyCreateGroup(m_image, QQSK::PropertyGroup::Image);
}

// ************* QQStyleKitHandleProperties ****************

QQStyleKitHandleProperties::QQStyleKitHandleProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent)
    : QQStyleKitDelegateProperties(group, parent)
{
}

QQStyleKitDelegateProperties *QQStyleKitHandleProperties::first() const
{
    return lazyCreateGroup(m_first, QQSK::PropertyGroup::DelegateSubtype1);
}

QQStyleKitDelegateProperties *QQStyleKitHandleProperties::second() const
{
    return lazyCreateGroup(m_second, QQSK::PropertyGroup::DelegateSubtype2);
}

// ************* QQStyleKitIndicatorProperties ****************

QQStyleKitIndicatorProperties::QQStyleKitIndicatorProperties(
    QQSK::PropertyGroup group, QQStyleKitControlProperties *parent)
    : QQStyleKitDelegateProperties(group, parent)
{
}

template <typename... CHANGED_SIGNALS>
void QQStyleKitIndicatorProperties::emitGlobally(
    QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const
{
    // Go through all instances of QQStyleKitIndicatorProperties
    const QQStyleKitControlProperties *cp = controlProperties();
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, indicator()->up());
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, indicator()->down());
}

QQStyleKitDelegateProperties *QQStyleKitIndicatorProperties::foreground() const
{
    return lazyCreateGroup(m_foreground, QQSK::PropertyGroup::Foreground);
}

// ************* QQStyleKitIndicatorWithSubTypes ****************

QQStyleKitIndicatorWithSubTypes::QQStyleKitIndicatorWithSubTypes(
    QQSK::PropertyGroup group, QQStyleKitControlProperties *parent)
    : QQStyleKitDelegateProperties(group, parent)
{
}

template <typename... CHANGED_SIGNALS>
void QQStyleKitIndicatorWithSubTypes::emitGlobally(
    QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const
{
    // Go through all instances of QQStyleKitIndicatorWithSubTypes
    const QQStyleKitControlProperties *cp = controlProperties();
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, indicator());
}

QQStyleKitDelegateProperties *QQStyleKitIndicatorWithSubTypes::foreground() const
{
    return lazyCreateGroup(m_foreground, QQSK::PropertyGroup::Foreground);
}

QQStyleKitIndicatorProperties *QQStyleKitIndicatorWithSubTypes::up() const
{
    return lazyCreateGroup(m_up, QQSK::PropertyGroup::DelegateSubtype1);
}

QQStyleKitIndicatorProperties *QQStyleKitIndicatorWithSubTypes::down() const
{
    return lazyCreateGroup(m_down, QQSK::PropertyGroup::DelegateSubtype2);
}

// ************* QQStyleKitTextProperties ****************
QQStyleKitTextProperties::QQStyleKitTextProperties(QQSK::PropertyGroup group, QQStyleKitControlProperties *parent)
    : QQStyleKitPropertyGroup(group, parent)
{
}

template <typename... CHANGED_SIGNALS>
void QQStyleKitTextProperties::emitGlobally(
    QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const
{
    const QQStyleKitControlProperties *cp = controlProperties();
    CONDITIONALLY_EMIT_SIGNALS_GLOBALLY_FOR(cp, text());
}

QColor QQStyleKitTextProperties::color() const
{
    return styleProperty<QColor>(QQSK::Property::Color);
}

void QQStyleKitTextProperties::setColor(const QColor &color)
{
    if (setStyleProperty(QQSK::Property::Color, color))
        handleStylePropertyChanged(&QQStyleKitTextProperties::colorChanged);
}

Qt::Alignment QQStyleKitTextProperties::alignment() const
{
    return styleProperty<Qt::Alignment>(QQSK::Property::Alignment);
}

void QQStyleKitTextProperties::setAlignment(Qt::Alignment alignment)
{
    if (setStyleProperty(QQSK::Property::Alignment, alignment))
        handleStylePropertyChanged(&QQStyleKitTextProperties::alignmentChanged);
}

bool QQStyleKitTextProperties::bold() const
{
    return styleProperty<bool>(QQSK::Property::Bold, false);
}

void QQStyleKitTextProperties::setBold(bool bold)
{
    if (setStyleProperty(QQSK::Property::Bold, bold))
        handleStylePropertyChanged(&QQStyleKitTextProperties::boldChanged);
}

bool QQStyleKitTextProperties::italic() const
{
    return styleProperty<bool>(QQSK::Property::Italic, false);
}

void QQStyleKitTextProperties::setItalic(bool italic)
{
    if (setStyleProperty(QQSK::Property::Italic, italic))
        handleStylePropertyChanged(&QQStyleKitTextProperties::italicChanged);
}

qreal QQStyleKitTextProperties::pointSize() const
{
    return styleProperty<qreal>(QQSK::Property::PointSize);
}

void QQStyleKitTextProperties::setPointSize(qreal pointSize)
{
    if (setStyleProperty(QQSK::Property::PointSize, pointSize))
        handleStylePropertyChanged(&QQStyleKitTextProperties::pointSizeChanged);
}

qreal QQStyleKitTextProperties::padding() const
{
    return styleProperty<qreal>(QQSK::Property::Padding);
}

void QQStyleKitTextProperties::setPadding(qreal padding)
{
    if (setStyleProperty(QQSK::Property::Padding, padding))
        handleStylePropertiesChanged<QQStyleKitTextProperties>(
            &QQStyleKitTextProperties::paddingChanged,
            &QQStyleKitTextProperties::leftPaddingChanged,
            &QQStyleKitTextProperties::rightPaddingChanged,
            &QQStyleKitTextProperties::topPaddingChanged,
            &QQStyleKitTextProperties::bottomPaddingChanged);
}

qreal QQStyleKitTextProperties::leftPadding() const
{
    return styleProperty<qreal>(QQSK::Property::LeftPadding, QQSK::Property::Padding);
}

void QQStyleKitTextProperties::setLeftPadding(qreal padding)
{
    if (setStyleProperty(QQSK::Property::LeftPadding, padding))
        handleStylePropertyChanged(&QQStyleKitTextProperties::leftPaddingChanged);
}

qreal QQStyleKitTextProperties::rightPadding() const
{
    return styleProperty<qreal>(QQSK::Property::RightPadding, QQSK::Property::Padding);
}

void QQStyleKitTextProperties::setRightPadding(qreal padding)
{
    if (setStyleProperty(QQSK::Property::RightPadding, padding))
        handleStylePropertyChanged(&QQStyleKitTextProperties::rightPaddingChanged);
}

qreal QQStyleKitTextProperties::topPadding() const
{
    return styleProperty<qreal>(QQSK::Property::TopPadding, QQSK::Property::Padding);
}

void QQStyleKitTextProperties::setTopPadding(qreal padding)
{
    if (setStyleProperty(QQSK::Property::TopPadding, padding))
        handleStylePropertyChanged(&QQStyleKitTextProperties::topPaddingChanged);
}

qreal QQStyleKitTextProperties::bottomPadding() const
{
    return styleProperty<qreal>(QQSK::Property::BottomPadding, QQSK::Property::Padding);
}

void QQStyleKitTextProperties::setBottomPadding(qreal padding)
{
    if (setStyleProperty(QQSK::Property::BottomPadding, padding))
        handleStylePropertyChanged(&QQStyleKitTextProperties::bottomPaddingChanged);
}

// ************* QQStyleKitControlProperties ****************

QQStyleKitControlProperties::QQStyleKitControlProperties(QQSK::PropertyGroup group, QObject *parent)
    : QQStyleKitPropertyGroup(group, parent)
{
    /* Calculate the free space storage ID space that can accommodate all unique style
     * properties that may be applied to a control. Since we'll prepend different states
     * and subtypes during the property propagation lookup phase later, we need to reserve
     * ID space for them both already now. More docs about the property space is written in
     * the implementation of PropertyPathId. */
    m_groupSpace.size = nestedGroupsStartSize;
    m_groupSpace.start = 0;

    if (group == QQSK::PropertyGroup::GlobalFlag) {
        /* A property path may include pseudo-groups that offers a convenient API for
         * reading properties with specific options applied. The 'global' group is one such
         * pseudo-group. When it is prefixed to a property path, it indicates that the property
         * should be read directly from the style, bypassing any active transitions that might
         * otherwise affect its value.
         * Note: The global group should be ignored when computing a PropertyPathId_t, as it
         * only affect _where_ the property should be read from, not its ID. */
        m_pathFlags.setFlag(QQSK::PropertyPathFlag::Global);
    }
}

QQStyleKitStyle *QQStyleKitControlProperties::style() const
{
    if (subclass() == QQSK::Subclass::QQStyleKitState) {
        /* A QQStyleKitControlState (and its subclasses) should always be a (grand)child of a
         * QQStyleKitStyle. And it belongs to that style, even it that style is not the
         * currently active application style. This is opposed to a QQStyleKitReader,
         * that normally belongs / communicates with the currently active style.
         * NOTE: a style can also be a fallback style for another style (which can be recursive,
         * meaning that a fallback style can also have its own fallback style, and so on). But
         * this function will return the nearest style, and not the root style */
        QObject *obj = parent();
        while (obj && !obj->metaObject()->inherits(&QQStyleKitStyle::staticMetaObject))
            obj = obj->parent();
        return obj ? static_cast<QQStyleKitStyle *>(obj) : nullptr;
    }

    /* A style reader belongs to the currently active application style. We could in theory
     * support being able to point a QQStyleKitReader to any style, which would basically
     * mean that you could mix controls from several styles inside the same application. But
     * there is currently no API (or use-case?) in Controls that lets you to do that, so its
     * disabled for now. */
    return QQStyleKitStyle::current();
}

QQSK::Subclass QQStyleKitControlProperties::subclass() const
{
    /* QQStyleKitControlProperties is subclassed by several different classes in this
     * framework. As such, it's basically just an interface because it only declares the
     * different properties that can be read or written to in a QQStyleKitStyle, such as
     * hovered.background.color or pressed.indicator.foreground.color. It says nothing
     * about how those properties are stored, instead that is up to each individual
     * subclass to decide */
    if (metaObject()->inherits(&QQStyleKitReader::staticMetaObject))
        return QQSK::Subclass::QQStyleKitReader;
    if (metaObject()->inherits(&QQStyleKitControlState::staticMetaObject))
        return QQSK::Subclass::QQStyleKitState;
    Q_UNREACHABLE();
}

QQStyleKitReader *QQStyleKitControlProperties::asQQStyleKitReader() const
{
    Q_ASSERT(subclass() == QQSK::Subclass::QQStyleKitReader);
    return static_cast<QQStyleKitReader *>(const_cast<QQStyleKitControlProperties *>(this));
}

QQStyleKitControlState *QQStyleKitControlProperties::asQQStyleKitState() const
{
    Q_ASSERT(subclass() == QQSK::Subclass::QQStyleKitState);
    Q_ASSERT(metaObject()->inherits(&QQStyleKitControlState::staticMetaObject));
    return static_cast<QQStyleKitControlState *>(const_cast<QQStyleKitControlProperties *>(this));
}

void QQStyleKitControlProperties::forEachUsedDelegate(
    std::function<void (QQStyleKitDelegateProperties *, QQSK::Delegate, const QString &)> f)
{
    // If adding more delegates here, remember to keep StyleKitAnimation.qml in sync
    if (m_background)
        f(m_background, QQSK::Delegate::Background, "background"_L1);

    if (m_indicator) {
        f(m_indicator, QQSK::Delegate::Indicator, "indicator"_L1);
        if (m_indicator->m_foreground)
            f(m_indicator->m_foreground, QQSK::Delegate::IndicatorForeground, "indicator.foreground"_L1);
        if (m_indicator->m_up) {
            f(m_indicator->m_up, QQSK::Delegate::IndicatorUp, "indicator.up"_L1);
            if (m_indicator->m_up->m_foreground)
                f(m_indicator->m_up->m_foreground, QQSK::Delegate::IndicatorUpForeground, "indicator.up.foreground"_L1);
        }
        if (m_indicator->m_down) {
            f(m_indicator->m_down, QQSK::Delegate::IndicatorDown, "indicator.down"_L1);
            if (m_indicator->m_down->m_foreground)
                f(m_indicator->m_down->m_foreground, QQSK::Delegate::IndicatorDownForeground, "indicator.down.foreground"_L1);
        }
    }

    if (m_handle) {
        f(m_handle, QQSK::Delegate::Handle, "handle"_L1);
        if (m_handle->m_first)
            f(m_handle->m_first, QQSK::Delegate::HandleFirst, "handle.first"_L1);
        if (m_handle->m_second)
            f(m_handle->m_second, QQSK::Delegate::HandleSecond, "handle.second"_L1);
    }
}

void QQStyleKitControlProperties::emitChangedForAllStyleProperties(EmitFlags emitFlags)
{
    /* This brute-force function will emit update signals for _all_ style properties
     * in the QQStyleKitStyle API. Doing so is typically needed after a style-, or theme
     * change, as we don't know which properties are affected by such a big change. */
    if (emitFlags.testFlag(EmitFlag::AllProperties)) {
        emit leftPaddingChanged();
        emit rightPaddingChanged();
        emit topPaddingChanged();
        emit bottomPaddingChanged();
        emit spacingChanged();
        emit transitionChanged();
        emit textChanged();
    }

    forEachUsedDelegate([=](QQStyleKitDelegateProperties *delegate, QQSK::Delegate, const QString &){
        delegate->emitChangedForAllStylePropertiesRecursive(emitFlags);
    });
}

template <typename... CHANGED_SIGNALS>
void QQStyleKitControlProperties::emitGlobally(
    QQStyleKitExtendableControlType controlType, CHANGED_SIGNALS... changedSignals) const
{
    for (QQStyleKitReader *reader : QQStyleKitReader::s_allReaders) {
        if (reader->type() != controlType)
            continue;
        ((reader->*changedSignals)(), ...);
    }
}

qreal QQStyleKitControlProperties::spacing() const
{
    return styleProperty<qreal>(QQSK::Property::Spacing);
}

void QQStyleKitControlProperties::setSpacing(qreal spacing)
{
    if (setStyleProperty(QQSK::Property::Spacing, spacing))
        handleStylePropertyChanged(&QQStyleKitControlProperties::spacingChanged);
}

qreal QQStyleKitControlProperties::padding() const
{
    return styleProperty<qreal>(QQSK::Property::Padding);
}

void QQStyleKitControlProperties::setPadding(qreal padding)
{
    if (setStyleProperty(QQSK::Property::Padding, padding))
        handleStylePropertiesChanged<QQStyleKitControlProperties>(
            &QQStyleKitControlProperties::paddingChanged,
            &QQStyleKitControlProperties::leftPaddingChanged,
            &QQStyleKitControlProperties::rightPaddingChanged,
            &QQStyleKitControlProperties::topPaddingChanged,
            &QQStyleKitControlProperties::bottomPaddingChanged);
}

qreal QQStyleKitControlProperties::leftPadding() const
{
    return styleProperty<qreal>(QQSK::Property::LeftPadding, QQSK::Property::Padding);
}

void QQStyleKitControlProperties::setLeftPadding(qreal leftPadding)
{
    if (setStyleProperty(QQSK::Property::LeftPadding, leftPadding))
        handleStylePropertyChanged(&QQStyleKitControlProperties::leftPaddingChanged);
}

qreal QQStyleKitControlProperties::rightPadding() const
{
    return styleProperty<qreal>(QQSK::Property::RightPadding, QQSK::Property::Padding);
}

void QQStyleKitControlProperties::setRightPadding(qreal rightPadding)
{
    if (setStyleProperty(QQSK::Property::RightPadding, rightPadding))
        handleStylePropertyChanged(&QQStyleKitControlProperties::rightPaddingChanged);
}

qreal QQStyleKitControlProperties::topPadding() const
{
    return styleProperty<qreal>(QQSK::Property::TopPadding, QQSK::Property::Padding);
}

void QQStyleKitControlProperties::setTopPadding(qreal topPadding)
{
    if (setStyleProperty(QQSK::Property::TopPadding, topPadding))
        handleStylePropertyChanged(&QQStyleKitControlProperties::topPaddingChanged);
}

qreal QQStyleKitControlProperties::bottomPadding() const
{
    return styleProperty<qreal>(QQSK::Property::BottomPadding, QQSK::Property::Padding);
}

void QQStyleKitControlProperties::setBottomPadding(qreal bottomPadding)
{
    if (setStyleProperty(QQSK::Property::BottomPadding, bottomPadding))
        handleStylePropertyChanged(&QQStyleKitControlProperties::bottomPaddingChanged);
}

QQuickTransition *QQStyleKitControlProperties::transition() const
{
    return styleProperty<QQuickTransition *>(QQSK::Property::Transition);
}

void QQStyleKitControlProperties::setTransition(QQuickTransition *transition)
{
    if (setStyleProperty(QQSK::Property::Transition, transition))
        handleStylePropertyChanged(&QQStyleKitControlProperties::transitionChanged);
}

QQStyleKitTextProperties *QQStyleKitControlProperties::text() const
{
    return lazyCreateGroup(m_text, QQSK::PropertyGroup::Text);
}

QQStyleKitDelegateProperties *QQStyleKitControlProperties::background() const
{
    return lazyCreateGroup(m_background, QQSK::PropertyGroup::Background);
}

QQStyleKitHandleProperties *QQStyleKitControlProperties::handle() const
{
    return lazyCreateGroup(m_handle, QQSK::PropertyGroup::Handle);
}

QQStyleKitIndicatorWithSubTypes *QQStyleKitControlProperties::indicator() const
{
    return lazyCreateGroup(m_indicator, QQSK::PropertyGroup::Indicator);
}

QT_END_NAMESPACE

#include "moc_qqstylekitcontrolproperties_p.cpp"
