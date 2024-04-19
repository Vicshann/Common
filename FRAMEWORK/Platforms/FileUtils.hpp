
//------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------
// NOTE: Initially set offs to 0
static sint ReadFileLine(PX::fdsc_t fd, achar* buf, size_t len, ssize_t* offs, achar** Line)   // Buffered
{
 sint pos = *offs;   // Points to current line to return
 len--;   // Space for terminating 0
 if(pos)  // Have some prev data
  {
   achar* LBeg = &buf[pos];
   for(;pos < len;pos++){achar v = buf[pos]; if(!v || (v == '\n'))break;}
   if(pos < len)   // Found \n or \0    // Not known huw much of actual data in the buffer
    {
     if(buf[pos])*offs = ++pos;
       else *offs = -1;  // No more data
     *Line = LBeg;
     return &buf[pos] - LBeg;
    }
    else  // The pos is at the end of the data (No \n or \0 is found)  
     {
      pos = &buf[pos] - LBeg;
      if(pos)memmove(buf, LBeg, pos);   // Move the incomplete line to beginning of the buffer
     }
  }
// Read rest of an incomplete line or a full new line
 sint rdl = NPTM::NAPI::read(fd, &buf[pos], len - pos);  // NOTE: May be less than requested
 if(rdl <= 0)return rdl;   // No more lines
 uint flen = pos + rdl;
 buf[flen] = 0;  // To avoid confusion
 for(;pos < flen;pos++){achar v = buf[pos]; if(!v || (v == '\n'))break;}               
 if(pos < flen)  // Includes '\n' or \0, may have more lines          
  {
   if(!buf[pos])*offs = -1;
   else if(++pos >= flen)   // Points beyond the read data - May have more lines
    {
     if(flen >= len)*offs = 0;    // At the end of the buffer - next line will read from beginning to make sure that this one was the last
       else *offs = -1;   // EOF after \n
    }
     else *offs = pos;    // Some chars left in the buffer
  }
   else   // The pos is at the end of read data (No \n or \0 is found)
    {
     if(flen >= len)return -PX::ENOMEM;  // The buffer is probably too small for a single line!  (Or a last line does not have '\n' and fits exactly into rest of the buffer)
     *offs = -1;   // -1 if no more lines (EOF)  // No \n, no \0 
    }
 *Line = buf;
 return pos;  
}
//------------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------------