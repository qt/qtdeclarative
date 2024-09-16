// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
