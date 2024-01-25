//==========================================================================================================================
//                                                       TIMER
//--------------------------------------------------------------------------------------------------------------------------
// NOTE: Calling thread must process window messages
class CSWTimer: public CObjBase
{
friend CWndBase;

 UINT Interval = 0;

//------------------------------------------------------------------------------------------------------------
static bool ProcessTimerCallback(CWndBase* Parent, WPARAM& wParam, LPARAM& lParam)
{
 CSWTimer* ti = (CSWTimer*)Parent->GetObj((UINT)wParam);
 if(!ti)return false;                  //TODO: Check class name hash to avoid processing of someone`s WM_TIMER messages
 if(ti->OnInterval)(ti->GetOwnerWnd()->*(ti->OnInterval))(ti);
 return true;
}
//------------------------------------------------------------------------------------------------------------

public:
 void (_fastcall CObjBase::*OnInterval)(CObjBase* Sender) = nullptr;      // Add a flag arg to stop the timer???


CSWTimer(void){} 
virtual ~CSWTimer(void){this->Stop();} 
//------------------------------------------------------------------------------------------------------------
int Create(UINT Intv=1000, bool StartNow=false)
{
 if(!this->OwnerObj)return -1;    // Must belong to a window: even with a callback proc it is called by DefWindowProc on WM_TIMER
 this->Interval = Intv;
 if(StartNow)return this->Start(Intv);
 return 0;
} 
//------------------------------------------------------------------------------------------------------------
UINT GetInterval(void){return this->Interval;}
//------------------------------------------------------------------------------------------------------------
int  SetInterval(UINT Intv)
{
 return this->Start(Intv);
}
//------------------------------------------------------------------------------------------------------------
int  Start(UINT Intv=0)
{
 if(Intv)this->Interval = Intv;
 return (int)SetTimer(this->GetOwnerHandle(), this->IdxInPar, this->Interval, nullptr) - 1;
}
//------------------------------------------------------------------------------------------------------------
int  Stop(void)
{
 return (int)KillTimer(this->GetOwnerHandle(), this->IdxInPar) - 1;
}
//------------------------------------------------------------------------------------------------------------

};