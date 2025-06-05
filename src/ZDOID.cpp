#include "ZDOID.h"

const ZDOID ZDOID::NONE = ZDOID();

// TODO use inline static within class
//decltype(ZDOID::INDEXED_USERS) ZDOID::INDEXED_USERS;

//UNORDERED_MAP_t<USER_ID_t, int> TEMP_USAGE_COUNTS;

ZDOID::ZDOID(std::int64_t user_id, std::uint32_t id) {
    this->set_user_id(user_id);
    this->set_id(id);
    
    //TEMP_USAGE_COUNTS[owner]++;
}
