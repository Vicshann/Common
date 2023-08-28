
#pragma once

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wcast-align"

struct NFMT
{

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
enum EFlags {
 FLAGS_ZEROPAD   = (1U <<  0U),
 FLAGS_LEFT      = (1U <<  1U),
 FLAGS_PLUS      = (1U <<  2U),
 FLAGS_SPACE     = (1U <<  3U),
 FLAGS_HASH      = (1U <<  4U),
 FLAGS_UPPERCASE = (1U <<  5U),
 FLAGS_CHAR      = (1U <<  6U),
 FLAGS_SHORT     = (1U <<  7U),
 FLAGS_LONG      = (1U <<  8U),
 FLAGS_LONG_LONG = (1U <<  9U),
 FLAGS_PRECISION = (1U << 10U),
 FLAGS_ADAPT_EXP = (1U << 11U)
};

//---------------------------------------------------------------------------
static constexpr _finline int8 ValIdxToSize(uint8 ValIdx)
{
 return 1 << ValIdx;
}
//---------------------------------------------------------------------------
// For values of 1,2,4,8 bytes
static consteval _finline int8 ValSizeToIdx(const int ValSize)
{
//                0  1  2   3  4   5   6   7  8
// int8 sizes[] = {-1, 0, 1, -1, 2, -1, -1, -1, 3};

 if(ValSize==2)return 1;      // 01
 else if(ValSize==4)return 2; // 10
 else if(ValSize==8)return 3; // 11
 return 0;
}
//---------------------------------------------------------------------------
// Max 32 elements is supported. Last element is skipped (expected to be a terminating 0)
template<typename T, uint N>static consteval _finline uint64 PackSizeBits(const T(&&arr)[N])
{
 uint64 res = 0;
 for(int idx=0;idx < (N-1);idx++)res |= (uint64)ValSizeToIdx(arr[idx]) << (idx << 1);
 return res;
}
//---------------------------------------------------------------------------
// Allows 255 items for ArgNum
static consteval size_t MakeArgCtrVal(uint ArgNum, uint64 SizeBits)
{
 if constexpr (IsArchX32)return ArgNum | ((SizeBits >> (sizeof(size_t)*8)) << 8);        // On x32, gets high 32 bits and stores them in the counter value
 return ArgNum;
}
//---------------------------------------------------------------------------
/*static consteval _finline int8 GetValSizeIdx(auto&& Value)
{
 using VT = typename RemoveConst<typename RemoveRef<decltype(Value)>::T>::T;  // May be const
 if constexpr (sizeof(VT)==2)return 1;      // 01
 else if constexpr (sizeof(VT)==4)return 2; // 10
 else if constexpr (sizeof(VT)==8)return 3; // 11
 return 0;
} */
//---------------------------------------------------------------------------
// Returns take address of an ref objects but returns pointers as is
// Needed because conditional expressions do not work with incompatible types which is required in folding: {((IsPtrType<RemoveRef<decltype(args)>::T>::V)?(args):(&args))...};
static constexpr _finline vptr GetValAddr(auto&& Value)
{
 using VT = typename RemoveConst<typename RemoveRef<decltype(Value)>::T>::T;  // May be const
 if constexpr (IsPtrType<VT>::V)return (vptr)Value;
 return (vptr)&Value;
}
//---------------------------------------------------------------------------
static _finline vptr* DecodeArgArray(vptr* ArgArr, uint& ArgNum, uint64& SizeBits)
{
 ArgNum = (size_t)ArgArr[0] & 0xFF;
 if constexpr (IsArchX32)SizeBits = ((uint64)((size_t)ArgArr[0] >> 8) << 32)|(size_t)ArgArr[1];
  else SizeBits = (size_t)ArgArr[1];
 return &ArgArr[2];
}
//---------------------------------------------------------------------------
// NOTE: T is type of argument is stored in ArgList, no type of argument to be read into
template<typename T> static constexpr _finline T GetArgAsType(uint& ArgIdx, void** ArgList, uint64 SizeBits)   // TODO: Make lambda and capture ArgList
{
 if constexpr (IsPtrType<T>::V)return (T)ArgList[ArgIdx++];  // Read it as a pointer itself, not pointer to a pointer variable
  else
  {
   int asize = (SizeBits >> (ArgIdx << 1)) & 3;   //ValIdxToSize((SizeBits >> (ArgIdx << 1)) & 3);
// Recover as floats
   if constexpr (SameTypes<T, flt32>::V)
    {
     if(asize == 2)return *(flt32*)ArgList[ArgIdx++];   // Read as is
    }
   else if constexpr (SameTypes<T, flt64>::V)
    {
     if(asize == 3)return *(flt64*)ArgList[ArgIdx++];   // Read as is
      else if(asize == 2)return *(flt32*)ArgList[ArgIdx++];   // Read as is
    }
// Recover as integers
   if constexpr((T)-1 < 0)
    {
     if(asize == 0)return *(int8*)ArgList[ArgIdx++];
     else if(asize == 1)return *(int16*)ArgList[ArgIdx++];
     else if(asize == 2)return *(int32*)ArgList[ArgIdx++];
     else if(asize == 3)return *(int64*)ArgList[ArgIdx++];
    }
   else
    {
     if(asize == 0)return *(uint8*)ArgList[ArgIdx++];
     else if(asize == 1)return *(uint16*)ArgList[ArgIdx++];
     else if(asize == 2)return *(uint32*)ArgList[ArgIdx++];
     else if(asize == 3)return *(uint64*)ArgList[ArgIdx++];
    }
  }
 return 0;  // Should not be reached
}
//---------------------------------------------------------------------------
// Can store sizes for max 32 values
// DO NOT USE! - SizeBits calrulation won`t be at compile time
//
/*static constexpr _finline void* GetValAddrAndSizeUpd(auto&& Value, uint64& SizeBits)
{
 using VT = typename RemoveConst<typename RemoveRef<decltype(Value)>::T>::T;  // May be const
 if constexpr (IsPtrType<VT>::V)return (void*)Value;

 SizeBits <<= 2;
 if(sizeof(VT)==2)SizeBits |= 1;      // 01
 else if(sizeof(VT)==4)SizeBits |= 2; // 10
 else if(sizeof(VT)==8)SizeBits |= 3; // 11
 return (void*)&Value;
}  */
//---------------------------------------------------------------------------
static inline bool _is_digit(char ch)
{
 return (ch >= '0') && (ch <= '9');
}
//---------------------------------------------------------------------------
// internal ASCII string to unsigned int conversion
static inline unsigned int _atoi(char** Str)
{
 unsigned int x = 0;
 for(unsigned char ch;(ch=*(*Str) - '0') <= 9;(*Str)++)x = (x*10) + ch;        // Can use a different base?
 return x;
}
//---------------------------------------------------------------------------
template<typename T> static inline unsigned int _strnlen_s(T str, size_t maxsize)
{
 int constexpr BSh = (sizeof(str[0]) > 1)?1:0;   // 2 or the same
 if(!str)return 0;
 T s = str;
 for (; *s && maxsize--; ++s);
 return (unsigned int)(((char*)s - (char*)str) >> BSh);
}
//---------------------------------------------------------------------------
template<typename T> static size_t _ntoa(char* buffer, size_t idx, size_t maxlen, T value, bool negative, T base, unsigned int prec, unsigned int width, unsigned int flags)
{
 char buf[32];
 size_t len = 0U;
 if (!value)flags &= ~FLAGS_HASH;   // no hash for 0 values
 // write if precision != 0 and value is != 0
 if (!(flags & FLAGS_PRECISION) || value)  // Reversed
 {
  if(base == 2)   // Binary
   {
    do {
     buf[len++] = '0' + (value & 1);  // Mod 2
     value >>= 1;  // div 2
    } while (value && (len < sizeof(buf)));
   }
  else if(base == 8)   // Octal
   {
    do {
     buf[len++] = '0' + (value & 7);  // mod 8
     value >>= 3;  // div 8
    } while (value && (len < sizeof(buf)));
   }
  else if(base == 10)   // Decimal
   {
    do {
     T vmod;
     value = NMATH::DivMod10U(value, vmod);
     buf[len++] = '0' + vmod;
    } while (value && (len < sizeof(buf)));
   }
  else  // Hexadecimal
   {
    char BaseCh = (flags & FLAGS_UPPERCASE ? 'A' : 'a');
    do {
     char digit = (char)(value & 15);  // mod 16
     buf[len++] = digit < 10 ? '0' + digit : BaseCh + digit - 10;
     value >>= 4;  // div 16
    } while (value && (len < sizeof(buf)));
   }

  /*  do {
      const char digit = (char)(value % base);    // Not cheap!!!
      buf[len++] = digit < 10 ? '0' + digit : (flags & FLAGS_UPPERCASE ? 'A' : 'a') + digit - 10;
      value /= base;
    } while (value && (len < sizeof(buf))); */

  }

  // pad leading zeros
  if (!(flags & FLAGS_LEFT)) {
    if (width && (flags & FLAGS_ZEROPAD) && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE))))width--;
    while ((len < prec) && (len < sizeof(buf)))buf[len++] = '0';
    while ((flags & FLAGS_ZEROPAD) && (len < width) && (len < sizeof(buf)))buf[len++] = '0';
  }

  // handle hash
  if (flags & FLAGS_HASH) {
    if (!(flags & FLAGS_PRECISION) && len && ((len == prec) || (len == width)) && (--len && (base == 16U)))len--;
    if ((base == 16U) && !(flags & FLAGS_UPPERCASE) && (len < sizeof(buf)))buf[len++] = 'x';
    else if ((base == 16U) && (flags & FLAGS_UPPERCASE) && (len < sizeof(buf)))buf[len++] = 'X';
    else if ((base == 2U) && (len < sizeof(buf)))buf[len++] = 'b';
    if(len < sizeof(buf))buf[len++] = '0';
  }

  if (len < sizeof(buf)) {
    if (negative)buf[len++] = '-';
    else if (flags & FLAGS_PLUS)buf[len++] = '+';  // ignore the space if the '+' exists
    else if (flags & FLAGS_SPACE)buf[len++] = ' ';
  }

  const size_t start_idx = idx;
  // pad spaces up to given width
  if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {
    for (size_t i = len; i < width; i++)buffer[idx++] = ' ';   // out(' ', buffer, idx++, maxlen);
  }

  // reverse string
  if((idx+len) > maxlen)return idx;     // ???
  while (len)buffer[idx++] = buf[--len];

  // append pad spaces up to given width
 if (flags & FLAGS_LEFT){while (idx - start_idx < width)buffer[idx++] = ' ';}    // out(' ', buffer, idx++, maxlen);    //    if(((width - (idx - start_idx)) + idx) > maxlen)   // Too costlly to check
 return idx;
}
//---------------------------------------------------------------------------
// TODO: Put this and all num-to-str/str-to-num functions into Format.hpp (mamespace NFMT)
// TODO: Use a template to generate a type validation string from all passed arguments
// TODO: Repeated chars
// TODO: Indexed arg reuse
// Arg sizes(bin): 00=1,01=2,10=4,11=8
// ArgList format:
//  size_t ArgNum
//  size_t ArgSizes;  // Max 28(x32)/32(x64) (may be extended in the future)
//  void*  Args...
//
_ninline static sint FormatToBuffer(char* format, char* buffer, uint maxlen, vptr* ArgList)
{
 size_t idx = 0U;
 uint ArgIdx = 0;
 uint ArgNum;
 uint64 SizeBits;
 ArgList = DecodeArgArray(ArgList, ArgNum, SizeBits);
 while(*format && (idx < (size_t)maxlen))   // format specifier?  %[flags][width][.precision][length]
  {
   // Check if the next 4 bytes contain %(0x25) or end of string. Using the 'hasless' trick: https://graphics.stanford.edu/~seander/bithacks.html#HasLessInWord
//   v = *(stbsp__uint32 *)f;
//   c = (~v) & 0x80808080;
//   if (((v ^ 0x25252525) - 0x01010101) & c)goto schk1;
//   if ((v - 0x01010101) & c)goto schk2;

   if(*format != '%'){buffer[idx++]=*(format++); continue;}
   format++;

   // evaluate flags
   unsigned int n;
   unsigned int flags = 0U;
    do { switch(*format) {
        case '0': flags |= FLAGS_ZEROPAD; format++; n = 1U; break;
        case '-': flags |= FLAGS_LEFT;    format++; n = 1U; break;
        case '+': flags |= FLAGS_PLUS;    format++; n = 1U; break;
        case ' ': flags |= FLAGS_SPACE;   format++; n = 1U; break;
        case '#': flags |= FLAGS_HASH;    format++; n = 1U; break;
        default :                                   n = 0U; break;
      } } while (n);

    // evaluate width field
    unsigned int width = 0U;
    if (_is_digit(*format))width = _atoi(&format);
    else if (*format == '*') {
      const int w = GetArgAsType<int>(ArgIdx, ArgList, SizeBits);  // va_arg(va, int);
      if (w < 0) {flags |= FLAGS_LEFT; width = (unsigned int)-w;}    // reverse padding
      else  width = (unsigned int)w;
      format++;
    }

    // evaluate precision field
    unsigned int precision = 0U;
    if (*format == '.') {
      flags |= FLAGS_PRECISION;
      format++;
      if (_is_digit(*format))precision = _atoi(&format);
      else if (*format == '*') {
        const int prec = GetArgAsType<int>(ArgIdx, ArgList, SizeBits);  // (int)va_arg(va, int);
        precision = prec > 0 ? (unsigned int)prec : 0U;
        format++;
      }
    }

    // evaluate length field
    switch (*format) {
      case 'l' :
        if(*(++format) == 'l'){flags |= FLAGS_LONG_LONG; format++;}
          else flags |= FLAGS_LONG;
        break;
      case 'h':
        if(*(++format) == 'h'){flags |= FLAGS_CHAR; format++;}
          else flags |= FLAGS_SHORT;
        break;
/*      case 't' :
        flags |= (sizeof(ptrdiff_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      case 'j' :
        flags |= (sizeof(intmax_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;  */
      case 'z' :
        flags |= (sizeof(size_t) == sizeof(long) ? FLAGS_LONG : FLAGS_LONG_LONG);
        format++;
        break;
      default :
        break;
    }

    switch (*format) {       // evaluate specifier
//      case 'd' :
      case 'i' :
      case 'u' :
      case 'x' :
      case 'X' :
      case 'o' :
      case 'b' : {
        // set the base
        unsigned int base;
        if (*format == 'x' || *format == 'X')base = 16U;
        else if (*format == 'o')base = 8U;
        else if (*format == 'b')base = 2U;
        else {base = 10U; flags &= ~FLAGS_HASH;}   // no hash for dec format

        if (*format == 'X')flags |= FLAGS_UPPERCASE;     // uppercase
        if (*format != 'i')flags &= ~(FLAGS_PLUS | FLAGS_SPACE);    // no plus or space flag for u, x, X, o, b   //  && (*format != 'd')
        // ignore '0' flag when precision is given
        if (flags & FLAGS_PRECISION)flags &= ~FLAGS_ZEROPAD;
        // convert the integer
        if (*format == 'i') {   // signed         //  || (*format == 'd')
          if (flags & FLAGS_LONG_LONG) {
            const long long value = GetArgAsType<long long>(ArgIdx, ArgList, SizeBits);  // va_arg(va, long long);
            idx = _ntoa<unsigned long long>(buffer, idx, maxlen, (unsigned long long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
          else if (flags & FLAGS_LONG) {
            const long value = GetArgAsType<long>(ArgIdx, ArgList, SizeBits);  // va_arg(va, long);
            idx = _ntoa<unsigned long>(buffer, idx, maxlen, (unsigned long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
          else {
            const int value = (flags & FLAGS_CHAR) ? GetArgAsType<char>(ArgIdx, ArgList, SizeBits) : (flags & FLAGS_SHORT) ? GetArgAsType<short int>(ArgIdx, ArgList, SizeBits) : GetArgAsType<int>(ArgIdx, ArgList, SizeBits);
            idx = _ntoa<unsigned long>(buffer, idx, maxlen, (unsigned int)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
        }
        else {   // unsigned
          if (flags & FLAGS_LONG_LONG)idx = _ntoa<unsigned long long>(buffer, idx, maxlen, GetArgAsType<unsigned long long>(ArgIdx, ArgList, SizeBits), false, base, precision, width, flags);   // va_arg(va, unsigned long long)
          else if (flags & FLAGS_LONG)idx = _ntoa<unsigned long>(buffer, idx, maxlen, GetArgAsType<unsigned long>(ArgIdx, ArgList, SizeBits), false, base, precision, width, flags);    // va_arg(va, unsigned long)
          else {
            const unsigned int value = (flags & FLAGS_CHAR) ? GetArgAsType<unsigned char>(ArgIdx, ArgList, SizeBits) : (flags & FLAGS_SHORT) ? GetArgAsType<unsigned short int>(ArgIdx, ArgList, SizeBits) : GetArgAsType<unsigned int>(ArgIdx, ArgList, SizeBits);
            idx = _ntoa<unsigned long>(buffer, idx, maxlen, value, false, base, precision, width, flags);
          }
        }
        format++;
        break;
      }
#ifndef FWK_NO_FPU
      case 'f' :
      case 'F' : {
        size_t prec = (flags & FLAGS_PRECISION)?precision:14;   // Is 14 optimal?
        if(*format == 'F') {flags |= FLAGS_UPPERCASE; prec |= 0x1000;}  // ffUpperCase
        if(flags & FLAGS_HASH)prec |= 0x8000;       // ffZeroPad                      // ffCommaSep  // Or may be ffZeroPad 0x8000?
        size_t len = 0;
        char buf[52];
        char* ptr = NCNV::ftoa_simple(GetArgAsType<double>(ArgIdx, ArgList, SizeBits), prec, buf, sizeof(buf), &len);   // After this we have some space at beginning of the buffer    // va_arg(va, double)
        bool negative = (*ptr == '-');
        if (!(flags & FLAGS_LEFT) && (flags & FLAGS_ZEROPAD)) {
          if (width && (negative || (flags & (FLAGS_PLUS | FLAGS_SPACE)))) width--;
          while ((len < width) && (len < sizeof(buf))){*(--ptr) = '0'; len++;}   // Width adds zeroes BEFORE
        }
        if ((len < sizeof(buf)) && !negative) {
          if (flags & FLAGS_PLUS){*(--ptr) = '+'; len++;} // ignore the space if the '+' exists
          else if (flags & FLAGS_SPACE){*(--ptr) = ' '; len++;}
        }
        const size_t start_idx = idx;
        if (!(flags & FLAGS_LEFT) && !(flags & FLAGS_ZEROPAD)) {    // pad spaces up to given width         // TODO: CopyPadded inline function
          for (size_t i = len; i < width; i++)buffer[idx++] = ' '; }  // out(' ', buffer, idx++, maxlen);
        while(len--)buffer[idx++] = *(ptr++);       //out(buf[--len], buffer, idx++, maxlen); // reverse string
        if (flags & FLAGS_LEFT) {     // append pad spaces up to given width
          while (idx - start_idx < width)buffer[idx++] = ' '; }  // out(' ', buffer, idx++, maxlen);
        format++; }
        break;
#endif
#if defined(PRINTF_SUPPORT_EXPONENTIAL)
      case 'e':
      case 'E':
      case 'g':
      case 'G':
        if ((*format == 'g')||(*format == 'G')) flags |= FLAGS_ADAPT_EXP;
        if ((*format == 'E')||(*format == 'G')) flags |= FLAGS_UPPERCASE;
        idx = _etoa(out, buffer, idx, maxlen, va_arg(va, double), precision, width, flags);
        format++;
        break;
#endif  // PRINTF_SUPPORT_EXPONENTIAL
      case 'c' : {
        unsigned int l = 1U;
        if (!(flags & FLAGS_LEFT)){while (l++ < width)buffer[idx++] = ' ';}  // pre padding         //  TODO: Buffer limit
        buffer[idx++] = GetArgAsType<char>(ArgIdx, ArgList, SizeBits);  // va_arg(va, int);  // char output       // out((char)va_arg(va, int), buffer, idx++, maxlen);
        if (flags & FLAGS_LEFT){while (l++ < width)buffer[idx++] = ' ';}    // post padding
        format++;
        break;
      }

      case 's' : {
        char* cp;
        wchar_t* wp;
        unsigned int l;
        if(flags & FLAGS_LONG)
         {
          wp = GetArgAsType<wchar_t*>(ArgIdx, ArgList, SizeBits);  // va_arg(va, wchar_t*);
          l  = _strnlen_s(wp, precision ? precision : (size_t)-1);    // precision or a full size
         }
        else
         {
          cp = GetArgAsType<char*>(ArgIdx, ArgList, SizeBits);  // va_arg(va, char*);
          l  = _strnlen_s(cp, precision ? precision : (size_t)-1);    // precision or a full size
         }
        unsigned int f = l;    // Full len  of the string
        if(flags & FLAGS_PRECISION) f = l = (l < precision ? l : precision);    // Not greater than precision or 0
        if(!(flags & FLAGS_LEFT)){while (l++ < width)buffer[idx++] = ' ';}      // pre padding
        if(flags & FLAGS_LONG)idx += NUTF::Utf16To8(buffer, wp, f, idx, 0);
         else {for (;f;f--)buffer[idx++] = *(cp++);}   // string output         //  out(*(p++), buffer, idx++, maxlen);  // while ((*p != 0) && (!(flags & FLAGS_PRECISION) || precision--))buffer[idx++] = *(p++);
        if(flags & FLAGS_LEFT){while (l++ < width)buffer[idx++] = ' ';}         // post padding
        format++;
        break;
      }

      case 'd' :                     // LOGMSG("\r\n%#*.32D",Size,Src);
      case 'D' : {   // Width is data block size, precision is line size(single line if not specified)  // '%*D' counted dump
        if(*format == 'D')flags |= FLAGS_UPPERCASE;
        unsigned char* DPtr  = GetArgAsType<unsigned char*>(ArgIdx, ArgList, SizeBits);  // va_arg(va, unsigned char*);
        unsigned int   RLen  = precision?precision:width;   // If no precision is specified then write everything in one line
        unsigned int DelMult = (flags & FLAGS_SPACE)?3:2;
        for(unsigned int offs=0,roffs=0,rpos=0;idx < maxlen;offs++,roffs++)
         {
          bool HaveMore = offs < width;
          if(!HaveMore || (roffs >= RLen))
           {
            if(flags & FLAGS_HASH)   // Include char dump
             {
              unsigned int IndCnt = (flags & FLAGS_SPACE)?1:2;
              IndCnt += (RLen - roffs) * DelMult;  // Indent missing HEX space     //if(!SingleLine)
              if((idx+IndCnt) > maxlen)goto Exit;    // No more space
              memset(&buffer[idx], ' ', IndCnt);
              idx += IndCnt;
              for(unsigned int ctr=0;(ctr < roffs)&&(idx < maxlen);ctr++)   // Create Text string
               {
                unsigned char Val = DPtr[rpos+ctr];
                if(Val < 0x20)Val = '.';
                buffer[idx++] = int8(Val);
               }
             }
            if(!HaveMore || ((idx+4) > maxlen))break;
            buffer[idx++] = '\r';      // Add only if there is another line follows
            buffer[idx++] = '\n';
            rpos  = offs;
            roffs = 0;
           }
          //unsigned short Val = NCNV::ByteToHexChar(DPtr[offs], true);   // Case flag?
          //buffer[idx++] = int8(Val);
          //buffer[idx++] = int8(Val >> 8);
          *(uint16*)&buffer[idx] = NCNV::ByteToHexChar(DPtr[offs], flags & FLAGS_UPPERCASE);  // Unaligned but should be faster
          idx += 2;
          if(flags & FLAGS_SPACE)buffer[idx++] = ' ';
         }
        format++;
        break; }

      case 'p' :       // NOTE: If a nonpointer type is passed then its address is read instead of its value
      {
//        width = sizeof(void*) * 2U;
        flags |= FLAGS_ZEROPAD | FLAGS_UPPERCASE;
        const bool is_ll = (sizeof(void*) == sizeof(long long)) || (flags & FLAGS_LONG);
        if (is_ll)idx = _ntoa<unsigned long long>(buffer, idx, maxlen, (unsigned long long)GetArgAsType<void*>(ArgIdx, ArgList, SizeBits), false, 16U, precision, width=sizeof(long long)*2, flags);  //  (unsigned long long)va_arg(va, unsigned long long)
          else idx = _ntoa<unsigned long>(buffer, idx, maxlen, (unsigned long)GetArgAsType<void*>(ArgIdx, ArgList, SizeBits), false, 16U, precision, width=sizeof(long)*2, flags); //  (unsigned long)va_arg(va, unsigned long)
        format++;
        break;
      }

      case 'n': {       // Nothing printed. The number of characters written so far is stored in the pointed location.
        int* p = GetArgAsType<int*>(ArgIdx, ArgList, SizeBits);  // va_arg(va, int*);
        *p = idx; }
        break;

      default:
        buffer[idx++] = *(format++);       //out(*format, buffer, idx++, maxlen); format++;
        break;
    }
  }

Exit:
 buffer[idx] = 0;  // out((char)0, buffer, idx < maxlen ? idx : maxlen - 1U, maxlen);
 return (int)idx;
}
//---------------------------------------------------------------------------
// TODO: Fix bad stack alignment at _start (It is misaligned by ABI on NIX and MacOS)
/*__attribute__((__force_align_arg_pointer__)) static int PrintFmt(char* buffer, uint maxlen, char* format, ...)   // How to force CLANG to make sure that stack is always aligned to 16? ('-mstack-alignment=16' have no effect)
{
 va_list  args;
 va_start(args,format);
 int MSize = FormatToBuffer(format, buffer, maxlen, args);
 va_end(args);
 return MSize;
}*/
//---------------------------------------------------------------------------
static sint _finline StrFmt(achar* buffer, uint maxlen, achar* format, auto&&... args)
{
 constexpr uint64 sbits = NFMT::PackSizeBits((int[]){sizeof(args)...,0});  // Last 0 needed in case of zero args number (No zero arrays allowed)
 constexpr size_t arnum = NFMT::MakeArgCtrVal(sizeof...(args), sbits);
 return FormatToBuffer(format, buffer, maxlen, (vptr[]){(vptr)arnum,(vptr)sbits,(NFMT::GetValAddr(args))...});
}
//---------------------------------------------------------------------------

};

#pragma clang diagnostic pop
