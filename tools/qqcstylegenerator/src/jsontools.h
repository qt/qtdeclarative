// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <stdexcept>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

namespace JsonTools
{

struct NoImageFoundException: public std::runtime_error {
    NoImageFoundException(const QString &msg = {});
};

struct NoArtboardFoundException: public std::runtime_error {
    NoArtboardFoundException(const QString &msg = {});
};

// Json general API.
// Whenever one of the search functions below are called, the
// return value will also be stored in one of the following variables.
extern QJsonObject lastObject;
extern QJsonArray lastArray;
extern QJsonValue lastValue;
extern QStringList getChild_resolvedPath;

QJsonObject getObject(const QString &key, const QJsonObject object = lastObject);
QJsonArray getArray(const QString &key, const QJsonObject object = lastObject);
QJsonValue getValue(const QString &key, const QJsonObject object = lastObject);

// QtBridge specific API.
QJsonObject getArtboardSet(const QString &templateName, const QJsonDocument &doc);
QMap<QString, QJsonObject> getArtboards(const QJsonObject &artboardSet);
QJsonObject getChild(const QStringList &path, const QJsonObject parent, const QJsonDocument &doc);

QJsonObject getObjectInArrayWithName(const QString &name, const QJsonArray &array);
QString getMetadataString(const QString &key, const QJsonObject artboard);
QString getImagePath(const QJsonObject object);

} // namespace
