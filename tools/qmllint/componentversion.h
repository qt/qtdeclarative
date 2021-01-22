/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef COMPONENTVERSION_H
#define COMPONENTVERSION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <QtCore/qglobal.h>

class ComponentVersion
{
public:
    static const int NoVersion = -1;

    ComponentVersion() = default;
    ComponentVersion(int major, int minor) : m_major(major), m_minor(minor) {}
    explicit ComponentVersion(const QString &versionString);

    int majorVersion() const { return m_major; }
    int minorVersion() const { return m_minor; }

    bool isValid() const { return m_major >= 0 && m_minor >= 0; }

private:
    int m_major = NoVersion;
    int m_minor = NoVersion;
};

bool operator<(const ComponentVersion &lhs, const ComponentVersion &rhs);
bool operator<=(const ComponentVersion &lhs, const ComponentVersion &rhs);
bool operator>(const ComponentVersion &lhs, const ComponentVersion &rhs);
bool operator>=(const ComponentVersion &lhs, const ComponentVersion &rhs);
bool operator==(const ComponentVersion &lhs, const ComponentVersion &rhs);
bool operator!=(const ComponentVersion &lhs, const ComponentVersion &rhs);

#endif // COMPONENTVERSION_H
