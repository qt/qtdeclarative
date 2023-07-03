// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <stdexcept>
#include <QMap>
#include "jsontools.h"

namespace JsonTools
{

QMap<QString, QString> g_idToPathMap;

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
        throw std::runtime_error("key not found: '" + key.toStdString() + "'");
    if (!foundValue.isObject())
        throw std::runtime_error("'" + key.toStdString() + "' is not an object!");
    return foundValue.toObject();
}

// Returns the array of the given key in the object
QJsonArray getArray(const QString &key, const QJsonObject object)
{
    const auto foundValue = object.value(key);
    if (foundValue.isUndefined())
        throw std::runtime_error("key not found: '" + key.toStdString() + "'");
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
        throw std::runtime_error("key not found: '" + key.toStdString() + "'");
    return foundValue;
}

// Returns the value of the given key in the object as a string. Same as
// object.value(), but throws an exception if not found.
QString getString(const QString &key, const QJsonObject object)
{
    const auto foundValue = object.value(key);
    if (foundValue.isUndefined())
        throw std::runtime_error("key not found: '" + key.toStdString() + "'");
    if (!foundValue.isString())
        throw std::runtime_error("'" + key.toStdString() + "' is not a string!");
    return foundValue.toString();
}

QStringList getStringList(const QString &key, const QJsonObject object, bool required)
{
    const auto value = object[key];
    if (value.isUndefined()) {
        if (required)
            throw std::runtime_error("key not found: '" + key.toStdString() + "'");
        return {};
    }

    if (value.isString()) {
        return {value.toString()};
    } else if (value.isArray()) {
        QStringList strings;
        const QJsonArray array = value.toArray();
        for (const QJsonValue &element : array)
            strings.append(element.toString());
        return strings;
    } else {
        throw std::runtime_error("key is not string or array: '" + key.toStdString() + "'");
    }
}

QString resolvedPath(const QString &figmaId)
{
    if (!g_idToPathMap.contains(figmaId))  {
        // If the map doesn't contain figmaId, it means that we have never searched
        // for the object before. To get the path, you could call (before calling this
        // function): findChild({"id", figmaId}, m_document.object(), false) to add
        // the path to the map.
        return {};
    }
    return g_idToPathMap[figmaId];
}

bool resolvedHidden(const QString &figmaId)
{
    return resolvedPath(figmaId).contains("[hidden]");
}

void findChildrenImpl(const QStringList &keyValueList
    , const QJsonObject &root
    , bool firstOnly
    , QStringList &currentPath
    , QList<QJsonObject> &result)
{
    // Assert that the key-value list comes in pairs:
    Q_ASSERT(keyValueList.length() % 2 == 0);

    const bool visible = root.value("visible").toBool(true);
    currentPath.append(root["name"].toString() + (visible ? "" : " [hidden]"));

    const auto children = root.value("children").toArray();
    for (auto it = children.constBegin(); it != children.constEnd(); ++it) {
        const auto value = *it;
        if (!value.isObject())
            throw NoChildFoundException(QStringLiteral("expected only objects in array, but found ")
                + QString::number(value.type()) + ". Searched for: " + keyValueList.join(","));

        const QJsonObject object = value.toObject();

        for (int i = 0; i < keyValueList.length(); i += 2) {
            const auto key = keyValueList[i];
            const auto value = keyValueList[i + 1];
            const auto foundValue = object.value(key).toString();

            QRegularExpression re('^' + value + '$', QRegularExpression::CaseInsensitiveOption);
            if (!re.isValid())
                throw NoChildFoundException("value is not a valid regexp: " + foundValue);
            if (!re.match(foundValue).hasMatch())
                break;
            if (i == keyValueList.length() - 2) {
                // All key-value pairs were matched, so add the object to
                // the container. If firstOnly is set, we're done searching.
                const QString figmaId = object["id"].toString("<no id>");
                if (!g_idToPathMap.contains(figmaId)) {
                    // Store the path to all objects we searched for, for both
                    // debugging and visibility purposes.
                    const bool visible = object.value("visible").toBool(true);
                    const QString currentPathString = currentPath.join(",");
                    g_idToPathMap[figmaId] = currentPathString + "," + object["name"].toString()
                            + (visible ? "" : " [hidden]");
                }

                result.append(object);
                if (firstOnly)
                    return;
            }
        }

        findChildrenImpl(keyValueList, object, firstOnly, currentPath, result);
        if (firstOnly && result.size() == 1)
            return;
    }

    currentPath.removeLast();
}

/**
 * Search for a json object recursively inside root with the given key
 * value pairs.
*/
QJsonObject findChildImpl(const QStringList &keyValueList
    , const QJsonObject &root
    , bool warnOnDuplicates
    , QStringList &currentPath)
{
    QList<QJsonObject> result;
    const bool firstOnly = !warnOnDuplicates;

    findChildrenImpl(keyValueList, root, firstOnly, currentPath, result);

    if (result.isEmpty()) {
        Q_ASSERT(keyValueList.length() % 2 == 0);
        QStringList msg;
        for (int i = 0; i < keyValueList.count(); i += 2)
            msg << "'" + keyValueList[i] + ":" + keyValueList[i + 1] + "'";
        throw NoChildFoundException(QStringLiteral("could not find Figma child: ") + msg.join(","));
    }

    if (warnOnDuplicates && result.count() > 1) {
        QStringList msg;
        for (int i = 0; i < keyValueList.count(); i += 2)
            msg << "'" + keyValueList[i] + ":" + keyValueList[i + 1] + "'";
        qWarning().nospace().noquote() << "Warning, found duplicates for: " + msg.join(",");
        for (int i = 0; i < result.count(); ++i) {
            const QString figmaId = result[i]["id"].toString();
            const QString path = resolvedPath(figmaId);
            qWarning().nospace().noquote() << "Duplicate "
                << QString::number(i) << ": "
                << path << (i == 0 ? " [used]" : "");
        }
    }

    return result.first();
}

/**
 * Search for all json objects recursively inside root with the given key
 * value pairs.
*/
QList<QJsonObject> findChildren(const QStringList &keyValueList, const QJsonObject &root)
{
    QList<QJsonObject> result;
    QStringList currentPath;
    const QString pathToRoot = resolvedPath(root["id"].toString());
    if (!pathToRoot.isEmpty()) {
        currentPath = pathToRoot.split(',');
        currentPath.removeLast();
    }
    findChildrenImpl(keyValueList, root, false, currentPath, result);
    return result;
}

/**
 * Search for a json object recursively inside root with the given key
 * value pairs.
*/
QJsonObject findChild(const QStringList &keyValueList, const QJsonObject &root, bool warnOnDuplicates)
{
    QStringList currentPath;
    const QString pathToRoot = resolvedPath(root["id"].toString());
    if (!pathToRoot.isEmpty()) {
        currentPath = pathToRoot.split(',');
        currentPath.removeLast();
    }
    return findChildImpl(keyValueList, root, warnOnDuplicates, currentPath);
}

/**
 * Search for a json object recursively inside root that has the given
 * name path. There can be other objects in-between for each node in the
 * name path.
*/
QJsonObject findNamedChild(const QStringList &namePath, const QJsonObject &root, bool warnOnDuplicates)
{
    QStringList currentPath;
    QJsonObject child = root;
    const QString pathToRoot = resolvedPath(root["id"].toString());
    if (!pathToRoot.isEmpty()) {
        currentPath = pathToRoot.split(',');
        currentPath.removeLast();
    }

    for (const QString &name : namePath)
        child = findChildImpl({"name", name}, child, warnOnDuplicates, currentPath);

    return child;
}

} // namespace
