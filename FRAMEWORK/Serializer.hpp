

/*
StructDesc:
  uint32 NameIdx
  uint32 FieldsNum
  FieldDesc Fields[]
---
FieldDesc:
  uint32 NameIdx
  uint32 TypeNameIdx
  uint32 FieldSize  // May change by version, extend truncate if necessary.   
  uint32 FieldOffs  // Relative to struct base. May change by version, build a map by name+type
---
StructRec
  uint32 StructIdx  // Index of struct descriptor
  uint32 Count      // If this is an array or a table
  ... Raw fields
*/

// By using field maps old readers may read new formats as long as expected fields are present even if their order has changed.
// If a field is not found should it fail or be set to default? Probably should be defined for each serializable type.

// Waiting for static reflection in C++...