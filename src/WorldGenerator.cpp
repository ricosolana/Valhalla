#include "WorldGenerator.h"
#include "VUtilsRandom.h"
#include "HashUtils.h"
#include "ZoneSystem.h"
#include "VUtilsResource.h"

namespace WorldGenerator {

	struct River {
		Vector2 p0;
		Vector2 p1;
		Vector2 center;
		float widthMin;
		float widthMax;
		float curveWidth;
		float curveWavelength;

		River() {}

		//River(const River& other) = delete;
	};

	struct RiverPoint {
		Vector2 p;
		float w;
		float w2;

		RiverPoint(Vector2 p_p, float p_w) {
			p = p_p;
			w = p_w;
			w2 = p_w * p_w;
		}

		//RiverPoint(const RiverPoint& other) = delete; // copy is deleteed because its not needed
	};

	static constexpr float m_waterTreshold = 0.05f;

	World m_world;
	int m_version;
	float m_offset0;
	float m_offset1;
	float m_offset2;
	float m_offset3;
	float m_offset4;
	int m_riverSeed;
	int m_streamSeed;
	std::vector<Vector2> m_mountains;
	std::vector<Vector2> m_lakes;
	std::vector<River> m_rivers;
	std::vector<River> m_streams;
	robin_hood::unordered_map<Vector2i, std::vector<RiverPoint>, HashUtils::Hasher> m_riverPoints;
	//std::vector<RiverPoint> m_cachedRiverPoints; //RiverPoint[] m_cachedRiverPoints;
	std::vector<RiverPoint>* m_cachedRiverPoints;
	Vector2i m_cachedRiverGrid = { -999999, -999999 };
	//ReaderWriterLockSlim m_riverCacheLock; // c# lock mechanism?
	//std::vector<Heightmap::Biome> m_biomes; // seems unused

	static constexpr float	riverGridSize = 64;
	static constexpr float	minRiverWidth = 60;
	static constexpr float	maxRiverWidth = 100;
	static constexpr float	minRiverCurveWidth = 50;
	static constexpr float	maxRiverCurveWidth = 80;
	static constexpr float	minRiverCurveWaveLength = 50;
	static constexpr float	maxRiverCurveWaveLength = 70;
	static constexpr int	streams = 3000;
	static constexpr float	streamWidth = 20;
	static constexpr float	meadowsMaxDistance = 5000;
	static constexpr float	minDeepForestNoise = 0.4f;
	static constexpr float	minDeepForestDistance = 600;
	static constexpr float	maxDeepForestDistance = 6000;
	static constexpr float	deepForestForestFactorMax = 0.9f;
	// Marsh is swamp
	static constexpr float	marshBiomeScale = 0.001f;
	static constexpr float	minMarshNoise = 0.6f;
	static constexpr float	minMarshDistance = 2000;
	static constexpr float	maxMarshDistance = 8000;
	static constexpr float	minMarshHeight = 0.05f;
	static constexpr float	maxMarshHeight = 0.25f;
	// Heath is plains
	//	PROOF: heathColor in MiniMap is used for plains biome
	static constexpr float	heathBiomeScale = 0.001f;
	static constexpr float	minHeathNoise = 0.4f;
	static constexpr float	minHeathDistance = 3000;
	static constexpr float	maxHeathDistance = 8000;
	// Darklands is mistlands
	//	PROOF: not definite, but these values are nearby in usage
	static constexpr float	darklandBiomeScale = 0.001f;
	static constexpr float	minDarklandNoise = 0.5f;
	static constexpr float	minDarklandDistance = 6000;
	static constexpr float	maxDarklandDistance = 10000;
	// ocean
	static constexpr float	oceanBiomeScale = 0.0005f;
	static constexpr float	oceanBiomeMinNoise = 0.4f;
	static constexpr float	oceanBiomeMaxNoise = 0.6f;
	static constexpr float	oceanBiomeMinDistance = 1000;
	static constexpr float	oceanBiomeMinDistanceBuffer = 256;
	static constexpr float	m_minMountainDistance = 1000; // usually mutable because version changes this
	static constexpr float	mountainBaseHeightMin = 0.4f;
	static constexpr float	deepNorthMinDistance = 12000;
	static constexpr float	deepNorthYOffset = 4000;
	static constexpr float	ashlandsMinDistance = 12000;
	static constexpr float	ashlandsYOffset = -4000;


	/*
	* See https://docs.unity3d.com/ScriptReference/Random.Range.html
	*	for random algorithm implementation discussion
	*	A c++ implementation of Unity.Random.Range needs to be created
	*		to perfectly recreate Valheim worldgen
	*/


	// Forward declarations
	void Pregenerate();
	void FindMountains();
	void FindLakes();
	std::vector<Vector2> MergePoints(std::vector<Vector2>& points, float range);
	int FindClosest(const std::vector<Vector2>& points, const Vector2& p, float maxDistance);
	std::vector<River> PlaceStreams();
	bool FindStreamEndPoint(VUtils::Random::State& state, int iterations, float minHeight, float maxHeight, const Vector2& start, float minLength, float maxLength, Vector2& end);
	bool FindStreamStartPoint(VUtils::Random::State& state, int iterations, float minHeight, float maxHeight, Vector2& p, float& starth);
	std::vector<River> PlaceRivers();
	int FindClosestRiverEnd(const std::vector<River>& rivers, const std::vector<Vector2>& points, const Vector2& p, float maxDistance, float heightLimit, float checkStep);
	int FindRandomRiverEnd(VUtils::Random::State& state, const std::vector<River>& rivers, const std::vector<Vector2>& points, const Vector2& p, float maxDistance, float heightLimit, float checkStep);
	bool HaveRiver(const std::vector<River>& rivers, const Vector2& p0);
	bool HaveRiver(const std::vector<River>& rivers, const Vector2& p0, const Vector2& p1);
	bool IsRiverAllowed(const Vector2& p0, const Vector2& p1, float step, float heightLimit);
	void RenderRivers(VUtils::Random::State& state, const std::vector<River>& rivers);
    void AddRiverPoint(robin_hood::unordered_map<Vector2i, std::vector<RiverPoint>, HashUtils::Hasher>& riverPoints,
                       const Vector2& p,
                       float r);
    void AddRiverPoint(robin_hood::unordered_map<Vector2i, std::vector<RiverPoint>, HashUtils::Hasher>& riverPoints, const Vector2i& grid, const Vector2& p, float r);
	bool InsideRiverGrid(const Vector2i& grid, const Vector2& p, float r);
	Vector2i GetRiverGrid(float wx, float wy);
	void GetRiverWeight(float wx, float wy, float& weight, float& width);
	void GetWeight(const std::vector<RiverPoint>& points, float wx, float wy, float& weight, float& width);
	void GenerateBiomes();
	float WorldAngle(float wx, float wy);
	float GetBaseHeight(float wx, float wy);
	float AddRivers(float wx, float wy, float h);
	float GetHeight(float wx, float wy);
	float GetBiomeHeight(Heightmap::Biome biome, float wx, float wy);
	float GetMarshHeight(float wx, float wy);
	float GetMeadowsHeight(float wx, float wy);
	float GetForestHeight(float wx, float wy);
	float GetPlainsHeight(float wx, float wy);
	float GetAshlandsHeight(float wx, float wy);
	float GetEdgeHeight(float wx, float wy);
	float GetOceanHeight(float wx, float wy);
	float BaseHeightTilt(float wx, float wy);
	float GetSnowMountainHeight(float wx, float wy);
	float GetDeepNorthHeight(float wx, float wy);



	void Init(World world) {
		m_world = world;
		m_version = m_world.m_worldGenVersion;

		VUtils::Random::State state(m_world.m_seed);
		m_offset0 = (float) state.Range(-10000, 10000);
		m_offset1 = (float) state.Range(-10000, 10000);
		m_offset2 = (float) state.Range(-10000, 10000);
		m_offset3 = (float) state.Range(-10000, 10000);
		m_riverSeed = state.Range(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
		m_streamSeed = state.Range(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
		m_offset4 = (float) state.Range(-10000, 10000);

		// TODO rename the geenrator functions not to 'FIND' but 'Generate' to be much more concise

		Pregenerate();
	}

	void Pregenerate() {
		FindMountains();
		FindLakes();
		m_rivers = PlaceRivers();
		m_streams = PlaceStreams();
	}

	void FindMountains() {
		std::vector<Vector2> list;
		for (float num = -10000; num <= 10000; num += 128)
		{
			for (float num2 = -10000; num2 <= 10000; num2 += 128)
			{
				if (VUtils::Math::Magnitude(num2, num) <= 10000
					&& GetBaseHeight(num2, num) > 0.45f)
				{
					list.push_back(Vector2(num2, num));
				}
			}
		}
		m_mountains = MergePoints(list, 800);
	}

	void FindLakes() {
		std::vector<Vector2> list;
		for (float num = -10000; num <= 10000; num += 128)
		{
			for (float num2 = -10000; num2 <= 10000; num2 += 128)
			{
				if (VUtils::Math::Magnitude(num2, num) <= 10000
					&& GetBaseHeight(num2, num) < 0.05f)
				{
					list.push_back(Vector2(num2, num));
				}
			}
		}
		m_lakes = MergePoints(list, 800);
	}

	// Basically blender merge nearby vertices
	std::vector<Vector2> MergePoints(std::vector<Vector2>& points, float range) {
		std::vector<Vector2> list;
		while (!points.empty()) {
			Vector2 vector = points[0];
			points.erase(points.begin()); // not efficient for vector
			while (!points.empty()) {
				int num = FindClosest(points, vector, range);
				if (num == -1)
				{
					break;
				}
				vector = (vector + points[num]) * 0.5f;
				points[num] = points[points.size() - 1];
				points.pop_back(); // .erase(points.end());
			}
			list.push_back(vector);
		}
		return list;
	}

	// Return the index in points of the nearest point to p
	int FindClosest(const std::vector<Vector2>& points, const Vector2& p, float maxDistance) {
		int result = -1;
		float num = std::numeric_limits<float>::max();
		for (int i = 0; i < points.size(); i++)
		{
			if (!(points[i] == p))
			{
				//float num2 = p.Distance(points[i]); // not optimal
				float num2 = p.SqDistance(points[i]);
				if (num2 < maxDistance * maxDistance && num2 < num)
				{
					result = i;
					num = num2;
				}
			}
		}
		return result;
	}

	std::vector<River> PlaceStreams() {
		VUtils::Random::State state(m_streamSeed);
		std::vector<River> list;
		int num = 0;
		for (int i = 0; i < streams; i++) {
			Vector2 vector;
			float num2;
			Vector2 vector2; // out
			if (FindStreamStartPoint(state, 100, 26, 31, vector, num2)
				&& FindStreamEndPoint(state, 100, 36, 44, vector, 80, 200, vector2)) {
				Vector2 vector3 = (vector + vector2) * 0.5f;
				float height = GetHeight(vector3.x, vector3.y);
				if (height >= 26 && height <= 44) {
					River river;
					river.p0 = vector;
					river.p1 = vector2;
					river.center = vector3;
					river.widthMax = 20;
					river.widthMin = 20;
					float num3 = river.p0.Distance(river.p1); // could be optimized
					river.curveWidth = num3 / 15;
					river.curveWavelength = num3 / 20;
					list.push_back(river); // use move / emplacer
					num++;
				}
			}
		}
		RenderRivers(state, list);
		return list;
	}

	bool FindStreamEndPoint(VUtils::Random::State &state, int iterations, float minHeight, float maxHeight, const Vector2& start, float minLength, float maxLength, Vector2& end) {
		float num = (maxLength - minLength) / (float)iterations;
		float num2 = maxLength;
		for (int i = 0; i < iterations; i++) {
			num2 -= num;
			float f = state.Range(0.f, PI * 2.0f);
			Vector2 vector = start + Vector2(sin(f), cos(f)) * num2;
			float height = GetHeight(vector.x, vector.y);
			if (height > minHeight && height < maxHeight)
			{
				end = vector;
				return true;
			}
		}
		end = Vector2::ZERO;
		return false;
	}

	bool FindStreamStartPoint(VUtils::Random::State& state, int iterations, float minHeight, float maxHeight, Vector2& p, float& starth) {
		for (int i = 0; i < iterations; i++) {
			float num = state.Range(-10000.f, 10000.f);
			float num2 = state.Range(-10000.f, 10000.f);
			float height = GetHeight(num, num2);
			if (height > minHeight && height < maxHeight)
			{
				p = Vector2(num, num2);
				starth = height;
				return true;
			}
		}
		p = Vector2::ZERO;
		starth = 0;
		return false;
	}

	std::vector<River> PlaceRivers() {
		VUtils::Random::State state(m_riverSeed);

		std::vector<River> list;
		std::vector<Vector2> list2(m_lakes);

		while (list2.size() > 1)
		{
			Vector2 vector = list2[0];
			int num = FindRandomRiverEnd(state, list, m_lakes, vector, 2000, 0.4f, 128);
			if (num == -1 && !HaveRiver(list, vector)) {
				num = FindRandomRiverEnd(state, list, m_lakes, vector, 5000, 0.4f, 128);
			}

			if (num != -1) {
				River river;
				river.p0 = vector;
				river.p1 = m_lakes[num];
				river.center = (river.p0 + river.p1) * 0.5f;
				river.widthMax = state.Range(minRiverWidth, maxRiverWidth);
				river.widthMin = state.Range(minRiverWidth, river.widthMax);
				float num2 = river.p0.Distance(river.p1);
				river.curveWidth = num2 / 15;
				river.curveWavelength = num2 / 20;
				list.push_back(river);
			}
			else
			{
				list2.erase(list2.begin());
			}
		}
		RenderRivers(state, list);
		return list;
	}

	int FindClosestRiverEnd(const std::vector<River>& rivers, const std::vector<Vector2>& points, const Vector2& p, float maxDistance, float heightLimit, float checkStep) {
		int result = -1;
		float num = 99999;
		for (auto i = 0; i < points.size(); i++) {
			if (!(points[i] == p)) {
				float num2 = p.Distance(points[i]);
				if (num2 < maxDistance && num2 < num
					&& !HaveRiver(rivers, p, points[i])
					&& IsRiverAllowed(p, points[i], checkStep, heightLimit))
				{
					result = i;
					num = num2;
				}
			}
		}
		return result;
	}

	int FindRandomRiverEnd(VUtils::Random::State& state, const std::vector<River>& rivers, const std::vector<Vector2>& points, const Vector2& p, float maxDistance, float heightLimit, float checkStep) {
		std::vector<int> list;
		for (int i = 0; i < points.size(); i++) {
			if (!(points[i] == p)
				&& p.Distance(points[i]) < maxDistance
				&& !HaveRiver(rivers, p, points[i])
				&& IsRiverAllowed(p, points[i], checkStep, heightLimit))
			{
				list.push_back(i);
			}
		}

		if (list.empty())
			return -1;

		return list[state.Range(0, list.size())];
	}

	bool HaveRiver(const std::vector<River>& rivers, const Vector2& p0) {
		for (auto&& river : rivers) {
			if (river.p0 == p0 || river.p1 == p0) {
				return true;
			}
		}
		return false;
	}

	bool HaveRiver(const std::vector<River>& rivers, const Vector2& p0, const Vector2& p1) {
		for (auto&& river : rivers)
		{
			if ((river.p0 == p0 && river.p1 == p1)
				|| (river.p0 == p1 && river.p1 == p0))
			{
				return true;
			}
		}
		return false;
	}

	bool IsRiverAllowed(const Vector2& p0, const Vector2& p1, float step, float heightLimit) {
		float num = p0.Distance(p1);
		Vector2 normalized = (p1 - p0).Normalized();
		bool flag = true;
		for (float num2 = step; num2 <= num - step; num2 += step) {
			Vector2 vector = p0 + normalized * num2;
			float baseHeight = GetBaseHeight(vector.x, vector.y);
			if (baseHeight > heightLimit)
				return false;

			if (baseHeight > 0.05f)
				flag = false;
		}
		return !flag;
	}

	void RenderRivers(VUtils::Random::State& state, const std::vector<River>& rivers) {
		//Dictionary<Vector2i, List<WorldGenerator.RiverPoint>> dictionary;
		robin_hood::unordered_map<Vector2i, std::vector<RiverPoint>, HashUtils::Hasher> dictionary;
		for (auto&& river : rivers) {
			float num = river.widthMin / 8;
			Vector2 normalized = (river.p1 - river.p0).Normalized();
			Vector2 a(-normalized.y, normalized.x);
			float num2 = river.p0.Distance(river.p1);
			for (float num3 = 0; num3 <= num2; num3 += num) {
				float num4 = num3 / river.curveWavelength;
				float d = sin(num4) * sin(num4 * 0.63412f) * sin(num4 * 0.33412f) * river.curveWidth;
				float r = state.Range(river.widthMin, river.widthMax);
				Vector2 p = river.p0 + normalized * num3 + a * d;
				AddRiverPoint(dictionary, p, r);
			}
		}

		for (auto&& keyValuePair : dictionary) {

			//WorldGenerator.RiverPoint[] collection;

			//if (m_riverPoints.TryGetValue(keyValuePair.Key, out collection))
			//{
			//	std::vector<RiverPoint> list(collection);
			//	list.AddRange(keyValuePair.Value);
			//	m_riverPoints[keyValuePair.Key] = list.ToArray();
			//}
			//else
			//{
			//	WorldGenerator.RiverPoint[] value = keyValuePair.Value.ToArray();
			//	m_riverPoints.Add(keyValuePair.Key, value);
			//}


			auto&& find = m_riverPoints.find(keyValuePair.first);

			if (find != m_riverPoints.end()) {
				auto list(find->second);
				list.insert(list.end(), keyValuePair.second.begin(), keyValuePair.second.end());
				find->second = list; // assign to the iterator instead of rehashing the entire thing
			}
			else {
				//WorldGenerator.RiverPoint[] value = keyValuePair.Value.ToArray();
				//auto value = keyValuePair.second;
				m_riverPoints.insert({ keyValuePair.first, keyValuePair.second });
				//m_riverPoints.Add(keyValuePair.Key, value);
			}
		}
	}

	void AddRiverPoint(robin_hood::unordered_map<Vector2i, std::vector<RiverPoint>, HashUtils::Hasher>& riverPoints,
		const Vector2& p,
		float r)
	{
		Vector2i riverGrid = GetRiverGrid(p.x, p.y);
		int num = ceil(r / riverGridSize); // Mathf.CeilToInt(r / 64);
		for (int i = riverGrid.y - num; i <= riverGrid.y + num; i++)
		{
			for (int j = riverGrid.x - num; j <= riverGrid.x + num; j++)
			{
				Vector2i grid(j, i);
				if (InsideRiverGrid(grid, p, r)) {
					AddRiverPoint(riverPoints, grid, p, r);
				}
			}
		}
	}

	void AddRiverPoint(robin_hood::unordered_map<Vector2i, std::vector<RiverPoint>, HashUtils::Hasher>& riverPoints, const Vector2i& grid, const Vector2& p, float r) {
		auto&& find = riverPoints.find(grid);
		if (find != riverPoints.end()) {
			find->second.push_back({ p, r });
		}
		else {
			// add a single dummy new list with 1 pt
			riverPoints.insert({ grid,
				std::vector<RiverPoint> { {p, r} } });
		}
	}

	std::mutex m_mutRiverCache;

	void GetRiverWeight(float wx, float wy, float& weight, float& width) {
		Vector2i riverGrid = GetRiverGrid(wx, wy);

		std::scoped_lock<std::mutex> lock(m_mutRiverCache);
		if (riverGrid == m_cachedRiverGrid) {
			if (m_cachedRiverPoints) {
				return GetWeight(*m_cachedRiverPoints, wx, wy, weight, width);
			}
		}
		else {
			auto&& find = m_riverPoints.find(riverGrid);
			if (find != m_riverPoints.end()) {
				GetWeight(find->second, wx, wy, weight, width);
				m_cachedRiverGrid = riverGrid;
				m_cachedRiverPoints = &find->second;
				return;
			}

			m_cachedRiverGrid = riverGrid;
			m_cachedRiverPoints = nullptr;
		}

		weight = 0;
		width = 0;
	}

	void GetWeight(const std::vector<RiverPoint>& points, float wx, float wy, float& weight, float& width) {
		Vector2 b(wx, wy);
		weight = 0;
		width = 0;
		float num = 0;
		float num2 = 0;
		for (auto&& riverPoint : points)
		{
			float num3 = (riverPoint.p - b).SqMagnitude();
			if (num3 < riverPoint.w2)
			{
				float num4 = sqrt(num3);
				float num5 = 1.f - num4 / riverPoint.w;
				if (num5 > weight)
				{
					weight = num5;
				}
				num += riverPoint.w * num5;
				num2 += num5;
			}
		}
		if (num2 > 0.f)
		{
			width = num / num2;
		}
	}

	//void GenerateBiomes() {
	//	int num = 400000000;
	//	for (int i = 0; i < num; i++)
	//	{
	//		this.m_biomes[i] = Heightmap.Biome.Meadows;
	//	}
	//}



	float WorldAngle(float wx, float wy) {
		return sin(atan2(wx, wy) * 20.f);
	}

	float GetBaseHeight(float wx, float wy) {
		//float num2 = VUtils.Length(wx, wy);
		float num2 = VUtils::Math::Magnitude(wx, wy);
		wx += 100000 + m_offset0;
		wy += 100000 + m_offset1;
		float num3 = 0;
		num3 += VUtils::Math::PerlinNoise(wx * 0.002f * 0.5f, wy * 0.002f * 0.5f) * VUtils::Math::PerlinNoise(wx * 0.003f * 0.5f, wy * 0.003f * 0.5f) * 1.0f;
		num3 += VUtils::Math::PerlinNoise(wx * 0.002f * 1.0f, wy * 0.002f * 1.0f) * VUtils::Math::PerlinNoise(wx * 0.003f * 1.0f, wy * 0.003f * 1.0f) * num3 * 0.9f;
		num3 += VUtils::Math::PerlinNoise(wx * 0.005f * 1.0f, wy * 0.005f * 1.0f) * VUtils::Math::PerlinNoise(wx * 0.010f * 1.0f, wy * 0.010f * 1.0f) * 0.5f * num3;
		num3 -= 0.07f;
		float num4 = VUtils::Math::PerlinNoise(wx * 0.002f * 0.25f + 0.123f, wy * 0.002f * 0.25f + 0.15123f);
		float num5 = VUtils::Math::PerlinNoise(wx * 0.002f * 0.25f + 0.321f, wy * 0.002f * 0.25f + 0.231f);
		float v = abs(num4 - num5);
		float num6 = 1.f - VUtils::Math::LerpStep(0.02f, 0.12f, v);
		num6 *= VUtils::Math::SmoothStep(744, 1000, num2);
		num3 *= 1.f - num6;
		if (num2 > 10000)
		{
			float t = VUtils::Math::LerpStep(10000, 10500, num2);
			num3 = VUtils::Math::Lerp(num3, -0.2f, t);
			float num7 = 10490;
			if (num2 > num7)
			{
				float t2 = VUtils::Math::LerpStep(num7, 10500, num2);
				num3 = VUtils::Math::Lerp(num3, -2, t2);
			}
		}

		if (num2 < m_minMountainDistance && num3 > 0.28f)
		{
			float t3 = VUtils::Math::Clamp01((num3 - 0.28f) / 0.099999994f);

			num3 = VUtils::Math::Lerp(
				VUtils::Math::Lerp(0.28f, 0.38f, t3),
				num3,
				VUtils::Math::LerpStep(m_minMountainDistance - 400.f, m_minMountainDistance, num2)
			);
		}
		return num3;
	}

	// this doesnt actually add rivers to the world
	// it might add two river points/weights together
	float AddRivers(float wx, float wy, float h) {
		float num;
		float v;
		GetRiverWeight(wx, wy, num, v);
		if (num <= 0.f)
		{
			return h;
		}
		float t = VUtils::Math::LerpStep(20, 60, v);
		float num2 = VUtils::Math::Lerp(0.14f, 0.12f, t);
		float num3 = VUtils::Math::Lerp(0.139f, 0.128f, t);
		if (h > num2)
		{
			h = VUtils::Math::Lerp(h, num2, num);
		}
		if (h > num3)
		{
			float t2 = VUtils::Math::LerpStep(0.85f, 1, num);
			h = VUtils::Math::Lerp(h, num3, t2);
		}
		return h;
	}


	float GetMarshHeight(float wx, float wy) {
		float wx2 = wx;
		float wy2 = wy;
		float num = 0.137f;
		wx += 100000.f;
		wy += 100000.f;
		float num2 = VUtils::Math::PerlinNoise(wx * 0.04f, wy * 0.04f) * VUtils::Math::PerlinNoise(wx * 0.08f, wy * 0.08f);
		num += num2 * 0.03f;
		num = AddRivers(wx2, wy2, num);
		num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
		return num + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
	}

	float GetMeadowsHeight(float wx, float wy) {
		float wx2 = wx;
		float wy2 = wy;
		float baseHeight = GetBaseHeight(wx, wy);
		wx += 100000.f + m_offset3;
		wy += 100000.f + m_offset3;
		float num = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f) * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
		num += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f) * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * num * 0.5f;
		float num2 = baseHeight;
		num2 += num * 0.1f;
		float num3 = 0.15f;
		float num4 = num2 - num3;
		float num5 = VUtils::Math::Clamp01(baseHeight / 0.4f);
		if (num4 > 0.f)
		{
			num2 -= num4 * (1.f - num5) * 0.75f;
		}
		num2 = AddRivers(wx2, wy2, num2);
		num2 += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
		return num2 + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
	}

	float GetForestHeight(float wx, float wy) {
		float wx2 = wx;
		float wy2 = wy;
		float num = GetBaseHeight(wx, wy);
		wx += 100000.f + m_offset3;
		wy += 100000.f + m_offset3;
		float num2 = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f) * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
		num2 += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f) * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * num2 * 0.5f;
		num += num2 * 0.1f;
		num = AddRivers(wx2, wy2, num);
		num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
		return num + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
	}

	float GetPlainsHeight(float wx, float wy) {
		float wx2 = wx;
		float wy2 = wy;
		float baseHeight = GetBaseHeight(wx, wy);
		wx += 100000.f + m_offset3;
		wy += 100000.f + m_offset3;
		float num = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f) * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
		num += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f) * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * num * 0.5f;
		float num2 = baseHeight;
		num2 += num * 0.1f;
		float num3 = 0.15f;
		float num4 = num2 - num3;
		float num5 = VUtils::Math::Clamp01(baseHeight / 0.4f);
		if (num4 > 0.f)
		{
			num2 -= num4 * (1.f - num5) * 0.75f;
		}
		num2 = AddRivers(wx2, wy2, num2);
		num2 += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
		return num2 + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
	}


	float GetAshlandsHeight(float wx, float wy) {
		float wx2 = wx;
		float wy2 = wy;
		float num = GetBaseHeight(wx, wy);
		wx += 100000.f + m_offset3;
		wy += 100000.f + m_offset3;
		float num2 = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f) * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
		num2 += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f) * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * num2 * 0.5f;
		num += num2 * 0.1f;
		num += 0.1f;
		num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
		num += VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
		return AddRivers(wx2, wy2, num);
	}

	float GetEdgeHeight(float wx, float wy) {
		float magnitude = VUtils::Math::Magnitude(wx, wy);
		float num = 10490;
		if (magnitude > num)
		{
			float num2 = VUtils::Math::LerpStep(num, 10500, magnitude);
			return -2.f * num2;
		}
		float t = VUtils::Math::LerpStep(10000, 10100, magnitude);
		float num3 = GetBaseHeight(wx, wy);
		num3 = VUtils::Math::Lerp(num3, 0, t);
		return AddRivers(wx, wy, num3);
	}

	float GetOceanHeight(float wx, float wy) {
		return GetBaseHeight(wx, wy);
	}

	float BaseHeightTilt(float wx, float wy) {
		float baseHeight = GetBaseHeight(wx - 1.f, wy);
		float baseHeight2 = GetBaseHeight(wx + 1.f, wy);
		float baseHeight3 = GetBaseHeight(wx, wy - 1.f);
		float baseHeight4 = GetBaseHeight(wx, wy + 1.f);
		return abs(baseHeight2 - baseHeight)
			+ abs(baseHeight3 - baseHeight4);
	}

	float GetSnowMountainHeight(float wx, float wy) {
		float wx2 = wx;
		float wy2 = wy;
		float num = GetBaseHeight(wx, wy);
		float num2 = BaseHeightTilt(wx, wy);
		wx += 100000.f + m_offset3;
		wy += 100000.f + m_offset3;
		float num3 = num - 0.4f;
		num += num3;
		float num4 = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f) * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
		num4 += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f) * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * num4 * 0.5f;
		num += num4 * 0.2f;
		num = AddRivers(wx2, wy2, num);
		num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
		num += VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
		return num + VUtils::Math::PerlinNoise(wx * 0.2f, wy * 0.2f) * 2.f * num2;
	}

	float GetDeepNorthHeight(float wx, float wy) {
		float wx2 = wx;
		float wy2 = wy;
		float num = GetBaseHeight(wx, wy);
		wx += 100000.f + m_offset3;
		wy += 100000.f + m_offset3;
		float num2 = std::max(0.f, num - 0.4f);
		num += num2;
		float num3 = VUtils::Math::PerlinNoise(wx * 0.01f, wy * 0.01f) * VUtils::Math::PerlinNoise(wx * 0.02f, wy * 0.02f);
		num3 += VUtils::Math::PerlinNoise(wx * 0.05f, wy * 0.05f) * VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * num3 * 0.5f;
		num += num3 * 0.2f;
		num *= 1.2f;
		num = AddRivers(wx2, wy2, num);
		num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
		return num + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
	}


	// public accessed methods:



	bool InsideRiverGrid(const Vector2i& grid, const Vector2& p, float r) {
		Vector2 b((float)grid.x * riverGridSize, (float)grid.y * riverGridSize);
		Vector2 vector = p - b;
		return std::abs(vector.x) < r + (riverGridSize * .5f)
			&& std::abs(vector.y) < r + (riverGridSize * .5f);
	}

	Vector2i GetRiverGrid(float wx, float wy) {
		auto x = (int32_t)((wx + riverGridSize * .5f) / riverGridSize);
		auto y = (int32_t)((wy + riverGridSize * .5f) / riverGridSize);
		return Vector2i(x, y);
	}




	// public
	Heightmap::BiomeArea GetBiomeArea(const Vector3& point) {
		auto&& biome = GetBiome(point);

		auto&& biome2 = GetBiome(point - Vector3(-ZoneSystem::ZONE_SIZE, 0, -ZoneSystem::ZONE_SIZE));
		auto&& biome3 = GetBiome(point - Vector3(ZoneSystem::ZONE_SIZE, 0, -ZoneSystem::ZONE_SIZE));
		auto&& biome4 = GetBiome(point - Vector3(ZoneSystem::ZONE_SIZE, 0, ZoneSystem::ZONE_SIZE));
		auto&& biome5 = GetBiome(point - Vector3(-ZoneSystem::ZONE_SIZE, 0, ZoneSystem::ZONE_SIZE));
		auto&& biome6 = GetBiome(point - Vector3(-ZoneSystem::ZONE_SIZE, 0, 0));
		auto&& biome7 = GetBiome(point - Vector3(ZoneSystem::ZONE_SIZE, 0, 0));
		auto&& biome8 = GetBiome(point - Vector3(0, 0, -ZoneSystem::ZONE_SIZE));
		auto&& biome9 = GetBiome(point - Vector3(0, 0, ZoneSystem::ZONE_SIZE));
		if (biome == biome2
			&& biome == biome3
			&& biome == biome4
			&& biome == biome5
			&& biome == biome6
			&& biome == biome7
			&& biome == biome8
			&& biome == biome9)
		{
			return Heightmap::BiomeArea::Median;
		}
		return Heightmap::BiomeArea::Edge;
	}

	// public
	Heightmap::Biome GetBiome(const Vector3& point) {
		return GetBiome(point.x, point.z);
	}

	// public
	Heightmap::Biome GetBiome(float wx, float wy) {
		auto magnitude = VUtils::Math::Magnitude(wx, wy);
		auto baseHeight = GetBaseHeight(wx, wy);
		float num = WorldAngle(wx, wy) * 100.f;

		// bottom curve of world are ashlands
		if (VUtils::Math::Magnitude(wx, wy + ashlandsYOffset) > ashlandsMinDistance + num)
			return Heightmap::Biome::AshLands;

		if ((double)baseHeight <= 0.02)
			return Heightmap::Biome::Ocean;

		// top curve of world is mostly deep north
		if (VUtils::Math::Magnitude(wx, wy + deepNorthYOffset) > deepNorthMinDistance + num) {
			if (baseHeight > mountainBaseHeightMin)
				return Heightmap::Biome::Mountain;
			return Heightmap::Biome::DeepNorth;
		}

		if (baseHeight > mountainBaseHeightMin)
			return Heightmap::Biome::Mountain;

		if (VUtils::Math::PerlinNoise((m_offset0 + wx) * marshBiomeScale, (m_offset0 + wy) * marshBiomeScale) > minMarshNoise
			&& magnitude > minMarshDistance && magnitude < maxMarshDistance && baseHeight > minMarshHeight && baseHeight < maxMarshHeight)
			return Heightmap::Biome::Swamp;

		if (VUtils::Math::PerlinNoise((m_offset4 + wx) * darklandBiomeScale, (m_offset4 + wy) * darklandBiomeScale) > minDarklandNoise
			&& magnitude > minDarklandDistance + num && magnitude < maxDarklandDistance)
			return Heightmap::Biome::Mistlands;

		if (VUtils::Math::PerlinNoise((m_offset1 + wx) * heathBiomeScale, (m_offset1 + wy) * heathBiomeScale) > minHeathNoise
			&& magnitude > minHeathDistance + num && magnitude < maxHeathDistance)
			return Heightmap::Biome::Plains;

		if (VUtils::Math::PerlinNoise((m_offset2 + wx) * 0.001f, (m_offset2 + wy) * 0.001f) > minDeepForestNoise
			&& magnitude > minDeepForestDistance + num && magnitude < maxDeepForestDistance)
			return Heightmap::Biome::BlackForest;

		if (magnitude > meadowsMaxDistance + num)
			return Heightmap::Biome::BlackForest;

		return Heightmap::Biome::Meadows;
	}


	// 
	float GetHeight(float wx, float wy) {
		auto biome = GetBiome(wx, wy);
		return GetBiomeHeight(biome, wx, wy);
	}

	// public
	float GetBiomeHeight(Heightmap::Biome biome, float wx, float wy) {
		switch (biome)
		{
		case Heightmap::Biome::Meadows:
			return GetMeadowsHeight(wx, wy) * 200.f;
		case Heightmap::Biome::Swamp:
			return GetMarshHeight(wx, wy) * 200.f;
		case Heightmap::Biome::Meadows | Heightmap::Biome::Swamp: //Heightmap::Biome::Swamp | Heightmap::Biome::Mountain: // (Heightmap::Biome)3:
			break; // why is this here?
		case Heightmap::Biome::Mountain:
			return GetSnowMountainHeight(wx, wy) * 200.f;
		case Heightmap::Biome::BlackForest:
			return GetForestHeight(wx, wy) * 200.f;
		case Heightmap::Biome::Plains:
			return GetPlainsHeight(wx, wy) * 200.f;
		case Heightmap::Biome::AshLands:
			return GetAshlandsHeight(wx, wy) * 200.f;
		case Heightmap::Biome::DeepNorth:
			return GetDeepNorthHeight(wx, wy) * 200.f;
		case Heightmap::Biome::Ocean:
			return GetOceanHeight(wx, wy) * 200.f;
		case Heightmap::Biome::Mistlands: // valheim doesnt correctly implement mistlands yet
			return GetForestHeight(wx, wy) * 200.f;
		default:
			LOG(ERROR) << "Tried to retrieve biome height for unknown biome " << (int)biome;
			//return 0.f;
			break;
		}
		return 0.f;
	}

	// public
	bool InForest(const Vector3& pos) {
		return GetForestFactor(pos) < 1.15f;
	}

	// public
	float GetForestFactor(const Vector3& pos) {
		float d = 0.4f;
		return VUtils::Math::Fbm(pos * 0.01f * d, 3, 1.6f, 0.7f);
	}

	// public
	void GetTerrainDelta(VUtils::Random::State& state, const Vector3& center, float radius, float& delta, Vector3& slopeDirection) {
		int num = 10;
		float num2 = std::numeric_limits<float>::min();
		float num3 = std::numeric_limits<float>::max();
		Vector3 b = center;
		Vector3 a = center;
		for (int i = 0; i < num; i++)
		{
			Vector2 vector = state.GetRandomUnitCircle() * radius;
			Vector3 vector2 = center + Vector3(vector.x, 0, vector.y);
			float height = GetHeight(vector2.x, vector2.z);
			if (height < num3)
			{
				num3 = height;
				a = vector2;
			}
			if (height > num2)
			{
				num2 = height;
				b = vector2;
			}
		}
		delta = num2 - num3;
		slopeDirection = (a - b).Normalize();
	}

	// public
	int GetSeed() {
		return m_world.m_seed;
	}
}
