struct Entry
{
    uint originalIndex;
    uint hash;
    uint key;
};

#define BITONIC_BLOCK_SIZE 512
#define TRANSPOSE_BLOCK_SIZE 16

RWStructuredBuffer<Entry> Entries : register(u2);

cbuffer SortConstants : register(b5)
{
    uint groupWidth;
    uint groupHeight;
    uint stepIndex;
    uint numOfEntries;

}

groupshared uint shared_data[512];

[numthreads(512, 1, 1)]
void Sort(  uint3 id : SV_DispatchThreadID, 
            uint3 Gid : SV_GroupID,
            uint3 GTid : SV_GroupThreadID,
            uint GI : SV_GroupIndex)
{
    uint i = id.x;
    
    uint hIndex = i & (groupWidth - 1);
    uint indexLeft = hIndex + (groupHeight + 1) * (i / groupWidth);
    uint rightStepSize = stepIndex == 0 ? groupHeight - 2 * hIndex : (groupHeight + 1) / 2;
    uint indexRight = indexLeft + rightStepSize;
    
    // Exit if out of bounds (for non-power of 2 input sizes)
    if (indexRight >= numOfEntries)
        return;
    
    uint valueLeft = Entries[indexLeft].key;
    uint valueRight = Entries[indexRight].key;

    // Swap entries if value is descending    
    if (valueLeft > valueRight)
    {
        Entry temp = Entries[indexLeft];
        Entries[indexLeft] = Entries[indexRight];
        Entries[indexRight] = temp;
    }

}

// Calculate offsets into the sorted buffer (used for spatial hashing).
// For example if the sorted buffer looks like -> Sorted: {0001223333}
// The resulting offsets would be -> Offsets: {0003446666}
// This means that, if for instance we look up Sorted[8] (which has a value of 3), we could then look up 
// Offsets[8] to get a value of 6, which is the index where the group of 3's begins in the Sorted buffer.
// NOTE: offsets buffer must filled with values equal to (or greater than) its length
RWStructuredBuffer<uint> Offsets : register(u3);

[numthreads(512, 1, 1)]
void CalculateOffsets(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= numOfEntries)
    {
        return;
    }
    uint i = id.x;

    uint key = Entries[i].key;
    uint keyPrev = i == 0 ? 9999999 : Entries[i - 1].key;
    if (key != keyPrev)
    {
        Offsets[key] = i;
    }
    
}