// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef THEME_H
#define THEME_H

#include <QtCore/qobject.h>
#include <QtCore/qrect.h>

namespace Utils {
class Theme : public QObject
{
    Q_OBJECT
public:
    explicit Theme(QObject *parent = nullptr);

    enum Area {
        TopLeft = 31,
        BottomRight = 47
    };

    Q_ENUM(Area);

    Q_INVOKABLE int index(Utils::Theme::Area area) const;
    Q_INVOKABLE QRectF area(Utils::Theme::Area area) const;
};
}

#endif // THEME_H
