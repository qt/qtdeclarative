// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickdesignersupportproperties_p.h"

#include "qqmldesignermetaobject_p.h"
#include "qquickdesignercustomobjectdata_p.h"

QT_BEGIN_NAMESPACE

static void addToPropertyNameListIfNotBlackListed(QQuickDesignerSupport::PropertyNameList *propertyNameList,
                                                  const QQuickDesignerSupport::PropertyName &propertyName)
{
    if (!QQuickDesignerSupportProperties::isPropertyBlackListed(propertyName))
        propertyNameList->append(propertyName);
}

void QQuickDesignerSupportProperties::createNewDynamicProperty(QObject *object,  QQmlEngine *engine, const QString &name)
{
    QQmlDesignerMetaObject::getNodeInstanceMetaObject(object, engine)->createNewDynamicProperty(name);
}

void QQuickDesignerSupportProperties::registerNodeInstanceMetaObject(QObject *object, QQmlEngine *engine)
{
    // we just create one and the ownership goes automatically to the object in nodeinstance see init method
    QQmlDesignerMetaObject::getNodeInstanceMetaObject(object, engine);
}

bool QQuickDesignerSupportProperties::hasFullImplementedListInterface(const QQmlListReference &list)
{
    return list.isValid() && list.canCount() && list.canAt() && list.canAppend() && list.canClear();
}

void QQuickDesignerSupportProperties::registerCustomData(QObject *object)
{
    QQuickDesignerCustomObjectData::registerData(object);
}

QVariant QQuickDesignerSupportProperties::getResetValue(QObject *object, const QQuickDesignerSupport::PropertyName &propertyName)
{
    return QQuickDesignerCustomObjectData::getResetValue(object, propertyName);
}

void QQuickDesignerSupportProperties::doResetProperty(QObject *object, QQmlContext *context, const QQuickDesignerSupport::PropertyName &propertyName)
{
    QQuickDesignerCustomObjectData::doResetProperty(object, context, propertyName);
}

bool QQuickDesignerSupportProperties::hasValidResetBinding(QObject *object, const QQuickDesignerSupport::PropertyName &propertyName)
{
    return QQuickDesignerCustomObjectData::hasValidResetBinding(object, propertyName);
}

bool QQuickDesignerSupportProperties::hasBindingForProperty(QObject *object,
                                                      QQmlContext *context,
                                                      const QQuickDesignerSupport::PropertyName &propertyName,
                                                      bool *hasChanged)
{
    return QQuickDesignerCustomObjectData::hasBindingForProperty(object, context, propertyName, hasChanged);
}

void QQuickDesignerSupportProperties::setPropertyBinding(QObject *object,
                                                   QQmlContext *context,
                                                   const QQuickDesignerSupport::PropertyName &propertyName,
                                                   const QString &expression)
{
    QQuickDesignerCustomObjectData::setPropertyBinding(object, context, propertyName, expression);
}

void QQuickDesignerSupportProperties::keepBindingFromGettingDeleted(QObject *object,
                                                              QQmlContext *context,
                                                              const QQuickDesignerSupport::PropertyName &propertyName)
{
    QQuickDesignerCustomObjectData::keepBindingFromGettingDeleted(object, context, propertyName);
}

bool QQuickDesignerSupportProperties::isPropertyQObject(const QMetaProperty &metaProperty)
{
    return metaProperty.metaType().flags().testFlag(QMetaType::PointerToQObject);
}


QObject *QQuickDesignerSupportProperties::readQObjectProperty(const QMetaProperty &metaProperty, QObject *object)
{
    return QQmlMetaType::toQObject(metaProperty.read(object));
}

void QQuickDesignerSupportProperties::getPropertyCache(QObject *object)
{
    QQmlMetaType::propertyCache(object->metaObject());
}

static QQuickDesignerSupport::PropertyNameList propertyNameListForWritableProperties(QObject *object,
                                                       const QQuickDesignerSupport::PropertyName &baseName,
                                                       QObjectList *inspectedObjects,
                                                       int depth = 0)
{
    QQuickDesignerSupport::PropertyNameList propertyNameList;

    if (depth > 2)
        return propertyNameList;

    if (!inspectedObjects->contains(object))
        inspectedObjects->append(object);

    const QMetaObject *metaObject = object->metaObject();
    for (int index = 0; index < metaObject->propertyCount(); ++index) {
        QMetaProperty metaProperty = metaObject->property(index);
        QQmlProperty declarativeProperty(object, QString::fromUtf8(metaProperty.name()));
        if (declarativeProperty.isValid() && !declarativeProperty.isWritable() && declarativeProperty.propertyTypeCategory() == QQmlProperty::Object) {
            if (declarativeProperty.name() != QLatin1String("parent")) {
                QObject *childObject = QQmlMetaType::toQObject(declarativeProperty.read());
                if (childObject)
                    propertyNameList.append(propertyNameListForWritableProperties(childObject,
                                                                                  baseName +  QQuickDesignerSupport::PropertyName(metaProperty.name())
                                                                                  + '.', inspectedObjects,
                                                                                  depth + 1));
            }
        } else if (QQmlGadgetPtrWrapper *valueType
                   = QQmlGadgetPtrWrapper::instance(qmlEngine(object), metaProperty.metaType())) {
            valueType->setValue(metaProperty.read(object));
            propertyNameList.append(propertyNameListForWritableProperties(valueType,
                                                                          baseName +  QQuickDesignerSupport::PropertyName(metaProperty.name())
                                                                          + '.', inspectedObjects,
                                                                          depth + 1));
        }

        if (metaProperty.isReadable() && metaProperty.isWritable()) {
            addToPropertyNameListIfNotBlackListed(&propertyNameList,
                                                  baseName + QQuickDesignerSupport::PropertyName(metaProperty.name()));
        }
    }

    return propertyNameList;
}

QQuickDesignerSupport::PropertyNameList QQuickDesignerSupportProperties::propertyNameListForWritableProperties(QObject *object)
{
    QObjectList localObjectList;
    return ::propertyNameListForWritableProperties(object, {}, &localObjectList);
}

bool QQuickDesignerSupportProperties::isPropertyBlackListed(const QQuickDesignerSupport::PropertyName &propertyName)
{
    if (propertyName.contains(".") && propertyName.contains("__"))
        return true;

    if (propertyName.count(".") > 1)
        return true;

    return false;
}

QQuickDesignerSupport::PropertyNameList QQuickDesignerSupportProperties::allPropertyNames(QObject *object,
                                  const QQuickDesignerSupport::PropertyName &baseName,
                                  QObjectList *inspectedObjects,
                                  int depth)
{
    QQuickDesignerSupport::PropertyNameList propertyNameList;

    QObjectList localObjectList;

    if (inspectedObjects == nullptr)
        inspectedObjects = &localObjectList;

    if (depth > 2)
        return propertyNameList;

    if (!inspectedObjects->contains(object))
        inspectedObjects->append(object);

    const QMetaObject *metaObject = object->metaObject();

    QStringList deferredPropertyNames;
    const int namesIndex = metaObject->indexOfClassInfo("DeferredPropertyNames");
    if (namesIndex != -1) {
        QMetaClassInfo classInfo = metaObject->classInfo(namesIndex);
        deferredPropertyNames = QString::fromUtf8(classInfo.value()).split(QLatin1Char(','));
    }

    for (int index = 0; index < metaObject->propertyCount(); ++index) {
        QMetaProperty metaProperty = metaObject->property(index);
        QQmlProperty declarativeProperty(object, QString::fromUtf8(metaProperty.name()));
        if (declarativeProperty.isValid() && declarativeProperty.propertyTypeCategory() == QQmlProperty::Object) {
            if (declarativeProperty.name() != QLatin1String("parent")
                    && !deferredPropertyNames.contains(declarativeProperty.name())) {
                QObject *childObject = QQmlMetaType::toQObject(declarativeProperty.read());
                if (childObject)
                    propertyNameList.append(allPropertyNames(childObject,
                                                             baseName
                                                             + QQuickDesignerSupport::PropertyName(metaProperty.name())
                                                             + '.', inspectedObjects,
                                                             depth + 1));
            }
        } else if (QQmlGadgetPtrWrapper *valueType
                   = QQmlGadgetPtrWrapper::instance(qmlEngine(object), metaProperty.metaType())) {
            valueType->setValue(metaProperty.read(object));
            propertyNameList.append(baseName + QQuickDesignerSupport::PropertyName(metaProperty.name()));
            propertyNameList.append(allPropertyNames(valueType,
                                                     baseName
                                                     + QQuickDesignerSupport::PropertyName(metaProperty.name())
                                                     + '.', inspectedObjects,
                                                     depth + 1));
        } else  {
            addToPropertyNameListIfNotBlackListed(&propertyNameList,
                                                  baseName + QQuickDesignerSupport::PropertyName(metaProperty.name()));
        }
    }

    return propertyNameList;
}


QT_END_NAMESPACE




