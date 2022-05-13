// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QAbstractListModel>
#include <qqml.h>

class TypeRoleModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Role {
        TypeRole = Qt::UserRole + 1,
        TextRole,
    };
    Q_ENUM(Role)

    enum class Type { PlainText, Markdown, Rect };
    Q_ENUM(Type)

    explicit TypeRoleModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override { return _mapRoleNames; }

private:
    QHash<int, QByteArray> _mapRoleNames;
};
