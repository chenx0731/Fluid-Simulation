#pragma once
#include "Engine/Math/IntVec3.hpp"
#include <unordered_map>
#include <vector>
//struct Vec3;
struct IntVec3;
class Particle;
struct SpatialCell
{
	unsigned int m_timestamp;
	std::vector<unsigned int> m_particleIndices;
};
class NeighborSearch
{
public:
	NeighborSearch(unsigned int numParticles = 0, float radius = 0.1, unsigned int maxNeighbors = 60, unsigned int maxParticlesPercell = 50);
	~NeighborSearch();
	unsigned int HashDictionary(IntVec3 const& pos);
	void UpdateNeighbors(std::vector<Particle>& particles, const unsigned int& start, const unsigned int& num);

	void CleanUp();
	void UpdateTimestamp() { m_currentTimestamp++; }
	void SetRadius(const float& radius) { m_radius = radius; m_gridSize = radius; }
	void SetMaxNeighbors(const unsigned int& num) { m_maxNeighbors = num; }

	float GetRadius() const { return m_radius; }
	unsigned int GetMaxNeighbors() const { return m_maxNeighbors; }
	std::vector<unsigned int> GetNeighbors(const unsigned int& index) const { return m_neighbors[index]; }
	unsigned int GetNeighborsNum(const unsigned int& index) const { return (unsigned int)m_neighbors[index].size(); }
	unsigned int GetNeighborByIndex(const unsigned int& i, const unsigned int& j) const { return m_neighbors[i][j]; }
public:
	float m_radius;
	float m_gridSize;
	unsigned int m_maxNeighbors;
	unsigned int m_numParticles;
	unsigned int m_currentTimestamp;
	unsigned int m_maxParticlesPerCell;
	std::unordered_map<IntVec3, SpatialCell*, IntVec3Hash, IntVec3Equal> m_spatialMap;
	std::vector<std::vector<unsigned int>> m_neighbors;
};