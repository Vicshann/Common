
#pragma once

//---------------------------------------------------------------------------
// For compile-time constants 64-bit results are free
static constexpr inline uint64 CT_SEED = (CRC32(__DATE__ __TIME__, CRC32(__FILE__, CRC32(__DATE__, uint32(-1)))) | ((uint64)CRC32(__TIME__ __DATE__, CRC32(__FILE__, CRC32(__TIME__, uint32(-1)))) << 32 ));

consteval uint64 CT_Random(uint64 Ctr)  // Test if it is OK after update to uint64
{
 return (1013904323 + 1664625 * (CT_SEED * (Ctr+1)));
}
consteval uint64 CT_Random(uint64 Ctr, uint64 VMin, uint64 VMax){return (VMin + (CT_Random(Ctr) % (VMax - VMin + 1)));}
//---------------------------------------------------------------------------



