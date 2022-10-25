#pragma once

#include "NetObject.h"

class Entity {
public:
	
};

class SyncEntity : public Entity {
public:
	std::unique_ptr<NetObject> m_obj;
};

class EntityArmorStand : public SyncEntity {

};

class EntityBed : public SyncEntity {

};

class EntityBeehive : public SyncEntity {

};

class EntityChair : public Entity {

};