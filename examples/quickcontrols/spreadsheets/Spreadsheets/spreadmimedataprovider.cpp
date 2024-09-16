// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "spreadmimedataprovider.h"

#include <QMimeData>
#ifndef QT_NO_CLIPBOARD
#include <QGuiApplication>
#include <QClipboard>
#endif

namespace {
static inline constexpr auto MIMETYPE_SPREADMODEL = "application/x-qtexamplespreadmodel";
static inline constexpr auto MIMETYPE_SELECTEDDATA = "model/data";
static inline constexpr auto MIMETYPE_TEXT = "text/plain";
}

QMimeData *SpreadMimeDataProvider::saveToMimeData() const
{
    if (m_data.empty())
        return nullptr;

    QByteArray data;
    QDataStream stream{&data, QDataStream::WriteOnly};
    for (auto it = m_data.begin(); it != m_data.end(); ++it) {
        const QPoint &cell = it->first;
        const QMap<int, QVariant> &item_data = it->second;
        stream << cell.x() << cell.y() << item_data;
    }

    QMimeData *mime_data = new QMimeData{};
    mime_data->setData(MIMETYPE_SPREADMODEL, QByteArray{});
    mime_data->setData(MIMETYPE_SELECTEDDATA, data);
    return mime_data;
}

bool SpreadMimeDataProvider::loadFromMimeData(const QMimeData *mimeData)
{
    if (!mimeData)
        return false;

    if (!mimeData->hasFormat(MIMETYPE_SPREADMODEL))
        return false;

    QByteArray data = mimeData->data(MIMETYPE_SELECTEDDATA);
    QDataStream stream{&data, QDataStream::ReadOnly};
    while (!stream.atEnd()) {
        QPoint cell;
        QMap<int, QVariant> item_data;
        stream >> cell.rx() >> cell.ry() >> item_data;
        m_data.push_back(std::make_pair(cell, item_data));
    }

    return true;
}

bool SpreadMimeDataProvider::saveToClipboard()
{
#ifdef QT_NO_CLIPBOARD
    qWarning() << "Clipboard is not supported";
    return false;
#else
    QMimeData *mime_data = saveToMimeData();
    if (!mime_data)
        return false;

    QGuiApplication::clipboard()->setMimeData(mime_data);
    return true;
#endif
}

bool SpreadMimeDataProvider::loadFromClipboard()
{
#ifdef QT_NO_CLIPBOARD
    qWarning() << "Clipboard is not supported";
    return false;
#else
    const QMimeData *mime_data = QGuiApplication::clipboard()->mimeData();
    if (!mime_data)
        return false;

    return loadFromMimeData(mime_data);
#endif
}

bool SpreadMimeDataProvider::saveDataToModel(int index,
                                             const QModelIndex &modelIndex,
                                             QAbstractItemModel *model) const
{
    const QMap<int, QVariant> &item_data = m_data.at(index).second;
    return item_data.isEmpty() ? model->clearItemData(modelIndex)
                               : model->setItemData(modelIndex, item_data);
}

void SpreadMimeDataProvider::loadDataFromModel(const QPoint &cell,
                                               const QModelIndex &index,
                                               const QAbstractItemModel *model)
{
    const QMap<int, QVariant> &item_data = model->itemData(index);
    m_data.push_back(std::make_pair(cell, item_data));
}
