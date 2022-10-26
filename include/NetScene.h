#pragma once

#include "NetSync.h"
#include "NetObject.h"


namespace NetScene {

	void Init();

	void Shutdown();

	void AddInstance(ZDO zdo, ZNetView *nview);

	void Destroy(GameObject go);

	GameObject GetPrefab(int hash);

	GameObject GetPrefab(const std::string &name);

	int GetPrefabHash(GameObject go);

	bool IsAreaReady(const Vector3& point);

	ZNetView FindInstance(ZDO *zdo);

	bool HaveInstance(ZDO *zdo);

	GameObject FindInstance(const ZDOID &id);

	void Update();

	bool InActiveArea(const Vector2i &zone, const Vector3 &refPoint);

	bool InActiveArea(const Vector2i &zone, const Vector2i &refCenterZone);

	bool OutsideActiveArea(const Vector3 &point);

	bool OutsideActiveArea(const Vector3 &point, const Vector3 &refPoint);

	bool HaveInstanceInSector(const Vector2i &sector);

	int NrOfInstances();

	void SpawnObject(const Vector3 &pos, const Quaternion &rot, GameObject prefab);

	List<string> GetPrefabNames();

	List<GameObject> m_prefabs = new List<GameObject>();

	List<GameObject> m_nonNetViewPrefabs = new List<GameObject>();

}
