#include "GeoManager.h"
#include "VUtilsRandom.h"
#include "HashUtils.h"
#include "ZoneManager.h"
#include "VUtilsMathf.h"

auto GEO_MANAGER(std::make_unique<IGeoManager>());
IGeoManager* GeoManager() {
	return GEO_MANAGER.get();
}



void IGeoManager::Init() {
	LOG(INFO) << "Initializing GeoManager";

	m_world = WorldManager()->GetWorld();
	assert(m_world);

	if (m_world->m_worldGenVersion <= 0)
		m_minMountainDistance = 1500;

	if (m_world->m_worldGenVersion <= 1) {
		minDarklandNoise = 0.5f;
		maxMarshDistance = 8000;
	}

	VUtils::Random::State state(m_world->m_seed);
	m_offset0 = state.Range(-worldSize, worldSize);
	m_offset1 = state.Range(-worldSize, worldSize);
	m_offset2 = state.Range(-worldSize, worldSize);
	m_offset3 = state.Range(-worldSize, worldSize);
	m_riverSeed = state.Range(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
	m_streamSeed = state.Range(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
	m_offset4 = state.Range(-worldSize, worldSize);

	// TODO rename run-once generator functions from 'Find...' to 'Generate...' for clarity

	Generate();
}

void IGeoManager::Generate() {
	GenerateLakes();
	GenerateRivers();
	GenerateStreams();
}

void IGeoManager::GenerateLakes() {
	std::vector<Vector2> list;
	for (float num = -worldSize; num <= worldSize; num += 128)
	{
		for (float num2 = -worldSize; num2 <= worldSize; num2 += 128)
		{
			if (VUtils::Math::Magnitude(num2, num) <= worldSize
				&& GetBaseHeight(num2, num) < 0.05f)
			{
				list.push_back(Vector2(num2, num));
			}
		}
	}
	m_lakes = MergePoints(list, 800);
}

// Basically blender merge nearby vertices
std::vector<Vector2> IGeoManager::MergePoints(std::vector<Vector2>& points, float range) {
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
int IGeoManager::FindClosest(const std::vector<Vector2>& points, const Vector2& p, float maxDistance) {
	int result = -1;
	float num = std::numeric_limits<float>::max();
	for (int i = 0; i < points.size(); i++)
	{
		if (!(points[i] == p))
		{
			//float num2 = p.Distance(points[i]); // not optimal
			float num2 = p.SqDistance(points[i]);
			if (num2 < maxDistance * maxDistance
				&& num2 < num)
			{
				result = i;
				num = num2;
			}
		}
	}
	return result;
}

void IGeoManager::GenerateStreams() {
	VUtils::Random::State state(m_streamSeed);
	int num = 0;
	for (int i = 0; i < streams; i++) {
		Vector2 vector;
		float num2;
		Vector2 vector2; // out
		if (FindStreamStartPoint(state, 100, 26, 31, vector, num2)
			&& FindStreamEndPoint(state, 100, 36, 44, vector, 80, 200, vector2)) {
			Vector2 vector3 = (vector + vector2) * 0.5f;
			float height = GetGenerationHeight(vector3.x, vector3.y);
			if (height >= 26 && height <= 44) {
				River river;
				river.p0 = vector;
				river.p1 = vector2;
				river.center = vector3;
				river.widthMax = 20;
				river.widthMin = 20;
				float num3 = river.p0.Distance(river.p1); // use sqdist?
				river.curveWidth = num3 / 15;
				river.curveWavelength = num3 / 20;
				m_streams.push_back(river); // use move / emplacer
				num++;
			}
		}
	}
	RenderRivers(state, m_streams);
}

bool IGeoManager::FindStreamEndPoint(VUtils::Random::State& state, int iterations, float minHeight, float maxHeight, const Vector2& start, float minLength, float maxLength, Vector2& end) {
	float num = (maxLength - minLength) / (float)iterations;
	float num2 = maxLength;
	for (int i = 0; i < iterations; i++) {
		num2 -= num;
		float f = state.Range(0.f, PI * 2.0f);
		Vector2 vector = start + Vector2(sin(f), cos(f)) * num2;
		float height = GetGenerationHeight(vector.x, vector.y);
		if (height > minHeight && height < maxHeight)
		{
			end = vector;
			return true;
		}
	}
	end = Vector2::ZERO;
	return false;
}

bool IGeoManager::FindStreamStartPoint(VUtils::Random::State& state, int iterations, float minHeight, float maxHeight, Vector2& p, float& starth) {
	for (int i = 0; i < iterations; i++) {
		auto num = state.Range((float)-worldSize, (float)worldSize);
		auto num2 = state.Range((float)-worldSize, (float)worldSize);
		auto height = GetGenerationHeight(num, num2);
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

void IGeoManager::GenerateRivers() {
	VUtils::Random::State state(m_riverSeed);

	//std::vector<River> list;
	std::vector<Vector2> list2(m_lakes); // TODO use list

	while (list2.size() > 1)
	{
		auto&& vector = list2[0];
		int num = FindRandomRiverEnd(state, m_rivers, m_lakes, vector, 2000, 0.4f, 128);
		if (num == -1 && !HaveRiver(m_rivers, vector)) {
			num = FindRandomRiverEnd(state, m_rivers, m_lakes, vector, 5000, 0.4f, 128);
		}

		if (num != -1) {
			River river;
			river.p0 = vector;
			river.p1 = m_lakes[num];
			river.center = (river.p0 + river.p1) * 0.5f;
			river.widthMax = state.Range(minRiverWidth, maxRiverWidth);
			river.widthMin = state.Range(minRiverWidth, river.widthMax);
			float num2 = river.p0.Distance(river.p1);
			river.curveWidth = num2 / 15.f;
			river.curveWavelength = num2 / 20.f;
			m_rivers.push_back(river);
		}
		else
		{
			list2.erase(list2.begin());
		}
	}
	RenderRivers(state, m_rivers);
}

int IGeoManager::FindRandomRiverEnd(VUtils::Random::State& state, const std::vector<River>& rivers, const std::vector<Vector2> &points, 
	const Vector2& p, float maxDistance, float heightLimit, float checkStep) const {

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

bool IGeoManager::HaveRiver(const std::vector<River>& rivers, const Vector2& p0) const {
	for (auto&& river : rivers) {
		if (river.p0 == p0 || river.p1 == p0) {
			return true;
		}
	}
	return false;
}

bool IGeoManager::HaveRiver(const std::vector<River>& rivers, const Vector2& p0, const Vector2& p1) const {
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

bool IGeoManager::IsRiverAllowed(const Vector2& p0, const Vector2& p1, float step, float heightLimit) const {
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

void IGeoManager::RenderRivers(VUtils::Random::State& state, const std::vector<River>& rivers) {
	//Dictionary<Vector2i, List<WorldGenerator.RiverPoint>> dictionary;
	robin_hood::unordered_map<Vector2i, std::vector<RiverPoint>> dictionary;
	for (auto&& river : rivers) {

		float num = river.widthMin / 8.f;
		const Vector2 normalized = (river.p1 - river.p0).Normalized();
		const Vector2 a(-normalized.y, normalized.x);
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
		auto&& list = m_riverPoints[keyValuePair.first];

		list.insert(list.end(), keyValuePair.second.begin(), keyValuePair.second.end());
	}
}

void IGeoManager::AddRiverPoint(robin_hood::unordered_map<Vector2i, std::vector<RiverPoint>>& riverPoints,
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
			if (InsideRiverGrid(grid, p, r))
				AddRiverPoint(riverPoints, grid, p, r);
		}
	}
}

void IGeoManager::AddRiverPoint(robin_hood::unordered_map<Vector2i, std::vector<RiverPoint>>& riverPoints, const Vector2i& grid, const Vector2& p, float r) {
	riverPoints[grid].push_back({ p, r });
}

std::mutex m_mutRiverCache;
void IGeoManager::GetRiverWeight(float wx, float wy, float& outWeight, float& outWidth) {
	Vector2i riverGrid = GetRiverGrid(wx, wy);

	std::scoped_lock<std::mutex> lock(m_mutRiverCache);
	if (riverGrid == m_cachedRiverGrid) {
		if (m_cachedRiverPoints) {
			return GetWeight(*m_cachedRiverPoints, wx, wy, outWeight, outWidth);
		}
	}
	else {
		auto&& find = m_riverPoints.find(riverGrid);
		if (find != m_riverPoints.end()) {
			GetWeight(find->second, wx, wy, outWeight, outWidth);
			m_cachedRiverGrid = riverGrid;
			m_cachedRiverPoints = &find->second;
			return;
		}

		m_cachedRiverGrid = riverGrid;
		m_cachedRiverPoints = nullptr;
	}

	outWeight = 0;
	outWidth = 0;
}

void IGeoManager::GetWeight(const std::vector<RiverPoint>& points, float wx, float wy, float& outWeight, float& outWidth) {

	outWeight = 0;
	outWidth = 0;

	Vector2 b(wx, wy);
	float num = 0;
	float num2 = 0;

	for (auto&& riverPoint : points)
	{
		float num3 = (riverPoint.p - b).SqMagnitude();
		if (num3 < riverPoint.w2)
		{
			float num4 = sqrt(num3);
			float num5 = 1.f - num4 / riverPoint.w;
			outWeight = std::max(num5, outWeight);

			num += riverPoint.w * num5;
			num2 += num5;
		}
	}

	if (num2 > 0.f)
		outWidth = num / num2;
}



float IGeoManager::WorldAngle(float wx, float wy) {
	return sin(atan2(wx, wy) * 20.f);
}

float IGeoManager::GetBaseHeight(float wx, float wy) const {
	//float num2 = VUtils.Length(wx, wy);
	float num2 = VUtils::Math::Magnitude(wx, wy);
	wx += 100000 + m_offset0;
	wy += 100000 + m_offset1;
	float num3 = 0;
	num3 += VUtils::Math::PerlinNoise(wx * 0.002f * 0.5f, wy * 0.002f * 0.5f)
		* VUtils::Math::PerlinNoise(wx * 0.003f * 0.5f, wy * 0.003f * 0.5f) * 1.0f;
	num3 += VUtils::Math::PerlinNoise(wx * 0.002f * 1.0f, wy * 0.002f * 1.0f)
		* VUtils::Math::PerlinNoise(wx * 0.003f * 1.0f, wy * 0.003f * 1.0f) * num3 * 0.9f;
	num3 += VUtils::Math::PerlinNoise(wx * 0.005f * 1.0f, wy * 0.005f * 1.0f)
		* VUtils::Math::PerlinNoise(wx * 0.010f * 1.0f, wy * 0.010f * 1.0f) * 0.5f * num3;
	num3 -= 0.07f;
	float num4 = VUtils::Math::PerlinNoise(wx * 0.002f * 0.25f + 0.123f, wy * 0.002f * 0.25f + 0.15123f);
	float num5 = VUtils::Math::PerlinNoise(wx * 0.002f * 0.25f + 0.321f, wy * 0.002f * 0.25f + 0.231f);
	float v = std::abs(num4 - num5);
	float num6 = 1.f - VUtils::Math::LerpStep(0.02f, 0.12f, v);
	num6 *= VUtils::Math::SmoothStep(744, 1000, num2);
	num3 *= 1.f - num6;
	if (num2 > 10000)
	{
		float t = VUtils::Math::LerpStep(10000, waterEdge, num2);
		num3 = VUtils::Mathf::Lerp(num3, -0.2f, t);
		float num7 = 10490;
		if (num2 > num7)
		{
			float t2 = VUtils::Math::LerpStep(num7, waterEdge, num2);
			num3 = VUtils::Mathf::Lerp(num3, -2, t2);
		}
	}

	if (num2 < m_minMountainDistance && num3 > 0.28f)
	{
		float t3 = VUtils::Mathf::Clamp01((num3 - 0.28f) / 0.099999994f);

		num3 = VUtils::Mathf::Lerp(
			VUtils::Mathf::Lerp(0.28f, 0.38f, t3),
			num3,
			VUtils::Math::LerpStep(m_minMountainDistance - 400.f, m_minMountainDistance, num2)
		);
	}
	return num3;
}

// this doesnt actually add rivers to the world
// it might add two river points/weights together
float IGeoManager::AddRivers(float wx, float wy, float h) {
	float num;
	float v;
	GetRiverWeight(wx, wy, num, v);
	if (num <= 0)
		return h;

	float t = VUtils::Math::LerpStep(20, 60, v);
	float num2 = VUtils::Mathf::Lerp(0.14f, 0.12f, t);
	float num3 = VUtils::Mathf::Lerp(0.139f, 0.128f, t);
	if (h > num2)
	{
		h = VUtils::Mathf::Lerp(h, num2, num);
	}
	if (h > num3)
	{
		float t2 = VUtils::Math::LerpStep(0.85f, 1, num);
		h = VUtils::Mathf::Lerp(h, num3, t2);
	}
	return h;
}


float IGeoManager::GetMarshHeight(float wx, float wy) {
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

float IGeoManager::GetMeadowsHeight(float wx, float wy) {
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
	float num5 = VUtils::Mathf::Clamp01(baseHeight / 0.4f);
	if (num4 > 0.f)
		num2 -= num4 * (1.f - num5) * 0.75f;

	num2 = AddRivers(wx2, wy2, num2);
	num2 += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
	return num2 + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
}

float IGeoManager::GetForestHeight(float wx, float wy) {
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

float IGeoManager::GetMistlandsHeight(float wx, float wy, float& mask) {
	float wx2 = wx;
	float wy2 = wy;
	float num = GetBaseHeight(wx, wy);
	wx += 100000.f + m_offset3;
	wy += 100000.f + m_offset3;
	float num2 = VUtils::Math::PerlinNoise(wx * 0.02f * 0.7f, wy * 0.02f * 0.7f)
		* VUtils::Math::PerlinNoise(wx * 0.04f * 0.7f, wy * 0.04f * 0.7f);
	num2 += VUtils::Math::PerlinNoise(wx * 0.03f * 0.7f, wy * 0.03f * 0.7f)
		* VUtils::Math::PerlinNoise(wx * 0.05f * 0.7f, wy * 0.05f * 0.7f) * num2 * 0.5f;
	num2 = (num2 > 0) ? std::pow(num2, 1.5f) : num2;
	num += num2 * 0.4f;
	num = AddRivers(wx2, wy2, num);
	float num3 = VUtils::Mathf::Clamp01(num2 * 7.f);
	num += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.03f * num3;
	num += VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.01f * num3;
	float num4 = 1.f - num3 * 1.2f;
	num4 -= 1.f - VUtils::Math::LerpStep(0.1f, 0.3f, num3);
	float a = num + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.002f;
	float num5 = num;
	num5 *= 400.f;
	num5 = std::ceil(num5);
	num5 /= 400.f;
	//num = VUtils::Mathf::Lerp(a, num5, num3);
	num = std::lerp(a, num5, num3);
	//mask = Color{ 0, 0, 0, num4 };
	mask = num4;
	return num;
}

float IGeoManager::GetPlainsHeight(float wx, float wy) {
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
	float num5 = VUtils::Mathf::Clamp01(baseHeight / 0.4f);
	if (num4 > 0.f)
		num2 -= num4 * (1.f - num5) * 0.75f;

	num2 = AddRivers(wx2, wy2, num2);
	num2 += VUtils::Math::PerlinNoise(wx * 0.1f, wy * 0.1f) * 0.01f;
	return num2 + VUtils::Math::PerlinNoise(wx * 0.4f, wy * 0.4f) * 0.003f;
}

float IGeoManager::GetAshlandsHeight(float wx, float wy) {
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

float IGeoManager::GetEdgeHeight(float wx, float wy) {
	float magnitude = VUtils::Math::Magnitude(wx, wy);
	float num = 10490;
	if (magnitude > num)
	{
		float num2 = VUtils::Math::LerpStep(num, 10500, magnitude);
		return -2.f * num2;
	}
	float t = VUtils::Math::LerpStep(10000, 10100, magnitude);
	float num3 = GetBaseHeight(wx, wy);
	num3 = VUtils::Mathf::Lerp(num3, 0, t);
	return AddRivers(wx, wy, num3);
}

float IGeoManager::GetOceanHeight(float wx, float wy) {
	return GetBaseHeight(wx, wy);
}

float IGeoManager::BaseHeightTilt(float wx, float wy) {
	float baseHeight = GetBaseHeight(wx - 1.f, wy);
	float baseHeight2 = GetBaseHeight(wx + 1.f, wy);
	float baseHeight3 = GetBaseHeight(wx, wy - 1.f);
	float baseHeight4 = GetBaseHeight(wx, wy + 1.f);
	return abs(baseHeight2 - baseHeight)
		+ abs(baseHeight3 - baseHeight4);
}

float IGeoManager::GetSnowMountainHeight(float wx, float wy) {
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

float IGeoManager::GetDeepNorthHeight(float wx, float wy) {
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



//
// public accessed methods:
//



bool IGeoManager::InsideRiverGrid(const Vector2i& grid, const Vector2& p, float r) {
	Vector2 b((float)grid.x * riverGridSize, (float)grid.y * riverGridSize);
	Vector2 vector = p - b;
	return std::abs(vector.x) < r + (riverGridSize * .5f)
		&& std::abs(vector.y) < r + (riverGridSize * .5f);
}

Vector2i IGeoManager::GetRiverGrid(float wx, float wy) {
	auto x = (int32_t)std::floorf((wx + riverGridSize * .5f) / riverGridSize);
	auto y = (int32_t)std::floorf((wy + riverGridSize * .5f) / riverGridSize);
	return Vector2i(x, y);
}

BiomeArea IGeoManager::GetBiomeArea(const Vector3& point) {
	auto&& biome = GetBiome(point);

	auto&& biome2 = GetBiome(point - Vector3(-IZoneManager::ZONE_SIZE, 0, -IZoneManager::ZONE_SIZE));
	auto&& biome3 = GetBiome(point - Vector3(IZoneManager::ZONE_SIZE, 0, -IZoneManager::ZONE_SIZE));
	auto&& biome4 = GetBiome(point - Vector3(IZoneManager::ZONE_SIZE, 0, IZoneManager::ZONE_SIZE));
	auto&& biome5 = GetBiome(point - Vector3(-IZoneManager::ZONE_SIZE, 0, IZoneManager::ZONE_SIZE));
	auto&& biome6 = GetBiome(point - Vector3(-IZoneManager::ZONE_SIZE, 0, 0));
	auto&& biome7 = GetBiome(point - Vector3(IZoneManager::ZONE_SIZE, 0, 0));
	auto&& biome8 = GetBiome(point - Vector3(0, 0, -IZoneManager::ZONE_SIZE));
	auto&& biome9 = GetBiome(point - Vector3(0, 0, IZoneManager::ZONE_SIZE));
	if (biome == biome2
		&& biome == biome3
		&& biome == biome4
		&& biome == biome5
		&& biome == biome6
		&& biome == biome7
		&& biome == biome8
		&& biome == biome9)
	{
		return BiomeArea::Median;
	}
	return BiomeArea::Edge;
}

// public
Biome IGeoManager::GetBiome(const Vector3& point) {
	return GetBiome(point.x, point.z);
}

// public
Biome IGeoManager::GetBiome(float wx, float wy) {
	auto magnitude = VUtils::Math::Magnitude(wx, wy);
	auto baseHeight = GetBaseHeight(wx, wy);
	float num = WorldAngle(wx, wy) * 100.f;

	// bottom curve of world are ashlands
	if (VUtils::Math::Magnitude(wx, wy + ashlandsYOffset) > ashlandsMinDistance + num)
		return Biome::AshLands;

	if (baseHeight <= 0.02f)
		return Biome::Ocean;

	// top curve of world is deep north
	if (VUtils::Math::Magnitude(wx, wy + deepNorthYOffset) > deepNorthMinDistance + num) {
		if (baseHeight > mountainBaseHeightMin)
			return Biome::Mountain;
		return Biome::DeepNorth;
	}

	if (baseHeight > mountainBaseHeightMin)
		return Biome::Mountain;

	if (VUtils::Math::PerlinNoise((m_offset0 + wx) * marshBiomeScale, (m_offset0 + wy) * marshBiomeScale) > minMarshNoise
		&& magnitude > minMarshDistance && magnitude < maxMarshDistance && baseHeight > minMarshHeight && baseHeight < maxMarshHeight)
		return Biome::Swamp;

	if (VUtils::Math::PerlinNoise((m_offset4 + wx) * darklandBiomeScale, (m_offset4 + wy) * darklandBiomeScale) > minDarklandNoise
		&& magnitude > minDarklandDistance + num && magnitude < maxDarklandDistance)
		return Biome::Mistlands;

	if (VUtils::Math::PerlinNoise((m_offset1 + wx) * heathBiomeScale, (m_offset1 + wy) * heathBiomeScale) > minHeathNoise
		&& magnitude > minHeathDistance + num && magnitude < maxHeathDistance)
		return Biome::Plains;

	if (VUtils::Math::PerlinNoise((m_offset2 + wx) * 0.001f, (m_offset2 + wy) * 0.001f) > minDeepForestNoise
		&& magnitude > minDeepForestDistance + num && magnitude < maxDeepForestDistance)
		return Biome::BlackForest;

	if (magnitude > meadowsMaxDistance + num)
		return Biome::BlackForest;

	return Biome::Meadows;
}

Biome IGeoManager::GetBiomes(float x, float z) {
	//ZoneID zone = IZoneManager::WorldToZonePos(Vector3(x, 0., z));
	//Vector3 center = IZoneManager::ZoneToWorldPos(zone) + ;
	return Biome(std::to_underlying(GetBiome(x - IZoneManager::ZONE_SIZE / 2, z - IZoneManager::ZONE_SIZE / 2))
		|| std::to_underlying(GetBiome(x - IZoneManager::ZONE_SIZE / 2, z + IZoneManager::ZONE_SIZE / 2))
		|| std::to_underlying(GetBiome(x + IZoneManager::ZONE_SIZE / 2, z - IZoneManager::ZONE_SIZE / 2))
		|| std::to_underlying(GetBiome(x + IZoneManager::ZONE_SIZE / 2, z + IZoneManager::ZONE_SIZE / 2))
		|| std::to_underlying(GetBiome(x, z)));
}

float IGeoManager::GetHeight(float wx, float wy) {
	float dummy;
	return GetHeight(wx, wy, dummy);
}

float IGeoManager::GetHeight(float wx, float wy, float& mask) {
	auto biome = GetBiome(wx, wy);
	return GetBiomeHeight(biome, wx, wy, mask);
}

// Used only early during generation
float IGeoManager::GetGenerationHeight(float wx, float wy) {
	auto biome = GetBiome(wx, wy);
	if (biome == Biome::Mistlands)
		return GetForestHeight(wx, wy) * 200.f;
	float dummy;
	return GetBiomeHeight(biome, wx, wy, dummy);
}

// public
float IGeoManager::GetBiomeHeight(Biome biome, float wx, float wy, float& mask) {
	switch (biome)
	{
	case Biome::Meadows:
		return GetMeadowsHeight(wx, wy) * 200.f;
	case Biome::Swamp:
		return GetMarshHeight(wx, wy) * 200.f;
	case Biome::Mountain:
		return GetSnowMountainHeight(wx, wy) * 200.f;
	case Biome::BlackForest:
		return GetForestHeight(wx, wy) * 200.f;
	case Biome::Plains:
		return GetPlainsHeight(wx, wy) * 200.f;
	case Biome::AshLands:
		return GetAshlandsHeight(wx, wy) * 200.f;
	case Biome::DeepNorth:
		return GetDeepNorthHeight(wx, wy) * 200.f;
	case Biome::Ocean:
		return GetOceanHeight(wx, wy) * 200.f;
	case Biome::Mistlands:
		return GetMistlandsHeight(wx, wy, mask) * 200.f;
	}
	return 0;
}

// public
bool IGeoManager::InForest(const Vector3& pos) {
	return GetForestFactor(pos) < 1.15f;
}

// public
float IGeoManager::GetForestFactor(const Vector3& pos) {
	float d = 0.4f;
	return VUtils::Math::Fbm(pos * 0.01f * d, 3, 1.6f, 0.7f);
}

// public
void IGeoManager::GetTerrainDelta(VUtils::Random::State& state, const Vector3& center, float radius, float& delta, Vector3& slopeDirection) {
	int num = 10;
	float num2 = std::numeric_limits<float>::min();
	float num3 = std::numeric_limits<float>::max();
	Vector3 b = center;
	Vector3 a = center;
	for (int i = 0; i < num; i++)
	{
		Vector2 vector = state.InsideUnitCircle() * radius;
		Vector3 vector2 = center + Vector3(vector.x, 0.f, vector.y);
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
int IGeoManager::GetSeed() {
	return m_world->m_seed;
}
