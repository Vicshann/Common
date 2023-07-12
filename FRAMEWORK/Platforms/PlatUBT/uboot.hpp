
struct NUBOOT
{

enum class EApiIdx: int    // In order of UBOOT`s jump table (generic)
{
 get_version,
 getc,
 tstc,
 putc,
 puts,
 printf,
 install_hdlr,    // null
 free_hdlr,       // null
 malloc,
 free,
 udelay,
 get_timer,
 vprintf,
 do_reset,
 getenv,
 setenv,
 simple_strtoul,
 strict_strtoul,
 simple_strtol,
 strcmp,
 i2c_write,
 i2c_read
};
//-----------------------------------------------------------------------------------------
// Jump Table functions
static uint   get_version(void);
static int    getc(void);
static int    tstc(void);
static void   putc(const achar chr);
static void   puts(const achar* str);
static int    printf(const achar* fmt, ...);
static int    printfNV(const achar* fmt);   // For variadic stub only!

static void   install_hdlr(int, void (*interrupt_handler_t)(vptr), vptr);
static void   free_hdlr(int);

static vptr   malloc(size_t size);
static void   free(vptr ptr);

static void   udelay(uint delay);
static uint   get_timer(uint base);
static int    vprintf(const achar* fmt, vptr);
static void   do_reset(void);    // noreturn      // used by bootm
static achar* getenv(const achar* name);
static int    setenv (const achar* varname, const achar* varvalue);

static uint   simple_strtoul(const achar* cp, achar** endp, uint32 base);
static int    strict_strtoul(const achar* cp, uint32 base, uint* res);
static sint   simple_strtol(const achar* cp, achar** endp, uint32 base);
static int    strcmp(const achar* cs, const achar* ct);
//static uint ustrtoul(const char *cp, char **endp, unsigned int base);
//static uint long ustrtoull(const char *cp, char **endp, unsigned int base);
static int    i2c_write (uint8, uint, int , uint8* , int);
static int    i2c_read (uint8, uint, int , uint8* , int);

//-----------------------------------------------------------------------------------------
// Internal Functions
static bool   ExecCmdLine(const achar* CmdLine, uint8 Flags);

//-----------------------------------------------------------------------------------------
};
