#include "sequencetypeexample.h"

SequenceTypeExample::SequenceTypeExample()
    : QObject()
{
    m_list << 1.1 << 2.2 << 3.3;
}

QList<qreal> SequenceTypeExample::qrealListProperty() const
{
    return m_list;
}

void SequenceTypeExample::setQrealListProperty(const QList<qreal> &list)
{
    m_list = list;
    emit qrealListPropertyChanged();
}
