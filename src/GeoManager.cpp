#include "GeoManager.h"

#if VH_IS_ON(VH_ZONE_GENERATION)
#include "VUtilsRandom.h"
#include "HashUtils.h"
#include "ZoneManager.h"
#include "VUtilsMathf.h"
#include "VUtilsMath.h"
#include "VUtilsMath2.h"

auto GEO_MANAGER(std::make_unique<IGeoManager>());
IGeoManager* GeoManager() {
	return GEO_MANAGER.get();
}



void IGeoManager::PostWorldInit() {
	LOG_INFO(LOGGER, "Initializing GeoManager");

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
	std::vector<Vector2f> list;
	for (float num = -worldSize; num <= worldSize; num = (double)num + 128.0)
	{
		for (float num2 = -worldSize; num2 <= worldSize; num2 = (double)num2 + 128.0)
		{
			if (VUtils::Math::Magnitude(num2, num) <= worldSize
				&& GetBaseHeight(num2, num) < 0.05f)
			{
				list.push_back(Vector2f(num2, num));
			}
		}
	}
	m_lakes = MergePoints(list, 800);
}

// Basically blender merge nearby vertices
std::vector<Vector2f> IGeoManager::MergePoints(std::vector<Vector2f>& points, float range) {
	std::vector<Vector2f> list;
	while (!points.empty()) {
		Vector2f vector = points[0];
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
int IGeoManager::FindClosest(const std::vector<Vector2f>& points, Vector2f p, float maxDistance) {
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
		Vector2f vector;
		float num2;
		Vector2f vector2; // out
		if (FindStreamStartPoint(state, 100, 26, 31, vector, num2)
			&& FindStreamEndPoint(state, 100, 36, 44, vector, 80, 200, vector2)) {
			Vector2f vector3 = (vector + vector2) * 0.5f;
			float height = GetGenerationHeight(vector3.x, vector3.y);
			if (height >= 26 && height <= 44) {
				River river;
				river.p0 = vector;
				river.p1 = vector2;
				river.center = vector3;
				river.widthMax = 20;
				river.widthMin = 20;
				
				float num3 = river.p0.Distance(river.p1); // TODO use sqdist?
				river.curveWidth = (double)num3 / 15.0;
				river.curveWavelength = (double)num3 / 20.0;
				m_streams.push_back(river); // TODO use move / emplacer
				num++;
			}
		}
	}
	RenderRivers(state, m_streams);
}

bool IGeoManager::FindStreamEndPoint(VUtils::Random::State& state, int iterations, float minHeight, float maxHeight, Vector2f start, float minLength, float maxLength, Vector2f& end) {
	float num = ((double)maxLength - (double)minLength) / (double)iterations;
	float num2 = maxLength;
	for (int i = 0; i < iterations; i++) {
		num2 = (double)num2 - (double)num;
		float f = state.Range(0.f, PI * 2.0f);
		Vector2f vector = start + Vector2f(std::sin(f), std::cos(f)) * num2;
		float height = GetGenerationHeight(vector.x, vector.y);
		if (height > minHeight && height < maxHeight)
		{
			end = vector;
			return true;
		}
	}
	end = Vector2f::Zero();
	return false;
}

bool IGeoManager::FindStreamStartPoint(VUtils::Random::State& state, int iterations, float minHeight, float maxHeight, Vector2f& p, float& starth) {
	for (int i = 0; i < iterations; i++) {
		auto num = state.Range((float)-worldSize, (float)worldSize);
		auto num2 = state.Range((float)-worldSize, (float)worldSize);
		auto height = GetGenerationHeight(num, num2);
		if (height > minHeight && height < maxHeight)
		{
			p = Vector2f(num, num2);
			starth = height;
			return true;
		}
	}
	p = Vector2f::Zero();
	starth = 0;
	return false;
}

void IGeoManager::GenerateRivers() {
	VUtils::Random::State state(m_riverSeed);

	//std::vector<River> list;
	std::vector<Vector2f> list2(m_lakes); // TODO use list

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
			river.curveWidth = (double)num2 / 15.0;
			river.curveWavelength = (double)num2 / 20.0;
			m_rivers.push_back(river);
		}
		else
		{
			list2.erase(list2.begin());
		}
	}
	RenderRivers(state, m_rivers);
}

int IGeoManager::FindRandomRiverEnd(VUtils::Random::State& state, const std::vector<River>& rivers, const std::vector<Vector2f> &points, 
	Vector2f p, float maxDistance, float heightLimit, float checkStep) const {

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

bool IGeoManager::HaveRiver(const std::vector<River>& rivers, Vector2f p0) const {
	for (auto&& river : rivers) {
		if (river.p0 == p0 || river.p1 == p0) {
			return true;
		}
	}
	return false;
}

bool IGeoManager::HaveRiver(const std::vector<River>& rivers, Vector2f p0, Vector2f p1) const {
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

bool IGeoManager::IsRiverAllowed(Vector2f p0, Vector2f p1, float step, float heightLimit) const {
	float num = p0.Distance(p1);
	Vector2f normalized = (p1 - p0).Normal();
	bool flag = true;
	for (float num2 = step; num2 <= (double)num - (double)step; num2 = (double)num2 + (double)step) {
		Vector2f vector = p0 + normalized * num2;
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
	UNORDERED_MAP_t<Vector2i, std::vector<RiverPoint>> dictionary;
	for (auto&& river : rivers) {

		float num = (double)river.widthMin / 8.0;
		const Vector2f normalized = (river.p1 - river.p0).Normal();
		const Vector2f a(-normalized.y, normalized.x);
		float num2 = river.p0.Distance(river.p1);

		for (float num3 = 0; num3 <= num2; num3 = (double)num3 + (double)num) {
			float num4 = (double)num3 / (double)river.curveWavelength;
			float d = std::sin((double)num4) * std::sin((double)num4 * 0.634119987487793) * std::sin((double)num4 * 0.3341200053691864) * (double)river.curveWidth;
			float r = state.Range(river.widthMin, river.widthMax);
			Vector2f p = river.p0 + normalized * num3 + a * d;
			AddRiverPoint(dictionary, p, r);
		}
	}

	for (auto&& keyValuePair : dictionary) {
		auto&& list = m_riverPoints[keyValuePair.first];

		list.insert(list.end(), keyValuePair.second.begin(), keyValuePair.second.end());
	}
}

void IGeoManager::AddRiverPoint(UNORDERED_MAP_t<Vector2i, std::vector<RiverPoint>>& riverPoints,
	Vector2f p,
	float r)
{
	Vector2i riverGrid = GetRiverGrid(p.x, p.y);
	int num = std::ceil((double)r / (double)riverGridSize); // Mathf.CeilToInt(r / 64);
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

void IGeoManager::AddRiverPoint(UNORDERED_MAP_t<Vector2i, std::vector<RiverPoint>>& riverPoints, Vector2i grid, Vector2f p, float r) {
	riverPoints[grid].push_back({ p, r });
}

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

	Vector2f b(wx, wy);
	float num = 0;
	float num2 = 0;

	for (auto&& riverPoint : points)
	{
		float num3 = (riverPoint.p - b).SqMagnitude();
		if (num3 < riverPoint.w2)
		{
			float num4 = std::sqrt((double)num3);
			float num5 = 1.0 - (double)num4 / (double)riverPoint.w;
			outWeight = std::max(num5, outWeight);

			num = (double)num + (double)riverPoint.w * (double)num5;
			num2 = (double)num2 + (double)num5;
		}
	}

	if (num2 > 0.f)
		outWidth = (double)num / (double)num2;
}



float IGeoManager::WorldAngle(float wx, float wy) {
	return std::sin(std::atan2((double)wx, (double)wy) * 20.0);
}

float IGeoManager::GetBaseHeight(float wx, float wy) const {
	
	// Now its weird here because doubles are defined
	//	unlike where floats were being casted prior...
	//	(maybe the devs overlooked something...)

	//float num4 = 

	double wx1 = wx;
	double wy1 = wy;

	float num2 = VUtils::Math::Magnitude(wx1, wy1);
	wx += 100000.0 + m_offset0;
	wy += 100000.0 + m_offset1;

	float num3 = 0;
	num3 += (double)VUtils::Math::PerlinNoise(wx1 * 0.002 * 0.5, wy1 * 0.002 * 0.5)
		* (double)VUtils::Math::PerlinNoise(wx1 * 0.003 * 0.5, wy1 * 0.003 * 0.5) * 1.0;
	num3 += VUtils::Math::PerlinNoise(wx1 * 0.002 * 1.0, wy1 * 0.002 * 1.0)
		* VUtils::Math::PerlinNoise(wx1 * 0.003 * 1.0, wy1 * 0.003 * 1.0) * (double)num3 * 0.9;
	num3 += VUtils::Math::PerlinNoise(wx1 * 0.005 * 1.0, wy1 * 0.005 * 1.0)
		* VUtils::Math::PerlinNoise(wx1 * 0.010f * 1.0, wy1 * 0.010 * 1.0) * 0.5 * (double)num3;
	num3 -= 0.07f;

	double num4 = VUtils::Math::PerlinNoise(wx1 * 0.002 * 0.25 + 0.123, wy1 * 0.002 * 0.25 + 0.15123);
	float num5 = VUtils::Math::PerlinNoise(wx1 * 0.002 * 0.25 + 0.321, wy1 * 0.002 * 0.25 + 0.231);
	float v = std::abs(float(num4 - (double)num5));
	// TODO stopped here, continue re-precision changes downward \/
	float num6 = 1.0 - VUtils::Math::LerpStep(0.02, 0.12, (double)v);
	num6 *= VUtils::Math::SmoothStep(744.0, 1000.0, (double)num2);
	num3 *= 1.0 - num6;
	if (num2 > 10000)
	{
		float t = VUtils::Math::LerpStep(10000.0, (double)waterEdge, (double)num2);
		num3 = VUtils::Math::Lerp((double)num3, -0.2, (double)t);
		float num7 = 10490;
		if (num2 > num7)
		{
			float t2 = VUtils::Math::LerpStep((double)num7, (double)waterEdge, (double)num2);
			num3 = VUtils::Math::Lerp((double)num3, -2.0, (double)t2);
		}
	}

	if (num2 < m_minMountainDistance && num3 > 0.28f)
	{
		float t3 = VUtils::Math::Clamp01(((double)num3 - 0.28) / 0.09999999403953552);
		
		// Dont know why the devs have allowed varying levels of precision
		//	They're enabling higher precision then nuking it by keeping floats
		num3 = VUtils::Math::Lerp(
			double(float(VUtils::Math::Lerp(0.28, 0.38, (double)t3))),
			(double)num3,
			double(float(VUtils::Math::LerpStep((double)m_minMountainDistance - 400.0, (double)m_minMountainDistance, (double)num2)))
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

	float t = VUtils::Math::LerpStep(20.0, 60.0, (double)v);
	float num2 = VUtils::Math::Lerp(0.14, 0.12, (double)t);
	float num3 = VUtils::Math::Lerp(0.139, 0.128, (double)t);
	if (h > num2)
	{
		h = VUtils::Math::Lerp((double)h, (double)num2, (double)num);
	}
	if (h > num3)
	{
		float t2 = VUtils::Math::LerpStep(0.85, 1.0, (double)num);
		h = VUtils::Math::Lerp((double)h, (double)num3, (double)t2);
	}
	return h;
}


float IGeoManager::GetMarshHeight(float wx, float wy) {
	float wx2 = wx;
	float wy2 = wy;
	float num = 0.137f;
	wx += 100000;
	wy += 100000;
	double num2 = wx;
	double num3 = wy;
	float num4 = (double)VUtils::Math::PerlinNoise(num2 * 0.04, num3 * 0.04) * (double)VUtils::Math::PerlinNoise(num2 * 0.08, num3 * 0.08);
	num += (double)num4 * 0.03;
	num = AddRivers(wx2, wy2, num);
	num += (double)VUtils::Math::PerlinNoise(num2 * 0.1, num3 * 0.1) * 0.01;
	return (double)num + (double)VUtils::Math::PerlinNoise(num2 * 0.4, num3 * 0.4) * 0.03;
}

float IGeoManager::GetMeadowsHeight(float wx, float wy) {
	float wx2 = wx;
	float wy2 = wy;
	float baseHeight = GetBaseHeight(wx, wy);
	wx += 100000.0 + (double)m_offset3;
	wy += 100000.0 + (double)m_offset3;
	double num = wx;
	double num2 = wy;
	float num3 = VUtils::Math::PerlinNoise(num * 0.01, num2 * 0.01) * (double)VUtils::Math::PerlinNoise(num * 0.02, num2 * 0.02);
	num3 = (float)((double)num3 + (double)VUtils::Math::PerlinNoise(num * 0.05, num2 * 0.05) * (double)VUtils::Math::PerlinNoise(num * 0.1, num2 * 0.1) * (double)num3 * 0.5);
	float num4 = baseHeight;
	num4 = (float)((double)num4 + (double)num3 * 0.1);
	float num5 = 0.15f;
	float num6 = (float)((double)num4 - (double)num5);
	float num7 = (float)VUtils::Math::Clamp01((double)float((double)baseHeight / 0.4));
	if (num6 > 0f)
	{
		num4 = (float)((double)num4 - (double)num6 * ((1.0 - (double)num7) * 0.75));
	}
	num4 = this.AddRivers(wx2, wy2, num4);
	num4 = (float)((double)num4 + (double)DUtils.PerlinNoise(num * 0.1, num2 * 0.1) * 0.01);
	return (float)((double)num4 + (double)DUtils.PerlinNoise(num * 0.4, num2 * 0.4) * 0.003);

	
	
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



bool IGeoManager::InsideRiverGrid(Vector2i grid, Vector2f p, float r) {
	Vector2f b((double)grid.x * (double)riverGridSize, (double)grid.y * (double)riverGridSize);
	Vector2f vector = p - b;
	return std::abs(vector.x) < float((double)r + ((double)riverGridSize * 0.5))
		&& std::abs(vector.y) < float((double)r + ((double)riverGridSize * 0.5));
}

Vector2i IGeoManager::GetRiverGrid(float wx, float wy) {
	auto x = (int32_t)std::floor(((double)wx + (double)riverGridSize * .5) / (double)riverGridSize);
	auto y = (int32_t)std::floor(((double)wy + (double)riverGridSize * .5) / (double)riverGridSize);
	return Vector2i(x, y);
}

BiomeArea IGeoManager::GetBiomeArea(Vector3f point) {
	auto&& biome = GetBiome(point);

	auto&& biome2 = GetBiome(point - Vector3f(-IZoneManager::ZONE_SIZE, 0, -IZoneManager::ZONE_SIZE));
	auto&& biome3 = GetBiome(point - Vector3f(IZoneManager::ZONE_SIZE, 0, -IZoneManager::ZONE_SIZE));
	auto&& biome4 = GetBiome(point - Vector3f(IZoneManager::ZONE_SIZE, 0, IZoneManager::ZONE_SIZE));
	auto&& biome5 = GetBiome(point - Vector3f(-IZoneManager::ZONE_SIZE, 0, IZoneManager::ZONE_SIZE));
	auto&& biome6 = GetBiome(point - Vector3f(-IZoneManager::ZONE_SIZE, 0, 0));
	auto&& biome7 = GetBiome(point - Vector3f(IZoneManager::ZONE_SIZE, 0, 0));
	auto&& biome8 = GetBiome(point - Vector3f(0, 0, -IZoneManager::ZONE_SIZE));
	auto&& biome9 = GetBiome(point - Vector3f(0, 0, IZoneManager::ZONE_SIZE));
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
Biome IGeoManager::GetBiome(Vector3f point) {
	return GetBiome(point.x, point.z);
}

// public
Biome IGeoManager::GetBiome(float wx, float wy) {
	auto magnitude = VUtils::Math::Magnitude(wx, wy);
	auto baseHeight = GetBaseHeight(wx, wy);
	float num = (double)WorldAngle(wx, wy) * 100.0;

	// bottom curve of world are ashlands
	if (VUtils::Math::Magnitude(wx, (double)wy + (double)ashlandsYOffset) > (double)ashlandsMinDistance + (double)num)
		return Biome::AshLands;

	if (baseHeight <= 0.02f)
		return Biome::Ocean;

	// top curve of world is deep north
	if (VUtils::Math::Magnitude(wx, (double)wy + (double)deepNorthYOffset) > (double)deepNorthMinDistance + (double)num) {
		if (baseHeight > mountainBaseHeightMin)
			return Biome::Mountain;
		return Biome::DeepNorth;
	}

	if (baseHeight > mountainBaseHeightMin)
		return Biome::Mountain;

	/*
	* Notice the doubles here:
	*	They DEFINITLY make a difference
	*/
	if (VUtils::Math::PerlinNoise(((double)m_offset0 + (double)wx) * (double)marshBiomeScale, ((double)m_offset0 + (double)wy) * (double)marshBiomeScale) > minMarshNoise
		&& magnitude > minMarshDistance && magnitude < maxMarshDistance && baseHeight > minMarshHeight && baseHeight < maxMarshHeight)
		return Biome::Swamp;

	if (VUtils::Math::PerlinNoise(((double)m_offset4 + (double)wx) * (double)darklandBiomeScale, ((double)m_offset4 + (double)wy) * (double)darklandBiomeScale) > minDarklandNoise
		&& magnitude > (double)minDarklandDistance + (double)num && magnitude < maxDarklandDistance)
		return Biome::Mistlands;

	if (VUtils::Math::PerlinNoise(((double)m_offset1 + (double)wx) * (double)heathBiomeScale, ((double)m_offset1 + (double)wy) * (double)heathBiomeScale) > minHeathNoise
		&& magnitude > (double)minHeathDistance + (double)num && magnitude < maxHeathDistance)
		return Biome::Plains;

	// TODO flesh out .001 constant here (and some above)
	//	or wait until devs use doubles everywhere (instead of elevating precision in-between calculations...)
	if (VUtils::Math::PerlinNoise(((double)m_offset2 + (double)wx) * (double)0.001f, ((double)m_offset2 + (double)wy) * (double)0.001f) > minDeepForestNoise
		&& (double)magnitude > (double)minDeepForestDistance + (double)num && magnitude < maxDeepForestDistance)
		return Biome::BlackForest;

	if (magnitude > (double)meadowsMaxDistance + (double)num)
		return Biome::BlackForest;

	return Biome::Meadows;
}

Biome IGeoManager::GetBiomes(float x, float z) {
	//ZoneID zone = IZoneManager::WorldToZonePos(Vector3f(x, 0., z));
	//Vector3f center = IZoneManager::ZoneToWorldPos(zone) + ;
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
		return (double)GetMeadowsHeight(wx, wy) * 200.0;
	case Biome::Swamp:
		return (double)GetMarshHeight(wx, wy) * 200.0;
	case Biome::Mountain:
		return (double)GetSnowMountainHeight(wx, wy) * 200.0;
	case Biome::BlackForest:
		return (double)GetForestHeight(wx, wy) * 200.0;
	case Biome::Plains:
		return (double)GetPlainsHeight(wx, wy) * 200.0;
	case Biome::AshLands:
		return (double)GetAshlandsHeight(wx, wy) * 200.0;
	case Biome::DeepNorth:
		return (double)GetDeepNorthHeight(wx, wy) * 200.0;
	case Biome::Ocean:
		return (double)GetOceanHeight(wx, wy) * 200.0;
	case Biome::Mistlands:
		return (double)GetMistlandsHeight(wx, wy, mask) * 200.0;
	}
	// TODO or throw
	//return 0;
	std::unreachable();
}

// public
bool IGeoManager::InForest(Vector3f pos) {
	return GetForestFactor(pos) < 1.15f;
}

// public
float IGeoManager::GetForestFactor(Vector3f pos) {
	float d = 0.4f;
	return VUtils::Math::Fbm(pos * 0.01f * d, 3, 1.6f, 0.7f);
}

// public
void IGeoManager::GetTerrainDelta(VUtils::Random::State& state, Vector3f center, float radius, float& delta, Vector3f& slopeDirection) {
	int num = 10;
	float num2 = std::numeric_limits<float>::min();
	float num3 = std::numeric_limits<float>::max();
	Vector3f b = center;
	Vector3f a = center;
	for (int i = 0; i < num; i++)
	{
		Vector2f vector = state.InsideUnitCircle() * radius;
		Vector3f vector2 = center + Vector3f(vector.x, 0.f, vector.y);
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
	slopeDirection = (a - b).Normal();
}

// public
int IGeoManager::GetSeed() {
#ifdef RUN_TESTS
	return 0;
#else
	return m_world->m_seed;
#endif
}
#endif // VH_GENERATE_ZONES