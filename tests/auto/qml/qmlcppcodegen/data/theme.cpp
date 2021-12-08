#include "theme.h"

namespace Utils {

Theme::Theme(QObject *parent) : QObject{parent} {}

int Theme::index(Area area) const
{
    switch(area) {
        case TopLeft: return 12;
        case BottomRight: return 13;
    }
    return -1;
}

}
