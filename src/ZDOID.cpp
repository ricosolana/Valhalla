#include "ZDOID.h"

const ZDOID ZDOID::NONE = ZDOID();

ankerl::unordered_dense::segmented_vector<OWNER_t> ZDOID::INDEXED_UIDS;