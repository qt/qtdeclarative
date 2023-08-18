// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <stdexcept>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

namespace JsonTools
{

struct NoChildFoundException: public std::runtime_error {
    NoChildFoundException(const QString &msg = {});
};

struct RestCallException: public std::runtime_error {
    RestCallException(const QString &msg = {});
};

QJsonObject getObject(const QString &key, const QJsonObject object);
QJsonArray getArray(const QString &key, const QJsonObject object);
QJsonValue getValue(const QString &key, const QJsonObject object);
QString getString(const QString &key, const QJsonObject object);
QStringList getStringList(const QString &key, const QJsonObject object, bool required = true);

void clearCache();
QString resolvedPath(const QString &figmaId);
bool resolvedHidden(const QString &figmaId);

QList<QJsonObject> findChildren(const QStringList &keyValueList, const QJsonObject &root);
QJsonObject findChild(const QStringList &keyValueList, const QJsonObject &root, bool warnOnDuplicates);
QJsonObject findNamedChild(const QStringList &namePath, const QJsonObject &root, bool warnOnDuplicates);
QJsonObject findChildWithKey(const QString &key, const QJsonObject &root);

} // namespace
