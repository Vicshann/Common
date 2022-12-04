//==========================================================================================================================
//                                               BASE VISUAL CLASS
//==========================================================================================================================
class CVisBase: public CObjBase     
{
public:
//------------------------------------------------------------------------------------------------------------
virtual bool Show(bool Show=true){ return false; }       // Separate 'Hide' method?
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult){ return false; } 
//------------------------------------------------------------------------------------------------------------
CVisBase(void)
{ 
 this->Flags |= flDerVisBase; 
}

}; 