#include "Game/NeighborSearch.hpp"
#include "Game/Particle.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec3.hpp"
//#include "Engine/Math/IntVec3.hpp"

const float PI = 3.141592653589793f;

NeighborSearch::NeighborSearch(unsigned int numParticles, float radius, unsigned int maxNeighbors, unsigned int maxParticlesPercell)
{
	m_numParticles = numParticles;
	m_radius = radius;
	m_gridSize = radius;
	m_maxNeighbors = maxNeighbors;
	m_maxParticlesPerCell = maxParticlesPercell;
}

NeighborSearch::~NeighborSearch()
{
	CleanUp();
}

unsigned int NeighborSearch::HashDictionary(IntVec3 const& pos)
{
	const int p1 = 73856093 * pos.x;
	const int p2 = 19349663 * pos.y;
	const int p3 = 83492791 * pos.z;
	return p1 + p2 + p3;
}	

void NeighborSearch::UpdateNeighbors(std::vector<Particle>& particles, const unsigned int& start, const unsigned int& num)
{
	const float factor = static_cast<float>(1.0 / m_gridSize);
	int numParticles = (int)particles.size();
	m_neighbors.assign(numParticles, std::vector<unsigned int>());

	//for (auto it = m_spatialMap.end(); it != m_spatialMap.end())

	// hash mapping
	for (int index = 0; index < numParticles; index++) {
		IntVec3 cellPos = IntVec3();
		cellPos.x = RoundDownToInt(particles[index].GetPosition().x * factor);
		cellPos.y = RoundDownToInt(particles[index].GetPosition().y * factor);
		cellPos.z = RoundDownToInt(particles[index].GetPosition().z * factor);

		SpatialCell* entry = nullptr;
		auto it = m_spatialMap.find(cellPos);
		if (it != m_spatialMap.end())
			entry = it->second;

		if (entry != nullptr) {
			if (entry->m_timestamp != m_currentTimestamp) {
				entry->m_timestamp = m_currentTimestamp;
				entry->m_particleIndices.clear();
			}
		}
		else {
			SpatialCell* newEntry = new SpatialCell();
			newEntry->m_particleIndices.reserve(m_maxParticlesPerCell);
			newEntry->m_timestamp = m_currentTimestamp;
			entry = newEntry;
			m_spatialMap.emplace(cellPos, entry);
		}
		entry->m_particleIndices.push_back(index);
	}

	// hash lookup

	int end = start + num;

	for (int index = start; index < end; index++)
	{
		if (index == 0) {
			if (particles[index].GetPosition().z < 0.05f && particles[index].GetPosition().z > -0.05f)
			{
				end = start + num;
			}
		}
		const int cellPos1 = RoundDownToInt(particles[index].GetPosition().x * factor);
		const int cellPos2 = RoundDownToInt(particles[index].GetPosition().y * factor);
		const int cellPos3 = RoundDownToInt(particles[index].GetPosition().z * factor);
		for (int i = -1; i < 2; i++)
		{
			for (int j = -1; j < 2; j++)
			{
				for (int k = -1; k < 2; k++)
				{
					IntVec3 cellPos(cellPos1 + i, cellPos2 + j, cellPos3 + k);
					auto it = m_spatialMap.find(cellPos);
					//it = m_spatialMap.find(IntVec3(0, 20, 100));
					if (it != m_spatialMap.end()) {
						SpatialCell* const* entry = &m_spatialMap.find(cellPos)->second;
						//m_spatialMap.query(cellPos);
						if ((entry != nullptr) && (*entry != nullptr) && ((*entry)->m_timestamp == m_currentTimestamp))
						{
							for (unsigned int m = 0; m < (*entry)->m_particleIndices.size(); ++m)
							{
								const int neighbor = (*entry)->m_particleIndices[m];
								// not consider itself.
								if (index != neighbor)
								{
									const float dist = GetDistance3D(particles[index].GetPosition(), particles[neighbor].GetPosition());
									if (dist < m_radius)
									{
										m_neighbors[index].push_back(neighbor);
									}
								}
							}
						}
					}
					
				}
			}
		}
	}
	for (auto it = m_spatialMap.begin(); it != m_spatialMap.end(); it++) {
		SpatialCell* temp = it->second;
		if (temp != nullptr) {
			delete temp;
			it->second = nullptr;
		}
	}
	m_spatialMap.clear();
}

void NeighborSearch::CleanUp()
{
	std::vector<std::vector<unsigned int>>().swap(m_neighbors);
	m_numParticles = 0;
	for (auto it = m_spatialMap.begin(); it != m_spatialMap.end(); it++) {
		SpatialCell* temp = it->second;
		if (temp != nullptr) {
			delete temp;
			it->second = nullptr;
		}
	}
	m_spatialMap.clear();
}
