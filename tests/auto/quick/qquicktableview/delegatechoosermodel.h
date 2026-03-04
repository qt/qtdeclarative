// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef DELEGATECHOOSERMODEL_H
#define DELEGATECHOOSERMODEL_H

#include <QtCore/QtCore>
#include <QtGui/QStandardItemModel>

#include <array>

class DelegateChooserModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Role {
        ChoiceRole = Qt::UserRole,
        Choice0DataRole,
        Choice1DataRole,
    };

    DelegateChooserModel(int rows, int columns, QObject *parent = nullptr) : QAbstractTableModel(parent), m_rows(rows), m_columns(columns)
    {
    }

    int rowCount(const QModelIndex &parent) const override { return parent.isValid() ? 0 : m_rows; }
    int columnCount(const QModelIndex &parent) const override { return parent.isValid() ? 0 : m_columns; }

    QHash<int, QByteArray> roleNames() const override
    {
        using namespace Qt::StringLiterals;
        return {
            { Qt::DisplayRole, "display"_ba     },
            { ChoiceRole,      "choice"_ba      },
            { Choice0DataRole, "choice0Data"_ba },
            { Choice1DataRole, "choice1Data"_ba },
        };
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        using namespace Qt::StringLiterals;

        QVariant ret;

        if (!index.isValid()) {
            return ret;
        }

        switch (role) {
        case Qt::DisplayRole:
            ret = m_choice ? u"Text"_s : u"Rectangle"_s;
            break;
        case ChoiceRole:
            ret = m_choice;
            break;
        case Choice0DataRole:
            ret = QColor{ Qt::red };
            break;
        case Choice1DataRole:
            ret = u"Text"_s;
            break;
        }

        return ret;
    }

    Q_INVOKABLE void switchChoice()
    {
        if (m_choice) {
            m_choice = 0;
        } else {
            m_choice = 1;
        }

        m_choice_delegates_count.fill(0);

        Q_EMIT dataChanged(index(0, 0), index(m_rows - 1, m_columns - 1), { ChoiceRole });
    }

    Q_INVOKABLE void delegate_created(int choice) { ++m_choice_delegates_count.at(static_cast<unsigned>(choice)); }

    const std::array<int, 2> &choice_delegates_count() const { return m_choice_delegates_count; }

private:
    int m_rows;
    int m_columns;
    int m_choice{ };
    std::array<int, 2> m_choice_delegates_count{ };
};

#endif
