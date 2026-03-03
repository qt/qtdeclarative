// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitdebug_p.h"
#include "qqstylekitcontrol_p.h"
#include "qqstylekitcontrols_p.h"
#include "qqstylekitstyle_p.h"
#include "qqstylekittheme_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype StyleKitDebug
    \inqmlmodule Qt.labs.StyleKit
    \brief Traces how style properties are resolved for a control.

    StyleKitDebug is a diagnostic tool that logs style property reads to the
    debug output. It is useful for understanding why a control ends up with a
    particular appearance, especially in complex styles that involve multiple
    \l {Theme}{themes} or \l {StyleVariation}{style variations}.

    It is accessed through \l {StyleKit::debug}{StyleKit.debug}.

    \note This tool is experimental. Enabling it will severely degrade performance and should only
    be used for debugging. The output format may change in future versions.

    To start tracing, assign the \l{The id Attribute}{id} of a \l Control to the \l control property:

    \code
    StyleKit.debug.control = myButton
    \endcode

    Each resolved property is printed as a single line showing where the value
    came from and what it resolved to:

    \code
    [read] StyleReader[Normal].button.background.color -> button[Normal] = #ff0000
    \endcode

    Use \l filter to limit the output to properties of interest.

    \labs

    \sa StyleKit::debug
*/

/*!
    \qmlproperty Item StyleKitDebug::control

    The \l{The id Attribute}{id} of the \l Control to trace.

    When set, StyleKit logs all style property reads
    for this item to the debug output. Set to \c null to stop tracing.

    \sa filter
*/

/*!
    \qmlproperty string StyleKitDebug::filter

    A regular expression used to filter the debug output. Only lines matching
    the pattern are printed. By default, all output is shown.

    For example, to show only reads of the background color:

    \code
    StyleKit.debug.filter = "background.color"
    \endcode

    \sa control
*/

const QQStyleKitPropertyGroup *QQStyleKitDebug::groupBeingRead = nullptr;
QPointer<QQuickItem> QQStyleKitDebug::m_item;
QString QQStyleKitDebug::m_filter;
int QQStyleKitDebug::m_outputCount = 0;

static const QChar kDot = '.'_L1;

template <typename EnumType>
QString QQStyleKitDebug::enumToString(EnumType enumValue)
{
    auto propertyMetaEnum = QMetaEnum::fromType<EnumType>();
    return QString::fromUtf8(propertyMetaEnum.valueToKeys(quint64(enumValue)));
}

QString QQStyleKitDebug::objectName(const QObject *obj) {
    QString str = QString::fromLatin1(obj->metaObject()->className());
    int idx = str.indexOf("_QMLTYPE"_L1);
    if (idx != -1)
        str = str.left(idx);
    else {
        const QString prefix("QQStyleKit"_L1);
        if (str.startsWith(prefix))
            str = str.mid(prefix.length());
    }
    const QString name = obj->objectName();
    if (!name.isEmpty())
        str = str + "("_L1 + name + ")"_L1;
    return str;
}

QString QQStyleKitDebug::stateToString(const QQSK::State state)
{
    const QStringList list = enumToString(state).split('|'_L1);
    return "["_L1 + list.join(','_L1) + "]"_L1;
}

QString QQStyleKitDebug::styleReaderToString(const QQStyleKitReader *reader)
{
    return "StyleKitReader"_L1 + stateToString(reader->controlState());
}

QString QQStyleKitDebug::propertyPath(const QQStyleKitPropertyGroup *group, const PropertyPathId property)
{
    const QString path = group->pathToString();
    QString propertyName = enumToString(property.property());
    propertyName[0] = propertyName[0].toLower();
    if (path.isEmpty())
        return propertyName;
    return path + kDot + propertyName;
}

QString QQStyleKitDebug::controlToString(const QQStyleKitControlProperties *control)
{
    const QObject *parentObj = control->parent();
    if (!parentObj)
        return "<no parent>"_L1;
    auto *controls = qobject_cast<const QQStyleKitControls *>(parentObj);
    if (!controls) {
        return "<"_L1 + QString::fromUtf8(parentObj->metaObject()->className()) + ">"_L1;
    }

    const int startIndex = QQStyleKitControlProperties::staticMetaObject.propertyOffset();
    const int endIndex = QQStyleKitControlProperties::staticMetaObject.propertyCount();

    const QMetaObject* parentMeta = parentObj->metaObject();
    for (int i = startIndex; i < endIndex; ++i) {
        const QMetaProperty prop = parentMeta->property(i);
        const QMetaObject* typeMeta = QMetaType::fromName(prop.typeName()).metaObject();
        if (!typeMeta || !typeMeta->inherits(&QQStyleKitControl::staticMetaObject))
            continue;

        QObject *propObj = prop.read(parentObj).value<QObject *>();
        if (propObj == control)
            return QString::fromUtf8(prop.name());
    }
    return "<unknown control: no property found>"_L1;
}

QString QQStyleKitDebug::objectPath(const QQStyleKitControlProperties *properties, QObject *from)
{
    QString path;
    const QObject *obj = properties;

    while (obj) {
        if (!path.isEmpty())
            path.prepend(kDot);

        if (auto *theme = qobject_cast<const QQStyleKitCustomTheme *>(obj)) {
            path.prepend(theme->name() + kDot);
        } else if (auto *theme = qobject_cast<const QQStyleKitTheme *>(obj)) {
            // Note: only one theme is instantiated at a time
            if (auto style = theme->style())
                path.prepend(style->themeName());
            else
                path.prepend(objectName(obj));
        } else if (auto *control = qobject_cast<const QQStyleKitControl *>(obj)) {
            path.prepend(controlToString(control));
        } else if (auto *reader = qobject_cast<const QQStyleKitReader *>(obj)) {
            path.prepend(styleReaderToString(reader));
        } else {
            path.prepend(objectName(obj));
        }

        if (obj == from)
            break;

        obj = obj->parent();
    }

    return path;
}

void QQStyleKitDebug::notifyPropertyRead(
    const PropertyPathId property,
    const QQStyleKitControlProperties *storage,
    const QQSK::State state,
    const QVariant &value)
{
    Q_ASSERT(enabled());

    const QQStyleKitControlProperties *reader = QQStyleKitDebug::groupBeingRead->controlProperties();
    if (reader->subclass() == QQSK::Subclass::QQStyleKitState) {
        /* The reader is in the UnfiedStyle, and not in the users application (which can happen
         * when e.g resolving local bindings between properties in the style). Those are not
         * interesting to print out when inspecting control-to-style mappings. Ignore. */
        return;
    }

    if (!insideControl(reader)) {
        // We should only debug reads that targets m_item. So return.
        return;
    }

    const QString _readerPath = objectPath(reader, m_item);
    const QString _readPropertyPath = propertyPath(QQStyleKitDebug::groupBeingRead, property);
    const QString queriedPath = _readerPath + kDot +_readPropertyPath;

    QString storagePath;
    if (storage->subclass() == QQSK::Subclass::QQStyleKitReader) {
        /* We read an interpolated value stored directly in the reader itself. While this
         * can be interesting to print out whe debugging the styling engine itself, it
         * comes across as noise when inspecting control-to-style mappings. Ignore. */
#if 0
        storagePath = "[local storage] "_L1;
#else
        return;
#endif
    } else {
        const QString _controlPathInStyle = objectPath(storage, storage->style());
        const QString _statePath = stateToString(state);
        storagePath = _controlPathInStyle + _statePath;
    }

    QString valueString = value.toString();
    if (!value.isValid()) // value was set, but probably to undefined
        valueString = "<undefined>"_L1;
    else if (valueString.isEmpty())
        valueString = "<object>"_L1;

    const QString output = queriedPath + " -> "_L1 + storagePath + " = "_L1 + valueString;

    if (!QRegularExpression(m_filter).match(output).hasMatch())
        return;

    qDebug().nospace().noquote() << m_outputCount++ << " | [read] "_L1 << output;
}

void QQStyleKitDebug::notifyPropertyWrite(
    const QQStyleKitPropertyGroup *group,
    const QQSK::Property property,
    const QQStyleKitControlProperties *storage,
    const QQSK::State state,
    const PropertyStorageId key,
    const QVariant &value)
{
#if 1
    Q_UNUSED(group);
    Q_UNUSED(property);
    Q_UNUSED(storage);
    Q_UNUSED(state);
    Q_UNUSED(key);
    Q_UNUSED(value);
#else
    /* Note: in order to catch _all_ writes, we cannot depend on enabling writes from
     * QML using a property, as that would resolve to 'true' too late. */
    QString storagePath;
    if (storage->subclass() == QQSK::Subclass::QQStyleKitReader) {
        storagePath = "[local storage]"_L1;
    } else {
        const QString _controlPathInStyle = objectPath(storage, storage->style());
        const QString _statePath = stateToString(state);
        storagePath = _controlPathInStyle + _statePath;
    }

    QString valueString = value.toString();
    if (!value.isValid()) // value was set, but probably to undefined
        valueString = "<undefined>"_L1;
    else if (valueString.isEmpty())
        valueString = "<object>"_L1;

    const QString path = propertyPath(group, property);
    const QString output = storagePath + kDot + path + " (storage key:"_L1 + QString::number(key) + ") = "_L1 + valueString;

    qDebug().nospace().noquote() << m_outputCount++ << " | [write] "_L1 << output;
#endif
}

void QQStyleKitDebug::notifyPropertyNotResolved(const PropertyPathId property)
{
    const QQStyleKitControlProperties *reader = QQStyleKitDebug::groupBeingRead->controlProperties();
    if (!insideControl(reader)) {
        // We should only debug reads that targets m_item. So return.
        return;
    }

    const QString _readerPath = objectPath(reader, m_item);
    const QString _propertyPath = propertyPath(QQStyleKitDebug::groupBeingRead, property);
    const QString queriedPath = _readerPath + kDot +_propertyPath;
    const QString output = queriedPath + " -> <property not set>"_L1;

    if (!QRegularExpression(m_filter).match(output).hasMatch())
        return;

    qDebug().nospace().noquote() << m_outputCount++ << " | [read] "_L1 << output;
}

void QQStyleKitDebug::trace(
    const PropertyPathId property,
    const QQStyleKitControlProperties *storage,
    const QQSK::State state,
    const PropertyStorageId key)
{
#if 1
    Q_UNUSED(property);
    Q_UNUSED(storage);
    Q_UNUSED(state);
    Q_UNUSED(key);
#else
    const QQStyleKitControlProperties *reader = QQStyleKitDebug::groupBeingRead->controlProperties();
    if (reader->subclass() == QQSK::Subclass::QQStyleKitState) {
        /* The reader is in the UnfiedStyle, and not in the users application (which can happen
         * when e.g resolving local bindings between properties in the style). Those are not
         * interesting to print out when inspecting control-to-style mappings. Ignore. */
        return;
    }

    if (!insideControl(reader)) {
        // We should only debug reads that targets m_item. So return.
        return;
    }

    const QString _readerPath = objectPath(reader, m_item);
    const QString _readPropertyPath = propertyPath(QQStyleKitDebug::groupBeingRead, property);
    const QString queriedPath = _readerPath + kDot +_readPropertyPath;

    QString storagePath;
    if (storage->subclass() == QQSK::Subclass::QQStyleKitReader) {
        /* We read an interpolated value stored directly in the reader itself. While this
         * can be interesting to print out whe debugging the styling engine itself, it
         * comes across as noise when inspecting control-to-style mappings. Ignore. */
#if 0
        storagePath = "[local storage]"_L1;
#else
        return;
#endif
    } else {
        const QString _controlPathInStyle = objectPath(storage, storage->style());
        const QString _statePath = stateToString(state);
        storagePath = _controlPathInStyle + _statePath;
    }

    const QString output = queriedPath + ", checking "_L1 + storagePath + " (storage key:"_L1 + QString::number(key)+ ")"_L1;

    if (!QRegularExpression(m_filter).match(output).hasMatch())
        return;

    qDebug().nospace().noquote() << m_outputCount++ << " | [trace] "_L1 << output;
#endif
}

QQuickItem *QQStyleKitDebug::control() const
{
    return m_item;
}

void QQStyleKitDebug::setControl(QQuickItem *item)
{
    if (m_item == item)
        return;

    m_item = item;
    emit controlChanged();
}

QString QQStyleKitDebug::filter() const
{
    return m_filter;
}

void QQStyleKitDebug::setFilter(const QString &filter)
{
    if (m_filter == filter)
        return;

    m_filter = filter;
    emit filterChanged();
}

bool QQStyleKitDebug::insideControl(const QObject *child)
{
    if (!m_item)
        return false;
    const QObject *obj = child;
    do {
        if (obj == m_item)
            return true;
        obj = obj->parent();
    } while (obj);
    return false;
}

QT_END_NAMESPACE

#include "moc_qqstylekitdebug_p.cpp"

