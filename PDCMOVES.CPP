/* pdcmoves.cpp - Human, computer move funcs for the PD-Chess package
 * (C) Copyright 1993, 1994 by Tom Weatherhead
 * Started April 1, 1993
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "pdchess.h"


#ifndef USE_GUI
bool human_move( army_type ** ownerPP )
{
    bool done = FALSE;
    char cmd[10], * ptr;
    int effect;
    unsigned int num_moves;
    mod_data_type mod_data;
    move_type move;
    army_type * owner = *ownerPP,
      * army = (owner->colour == O_WHITE ) ? owner : owner->opponent;

    if( owner->auto_until > owner->sh->move_rec_idx ) {
        return( computer_move( ownerPP ) );
    }

    do {
        printf( "%s: ", owner->name );

        do {
            gets( cmd );
        } while( strlen( cmd ) == 0 ) ;

        ptr = eat_spaces( cmd );

        if( *ptr == ':' ) {     /* Command interpreter */
            eat_spaces( ++ptr );

            switch( tolower(*ptr) ) {
                case 'a':               /* Automate this army */
                    printf( "Automate for how many moves? " );
                    scanf( "%d", &num_moves );

                    if( num_moves == 0 ) break;

                    owner->auto_until = owner->sh->move_rec_idx + 4*num_moves;
                    init_computer_army( owner );
                    return( computer_move( ownerPP ) );
                case 'b':               /* Backup */

                    if( owner->sh->move_rec_idx >= 4 ) {
                        backup( owner->opponent );
                        backup( owner );
                        print_board( owner );
                    } else {
                        printf( "Too early to backup\n\n" );
                    }

                    break;
                case 'h':               /* Help */
                    printf( "Help: max_ply: " );
                    scanf( "%d", &owner->max_ply );
                    printf( "      max_ply_cap: " );
                    scanf( "%d", &owner->max_ply_cap );
                    effect = best_move( owner, owner, 1, 0, 0 );
                    printf( "Suggested move: %s\n", move_int_to_ext(
                      &owner->opt_move ) );
                    printf( "Expected effect: %d\n\n", effect );
                    break;
                case 'l':               /* Load a game */
                    if( load_game(army, ownerPP ) ) {
                        return( (*((*ownerPP)->move_proc))( ownerPP ) );
                        /* In case opposite colour moves first in loaded game*/
                    }

                    break;
                case 'q':
                    printf( "%s resigns\n\n", owner->name );
                    return( FALSE );
                case 's':               /* Save a game */
                    save_game( army );
                    break;
                case '?':
                    printf( "Commands: abhlqs\n\n" );
                    break;
                default:
                    printf( "Invalid directive\n" );
                    break;
            }

            continue;
        }

        if( !move_ext_to_int( cmd, &move ) ) {
            printf( "Syntax error\n" );
            continue;
        }

	move.pc = read_board(owner,move.r1,move.c1);
	move.dstPP = &owner->sh->board.ptr[move.r2][move.c2];

	if( move.pc->def->token == PAWN_TOKEN && move.r2 == 7*owner->colour ) {

	    do {
		printf( "Promote pawn to (n|b|r|q): " );
		scanf( "%s", cmd );

		switch( tolower(cmd[0]) ) {
		  case 'n':	move.type = PROMOTE_TO_N;	break;
		  case 'b':	move.type = PROMOTE_TO_B;	break;
		  case 'r':	move.type = PROMOTE_TO_R;	break;
		  case 'q':	move.type = PROMOTE_TO_Q;	break;
		}
	    } while( move.type == REGULAR_MOVE );
	}

        if( !is_valid_move( owner, &move ) ) {
            printf( "Attempted move is illegal\n" );
            continue;
        }

        move_piece( owner, &move, &mod_data );

        if( is_check( owner ) ) {
            printf( "Invalid move: can't move into check\n" );
        } else {
            done = TRUE;
        }

        undo_move( &mod_data );
    } while( !done );

    move_piece( owner, &move, NULL );

    if( in_opening_book( owner->sh ) ) {
//      printf( "Matching move in book...\n" );
        open_match_move( owner->sh, &move );
    }

    return( TRUE );
} // human_move()
#endif


bool computer_move( army_type ** ownerPP )
{
    char * opening_move, move_str[10];
    int effect;
    move_type move;
    army_type * owner = *ownerPP;

//    printf( "%s: Working...\n", owner->name );

    /* Seed random-number generator */
    srand( time( NULL ) );

    opening_move = open_get_move(owner->sh); /* NULL if opening book closed */

    if( opening_move != NULL ) {
//        printf( "Using opening book...\n" );
        move_ext_to_int( opening_move, &move );
        strcpy( move_str, opening_move );
    } else {
//        printf( "Not using opening book.\n" );
        owner->sh->num_moves_gend = owner->sh->num_moves_made = 0;

        effect = best_move( owner, owner, 1, 0, 0 );
//        printf( "%d moves generated\n", owner->sh->num_moves_gend );
//        printf( "%d moves made during search\n", owner->sh->num_moves_made );
//        printf( "Optimal effect of move: %d\n", effect );

	if( army_value( owner ) + effect + owner->sh->resign_thresh
	  <= army_value( owner->opponent ) ) {
//            printf( "%s resigns\n\n", owner->name );
            return( FALSE );
        }

	move = owner->opt_move;
        strcpy( move_str, move_int_to_ext( &move ) );
    }

    move.pc = read_board(owner,move.r1,move.c1);
    move.dstPP = &owner->sh->board.ptr[move.r2][move.c2];

    if( !is_valid_move( owner, &move ) ) {
        printf( "Error: %s attempted illegal move: %s\n", owner->name,
          move_str );
        return( FALSE );        // ie. stop game
    }

    move_piece( owner, &move, NULL );
//    printf( "%s: %s\n", owner->name, move_str );
    owner->opt_move = move;
    return( TRUE );
}

