// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SPREADCELL_H
#define SPREADCELL_H

#include <QVariant>

struct SpreadCell {
    friend struct DataModel;

    bool isNull() const;
    bool has(int role) const;
    void set(int role, const QVariant &data);
    QVariant get(int role) const;

private:
    QString displayText() const;

private:
    uint id = 0;
    QString text;
    bool highlight = false;
};

#endif // SPREADCELL_H
