#include "Game/GameCommon.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Audio/AudioSystem.hpp"
void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color)
{
	float halfThickness = 0.5f * thickness;
	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;
	constexpr int NUM_SIDES = 32;
	constexpr int NUM_TRIS = 2 * NUM_SIDES;
	constexpr int NUM_VERTS = 3 * NUM_TRIS;
	Vertex_PCU verts[NUM_VERTS];
	constexpr float DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);
	for (int sideNum = 0; sideNum < NUM_SIDES; sideNum++)
	{
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
		float endDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
		float cosStart = CosDegrees(startDegrees);
		float sinStart = SinDegrees(startDegrees);
		float cosEnd = CosDegrees(endDegrees);
		float sinEnd = SinDegrees(endDegrees);

		Vec3 innerStartPos(center.x + innerRadius * cosStart, center.y + innerRadius * sinStart, 0.f);
		Vec3 outerStartPos(center.x + outerRadius * cosStart, center.y + outerRadius * sinStart, 0.f);
		Vec3 outerEndPos(center.x + outerRadius * cosEnd, center.y + outerRadius * sinEnd, 0.f);
		Vec3 innerEndPos(center.x + innerRadius * cosEnd, center.y + innerRadius * sinEnd, 0.f);

		int vertIndexA = (6 * sideNum) + 0;
		int vertIndexB = (6 * sideNum) + 1;
		int vertIndexC = (6 * sideNum) + 2;
		int vertIndexD = (6 * sideNum) + 3;
		int vertIndexE = (6 * sideNum) + 4;
		int vertIndexF = (6 * sideNum) + 5;

		verts[vertIndexA].m_position = innerEndPos;
		verts[vertIndexB].m_position = innerStartPos;
		verts[vertIndexC].m_position = outerStartPos;
		verts[vertIndexA].m_color = color;
		verts[vertIndexB].m_color = color;
		verts[vertIndexC].m_color = color;

		verts[vertIndexD].m_position = innerEndPos;
		verts[vertIndexE].m_position = outerStartPos;
		verts[vertIndexF].m_position = outerEndPos;
		verts[vertIndexD].m_color = color;
		verts[vertIndexE].m_color = color;
		verts[vertIndexF].m_color = color;
	}
	g_theRenderer->DrawVertexArray(NUM_VERTS, verts);
}

void DebugDrawLine(Vec2 const& start, float forwardOrientationDegree, float length, float thickness, Rgba8 const& color)
{
	float halfThickness = 0.5f * thickness;
	Vec2 forward = Vec2(length * CosDegrees(forwardOrientationDegree), length * SinDegrees(forwardOrientationDegree));
	Vec2 leftShort = forward.GetRotated90Degrees();
	leftShort.SetLength(halfThickness);
	Vec2 startLeft = start + leftShort;
	Vec2 startRight = start - leftShort;
	Vec2 endLeft = start + forward + leftShort;
	Vec2 endRight = start + forward - leftShort;

	constexpr int NUM_TRIS = 2;
	constexpr int NUM_VERTS = 3 * NUM_TRIS;
	Vertex_PCU verts[NUM_VERTS];

	verts[0].m_position = Vec3(startLeft.x, startLeft.y, 0.f);
	verts[1].m_position = Vec3(startRight.x, startRight.y, 0.f);
	verts[2].m_position = Vec3(endRight.x, endRight.y, 0.f);

	verts[3].m_position = Vec3(endRight.x, endRight.y, 0.f);
	verts[4].m_position = Vec3(endLeft.x, endLeft.y, 0.f);
	verts[5].m_position = Vec3(startLeft.x, startLeft.y, 0.f);

	for (int vertIndex = 0; vertIndex < NUM_VERTS; vertIndex++)
	{
		verts[vertIndex].m_color = color;
	}

	g_theRenderer->DrawVertexArray(NUM_VERTS, verts);
}

void DebugDrawLine(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 color)
{
	Vec2 startToEnd = end - start;
	float orientationDegree = Atan2Degrees(startToEnd.y, startToEnd.x);
	DebugDrawLine(start, orientationDegree, startToEnd.GetLength(), thickness, color);
}

void DrawCircle(Vec2 const& position, float radius, Rgba8 color)
{
	const int NUM_SIDES = 32;
	const int NUM_VERTS = NUM_SIDES * 3;
	Vertex_PCU tempVertexes[NUM_VERTS];
	float angle = 360.f / static_cast<float>(NUM_SIDES);
	for (int i = 0; i < NUM_SIDES; i++)
	{
		tempVertexes[i * 3].m_position = Vec3(position.x, position.y, 0.f);
		Vec2 pointOne = Vec2::MakeFromPolarDegrees(angle * i, radius);
		tempVertexes[i * 3 + 1].m_position = Vec3(position.x + pointOne.x, position.y + pointOne.y, 0.f);
		Vec2 pointTwo = Vec2::MakeFromPolarDegrees(angle * (i + 1), radius);
		tempVertexes[i * 3 + 2].m_position = Vec3(position.x + pointTwo.x, position.y + pointTwo.y, 0.f);
	}
	for (int i = 0; i < NUM_VERTS; i++)
	{
		tempVertexes[i].m_color = color;
	}

	g_theRenderer->DrawVertexArray(NUM_VERTS, tempVertexes);
}

void DrawSquare(Vec2 const& start, Vec2 const& end, Rgba8 color)
{
	Vertex_PCU verts[6];
	verts[0].m_position = Vec3(start.x, start.y, 0.f);
	verts[2].m_position = Vec3(start.x, end.y, 0.f);
	verts[1].m_position = Vec3(end.x, start.y, 0.f);
	verts[3].m_position = Vec3(start.x, end.y, 0.f);
	verts[4].m_position = Vec3(end.x, start.y, 0.f);
	verts[5].m_position = Vec3(end.x, end.y, 0.f);
	for (int i = 0; i < 6; i++)
	{
		verts[i].m_color = color;
	}
	verts[0].m_uvTexCoords = Vec2(0.f, 1.f);
	verts[2].m_uvTexCoords = Vec2(0.f, 0.f);
	verts[1].m_uvTexCoords = Vec2(1.f, 1.f);
	verts[3].m_uvTexCoords = Vec2(0.f, 0.f);
	verts[4].m_uvTexCoords = Vec2(1.f, 1.f);
	verts[5].m_uvTexCoords = Vec2(1.f, 0.f);
	g_theRenderer->DrawVertexArray(6, verts);
}

void DrawStarRing(Vec2 const& center, float radius, float thickness, Rgba8 color)
{
	float halfThickness = 0.5f * thickness;
	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;
	constexpr int NUM_SIDES = 16;
	constexpr int NUM_TRIS = 2 * NUM_SIDES;
	constexpr int NUM_VERTS = 3 * NUM_TRIS;
	Vertex_PCU verts[NUM_VERTS];
	constexpr float DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);
	int start = g_rng->RollRandomIntInRange(0, 16);
	int end;
	if (start > 8)
		end = start - 8;
	else end = start + 8;
	for (int sideNum = 0; sideNum < NUM_SIDES; sideNum++)
	{
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
		float endDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
		float cosStart = CosDegrees(startDegrees);
		float sinStart = SinDegrees(startDegrees);
		float cosEnd = CosDegrees(endDegrees);
		float sinEnd = SinDegrees(endDegrees);

		Vec3 innerStartPos(center.x + innerRadius * cosStart, center.y + innerRadius * sinStart, 0.f);
		Vec3 outerStartPos(center.x + outerRadius * cosStart, center.y + outerRadius * sinStart, 0.f);
		Vec3 outerEndPos(center.x + outerRadius * cosEnd, center.y + outerRadius * sinEnd, 0.f);
		Vec3 innerEndPos(center.x + innerRadius * cosEnd, center.y + innerRadius * sinEnd, 0.f);

		int vertIndexA = (6 * sideNum) + 0;
		int vertIndexB = (6 * sideNum) + 1;
		int vertIndexC = (6 * sideNum) + 2;
		int vertIndexD = (6 * sideNum) + 3;
		int vertIndexE = (6 * sideNum) + 4;
		int vertIndexF = (6 * sideNum) + 5;

		if (start > end)
		{
			if (sideNum < end || sideNum > start)
				color.a = 127;
		}
		else
		{
			if (sideNum > start && sideNum < end)
			{
				color.a = 127;
			}
			else if (sideNum >= end)
			{
				color.a = 255;
			}
		}
		
		verts[vertIndexA].m_position = innerEndPos;
		verts[vertIndexB].m_position = innerStartPos;
		verts[vertIndexC].m_position = outerStartPos;
		verts[vertIndexA].m_color = color;
		verts[vertIndexB].m_color = color;
		verts[vertIndexC].m_color = color;
	
		verts[vertIndexD].m_position = innerEndPos;
		verts[vertIndexE].m_position = outerStartPos;
		verts[vertIndexF].m_position = outerEndPos;
		verts[vertIndexD].m_color = color;
		verts[vertIndexE].m_color = color;
		verts[vertIndexF].m_color = color;

	}
	
	g_theRenderer->DrawVertexArray(NUM_VERTS, verts);
}

void DrawStarCircle(Vec2 const& center, float radius, float thickness, float gap, Rgba8 color)
{
	int layer = static_cast<int> (radius / (thickness + gap));
	for (int i = 0; i < layer; i++)
	{
		float index = static_cast<float>(i);
		Rgba8 tempColor = color;
		/*float scale = SinDegrees(50.f * static_cast<float> (GetCurrentTimeSeconds()));
		if (scale < 0) scale = -scale;
		tempColor.b += static_cast<unsigned char>(255.f * scale);
		if (tempColor.b > 150)
		{
			tempColor.b = 150;
		}
		else if(tempColor.b < 100)
		{
			tempColor.b = 100;
		}*/
		DrawStarRing(center, thickness * (index + 1.f) + gap * index, thickness, tempColor);
	}
}

void DrawLaser(Vec2 const& center, float radius, float thickness, float startDegree, float endDegree, Rgba8 color)
{
	float halfThickness = 0.5f * thickness;
	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;
	constexpr int NUM_SIDES = 32;
	constexpr int NUM_TRIS = 2 * NUM_SIDES;
	constexpr int NUM_VERTS = 3 * NUM_TRIS;
	Vertex_PCU verts[NUM_VERTS];
	float DEGREES_PER_SIDE = (endDegree - startDegree) / static_cast<float>(NUM_SIDES);
	int start = g_rng->RollRandomIntInRange(0, 16);
	int end;
	if (start > 8)
		end = start - 8;
	else end = start + 8;
	for (int sideNum = 0; sideNum < NUM_SIDES; sideNum++)
	{
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum) ;
		float endDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);
		float cosStart = CosDegrees(startDegrees + startDegree);
		float sinStart = SinDegrees(startDegrees + startDegree);
		float cosEnd = CosDegrees(endDegrees);
		float sinEnd = SinDegrees(endDegrees);

		Vec3 innerStartPos(center.x + innerRadius * cosStart, center.y + innerRadius * sinStart, 0.f);
		Vec3 outerStartPos(center.x + outerRadius * cosStart, center.y + outerRadius * sinStart, 0.f);
		Vec3 outerEndPos(center.x + outerRadius * cosEnd, center.y + outerRadius * sinEnd, 0.f);
		Vec3 innerEndPos(center.x + innerRadius * cosEnd, center.y + innerRadius * sinEnd, 0.f);

		int vertIndexA = (6 * sideNum) + 0;
		int vertIndexB = (6 * sideNum) + 1;
		int vertIndexC = (6 * sideNum) + 2;
		int vertIndexD = (6 * sideNum) + 3;
		int vertIndexE = (6 * sideNum) + 4;
		int vertIndexF = (6 * sideNum) + 5;

		if (start > end)
		{
			if (sideNum < end || sideNum > start)
				color.a = 127;
		}
		else
		{
			if (sideNum > start && sideNum < end)
			{
				color.a = 127;
			}
			else if (sideNum >= end)
			{
				color.a = 255;
			}
		}

		verts[vertIndexA].m_position = innerEndPos;
		verts[vertIndexB].m_position = innerStartPos;
		verts[vertIndexC].m_position = outerStartPos;
		verts[vertIndexA].m_color = color;
		verts[vertIndexB].m_color = color;
		verts[vertIndexC].m_color = color;

		verts[vertIndexD].m_position = innerEndPos;
		verts[vertIndexE].m_position = outerStartPos;
		verts[vertIndexF].m_position = outerEndPos;
		verts[vertIndexD].m_color = color;
		verts[vertIndexE].m_color = color;
		verts[vertIndexF].m_color = color;

	}

	g_theRenderer->DrawVertexArray(NUM_VERTS, verts);
}

void DrawStarCurve(Vec2 const& center, float radius, float thickness, float startDegree, float endDegree, Rgba8 color)
{
	float halfThickness = 0.5f * thickness;
	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;
	constexpr int NUM_SIDES = 16;
	constexpr int NUM_TRIS = 2 * NUM_SIDES;
	constexpr int NUM_VERTS = 3 * NUM_TRIS;
	Vertex_PCU verts[NUM_VERTS];
	float DEGREES_PER_SIDE = (endDegree - startDegree) / static_cast<float>(NUM_SIDES);
	int start = g_rng->RollRandomIntInRange(0, 16);
	int end;
	if (start > 8)
		end = start - 8;
	else end = start + 8;
	for (int sideNum = 0; sideNum < NUM_SIDES; sideNum++)
	{
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum) + startDegree;
		float endDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1) + startDegree;
		float cosStart = CosDegrees(startDegrees);
		float sinStart = SinDegrees(startDegrees);
		float cosEnd = CosDegrees(endDegrees);
		float sinEnd = SinDegrees(endDegrees);

		Vec3 innerStartPos(center.x + innerRadius * cosStart, center.y + innerRadius * sinStart, 0.f);
		Vec3 outerStartPos(center.x + outerRadius * cosStart, center.y + outerRadius * sinStart, 0.f);
		Vec3 outerEndPos(center.x + outerRadius * cosEnd, center.y + outerRadius * sinEnd, 0.f);
		Vec3 innerEndPos(center.x + innerRadius * cosEnd, center.y + innerRadius * sinEnd, 0.f);

		int vertIndexA = (6 * sideNum) + 0;
		int vertIndexB = (6 * sideNum) + 1;
		int vertIndexC = (6 * sideNum) + 2;
		int vertIndexD = (6 * sideNum) + 3;
		int vertIndexE = (6 * sideNum) + 4;
		int vertIndexF = (6 * sideNum) + 5;

		if (start > end)
		{
			if (sideNum < end || sideNum > start)
				color.a = 127;
		}
		else
		{
			if (sideNum > start && sideNum < end)
			{
				color.a = 127;
			}
			else if (sideNum >= end)
			{
				color.a = 255;
			}
		}

		verts[vertIndexA].m_position = innerEndPos;
		verts[vertIndexB].m_position = innerStartPos;
		verts[vertIndexC].m_position = outerStartPos;
		verts[vertIndexA].m_color = color;
		verts[vertIndexB].m_color = color;
		verts[vertIndexC].m_color = color;

		verts[vertIndexD].m_position = innerEndPos;
		verts[vertIndexE].m_position = outerStartPos;
		verts[vertIndexF].m_position = outerEndPos;
		verts[vertIndexD].m_color = color;
		verts[vertIndexE].m_color = color;
		verts[vertIndexF].m_color = color;

	}

	g_theRenderer->DrawVertexArray(NUM_VERTS, verts);
}

void DrawStarCurves(Vec2 const& center, float radius, float thickness, float gap, float width, float startDegree, float endDegree, Rgba8 color)
{
	int layer = static_cast<int> (width / (thickness + gap));
	for (int i = 0; i < layer; i++)
	{
		float index = static_cast<float>(i);
		Rgba8 tempColor = color;
		DrawStarCurve(center, radius - index * (thickness + gap), thickness, startDegree, endDegree, tempColor);
	}
}

void PlayMusicLoop(Music music)
{
	g_lastPlayedMusic[music] = g_theAudio->StartSound(g_musicLibrary[music], true);
}

void PlayMusic(Music music)
{
	g_lastPlayedMusic[music] = g_theAudio->StartSound(g_musicLibrary[music]);
}

void StopMusic(Music music)
{
	g_theAudio->StopSound(g_lastPlayedMusic[music]);
}

void PauseMusic(Music music)
{
	g_theAudio->SetSoundPlaybackSpeed(g_lastPlayedMusic[music], 0.f);
}

void ResumeMusic(Music music)
{
	g_theAudio->SetSoundPlaybackSpeed(g_lastPlayedMusic[music], 1.f);
}

void SetMusicSpeed(Music music, float speed)
{
	g_theAudio->SetSoundPlaybackSpeed(g_lastPlayedMusic[music], speed);
}
