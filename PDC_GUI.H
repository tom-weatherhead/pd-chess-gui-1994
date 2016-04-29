// pdc_gui.h
// A component of the Portable Didactic Chess package


#define	MIN_ROW			32
#define	MIN_COL			32
#define	SQUARE_HEIGHT		32
#define	SQUARE_WIDTH		32
#define LEVELEDITSIZE		16 // TW 2016/04/29 : Was 3


class TOptionsDlg : public TDialog
{
  private:
    void FillBuffers( void );

  public:
    char WLevelEdit[LEVELEDITSIZE], BLevelEdit[LEVELEDITSIZE];
    PlayerTypeType WPlayerType, BPlayerType;
    TCheckBox * WAutoBox, * BAutoBox;
    TEdit * WLevelEditObj, * BLevelEditObj;

    TOptionsDlg( PTWindowsObject AParent, LPSTR name );
    void SetupWindow( void );
    virtual BOOL CanClose( void );
    virtual void HandleWAutoBox( RTMessage )
      = [ID_FIRST + ID_WCOMPBTN];
    virtual void HandleBAutoBox( RTMessage )
      = [ID_FIRST + ID_BCOMPBTN];
    ~TOptionsDlg( void );
}; // TOptionsDlg


_CLASSDEF(TChessWindow)
class TChessWindow : public TWindow
{
  private:
    BOOL BeforeFirstMove;
    char MsgLine[40], LastMove[20], * PlayerName;
    ButtonStateType LButtonState; // ChooseSrc, ChooseDst
    int MoveNum;

    BOOL AssessMove( void );

  public:
    BOOL ButtonDown, PlayingGame, got_a_DC;
//    char FileName[MAXPATH];
//    board_type board;
//    coord_heap_class coord_heap;
    shared_type shared;
    army_type army[2], * player;
    HDC BaseDC;		// Display Context

    TChessWindow(PTWindowsObject AParent, LPSTR ATitle);
    void ClearSquare( HDC, int, int );
    void DrawSquare( HDC, int, int );
    void DrawMove( move_type * );
    void DrawBoard( HDC );
    void GameMain( void );
    void SetupGame( void );
    virtual void CMFileExit( RTMessage )
      = [CM_FIRST + CM_FILEEXIT];
    virtual void CMGameOptions( RTMessage )
      = [CM_FIRST + CM_GAMEOPTIONS];
    virtual void CMGamePlay( RTMessage )
      = [CM_FIRST + CM_GAMEPLAY];
    virtual void CMAbout( RTMessage )
      = [CM_FIRST + CM_ABOUT];
    virtual void WMLButtonDown( RTMessage )
      = [WM_FIRST + WM_LBUTTONDOWN];
    virtual void WMLButtonUp( RTMessage )
      = [WM_FIRST + WM_LBUTTONUP];
    virtual void WMRButtonDown( RTMessage )
      = [WM_FIRST + WM_RBUTTONDOWN];
    virtual void Paint( HDC, PAINTSTRUCT& );
    ~TChessWindow( void );
}; // TChessWindow


class TChessApp : public TApplication
{
  public:
    TChessApp(LPSTR AName, HINSTANCE hInstance,
      HINSTANCE hPrevInstance,
      LPSTR lpCmdLine, int nCmdShow)
      : TApplication(AName, hInstance, hPrevInstance,
          lpCmdLine, nCmdShow) {};
    virtual void InitMainWindow( void );
}; // TChessApp


// **** End of pdc_gui.h ****
