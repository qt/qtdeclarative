// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qqmltreerow_p.h"

#include <QtCore/qjsonobject.h>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

QQmlTreeRow::QQmlTreeRow(QQmlTreeRow *parentItem)
    : m_parent(parentItem)
{

}

QQmlTreeRow::QQmlTreeRow(const QVariant &data, QQmlTreeRow *parentItem)
    : m_parent(parentItem)
{
    QVariantMap variantMap = data.toMap();
    unpackVariantMap(variantMap);
}

QQmlTreeRow::QQmlTreeRow(const QVariantMap &data, QQmlTreeRow *parentItem)
    : m_parent(parentItem)
{
    unpackVariantMap(data);
}

QQmlTreeRow::~QQmlTreeRow()
{
    // Letting m_children destroy the subtree would recurse once per level of
    // the tree, which can be arbitrarily deep. Detach the children level by
    // level instead, so that every row is destroyed without any children left.
    std::vector<std::unique_ptr<QQmlTreeRow>> pending = std::move(m_children);
    m_children.clear();

    while (!pending.empty()) {
        std::unique_ptr<QQmlTreeRow> row = std::move(pending.back());
        pending.pop_back();

        for (auto &child : row->m_children)
            pending.push_back(std::move(child));
        row->m_children.clear();
    }
}

void QQmlTreeRow::addChild(QQmlTreeRow *child)
{
    m_children.push_back(std::unique_ptr<QQmlTreeRow>(child));
    child->setParent(this);
}

void QQmlTreeRow::insertChild(int index, std::unique_ptr<QQmlTreeRow> child)
{
    child->setParent(this);
    m_children.insert(m_children.begin() + index, std::move(child));
}

void QQmlTreeRow::unpackVariantMap(const QVariantMap &variantMap)
{
    // Building the children through the constructor would recurse once per
    // level of the nested rows, which can be arbitrarily deep. Use an explicit
    // stack instead: each entry is a row whose data and children still have to
    // be filled in from the given map.
    std::vector<std::pair<QQmlTreeRow *, QVariantMap>> pending;
    pending.emplace_back(this, variantMap);

    while (!pending.empty()) {
        auto [row, map] = std::move(pending.back());
        pending.pop_back();

        for (auto it = map.cbegin(); it != map.cend(); ++it) {
            const QString& key = it.key();
            const QVariant& value = it.value();

            if ((value.typeId() == QMetaType::Type::QVariantList) && (key == ROWS_PROPERTY_NAME)) {
                const QList<QVariant> variantList = value.toList();
                for (const QVariant &rowAsVariant : variantList) {
                    auto child = std::make_unique<QQmlTreeRow>(row);
                    pending.emplace_back(child.get(), rowAsVariant.toMap());
                    row->m_children.push_back(std::move(child));
                }
            } else {
                row->dataMap.insert(key, value);
            }
        }
    }
}

void QQmlTreeRow::removeChild(std::vector<std::unique_ptr<QQmlTreeRow>>::const_iterator &child)
{
    m_children.erase(child);
}

void QQmlTreeRow::removeChildAt(int i)
{
    m_children.erase(std::next(m_children.begin(), i));
}

std::unique_ptr<QQmlTreeRow> QQmlTreeRow::takeChild(int i)
{
    auto it = std::next(m_children.begin(), i);
    std::unique_ptr<QQmlTreeRow> child = std::move(*it);
    m_children.erase(it);
    return child;
}

void QQmlTreeRow::setData(const QVariant &data)
{
    dataMap.clear();
    const QVariantMap variantMap = data.toJsonObject().toVariantMap();
    unpackVariantMap(variantMap);
}

void QQmlTreeRow::setData(const QVariantMap &data)
{
    dataMap.clear();
    unpackVariantMap(data);
}

void QQmlTreeRow::setField(const QString &key, const QVariant &value)
{
    if (dataMap.contains(key))
        dataMap[key] = value;
}

QVariant QQmlTreeRow::toVariant() const
{
    // Recursing into the children would recurse once per level of the tree,
    // which can be arbitrarily deep. Use an explicit stack instead, in
    // post-order: a row's variant is complete once the variants of all its
    // children have been collected.
    struct Frame
    {
        const QQmlTreeRow *row;
        size_t nextChild = 0;
        QVariantList children = {};
    };

    std::vector<Frame> pending;
    pending.push_back({ this });

    QVariant result;
    while (!pending.empty()) {
        Frame &top = pending.back();
        if (top.nextChild < top.row->m_children.size()) {
            const QQmlTreeRow *child = top.row->m_children[top.nextChild++].get();
            pending.push_back({ child }); // invalidates top
            continue;
        }

        QVariantMap variantMap(top.row->dataMap);
        if (!top.row->m_children.empty())
            variantMap[ROWS_PROPERTY_NAME] = top.children;
        pending.pop_back(); // invalidates top
        if (pending.empty())
            result = variantMap;
        else
            pending.back().children.append(QVariant(variantMap));
    }

    return result;
}

int QQmlTreeRow::subTreeSize() const
{
    // Recursing into the children would recurse once per level of the tree,
    // which can be arbitrarily deep. Count the rows using an explicit stack.
    int size = 0;

    std::vector<const QQmlTreeRow *> pending{ this };
    while (!pending.empty()) {
        const QQmlTreeRow *row = pending.back();
        pending.pop_back();

        ++size;
        for (const auto &child : row->m_children)
            pending.push_back(child.get());
    }

    return size;
}

QT_END_NAMESPACE
