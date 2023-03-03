#include "ZDOID.h"

const ZDOID ZDOID::NONE = ZDOID();

std::ostream& operator<<(std::ostream& st, ZDOID& zdoid) {
    return st << zdoid.m_uuid << ":" << zdoid.m_id;
}