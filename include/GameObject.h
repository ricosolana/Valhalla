#pragma once

#include "NetObject.h"
#include "VUtils.h"
#include "Vector.h"

struct Transform {};

class GameObject {
public:
	std::string name;
	Transform transform;

private:
	Vector3 m_pos;
	//Quaternion m_rot;

	NetObject* m_view;



public:
	GameObject();

};
