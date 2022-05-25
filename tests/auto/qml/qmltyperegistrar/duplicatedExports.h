/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef DUPLICATEDEXPORTS_H
#define DUPLICATEDEXPORTS_H

#include <QtCore/qobject.h>
#include <qqml.h>

class Exported : public QObject
{
    Q_OBJECT
public:
    Exported(QObject *parent = nullptr) : QObject(parent) { }
};

class ExportedQmlElement : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_VERSION(1, 2)
public:
    ExportedQmlElement(QObject *parent = nullptr) : QObject(parent) { }
};

class ExportedQmlElement2 : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ExportedQmlElement)
    QML_ADDED_IN_VERSION(1, 2)
public:
    ExportedQmlElement2(QObject *parent = nullptr) : QObject(parent) { }
};

class ExportedQmlElementDifferentVersion : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ExportedQmlElement)
    QML_ADDED_IN_VERSION(1, 0)
    QML_REMOVED_IN_VERSION(1, 7)
public:
    ExportedQmlElementDifferentVersion(QObject *parent = nullptr) : QObject(parent) { }
};

class SameNameSameExport : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SameNameSameExport)
    QML_FOREIGN(Exported)
    QML_ADDED_IN_VERSION(1, 2)
public:
    SameNameSameExport(QObject *parent = nullptr) : QObject(parent) { }
};
class SameNameSameExport2 : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SameNameSameExport)
    QML_FOREIGN(Exported)
    QML_ADDED_IN_VERSION(1, 2)
public:
    SameNameSameExport2(QObject *parent = nullptr) : QObject(parent) { }
};
class SameNameSameExportDifferentVersion : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SameNameSameExport)
    QML_FOREIGN(Exported)
    QML_ADDED_IN_VERSION(1, 0)
public:
    SameNameSameExportDifferentVersion(QObject *parent = nullptr) : QObject(parent) { }
};

struct SameName
{
    Q_GADGET
    QML_ELEMENT
    QML_FOREIGN(Exported);
    QML_ADDED_IN_VERSION(1, 2)
};
struct SameName2
{
    Q_GADGET
    QML_ELEMENT
    QML_FOREIGN(Exported);
    QML_ADDED_IN_VERSION(1, 2)
};
struct SameNameDifferentVersion
{
    Q_GADGET
    QML_ELEMENT
    QML_FOREIGN(Exported);
    QML_ADDED_IN_VERSION(1, 0)
};

#endif // DUPLICATEDEXPORTS_H
