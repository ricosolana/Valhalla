#include "ZDOID.h"

const ZDOID ZDOID::NONE = ZDOID();

ZDOID::ZDOID(std::int64_t user_id, std::uint32_t id)
{
    this->set_user_id(user_id);
    this->set_id(id);
}

std::ostream &operator<<(std::ostream &st, ZDOID const &zdoid)
{
    st << zdoid.get_user_id() << ":" << zdoid.get_id();
    return st;
}