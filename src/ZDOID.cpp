#include "ZDOID.h"

const ZDOID ZDOID::NONE = ZDOID();

ZDOID::ZDOID(std::int64_t user_id, std::uint32_t id) {
    this->set_user_id(user_id);
    this->set_id(id);
}
