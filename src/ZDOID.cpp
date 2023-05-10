#include "ZDOID.h"

const ZDOID ZDOID::NONE = ZDOID();

std::ostream& operator<<(std::ostream& st, const ZDOID& zdoid) {
    return st << zdoid.GetOwner() << ":" << zdoid.GetUID();
}