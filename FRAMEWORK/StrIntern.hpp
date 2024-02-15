

// Strings are not zero terminated so new strings can refer to parts of previously added ones
// No string removal
// NOTE: Adding bigger strings first can squeeze some space
// TODO: Tree - for optimal string search
// Or align every string to 16 or 8 for fast hashing and lose ability to reuse string parts
class CStrIn
{


public:
struct SStr     // 16 bytes
{
// uint64 Hash;   // What for?  // Tree structure should help to avoid that
 uint32 Offs;   // Max 4GB
 uint32 Size;   // Not for entire books :)
};


};
