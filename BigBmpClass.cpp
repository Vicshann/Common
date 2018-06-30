//---------------------------------------------------------------------------


#pragma hdrstop

#include "BigBmpClass.h"
//---------------------------------------------------------------------------

#pragma package(smart_init)
//===========================================================================
//
//                             P U B L I C
//
//===========================================================================
_stdcall CBIGBITMAP::CBIGBITMAP(void)
{
 SYSTEM_INFO sinf;

 GetSystemInfo(&sinf);
 MapPageSize   = sinf.dwAllocationGranularity;
 DefHeap       = GetProcessHeap();
 hFileBmp      = INVALID_HANDLE_VALUE;
 BmpFExtended  = false;
 CallBackProc  = NULL;
 RowIndexBuff  = NULL;
 ResAverBufPtr = NULL;
 MemBmpBuffer  = NULL;
 DecRowBuffer  = NULL;
 ReadedRowsBuf = NULL;
 MappedSection = NULL;
 BmpInfoProcs  = NULL;
 BmpInfoOrig   = NULL;
 IndexBuffer   = NULL;
 BitmapInfo    = NULL;
 hMappedBmp    = NULL;
 DstWidth      = 0;
 DstHeight     = 0;
 ImageType     = 0;
 InitializeCriticalSection(&CSecRowsMap);
}
//===========================================================================
_stdcall CBIGBITMAP::~CBIGBITMAP()
{
 FreeBitmap();
 DeleteCriticalSection(&CSecRowsMap);
}
//===========================================================================
void  _stdcall CBIGBITMAP::SetCallBack(PROGRESSCALLBACK CallBack)
{
 __GUARDNEEDED(ProcGuardEnter());
 CallBackProc = CallBack;
 __GUARDNEEDED(ProcGuardExit());
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::SetView(HDC TargetDC, RECT *DstRect, RECT *SrcRect)
{
 DWORD BmpBufSize;

 __GUARDNEEDED(ProcGuardEnter());
 if(TargetDC){DstWndDC = TargetDC;if(!ImageType){__GUARDNEEDED(ProcGuardExit());return 0;}}
 if(!ImageType){__GUARDNEEDED(ProcGuardExit());return 1;}
 if(SrcRect)
  {        // Safe RECT change
   ViewRect.top    = (SrcRect->top  > BmpHeight)?(0):SrcRect->top;
   ViewRect.left   = (SrcRect->left > BmpWidth)?(0):SrcRect->left;
   ViewRect.right  = ((ViewRect.left+SrcRect->right) > BmpWidth)?(BmpWidth-ViewRect.left):SrcRect->right;   // This is right ?  (Need only for resized view)
   ViewRect.bottom = ((ViewRect.top+SrcRect->bottom) > BmpHeight)?(BmpHeight-ViewRect.top):SrcRect->bottom; // This is right ?  (Need only for resized view)
  }
 if(DstRect && !EqualRect(&TargetRect,DstRect))
  {
   // Recreate View Buffer
   CopyRect(&TargetRect,DstRect);
   if(MemBmpBuffer)VirtualFree(MemBmpBuffer,NULL,MEM_RELEASE);
   // Get width and height of destination rect
   DstWidth     = (TargetRect.right-TargetRect.left);
   DstHeight    = (TargetRect.bottom-TargetRect.top);
   BmpBufSize   = (DstWidth*sizeof(DWORD));                          // Assume as used 32bpp - Max
   BmpBufSize   = _ALIGN_FRWRD(BmpBufSize,sizeof(DWORD))*DstHeight;  // Size of window in bytes
   MemBmpBuffer = VirtualAlloc(NULL,BmpBufSize,MEM_COMMIT,PAGE_READWRITE);
   RowsToBuffer = (DstHeight/ROWBUFDIV);                             // This is Fine ????
   SrcWidth     = ((DstWidth  > BmpWidth)&&!ResizePixels)?BmpWidth:DstWidth;
   SrcHeight    = ((DstHeight > BmpHeight)&&!ResizePixels)?BmpHeight:DstHeight;
   DRowWidth    = ((ViewRect.left+SrcWidth) > BmpWidth)?(BmpWidth-ViewRect.left):SrcWidth;
   DstRowSize   = (ResizePixels)?(SrcWidth):DRowWidth;
   DstRowSize   = (DstBytesPP*DstRowSize);
   ADstRowSize  = _ALIGN_FRWRD(DstRowSize,sizeof(DWORD));
   BmpViewInfoHdr->bmiHeader.biWidth  = SrcWidth;                    // For currently selected header
   BmpViewInfoHdr->bmiHeader.biHeight = SrcHeight*(-1);
  }
 BmpBufSize = UpdateBitmapBuffer();
 __GUARDNEEDED(ProcGuardExit());
 return BmpBufSize;     // Error code from "UpdateBitmapBuffer"
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::SetProcessing(bool Resizing, bool Grayscale, bool Sharpen, bool Blur, int ResQuality, int Contrast, int Brightness)
{
 if(!ImageType)return 1;
 __GUARDNEEDED(ProcGuardEnter());
 ResizeQuality = ResQuality;
 ProcessPixels = (Blur||Sharpen||Grayscale||Contrast||Brightness);
 ResizePixels  = Resizing;
 FGrayscale    = Grayscale;
 FBrightness   = Brightness;
 FContrast     = Contrast;
 FSharpen      = Sharpen;
 FBlur         = Blur;
 if(Resizing||(ProcessPixels&&(BytesInPixel<3)))
  {
   BmpInfoProcs->bmiHeader.biWidth  = BmpViewInfoHdr->bmiHeader.biWidth;
   BmpInfoProcs->bmiHeader.biHeight = BmpViewInfoHdr->bmiHeader.biHeight;
   BmpViewInfoHdr = BmpInfoProcs;
   DstBytesPP     = 4;
  }
   else
    {
     BmpInfoOrig->bmiHeader.biWidth  = BmpViewInfoHdr->bmiHeader.biWidth;
     BmpInfoOrig->bmiHeader.biHeight = BmpViewInfoHdr->bmiHeader.biHeight;
     BmpViewInfoHdr = BmpInfoOrig;
     DstBytesPP     = BytesInPixel;
	}

 if(ResizePixels && !ResAverBufPtr){if(!(ResAverBufPtr = (RESIZEAVERBUF*)VirtualAlloc(NULL,(BaseBmpWidth*sizeof(RESIZEAVERBUF)),MEM_COMMIT,PAGE_READWRITE))){__GUARDNEEDED(ProcGuardExit());return 1;}}  // Allocate color averaging buffer (used for fast averaging row by row)
 SrcWidth     = ((DstWidth  > BmpWidth)&&!ResizePixels)?BmpWidth:DstWidth;
 SrcHeight    = ((DstHeight > BmpHeight)&&!ResizePixels)?BmpHeight:DstHeight;
 DRowWidth    = ((ViewRect.left+SrcWidth) > BmpWidth)?(BmpWidth-ViewRect.left):SrcWidth;
 DstRowSize   = (ResizePixels)?(SrcWidth):DRowWidth;
 DstRowSize   = (DstBytesPP*DstRowSize);
 ADstRowSize  = _ALIGN_FRWRD(DstRowSize,sizeof(DWORD));
 BmpViewInfoHdr->bmiHeader.biWidth  = SrcWidth;                    // For currently selected header
 BmpViewInfoHdr->bmiHeader.biHeight = SrcHeight*(-1);

 __GUARDNEEDED(ProcGuardExit());
 return 0;
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::SelectImageIndex(int Index)
{
 int Width;
 int Height;
 int Offset;

 __GUARDNEEDED(ProcGuardEnter());
 if((!ImageType)||(Index >= BmpAHdr.LevelsCount)||(Index == BAGImageIndex)){__GUARDNEEDED(ProcGuardExit());return 1;}
 Width           = BmpAHdr.BaseWidth;
 Height          = BmpAHdr.BaseHeight;
 Offset          = 0;
 for(int ctr=0;ctr<Index;ctr++){Offset+=Height;Height=(Height/2);Width=(Width/2);}
 RowIndexBuff    = &((DWORD*)IndexBuffer)[Offset];
 UsedRowSize     = (Width*BytesInPixel);                 // Number of used bytes in row
 AlignedRowSize  = _ALIGN_FRWRD(UsedRowSize,sizeof(DWORD));
 BmpHeight       = Height;
 BmpWidth        = Width;
 PrvRowIndex     = -1;
 PrvPixCount     = -1;
 PrvPixOffset    = -1;
 FirstRowLoaded  = -1;
 TotalRowsLoaded = -1;
 SrcWidth        = ((DstWidth  > BmpWidth)&&!ResizePixels)?BmpWidth:DstWidth;
 SrcHeight       = ((DstHeight > BmpHeight)&&!ResizePixels)?BmpHeight:DstHeight;

 if(Index > BAGImageIndex)
  {                      // Keep position in smaller image  (Safe shrink)
   Offset = (1 << (Index - BAGImageIndex)); // 2 pow (Index - BAGImageIndex)
   ViewRect.top  = (SrcHeight  < BmpHeight)?(ViewRect.top / Offset):0;
   ViewRect.left = (SrcWidth   < BmpWidth)?(ViewRect.left / Offset):0;
   if((ViewRect.top+SrcHeight) > BmpHeight)ViewRect.top = (BmpHeight-SrcHeight);
   if((ViewRect.left+SrcWidth) > BmpWidth)ViewRect.left = (BmpWidth-SrcWidth);
  }
   else
    {    // Keep position in bigger image
     Offset        = (Index - BAGImageIndex)*2;
     ViewRect.top  = (ViewRect.top  * Offset);
     ViewRect.left = (ViewRect.left * Offset);
    }

 DRowWidth       = ((ViewRect.left+SrcWidth) > BmpWidth)?(BmpWidth-ViewRect.left):SrcWidth;
 DstRowSize      = (ResizePixels)?(SrcWidth):DRowWidth;
 DstRowSize      = (DstBytesPP*DstRowSize);
 ADstRowSize     = _ALIGN_FRWRD(DstRowSize,sizeof(DWORD));
 BAGImageIndex   = Index;
 BmpViewInfoHdr->bmiHeader.biWidth  = SrcWidth;            // For currently selected header
 BmpViewInfoHdr->bmiHeader.biHeight = SrcHeight*(-1);
 __GUARDNEEDED(ProcGuardExit());
 return 0;
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::SetDrawExParams(int PosX, int PosY, int DestWidth, int DestHeight, DWORD RasterOp, DWORD TranspClr, DWORD TranspMode)
{
 if(!ImageType)return 1;
 __GUARDNEEDED(ProcGuardEnter());

 DExPosX       = PosX;
 DExPosY       = PosY;
 DExWidth      = DestWidth;
 DExHeight     = DestHeight;
 DExRasterOp   = RasterOp;
 DExTranspMode = TranspMode;
 DExTranspClr  = (TranspMode > 1)?(TrpModeTwoClr):(TranspClr);

 __GUARDNEEDED(ProcGuardExit());
 return 0;
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::GetBitmapInfo(BIGBMPINFO *BigInfo)
{
 if(!ImageType){ZeroMemory(BigInfo,sizeof(BIGBMPINFO));return 1;}
 __GUARDNEEDED(ProcGuardEnter());
 CopyRect(&BigInfo->ImageRect,&ViewRect);
 CopyRect(&BigInfo->DcRect,&TargetRect);
 BigInfo->ExTranspMode = DExTranspMode;
 BigInfo->ExTranspClr  = DExTranspClr;
 BigInfo->ExRop        = DExRasterOp;
 BigInfo->ExPosX	   = DExPosX;
 BigInfo->ExPosY	   = DExPosY;
 BigInfo->ExWidth      = DExWidth;
 BigInfo->ExHeight     = DExHeight;
 BigInfo->Width        = BmpWidth;
 BigInfo->Height       = BmpHeight;
 BigInfo->BitsPP       = (BytesInPixel*8);
 BigInfo->Images       = ImagesNumber;
 BigInfo->IType        = ImageType;
 BigInfo->CurImage     = BAGImageIndex;
 BigInfo->Contrast     = FContrast;
 BigInfo->Brightness   = FBrightness;
 BigInfo->ResQuality   = ResizeQuality;
 BigInfo->Resizing     = ResizePixels;
 BigInfo->Grayscale    = FGrayscale;
 BigInfo->Sharpen      = FSharpen;
 BigInfo->Blur         = FBlur;
 __GUARDNEEDED(ProcGuardExit());
 return 0;
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::ReDraw(void)
{
 // SetDIBitsToDevice flips rows of BOTTOMTOP bitmaps to normal TOPBOTTOM
 __GUARDNEEDED(ProcGuardEnter());
 if(!MemBmpBuffer || !ImageType){__GUARDNEEDED(ProcGuardExit());return 1;}
 if(!SetDIBitsToDevice(DstWndDC,
					   TargetRect.left,   // X pos of SrcLeftCorner in Target DC
					   TargetRect.top,    // Y pos of SrcLeftCorner in Target DC
					   SrcWidth,          // Trim to target DC width, width may be greater - it is safe
					   SrcHeight,         // (For correct target DC pos, must be equal real number of used scanlines)if greater real image height, difference offsets from top of target window
					   0,
					   0,                 // Y offset controlled by scanline parameters
					   0,
					   SrcHeight,         // Draws only specified number of lines(TOPDOWN trims down,BOTTOMUP trims up)
					   MemBmpBuffer,
					   BmpViewInfoHdr,
					   DIB_RGB_COLORS)){__GUARDNEEDED(ProcGuardExit());return 2;}
 __GUARDNEEDED(ProcGuardExit());
 return 0;
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::ReDrawEx(void)
{
 // SetDIBitsToDevice flips rows of BOTTOMTOP bitmaps to normal TOPBOTTOM
 __GUARDNEEDED(ProcGuardEnter());
 if(!MemBmpBuffer || !ImageType){__GUARDNEEDED(ProcGuardExit());return 1;}
 if(DExTranspMode)
  {
   if(!TranspStrDIBits(DstWndDC,
					 DExPosX,   // X pos of SrcLeftCorner in Target DC
					 DExPosY,    // Y pos of SrcLeftCorner in Target DC
					 DExWidth,
					 DExHeight,
					 0,
					 0,
					 SrcWidth,
					 SrcHeight,
					 MemBmpBuffer,
					 BmpViewInfoHdr,
					 ((DExTranspMode > 1)?(TrpModeTwoClr):(DExTranspClr)))){__GUARDNEEDED(ProcGuardExit());return GetLastError();}
  }
   else
	{
	 if(!StretchDIBits(DstWndDC,
					   DExPosX,   // X pos of SrcLeftCorner in Target DC
					   DExPosY,    // Y pos of SrcLeftCorner in Target DC
					   DExWidth,
					   DExHeight,
					   0,
					   0,
					   SrcWidth,
					   SrcHeight,
					   MemBmpBuffer,
					   BmpViewInfoHdr,
					   DIB_RGB_COLORS,
					   DExRasterOp)){__GUARDNEEDED(ProcGuardExit());return GetLastError();}
	}
 __GUARDNEEDED(ProcGuardExit());
 return 0;
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::FreeBitmap(void)
{
 __GUARDNEEDED(ProcGuardEnter());
 if(MappedSection){UnmapViewOfFile(MappedSection);MappedSection=NULL;ReadedRowsBuf=NULL;}
 if(hMappedBmp){CloseHandle(hMappedBmp);hMappedBmp=NULL;}
 if(hFileBmp != INVALID_HANDLE_VALUE){if(BmpFExtended){SetFilePointer(hFileBmp,-((int)(sizeof(DWORD))),NULL,FILE_END);SetEndOfFile(hFileBmp);BmpFExtended=false;}CloseHandle(hFileBmp);hFileBmp = INVALID_HANDLE_VALUE;} // Remove if needed exess bytes from file
 if(BitmapInfo){HeapFree(DefHeap, NULL, BitmapInfo);BitmapInfo=NULL;}
 if(BmpInfoProcs){HeapFree(DefHeap, NULL, BmpInfoProcs);BmpInfoProcs=NULL;}
 if(BmpInfoOrig){HeapFree(DefHeap, NULL, BmpInfoOrig);BmpInfoOrig=NULL;}
 if(ResAverBufPtr){VirtualFree(ResAverBufPtr,NULL,MEM_RELEASE);ResAverBufPtr=NULL;}
 if(MemBmpBuffer){VirtualFree(MemBmpBuffer,NULL,MEM_RELEASE);MemBmpBuffer=NULL;}
 if(DecRowBuffer){VirtualFree(DecRowBuffer,NULL,MEM_RELEASE);DecRowBuffer=NULL;}
 if(IndexBuffer){VirtualFree(IndexBuffer,NULL,MEM_RELEASE);IndexBuffer=NULL;RowIndexBuff=NULL;}
 ZeroMemory(&ViewRect,sizeof(RECT));
 ZeroMemory(&TargetRect,sizeof(RECT));                                         
 ZeroMemory(&BitmapInfo,sizeof(BITMAPINFO));
 ZeroMemory(&BmpAHdr,sizeof(BMPARCHIVEHDR));
 FirstRowLoaded   = -1;
 TotalRowsLoaded  = -1;
 BAGImageIndex    = -1;
 PrvRowIndex      = -1;
 PrvPixCount      = -1;
 PrvPixOffset     = -1;
 DExWidth         = 0;
 DExHeight        = 0;
 DExTranspClr     = 0;
 DExTranspMode    = 0;
 ImagesNumber     = 0;
 RowsToBuffer     = 0;
 RowsBufSize      = 0;
 ImageType        = 0;
 FContrast        = 0;
 FBrightness      = 0;
 FGrayscale       = false;
 FSharpen         = false;
 FBlur            = false;
 BmpFExtended     = false;
 DExRasterOp      = SRCCOPY;        // Default ROP

 __GUARDNEEDED(ProcGuardExit());
 return 0;
}
//===========================================================================
//            Set better file reading and mapping attrebutes
//
DWORD _stdcall CBIGBITMAP::LoadBitmap(LPSTR FileName)
{
 BITMAPFILEHEADER BmpFHdr;
 BMSFHEADER       *BmsFHdr;
 DWORD            FileBytesUsed = 0;
 DWORD            HdrSize;
 DWORD            ErrCode = 0;
 DWORD            Result;

 FreeBitmap();
 __GUARDNEEDED(ProcGuardEnter());
 if((hFileBmp = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL))==INVALID_HANDLE_VALUE){ErrCode=1;goto RExit;}
 SetFilePointer(hFileBmp,0,NULL,FILE_BEGIN);
 ReadFile(hFileBmp,&BmpFHdr,sizeof(BITMAPFILEHEADER),&Result,NULL);
 if(Result != sizeof(BITMAPFILEHEADER)){ErrCode=2;goto RExit;}

 switch(BmpFHdr.bfType)
  {
   case FLAG_BM:    // Ordinary BMP file
    {
	 HdrSize    = BmpFHdr.bfOffBits;   // Do not exclude size of BITMAPFILEHEADER for safe
     if(!(BitmapInfo = (BITMAPINFO*)HeapAlloc(DefHeap, HEAP_ZERO_MEMORY, HdrSize))){ErrCode=3;goto RExit;}
     ReadFile(hFileBmp,BitmapInfo,(HdrSize-sizeof(BITMAPFILEHEADER)),&Result,NULL);   // Read bitmap parameters
     if(Result != (HdrSize-sizeof(BITMAPFILEHEADER))){ErrCode=4;goto RExit;}
     if((BitmapInfo->bmiHeader.biCompression != BI_RGB)&&(BitmapInfo->bmiHeader.biCompression != BI_BITFIELDS)||(BitmapInfo->bmiHeader.biBitCount < 8)){ErrCode=5;goto RExit;} // Format not supported ( Rle, Jpeg, ...)

	 FileBytesUsed = (BitmapInfo->bmiHeader.biWidth*(BitmapInfo->bmiHeader.biBitCount/8));
	 FileBytesUsed = _ALIGN_FRWRD(FileBytesUsed,sizeof(DWORD));               // For safe buffer size detection
	 FileBytesUsed = ((int)BitmapInfo->bmiHeader.biHeight < 0)?(FileBytesUsed * (-((int)BitmapInfo->bmiHeader.biHeight))):(FileBytesUsed * BitmapInfo->bmiHeader.biHeight);
	 FileBytesUsed = (FileBytesUsed + HdrSize);

	 ImagesNumber  = 1;
	 RowsOffset    = BmpFHdr.bfOffBits;
	 ImageType     = FLAG_BM;
	 break;
	}
   case FLAG_BS:  // My Modified RLE with indexed rows file
	{
     BmsFHdr    = ((BMSFHEADER*)&BmpFHdr);
     if(!BmsFHdr->IndexOffset){ErrCode=6;goto RExit;}  // Supported only indexed RLE
     HdrSize    = BmsFHdr->IndexOffset;  // Indexes always after header;  Do not exclude size of BMSFHEADER for safe
	 if(!(BitmapInfo = (BITMAPINFO*)HeapAlloc(DefHeap, HEAP_ZERO_MEMORY, HdrSize))){ErrCode=7;goto RExit;}
	 ReadFile(hFileBmp,BitmapInfo,(HdrSize-sizeof(BMSFHEADER)),&Result,NULL);   // Read bitmap parameters
     if(Result != (HdrSize-sizeof(BMSFHEADER))){ErrCode=8;goto RExit;}
     if((BitmapInfo->bmiHeader.biCompression != BI_IRC)||(BitmapInfo->bmiHeader.biBitCount < 8)){ErrCode=9;goto RExit;} // Format not supported ( Not indexed rows, Pixel < 1Byte)

     RowsOffset   = (BmsFHdr->PixelsOffset-BmsFHdr->IndexOffset);  // Use difference, because do not easy get a bitmap height (may be nagative)
     if(!(IndexBuffer = VirtualAlloc(NULL,(RowsOffset+sizeof(DWORD)),MEM_COMMIT,PAGE_READWRITE))){ErrCode=10;goto RExit;} // Buffer for row indexes
     RowIndexBuff = IndexBuffer;
	 SetFilePointer(hFileBmp,BmsFHdr->IndexOffset,NULL,FILE_BEGIN);
     ReadFile(hFileBmp,RowIndexBuff,RowsOffset,&Result,NULL);  // Read row indexes into buffer
     if(Result != RowsOffset){ErrCode=11;goto RExit;}

	 FileBytesUsed = GetComprRowWidthF(hFileBmp,((DWORD*)RowIndexBuff)[(RowsOffset/sizeof(DWORD))-1],BmsFHdr->RleFlag,BitmapInfo->bmiHeader.biWidth,(BitmapInfo->bmiHeader.biBitCount/8));
	 ((DWORD*)RowIndexBuff)[(RowsOffset/sizeof(DWORD))] = (((DWORD*)RowIndexBuff)[(RowsOffset/sizeof(DWORD))-1] + FileBytesUsed); // For last row size detection
	 FileBytesUsed = ((DWORD*)RowIndexBuff)[(RowsOffset/sizeof(DWORD))];

	 RowsOffset    = (BitmapInfo->bmiHeader.biWidth*(BitmapInfo->bmiHeader.biBitCount/8));
	 RowsOffset    = _ALIGN_FRWRD(RowsOffset,sizeof(DWORD));
     if(!(DecRowBuffer = VirtualAlloc(NULL,RowsOffset,MEM_COMMIT,PAGE_READWRITE))){ErrCode=12;goto RExit;} // Buffer for row decompression
     BitmapInfo->bmiHeader.biCompression = BI_RGB;  // Normal RGB format - For WinAPI

	 ImagesNumber  = 1;
	 RleFlagCode   = BmsFHdr->RleFlag;
	 RowsOffset    = BmsFHdr->PixelsOffset;
	 ImageType     = FLAG_BS;
     break;
    }
   case FLAG_BG:  // My Modified RLE with indexed rows pow2 images group file
    {
     if(((BMPARCHIVEHDR*)&BmpFHdr)->FlagBGAR != FLAG_BAGR){ErrCode=13;goto RExit;}  // Unknow file type
	 SetFilePointer(hFileBmp,0,NULL,FILE_BEGIN);
	 ReadFile(hFileBmp,&BmpAHdr,sizeof(BMPARCHIVEHDR),&Result,NULL);   // Read bitmap parameters
     if(Result != sizeof(BMPARCHIVEHDR)){ErrCode=14;goto RExit;}
     HdrSize = (BmpAHdr.PaletteSize+sizeof(BITMAPINFO));
     if(!(BitmapInfo = (BITMAPINFO*)HeapAlloc(DefHeap, HEAP_ZERO_MEMORY, HdrSize))){ErrCode=15;goto RExit;}
     if(BmpAHdr.PaletteSize)
      {
       ReadFile(hFileBmp,&BitmapInfo->bmiColors,BmpAHdr.PaletteSize,&Result,NULL);   // Read bitmap pallette
       if(Result != BmpAHdr.PaletteSize){ErrCode=16;goto RExit;}
      }

	 if(!(IndexBuffer = VirtualAlloc(NULL,(BmpAHdr.IndexesSize+sizeof(DWORD)),MEM_COMMIT,PAGE_READWRITE))){ErrCode=17;goto RExit;} // Buffer for row indexes
	 RowIndexBuff = IndexBuffer;
	 ReadFile(hFileBmp,RowIndexBuff,BmpAHdr.IndexesSize,&Result,NULL);  // Read row indexes into buffer
     if(Result   != BmpAHdr.IndexesSize){ErrCode=18;goto RExit;}

	 FileBytesUsed = (BmpAHdr.BaseWidth / (1 << BmpAHdr.LevelsCount));        
	 FileBytesUsed = GetComprRowWidthF(hFileBmp,((DWORD*)RowIndexBuff)[(BmpAHdr.IndexesSize/sizeof(DWORD))-1],BmpAHdr.RlePFlag,FileBytesUsed,BmpAHdr.BytesPerPixel);
	 ((DWORD*)RowIndexBuff)[(BmpAHdr.IndexesSize/sizeof(DWORD))] = (((DWORD*)RowIndexBuff)[(BmpAHdr.IndexesSize/sizeof(DWORD))-1] + FileBytesUsed); // For last row size detection
	 FileBytesUsed = ((DWORD*)RowIndexBuff)[(BmpAHdr.IndexesSize/sizeof(DWORD))];

	 RowsOffset    = (BmpAHdr.BaseWidth*BmpAHdr.BytesPerPixel);
	 RowsOffset    = _ALIGN_FRWRD(RowsOffset,sizeof(DWORD));
     if(!(DecRowBuffer = VirtualAlloc(NULL,RowsOffset,MEM_COMMIT,PAGE_READWRITE))){ErrCode=19;goto RExit;} // Buffer for row decompression

     BitmapInfo->bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
	 BitmapInfo->bmiHeader.biWidth         = BmpAHdr.BaseWidth;
     BitmapInfo->bmiHeader.biHeight        = BmpAHdr.BaseHeight*(-1);   // Always TOPDOWN
	 BitmapInfo->bmiHeader.biPlanes        = 1;
	 BitmapInfo->bmiHeader.biBitCount      = (BmpAHdr.BytesPerPixel*8);
     BitmapInfo->bmiHeader.biCompression   = BI_RGB;
     BitmapInfo->bmiHeader.biSizeImage     = (RowsOffset*BmpAHdr.BaseHeight);
     BitmapInfo->bmiHeader.biXPelsPerMeter = 0;
     BitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	 BitmapInfo->bmiHeader.biClrUsed       = 0;
     BitmapInfo->bmiHeader.biClrImportant  = 0;

	 ImagesNumber = BmpAHdr.LevelsCount;
	 RleFlagCode  = BmpAHdr.RlePFlag;
	 RowsOffset   = (sizeof(BMPARCHIVEHDR)+BmpAHdr.PaletteSize+BmpAHdr.IndexesSize);
	 ImageType    = FLAG_BG;
	 break;
    }
   default : {ErrCode=20;goto RExit;}  // Unknow file type
  }

 // Determine rows order and true bitmap height
 if((int)BitmapInfo->bmiHeader.biHeight < 0){BmpHeight = -((int)BitmapInfo->bmiHeader.biHeight);BmpTopDown=true;}
   else {BmpHeight = BitmapInfo->bmiHeader.biHeight;BmpTopDown=false;}

 BmpWidth          = BitmapInfo->bmiHeader.biWidth;
 BaseBmpWidth      = BmpWidth;
 BytesInPixel      = (BitmapInfo->bmiHeader.biBitCount/8);
 UsedRowSize       = (BmpWidth*BytesInPixel); // Number of used bytes in row
 SizeMask          = (0xFFFFFFFF >> ((sizeof(DWORD)-BytesInPixel)*8));
 FlagCode          = (RleFlagCode & SizeMask);
 AlignedRowSize    = _ALIGN_FRWRD(UsedRowSize,sizeof(DWORD));
 if(!(BmpInfoProcs = (BITMAPINFO*)HeapAlloc(DefHeap, HEAP_ZERO_MEMORY, sizeof(BITMAPINFO)))){ErrCode=21;goto RExit;}  // **********  SIZE ???
 if(!(BmpInfoOrig  = (BITMAPINFO*)HeapAlloc(DefHeap, HEAP_ZERO_MEMORY, HdrSize))){ErrCode=22;goto RExit;}
 BmpViewInfoHdr    = BmpInfoOrig;
 ProcessPixels     = false;
 ResizePixels      = false;
 ResizeQuality     = 0;
 BAGImageIndex     = 0;
 DExWidth          = 100;    //  ???? Use Predefined ??
 DExHeight         = 100;    //  ???? Use Predefined ??
 DstBytesPP        = BytesInPixel;
 CopyMemory(BmpInfoOrig,BitmapInfo,HdrSize);      // Original header with palette
 BmpInfoOrig->bmiHeader.biSizeImage     = 0;      // Do Not used in BI_RGB
 BmpInfoOrig->bmiHeader.biCompression   = BI_RGB;
 CopyMemory(BmpInfoProcs,BmpInfoOrig,sizeof(BITMAPINFO)); // ************ SIZE ???
 BmpInfoProcs->bmiHeader.biClrUsed      = 0;
 BmpInfoProcs->bmiHeader.biClrImportant = 0;
 BmpInfoProcs->bmiHeader.biBitCount     = 32;     // Processing uses 32 Bpp color - faster

 if(GetFileSize(hFileBmp,NULL) < (FileBytesUsed+sizeof(DWORD)))
  {
   SetFilePointer(hFileBmp,sizeof(DWORD),NULL,FILE_END);   // Add mapping buffer to end of file
   BmpFExtended = SetEndOfFile(hFileBmp);  // Some functions reas mapped section by DWORDs - Results AV if read DWORD in middle of section and unallocated address scpace
  }
 if(!(hMappedBmp = CreateFileMapping(hFileBmp,NULL,PAGE_READWRITE,0,0,NULL)))ErrCode=23;
 TrpModeTwoClr   = GetBitmapPixel(0,0);  // Most TopLeft pixel as transparent color for mode 2

RExit:
 __GUARDNEEDED(ProcGuardExit());
 if(ErrCode)FreeBitmap();
 return ErrCode;
}
//===========================================================================
//    For now, DstRect used only for width and height aquring
//
DWORD _stdcall CBIGBITMAP::SaveBitmap(LPSTR FileName, RECT *DstRect, RECT *SrcRect, DWORD BitPerPix, DWORD Format, DWORD ResQuality, bool Resize, bool Grayscale, bool Sharpen, bool Blur, int Contrast, int Brightness)
{
 int     DestWidth;
 int     DestHeight;
 int     PalIndex;
 bool    UseFilter;
 RECT    RSource;
 RECT    RDestin;
 DWORD   Result;
 DWORD   ErrCode;
 DWORD   FlagRLE;
 DWORD   HdrSize;
 DWORD   DataSize;
 DWORD   ComprSize;
 DWORD   ImagesNum;
 DWORD   IndexCount;
 DWORD   DstBytesPP;
 DWORD   SrcBytesPP;
 DWORD   CBackPrvPos;
 DWORD   CallbackMax;
 DWORD   SaveRowSize;
 PVOID   PalettePtr = NULL;
 PVOID   SrcRowPtr  = NULL;
 PVOID   DstRowPtr  = NULL;
 PVOID   FmtRowPtr  = NULL;
 PVOID   RowToSave  = NULL;
 PVOID   IndexBlock = NULL;
 PVOID   RowCompBuf = NULL;
 PVOID   ClrAverBuf = NULL;
 HANDLE  hSaveFile;
 RGBQUAD Palette[256];
 BMSFHEADER       BSFHdr;
 BMPARCHIVEHDR    BGFHdr;
 BITMAPFILEHEADER BmpFHdr;
 BITMAPINFOHEADER BmpIHdr;
 OCTREEQUANTIZER  *Quantizer = NULL;

 if((BitPerPix != 0)&&(BitPerPix != 8)&&(BitPerPix != 16)&&(BitPerPix != 24)&&(BitPerPix != 32))return 1;
 if((Format != FLAG_BM)&&(Format != FLAG_BS)&&(Format != FLAG_BG))return 2;

 __GUARDNEEDED(ProcGuardEnter());
 DeleteFile(FileName);
 if((hSaveFile = CreateFile(FileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL))==INVALID_HANDLE_VALUE){ErrCode=3;goto Exit;}

 if(!SrcRect)SetRect(&RSource,0,0,BmpWidth,BmpHeight);
   else CopyRect(&RSource,SrcRect);
 if(!DstRect){CopyRect(&RDestin,&RSource);Resize=false;}
  else CopyRect(&RDestin,DstRect);
 DestWidth   = (RDestin.right-RDestin.left);
 DestHeight  = (RDestin.bottom-RDestin.top);
 DstBytesPP  = (BitPerPix)?(BitPerPix/8):BytesInPixel;
 IndexCount  = 0;
 ImagesNum   = 0;
 DataSize    = 0;
 PalIndex    = 0;
 ErrCode     = 5;

 // This is instead 'SetView' - very important!
 RowsToBuffer   = ((MEGABYTE*RBUFSAVEMB)/(DestWidth*4));
 if(RowsToBuffer > (DestHeight/2))RowsToBuffer = (DestHeight/2);   // Prebuffering of rows
 DstWidth       = DestWidth;
 DstHeight      = DestHeight;

 UseFilter      = (Blur||Sharpen||Grayscale||Contrast||Brightness);
 SrcBytesPP     = (Resize || (UseFilter && (BytesInPixel < 2)))?(4):BytesInPixel;
 PalettePtr     = (DstBytesPP != SrcBytesPP)?(&Palette):((PVOID)&BitmapInfo->bmiColors);
 SaveRowSize    = _ALIGN_FRWRD((DestWidth*DstBytesPP),sizeof(DWORD));
 DstRowPtr      = HeapAlloc(DefHeap, HEAP_ZERO_MEMORY, (DestWidth*4)+16);  // For safe use maximum Bpp
 if(!ResAverBufPtr)
 {
  ClrAverBuf    = HeapAlloc(DefHeap, HEAP_ZERO_MEMORY, ((DestWidth+1)*sizeof(RESIZEAVERBUF)));  // Temporary allocate the color averaging buffer
  ResAverBufPtr = (RESIZEAVERBUF*)ClrAverBuf;
  if(!ClrAverBuf){ErrCode=4;goto Exit;}
 }
 if((DstBytesPP != SrcBytesPP)||(Format == FLAG_BG)){if(!(FmtRowPtr = HeapAlloc(DefHeap, HEAP_ZERO_MEMORY, (DstBytesPP*(DestWidth+4))))){ErrCode=5;goto Exit;}}   // 4-For heap safe
 if(Format  == FLAG_BM)IndexCount = DestHeight;
 if((Format == FLAG_BS)||(Format == FLAG_BG))
  {
   if(Format == FLAG_BS)IndexCount = DestHeight;
   if(Format == FLAG_BG)
	{
	 int Hght=DestHeight;
	 do
      {
	   ImagesNum++;
	   IndexCount += Hght;
	   Hght = (Hght / 2);
      }
	   while(Hght >= IMGMINSIZE);
	}
   if(!(IndexBlock = HeapAlloc(DefHeap, HEAP_ZERO_MEMORY, ((IndexCount+4)*sizeof(DWORD))))){ErrCode=5;goto Exit;} // 4-For heap safe
   if(!(RowCompBuf = HeapAlloc(DefHeap, HEAP_ZERO_MEMORY, (SaveRowSize+sizeof(DWORD))))){ErrCode=5;goto Exit;}   // 4-For heap safe
   FlagRLE = ((DstBytesPP==1)?(FLAG_RGB08):((DstBytesPP==2)?(FLAG_RGB16):((DstBytesPP==3)?(FLAG_RGB24):FLAG_RGB32)));
  }

 switch(Format)  // Calculate headers size
  {
   case FLAG_BM:
     HdrSize = (((DstBytesPP==1)?(sizeof(Palette)):0)+(sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)));
	 break;
   case FLAG_BS:
     HdrSize = (((DstBytesPP==1)?(sizeof(Palette)):0)+(sizeof(BMSFHEADER)+sizeof(BITMAPINFOHEADER))+(IndexCount*sizeof(DWORD)));
     break;
   case FLAG_BG:
     HdrSize = (((DstBytesPP==1)?(sizeof(Palette)):0)+(sizeof(BMPARCHIVEHDR))+(IndexCount*sizeof(DWORD)));
	 BGFHdr.FlagBGAR      = FLAG_BAGR;
     BGFHdr.RlePFlag      = FlagRLE;
	 BGFHdr.BaseWidth     = DestWidth;
     BGFHdr.BaseHeight    = DestHeight;
     BGFHdr.LevelsCount   = ImagesNum;
     BGFHdr.PaletteSize   = ((DstBytesPP==1)?(sizeof(Palette)):0);
	 BGFHdr.IndexesSize   = (IndexCount*sizeof(DWORD));
     BGFHdr.BytesPerPixel = DstBytesPP;
     break;
   default : {ErrCode=5;goto Exit;}
  }

 // Process bitmap rows
 Quantizer = new OCTREEQUANTIZER;          // Create always for safe !!!
 SetFilePointer(hSaveFile,HdrSize,NULL,FILE_BEGIN); // Enlarge file
 if(((DstBytesPP != SrcBytesPP)||UseFilter)&&(DstBytesPP == 1))     // For filter need recreate 8 bit palette
  {
   CBackPrvPos = DestWidth+30;          // 30 - for GetPalette delay
   CallbackMax = (IndexCount+CBackPrvPos);
   ZeroMemory(&Palette,sizeof(Palette));
   if(CallBackProc){if(CallBackProc(-1,CallbackMax)){ErrCode=10;goto Exit;}}
   if(ResQuality > 9)Quantizer->SetOctreeDepth(5+(ResQuality-9)); // More smooth, but colors differents
   for(int CurRow=0,RowBPP=(UseFilter)?(4):SrcBytesPP;CurRow < DestHeight;CurRow++)
	{
	 SrcRowPtr = GetBitmapRow(CurRow,RSource.left,DestWidth);
	 if(UseFilter){FilterRow(DstRowPtr,SrcRowPtr,RowBPP,BytesInPixel,DestWidth,Brightness,Contrast,Grayscale,Sharpen,Blur);SrcRowPtr = DstRowPtr;}
	 for(int CurPix=0;CurPix < DestWidth;CurPix++)
	  {
       Result = ExtractRGB(&((BYTE*)SrcRowPtr)[RowBPP*CurPix], RowBPP);
       Quantizer->AppendColor(((BYTE*)&Result)[2],((BYTE*)&Result)[1],((BYTE*)&Result)[0]);
	  }
     if(CallBackProc){if(CallBackProc(CurRow,CallbackMax)){ErrCode=10;goto Exit;}}
	}
   Quantizer->GetPalette((RGBQUAD*)((PVOID)&Palette),256);    // Always 256-color Palette
   if(CallBackProc){if(CallBackProc((CBackPrvPos-30),CallbackMax)){ErrCode=10;goto Exit;}}

   // Preserve basic colors
   if(Quantizer->ColorsInTree >= 256)     // If resulting colors is wrong - check here
	{
	 Result = Quantizer->FindNearestColor(0x00,0x00,0x00);  // BLACK
	 Palette[Result].rgbRed   = 0x00;
	 Palette[Result].rgbBlue  = 0x00;
	 Palette[Result].rgbGreen = 0x00;
	 Result = Quantizer->FindNearestColor(0xFF,0xFF,0xFF);  // WHITE
	 Palette[Result].rgbRed   = 0xFF;
	 Palette[Result].rgbBlue  = 0xFF;
	 Palette[Result].rgbGreen = 0xFF;
	 Result = Quantizer->FindNearestColor(0xFF,0x00,0x00);  // RED
	 Palette[Result].rgbRed   = 0xFF;
	 Palette[Result].rgbBlue  = 0x00;
	 Palette[Result].rgbGreen = 0x00;
	 Result = Quantizer->FindNearestColor(0x00,0xFF,0x00);  // GREEN
	 Palette[Result].rgbRed   = 0x00;
	 Palette[Result].rgbBlue  = 0x00;
	 Palette[Result].rgbGreen = 0xFF;
	 Result = Quantizer->FindNearestColor(0x00,0x00,0xFF);  // BLUE
	 Palette[Result].rgbRed   = 0x00;
	 Palette[Result].rgbBlue  = 0xFF;
	 Palette[Result].rgbGreen = 0x00;                       // ???
	 Result = Quantizer->FindNearestColor(0xFF,0x00,0xFF);
	 Palette[Result].rgbRed   = 0xFF;
	 Palette[Result].rgbBlue  = 0xFF;
	 Palette[Result].rgbGreen = 0x00;
	 Result = Quantizer->FindNearestColor(0xFF,0xFF,0x00);  // ???
	 Palette[Result].rgbRed   = 0xFF;
	 Palette[Result].rgbBlue  = 0x00;
	 Palette[Result].rgbGreen = 0xFF;
	 Result = Quantizer->FindNearestColor(0x00,0xFF,0xFF);  // ???
	 Palette[Result].rgbRed   = 0x00;
	 Palette[Result].rgbBlue  = 0xFF;
	 Palette[Result].rgbGreen = 0xFF;
 	}

   if(CallBackProc){if(CallBackProc(CBackPrvPos,CallbackMax)){ErrCode=10;goto Exit;}}
  }
   else
	{
	 CBackPrvPos             = 0;
	 CallbackMax             = IndexCount;
	 Quantizer->CurPalette   = (RGBQUAD*)PalettePtr;
	 Quantizer->ColorsInTree = 256;
     if(CallBackProc){if(CallBackProc(-1,CallbackMax)){ErrCode=10;goto Exit;}}
	}
 for(int CurRow=0,Index=0;CurRow < DestHeight;CurRow++,Index++)
  {
   if(CallBackProc){if(CallBackProc((CBackPrvPos+Index),CallbackMax)){ErrCode=10;goto Exit;}}
   if(Resize)
	{   // Use smooth resizing algorythm
	 ResizePixelsRow(DstRowPtr,&RSource,DestWidth,DestHeight,CurRow,ResQuality);   // Standart sharpen after resizing
	 FilterRow(DstRowPtr,DstRowPtr,4,4,DestWidth,Brightness,Contrast,Grayscale,true,Blur); // Always sharpen after resizing
	 if(Sharpen)FilterRow(DstRowPtr,DstRowPtr,4,4,DestWidth,0,0,false,Sharpen,false);   // More Sharpen
	}
	 else
	  {   // Use direct pixels copy (No resizing)
	   SrcRowPtr = GetBitmapRow(CurRow,RSource.left,DestWidth);
	   if(UseFilter)FilterRow(DstRowPtr,SrcRowPtr,SrcBytesPP,BytesInPixel,DestWidth,Brightness,Contrast,Grayscale,Sharpen,Blur);
		 else CopyMemory(DstRowPtr,SrcRowPtr,(DestWidth*SrcBytesPP));
	  }
   if(DstBytesPP != SrcBytesPP){PalIndex += ChangeColorFormat(FmtRowPtr,DstRowPtr,DstBytesPP,SrcBytesPP,DestWidth,Quantizer);RowToSave = FmtRowPtr;}
	 else RowToSave = DstRowPtr;

   switch(Format)  // Write Rows
	{
     case FLAG_BM:
      {
	   WriteFile(hSaveFile,RowToSave,SaveRowSize,&Result,NULL);
	   if(Result != SaveRowSize){ErrCode=8;goto Exit;}
       break;
      }
     case FLAG_BS:
	  {
       ((DWORD*)IndexBlock)[Index] = (HdrSize+DataSize);
       ComprSize  = CompressRow(RowCompBuf,RowToSave,SaveRowSize,DstBytesPP,FlagRLE);
       DataSize  += ComprSize;
	   WriteFile(hSaveFile,RowCompBuf,ComprSize,&Result,NULL);
	   if(Result != ComprSize){ErrCode=8;goto Exit;}
	   break;
      }
	 case FLAG_BG:
      {
       ((DWORD*)IndexBlock)[Index] = (HdrSize+DataSize);
       ComprSize  = CompressRow(RowCompBuf,RowToSave,SaveRowSize,DstBytesPP,FlagRLE);
	   DataSize  += ComprSize;
       WriteFile(hSaveFile,RowCompBuf,ComprSize,&Result,NULL);
       if(Result != ComprSize){ErrCode=8;goto Exit;}
	   if(((CurRow+1) >= DestHeight)&&(ImagesNum > 1))
		{
		 Resize      = true;
		 DestWidth    = (DestWidth/2);
         DestHeight   = (DestHeight/2);
		 SrcBytesPP  = 4;
         SaveRowSize = _ALIGN_FRWRD((DestWidth*DstBytesPP),sizeof(DWORD));
         if((DestWidth >= IMGMINSIZE)&&(DestHeight >= IMGMINSIZE))CurRow = -1;
        }
	   break;
      }
     default : {ErrCode=5;goto Exit;}
    }
  }

 // Set Safe DWORD Mapped sec read buffer
 SetFilePointer(hSaveFile,sizeof(DWORD),NULL,FILE_CURRENT);
 BmpFExtended = SetEndOfFile(hSaveFile);

 // Write Headers
 SetFilePointer(hSaveFile,0,NULL,FILE_BEGIN);
 switch(Format)
  {
   case FLAG_BM:
	{
	 BmpFHdr.bfType          = FLAG_BM;
	 BmpFHdr.bfSize          = (HdrSize+(SaveRowSize*DestHeight));
	 BmpFHdr.bfReserved1     = 0;
	 BmpFHdr.bfReserved2     = 0;
     BmpFHdr.bfOffBits       = HdrSize;
	 BmpIHdr.biSize          = sizeof(BITMAPINFOHEADER);
     BmpIHdr.biWidth         = DestWidth;
	 BmpIHdr.biHeight        = DestHeight*(-1);  // Always saved as TOPDOWN
	 BmpIHdr.biPlanes        = 1;
     BmpIHdr.biBitCount      = (DstBytesPP*8);
     BmpIHdr.biCompression   = BI_RGB;
	 BmpIHdr.biSizeImage     = (SaveRowSize*DestHeight);
	 BmpIHdr.biXPelsPerMeter = 0;
	 BmpIHdr.biYPelsPerMeter = 0;
	 BmpIHdr.biClrUsed       = PalIndex;
	 BmpIHdr.biClrImportant  = PalIndex;
	 WriteFile(hSaveFile,&BmpFHdr,sizeof(BITMAPFILEHEADER),&Result,NULL);
	 if(Result != sizeof(BITMAPFILEHEADER)){ErrCode=6;goto Exit;}
	 WriteFile(hSaveFile,&BmpIHdr,sizeof(BITMAPINFOHEADER),&Result,NULL);
	 if(Result != sizeof(BITMAPINFOHEADER)){ErrCode=6;goto Exit;}
	 if(DstBytesPP == 1){WriteFile(hSaveFile,PalettePtr,sizeof(Palette),&Result,NULL);if(Result != sizeof(Palette)){ErrCode=6;goto Exit;}}
	 break;
	}
   case FLAG_BS:
	{
	 BSFHdr.FlagBS           = FLAG_BS;
	 BSFHdr.RleFlag          = FlagRLE;
	 BSFHdr.IndexOffset      = HdrSize-(DestHeight*sizeof(DWORD));
	 BSFHdr.PixelsOffset     = HdrSize;
	 BmpIHdr.biSize          = sizeof(BITMAPINFOHEADER);
	 BmpIHdr.biWidth         = DestWidth;
	 BmpIHdr.biHeight        = DestHeight*(-1);    // Always saved as TOPDOWN
	 BmpIHdr.biPlanes        = 1;
	 BmpIHdr.biBitCount      = (DstBytesPP*8);
	 BmpIHdr.biCompression   = BI_IRC;
	 BmpIHdr.biSizeImage     = (SaveRowSize*DestHeight);
	 BmpIHdr.biXPelsPerMeter = 0;
	 BmpIHdr.biYPelsPerMeter = 0;
	 BmpIHdr.biClrUsed       = PalIndex;
	 BmpIHdr.biClrImportant  = PalIndex;
	 WriteFile(hSaveFile,&BSFHdr,sizeof(BMSFHEADER),&Result,NULL);
	 if(Result != sizeof(BMSFHEADER)){ErrCode=6;goto Exit;}
	 WriteFile(hSaveFile,&BmpIHdr,sizeof(BITMAPINFOHEADER),&Result,NULL);
	 if(Result != sizeof(BITMAPINFOHEADER)){ErrCode=6;goto Exit;}
	 if(DstBytesPP == 1){WriteFile(hSaveFile,PalettePtr,sizeof(Palette),&Result,NULL);if(Result != sizeof(Palette)){ErrCode=6;goto Exit;}}
	 WriteFile(hSaveFile,IndexBlock,(IndexCount*sizeof(DWORD)),&Result,NULL);
	 if(Result != (IndexCount*sizeof(DWORD))){ErrCode=6;goto Exit;}
	 break;
	}
   case FLAG_BG:
    {
     WriteFile(hSaveFile,&BGFHdr,sizeof(BMPARCHIVEHDR),&Result,NULL);
	 if(Result != sizeof(BMPARCHIVEHDR)){ErrCode=6;goto Exit;}
	 if(DstBytesPP == 1){WriteFile(hSaveFile,PalettePtr,sizeof(Palette),&Result,NULL);if(Result != sizeof(Palette)){ErrCode=6;goto Exit;}}
	 WriteFile(hSaveFile,IndexBlock,(IndexCount*sizeof(DWORD)),&Result,NULL);
	 if(Result != (IndexCount*sizeof(DWORD))){ErrCode=6;goto Exit;}
     break;
	}
   default : {ErrCode=10;goto Exit;}
  }
 ErrCode = 0;

Exit:
 if(CallBackProc)CallBackProc(CallbackMax,CallbackMax);
 if(DstRowPtr) HeapFree(DefHeap, NULL, DstRowPtr);
 if(FmtRowPtr) HeapFree(DefHeap, NULL, FmtRowPtr);
 if(IndexBlock)HeapFree(DefHeap, NULL, IndexBlock);
 if(RowCompBuf)HeapFree(DefHeap, NULL, RowCompBuf);
 if(ClrAverBuf){HeapFree(DefHeap, NULL, ClrAverBuf);ResAverBufPtr=NULL;}   // Set to zero as prev
 if(hSaveFile != INVALID_HANDLE_VALUE)CloseHandle(hSaveFile);
 if(ErrCode)DeleteFile(FileName);   // Do not keep broken file
 if(Quantizer)delete(Quantizer);
 __GUARDNEEDED(ProcGuardExit());
 return ErrCode;
}
//===========================================================================
//
//                              P U B L I C
//
DWORD _stdcall CBIGBITMAP::LoadBitmapRows(int StartRow, int RowsNumber)
{
 int   UpperRow;
 int   LowerRow;
 int   ExtraRows;
 DWORD DataOffset;
 QWORD FileOffset;
 QWORD AlignedFOff;
 DWORD MapDataSize;

 FirstRowLoaded  = ((StartRow > RowsToBuffer)?(StartRow-RowsToBuffer):0);
 if(BmpTopDown)
  {
   UpperRow    = FirstRowLoaded;
   LowerRow    = (StartRow+RowsNumber+RowsToBuffer)-1;
  }
   else
	{
	 UpperRow  = (BmpHeight-StartRow-RowsNumber-RowsToBuffer-1);
     LowerRow  = (BmpHeight-StartRow-1)+RowsToBuffer;
	}
 if(UpperRow <  0)UpperRow = 0;
 if(LowerRow >= BmpHeight)LowerRow = (BmpHeight-1);

 switch(ImageType)
  {                                        
   case FLAG_BM:
    {
     FileOffset      = (RowsOffset+(AlignedRowSize*UpperRow));        // Get offset of LoadedUpperRow in file for TOPBOTTOM
     AlignedFOff     = _ALIGN_BKWRD(FileOffset,MapPageSize);          // Align Offset to full Page size for mapping
     DataOffset      = (FileOffset - AlignedFOff);                    // Extra bytes of alignment
     ExtraRows       = (DataOffset / AlignedRowSize);                 // Additional rows, resulting by alignment
     DataOffset      = (DataOffset % AlignedRowSize);                 // Additional bytes, not enough for row
     UpperRow       -= ExtraRows;                                     // Expand upper border
     TotalRowsLoaded = (LowerRow-UpperRow)+1;                         // Total number of rows to map (DstRectHeight+UpBorder+DownBorder)
     MapDataSize     = (DataOffset+(TotalRowsLoaded*AlignedRowSize)); // All bytes to mapping
     break;
    }
   case FLAG_BS:
   case FLAG_BG:
    {
     FileOffset      = ((DWORD*)RowIndexBuff)[UpperRow];               // Upper row offset
     AlignedFOff     = _ALIGN_BKWRD(FileOffset,MapPageSize);           // Align Offset to full Page size for mapping
     ExtraRows       = 0;
     for(int Index=(UpperRow-1);(Index>=0)&&(((DWORD*)RowIndexBuff)[Index]>=AlignedFOff);Index--)ExtraRows++;
     UpperRow       -= ExtraRows;
     TotalRowsLoaded = (LowerRow-UpperRow)+1;                          // Total number of rows to map (DstRectHeight+UpBorder+DownBorder)
     MapDataSize     = (((DWORD*)RowIndexBuff)[LowerRow+1]-AlignedFOff);
     DataOffset      = (((DWORD*)RowIndexBuff)[UpperRow]-AlignedFOff); // Extra bytes of alignment
     break;
	}
   default : {__GUARDNEEDED(ProcGuardExit());return 3;}
  }

 if(BmpTopDown)FirstRowLoaded -= ExtraRows;
 if(MappedSection)UnmapViewOfFile(MappedSection);
 if(!(MappedSection = MapViewOfFile(hMappedBmp,FILE_MAP_READ|FILE_MAP_WRITE,HIDWORD(AlignedFOff),LODWORD(AlignedFOff),(MapDataSize+3)))){ReadedRowsBuf=NULL;return 3;} // Map rows for screen and top-bottom buffers ;+3 works together with temporary bitmap file enlarging
 ReadedRowsBuf      = &((BYTE*)MappedSection)[DataOffset];
 return 0;
}
//===========================================================================
//
//        NEED MAXIMUM PERFOMANCE - THE BASE CALCULATIONS ARE MAKED HERE !!!
//
//
DWORD _stdcall CBIGBITMAP::UpdateBitmapBuffer(void)   // CallBack Here is makes it is a bit slow
{
 PVOID SrcRowPtr;
 PVOID DstRowPtr;

// if(CallBackProc)CallBackProc(-1,SrcHeight);  // !!!!!! SLOWER !!!!!!
 if(ResizePixels)
  {
   // Use smooth resizing algorythm
   if(ProcessPixels)
	{
	 for(int BRow=0;BRow < SrcHeight;BRow++)
	  {
	 //  if(CallBackProc)CallBackProc(BRow,SrcHeight);     // !!!!!! SLOWER !!!!!!
	   DstRowPtr = &((BYTE*)MemBmpBuffer)[(ADstRowSize*BRow)];
	   ResizePixelsRow(DstRowPtr,&ViewRect,DstWidth,DstHeight,(BRow+ViewRect.top),ResizeQuality);  // *** Dst... OR Src... ??? *****
	   FilterRow(DstRowPtr,DstRowPtr,4,4,DstWidth,FBrightness,FContrast,FGrayscale,FSharpen,FBlur);
      }
    }
	 else
      {
       for(int BRow=0;BRow < SrcHeight;BRow++)
        {
      //   if(CallBackProc)CallBackProc(BRow,SrcHeight);     // !!!!!! SLOWER !!!!!!
		 DstRowPtr = &((BYTE*)MemBmpBuffer)[(ADstRowSize*BRow)];
         ResizePixelsRow(DstRowPtr,&ViewRect,DstWidth,DstHeight,(BRow+ViewRect.top),ResizeQuality);
        }
      }
  }
   else
    {
     // Use direct pixels copy (No resizing)
     if(ProcessPixels)
      {
       for(int BRow=0;BRow < SrcHeight;BRow++)
        {
     //    if(CallBackProc)CallBackProc(BRow,SrcHeight);      // !!!!!! SLOWER !!!!!!
         SrcRowPtr = GetBitmapRow((BRow+ViewRect.top),ViewRect.left,DRowWidth);
         DstRowPtr = &((BYTE*)MemBmpBuffer)[(ADstRowSize*BRow)];
		 FilterRow(DstRowPtr,SrcRowPtr,DstBytesPP,BytesInPixel,DRowWidth,FBrightness,FContrast,FGrayscale,FSharpen,FBlur);
        }
	  }
	   else
        {
         for(int BRow=0;BRow < SrcHeight;BRow++)
          {
      //     if(CallBackProc)CallBackProc(BRow,SrcHeight);      // !!!!!! SLOWER !!!!!!
		   SrcRowPtr = GetBitmapRow((BRow+ViewRect.top),ViewRect.left,DRowWidth);
		   DstRowPtr = &((BYTE*)MemBmpBuffer)[(ADstRowSize*BRow)];
		   CopyMemory(DstRowPtr,SrcRowPtr,DstRowSize);     // Copy image row
		  }
        }
    }
// if(CallBackProc)CallBackProc(SrcHeight,SrcHeight);    // !!!!!! SLOWER !!!!!!
 return 0;
}
//===========================================================================
void _stdcall CBIGBITMAP::SetPixelRGB(PVOID DataPtr, DWORD BytesPP, BYTE Red, BYTE Green, BYTE Blue)
{
 DWORD PixColor;

 switch(BytesPP)
  {
   case 4:
    {
     ((BYTE*)DataPtr)[0] = Blue;
     ((BYTE*)DataPtr)[1] = Green;
     ((BYTE*)DataPtr)[2] = Red;
     ((BYTE*)DataPtr)[3] = 0;
     break;
    }
   case 3:
	{
	 ((BYTE*)DataPtr)[0] = Blue;
     ((BYTE*)DataPtr)[1] = Green;
	 ((BYTE*)DataPtr)[2] = Red;
	 break;
    }
   case 2:
    {
     Red      = (Red>>3);
     Green    = (Green>>3);
     Blue     = (Blue>>3);
     PixColor = Red;
     PixColor = ((PixColor<<5)|Green);
     PixColor = ((PixColor<<5)|Blue);
     ((BYTE*)DataPtr)[0]  = ((BYTE*)&PixColor)[0];
     ((BYTE*)DataPtr)[1]  = ((BYTE*)&PixColor)[1];
     break;
    }
   case 1:
    {
     // Direct filtering impossible with indexed colors !!!!!
     break;
    }
  }
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::ExtractRGB(PVOID DataPtr, DWORD BytesPP)
{
 DWORD PixColor;
 BYTE  R,G,B;

 switch(BytesPP)
  {
   case 4:
    {
	 PixColor = ((DWORD*)DataPtr)[0];              // Assume - always direct colors
     break;
    }
   case 3:
    {
     PixColor = (((DWORD*)DataPtr)[0])&0x00FFFFFF; // Assume - always direct colors
     break;
    }
   case 2:
    {
     PixColor = ((DWORD*)DataPtr)[0];
     R = (PixColor&0x0000001F);
     G = (PixColor>>5)&0x0000001F;
     B = (PixColor>>10)&0x0000001F;
     ((BYTE*)&PixColor)[0] = (R<<3)|0x00000003; // In ASM May be used shl with CARRY flag
     ((BYTE*)&PixColor)[1] = (G<<3)|0x00000003; // Update contrast
     ((BYTE*)&PixColor)[2] = (B<<3)|0x00000003; // Update contrast
     ((BYTE*)&PixColor)[3] = 0;
     break;
    }
   case 1:
    {
     PixColor = ((DWORD*)&BitmapInfo->bmiColors[(((BYTE*)DataPtr)[0])])[0]; // Assume - always indexed colors
     break;
    }
   default : PixColor = 0;
  }
 return PixColor;
}
//===========================================================================
void _stdcall CBIGBITMAP::FilterRow(PVOID DstRowPtr, PVOID SrcRowPtr, int DstBytesPP, int SrcBytesPP, int RowWidth, int Brightness, int Contrast, bool Grayscale, bool Sharpen, bool Blur)
{
 int   R,G,B;
 float CValue;
 PVOID PixPtr;
 DWORD PixColor;
 DWORD CPixAfter;
 DWORD CPixBefore;

 for(int ctr=0;ctr < RowWidth;ctr++)
  {
   PixPtr   = &((BYTE*)SrcRowPtr)[(SrcBytesPP*ctr)];
   PixColor = ExtractRGB(PixPtr, SrcBytesPP);
   B = ((BYTE*)&PixColor)[0];
   G = ((BYTE*)&PixColor)[1];
   R = ((BYTE*)&PixColor)[2];

   if(Blur)
    {
     CPixAfter  = ((ctr+1)<RowWidth)?(ExtractRGB(&((BYTE*)SrcRowPtr)[(SrcBytesPP*(ctr+1))],SrcBytesPP)):0;
     CPixBefore = ((ctr-1)<0)?(0):ExtractRGB(&((BYTE*)SrcRowPtr)[(SrcBytesPP*(ctr-1))],SrcBytesPP);
     B = FPURound32((float)( ((1)*((BYTE*)&CPixBefore)[0]) + (1*B) + ((1)*((BYTE*)&CPixAfter)[0])) / (1+1+1));
     G = FPURound32((float)( ((1)*((BYTE*)&CPixBefore)[1]) + (1*G) + ((1)*((BYTE*)&CPixAfter)[1])) / (1+1+1));
     R = FPURound32((float)( ((1)*((BYTE*)&CPixBefore)[2]) + (1*R) + ((1)*((BYTE*)&CPixAfter)[2])) / (1+1+1));
    }

   if(Sharpen)    // Something wrong with the Matrix, but this is work !
    {
     CPixAfter  = ((ctr+1)<RowWidth)?(ExtractRGB(&((BYTE*)SrcRowPtr)[(SrcBytesPP*(ctr+1))],SrcBytesPP)):0;
     CPixBefore = ((ctr-1)<0)?(0):ExtractRGB(&((BYTE*)SrcRowPtr)[(SrcBytesPP*(ctr-1))],SrcBytesPP);
     B = FPURound32((float)( ((-1)*((BYTE*)&CPixBefore)[0]) + (5*B) + ((-1)*((BYTE*)&CPixAfter)[0])) / ((-1)+5+(-1)));
     G = FPURound32((float)( ((-1)*((BYTE*)&CPixBefore)[1]) + (5*G) + ((-1)*((BYTE*)&CPixAfter)[1])) / ((-1)+5+(-1)));
     R = FPURound32((float)( ((-1)*((BYTE*)&CPixBefore)[2]) + (5*R) + ((-1)*((BYTE*)&CPixAfter)[2])) / ((-1)+5+(-1)));
    }

   // ***** Best Order is Brightness before Contrast ??? *****

   // Update Brightness
   if(Brightness != 0)      // (-255) - 0 - (+255)
    {
     R = R+Brightness;
     G = G+Brightness;
     B = B+Brightness;
    }

    // Update Contrast
   if(Contrast != 0)
    {
     if(Contrast > 0)CValue = (1 + (((float)Contrast) / 10)); // 0 - (+255)
       else CValue = (1 - (FPUSqrt32(-Contrast) / 16));       // 0 - (-255)
	 R = FPURound32(128 + ((R - 128) * CValue));
	 G = FPURound32(128 + ((G - 128) * CValue));
	 B = FPURound32(128 + ((B - 128) * CValue));
    }

   // Grayscale after all other filters
   if(Grayscale)R=G=B = FPURound32((R * 0.56)+(G * 0.33)+(B * 0.11));

   R = BYTELIMIT(R);
   G = BYTELIMIT(G);
   B = BYTELIMIT(B);
   SetPixelRGB(&((BYTE*)DstRowPtr)[(DstBytesPP*ctr)],DstBytesPP,R,G,B);
  }
}
//===========================================================================
PVOID _stdcall CBIGBITMAP::GetBitmapRow(int Index, int PixOffset, int PixCount)
{
 DWORD Offset;
 PVOID RowPtr;

 if((Index < FirstRowLoaded)||(Index > (FirstRowLoaded+TotalRowsLoaded-1))){if(LoadBitmapRows(Index,(RowsToBuffer/2)))return NULL;}
 switch(ImageType)
  {
   case FLAG_BM:
	{
     if(BmpTopDown)RowPtr = &((BYTE*)ReadedRowsBuf)[(AlignedRowSize*(Index-FirstRowLoaded))];
       else RowPtr = &((BYTE*)ReadedRowsBuf)[(AlignedRowSize*((TotalRowsLoaded-(Index-FirstRowLoaded))-1))];
	 RowPtr = &((BYTE*)RowPtr)[(PixOffset*BytesInPixel)];
     break;
    }
   case FLAG_BS:
   case FLAG_BG:
    {
     if((PixOffset+PixCount) > BmpWidth)PixCount = (BmpWidth - PixOffset);    // Uot of range guard
	 if((Index != PrvRowIndex)||(PrvPixOffset > PixOffset)||(PixOffset >= (PrvPixOffset+PrvPixCount)))
	  {                       // Row was changed, decompress another row
       if(BmpTopDown)
		{
         Offset = ((DWORD*)RowIndexBuff)[FirstRowLoaded]; // In BS offsets are DWORD
         Offset = (((DWORD*)RowIndexBuff)[Index]-Offset); // Offset of compressed row in buffer
        }
         else
		  {
		   Offset = ((DWORD*)RowIndexBuff)[(BmpHeight-FirstRowLoaded-TotalRowsLoaded)]; // File offset same row as first row in buffer
           Offset = (((DWORD*)RowIndexBuff)[(BmpHeight-Index-1)])-Offset;               // Buffer offset of requested row
		  }
	   PrvRowIndex  = Index;
	   PrvPixCount  = PixCount;
	   PrvPixOffset = PixOffset;
	   DeCompressRowRLE(&((BYTE*)ReadedRowsBuf)[Offset],PixOffset,PixCount);
	  }
	 RowPtr = &((BYTE*)DecRowBuffer)[((PixOffset-PrvPixOffset)*BytesInPixel)];  // Dada in decompressed row buffer
	 break;
    }
   default : RowPtr = NULL;
  }
 return RowPtr;
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::GetBitmapPixel(int XPos, int YPos)
{
 return ExtractRGB(GetBitmapRow(YPos,XPos,DstWidth), BytesInPixel);
}
//===========================================================================
void _stdcall CBIGBITMAP::ResizePixelsRow(PVOID RowBuffer, RECT *SrcRect, int NewWidth, int NewHeight, int DstRowIndex, int Quality)
{
 int   xpix, ypix, pxpix, pypix;
 float ISrcWidth, ISrcHeight, PStep, xini, xfi, yini, yfi, saltx, salty;
 DWORD PixelColor;

 // Allowed values: 0 (+)-> 9  converted to 1.0 (-)-> 0.1
 if(Quality>=9)PStep=0.1;
   else {if(Quality<=0)PStep=1;
		   else PStep = ((float)(10-Quality))/10;}

 // Set target size
 ISrcWidth  = (SrcRect->right-SrcRect->left)-1;
 ISrcHeight = (SrcRect->bottom-SrcRect->top)-1;
 // Calcs width & height of every area of pixels of the source bitmap
 saltx = (ISrcWidth / NewWidth);
 salty = (ISrcHeight / NewHeight);
 // Set the initial and final Y coordinate of a pixel area
 yini  = (salty * DstRowIndex);       // Start from specified row
 yfi   = (yini + salty);              // Pixels step to next pixel
 pxpix = pypix = -1;

 ZeroMemory(ResAverBufPtr,(NewWidth*sizeof(RESIZEAVERBUF)));  // Reset averaging info
 // This loop calcs del average result color of a pixel area of the imaginary grid
 for(float py = yini;py <= yfi;py+=PStep)
  {
   ypix = (FPURound32(py)+SrcRect->top);
   if(ypix >= BmpHeight)continue;        // On excess pixel may occur when shrinking with (PStep < 1) // Scan rect must be within image rect - Y border
   xfi  = 0;
   for(int x = 0;x < NewWidth;x++)
	{
	 // Set the inital and final X coordinate of a pixel area
	 xini  = xfi;
	 xfi  += saltx;     // Pixels step to next pixel
	 for(float px = xini;px <= xfi;px += PStep)  // Calculate horizontal line average color
	  {
	   xpix       = (FPURound32(px)+SrcRect->left);
	   if(xpix   >= BmpWidth)continue;       // On excess pixel may occur when shrinking with (PStep < 1) Scan rect must be within image rect - X border
	   if((pxpix != xpix)||(pypix != ypix))  // Do not faster, but more balanced with different pixel formats
		{
		 PixelColor = GetBitmapPixel(xpix,ypix);   // Read Source pixel for averaging color in format of full image
		 pxpix      = xpix;
		 pypix      = ypix;
		}
	   ResAverBufPtr[x].TotalRed   += ((BYTE*)&PixelColor)[0];
	   ResAverBufPtr[x].TotalGreen += ((BYTE*)&PixelColor)[1];
	   ResAverBufPtr[x].TotalBlue  += ((BYTE*)&PixelColor)[2];
	   ResAverBufPtr[x].PixCount++;
	  }
	}
  }

 // Convert Average info to new pixels value
 for(int x = 0;x < NewWidth;x++)
  {
   PixelColor = 0;
   if(ResAverBufPtr[x].PixCount)
	{
	 // Draws the result pixel
	 ((BYTE*)&PixelColor)[0] = FPURound32((float)ResAverBufPtr[x].TotalRed   / ResAverBufPtr[x].PixCount);
	 ((BYTE*)&PixelColor)[1] = FPURound32((float)ResAverBufPtr[x].TotalGreen / ResAverBufPtr[x].PixCount);
	 ((BYTE*)&PixelColor)[2] = FPURound32((float)ResAverBufPtr[x].TotalBlue  / ResAverBufPtr[x].PixCount);
	}
   ((DWORD*)RowBuffer)[x] = PixelColor;  // Always assumed 32 Bpp
  }
}
//===========================================================================
int _stdcall CBIGBITMAP::ChangeColorFormat(PVOID DstPixels, PVOID SrcPixels, DWORD DstBytePP, DWORD SrcBytePP, DWORD PixNumber, OCTREEQUANTIZER *Quantizer)
{
 DWORD PixColor;

 if(DstBytePP > 1)
  {
   for(int pctr=0;pctr < PixNumber;pctr++)
    {
	 PixColor = ExtractRGB(&((BYTE*)SrcPixels)[SrcBytePP*pctr],SrcBytePP);
     SetPixelRGB(&((BYTE*)DstPixels)[DstBytePP*pctr],DstBytePP,((BYTE*)&PixColor)[2],((BYTE*)&PixColor)[1],((BYTE*)&PixColor)[0]);
    }
  }
   else
    {
     for(int pctr=0;pctr < PixNumber;pctr++)
      {
       PixColor = ExtractRGB(&((BYTE*)SrcPixels)[SrcBytePP*pctr],SrcBytePP);
       ((BYTE*)DstPixels)[DstBytePP*pctr] = Quantizer->FindNearestColor(((BYTE*)&PixColor)[2],((BYTE*)&PixColor)[1],((BYTE*)&PixColor)[0]);
      }
    }
 return 0;
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::GetComprRowWidthF(HANDLE hDataFile, DWORD RowFileOffset, DWORD RleFlagCode, int PixelsInRow, int BytesInPixel)
{
 DWORD Result;
 DWORD CurBlock;
 DWORD CurPixel;
 DWORD PrevFilePtr;
 DWORD CounterSize;
 DWORD PixelsCounter;
 DWORD BytesAnalyzed;
 BYTE  DataBuffer[256];

 CurBlock      = -1;
 BytesAnalyzed = 0;
 PrevFilePtr   = SetFilePointer(hDataFile,0,NULL,FILE_CURRENT);
 SetFilePointer(hDataFile,RowFileOffset,NULL,FILE_BEGIN);
 while(PixelsInRow > 0)
  {
   // Get pixel for analysis
   if(((int)(BytesAnalyzed/sizeof(DataBuffer))) > ((int)CurBlock)){ReadFile(hDataFile,DataBuffer,128,&Result,NULL);CurBlock++;}
   CurPixel = (((DWORD*)&DataBuffer[(BytesAnalyzed-(CurBlock*sizeof(DataBuffer)))])[0] & (0xFFFFFFFF >> ((sizeof(DWORD)-BytesInPixel)*8)));
   if(CurPixel == RleFlagCode)
	{
	 BytesAnalyzed += BytesInPixel;
	 CurPixel = ((DWORD*)&DataBuffer[(BytesAnalyzed-(CurBlock*sizeof(DataBuffer)))])[0];
	 // Get pixels counter
	 if((CurPixel & 0x00000001))
	  {
	   CounterSize   = ((CurPixel>>2)&0x00000003);
	   PixelsCounter = ((0xFFFFFFFF >> (((sizeof(DWORD)-CounterSize)*8)-4))&(CurPixel>>4));
	  }
	   else {CounterSize = 0;PixelsCounter = ((CurPixel>>2)&0x0000003F);}
	 BytesAnalyzed += (CounterSize+1);
	 // Check pixel type
	 if(!(CurPixel & 0x00000002))BytesAnalyzed += BytesInPixel;
	 PixelsInRow -= PixelsCounter;
	}
	 else {BytesAnalyzed += BytesInPixel; PixelsInRow--;}
  }

 SetFilePointer(hDataFile,PrevFilePtr,NULL,FILE_BEGIN);
 return BytesAnalyzed;
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::DeCompressRowRLE(PVOID SrcBuffer, int PixOffset, int PixCount)
{
 DWORD CurPixel;
 DWORD CounterSize;
 DWORD PixelsCounter;
 DWORD BytesRestored;
 DWORD BytesAnalyzed;

 BytesRestored = BytesAnalyzed = 0;
 while(PixCount > 0)
  {
   // Get pixel for analysis
   CurPixel = (((DWORD*)(&((BYTE*)SrcBuffer)[BytesAnalyzed]))[0] & SizeMask);
   if(CurPixel == RleFlagCode)
	{
	 BytesAnalyzed += BytesInPixel;
	 CurPixel = ((DWORD*)(&((BYTE*)SrcBuffer)[BytesAnalyzed]))[0];
	 // Get pixels counter
	 if((CurPixel & 0x00000001))
	  {
	   CounterSize   = ((CurPixel>>2)&0x00000003);
	   PixelsCounter = ((0xFFFFFFFF >> (((sizeof(DWORD)-CounterSize)*8)-4))&(CurPixel>>4));
	  }
	   else {CounterSize = 0;PixelsCounter = ((CurPixel>>2)&0x0000003F);}
	 BytesAnalyzed += (CounterSize+1);
	 // Check pixel type
	 if(!(CurPixel & 0x00000002))
	  {
	   CurPixel = (((DWORD*)(&((BYTE*)SrcBuffer)[BytesAnalyzed]))[0] & SizeMask);
	   BytesAnalyzed += BytesInPixel;
	  }
	   else CurPixel = FlagCode;
	 // Restore full sequence
	 if(PixOffset < PixelsCounter)
      {
       PixelsCounter -= PixOffset;
       PixOffset      = 0;
	   PixelsCounter  = (PixelsCounter > PixCount)?(PixCount):PixelsCounter;
       PixCount      -= PixelsCounter;
       while(PixelsCounter > 0)
        {
		 ((DWORD*)(&((BYTE*)DecRowBuffer)[BytesRestored]))[0] = CurPixel;
         BytesRestored += BytesInPixel;
         PixelsCounter--;
        }
	  }
       else {PixOffset -= PixelsCounter;}
	}
	 else
	  {
	   // Just write current pixel
	   if(!PixOffset)
		{
		 ((DWORD*)(&((BYTE*)DecRowBuffer)[BytesRestored]))[0] = CurPixel;
		 BytesRestored += BytesInPixel;
		 PixCount--;
		}
         else PixOffset--;
	   BytesAnalyzed += BytesInPixel;
	  }
  }

 return BytesRestored;
}
//===========================================================================
DWORD _stdcall CBIGBITMAP::CompressRow(PVOID DstBuffer,PVOID SrcBuffer,DWORD RowLength,DWORD PixelSize,DWORD FlagCode)
{
 DWORD WCtr;
 DWORD SizeMask;
 DWORD DstOffset;
 DWORD PixelsNumber;
 DWORD ColorValue;
 DWORD ColorValuePrv;
 DWORD Compressed;

 WCtr           = 0;
 DstOffset      = 0;
 Compressed     = 0;
 PixelsNumber   = 1;
 SizeMask       = (0xFFFFFFFF >> ((sizeof(DWORD)-PixelSize)*8));  // 0xFFFFFFFF=32bpp,0x00FFFFFF=24bpp,0x0000FFFF=16bpp,0x000000FF=8bpp
 ColorValuePrv  = ((((DWORD*)SrcBuffer)[0]) & SizeMask);          // Get first color by mask
 // Process row
 do
  {
   // Get pixel value
   WCtr      += PixelSize;
   ColorValue = ((((DWORD*)(&((BYTE*)SrcBuffer)[WCtr]))[0]) & SizeMask);   // Get next color by mask
   if((ColorValue == ColorValuePrv)&&(WCtr < RowLength))PixelsNumber++;  // if prev (color = cur color) and Src block is not end - increment counter of SAMEPIXELs
	 else
	  {
	   // Compress same pixels
	   Compressed   += CompressPixels(DstBuffer,&DstOffset,ColorValuePrv,PixelsNumber,PixelSize,FlagCode);
	   PixelsNumber  = 1;            // New pixels count
	   ColorValuePrv = ColorValue;   // New pixel Base
	  }
  }
   while(WCtr < RowLength);
 return Compressed; // Size of compressed block in bytes (May be larger)
}
//===========================================================================
//
//                      COMPRESS SAME PIXELS BLOCK
//
//---------------------------------------------------------------------------
// DstBuffer    - (in)     Compressed data stores there
// DstOffset    - (in/out) Bytes Offset in DstBuffer for new compressed data block
// ColorValue   - (in)     Pixel color to be compressed
// PixelsNumber - (in)     Number of pixels to collapse
// PixelSize    - (in)     Size of duplicated block for replace in bytes (32bit pixel = 4 bytes, 24bit pixel = 3 bytes, 16bit pixel = 2 bytes, 8bit pixel = 1 byte)
// FlagCode     - (in)     Flag paced before block to expand
//
// RETURNS: Compressed data size in bytes
//---------------------------------------------------------------------------
DWORD _stdcall CBIGBITMAP::CompressPixels(PVOID DstBuffer,DWORD *DstOffset,DWORD ColorValue,DWORD PixelsNumber,DWORD PixelSize,DWORD FlagCode)
{
 DWORD Remainder;
 DWORD CtrModSize;
 DWORD ModeAndCtr;
 DWORD Compressed;

 Compressed = 0;
 while(PixelsNumber)
  {
   if((ModeAndCtr = CreateModeAndCounter(ColorValue,PixelsNumber,PixelSize,FlagCode,&Remainder,&CtrModSize)))
	{
	 // Write compressed pixels
	 Compressed  += CtrModSize;
	 ((DWORD*)(&((BYTE*)DstBuffer)[(*DstOffset)]))[0] = FlagCode;
	 (*DstOffset) += PixelSize;              // FlagSize equals PixelSize
	 ((DWORD*)(&((BYTE*)DstBuffer)[(*DstOffset)]))[0] = ModeAndCtr;
     // Check if need write pixel (or not, if pixel=flag)
	 if(!(ModeAndCtr & 0x00000002))
      {
       // Write normal expanding pixel
       (*DstOffset) += (CtrModSize-(PixelSize*2)); // Size of counter
       ((DWORD*)(&((BYTE*)DstBuffer)[(*DstOffset)]))[0] = ColorValue;
       (*DstOffset) += PixelSize;
      }
       else (*DstOffset) += (CtrModSize-PixelSize); // Size of counter
	}
	 else
	  {
	   // Write direct pixels (compression not effective, or pixel=flag)
	   for(DWORD ctr = 0;ctr < PixelsNumber;ctr++){((DWORD*)(&((BYTE*)DstBuffer)[(*DstOffset)]))[0] = ColorValue;(*DstOffset) += PixelSize;}   // Order of bytes in ColorValue ?????!!!!
	   Compressed    = (PixelsNumber*PixelSize);
	  }
   PixelsNumber = Remainder;  // Create from remainder a new group - if same pixels number greater than counter resolution
  }
 return Compressed;  // Size of compressed block in bytes (May be larger)
}
//===========================================================================
//
//                   CREATE COUNTER AND MODE IN DWORD
//
//---------------------------------------------------------------------------
// ColorValue   - (in)  Pixel color to be compressed
// PixelsNumber - (in)  Number of pixels to collapse
// PixelSize    - (in)  Size of duplicated block for replace in bytes (32bit pixel = 4 bytes, 24bit pixel = 3 bytes, 16bit pixel = 2 bytes, 8bit pixel = 1 byte)
// FlagCode     - (in)  Flag paced before block to expand
// Remainder    - (out) if pixels number greater than counter resolution
// CtrModSize   - (out) Size of compressed pixels
//
// RETURNS:  COUNTER_MODE if compression effective
//---------------------------------------------------------------------------
DWORD _stdcall CBIGBITMAP::CreateModeAndCounter(DWORD ColorValue,DWORD PixelsNumber,DWORD PixelSize,DWORD FlagCode,DWORD *Remainder,DWORD *CtrModSize)
{
 DWORD InfoAndCtr;

 (*Remainder)  = 0;
 (*CtrModSize) = 1;
 // Create Repeats counter
 if(PixelsNumber > 0x0000003F) // if counter uses > 6 bit (1 byte, with 2 mode bits)
  {
   // Need additional counter
   if(PixelsNumber > 0x00000FFF)(*CtrModSize)++;  // Counter 3 bytes
   if(PixelsNumber > 0x000FFFFF)(*CtrModSize)++;  // Counter 4 bytes
   if(PixelsNumber > 0x0FFFFFFF){(*Remainder) = (PixelsNumber-0x0FFFFFFF);PixelsNumber = 0x0FFFFFFF;} // Pixels number exeeds counter resolution  (Counter 4 bytes)
   InfoAndCtr = ((((*CtrModSize)<<2)|0x00000001)|(PixelsNumber<<4));  // (COUNTER(Xbit)_CTRSIZE(2bit)_MODE(2bit))
   (*CtrModSize)++;  // For mode byte
  }
   else InfoAndCtr = (PixelsNumber<<2);  // 1 Byte Counter, InfoAndCtr(NNNNNNXX), 6bit counter(N),X-Mode=0
 // Check if data block matched with the flag
 if(ColorValue == FlagCode){(*CtrModSize) += PixelSize;InfoAndCtr = (InfoAndCtr|0x00000002);return InfoAndCtr;} // Only size of flag, return always (need mark same pixel as flag)
   else {(*CtrModSize) += (PixelSize*2);if((*CtrModSize) < PixelsNumber)return InfoAndCtr;}   // Size of flag and size of pixel, if compression is effective - exit
 return 0; // Compression will not be effective
}
//===========================================================================
void inline CBIGBITMAP::ProcGuardEnter(void)
{
 EnterCriticalSection(&CSecRowsMap);
 //
 // Set Exception handler
 //
}
//===========================================================================
void inline CBIGBITMAP::ProcGuardExit(void)
{
 LeaveCriticalSection(&CSecRowsMap);
 //
 // Remove Exception handler
 //
}
//===========================================================================

