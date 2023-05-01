
#pragma once

static const uint MEMPAGESIZE = 0x00001000;  // 4K       // ARM?
static const uint MEMGRANSIZE = 0x00001000;  // 64K      // ARM?
static const uint MEMADDRBASE = 0x00010000;  // Base addr for allocations  // Nix?
static const uint CPUCACHELNE = 64;          // x86/ARM (Most models)
static const uint PATH_MAX    = 1024;        // Chars in a path name including nul  // 4096 in limits.h
static const uint PIPE_BUF    = 4096;        // Bytes in atomic write to a pipe
//---------------------------------------------------------------------------
