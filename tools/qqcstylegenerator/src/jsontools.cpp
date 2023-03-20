// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <stdexcept>
#include <QMap>
#include "jsontools.h"

namespace JsonTools
{

NoChildFoundException::NoChildFoundException(const QString &msg)
    : std::runtime_error(msg.toLatin1().data())
{}

RestCallException::RestCallException(const QString &msg)
    : std::runtime_error(msg.toLatin1().data())
{}

// Returns the object with the given key in the object. The
// object needs to have a "name" key, as such.
QJsonObject getObject(const QString &key, const QJsonObject object)
{
    const auto foundValue = object.value(key);
    if (foundValue.isUndefined())
        throw std::runtime_error("could not find object '" + key.toStdString() + "'");
    if (!foundValue.isObject())
        throw std::runtime_error("'" + key.toStdString() + "' is not an object!");
    return foundValue.toObject();
}

// Returns the array of the given key in the object
QJsonArray getArray(const QString &key, const QJsonObject object)
{
    const auto foundValue = object.value(key);
    if (foundValue.isUndefined())
        throw std::runtime_error("could not find array '" + key.toStdString() + "'");
    if (!foundValue.isArray())
        throw std::runtime_error("'" + key.toStdString() + "' is not an array!");
    return foundValue.toArray();
}

// Returns the value of the given key in the object. Same as
// object.value(), but throws an exception if not found.
QJsonValue getValue(const QString &key, const QJsonObject object)
{
    const auto foundValue = object.value(key);
    if (foundValue.isUndefined())
        throw std::runtime_error("could not find value '" + key.toStdString() + "'");
    return foundValue;
}

// Returns the value of the given key in the object as a string. Same as
// object.value(), but throws an exception if not found.
QString getString(const QString &key, const QJsonObject object)
{
    const auto foundValue = object.value(key);
    if (foundValue.isUndefined())
        throw std::runtime_error("could not find value '" + key.toStdString() + "'");
    if (!foundValue.isString())
        throw std::runtime_error("'" + key.toStdString() + "' is not a string!");
    return foundValue.toString();
}

void findChildrenImpl(const QStringList &keyValueList, const QJsonObject &root, bool firstOnly, QList<QJsonObject> &result)
{
    // Assert that the key-value list comes in pairs:
    Q_ASSERT(keyValueList.length() % 2 == 0);
    const auto children = root.value("children").toArray();

    for (auto it = children.constBegin(); it != children.constEnd(); ++it) {
        const auto value = *it;
        if (!value.isObject())
            throw NoChildFoundException(QStringLiteral("expected only objects in array, but found ")
                + QString::number(value.type()) + ". Searched for: " + keyValueList.join(","));

        const auto object = value.toObject();
        for (int i = 0; i < keyValueList.length(); i += 2) {
            const auto key = keyValueList[i];
            const auto value = keyValueList[i + 1];
            const auto foundValue = object.value(key).toString();
            if (foundValue.compare(value, Qt::CaseInsensitive) != 0)
                break;
            if (i == keyValueList.length() - 2) {
                // All key-value pairs were matched, so add the object to
                // the container. If firstOnly is set, we're done searching.
                result.append(object);
                if (firstOnly)
                    return;
            }
        }

        findChildrenImpl(keyValueList, object, firstOnly, result);
        if (firstOnly && result.length() == 1)
            return;
    }
}

QList<QJsonObject> findChildren(const QStringList &keyValueList, const QJsonObject &root)
{
    QList<QJsonObject> result;
    findChildrenImpl(keyValueList, root, false, result);
    return result;
}

/**
 * Search for a json object recursively inside root with the given key
 * value pairs.
*/
QJsonObject findChild(const QStringList &keyValueList, const QJsonObject &root)
{
    QList<QJsonObject> result;
    findChildrenImpl(keyValueList, root, true, result);
    if (result.isEmpty()) {
        Q_ASSERT(keyValueList.length() % 2 == 0);
        QStringList msg;
        for (int i = 0; i < keyValueList.count(); i += 2)
            msg << "'" + keyValueList[i] + ":" + keyValueList[i + 1] + "'";
        throw NoChildFoundException(QStringLiteral("could not find node: ") + msg.join(","));
    }
    return result.value(0);
}

/**
 * Search for a json object recursively inside root that has the given
 * name path. There can be other objects in-between for each node in the
 * name path.
*/
QJsonObject findNamedChild(const QStringList &namePath, const QJsonObject &root)
{
    QJsonObject child = root;
    for (const QString &name : namePath)
        child = findChild({"name", name}, child);
    return child;
}

} // namespace
