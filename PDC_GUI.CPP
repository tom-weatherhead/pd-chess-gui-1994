// pdc_gui.cpp
// ObjectWindows-based UI for PD-Chess       October 19, 1994
// (C) Copyright 1994 by Tom Weatherhead

// ObjectWindows - (C) Copyright 1992 by Borland International

#define WIN31

#include <stdio.h>
#include <stdlib.h>
#include <dir.h>
#include <string.h>
#include <ctype.h>
#include <owl.h>
#include <dialog.h>
#include <edit.h>
#include <checkbox.h>
#include "pdchess.h"
#include "pdcdispa.h"
#include "pdc_gui.h"



// **** TOptionsDlg Member Functions ****


TOptionsDlg::TOptionsDlg( PTWindowsObject AParent, LPSTR name )
  : TDialog( AParent, name )
{
  WAutoBox = new TCheckBox( this, ID_WCOMPBTN, NULL );
  BAutoBox = new TCheckBox( this, ID_BCOMPBTN, NULL );
  WLevelEditObj = new TEdit(this, ID_WLEVELEDIT, sizeof(WLevelEdit) - 1);
  BLevelEditObj = new TEdit(this, ID_BLEVELEDIT, sizeof(BLevelEdit) - 1);
  TransferBuffer = (void far *)NULL;
} // TOptionsDlg::TOptionsDlg()


void TOptionsDlg::SetupWindow( void )
{
  TDialog::SetupWindow();

  TChessWindow * MainWin = (TChessWindow *)Parent;

  // Display current values
  WAutoBox->SetCheck( (MainWin->army[0].type == PT_HUMAN)
    ? BF_UNCHECKED : BF_CHECKED );
  BAutoBox->SetCheck( (MainWin->army[1].type == PT_HUMAN)
    ? BF_UNCHECKED : BF_CHECKED );
  sprintf( WLevelEdit, "%d", MainWin->army[0].max_ply );
  sprintf( BLevelEdit, "%d", MainWin->army[1].max_ply );
  WLevelEditObj->Insert( WLevelEdit );
  BLevelEditObj->Insert( BLevelEdit );
#if 0 // TW 2016/04/29 : Suppressing these four calls to SendDlgItemMsg() fixed the crash that was happening when the Game -> Options menu item was clicked.
  // Are these needed ???
  SendDlgItemMsg( ID_WCOMPBTN, WM_DRAWITEM, NULL, 0 );
  SendDlgItemMsg( ID_BCOMPBTN, WM_DRAWITEM, NULL, 0 );
  SendDlgItemMsg( ID_WLEVELEDIT, WM_DRAWITEM, NULL, 0 );
  SendDlgItemMsg( ID_BLEVELEDIT, WM_DRAWITEM, NULL, 0 );
#endif
} // TOptionsDlg::SetupWindow()


BOOL TOptionsDlg::CanClose( void )
{
  int WLevel, BLevel;
  TChessWindow * MainWin = (TChessWindow *)Parent;

  FillBuffers();
  WLevel = atoi( WLevelEdit );

  if( WLevel < 2  ||  WLevel > 5 ) {
  	MessageBox(HWindow, "Invalid skill level for White",
  	  "Input Error", MB_OK);
  	return( FALSE );
  }

  BLevel = atoi( BLevelEdit );

  if( BLevel < 2  ||  BLevel > 5 ) {
  	MessageBox(HWindow, "Invalid skill level for Black",
  	  "Input Error", MB_OK);
  	return( FALSE );
  }

  // Don't copy value until verified as valid
  MainWin->army[0].max_ply = WLevel;
  MainWin->army[1].max_ply = BLevel;
  MainWin->army[0].max_ply_cap = WLevel + 2;
  MainWin->army[1].max_ply_cap = BLevel + 2;
  MainWin->army[0].type = WPlayerType;
  MainWin->army[1].type = BPlayerType;
  return( TRUE );
} // TOptionsDlg::CanClose()


// Retrieve data

void TOptionsDlg::FillBuffers( void )
{
  GetDlgItemText( HWindow, ID_WLEVELEDIT, WLevelEdit, LEVELEDITSIZE );
  GetDlgItemText( HWindow, ID_BLEVELEDIT, BLevelEdit, LEVELEDITSIZE );
  WPlayerType = ( WAutoBox->GetCheck() == BF_UNCHECKED ) ? PT_HUMAN : PT_COMPUTER;
  BPlayerType = ( BAutoBox->GetCheck() == BF_UNCHECKED ) ? PT_HUMAN : PT_COMPUTER;
} // TOptionsDlg::FillBuffers()


void TOptionsDlg::HandleWAutoBox( RTMessage )
{
  WAutoBox->Toggle();
} // TOptionsDlg::HandleWAutoBox()


void TOptionsDlg::HandleBAutoBox( RTMessage )
{
  BAutoBox->Toggle();
} // TOptionsDlg::HandleBAutoBox()


TOptionsDlg::~TOptionsDlg( void )
{
  delete WAutoBox;
  delete BAutoBox;
  delete WLevelEditObj;
  delete BLevelEditObj;
} // TOptionsDlg::~TOptionsDlg()


// **** TChessWindow Member Functions ****


TChessWindow::TChessWindow(PTWindowsObject AParent, LPSTR ATitle)
  : TWindow(AParent, ATitle)
{
#if 0
  int row, col;

  // DO THIS FIRST, or Paint will hang
  // Put in BoardClass::Init() ???

  for( row = 0; row < BOARD_SIZE; row++ ) {

    for( col = 0; col < BOARD_SIZE; col++ ) {
	    shared.board.ptr[row][col] = NULL;
    }
  }
#endif
  memset( army, 0, 2 * sizeof( army_type ) );
  army[0].sh = army[1].sh = &shared;
  army[0].max_ply = army[1].max_ply = 3;

  Attr.X = Attr.Y = 0;
  Attr.W = 480;
  Attr.H = 352;
  AssignMenu( "COMMANDS" );

  // Split init_army into (once-only) and (every-game) operations
  init_board( army );
  LButtonState = BS_CHOOSE_SRC;
  ButtonDown = FALSE;
  PlayingGame = FALSE;
#if 1
  got_a_DC = FALSE;
#else
  // This causes grid to be drawn outside of window (?)
  got_a_DC = TRUE;
  BaseDC = GetDC( HWindow );
#endif

  MsgLine[0] = '\0';
  LastMove[0] = '\0';
  PlayerName = "Error";
  MoveNum = 0;	// Initialize it so it won't be displayed
  player = &army[0];
  army[0].sh->CaptureMsg[0] = '\0';
  //    SetupGame();
} // TChessWindow::TChessWindow()


void TChessWindow::CMGameOptions( RTMessage )
{
  GetModule()->ExecDialog( new TOptionsDlg( this, "GAMEOPTIONS" ) );
} // TChessWindow::CMGameOptions()


void TChessWindow::CMAbout( RTMessage )
{
  GetModule()->ExecDialog( new TDialog( this, "ABOUT" ) );
} // TChessWindow::CMAbout()


void TChessWindow::DrawMove( move_type * move )
{
  char buf[20];
  int rook_c1, rook_c2;

  strcpy( LastMove, move_int_to_ext( move ) );
  strcat( LastMove, "    " );
  TextOut( BaseDC, 321, 49, LastMove, strlen( LastMove ) );
  ClearSquare( BaseDC, move->r1, move->c1 );
  DrawSquare( BaseDC, move->r1, move->c1 );
  ClearSquare( BaseDC, move->r2, move->c2 );
  DrawSquare( BaseDC, move->r2, move->c2 );

  if( move->type == EN_PASSANT ) {
  	ClearSquare( BaseDC, move->r1, move->c2 );
  	DrawSquare( BaseDC, move->r1, move->c2 );
  }

  if( move->type != CASTLE_KSIDE  &&  move->type != CASTLE_QSIDE ) return;

  rook_c1 = ( move->type == CASTLE_KSIDE ) ? 7 : 0;
  rook_c2 = ( move->type == CASTLE_KSIDE ) ? 5 : 3;
  // move->r1 == move->r2
  ClearSquare( BaseDC, move->r1, rook_c1 );
  DrawSquare( BaseDC, move->r1, rook_c1 );
  ClearSquare( BaseDC, move->r1, rook_c2 );
  DrawSquare( BaseDC, move->r1, rook_c2 );
} // TChessWindow::DrawMove()


void TChessWindow::WMLButtonDown( RTMessage Msg )
{
  char buf[40];
  // row, col must be unsigned
  unsigned int row, col;
  static move_type move;

  if( !PlayingGame  ||  ButtonDown ) return;

  ButtonDown = TRUE;

  if( !got_a_DC ) return;

  // Coordinate is Msg.LP.Lo, Msg.LP.Hi
  col = (Msg.LP.Lo - MIN_COL) / SQUARE_WIDTH;
  row = (Msg.LP.Hi - MIN_ROW) / SQUARE_HEIGHT;

  // Assumes row, col are unsigned

  if( col >= BOARD_SIZE  ||  row >= BOARD_SIZE ) return;

  if( LButtonState == BS_CHOOSE_SRC ) {
  	move.r1 = row;
  	move.c1 = col;
    move.pc = read_board(player,row,col);
    // Display first part of move...
  	LButtonState = BS_CHOOSE_DST;
  	return;
  }

  // Pawn promotion currently not supported
  move.r2 = row;
  move.c2 = col;
  move.dstPP = &player->sh->board.ptr[row][col];
  move.type = REGULAR_MOVE;

  if( move.r1 == 7*(1-player->colour)  &&  move.r1 == move.r2
    &&  move.c1 == 4 ) {

  	if( move.c2 == 6 ) {
      move.type = CASTLE_KSIDE;
  	} else if( move.c2 == 2 ) {
      move.type = CASTLE_QSIDE;
  	}
  }

  // Display second part of move (or O-O or O-O-O)...
  LButtonState = BS_CHOOSE_SRC;

  if( !is_valid_move( player, &move ) ) {
    // Invalid move; this player must try again
    MessageBox( HWindow, "Move is invalid",
      "Debug Info", MB_OK );
    ButtonDown = FALSE;
    return;
  }

  move_piece( player, &move, NULL );
  DrawMove( &move );

  if( in_opening_book( player->sh ) ) {
	  open_match_move( player->sh, &move );
  }

  GameMain();
} // TChessWindow::WMLButtonDown()


void TChessWindow::WMLButtonUp( RTMessage )
{
  if( !ButtonDown ) return;

  ButtonDown = FALSE;
} // TChessWindow::WMLButtonUp()


void TChessWindow::WMRButtonDown( RTMessage )
{
  // Will cause WM_PAINT message to be sent
  InvalidateRect( HWindow, NULL, TRUE );
} // TChessWindow::WMRButtonDown()


void TChessWindow::DrawBoard( HDC DC )
{
  int r, c;

  for( r = 0; r < BOARD_SIZE; r++ ) {

	  for( c = 0; c < BOARD_SIZE; c++ ) {
	    DrawSquare( DC, r, c );
  	} // for
  } // for
} // TChessWindow::DrawBoard()


void TChessWindow::Paint( HDC DC, PAINTSTRUCT& )
{
  char buf[25], * CaptureMsg = army[0].sh->CaptureMsg;
  int r;

  // Draw grid, 9 vert. lines, 9 horiz.

  for( r = 0; r <= BOARD_SIZE; r++ ) {
  	MoveTo( DC, MIN_COL, MIN_ROW+r*SQUARE_HEIGHT );
  	LineTo( DC, MIN_COL+8*SQUARE_WIDTH, MIN_ROW+r*SQUARE_HEIGHT );
  	MoveTo( DC, MIN_COL+r*SQUARE_WIDTH, MIN_ROW );
  	LineTo( DC, MIN_COL+r*SQUARE_WIDTH, MIN_ROW+8*SQUARE_HEIGHT+1 );
  } // for

  DrawBoard( DC );

  if( MsgLine[0] != '\0' ) {
  	TextOut( DC, 321, 17, MsgLine, strlen( MsgLine ) );
  }

  if( MoveNum > 0 ) {
	  sprintf( buf, "Move #%d; %s ", MoveNum, PlayerName );
  	TextOut( DC, 321, 33, buf, strlen( buf ) );
  }

  if( LastMove[0] != '\0' ) {
	  TextOut( DC, 321, 49, LastMove, strlen( LastMove ) );
  }

  if( CaptureMsg[0] != '\0' ) {
	  TextOut( DC, 1, 1, CaptureMsg, strlen( CaptureMsg ) );
  }
} // TChessWindow::Paint()


void TChessWindow::CMFileExit( RTMessage )
{
  CloseWindow();
} // TChessWindow::CMFileExit()


void TChessWindow::CMGamePlay( RTMessage )
{
  PlayingGame = TRUE;
  SetupGame();
} // TChessWindow::CMGamePlay()


void TChessWindow::ClearSquare( HDC DCparam, int r, int c )
{

  if( !got_a_DC ) return;

  TextOut( DCparam, c * SQUARE_WIDTH + MIN_COL + 10,
    r * SQUARE_HEIGHT + MIN_ROW + 10, "   ", 3 );
} // TChessWindow::ClearSquare()


void TChessWindow::DrawSquare( HDC DCparam, int r, int c )
{
  char t, str[2];
  piece_type * pc;

  if( !got_a_DC ) return;

  pc = read_board(player,r,c);

  if( pc == NULL ) {
    str[0] = ((r + c) % 2) ? '+' : '-';
  } else {
    t = pc->def->token;
	  str[0] = (pc->owner->colour == O_WHITE) ? t : tolower(t);
  }

  str[1] = '\0';
  TextOut( DCparam, c * SQUARE_WIDTH + MIN_COL + 10,
    r * SQUARE_HEIGHT + MIN_ROW + 10, str, 1 );
} // TChessWindow::DrawSquare()


// Returns TRUE iff the game can continue

BOOL TChessWindow::AssessMove( void )
{
  BOOL check, mate;
  char buf[40], * format_str;

#if 0
  if( is_check( owner->opponent ) ) {
    printf( "Semantic error: %s could capture %s's king\n\n",
      owner->name, owner->opponent->name );
    break;
  }
#endif

  if( is_draw( player ) ) return( FALSE );

  check = is_check( player->opponent );
  mate = is_checkmate( player->opponent );
  format_str = (check) ? ((mate) ? "%s has been checkmated"
    : "%s is in check") : ((mate) ? "%s has been stalemated" : NULL);

  if( format_str != NULL ) {
    sprintf( buf, format_str, player->opponent->name );
    TextOut( BaseDC, 1, 16, buf, strlen( buf ) );

    if( mate ) {
	    PlayingGame = FALSE;	// Deadlock
	    return( FALSE );
    }
  }

  return( TRUE );
} // TChessWindow::AssessMove()


void TChessWindow::GameMain( void )
{
  char buf[25], * CaptureMsg;
  int len;
  move_type * move;

  for( ; ; ) {

    if( BeforeFirstMove ) {
	    BeforeFirstMove = FALSE;
  	} else {

	    if( player->sh->CaptureMsg[0] != '\0' ) {
    		CaptureMsg = player->sh->CaptureMsg;
    		len = strlen( CaptureMsg );
	    } else {
      	CaptureMsg = buf;
		    len = sizeof( buf );
        memset( buf, ' ', len );
	    }

	    TextOut( BaseDC, 1, 1, CaptureMsg, len );

	    if( !AssessMove() ) break;

	    player = player->opponent;
  	}

  	MoveNum = player->sh->move_rec_idx / 4 + 1;
  	PlayerName = player->name;
  	sprintf( buf, "Move #%d; %s ", MoveNum, PlayerName );
    	TextOut( BaseDC, 321, 33, buf, strlen( buf ) );

  	if( player->type == PT_HUMAN ) break;

  	// Play computer's move

  	if( !computer_move( &player ) ) {
	    PlayingGame = FALSE;
	    break;
    }

    move = &player->opt_move;

    if( move->type == CASTLE_KSIDE  ||  move->type == CASTLE_QSIDE ) {
	    move->r1 = move->r2 = 7*(1-player->colour);
	    move->c1 = 4;
	    move->c2 = (move->type == CASTLE_KSIDE) ? 6 : 2;
  	}

  	DrawMove( move );
  } // for
} // TChessWindow::GameMain()


void TChessWindow::SetupGame( void )
{

  if( !got_a_DC ) {
  	// Get a new DC
  	got_a_DC = TRUE;
  	BaseDC = GetDC( HWindow );
  }

  // Forces a call to WMPaint()
  InvalidateRect( HWindow, NULL, TRUE );
  //    init_board( army );

  if( army[0].type == PT_COMPUTER  &&  army[1].type == PT_COMPUTER ) {
  	DrawBoard( BaseDC );
  }

  player = &army[0]; // White
  PlayingGame = BeforeFirstMove = TRUE;
  GameMain();
} // TChessWindow::SetupGame()


TChessWindow::~TChessWindow( void )
{
  free_stuff( &shared );

  if( got_a_DC ) {
  	ReleaseDC( HWindow, BaseDC );
  }
} // TChessWindow::~TChessWindow()


// **** TChessApp Member Functions ****


void TChessApp::InitMainWindow( void )
{
  MainWindow = new TChessWindow(NULL, Name);
  //    OthelloMainWin = (TOthelloWindow *)MainWindow;
} // TChessApp::InitMainWindow()


int PASCAL WinMain(HINSTANCE hInstance,
  HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  TChessApp MyApp("PD-Chess", hInstance, hPrevInstance,
    lpCmdLine, nCmdShow);

  MyApp.Run();
  return( MyApp.Status );
} // WinMain()


// ******** end of othelgui.cpp ********
