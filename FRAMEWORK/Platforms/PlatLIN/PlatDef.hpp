
#pragma once

SCVR uint MEMPAGESIZE = NCFG::MemPageSize?NCFG::MemPageSize:0x00001000;  // 4K       // ARM?
SCVR uint MEMGRANSIZE = NCFG::MemGranSize?NCFG::MemGranSize:0x00001000;  // 64K      // ARM?
SCVR uint MEMADDRBASE = 0x00010000;  // Base addr for allocations  // Nix?
SCVR uint CPUCACHELNE = 64;          // x86/ARM (Most models)
SCVR uint PATH_MAX    = 1024;        // Chars in a path name including nul  // 4096 in limits.h
SCVR uint PIPE_BUF    = 4096;        // Bytes in atomic write to a pipe
//---------------------------------------------------------------------------
