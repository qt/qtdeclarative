// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITPROPERTYRESOLVER_P_H
#define QQSTYLEKITPROPERTYRESOLVER_P_H

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

#include "qqstylekitglobal_p.h"

QT_BEGIN_NAMESPACE

class QQStyleKitControl;
class QQStyleKitControls;
class QQStyleKitStyle;
class QQStyleKitReader;
class QQStyleKitPropertyGroup;

class QQStyleKitPropertyResolver
{
    Q_GADGET

public:
    enum class PathId { // REMOVE THIS AS WELL
        ExcludeSubType,
        IncludeSubType
    };
    Q_ENUM(PathId)

    struct PropertyPathIds {
        PropertyPathId property;
        PropertyPathId alternative;
        PropertyPathId subTypeProperty;
        PropertyPathId subTypeAlternative;
    };

    static QVariant readStyleProperty(
        const QQStyleKitPropertyGroup *group,
        const QQSK::Property property,
        const QQSK::Property alternative = QQSK::Property::NoProperty);

    static bool writeStyleProperty(
        const QQStyleKitPropertyGroup *group,
        const QQSK::Property property,
        const QVariant &value);

    static bool hasLocalStyleProperty(
        const QQStyleKitPropertyGroup *group,
        const QQSK::Property property);

    static const QList<QQStyleKitExtendableControlType> baseTypesForType(
        QQStyleKitExtendableControlType exactType);

private:
    static bool s_styleWarningsIssued;
    static bool s_isReadingProperty;
    static QQSK::State s_cachedState;
    static QVarLengthArray<QQSK::StateFlag, 10> s_cachedStateList;

private:
    template <class T>
    static QVariant readPropertyInStorageForState(
        const PropertyPathId main, const PropertyPathId alternative,
        const T *storageProvider, QQSK::State state);

    template <class INDICES_CONTAINER>
    static QVariant readPropertyInControlForStates(
        const PropertyPathId main, const PropertyPathId alternative,
        const QQStyleKitControl *control, INDICES_CONTAINER &stateListIndices,
        int startIndex, int recursionLevel);

    static QVariant readPropertyInControl(
        const PropertyPathIds &ids, const QQStyleKitControl *control);

    static QVariant readPropertyInRelevantControls(
        const QQStyleKitControls *controls, const PropertyPathIds &ids,
        const QQStyleKitExtendableControlType exactType,
        const QList<QQStyleKitExtendableControlType> baseTypes);

    static QVariant readPropertyInStyle(
        const PropertyPathIds &ids,
        const QQStyleKitExtendableControlType exactType,
        const QList<QQStyleKitExtendableControlType> baseTypes,
        const QQStyleKitStyle *style);

    static QVariant readProperty(
        const PropertyPathIds &ids,
        QQStyleKitReader *styleReader,
        QQStyleKitStyle *style);

    static void cacheReaderState(QQSK::State state);
    static void addInstanceVariationsToReader(
        QQStyleKitReader *styleReader,
        const QStringList &inAppVariationNames,
        const QVarLengthArray<const QQStyleKitControls *, 6> &stylesAndThemes);
    static void addTypeVariationsToReader(
        QQStyleKitReader *styleReader,
        const QQStyleKitExtendableControlType parentType,
        const QQStyleKitStyle *style);
    static void rebuildVariationsForReader(
        QQStyleKitReader *styleReader,
        const QQStyleKitStyle *style);
};

QT_END_NAMESPACE

#endif // QQSTYLEKITPROPERTYRESOLVER_P_H
