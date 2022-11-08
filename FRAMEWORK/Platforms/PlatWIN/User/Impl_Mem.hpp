#pragma once

//============================================================================================================
static void* PXCALL mmap(void* addr, size_t length, int prot, int flags, int fd, size_t offset)
{
 return 0;
}
//------------------------------------------------------------------------------------------------------------
// Expect it in '/dev/shm'
//
static int PXCALL shm_open(const achar* name, int oflag, PX::mode_t mode)
{
// oflag |= O_NOFOLLOW | O_CLOEXEC;
 return 0;
}
//============================================================================================================
