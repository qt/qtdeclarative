// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef MODEL_H
#define MODEL_H

#include <QAbstractListModel>
#include <QStringList>

//![0]
class Animal
{
public:
    Animal(const QString &type, const QString &size);
//![0]

    QString type() const;
    QString size() const;

private:
    QString m_type;
    QString m_size;
//![1]
};

class AnimalModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum AnimalRoles {
        TypeRole = Qt::UserRole + 1,
        SizeRole
    };

    AnimalModel(QObject *parent = nullptr);
//![1]

    void addAnimal(const Animal &animal);

    int rowCount(const QModelIndex & parent = QModelIndex()) const;

    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

protected:
    QHash<int, QByteArray> roleNames() const;
private:
    QList<Animal> m_animals;
//![2]
};
//![2]

#endif // MODEL_H
