//==========================================================================================================================
//                                               POPUP MENU
//--------------------------------------------------------------------------------------------------------------------------
// NOTE: May exist without main form
class CSWPopUpMenu: public CCldBase
{
public:
// void (_fastcall CWndBase::*OnMouseBtnUp)(CWndBase* Sender, WORD WMsg, WORD KeyEx, int x, int y) = nullptr;    // If a callback returns true then no need to pass this message next to original WindowProc

CSWPopUpMenu(void){ }
//------------------------------------------------------------------------------------------------------------
int Create(SWDim& Wdim, LPCWSTR Text=L"", DWORD Style=0, DWORD ExStyle=0)  // Separate from constructor to allow some additional configuration
{                     
 if(!this->SuperClassSysCtrl(L"SysDateTimePick32", L"CSWDTPicker"))return -1;
 return (this->CreateWnd((LPCWSTR)this->WndClass, Text, Style|WS_CHILD, ExStyle, Wdim.PosX, Wdim.PosY, Wdim.Width, Wdim.Height, 0))?(0):(-2);
}  

};