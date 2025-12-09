// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitcontrol_p.h"
#include "qqstylekitcontrols_p.h"
#include "qqstylekitpropertyresolver_p.h"

QT_BEGIN_NAMESPACE

int QQStyleKitControlAttached::s_variationCount = 0;

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

QQStyleKitControlAttached *QQStyleKitControl::qmlAttachedProperties(QObject *object)
{
    return new QQStyleKitControlAttached(object);
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

QQStyleKitControlAttached::QQStyleKitControlAttached(QObject *parent)
    : QObject(parent)
{
}

QStringList QQStyleKitControlAttached::variations() const
{
    return m_variations;
}

void QQStyleKitControlAttached::setVariations(const QStringList &variations)
{
    if (m_variations == variations)
        return;

    /* As an optimization, we count the number of variations set from the application.
     * That way, if s_variationCount == 1, for example, and we found a variation while
     * resolving the effective variations for a specific QQStyleReader, we can stop the search. */
    s_variationCount++;

    m_variations = variations;
    emit variationsChanged();
}

QQStyleKitExtendableControlType QQStyleKitControlAttached::controlType()
{
    return m_controlType;
}

void QQStyleKitControlAttached::setControlType(QQStyleKitExtendableControlType type)
{
    if (m_controlType == type)
        return;

    m_controlType = type;
    emit controlTypeChanged();
}

QT_END_NAMESPACE

#include "moc_qqstylekitcontrol_p.cpp"
