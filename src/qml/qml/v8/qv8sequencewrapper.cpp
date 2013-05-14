/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include <QtQml/qqml.h>

#include "qv8sequencewrapper_p.h"
#include "qv8sequencewrapper_p_p.h"
#include "qv8engine_p.h"

#include <private/qv4functionobject_p.h>

QT_BEGIN_NAMESPACE

QV8SequenceWrapper::QV8SequenceWrapper()
    : m_engine(0)
{
}

QV8SequenceWrapper::~QV8SequenceWrapper()
{
}

#define REGISTER_QML_SEQUENCE_METATYPE(unused, unused2, SequenceType, unused3) qRegisterMetaType<SequenceType>();
void QV8SequenceWrapper::init(QV8Engine *engine)
{
    FOREACH_QML_SEQUENCE_TYPE(REGISTER_QML_SEQUENCE_METATYPE)

    m_engine = QV8Engine::getV4(engine);

    m_prototype = QV4::Value::fromObject(m_engine->newObject());
    m_prototype.value().asObject()->prototype = m_engine->arrayPrototype;
    QQmlSequencePrototype::initClass(m_engine, m_prototype.value());
}
#undef REGISTER_QML_SEQUENCE_METATYPE

void QV8SequenceWrapper::destroy()
{
}

#define IS_SEQUENCE(unused1, unused2, SequenceType, unused3) \
    if (sequenceTypeId == qMetaTypeId<SequenceType>()) { \
        return true; \
    } else

bool QV8SequenceWrapper::isSequenceType(int sequenceTypeId) const
{
    FOREACH_QML_SEQUENCE_TYPE(IS_SEQUENCE) { /* else */ return false; }
}
#undef IS_SEQUENCE

#define NEW_REFERENCE_SEQUENCE(ElementType, ElementTypeName, SequenceType, unused) \
    if (sequenceType == qMetaTypeId<SequenceType>()) { \
        QV4::Object *obj = new (m_engine->memoryManager) QQml##ElementTypeName##List(m_engine, object, propertyIndex); \
        obj->prototype = m_prototype.value().asObject(); \
        return QV4::Value::fromObject(obj); \
    } else

QV4::Value QV8SequenceWrapper::newSequence(int sequenceType, QObject *object, int propertyIndex, bool *succeeded)
{
    // This function is called when the property is a QObject Q_PROPERTY of
    // the given sequence type.  Internally we store a typed-sequence
    // (as well as object ptr + property index for updated-read and write-back)
    // and so access/mutate avoids variant conversion.
    *succeeded = true;
    FOREACH_QML_SEQUENCE_TYPE(NEW_REFERENCE_SEQUENCE) { /* else */ *succeeded = false; return QV4::Value::undefinedValue(); }
}
#undef NEW_REFERENCE_SEQUENCE

#define NEW_COPY_SEQUENCE(ElementType, ElementTypeName, SequenceType, unused) \
    if (sequenceType == qMetaTypeId<SequenceType>()) { \
        QV4::Object *obj = new (m_engine->memoryManager) QQml##ElementTypeName##List(m_engine, v.value<SequenceType >()); \
        obj->prototype = m_prototype.value().asObject(); \
        return QV4::Value::fromObject(obj); \
    } else

QV4::Value QV8SequenceWrapper::fromVariant(const QVariant& v, bool *succeeded)
{
    // This function is called when assigning a sequence value to a normal JS var
    // in a JS block.  Internally, we store a sequence of the specified type.
    // Access and mutation is extremely fast since it will not need to modify any
    // QObject property.
    int sequenceType = v.userType();
    *succeeded = true;
    FOREACH_QML_SEQUENCE_TYPE(NEW_COPY_SEQUENCE) { /* else */ *succeeded = false; return QV4::Value::undefinedValue(); }
}
#undef NEW_COPY_SEQUENCE

#define SEQUENCE_TO_VARIANT(ElementType, ElementTypeName, SequenceType, unused) \
    if (QQml##ElementTypeName##List *list = object->asQml##ElementTypeName##List()) \
        return list->toVariant(); \
    else

QVariant QV8SequenceWrapper::toVariant(QV4::Object *object)
{
    Q_ASSERT(object->isListType());
    FOREACH_QML_SEQUENCE_TYPE(SEQUENCE_TO_VARIANT) { /* else */ return QVariant(); }
}

#define SEQUENCE_TO_VARIANT(ElementType, ElementTypeName, SequenceType, unused) \
    if (typeHint == qMetaTypeId<SequenceType>()) { \
        return QQml##ElementTypeName##List::toVariant(a); \
    } else

QVariant QV8SequenceWrapper::toVariant(const QV4::Value &array, int typeHint, bool *succeeded)
{
    *succeeded = true;

    QV4::ArrayObject *a = array.asArrayObject();
    if (!a) {
        *succeeded = false;
        return QVariant();
    }
    FOREACH_QML_SEQUENCE_TYPE(SEQUENCE_TO_VARIANT) { /* else */ *succeeded = false; return QVariant(); }
}

#undef SEQUENCE_TO_VARIANT

#define MAP_META_TYPE(ElementType, ElementTypeName, SequenceType, unused) \
    case QV4::Managed::Type_Qml##ElementTypeName##List: return qMetaTypeId<SequenceType>();

int QV8SequenceWrapper::metaTypeForSequence(QV4::Object *object)
{
    switch (object->internalType()) {
    FOREACH_QML_SEQUENCE_TYPE(MAP_META_TYPE)
    default:
        return -1;
    }
}

#undef MAP_META_TYPE

#include "qv8sequencewrapper_p_p_jsclass.cpp"

QT_END_NAMESPACE
