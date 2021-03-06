/* pdcmvcnv.cpp - A module of the PD-Chess package
 * (C) Copyright 1993, 1994 by Tom Weatherhead
 * Started April 1, 1993
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include "pdchess.h"

#define eat_spaces_mac(p)	for(;isspace(*p);p++)	// No return value


char * eat_spaces( char * ptr )  /* Parameter points to start */
{
    for( ; isspace(*ptr); ptr++ );

    return( ptr );
} // eat_spaces()


bool coord_ext_to_int( char * str, unsigned int * row, unsigned int * col )
{
    char c0 = toupper(str[0]), c1 = str[1];

    if( c0 < 'A'  ||  c0 > 'H'  ||  c1 < '1'  ||  c1 > '8' ) return( FALSE );

    *row = BOARD_SIZE - (c1 - '0');
    *col = c0 - 'A';
    return( TRUE );
} // coord_ext_to_int()


bool move_ext_to_int( char * str, move_type * move )
{
    char * ptr;
    unsigned int num_o;
    move_type temp_move = { 0, 0, 0, 0, REGULAR_MOVE, NULL, NULL };

    ptr = eat_spaces( str );
    //* Consistency needed for open_match_move :
    *move = temp_move;

    if( toupper(*ptr) == 'O' ) {        // Possible castling move
        num_o = 1;

        for( ++ptr; *ptr != '\0'; ptr++ ) {

            if( toupper(*ptr) == 'O'  ||  *ptr == '0' ) {
                num_o++;
            }
        }

        switch( num_o ) {
            case 2:                     /* Kingside */
		move->type = CASTLE_KSIDE;
                return( TRUE );
            case 3:                     /* Queenside */
		move->type = CASTLE_QSIDE;
                return( TRUE );
            default:
                return( FALSE );
        }
    }

    if( !coord_ext_to_int( ptr, &move->r1, &move->c1 ) ) return( FALSE );

    ptr = eat_spaces( ptr + 2 );
    return( coord_ext_to_int( ptr, &move->r2, &move->c2 ) );
} // move_ext_to_int()


char * move_int_to_ext( move_type * move )
{
    static char str[10];

    if( move->type == CASTLE_KSIDE ) {
        return( "0-0 " );
    } else if( move->type == CASTLE_QSIDE ) {
        return( "0-0-0 " );
    }

    sprintf( str, "%c%d %c%d ", move->c1 + 'a', BOARD_SIZE - move->r1,
      move->c2 + 'a', BOARD_SIZE - move->r2 );
    return( str );
}


void print_move_rec( shared_type * sh )
{
    unsigned int i, j = 1, src, dst;
    char str[10];
    move_type move;

    printf( "Print record of moves? " );
    scanf( "%s", str );

    if( str[0] != 'y' ) return;

    for( i = 0; i < sh->move_rec_idx; i += 2 ) {

        if( i % 4 == 0 ) {
            printf( "\n%3d", j++ );
        }

        src = (unsigned int)(sh->move_rec[i]);

	if( src == 64 ) {
	    move.type = CASTLE_KSIDE;
	} else if ( src == 72 ) {
	    move.type = CASTLE_QSIDE;
	} else {
	    dst = (unsigned int)(sh->move_rec[i + 1]);
	    move.type = REGULAR_MOVE;
	    move.r1 = src / 8;
	    move.c1 = src % 8;
	    move.r2 = dst / 8;
	    move.c2 = dst % 8;
	}

        printf( "%20s", move_int_to_ext( &move ) );
    }

    printf( "\n" );
}

