#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>

#include "ZDO.h"
#include "WorldManager.h"
#include "DataWriter.h"
#include "NetManager.h"
#include "VUtilsPhysics.h"
#include "DungeonManager.h"
#include "VUtils.h"

class VHTest {
public:
	VHTest() {}

public:
	void Test_ZDOConnectors();

};
