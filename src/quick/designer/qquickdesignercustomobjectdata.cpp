// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdesignersupportmetainfo_p.h"
#include "qquickdesignersupportproperties_p.h"

#include "qquickdesignercustomobjectdata_p.h"

#include <QGlobalStatic>
#include <QQmlContext>
#include <QQmlEngine>

#include <private/qqmlanybinding_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

typedef QHash<QObject*, QQuickDesignerCustomObjectData*> CustomObjectDataHash;
Q_GLOBAL_STATIC(CustomObjectDataHash, s_designerObjectToDataHash)

struct HandleDestroyedFunctor {
  QQuickDesignerCustomObjectData *data;
  void operator()() { data->handleDestroyed(); }
};

QQuickDesignerCustomObjectData::QQuickDesignerCustomObjectData(QObject *object)
    : m_object(object)
{
    if (object) {
        populateResetHashes();
        s_designerObjectToDataHash()->insert(object, this);

        HandleDestroyedFunctor functor;
        functor.data = this;
        QObject::connect(object, &QObject::destroyed, functor);
    }
}

void QQuickDesignerCustomObjectData::registerData(QObject *object)
{
    new QQuickDesignerCustomObjectData(object);
}

QQuickDesignerCustomObjectData *QQuickDesignerCustomObjectData::get(QObject *object)
{
    return s_designerObjectToDataHash()->value(object);
}

QVariant QQuickDesignerCustomObjectData::getResetValue(QObject *object, const QQuickDesignerSupport::PropertyName &propertyName)
{
    QQuickDesignerCustomObjectData* data = get(object);

    if (data)
        return data->getResetValue(propertyName);

    return QVariant();
}

void QQuickDesignerCustomObjectData::doResetProperty(QObject *object, QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName)
{
    QQuickDesignerCustomObjectData* data = get(object);

    if (data)
        data->doResetProperty(context, propertyName);
}

bool QQuickDesignerCustomObjectData::hasValidResetBinding(QObject *object, const QQuickDesignerSupport::PropertyName &propertyName)
{
    QQuickDesignerCustomObjectData* data = get(object);

    if (data)
        return data->hasValidResetBinding(propertyName);

    return false;
}

bool QQuickDesignerCustomObjectData::hasBindingForProperty(QObject *object,
                                                     QQmlContext *context,
                                                     const QQuickDesignerSupport::PropertyName &propertyName,
                                                     bool *hasChanged)
{
    QQuickDesignerCustomObjectData* data = get(object);

    if (data)
        return data->hasBindingForProperty(context, propertyName, hasChanged);

    return false;
}

void QQuickDesignerCustomObjectData::setPropertyBinding(QObject *object,
                                                  QQmlContext *context,
                                                  const QQuickDesignerSupport::PropertyName &propertyName,
                                                  const QString &expression)
{
    QQuickDesignerCustomObjectData* data = get(object);

    if (data)
        data->setPropertyBinding(context, propertyName, expression);
}

void QQuickDesignerCustomObjectData::keepBindingFromGettingDeleted(QObject *object,
                                                             QQmlContext *context,
                                                             const QQuickDesignerSupport::PropertyName &propertyName)
{
    QQuickDesignerCustomObjectData* data = get(object);

    if (data)
        data->keepBindingFromGettingDeleted(context, propertyName);
}

void QQuickDesignerCustomObjectData::populateResetHashes()
{
    const QQuickDesignerSupport::PropertyNameList propertyNameList =
            QQuickDesignerSupportProperties::propertyNameListForWritableProperties(object());

    const QMetaObject *mo = object()->metaObject();
    QByteArrayList deferredPropertyNames;
    const int namesIndex = mo->indexOfClassInfo("DeferredPropertyNames");
    if (namesIndex != -1) {
        QMetaClassInfo classInfo = mo->classInfo(namesIndex);
        deferredPropertyNames = QByteArray(classInfo.value()).split(',');
    }

    for (const QQuickDesignerSupport::PropertyName &propertyName : propertyNameList) {

        if (deferredPropertyNames.contains(propertyName))
            continue;

        QQmlProperty property(object(), QString::fromUtf8(propertyName), QQmlEngine::contextForObject(object()));

        auto binding = QQmlAnyBinding::ofProperty(property);

        if (binding) {
            m_resetBindingHash.insert(propertyName, binding);
        } else if (property.isWritable()) {
            m_resetValueHash.insert(propertyName, property.read());
        }
    }
}

QObject *QQuickDesignerCustomObjectData::object() const
{
    return m_object;
}

QVariant QQuickDesignerCustomObjectData::getResetValue(const QQuickDesignerSupport::PropertyName &propertyName) const
{
    return m_resetValueHash.value(propertyName);
}

void QQuickDesignerCustomObjectData::doResetProperty(QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName)
{
    QQmlProperty property(object(), QString::fromUtf8(propertyName), context);

    if (!property.isValid())
        return;

    // remove existing binding
    QQmlAnyBinding::takeFrom(property);


    if (hasValidResetBinding(propertyName)) {
        QQmlAnyBinding binding = getResetBinding(propertyName);
        binding.installOn(property);

        if (binding.isAbstractPropertyBinding()) {
            // for new style properties, we will evaluate during setBinding anyway
            static_cast<QQmlBinding *>(binding.asAbstractBinding())->update();
        }

    } else if (property.isResettable()) {
        property.reset();
    } else if (property.propertyTypeCategory() == QQmlProperty::List) {
        QQmlListReference list = qvariant_cast<QQmlListReference>(property.read());

        if (!QQuickDesignerSupportProperties::hasFullImplementedListInterface(list)) {
            qWarning() << "Property list interface not fully implemented for Class " << property.property().typeName() << " in property " << property.name() << "!";
            return;
        }

        list.clear();
    } else if (property.isWritable()) {
        if (property.read() == getResetValue(propertyName))
            return;

        property.write(getResetValue(propertyName));
    }
}

bool QQuickDesignerCustomObjectData::hasValidResetBinding(const QQuickDesignerSupport::PropertyName &propertyName) const
{
    return m_resetBindingHash.contains(propertyName) &&  m_resetBindingHash.value(propertyName);
}

QQmlAnyBinding QQuickDesignerCustomObjectData::getResetBinding(const QQuickDesignerSupport::PropertyName &propertyName) const
{
    return m_resetBindingHash.value(propertyName);
}

bool QQuickDesignerCustomObjectData::hasBindingForProperty(QQmlContext *context,
                                                     const QQuickDesignerSupport::PropertyName &propertyName,
                                                     bool *hasChanged) const
{
    if (QQuickDesignerSupportProperties::isPropertyBlackListed(propertyName))
        return false;

    QQmlProperty property(object(), QString::fromUtf8(propertyName), context);

    bool hasBinding = QQmlAnyBinding::ofProperty(property);

    if (hasChanged) {
        *hasChanged = hasBinding != m_hasBindingHash.value(propertyName, false);
        if (*hasChanged)
            m_hasBindingHash.insert(propertyName, hasBinding);
    }

    return hasBinding;
}

void QQuickDesignerCustomObjectData::setPropertyBinding(QQmlContext *context,
                                                  const QQuickDesignerSupport::PropertyName &propertyName,
                                                  const QString &expression)
{
    QQmlProperty property(object(), QString::fromUtf8(propertyName), context);

    if (!property.isValid())
        return;

    if (property.isProperty()) {
        QString url = u"@designer"_s;
        int lineNumber = 0;
        QQmlAnyBinding binding = QQmlAnyBinding::createFromCodeString(property,
                                                                      expression, object(), QQmlContextData::get(context), url, lineNumber);

        binding.installOn(property);
        if (binding.isAbstractPropertyBinding()) {
            // for new style properties, we will evaluate during setBinding anyway
            static_cast<QQmlBinding *>(binding.asAbstractBinding())->update();
        }

        if (binding.hasError()) {
            if (property.property().userType() == QMetaType::QString)
                property.write(QVariant(QLatin1Char('#') + expression + QLatin1Char('#')));
        }

    } else {
        qWarning() << Q_FUNC_INFO << ": Cannot set binding for property" << propertyName << ": property is unknown for type";
    }
}

void QQuickDesignerCustomObjectData::keepBindingFromGettingDeleted(QQmlContext *context,
                                                             const QQuickDesignerSupport::PropertyName &propertyName)
{
    //Refcounting is taking care
    Q_UNUSED(context);
    Q_UNUSED(propertyName);
}

void QQuickDesignerCustomObjectData::handleDestroyed()
{
    s_designerObjectToDataHash()->remove(m_object);
    delete this;
}

QT_END_NAMESPACE

