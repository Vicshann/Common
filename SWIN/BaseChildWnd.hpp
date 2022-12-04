//==========================================================================================================================
//                                           BASE CHILD WINDOW CLASS   (Subject to layout rules)
//==========================================================================================================================
class CCldBase: public CWndBase     // Parent window must notify its children when resized!!!   // Or any child, when moved or resized
{
 BYTE ESnap;
 RECT SnapDist;

//------------------------------------------------------------------------------------------------------------
void ResetSnapByParentRect(RECT* prec, RECT* crec)
{
 int pwidth  = prec->left   - prec->right;
 int pheight = prec->bottom - prec->top;
 this->SnapDist.top    = crec->top;
 this->SnapDist.left   = crec->left;
 this->SnapDist.right  = pwidth  - crec->right;
 this->SnapDist.bottom = pheight - crec->bottom;
}
//------------------------------------------------------------------------------------------------------------
void UpdateSnapByParentRect(RECT* prec)
{
/*
      int pwidth  = prec.left   - prec.right;
      int pheight = prec.bottom - prec.top;

*/
}
//------------------------------------------------------------------------------------------------------------

public:
CCldBase(void)
{ 
 this->Flags |= flDerCldBase; 
}
//------------------------------------------------------------------------------------------------------------
virtual bool WindowProc(HWND& hWnd, UINT& Msg, WPARAM& wParam, LPARAM& lParam, LRESULT& lResult)
{
 CWndBase::WindowProc(hWnd, Msg, wParam, lParam, lResult);  // Let base class to process the messages before we extend them
 switch(Msg)
  {
   case WM_CREATE:
    {
     RECT prec, crec;
     CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
     GetClientRect(this->GetOwnerHandle(), &prec);   // Take client area of parent
     crec.top    = cs->y;
     crec.left   = cs->x;
     crec.right  = crec.left + cs->cx;
     crec.bottom = crec.top  + cs->cy;
     this->ResetSnapByParentRect(&prec, &crec);
    }
    break;
   case WM_SIZE:
    if(hWnd == this->GetOwnerHandle())  // Parent window resized
     {
      RECT prec;
      GetClientRect(hWnd, &prec);
      this->UpdateSnapByParentRect(&prec);    // WM_SIZE will be sent if this component resized?
     }
    else if(hWnd == this->GetHandle())  // Resized itself - keep it snapped
     {
      //
      // Update snaps
      //
      this->NotifyChildren(hWnd, Msg, wParam, lParam);
     }
    break;
  }
 return true;
}
//------------------------------------------------------------------------------------------------------------

};
