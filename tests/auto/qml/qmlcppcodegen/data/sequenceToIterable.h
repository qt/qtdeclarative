// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SEQUENCETOITERABLE_H
#define SEQUENCETOITERABLE_H

#include <QtCore/qobject.h>
#include <QtQml/qqml.h>

class Entry : public QObject {
    Q_OBJECT

public:
    explicit Entry(const QString &name, QObject *parent = nullptr)
        : QObject(parent), m_name(name)
    {
        setObjectName(name);
    }

private:
    QString m_name;
};

class EntryWrapper {
    Q_GADGET
    QML_FOREIGN(Entry)
    QML_NAMED_ELEMENT(Entry)
    QML_UNCREATABLE("These are my Entry objects")
};

class EntryListRegistration
{
    Q_GADGET
    QML_FOREIGN(QList<Entry*>)
    QML_ANONYMOUS
    QML_SEQUENTIAL_CONTAINER(Entry*)
};

class EntrySource : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit EntrySource(QObject* parent = nullptr) : QObject(parent) {
        for (int i = 0; i < 10; i++) {
            m_entries.push_back(new Entry(QStringLiteral("Item %1").arg(i), this));
        }
    }
    Q_INVOKABLE QList<Entry*> getEntries() const { return m_entries; }

private:
    QList<Entry*> m_entries;
};

#endif // SEQUENCETOITERABLE_H
