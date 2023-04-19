#include "ZDOID.h"

const ZDOID ZDOID::NONE = ZDOID();

std::ostream& operator<<(std::ostream& st, ZDOID& zdoid) {
    return st << zdoid.GetOwner() << ":" << zdoid.GetUID();
}