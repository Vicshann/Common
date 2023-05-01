
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
     value = NMATH::UDivMod10(value, vmod);
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
// NOTE: T is type of argument is stored in ArgList, no type of argument to be read into
template<typename T> static constexpr _finline T GetArgAsType(uint& ArgIdx, void** ArgList)   // TODO: Make lambda and capture ArgList
{
 if constexpr (IsPtrType<T>::V)return (T)ArgList[ArgIdx++];  // Read it as a pointer itself, not pointer to a pointer variable
 return *(T*)ArgList[ArgIdx++];
}
//---------------------------------------------------------------------------
// TODO: Put this and all num-to-str/str-to-num functions into Format.hpp (mamespace NFMT)
// TODO: Use a template to generate a type validation string from all passed arguments
// TODO: Repeated chars
// TODO: Indexed arg reuse
//
_ninline static sint FormatToBuffer(char*  format, char* buffer, uint maxlen, uint ArgNum, void** ArgList)
{
 size_t idx = 0U;
 uint ArgIdx = 0;
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
      const int w = GetArgAsType<int>(ArgIdx, ArgList);  // va_arg(va, int);
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
        const int prec = GetArgAsType<int>(ArgIdx, ArgList);  // (int)va_arg(va, int);
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
            const long long value = GetArgAsType<long long>(ArgIdx, ArgList);  // va_arg(va, long long);
            idx = _ntoa<unsigned long long>(buffer, idx, maxlen, (unsigned long long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
          else if (flags & FLAGS_LONG) {
            const long value = GetArgAsType<long>(ArgIdx, ArgList);  // va_arg(va, long);
            idx = _ntoa<unsigned long>(buffer, idx, maxlen, (unsigned long)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
          else {
            const int value = (flags & FLAGS_CHAR) ? GetArgAsType<char>(ArgIdx, ArgList) : (flags & FLAGS_SHORT) ? GetArgAsType<short int>(ArgIdx, ArgList) : GetArgAsType<int>(ArgIdx, ArgList);
            idx = _ntoa<unsigned long>(buffer, idx, maxlen, (unsigned int)(value > 0 ? value : 0 - value), value < 0, base, precision, width, flags);
          }
        }
        else {   // unsigned
          if (flags & FLAGS_LONG_LONG)idx = _ntoa<unsigned long long>(buffer, idx, maxlen, GetArgAsType<unsigned long long>(ArgIdx, ArgList), false, base, precision, width, flags);   // va_arg(va, unsigned long long)
          else if (flags & FLAGS_LONG)idx = _ntoa<unsigned long>(buffer, idx, maxlen, GetArgAsType<unsigned long>(ArgIdx, ArgList), false, base, precision, width, flags);    // va_arg(va, unsigned long)
          else {
            const unsigned int value = (flags & FLAGS_CHAR) ? GetArgAsType<unsigned char>(ArgIdx, ArgList) : (flags & FLAGS_SHORT) ? GetArgAsType<unsigned short int>(ArgIdx, ArgList) : GetArgAsType<unsigned int>(ArgIdx, ArgList);
            idx = _ntoa<unsigned long>(buffer, idx, maxlen, value, false, base, precision, width, flags);
          }
        }
        format++;
        break;
      }
      case 'f' :
      case 'F' : {
        size_t prec = (flags & FLAGS_PRECISION)?precision:14;   // Is 14 optimal?
        if(*format == 'F') {flags |= FLAGS_UPPERCASE; prec |= 0x1000;}  // ffUpperCase
        if(flags & FLAGS_HASH)prec |= 0x8000;       // ffZeroPad                      // ffCommaSep  // Or may be ffZeroPad 0x8000?
        size_t len = 0;
        char buf[52];
        char* ptr = NCNV::ftoa_simple(GetArgAsType<double>(ArgIdx, ArgList), prec, buf, sizeof(buf), &len);   // After this we have some space at beginning of the buffer    // va_arg(va, double)
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
        buffer[idx++] = GetArgAsType<char>(ArgIdx, ArgList);  // va_arg(va, int);  // char output       // out((char)va_arg(va, int), buffer, idx++, maxlen);
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
          wp = GetArgAsType<wchar_t*>(ArgIdx, ArgList);  // va_arg(va, wchar_t*);
          l  = _strnlen_s(wp, precision ? precision : (size_t)-1);    // precision or a full size
         }
        else
         {
          cp = GetArgAsType<char*>(ArgIdx, ArgList);  // va_arg(va, char*);
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
        unsigned char* DPtr  = GetArgAsType<unsigned char*>(ArgIdx, ArgList);  // va_arg(va, unsigned char*);
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
        if (is_ll)idx = _ntoa<unsigned long long>(buffer, idx, maxlen, (unsigned long long)GetArgAsType<void*>(ArgIdx, ArgList), false, 16U, precision, width=sizeof(long long)*2, flags);  //  (unsigned long long)va_arg(va, unsigned long long)
          else idx = _ntoa<unsigned long>(buffer, idx, maxlen, (unsigned long)GetArgAsType<void*>(ArgIdx, ArgList), false, 16U, precision, width=sizeof(long)*2, flags); //  (unsigned long)va_arg(va, unsigned long)
        format++;
        break;
      }

      case 'n': {       // Nothing printed. The number of characters written so far is stored in the pointed location.
        int* p = GetArgAsType<int*>(ArgIdx, ArgList);  // va_arg(va, int*);
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

};

#pragma clang diagnostic pop
