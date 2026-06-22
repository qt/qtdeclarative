/****************************************************************************
**
** Copyright (C) 2017 Crimson AS <info@crimson.no>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the qmlbench tool.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "testmodel.h"

#include <QDebug>
#include <cmath>

TestModel::TestModel()
    : m_rowNumber(0)
{
    m_roleNames[StringRole] = "stringRole";
    m_roleNames[ByteArrayRole] = "byteArrayRole";
    m_roleNames[BoolRole] = "boolRole";
    m_roleNames[IntRole] = "intRole";
    m_roleNames[RealRole] = "realRole";
    m_roleNames[UrlRole] = "urlRole";
    m_roleNames[JsValueRole] = "jsValueRole";
    m_roleNames[DateTimeRole] = "dateTimeRole";
    m_roleNames[ColorRole] = "colorRole";
    m_roleNames[SizeRole] = "sizeRole";
    m_roleNames[PointRole] = "pointRole";
    m_roleNames[RectRole] = "rectRole";
}

TestModel::~TestModel()
{

}

int TestModel::rowNumber() const
{
    return m_rowNumber;
}

void TestModel::setRowNumber(int rowNumber)
{
    if (rowNumber == m_rowNumber)
        return;

    beginResetModel();
    m_rowNumber = rowNumber;

    // Only create data we don't already have.
    for (int i = m_strings.size(); i < m_rowNumber; ++i) {
        m_strings.append(QString("Test string %1").arg(i));
        m_byteArrays.append(QByteArray("Test ByteArray ") + QByteArray::number(i));
        m_bools.append(i % 2 == 0 ? true : false);
        m_ints.append(i);
        m_reals.append(i + 0.5);
        m_urls.append(QUrl(QString("http://example.org/some-thing-here/%1").arg(i)));
        m_jsValues.append(QJSValue(i)); // TODO: should we test object types (like strings?)
        m_dateTimes.append(QDateTime(QDate(1995, 5, 20), QTime(0, 0)).addSecs(i));
        m_colors.append(QColor::fromHslF(std::fmod(0.57 * i, 1.0), 0.5, 0.5));
        m_sizes.append(QSize(i, i));
        m_points.append(QPoint(i, i));
        m_rects.append(QRect(0, 0, i, i));
    }

    emit rowNumberChanged();
    endResetModel();
}

QHash<int, QByteArray> TestModel::roleNames() const
{
    return m_roleNames;
}

int TestModel::rowCount(const QModelIndex&) const
{
    return m_rowNumber;
}

QVariant TestModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case StringRole:
        return m_strings.at(index.row());
        break;
    case ByteArrayRole:
        return m_byteArrays.at(index.row());
        break;
    case BoolRole:
        return m_bools.at(index.row());
        break;
    case IntRole:
        return m_ints.at(index.row());
        break;
    case RealRole:
        return m_reals.at(index.row());
        break;
    case UrlRole:
        return m_urls.at(index.row());
        break;
    case JsValueRole:
        return QVariant::fromValue(m_jsValues.at(index.row()));
        break;
    case DateTimeRole:
        return m_dateTimes.at(index.row());
        break;
    case ColorRole:
        return m_colors.at(index.row());
        break;
    case SizeRole:
        return m_sizes.at(index.row());
        break;
    case PointRole:
        return m_points.at(index.row());
        break;
    case RectRole:
        return m_rects.at(index.row());
        break;
    }

    return QVariant();
}

