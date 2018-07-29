
#pragma once

#ifndef _RGBOctreeQuantizerH_
#define _RGBOctreeQuantizerH_

// TODO: Optimize memory
//------------------------------------------------------------------------------------------------------------------------------
class CRGBOTQuantizer     // OCTREEQUANTIZER
{
static const int TREEMAXDEPTH = 9;
static const int DEFAULTDEPTH = 5;
static const int TOTALCOLORS  = 256;
static const int COLRANGEMAX  = TOTALCOLORS-1;

struct RGBQUAD 
{
 unsigned char Blue;
 unsigned char Green;
 unsigned char Red;
 unsigned char Unused;
};
//------------------------------------------------------------------------------------------------------------------------------
struct COLORVALUE
{
 unsigned int ClrValArrayR[TOTALCOLORS*2];  // WORD will be enough, but with DWORD no data alignment penalty on CPU
 unsigned int ClrValArrayG[TOTALCOLORS*2];
 unsigned int ClrValArrayB[TOTALCOLORS*2];
 unsigned int *CVAMiddleR;
 unsigned int *CVAMiddleG;
 unsigned int *CVAMiddleB;
};
//------------------------------------------------------------------------------------------------------------------------------
struct FPQINDEX      // First Pass Quantize color index table
{
 unsigned int ArrayB[TOTALCOLORS];
 unsigned int ArrayG[TOTALCOLORS];
 unsigned int ArrayR[TOTALCOLORS];
};
//------------------------------------------------------------------------------------------------------------------------------
struct COLORNODE     // Represents positions of nodes with colors (For first - all colors, mapped to BASE level, then redused number of colors(nodes) on different tree levels)
{
 unsigned int NodeLevel;    // Level of node with color in octree
 unsigned int NodeIndex;    // Index of node with color in octree on 'NodeLevel' level
};
//------------------------------------------------------------------------------------------------------------------------------
struct OCTREENODE    // Represents each node in octree (24 bytes)
{
 unsigned int  ValueR;       // Weight of R
 unsigned int  ValueG;       // Weight of G
 unsigned int  ValueB;       // Weight of B
 unsigned int  CurPixNum;    // Number of pixels, mapped to this node
 unsigned int  ChiPixNum;    // Number of pixels, mapped to all child nodes of this node
 unsigned char ChiNodesMask; // Mask of used child nodes (D0-D7, Foe each node if bit is set - node is not empty)
// unsigned short  PalIndex;     // Index in palette where color value of this node was placed
// unsigned char   FDirty;       // Use 'ChiPixNum' instead; Checking each node on 'AppendColor' and 'CreateTreeNodes' also makes the code slower
};                   // When Data alignment=4, at end 3 bytes after 'ChiNodesMask' is free
//------------------------------------------------------------------------------------------------------------------------------


private:
   unsigned int MaxNodes;
   unsigned int PalColorIndex;
   COLORNODE*   ColorNodes;
   FPQINDEX     FPQTable;
   COLORVALUE   ClrValTable;

public:
   unsigned int TreeDepth;
   unsigned int ColorsInTree;
   RGBQUAD*     CurPalette;
   OCTREENODE*  NodeLevels[TREEMAXDEPTH];

private:
//==============================================================================================================================
// Count nodes with pixels
// We using a 6-LEVEL tree:
//
//               |
//              |||
//             |||||
//            |||||||
//           |||||||||
//  BASE:   ||||||||||| - Colors was mapped here
//
// If use full tree depth (8) size of tree = 48 MB  => 2097152 Nodes
// Set for each node number of mapped pixels in all his child nodes
// ColorNodes counts only BASE nodes with min 1 pixels mapped
// Tree builds DOWN -> TOP
//------------------------------------------------------------------------------------------------------------------------------
void CreateTreeNodes(void)
{
 for(unsigned int nctr=0;nctr < MaxNodes;nctr++)
  {
   if(NodeLevels[TreeDepth][nctr].CurPixNum)
    {                                             // Current node counts minimum 1 pixel
     ColorsInTree++;                              // Count of total used colors ( <= 32768)
     ColorNodes[ColorsInTree].NodeLevel    = TreeDepth;    // Depth Max (0 - (TreeDepth-1)), initially each node assumed depth is BASE
     ColorNodes[ColorsInTree].NodeIndex    = nctr;         // Base Node Index
     NodeLevels[TreeDepth][nctr].ChiPixNum = NodeLevels[TreeDepth][nctr].CurPixNum; // Set ChildPixelsCtr To CurPixelsCtr for base node with no child nodes
     // If node not empty, set for his parents on all tree levels, bit ChildNMask so all child will bound one by one
     for(int cnode=(TreeDepth-1),index=nctr;cnode >= 0;cnode--) // Build child for parent nodes from BASE node with color (Adds one child node per level on each pass)
      {
       unsigned int ChildNMask = (1 << (index & 0x00000007));  // Use index in BASE level as Low_8 index of child in parent
       index = (index / 8);                  // For each next level, nodes count = (CurLevelNodes / 8)
       NodeLevels[cnode][index].ChiPixNum    += NodeLevels[TreeDepth][nctr].CurPixNum; // Each node contains counter(Sum) of pixels in all his child nodes
       NodeLevels[cnode][index].ChiNodesMask |= ChildNMask;  // Mask of Child nodes (For each NotEmpty BASE nodes, allk parents have Bit is Set)
      }
    }
  }
 for(unsigned int cind=ColorsInTree;cind > 0;cind--)UpdateColorNodes(cind); // Set ColorNodes to Octree nodes with approriate color values
}
//------------------------------------------------------------------------------------------------------------------------------
void UpdateColorNodes(unsigned int ColorIndex)
{
 unsigned int PrvNLevel = ColorNodes[ColorIndex].NodeLevel;
 unsigned int PrvNIndex = ColorNodes[ColorIndex].NodeIndex;
 unsigned int PrvNCPixN = NodeLevels[PrvNLevel][PrvNIndex].ChiPixNum;

 while(ColorIndex <= (ColorsInTree / 2))
  {
   unsigned int NStepCInd = (ColorIndex * 2);
   if(NStepCInd < ColorsInTree)NStepCInd += (bool)((NodeLevels[(ColorNodes[NStepCInd].NodeLevel)][(ColorNodes[NStepCInd].NodeIndex)].ChiPixNum) > (NodeLevels[(ColorNodes[NStepCInd+1].NodeLevel)][(ColorNodes[NStepCInd+1].NodeIndex)].ChiPixNum));
   if(PrvNCPixN <= (NodeLevels[(ColorNodes[NStepCInd].NodeLevel)][(ColorNodes[NStepCInd].NodeIndex)].ChiPixNum))break;
   ColorNodes[ColorIndex] = ColorNodes[NStepCInd];   // Copy full SARRAY struct
   ColorIndex = NStepCInd;
  }
 ColorNodes[ColorIndex].NodeLevel = PrvNLevel;
 ColorNodes[ColorIndex].NodeIndex = PrvNIndex;
}
//------------------------------------------------------------------------------------------------------------------------------
void ReduceColorsInTree(unsigned int MaxColors)
{
 while(ColorsInTree > MaxColors)
  {
   unsigned int FCNIndex = ColorNodes[1].NodeIndex;
   unsigned int FCNLevel = (ColorNodes[1].NodeLevel - (bool)ColorNodes[1].NodeLevel);
   OCTREENODE*  SrcNode  = &NodeLevels[(ColorNodes[1].NodeLevel)][FCNIndex];
   OCTREENODE*  DstNode  = &NodeLevels[FCNLevel][(FCNIndex / 8)];

   if(DstNode->CurPixNum != 0)
    {
     ColorNodes[1] = ColorNodes[ColorsInTree];
     ColorsInTree--;
    }
     else
      {
       ColorNodes[1].NodeLevel = FCNLevel;
       ColorNodes[1].NodeIndex = (FCNIndex / 8);
      }
   DstNode->CurPixNum    += SrcNode->CurPixNum;
   DstNode->ValueR       += SrcNode->ValueR;
   DstNode->ValueG       += SrcNode->ValueG;
   DstNode->ValueB       += SrcNode->ValueB;
   DstNode->ChiNodesMask &= ~(1 << (FCNIndex & 0x07));   // (2 pow (TmpCtr & 0x07))
   UpdateColorNodes(1);
  }
}
//------------------------------------------------------------------------------------------------------------------------------
void MakePaletteFromTree(unsigned int Level, unsigned int Index)
{
 if(NodeLevels[Level][Index].ChiNodesMask != 0)  // If current node have any child nodes{node is not leaf} - Go recursion trough them first
  {
   for(int ctr=7;ctr >= 0;ctr--)  // Check each of 8 child nodes
    {
     if((NodeLevels[Level][Index].ChiNodesMask & (1 << ctr)))MakePaletteFromTree((Level+1),(ctr+(Index*8))); // Go to recursion if Bit of child node in mask is Set ((Index*8) Group of child nodes of current node on next tree level; 'ctr' used as index of child node in group)
    }
  }

 if(NodeLevels[Level][Index].CurPixNum != 0)  // If node contains color info, create a palette entry from it
  {
   CurPalette[PalColorIndex].Red   = ((NodeLevels[Level][Index].ValueR + (NodeLevels[Level][Index].CurPixNum / 2)) / NodeLevels[Level][Index].CurPixNum); // Create value of RED   from WeightR and PixCount
   CurPalette[PalColorIndex].Green = ((NodeLevels[Level][Index].ValueG + (NodeLevels[Level][Index].CurPixNum / 2)) / NodeLevels[Level][Index].CurPixNum); // Create value of GREEN from WeightG and PixCount
   CurPalette[PalColorIndex].Blue  = ((NodeLevels[Level][Index].ValueB + (NodeLevels[Level][Index].CurPixNum / 2)) / NodeLevels[Level][Index].CurPixNum); // Create value of BLUE  from WeightB and PixCount
  // NodeLevels[Level][Index].PalIndex = PalColorIndex;     // Index in palette where color value of this node was placed: This value is only for statistic
   PalColorIndex++;        // Increase palette entry counter
  }
}

public:
//------------------------------------------------------------------------------------------------------------------------------
int GetPalette(RGBQUAD *Palette, int MaxColors)
{
 if(CurPalette){for(int ctr=0,Nodes;ctr < TREEMAXDEPTH;ctr++,Nodes=(Nodes*8))memset(NodeLevels[ctr], 0, sizeof(OCTREENODE)*Nodes);}  // If Tree nodes has been already used, clear nodes; (After VirtualAlloc memory is ZERO) - Slow, maybe better use a DIRTY flag of tree node ?

 CurPalette    = Palette;
 ColorsInTree  = 0;
 PalColorIndex = 0;

 this->CreateTreeNodes();
 this->ReduceColorsInTree(MaxColors);
 this->MakePaletteFromTree(0,0);
 return PalColorIndex;
}
//------------------------------------------------------------------------------------------------------------------------------
// Because we do not use full depth octree and do not store colors in nodes by
// their RGB indexes, we cannot search nearest colors in tree by requested RGB
//
int FindNearestColor(unsigned char Red, unsigned char Green, unsigned char Blue)
{
 int Index = -1;
 unsigned int MaxValue = -1;  // Low Color trim data loss constant

 for(unsigned int ctr=0;ctr < ColorsInTree;ctr++)
  {
   unsigned int Value = (ClrValTable.CVAMiddleR[ (CurPalette[ctr].Red-Red) ])+(ClrValTable.CVAMiddleG[ (CurPalette[ctr].Green-Green) ])+(ClrValTable.CVAMiddleB[ (CurPalette[ctr].Blue-Blue) ]);
   if(Value < MaxValue){MaxValue = Value; Index = ctr;}
  }
 return Index;
}
//------------------------------------------------------------------------------------------------------------------------------
// If Depth = 0 - Releases all used memory
// VirtualAlloc is faster than HeapAlloc ???
// SetOctreeDepth mainly affects to First Pass Quantization
//
void SetOctreeDepth(int Depth)
{
 int Nodes = 0;
 Depth     = (Depth > TREEMAXDEPTH)?TREEMAXDEPTH:Depth;
 if((unsigned int)Depth == (TreeDepth+1))return;      // Do not change anything !!!
 MaxNodes  = 0;

 for(int ctr=0,nctr=1;ctr < TREEMAXDEPTH;ctr++,nctr*=8)
  {
   if(ctr  <  Depth)Nodes = nctr;                        // TODO: Optimize - rearrange
   if((ctr <  Depth)&&(NodeLevels[ctr] == NULL)){NodeLevels[ctr] = new OCTREENODE[nctr]();}   // Specifying '()' for POD allocation forces a compiler to set that memory to 0 (C++ spec); MSVC invokes memset for this when SPEED optimization is enabled or uses 'rep stos' without it. Also with SPEED optimization it uses malloc instead of global 'new'        // (OCTREENODE*)VirtualAlloc(NULL,(sizeof(OCTREENODE)*nctr),MEM_COMMIT,PAGE_READWRITE);
   if((ctr >= Depth)&&(NodeLevels[ctr] != NULL)){delete[] NodeLevels[ctr]; NodeLevels[ctr] = NULL;}    // VirtualFree(NodeLevels[ctr],NULL,MEM_RELEASE);
  }

 TreeDepth = Depth;
 if(ColorNodes){delete[] ColorNodes; ColorNodes = NULL;}   //  VirtualFree(ColorNodes,NULL,MEM_RELEASE);
 if(!TreeDepth)return;       // Exit, if Depth = 0

 TreeDepth--;
 MaxNodes   = Nodes;
 ColorNodes = new COLORNODE[Nodes+1]();        //(COLORNODE*)VirtualAlloc(NULL,(sizeof(COLORNODE)*(Nodes+1)),MEM_COMMIT,PAGE_READWRITE);

 // Create a first pass color quantization table
 // Makes adding colors to Octree much more faster (Nodes indexing by combined colors)
 // For max (16777216) colors if Depth = 9
 // Mask Trimmed to tree depth
 // This Is BEST WAY ???
 //
 // ArrayB[Counter] = Counter => D7-D6-D5-D4-D3-D2-D1-D0 => D7-0-0-D6-0-0-D5-0-0-D4-0-0-D3-0-0-D2-0-0-D1-0-0-D0
 // ArrayG[Counter] = Counter => D7-D6-D5-D4-D3-D2-D1-D0 => D7-0-0-D6-0-0-D5-0-0-D4-0-0-D3-0-0-D2-0-0-D1-0-0-D0-0
 // ArrayR[Counter] = Counter => D7-D6-D5-D4-D3-D2-D1-D0 => D7-0-0-D6-0-0-D5-0-0-D4-0-0-D3-0-0-D2-0-0-D1-0-0-D0-0-0
 //
 for(unsigned int Counter=0,Mask=0;Counter < 256;Counter++)
  {
   Mask = (((Counter & 0x80) << 14)|((Counter & 0x40)<<12)|((Counter & 0x20)<<10)|((Counter & 0x10)<<8)|((Counter & 0x08)<<6)|((Counter & 0x04)<<4)|((Counter & 0x02)<<2)|(Counter & 0x01));
   Mask = (Mask >> ((TREEMAXDEPTH - Depth) * 3)); // Trim Max Colors to Tree Depth
   FPQTable.ArrayR[Counter] = (FPQTable.ArrayG[Counter] = (FPQTable.ArrayB[Counter] = Mask) << 1) << 1;
  }
}
//------------------------------------------------------------------------------------------------------------------------------
void AppendColor(unsigned char Red, unsigned char Green, unsigned char Blue)
{
 unsigned int NodeIndex = (FPQTable.ArrayB[ Blue ] + FPQTable.ArrayG[ Green ] + FPQTable.ArrayR[ Red ]);  // Get quantized index of node, some different colors will be mapped as same; NodeIndex will be optimized to REGISTER variable
 NodeLevels[TreeDepth][NodeIndex].ValueR   += Red;    // Update weight of RED
 NodeLevels[TreeDepth][NodeIndex].ValueG   += Green;  // Update weight of GREEN
 NodeLevels[TreeDepth][NodeIndex].ValueB   += Blue;   // Update weight of BLUE
 NodeLevels[TreeDepth][NodeIndex].CurPixNum++;        // Update mapped pixels counter for node
}
//------------------------------------------------------------------------------------------------------------------------------
CRGBOTQuantizer(void)
{
 CurPalette    = NULL;
 MaxNodes      = 0;
 ColorNodes    = NULL;
 TreeDepth     = 0;
 ColorsInTree  = 0;
 PalColorIndex = 0;

 for(int ctr=0;ctr < TREEMAXDEPTH;ctr++)NodeLevels[ctr] = NULL;
 for(int ctr=-COLRANGEMAX,index=0;ctr <= COLRANGEMAX;ctr++,index++) // Precalculate ccolor values (fi = 30*(Ri-R0)2+59*(Gi-G0)2+11*(Bi-B0)2)   // Range: -255 <> +255
  {
   ClrValTable.ClrValArrayR[index] = (ctr*ctr)*32;    // 30 // Numbers are human eye color perception differences
   ClrValTable.ClrValArrayG[index] = (ctr*ctr)*64;    // 59
   ClrValTable.ClrValArrayB[index] = (ctr*ctr)*16;    // 11
  }
 ClrValTable.CVAMiddleR = &ClrValTable.ClrValArrayR[COLRANGEMAX];  // Or 256(Second half)???????
 ClrValTable.CVAMiddleG = &ClrValTable.ClrValArrayG[COLRANGEMAX];
 ClrValTable.CVAMiddleB = &ClrValTable.ClrValArrayB[COLRANGEMAX];
 this->SetOctreeDepth(DEFAULTDEPTH);
}
//------------------------------------------------------------------------------------------------------------------------------
~CRGBOTQuantizer()
{
 this->SetOctreeDepth(0);
}
//------------------------------------------------------------------------------------------------------------------------------
};
//------------------------------------------------------------------------------------------------------------------------------
#endif
