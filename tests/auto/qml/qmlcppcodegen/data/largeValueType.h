#include <qqmlintegration.h>
#include <qqml.h>

class LargeValueType
{
    Q_GADGET
    QML_VALUE_TYPE(largeValueType)
    Q_PROPERTY(int i READ i)

private:
    int i() const { return m_i; }

    char m_probablyDefinitelyBiggerThanQVariantInlineStorage[2048]{};
    int m_i = 5;
};
