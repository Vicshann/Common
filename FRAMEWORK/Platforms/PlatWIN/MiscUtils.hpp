
struct NWIN
{

static sint GetPhysDiskForVolume(achar Vol, sint* Type=nullptr)
{
 achar DrvPath[] = {Vol,':',0}; 

 sint hDrv = NPTM::NAPI::open(DrvPath,NPTM::PX::O_RDONLY,0);
 if(hDrv < 0)return hDrv;
 if(Type)
  {
   NPTM::NT::STORAGE_DEVICE_NUMBER sdn;
   uint32 dwSize = sizeof(sdn);
   *Type = -1;
   sint res = NPTM::NAPI::ioctl(hDrv, NPTM::NT::IOCTL_STORAGE_GET_DEVICE_NUMBER, &sdn, &dwSize);
   if(!res)*Type = sdn.DeviceType; 
  }

 NPTM::NT::VOLUME_DISK_EXTENTS<16> dex;  // 16 should be enough
 uint32 dwSize = sizeof(dex);
 sint res = NPTM::NAPI::ioctl(hDrv, NPTM::NT::IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, &dex, &dwSize);    // Retrieves the physical location of a specified volume on one or more disks. (a volume can span multiple disks)
 NPTM::NAPI::close(hDrv);
 if(res < 0)return res;
 return dex.Extents[0].DiskNumber;
}
//------------------------------------------------------------------------------------------------------------
static sint OpenPhysDiskByIdx(uint DrvIdx, uint64* DriveSize=nullptr, uint32* SecSize=nullptr)
{
 achar buf[32];
 achar DrvPath[32] = {':','P','h','y','s','i','c','a','l','D','r','i','v','e'};   // Note: Max drive 9!!!
 NSTR::StrCopy(&DrvPath[14], NCNV::DecNumToStrU<uint32>(DrvIdx, buf));

 sint hDrv = NPTM::NAPI::open(DrvPath,NPTM::PX::O_RDONLY,0);
 if(hDrv < 0)return hDrv;

 NPTM::NT::DISK_GEOMETRY_EX gex;
 uint32 dwSize = sizeof(gex);
 if(DriveSize || SecSize)
  {
   sint res = NPTM::NAPI::ioctl(hDrv, NPTM::NT::IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, &gex, &dwSize);
   if(res < 0){NPTM::NAPI::close(hDrv); return res;}
   if(DriveSize)*DriveSize = gex.DiskSize;
   if(SecSize)*SecSize = gex.Geometry.BytesPerSector;
  }
 return hDrv;
}
//------------------------------------------------------------------------------------------------------------
static sint OpenPhysDiskByVolume(achar Vol, uint64* DriveSize=nullptr, uint32* SecSize=nullptr)
{
 sint vidx = GetPhysDiskForVolume(Vol);
 if(vidx < 0)return vidx;
 return OpenPhysDiskByIdx((uint)vidx, DriveSize, SecSize);
}
//------------------------------------------------------------------------------------------------------------
// To zero out a range of the file and maintain sparseness, use FSCTL_SET_ZERO_DATA
static sint SetSparseRange(uint hSparseFile, uint64 Start, uint64 Size)    // TODO: Investigate 'fallocate' syscall on Linux
{   
 NPTM::NT::FILE_ZERO_DATA_INFORMATION fzdi;    // Specify the starting and the ending address (not the size) of the sparse zero block
 fzdi.FileOffset = Start;
 fzdi.BeyondFinalZero = Start + Size;
 uint32 dwSize = sizeof(fzdi);
 return NPTM::NAPI::fcntl(hSparseFile, NPTM::NT::FSCTL_SET_ZERO_DATA, &fzdi, &dwSize);
}
//------------------------------------------------------------------------------------------------------------
static sint CreateSparseFile(const achar* FilePath, uint64 Size)
{
 sint hFile = NPTM::NAPI::open(FilePath,NPTM::PX::O_CREAT|NPTM::PX::O_RDWR,0);
 if(hFile < 0)return hFile;
 uint32 dwSize = 0;
 sint res = NPTM::NAPI::fcntl(hFile, NPTM::NT::FSCTL_SET_SPARSE, nullptr, &dwSize);
 if(res < 0){NPTM::NAPI::close(hFile); return res;}
 res = NPTM::NAPI::ftruncate(hFile, Size); 
 if(res < 0){NPTM::NAPI::close(hFile); return res;}
 res = SetSparseRange(hFile, 0, Size);
 if(res < 0){NPTM::NAPI::close(hFile); return res;}
 return hFile;
}
//------------------------------------------------------------------------------------------------------------



};