
#pragma once


// Platform dependant (UBOOT,EFI,WASM have some of this exported)
// Some of useful functions, similair to ones in LIBC
namespace LIBC
{
//------------------------------------------------------------------------------------------------------------
// NOTE: Be aware that some platforms use CR+LF
// NOTE: Will not behave as expected on nonblocking descriptor (waiting for user input)
// NOTE: No need to lock for STDIN/STDOUT ?
//
static sint _finline getc(bool lock=false, NPTM::PX::fdsc_t fd=NPTM::GetStdIn())
{
 achar buf = 0;
 int lr = 0;
 if(lock)lr = NPTM::NAPI::flock(fd, NPTM::PX::LOCK_EX);   // LOCK_NB ?
 sint res = NPTM::NAPI::read(fd, &buf, 1);
 if(lock && (lr >= 0))NPTM::NAPI::flock(fd, NPTM::PX::LOCK_UN);
 return (res > 0)?((sint)buf):res;
}
//------------------------------------------------------------------------------------------------------------
// Please don`t send entire strings by using this function:)
static sint _finline putc(achar val, bool lock=false, NPTM::PX::fdsc_t fd=NPTM::GetStdOut())
{
 int lr = 0;
 if(lock)lr = NPTM::NAPI::flock(fd, NPTM::PX::LOCK_EX);   // LOCK_NB ?
 sint res = NPTM::NAPI::write(fd, &val, 1);
 if(lock && (lr >= 0))NPTM::NAPI::flock(fd, NPTM::PX::LOCK_UN);
 return res;
}
//------------------------------------------------------------------------------------------------------------
static sint puts(const achar* buf, size_t len=(size_t)-1, bool lock=false, NPTM::PX::fdsc_t fd=NPTM::GetStdOut())
{
 int lr = 0;
 if(len == (size_t)-1)len = NSTR::StrLen(buf);
 if(lock)lr = NPTM::NAPI::flock(fd, NPTM::PX::LOCK_EX);   // LOCK_NB ?
 sint res = NPTM::NAPI::write(fd, buf, len);
 if(lock && (lr >= 0))NPTM::NAPI::flock(fd, NPTM::PX::LOCK_UN);
 return res;
}
//------------------------------------------------------------------------------------------------------------
// Inefficient as it is:)
static sint gets(achar* buf, size_t len, bool lock=false, NPTM::PX::fdsc_t fd=NPTM::GetStdIn())
{
 if(len < 1)return 0;
 int lr = 0;
 achar val = 0;
 uint idx = 0;
 for(len--;idx < len;)
  {
   sint res = NPTM::NAPI::read(fd, &val, 1);  // Have to read chars one by one, unfortunately. (no pipe peeking on NIX)
   if(res <= 0)break;     // Error or end-of-file (closed pipe)
   buf[idx++] = val;
   if(val == 0x0A)break;  // EOL
  }
 buf[idx] = 0;  // Write terminating 0
 return idx;    // Return number of actually read chars
}
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------
};



