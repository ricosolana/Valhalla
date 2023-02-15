#pragma once

#include <robin_hood.h>
#include "Object.h"

// prefab: guard_stone
// component: PrivateArea
struct Ward : public ObjectView {
	const std::string& GetCreatorName() {
		return m_zdo->GetString("creatorName", "");
	}

	//List<KeyValuePair<long, string>> list = new List<KeyValuePair<long, string>>();
	//int @int = this.m_nview.GetZDO().GetInt("permitted", 0);
	//for (int i = 0; i < @int; i++)
	//{
	//	long @long = this.m_nview.GetZDO().GetLong("pu_id" + i.ToString(), 0L);
	//	string @string = this.m_nview.GetZDO().GetString("pu_name" + i.ToString(), "");
	//	if (@long != 0L)
	//	{
	//		list.Add(new KeyValuePair<long, string>(@long, @string));
	//	}
	//}
	//
	//robin_hood::unordered_map<

};