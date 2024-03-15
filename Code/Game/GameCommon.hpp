#pragma once
struct Vec2;
struct Rgba8;
struct Vertex_PCU;
class App;
class Game;
class Renderer;
class RandomNumberGenerator;
class InputSystem;
class AudioSystem;
class ImGuiBase;
class Texture;
class NamedStrings;
class SpriteSheet;
class SpriteAnimDefinition;
class Window;

typedef size_t SoundID;
typedef size_t SoundPlaybackID;

enum Music
{
	MUSIC_NULL = -1,

	MUSIC_WELCOME,

	MUSIC_NUM
};

enum TexturePics
{
	TEXTURE_NULL = -1,

	TEXTURE_DEFAULT,

	TEXTURE_SPHERE,

	TEXTURE_CUBE,

	TEXTURE_NUM
};

extern App* g_theApp;
extern Renderer* g_theRenderer;
extern RandomNumberGenerator* g_rng;
extern InputSystem* g_theInput;
extern Window* g_theWindow;
extern AudioSystem* g_theAudio;
extern Game* g_theGame;
extern SpriteSheet* g_terrainSprites;
extern SpriteAnimDefinition* g_explosionAnim;
extern ImGuiBase* g_ImGui;

extern SoundID					g_musicLibrary[MUSIC_NUM];
extern SoundPlaybackID			g_lastPlayedMusic[MUSIC_NUM];
extern Texture*					g_textureLibrary[TEXTURE_NUM];

constexpr float			WORLD_SIZE_X = 25.f;
constexpr float			WORLD_SIZE_Y = 32.f;
constexpr float			WORLD_CAMERA_SIZE_X = 1.f;
constexpr float			WORLD_CAMERA_SIZE_Y = 1.f;
constexpr float			SCREEN_CAMERA_SIZE_X = 1600.f;
constexpr float			SCREEN_CAMERA_SIZE_Y = 800.f;
constexpr float			WORLD_CENTER_X = WORLD_SIZE_X / 2.f;
constexpr float			WORLD_CENTER_Y = WORLD_SIZE_Y / 2.f;

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& start, float forwardOrientationDegree,float length, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 color);
void DrawCircle(Vec2 const& position, float radius, Rgba8 color);
void DrawSquare(Vec2 const& start, Vec2 const& end, Rgba8 color);
void DrawStarRing(Vec2 const& center, float radius, float thickness, Rgba8 color);
void DrawStarCircle(Vec2 const& center, float radius, float thickness, float gap, Rgba8 color);
void DrawLaser(Vec2 const& center, float radius, float thickness, float startDegree, float endDegree, Rgba8 color);
void DrawStarCurve(Vec2 const& center, float radius, float thickness, float startDegree, float endDegree, Rgba8 color);
void DrawStarCurves(Vec2 const& center, float radius, float thickness, float gap, float width, float startDegree, float endDegree, Rgba8 color);
void PlayMusicLoop(Music music);
void PlayMusic(Music music);
void StopMusic(Music music);
void PauseMusic(Music music);
void ResumeMusic(Music music);
void SetMusicSpeed(Music music, float speed);