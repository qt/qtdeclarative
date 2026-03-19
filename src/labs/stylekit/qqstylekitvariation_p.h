// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQSTYLEKITVARIATION_P_H
#define QQSTYLEKITVARIATION_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtQml/QtQml>

#include "qqstylekitcontrols_p.h"

QT_BEGIN_NAMESPACE

class QQStyleKitVariationAttached;
class QQStyleKitStyleAndThemeBase;
class QQStyleKitStyle;

class QQStyleKitVariation : public QQStyleKitControls
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged FINAL)
    QML_ATTACHED(QQStyleKitVariationAttached)
    QML_NAMED_ELEMENT(StyleVariation)

public:
    QQStyleKitVariation(QObject *parent = nullptr);

    void componentComplete() override;

    QString name() const;
    void setName(const QString &name);

    static QQStyleKitVariationAttached *qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void nameChanged();

private:
    Q_DISABLE_COPY(QQStyleKitVariation)

    static void resetVariationsForStyle(QQStyleKitStyle *style);

    QString m_name;
    QList<QPointer<const QQStyleKitStyleAndThemeBase>> m_usageContext;

    friend class QQStyleKitPropertyResolver;
    friend class QQStyleKitStyle;
};

class QQStyleKitVariationAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList variations READ variations WRITE setVariations NOTIFY variationsChanged FINAL)
    Q_PROPERTY(QQStyleKitExtendableControlType controlType READ controlType WRITE setControlType NOTIFY controlTypeChanged FINAL)

public:
    QQStyleKitVariationAttached(QObject *parent);

     QStringList variations() const;
     void setVariations(const QStringList &variations);

     QQStyleKitExtendableControlType controlType() const;
     void setControlType(QQStyleKitExtendableControlType type);

 signals:
     void variationsChanged();
     void controlTypeChanged();

 private:
     QStringList m_variations;
     QQStyleKitExtendableControlType m_controlType = QQStyleKitReader::ControlType::Unspecified;

     friend class QQStyleKit;
     friend class QQStyleKitPropertyResolver;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITVARIATION_P_H
