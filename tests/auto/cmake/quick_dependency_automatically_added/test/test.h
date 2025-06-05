#pragma once

#include <QStringListModel>
#include <qqmlintegration.h>

struct DataModel : QStringListModel
{
    Q_OBJECT
    QML_ELEMENT
};
