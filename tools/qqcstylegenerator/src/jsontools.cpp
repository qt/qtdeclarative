// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <stdexcept>
#include <QMap>
#include "jsontools.h"

namespace JsonTools
{

QJsonObject lastObject;
QJsonArray lastArray;
QJsonValue lastValue;

QStringList lastArtboardPath;
QStringList lastArtboardActualPath;
QStringList getChild_resolvedPath;

NoImageFoundException::NoImageFoundException(const QString &msg)
    : std::runtime_error(msg.toLatin1().data())
{}

NoArtboardFoundException::NoArtboardFoundException(const QString &msg)
    : std::runtime_error(msg.toLatin1().data())
{}

// Returns the object with the given name in the array. The
// object needs to have a "name" key, as such.
QJsonObject getObjectInArrayWithName(const QString &name, const QJsonArray &array)
{
    for (auto it = array.constBegin(); it != array.constEnd(); ++it) {
        const auto value = *it;
        if (!value.isObject())
            continue;

        const auto object = value.toObject();
        const auto nameValue = object.value("name");
        if (nameValue.isNull())
            continue;
        const QString foundName = nameValue.toString();
        if (foundName == name) {
            lastObject = object;
            return lastObject;
        }
    }

    throw std::runtime_error("could not find object with name '" + name.toStdString() + "' in array!");
}

// Returns the object with the given key in the object. The
// object needs to have a "name" key, as such.
QJsonObject getObject(const QString &key, const QJsonObject object)
{
    const auto foundValue = object.value(key);
    if (foundValue.isUndefined())
        throw std::runtime_error("could not find object '" + key.toStdString() + "'");
    if (!foundValue.isObject())
        throw std::runtime_error("'" + key.toStdString() + "' is not an object!");

    lastObject = foundValue.toObject();
    return lastObject;
}

// Returns the array of the given key in the object
QJsonArray getArray(const QString &key, const QJsonObject object)
{
    const auto foundValue = object.value(key);
    if (foundValue.isUndefined())
        throw std::runtime_error("could not find array '" + key.toStdString() + "'");
    if (!foundValue.isArray())
        throw std::runtime_error("'" + key.toStdString() + "' is not an array!");

    lastArray = foundValue.toArray();
    return lastArray;
}

// Returns the value of the given key in the object. Same as
// object.value(), but throws an exception if not found.
QJsonValue getValue(const QString &key, const QJsonObject object)
{
    const auto foundValue = object.value(key);
    if (foundValue.isUndefined())
        throw std::runtime_error("could not find value '" + key.toStdString() + "'");

    lastValue = foundValue;
    return lastValue;
}

/**
 * Get the json object that points to the root of the
 * tree that describes a control (quick) / template (figma) / artboard (qtbridge)
*/
QJsonObject getArtboardSet(const QString &artboardSetName, const QJsonDocument &doc)
{
    // TODO: When using real-life data, 'artboardSets' will probably not
    // be on the root node, so this function will need to be adjusted!
    const auto array = getArray("artboardSets", doc.object());
    return getObjectInArrayWithName(artboardSetName, array);
}

/**
 * Return all the artboard children of the given artboard set as a
 * map from state name to artboard.
*/
QMap<QString, QJsonObject> getArtboards(const QJsonObject &artboardSet)
{
    QMap<QString, QJsonObject> stateMap;

    const auto artboardValue = artboardSet.value("artboards");
    if (!artboardValue.isArray())
        throw std::runtime_error("artboards is not an array!");

    const QJsonArray artboardArray = artboardValue.toArray();

    for (auto it = artboardArray.constBegin(); it != artboardArray.constEnd(); ++it) {
        const auto value = *it;
        if (!value.isObject())
            continue;
        const auto object = value.toObject();
        const auto nameValue = object.value("name");
        if (nameValue.isNull())
            continue;
        const QString foundName = nameValue.toString();
        static const QString prefix = QLatin1String("state=");
        if (!foundName.startsWith(prefix))
            continue;
        const QString stateName = foundName.sliced(prefix.length());
        stateMap[stateName] = object;
    }

    if (stateMap.isEmpty())
        throw std::runtime_error("could not find any atboards with a 'name:state=' field");

    return stateMap;
}

/**
 * Returns the value of key in the artboards metadata as a string
*/
QString getMetadataString(const QString &key, const QJsonObject artboard)
{
    getObject("metadata", artboard);
    return lastObject.value(key).toString();
}

/**
 * Nodes with an image will store the path to that image inside the metadata
 * Since this structure seems to be the same for all nodes, it's factored out
 * to this function.
 * \sa https://www.qt.io/blog/qt-bridge-metadata-format
 */
QString getImagePath(const QJsonObject artboard)
{
    try {
        getObject("metadata", artboard);
        getObject("assetData");
        return getValue("assetPath").toString();
    } catch (std::exception &) {
        throw NoImageFoundException();
    }
}

/**
 * Note: this function assumes that the component with the given
 * uuid is a child of artboardSets. The latter is expected to be
 * an array at the root of the document. The function will therefore
 * not search the whole document.
*/
QJsonObject getTypeWithUuid(const QString &uuid, const QJsonDocument &doc)
{
    const auto artboardSetsArray = getArray("artboardSets", doc.object());
    for (auto it1 = artboardSetsArray.constBegin(); it1 != artboardSetsArray.constEnd(); ++it1) {
        const auto artboardSetValue = *it1;
        if (!artboardSetValue.isObject())
            continue;

        const auto artboardArray = getArray("artboards", artboardSetValue.toObject());
        for (auto it2 = artboardArray.constBegin(); it2 != artboardArray.constEnd(); ++it2) {
            const auto artboardValue = *it2;
            if (!artboardValue.isObject())
                continue;

            const auto artboard = artboardValue.toObject();
            const QString artboardUuid = getMetadataString("uuid", artboard);
            if (artboardUuid == uuid)
                return artboard;
        }
    }
    throw NoArtboardFoundException(uuid);
}

/**
 * Returns a child/descendant of the given parent. If one of the children along
 * the path (other than the last element) has a link to a type (that is, have a
 * typeUuid), the link will be followed, since it's assumed that the remaining
 * children is described in the type/component rather than the instance (the child
 * with the link).
*/
QJsonObject getChild(const QStringList &path, const QJsonObject parent, const QJsonDocument &doc)
{
    getChild_resolvedPath.clear();
    try {
        QJsonObject child = parent;
        for (int i = 0; i < path.count(); ++i) {
            const QString pathName = path.at(i);
            getChild_resolvedPath.append(pathName);
            const auto children = getArray("children", child);
            child = getObjectInArrayWithName(pathName, children);
            if (i < path.count() - 1) {
                const QString typeUuid = getMetadataString("typeUuid", child);
                if (!typeUuid.isEmpty()) {
                    getChild_resolvedPath.last() += " (link)";
                    child = getTypeWithUuid(typeUuid, doc);
                }
            }
        }
        return child;
    } catch (std::exception &) {
        throw NoArtboardFoundException(getChild_resolvedPath.join(", "));
    }
}

} // namespace
