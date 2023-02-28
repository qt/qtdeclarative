// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllistaccessor_p.h"

#include <private/qqmlmetatype_p.h>

#include <QtCore/qstringlist.h>
#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>

// ### Remove me
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

QQmlListAccessor::QQmlListAccessor()
: m_type(Invalid)
{
}

QQmlListAccessor::~QQmlListAccessor()
{
}

QVariant QQmlListAccessor::list() const
{
    return d;
}

void QQmlListAccessor::setList(const QVariant &v)
{
    d = v;

    // An incoming JS array as model is treated as a variant list, so we need to
    // convert it first with toVariant().
    QMetaType variantsType = d.metaType();
    if (variantsType == QMetaType::fromType<QJSValue>()) {
        d = d.value<QJSValue>().toVariant();
        variantsType = d.metaType();
    }

    if (!d.isValid()) {
        m_type = Invalid;
    } else if (variantsType == QMetaType::fromType<QStringList>()) {
        m_type = StringList;
    } else if (variantsType == QMetaType::fromType<QList<QUrl>>()) {
        m_type = UrlList;
    } else if (variantsType == QMetaType::fromType<QVariantList>()) {
        m_type = VariantList;
    } else if (variantsType == QMetaType::fromType<QList<QObject *>>()) {
        m_type = ObjectList;
    } else if (variantsType.flags() & QMetaType::IsQmlList) {
        d = QVariant::fromValue(QQmlListReference(d));
        m_type = ListProperty;
    } else if (variantsType == QMetaType::fromType<QQmlListReference>()) {
        m_type = ListProperty;
    } else if (variantsType.flags() & QMetaType::PointerToQObject) {
        m_type = Instance;
    } else if (int i = 0; [&](){bool ok = false; i = v.toInt(&ok); return ok;}()) {
        // Here we have to check for an upper limit, because down the line code might (well, will)
        // allocate memory depending on the number of elements. The upper limit cannot be INT_MAX:
        //      QVector<QPointer<QQuickItem>> something;
        //      something.resize(count());
        // (See e.g. QQuickRepeater::regenerate())
        // This will allocate data along the lines of:
        //      sizeof(QPointer<QQuickItem>) * count() + QVector::headerSize
        // So, doing an approximate round-down-to-nice-number, we get:
        const int upperLimit = 100 * 1000 * 1000;

        if (i < 0) {
            qWarning("Model size of %d is less than 0", i);
            m_type = Invalid;
        } else if (i > upperLimit) {
            qWarning("Model size of %d is bigger than the upper limit %d", i, upperLimit);
            m_type = Invalid;
        } else {
            m_type = Integer;
            d = i;
        }
    } else {
        const QQmlType type = QQmlMetaType::qmlListType(v.metaType());
        if (type.isSequentialContainer()) {
            m_metaSequence = type.listMetaSequence();
            m_type = Sequence;
        } else {
            m_type = Instance;
        }
    }
}

qsizetype QQmlListAccessor::count() const
{
    switch(m_type) {
    case StringList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QStringList>());
        return reinterpret_cast<const QStringList *>(d.constData())->size();
    case UrlList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QList<QUrl>>());
        return reinterpret_cast<const QList<QUrl> *>(d.constData())->size();
    case VariantList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QVariantList>());
        return reinterpret_cast<const QVariantList *>(d.constData())->size();
    case ObjectList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QList<QObject *>>());
        return reinterpret_cast<const QList<QObject *> *>(d.constData())->size();
    case ListProperty:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QQmlListReference>());
        return reinterpret_cast<const QQmlListReference *>(d.constData())->count();
    case Sequence:
        Q_ASSERT(m_metaSequence != QMetaSequence());
        return m_metaSequence.size(d.constData());
    case Instance:
        return 1;
    case Integer:
        return *reinterpret_cast<const int *>(d.constData());
    case Invalid:
        return 0;
    }
    Q_UNREACHABLE_RETURN(0);
}

QVariant QQmlListAccessor::at(qsizetype idx) const
{
    Q_ASSERT(idx >= 0 && idx < count());
    switch(m_type) {
    case StringList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QStringList>());
        return QVariant::fromValue(reinterpret_cast<const QStringList *>(d.constData())->at(idx));
    case UrlList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QList<QUrl>>());
        return QVariant::fromValue(reinterpret_cast<const QList<QUrl> *>(d.constData())->at(idx));
    case VariantList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QVariantList>());
        return reinterpret_cast<const QVariantList *>(d.constData())->at(idx);
    case ObjectList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QList<QObject *>>());
        return QVariant::fromValue(reinterpret_cast<const QList<QObject *> *>(d.constData())->at(idx));
    case ListProperty:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QQmlListReference>());
        return QVariant::fromValue(reinterpret_cast<const QQmlListReference *>(d.constData())->at(idx));
    case Sequence: {
        Q_ASSERT(m_metaSequence != QMetaSequence());
        QVariant result;
        const QMetaType valueMetaType = m_metaSequence.valueMetaType();
        if (valueMetaType == QMetaType::fromType<QVariant>()) {
            m_metaSequence.valueAtIndex(d.constData(), idx, &result);
        } else {
            result = QVariant(valueMetaType);
            m_metaSequence.valueAtIndex(d.constData(), idx, result.data());
        }
        return result;
    }
    case Instance:
        return d;
    case Integer:
        return QVariant(idx);
    case Invalid:
        return QVariant();
    }
    Q_UNREACHABLE_RETURN(QVariant());
}

void QQmlListAccessor::set(qsizetype idx, const QVariant &value)
{
    Q_ASSERT(idx >= 0 && idx < count());
    switch (m_type) {
    case StringList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QStringList>());
        (*static_cast<QStringList *>(d.data()))[idx] = value.toString();
        break;
    case UrlList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QList<QUrl>>());
        (*static_cast<QList<QUrl> *>(d.data()))[idx] = value.value<QUrl>();
        break;
    case VariantList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QVariantList>());
        (*static_cast<QVariantList *>(d.data()))[idx] = value;
        break;
    case ObjectList:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QList<QObject *>>());
        (*static_cast<QList<QObject *> *>(d.data()))[idx] = value.value<QObject *>();
        break;
    case ListProperty:
        Q_ASSERT(d.metaType() == QMetaType::fromType<QQmlListReference>());
        static_cast<QQmlListReference *>(d.data())->replace(idx, value.value<QObject *>());
        break;
    case Sequence: {
        Q_ASSERT(m_metaSequence != QMetaSequence());
        const QMetaType valueMetaType = m_metaSequence.valueMetaType();
        if (valueMetaType == QMetaType::fromType<QVariant>()) {
            m_metaSequence.setValueAtIndex(d.data(), idx, &value);
        } else if (valueMetaType == value.metaType()) {
            m_metaSequence.setValueAtIndex(d.data(), idx, value.constData());
        } else {
            QVariant converted = value;
            converted.convert(valueMetaType);
            m_metaSequence.setValueAtIndex(d.data(), idx, converted.constData());
        }
        break;
    }
    case Instance:
        d = value;
        break;
    case Integer:
        break;;
    case Invalid:
        break;
    }
}

bool QQmlListAccessor::isValid() const
{
    return m_type != Invalid;
}

QT_END_NAMESPACE
