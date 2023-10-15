#include "ZDOID.h"

const ZDOID ZDOID::NONE = ZDOID();

// TODO use inline static within class
//decltype(ZDOID::INDEXED_USERS) ZDOID::INDEXED_USERS;

//UNORDERED_MAP_t<OWNER_t, int> TEMP_USAGE_COUNTS;

ZDOID::ZDOID(OWNER_t owner, uint32_t uid) {
    this->SetOwner(owner);
    this->SetUID(uid);
    
    //TEMP_USAGE_COUNTS[owner]++;
}
