#pragma once

#include "../ZDO.h"

class ObjectView {

public:
    ZDO* m_zdo;

    ObjectView(ZDO* zdo) : m_zdo(zdo) {}

    //virtual void Update() {}

    virtual void FixedUpdate() {}


};
