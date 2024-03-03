
// NOTE: Every char position in a token takes 8K. Strings and comments must be tokenized to lower memory cost.
// Reserve two 4k pages but second one is for UTF-8 extension chars and unlikely to be used
// First 32 bytes on each 8k block is a header because position of 0 char value is not used
// NOTE: Wery cache inefficient(?) but avoids hashing entirely and can reference string which is part of another strings
// A string entry point is its node index + pos in it (Packed as a page pointer)
// Entry points are not unique. A specific string cannot be exracted by a entry point, only a presence can be tested by mapping
// Exit points are not unique either
// Looks like a some kind of a model, cannot extract precise data, only match it
// Good only for autocomplete?

class CTknIntern
{
 SCVR uint TotalChars  = 256;
 SCVR uint BytesPerPos = TotalChars / 8;  // 32

union SBlkHdr     // 4096 / 32 = 128
{
 struct
  {
   SBlkHdr* Next;
   SBlkHdr* Ends;    // Termination points for this page // Each set bit means that some token terminates at this position (Can conclude that this word is present as a whole, not just as part of another word) 
   uint8*   ExPage;  // For values >= 128     // Arrays by 32 bytes for indexes >= 128
   uint32   Flags;   // Second page is reserved(Windows only?), this is first of 64k allocation
  } Hdr;
 uint8 Raw[BytesPerPos];
};

SBlkHdr* First;
// The memory must be contiguous to access blocks by position index

 //uint8 RootArr[BytesPerPos];  // Each bit position is an index of 32 byte array in next 8K block    // 0 means no next char
  // Preallocated by 64k
// Pointers to leaves would be useful for extraction of actual strings     // And leave do not have to stay leaves forever  // Compare will be bad for cache!
// With termination points this strructure could be used to actually store strings
};