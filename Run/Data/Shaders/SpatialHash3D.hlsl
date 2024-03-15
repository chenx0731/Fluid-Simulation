static const int3 offsets3D[27] =
{
	int3(-1, -1, -1),
	int3(-1, -1, 0),
	int3(-1, -1, 1),
	int3(-1, 0, -1),
	int3(-1, 0, 0),
	int3(-1, 0, 1),
	int3(-1, 1, -1),
	int3(-1, 1, 0),
	int3(-1, 1, 1),
	int3(0, -1, -1),
	int3(0, -1, 0),
	int3(0, -1, 1),
	int3(0, 0, -1),
	int3(0, 0, 0),
	int3(0, 0, 1),
	int3(0, 1, -1),
	int3(0, 1, 0),
	int3(0, 1, 1),
	int3(1, -1, -1),
	int3(1, -1, 0),
	int3(1, -1, 1),
	int3(1, 0, -1),
	int3(1, 0, 0),
	int3(1, 0, 1),
	int3(1, 1, -1),
	int3(1, 1, 0),
	int3(1, 1, 1)
};


static const uint hashK1 = 15823;
static const uint hashK2 = 9737333;
static const uint hashK3 = 440817757;
             
/*
int GetCell3D(float3 position, float radius)
{
    //return (int3) floor(float3(position.x / radius, position.y / radius, position.z / radius));
    return int3(floor(position.x / radius), floor(position.y / radius), floor(position.z / radius));

}*/

int3 GetCell3D(float3 position, float radius)
{
    return (int3) floor(position / radius);
    //return int3((int)floor(x / radius), (int)floor(y / radius), (int)floor(z / radius));

}

uint HashCell3D(int3 cell)
{
	cell = (uint3) cell;
    return cell.x * hashK1 + cell.y * hashK2 + cell.z * hashK3;
}

uint KeyFromHash(uint hash, uint tableSize)
{
    return hash % tableSize;
}