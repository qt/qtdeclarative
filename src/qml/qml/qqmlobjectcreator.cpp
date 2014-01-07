/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlobjectcreator_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlvmemetaobject_p.h>
#include <private/qv4function_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlstringconverters_p.h>
#include <private/qqmlboundsignal_p.h>
#include <private/qqmltrace_p.h>
#include <private/qqmlcomponentattached_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlcustomparser_p.h>

QT_USE_NAMESPACE

namespace {
struct ActiveOCRestorer
{
    ActiveOCRestorer(QmlObjectCreator *creator, QQmlEnginePrivate *ep)
    : ep(ep), oldCreator(ep->activeObjectCreator) { ep->activeObjectCreator = creator; }
    ~ActiveOCRestorer() { ep->activeObjectCreator = oldCreator; }

    QQmlEnginePrivate *ep;
    QmlObjectCreator *oldCreator;
};
}

static void removeBindingOnProperty(QObject *o, int index)
{
    int coreIndex = index & 0x0000FFFF;
    int valueTypeIndex = (index & 0xFFFF0000 ? index >> 16 : -1);

    QQmlAbstractBinding *binding = QQmlPropertyPrivate::setBinding(o, coreIndex, valueTypeIndex, 0);
    if (binding) binding->destroy();
}

QmlObjectCreator::QmlObjectCreator(QQmlContextData *parentContext, QQmlCompiledData *compiledData)
    : componentAttached(0)
    , url(compiledData->url)
    , engine(parentContext->engine)
    , qmlUnit(compiledData->qmlUnit)
    , jsUnit(compiledData->compilationUnit)
    , parentContext(parentContext)
    , context(0)
    , resolvedTypes(compiledData->resolvedTypes)
    , propertyCaches(compiledData->propertyCaches)
    , vmeMetaObjectData(compiledData->datas)
    , compiledData(compiledData)
    , rootContext(0)
    , _qobject(0)
    , _qobjectForBindings(0)
    , _valueTypeProperty(0)
    , _compiledObject(0)
    , _ddata(0)
    , _propertyCache(0)
    , _vmeMetaObject(0)
    , _qmlContext(0)
{
    if (!compiledData->isInitialized())
        compiledData->initialize(engine);
}

QObject *QmlObjectCreator::create(int subComponentIndex, QObject *parent)
{
    int objectToCreate;

    if (subComponentIndex == -1) {
        objectIndexToId = compiledData->objectIndexToIdForRoot;
        objectToCreate = qmlUnit->indexOfRootObject;
    } else {
        objectIndexToId = compiledData->objectIndexToIdPerComponent[subComponentIndex];
        const QV4::CompiledData::Object *compObj = qmlUnit->objectAt(subComponentIndex);
        objectToCreate = compObj->bindingTable()->value.objectIndex;
    }

    context = new QQmlContextData;
    context->isInternal = true;
    context->url = compiledData->url;
    context->urlString = compiledData->name;
    context->imports = compiledData->importCache;
    context->imports->addref();
    context->setParent(parentContext);

    if (!rootContext)
        rootContext = context;

    QVector<QQmlContextData::ObjectIdMapping> mapping(objectIndexToId.count());
    for (QHash<int, int>::ConstIterator it = objectIndexToId.constBegin(), end = objectIndexToId.constEnd();
         it != end; ++it) {
        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(it.key());

        QQmlContextData::ObjectIdMapping m;
        m.id = it.value();
        m.name = stringAt(obj->idIndex);
        mapping[m.id] = m;
    }
    context->setIdPropertyData(mapping);

    if (subComponentIndex == -1) {
        QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
        QV4::Scope scope(v4);
        QV4::ScopedObject scripts(scope, v4->newArrayObject(compiledData->scripts.count()));
        for (int i = 0; i < compiledData->scripts.count(); ++i) {
            QQmlScriptData *s = compiledData->scripts.at(i);
            scripts->putIndexed(i, s->scriptValueForContext(context));
        }
        context->importedScripts = scripts;
    } else if (parentContext) {
        context->importedScripts = parentContext->importedScripts;
    }

    QVector<QQmlParserStatus*> parserStatusCallbacks;
    parserStatusCallbacks.resize(qmlUnit->nObjects);
    qSwap(_parserStatusCallbacks, parserStatusCallbacks);

    QObject *instance = createInstance(objectToCreate, parent);
    if (instance) {
        QQmlData *ddata = QQmlData::get(instance);
        Q_ASSERT(ddata);
        ddata->compiledData = compiledData;
        ddata->compiledData->addref();

        context->contextObject = instance;
    }

    qSwap(_parserStatusCallbacks, parserStatusCallbacks);
    allParserStatusCallbacks.prepend(parserStatusCallbacks);

    return instance;
}

void QmlObjectCreator::setPropertyValue(QQmlPropertyData *property, const QV4::CompiledData::Binding *binding)
{
    QQmlPropertyPrivate::WriteFlags propertyWriteFlags = QQmlPropertyPrivate::BypassInterceptor |
                                                               QQmlPropertyPrivate::RemoveBindingOnAliasWrite;
    int propertyWriteStatus = -1;
    void *argv[] = { 0, 0, &propertyWriteStatus, &propertyWriteFlags };

    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
    QV4::Scope scope(v4);
    // ### enums

    switch (property->propType) {
    case QMetaType::QVariant: {
        if (binding->type == QV4::CompiledData::Binding::Type_Number) {
            double n = binding->valueAsNumber();
            if (double(int(n)) == n) {
                if (property->isVarProperty()) {
                    _vmeMetaObject->setVMEProperty(property->coreIndex, QV4::Primitive::fromInt32(int(n)));
                } else {
                    int i = int(n);
                    QVariant value(i);
                    argv[0] = &value;
                    QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
                }
            } else {
                if (property->isVarProperty()) {
                    _vmeMetaObject->setVMEProperty(property->coreIndex, QV4::Primitive::fromDouble(n));
                } else {
                    QVariant value(n);
                    argv[0] = &value;
                    QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
                }
            }
        } else if (binding->type == QV4::CompiledData::Binding::Type_Boolean) {
            if (property->isVarProperty()) {
                _vmeMetaObject->setVMEProperty(property->coreIndex, QV4::Primitive::fromBoolean(binding->valueAsBoolean()));
            } else {
                QVariant value(binding->valueAsBoolean());
                argv[0] = &value;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            }
        } else {
            QString stringValue = binding->valueAsString(&qmlUnit->header);
            if (property->isVarProperty()) {
                QV4::ScopedString s(scope, v4->newString(stringValue));
                _vmeMetaObject->setVMEProperty(property->coreIndex, s);
            } else {
                QVariant value = QQmlStringConverters::variantFromString(stringValue);
                argv[0] = &value;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            }
        }
    }
    break;
    case QVariant::String: {
        if (binding->type == QV4::CompiledData::Binding::Type_String) {
            QString value = binding->valueAsString(&qmlUnit->header);
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: string expected"));
        }
    }
    break;
    case QVariant::StringList: {
        if (binding->type == QV4::CompiledData::Binding::Type_String) {
            QStringList value(binding->valueAsString(&qmlUnit->header));
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: string or string list expected"));
        }
    }
    break;
    case QVariant::ByteArray: {
        if (binding->type == QV4::CompiledData::Binding::Type_String) {
            QByteArray value(binding->valueAsString(&qmlUnit->header).toUtf8());
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: byte array expected"));
        }
    }
    break;
    case QVariant::Url: {
        if (binding->type == QV4::CompiledData::Binding::Type_String) {
            QString string = binding->valueAsString(&qmlUnit->header);
            // Encoded dir-separators defeat QUrl processing - decode them first
            string.replace(QLatin1String("%2f"), QLatin1String("/"), Qt::CaseInsensitive);
            QUrl value = string.isEmpty() ? QUrl() : this->url.resolved(QUrl(string));
            // Apply URL interceptor
            if (engine->urlInterceptor())
                value = engine->urlInterceptor()->intercept(value, QQmlAbstractUrlInterceptor::UrlString);
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: url expected"));
        }
    }
    break;
    case QVariant::UInt: {
        if (binding->type == QV4::CompiledData::Binding::Type_Number) {
            double d = binding->valueAsNumber();
            if (double(uint(d)) == d) {
                uint value = uint(d);
                argv[0] = &value;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
                break;
            }
        }
        recordError(binding->location, tr("Invalid property assignment: unsigned int expected"));
    }
    break;
    case QVariant::Int: {
        if (binding->type == QV4::CompiledData::Binding::Type_Number) {
            double d = binding->valueAsNumber();
            if (double(int(d)) == d) {
                int value = int(d);
                argv[0] = &value;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
                break;
            }
        }
        recordError(binding->location, tr("Invalid property assignment: int expected"));
    }
    break;
    case QMetaType::Float: {
        if (binding->type == QV4::CompiledData::Binding::Type_Number) {
            float value = float(binding->valueAsNumber());
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: number expected"));
        }
    }
    break;
    case QVariant::Double: {
        if (binding->type == QV4::CompiledData::Binding::Type_Number) {
            double value = binding->valueAsNumber();
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: number expected"));
        }
    }
    break;
    case QVariant::Color: {
        bool ok = false;
        uint colorValue = QQmlStringConverters::rgbaFromString(binding->valueAsString(&qmlUnit->header), &ok);

        if (ok) {
            struct { void *data[4]; } buffer;
            if (QQml_valueTypeProvider()->storeValueType(property->propType, &colorValue, &buffer, sizeof(buffer))) {
                argv[0] = reinterpret_cast<void *>(&buffer);
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            }
        } else {
            recordError(binding->location, tr("Invalid property assignment: color expected"));
        }
    }
    break;
#ifndef QT_NO_DATESTRING
    case QVariant::Date: {
        bool ok = false;
        QDate value = QQmlStringConverters::dateFromString(binding->valueAsString(&qmlUnit->header), &ok);
        if (ok) {
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: date expected"));
        }
    }
    break;
    case QVariant::Time: {
        bool ok = false;
        QTime value = QQmlStringConverters::timeFromString(binding->valueAsString(&qmlUnit->header), &ok);
        if (ok) {
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: time expected"));
        }
    }
    break;
    case QVariant::DateTime: {
        bool ok = false;
        QDateTime value = QQmlStringConverters::dateTimeFromString(binding->valueAsString(&qmlUnit->header), &ok);
        if (ok) {
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: datetime expected"));
        }
    }
    break;
#endif // QT_NO_DATESTRING
    case QVariant::Point: {
        bool ok = false;
        QPoint value = QQmlStringConverters::pointFFromString(binding->valueAsString(&qmlUnit->header), &ok).toPoint();
        if (ok) {
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: point expected"));
        }
    }
    break;
    case QVariant::PointF: {
        bool ok = false;
        QPointF value = QQmlStringConverters::pointFFromString(binding->valueAsString(&qmlUnit->header), &ok);
        if (ok) {
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: point expected"));
        }
    }
    break;
    case QVariant::Size: {
        bool ok = false;
        QSize value = QQmlStringConverters::sizeFFromString(binding->valueAsString(&qmlUnit->header), &ok).toSize();
        if (ok) {
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: size expected"));
        }
    }
    break;
    case QVariant::SizeF: {
        bool ok = false;
        QSizeF value = QQmlStringConverters::sizeFFromString(binding->valueAsString(&qmlUnit->header), &ok);
        if (ok) {
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: size expected"));
        }
    }
    break;
    case QVariant::Rect: {
        bool ok = false;
        QRect value = QQmlStringConverters::rectFFromString(binding->valueAsString(&qmlUnit->header), &ok).toRect();
        if (ok) {
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: point expected"));
        }
    }
    break;
    case QVariant::RectF: {
        bool ok = false;
        QRectF value = QQmlStringConverters::rectFFromString(binding->valueAsString(&qmlUnit->header), &ok);
        if (ok) {
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: point expected"));
        }
    }
    break;
    case QVariant::Bool: {
        if (binding->type == QV4::CompiledData::Binding::Type_Boolean) {
            bool value = binding->valueAsBoolean();
            argv[0] = &value;
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: boolean expected"));
        }
    }
    break;
    case QVariant::Vector3D: {
        struct {
            float xp;
            float yp;
            float zy;
        } vec;
        if (QQmlStringConverters::createFromString(QMetaType::QVector3D, binding->valueAsString(&qmlUnit->header), &vec, sizeof(vec))) {
            argv[0] = reinterpret_cast<void *>(&vec);
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: 3D vector expected"));
        }
    }
    break;
    case QVariant::Vector4D: {
        struct {
            float xp;
            float yp;
            float zy;
            float wp;
        } vec;
        if (QQmlStringConverters::createFromString(QMetaType::QVector4D, binding->valueAsString(&qmlUnit->header), &vec, sizeof(vec))) {
            argv[0] = reinterpret_cast<void *>(&vec);
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: 4D vector expected"));
        }
    }
    break;
    case QVariant::RegExp:
        recordError(binding->location, tr("Invalid property assignment: regular expression expected; use /pattern/ syntax"));
        break;
    default: {
        // generate single literal value assignment to a list property if required
        if (property->propType == qMetaTypeId<QList<qreal> >()) {
            if (binding->type == QV4::CompiledData::Binding::Type_Number) {
                QList<qreal> value;
                value.append(binding->valueAsNumber());
                argv[0] = reinterpret_cast<void *>(&value);
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            } else {
                recordError(binding->location, tr("Invalid property assignment: real or array of reals expected"));
            }
            break;
        } else if (property->propType == qMetaTypeId<QList<int> >()) {
            if (binding->type == QV4::CompiledData::Binding::Type_Number) {
                double n = binding->valueAsNumber();
                if (double(int(n)) == n) {
                    QList<int> value;
                    value.append(int(n));
                    argv[0] = reinterpret_cast<void *>(&value);
                    QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
                    break;
                } else {
                    recordError(binding->location, tr("Invalid property assignment: int or array of ints expected"));
                }
            }
            break;
        } else if (property->propType == qMetaTypeId<QList<bool> >()) {
            if (binding->type == QV4::CompiledData::Binding::Type_Boolean) {
                QList<bool> value;
                value.append(binding->valueAsBoolean());
                argv[0] = reinterpret_cast<void *>(&value);
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            } else {
                recordError(binding->location, tr("Invalid property assignment: bool or array of bools expected"));
            }
            break;
        } else if (property->propType == qMetaTypeId<QList<QUrl> >()) {
            if (binding->type == QV4::CompiledData::Binding::Type_String) {
                QString urlString = binding->valueAsString(&qmlUnit->header);
                QUrl u = urlString.isEmpty() ? QUrl() : this->url.resolved(QUrl(urlString));
                QList<QUrl> value;
                value.append(u);
                argv[0] = reinterpret_cast<void *>(&value);
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            } else {
                recordError(binding->location, tr("Invalid property assignment: url or array of urls expected"));
            }
            break;
        } else if (property->propType == qMetaTypeId<QList<QString> >()) {
            if (binding->type == QV4::CompiledData::Binding::Type_String) {
                QList<QString> value;
                value.append(binding->valueAsString(&qmlUnit->header));
                argv[0] = reinterpret_cast<void *>(&value);
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            } else {
                recordError(binding->location, tr("Invalid property assignment: string or array of strings expected"));
            }
            break;
        } else if (property->propType == qMetaTypeId<QJSValue>()) {
            QJSValue value;
            if (binding->type == QV4::CompiledData::Binding::Type_Boolean) {
                value = QJSValue(binding->valueAsBoolean());
            } else if (binding->type == QV4::CompiledData::Binding::Type_Number) {
                double n = binding->valueAsNumber();
                if (double(int(n)) == n) {
                    value = QJSValue(int(n));
                } else
                    value = QJSValue(n);
            } else {
                value = QJSValue(binding->valueAsString(&qmlUnit->header));
            }
            argv[0] = reinterpret_cast<void *>(&value);
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            break;
        }

        // otherwise, try a custom type assignment
        QString stringValue = binding->valueAsString(&qmlUnit->header);
        QQmlMetaType::StringConverter converter = QQmlMetaType::customStringConverter(property->propType);
        if (converter) {
            QVariant value = (*converter)(stringValue);

            QMetaProperty metaProperty = _qobject->metaObject()->property(property->coreIndex);
            if (value.isNull() || ((int)metaProperty.type() != property->propType && metaProperty.userType() != property->propType)) {
                recordError(binding->location, tr("Cannot assign value %1 to property %2").arg(stringValue).arg(QString::fromUtf8(metaProperty.name())));
                break;
            }

            argv[0] = value.data();
            QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
        } else {
            recordError(binding->location, tr("Invalid property assignment: unsupported type \"%1\"").arg(QString::fromLatin1(QMetaType::typeName(property->propType))));
        }
    }
    break;
    }
}

void QmlObjectCreator::setupBindings()
{
    QQmlListProperty<void> savedList;
    qSwap(_currentList, savedList);

    QQmlPropertyData *property = 0;
    bool defaultPropertyQueried = false;
    QQmlPropertyData *defaultProperty = 0;

    QString id = stringAt(_compiledObject->idIndex);
    if (!id.isEmpty()) {
        QQmlPropertyData *idProperty = _propertyCache->property(QStringLiteral("id"), _qobject, context);
        if (idProperty) {
            QV4::CompiledData::Binding idBinding;
            idBinding.propertyNameIndex = 0; // Not used
            idBinding.flags = 0;
            idBinding.type = QV4::CompiledData::Binding::Type_String;
            idBinding.stringIndex = _compiledObject->idIndex;
            idBinding.location = _compiledObject->location; // ###
            setPropertyValue(idProperty, &idBinding);
        }
    }

    const QV4::CompiledData::Binding *binding = _compiledObject->bindingTable();
    for (quint32 i = 0; i < _compiledObject->nBindings; ++i, ++binding) {

        QString name = stringAt(binding->propertyNameIndex);
        if (name.isEmpty())
            property = 0;

        if (!property || (i > 0 && (binding - 1)->propertyNameIndex != binding->propertyNameIndex)) {
            if (!name.isEmpty())
                property = _propertyCache->property(name, _qobject, context);
            else {
                if (!defaultPropertyQueried) {
                    defaultProperty = _propertyCache->defaultProperty();
                    defaultPropertyQueried = true;
                }
                property = defaultProperty;
            }

            if (property && property->isQList()) {
                void *argv[1] = { (void*)&_currentList };
                QMetaObject::metacall(_qobject, QMetaObject::ReadProperty, property->coreIndex, argv);
            } else if (_currentList.object)
                _currentList = QQmlListProperty<void>();

        }

        if (!setPropertyValue(property, i, binding))
            return;
    }

    qSwap(_currentList, savedList);
}

bool QmlObjectCreator::setPropertyValue(QQmlPropertyData *property, int bindingIndex, const QV4::CompiledData::Binding *binding)
{
    if (binding->type == QV4::CompiledData::Binding::Type_AttachedProperty) {
        Q_ASSERT(stringAt(qmlUnit->objectAt(binding->value.objectIndex)->inheritedTypeNameIndex).isEmpty());
        QQmlType *attachedType = resolvedTypes.value(binding->propertyNameIndex).type;
        const int id = attachedType->attachedPropertiesId();
        QObject *qmlObject = qmlAttachedPropertiesObjectById(id, _qobject);
        QQmlRefPointer<QQmlPropertyCache> cache = QQmlEnginePrivate::get(engine)->cache(qmlObject);
        if (!populateInstance(binding->value.objectIndex, qmlObject, cache, _qobject, /*value type property*/0))
            return false;
        return true;
    }

    QObject *createdSubObject = 0;
    if (binding->type == QV4::CompiledData::Binding::Type_Object) {
        createdSubObject = createInstance(binding->value.objectIndex, _qobject);
        if (!createdSubObject)
            return false;
    }

    if (!property) // ### error
        return true;

    if (binding->type == QV4::CompiledData::Binding::Type_GroupProperty) {
        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(binding->value.objectIndex);
        if (stringAt(obj->inheritedTypeNameIndex).isEmpty()) {

            QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);
            QQmlRefPointer<QQmlPropertyCache> groupedObjCache;
            QObject *groupedObjInstance = 0;
            QObject *objForBindings = _qobjectForBindings;
            QQmlValueType *valueType = 0;
            QQmlPropertyData *valueTypeProperty = 0;

            if (QQmlValueTypeFactory::isValueType(property->propType)) {
                valueType = QQmlValueTypeFactory::valueType(property->propType);
                if (!valueType) {
                    recordError(binding->location, tr("Cannot set properties on %1 as it is null").arg(stringAt(binding->propertyNameIndex)));
                    return false;
                }

                valueType->read(_qobject, property->coreIndex);

                groupedObjCache = enginePrivate->cache(valueType);
                groupedObjInstance = valueType;
                valueTypeProperty = property;
            } else {
                groupedObjCache = enginePrivate->propertyCacheForType(property->propType);
                if (!groupedObjCache) {
                    recordError(binding->location, tr("Invalid grouped property access"));
                    return false;
                }

                void *argv[1] = { &groupedObjInstance };
                QMetaObject::metacall(_qobject, QMetaObject::ReadProperty, property->coreIndex, argv);
                if (!groupedObjInstance) {
                    recordError(binding->location, tr("Cannot set properties on %1 as it is null").arg(stringAt(binding->propertyNameIndex)));
                    return false;
                }

                objForBindings = groupedObjInstance;
            }

            if (!populateInstance(binding->value.objectIndex, groupedObjInstance, groupedObjCache, objForBindings, valueTypeProperty))
                return false;

            if (valueType)
                valueType->write(_qobject, property->coreIndex, QQmlPropertyPrivate::BypassInterceptor);

            return true;
        }
    }

    if (_ddata->hasBindingBit(property->coreIndex))
        removeBindingOnProperty(_qobject, property->coreIndex);

    if (binding->type == QV4::CompiledData::Binding::Type_Script) {
        QV4::Function *runtimeFunction = jsUnit->runtimeFunctions[binding->value.compiledScriptIndex];

        QV4::Scope scope(_qmlContext);
        QV4::ScopedFunctionObject function(scope, QV4::FunctionObject::creatScriptFunction(_qmlContext, runtimeFunction));

        if (binding->flags & QV4::CompiledData::Binding::IsSignalHandlerExpression) {
            int signalIndex = _propertyCache->methodIndexToSignalIndex(property->coreIndex);
            QQmlBoundSignal *bs = new QQmlBoundSignal(_qobject, signalIndex, _qobject, engine);
            QQmlBoundSignalExpression *expr = new QQmlBoundSignalExpression(_qobject, signalIndex,
                                                                            context, _qobject, function);

            bs->takeExpression(expr);
        } else {
            QQmlBinding *qmlBinding = new QQmlBinding(function, _qobject, context,
                                                      QString(), 0, 0); // ###

            // When writing bindings to grouped properties implemented as value types,
            // such as point.x: { someExpression; }, then the binding is installed on
            // the point property (_qobjectForBindings) and after evaluating the expression,
            // the result is written to a value type virtual property, that contains the sub-index
            // of the "x" property.
            QQmlPropertyData targetCorePropertyData = *property;
            if (_valueTypeProperty)
                targetCorePropertyData = QQmlPropertyPrivate::saveValueType(*_valueTypeProperty, _qobject->metaObject(), property->coreIndex, engine);

            qmlBinding->setTarget(_qobjectForBindings, targetCorePropertyData, context);
            qmlBinding->addToObject();

            _createdBindings[bindingIndex] = qmlBinding;
            qmlBinding->m_mePtr = &_createdBindings[bindingIndex];
        }
        return true;
    }

    if (binding->type == QV4::CompiledData::Binding::Type_Object) {
        QQmlPropertyPrivate::WriteFlags propertyWriteFlags = QQmlPropertyPrivate::BypassInterceptor |
                                                                   QQmlPropertyPrivate::RemoveBindingOnAliasWrite;
        int propertyWriteStatus = -1;
        void *argv[] = { 0, 0, &propertyWriteStatus, &propertyWriteFlags };

        if (const char *iid = QQmlMetaType::interfaceIId(property->propType)) {
            void *ptr = createdSubObject->qt_metacast(iid);
            if (ptr) {
                argv[0] = &ptr;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            } else {
                recordError(binding->location, tr("Cannot assign object to interface property"));
                return false;
            }
        } else if (property->propType == QMetaType::QVariant) {
            if (property->isVarProperty()) {
                QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
                QV4::Scope scope(v4);
                QV4::ScopedValue wrappedObject(scope, QV4::QObjectWrapper::wrap(QV8Engine::getV4(engine), createdSubObject));
                _vmeMetaObject->setVMEProperty(property->coreIndex, wrappedObject);
            } else {
                QVariant value = QVariant::fromValue(createdSubObject);
                argv[0] = &value;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            }
        } else if (property->isQList()) {
            Q_ASSERT(_currentList.object);

            void *itemToAdd = createdSubObject;

            const char *iid = 0;
            int listItemType = QQmlEnginePrivate::get(engine)->listType(property->propType);
            if (listItemType != -1)
                iid = QQmlMetaType::interfaceIId(listItemType);
            if (iid)
                itemToAdd = createdSubObject->qt_metacast(iid);

            if (_currentList.append)
                _currentList.append(&_currentList, itemToAdd);
        } else {
            QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);

            // We want to raw metaObject here as the raw metaobject is the
            // actual property type before we applied any extensions that might
            // effect the properties on the type, but don't effect assignability
            QQmlPropertyCache *propertyMetaObject = enginePrivate->rawPropertyCacheForType(property->propType);

            // Will be true if the assgned type inherits propertyMetaObject
            bool isAssignable = false;
            // Determine isAssignable value
            if (propertyMetaObject) {
                QQmlPropertyCache *c = propertyCaches.value(binding->value.objectIndex);
                if (!c)
                    c = enginePrivate->cache(createdSubObject);
                while (c && !isAssignable) {
                    isAssignable |= c == propertyMetaObject;
                    c = c->parent();
                }
            }

            if (isAssignable) {
                argv[0] = &createdSubObject;
                QMetaObject::metacall(_qobject, QMetaObject::WriteProperty, property->coreIndex, argv);
            } else {
                recordError(binding->location, tr("Cannot assign object to property"));
                return false;
            }
        }
        return true;
    }

    if (property->isQList()) {
        recordError(binding->location, tr("Cannot assign primitives to lists"));
        return false;
    }

    setPropertyValue(property, binding);
    return true;
}

void QmlObjectCreator::setupFunctions()
{
    QV4::Scope scope(_qmlContext);
    QV4::ScopedValue function(scope);
    QQmlVMEMetaObject *vme = QQmlVMEMetaObject::get(_qobject);

    const quint32 *functionIdx = _compiledObject->functionOffsetTable();
    for (quint32 i = 0; i < _compiledObject->nFunctions; ++i, ++functionIdx) {
        QV4::Function *runtimeFunction = jsUnit->runtimeFunctions[*functionIdx];
        const QString name = runtimeFunction->name->toQString();

        QQmlPropertyData *property = _propertyCache->property(name, _qobject, context);
        if (!property->isVMEFunction())
            continue;

        function = QV4::FunctionObject::creatScriptFunction(_qmlContext, runtimeFunction);
        vme->setVmeMethod(property->coreIndex, function);
    }
}

void QmlObjectCreator::recordError(const QV4::CompiledData::Location &location, const QString &description)
{
    QQmlError error;
    error.setUrl(url);
    error.setLine(location.line);
    error.setColumn(location.column);
    error.setDescription(description);
    errors << error;
}

QObject *QmlObjectCreator::createInstance(int index, QObject *parent)
{
    ActiveOCRestorer ocRestorer(this, QQmlEnginePrivate::get(engine));

    bool isComponent = false;
    QObject *instance = 0;
    QQmlCustomParser *customParser = 0;

    if (compiledData->isComponent(index)) {
        isComponent = true;
        QQmlComponent *component = new QQmlComponent(engine, compiledData, index, parent);
        QQmlComponentPrivate::get(component)->creationContext = context;
        instance = component;
    } else {
        const QV4::CompiledData::Object *obj = qmlUnit->objectAt(index);

        QQmlCompiledData::TypeReference typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex);
        QQmlType *type = typeRef.type;
        if (type) {
            instance = type->create();
            if (!instance) {
                recordError(obj->location, tr("Unable to create object of type %1").arg(stringAt(obj->inheritedTypeNameIndex)));
                return 0;
            }

            const int parserStatusCast = type->parserStatusCast();
            if (parserStatusCast != -1) {
                QQmlParserStatus *parserStatus = reinterpret_cast<QQmlParserStatus*>(reinterpret_cast<char *>(instance) + parserStatusCast);
                parserStatus->classBegin();
                _parserStatusCallbacks[index] = parserStatus;
                parserStatus->d = &_parserStatusCallbacks[index];
            }

            customParser = type->customParser();
        } else {
            Q_ASSERT(typeRef.component);
            if (typeRef.component->qmlUnit->isSingleton())
            {
                recordError(obj->location, tr("Composite Singleton Type %1 is not creatable").arg(stringAt(obj->inheritedTypeNameIndex)));
                return 0;
            }
            QmlObjectCreator subCreator(context, typeRef.component);
            instance = subCreator.create();
            if (!instance) {
                errors += subCreator.errors;
                return 0;
            }
            if (subCreator.componentAttached)
                subCreator.componentAttached->add(&componentAttached);
            allCreatedBindings << subCreator.allCreatedBindings;
            allParserStatusCallbacks << subCreator.allParserStatusCallbacks;
        }
        // ### use no-event variant
        if (parent)
            instance->setParent(parent);
    }

    QQmlData *ddata = QQmlData::get(instance, /*create*/true);
    if (static_cast<quint32>(index) == qmlUnit->indexOfRootObject) {
        if (ddata->context) {
            Q_ASSERT(ddata->context != context);
            Q_ASSERT(ddata->outerContext);
            Q_ASSERT(ddata->outerContext != context);
            QQmlContextData *c = ddata->context;
            while (c->linkedContext) c = c->linkedContext;
            c->linkedContext = context;
        } else
            context->addObject(instance);
        ddata->ownContext = true;
    } else if (!ddata->context)
        context->addObject(instance);

    ddata->outerContext = context;

    QHash<int, int>::ConstIterator idEntry = objectIndexToId.find(index);
    if (idEntry != objectIndexToId.constEnd())
        context->setIdProperty(idEntry.value(), instance);

    if (customParser) {
        QByteArray data = compiledData->customParserData.value(index);
        customParser->setCustomData(instance, data);
    }

    if (!isComponent) {
        QQmlRefPointer<QQmlPropertyCache> cache = propertyCaches.value(index);
        Q_ASSERT(!cache.isNull());

        if (!populateInstance(index, instance, cache, instance, /*value type property*/0))
            return 0;
    }

    return instance;
}

QQmlContextData *QmlObjectCreator::finalize()
{
    {
    QQmlTrace trace("VME Binding Enable");
    trace.event("begin binding eval");

    Q_ASSERT(allCreatedBindings.isEmpty() || allCreatedBindings.isDetached());

    for (QLinkedList<QVector<QQmlAbstractBinding*> >::Iterator it = allCreatedBindings.begin(), end = allCreatedBindings.end();
         it != end; ++it) {
        const QVector<QQmlAbstractBinding *> &bindings = *it;
        for (int i = 0; i < bindings.count(); ++i) {
            QQmlAbstractBinding *b = bindings.at(i);
            if (!b)
                continue;
            b->m_mePtr = 0;
            QQmlData *data = QQmlData::get(b->object());
            Q_ASSERT(data);
            data->clearPendingBindingBit(b->propertyIndex());
            b->setEnabled(true, QQmlPropertyPrivate::BypassInterceptor |
                          QQmlPropertyPrivate::DontRemoveBinding);
        }
    }
    }

    if (true /* ### componentCompleteEnabled()*/) { // the qml designer does the component complete later
        QQmlTrace trace("VME Component Complete");
        for (QLinkedList<QVector<QQmlParserStatus*> >::ConstIterator it = allParserStatusCallbacks.constBegin(), end = allParserStatusCallbacks.constEnd();
             it != end; ++it) {
            const QVector<QQmlParserStatus *> &parserStatusCallbacks = *it;
            for (int i = parserStatusCallbacks.count() - 1; i >= 0; --i) {
                QQmlParserStatus *status = parserStatusCallbacks.at(i);

                if (status && status->d) {
                    status->d = 0;
                    status->componentComplete();
                }

    #if 0 // ###
                if (watcher.hasRecursed() || interrupt.shouldInterrupt())
                    return 0;
    #endif
            }
        }
        allParserStatusCallbacks.clear();
    }

    {
    QQmlTrace trace("VME Component.onCompleted Callbacks");
    while (componentAttached) {
        QQmlComponentAttached *a = componentAttached;
        a->rem();
        QQmlData *d = QQmlData::get(a->parent());
        Q_ASSERT(d);
        Q_ASSERT(d->context);
        a->add(&d->context->componentAttached);
        // ### designer if (componentCompleteEnabled())
            emit a->completed();

#if 0 // ###
        if (watcher.hasRecursed() || interrupt.shouldInterrupt())
            return 0;
#endif
    }
    }

    return rootContext;
}

bool QmlObjectCreator::populateInstance(int index, QObject *instance, QQmlRefPointer<QQmlPropertyCache> cache,
                                        QObject *scopeObjectForBindings, QQmlPropertyData *valueTypeProperty)
{
    const QV4::CompiledData::Object *obj = qmlUnit->objectAt(index);

    Q_ASSERT(scopeObjectForBindings);

    QQmlData *declarativeData = QQmlData::get(instance, /*create*/true);

    qSwap(_propertyCache, cache);
    qSwap(_qobject, instance);
    qSwap(_qobjectForBindings, scopeObjectForBindings);
    qSwap(_valueTypeProperty, valueTypeProperty);
    qSwap(_compiledObject, obj);
    qSwap(_ddata, declarativeData);

    QQmlVMEMetaObject *vmeMetaObject = 0;
    const QByteArray data = vmeMetaObjectData.value(index);
    if (!data.isEmpty()) {
        // install on _object
        vmeMetaObject = new QQmlVMEMetaObject(_qobjectForBindings, _propertyCache, reinterpret_cast<const QQmlVMEMetaData*>(data.constData()));
        if (_ddata->propertyCache)
            _ddata->propertyCache->release();
        _ddata->propertyCache = _propertyCache;
        _ddata->propertyCache->addref();
    } else {
        vmeMetaObject = QQmlVMEMetaObject::get(_qobjectForBindings);
    }

    _ddata->lineNumber = _compiledObject->location.line;
    _ddata->columnNumber = _compiledObject->location.column;

    qSwap(_vmeMetaObject, vmeMetaObject);

    QVector<QQmlAbstractBinding*> createdBindings(_compiledObject->nBindings, 0);
    qSwap(_createdBindings, createdBindings);

    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
    QV4::Scope valueScope(v4);
    QV4::ScopedObject scopeObject(valueScope, QV4::QmlContextWrapper::qmlScope(QV8Engine::get(engine), context, _qobjectForBindings));
    QV4::Scoped<QV4::QmlBindingWrapper> qmlBindingWrapper(valueScope, new (v4->memoryManager) QV4::QmlBindingWrapper(v4->rootContext, scopeObject));
    QV4::ExecutionContext *qmlContext = qmlBindingWrapper->context();

    qSwap(_qmlContext, qmlContext);

    setupBindings();
    setupFunctions();

    allCreatedBindings.append(_createdBindings);

    qSwap(_qmlContext, qmlContext);

    qSwap(_createdBindings, createdBindings);
    qSwap(_vmeMetaObject, vmeMetaObject);
    qSwap(_propertyCache, cache);
    qSwap(_ddata, declarativeData);
    qSwap(_compiledObject, obj);
    qSwap(_valueTypeProperty, valueTypeProperty);
    qSwap(_qobjectForBindings, scopeObjectForBindings);
    qSwap(_qobject, instance);

    return errors.isEmpty();
}


