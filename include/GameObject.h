#pragma once

#include "NetObject.h"
#include "VUtils.h"
#include "Vector.h"

class GameObject {

private:
	Vector3 m_pos;
	//Quaternion m_rot;

	NetObject* m_view;



public:
	GameObject();

};
