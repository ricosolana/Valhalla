#pragma once

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "DataStream.h"
#include "DungeonManager.h"
#include "NetManager.h"
#include "VUtils.h"
#include "VUtilsPhysics.h"
#include "VUtilsResource.h"
#include "WorldManager.h"
#include "ZDO.h"

class VHTest
{
  public:
    VHTest()
    {
        //VUtils::Resource::
        //std::filesystem::pa
    }

  private:
    //template<typename T>
    //void ZDO_SetGet(ZDO& zdo, std::string_view key, const T& value) {
    //	zdo.Set(key, value);
    //	//auto&& ptr = zdo.Get<std::remove_cvref_t<decltype(value)>>(key);
    //	const T* ptr = zdo.Get<T>(key);
    //	assert(ptr && "Member missing");
    //	assert(*ptr == value && "Member mismatch");
    //}

    // Set ZDO dummy values
    void ZDO_Sets(ZDO zdo);

    // Tests ZDO gets
    void Test_ZDO_Gets(ZDO zdo);

  public:
    void Test_ZDOConnectors();

    void Test_Quaternion();

    void Test_ZDO_SetsGets();

    //void ZDO_SetsGets();
    void Test_ZDO_LoadSave();
};
