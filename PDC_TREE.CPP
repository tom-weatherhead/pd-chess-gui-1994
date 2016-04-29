/* pdc_tree.c : Opening move tree ops
 * By Tom Weatherhead
 * Started February 24, 1994
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pdchess.h"


bool omt_encode_move( omt_move_type * move, char * str )
{
    unsigned int r1, c1, r2, c2;

    if( !move_ext_to_int( str, &r1, &c1, &r2, &c2 ) ) return( FALSE );

    move->c[0] = (char)(8 * r1 + c1);
    move->c[1] = (char)(8 * r2 + c2);
    return( TRUE );
}


char * omt_decode_move( omt_move_type * move )
{
    unsigned int r1 = ((unsigned int)(move->c[0])) / 8,
		 c1 = ((unsigned int)(move->c[0])) % 8,
		 r2 = ((unsigned int)(move->c[1])) / 8,
		 c2 = ((unsigned int)(move->c[1])) % 8;

    return( move_int_to_ext( r1, c1, r2, c2 ) );
}


void traverse_subtree( omt_node_type * node )
{
    num_type i;

    if( node == NULL ) return;

    printf( "%d moves:  ", node->num_moves );

    for( i = 0; i < node->num_moves; i++ ) {
	printf( "(%s) ", omt_decode_move( &node->moves[i] ) );
    }

    printf( "%d children:\n\n", node->num_children );

    for( i = 0; i < node->num_children; i++ ) {
	traverse_subtree( node->child[i] );
    }
}


static void describe_node( omt_node_type * node )
{
    if( node == NULL ) {
	printf( "Node is NULL\n" );
    } else {
	printf( "Node has %d moves, %d children\n",
	  node->num_moves, node->num_children );
    }
}


omt_node_type * create_node( omt_move_type * moves, num_type num_moves,
  num_type num_children, omt_node_type ** self_ptr )
{
    omt_node_type * node = (omt_node_type *)malloc( sizeof( omt_node_type ) );

    node->num_moves = num_moves;
    node->move_buf_size = num_moves * sizeof( omt_move_type );
    node->num_children = num_children;
    node->self_ptr = self_ptr;
    *self_ptr = node;
    node->moves = (omt_move_type *)malloc( node->move_buf_size );
    memcpy( node->moves, moves, node->move_buf_size );
    node->child = (omt_node_type **)((num_children == 0) ? NULL
      : malloc( num_children * sizeof( omt_node_type * ) ) );
    return( node );
}


void write_node( omt_node_type * node, FILE * out )
{
    unsigned int i;

    fwrite( &node->num_moves, sizeof(node->num_moves), 1, out );
    fwrite( &node->num_children, sizeof(node->num_children), 1, out );
    fwrite( node->moves, node->num_moves, sizeof(omt_move_type), out );

    if( node->num_children > 0 ) {

	for( i = 0; i < node->num_children; i++ ) {
	    write_node( node->child[i], out );
	}

	free( node->child );
    }

    free( node->moves );
    free( node );
}


omt_node_type * read_node( FILE * in, omt_node_type ** self_ptr )
{
    omt_node_type * node;
    unsigned int i;

    if( feof( in ) ) return( NULL );

    node = (omt_node_type *)malloc( sizeof( omt_node_type ) );
    fread( &node->num_moves, sizeof(node->num_moves), 1, in );
    fread( &node->num_children, sizeof(node->num_children), 1, in );
    node->move_buf_size = node->num_moves * sizeof(omt_move_type);
    node->moves = (omt_move_type *)malloc( node->move_buf_size );
    fread( node->moves, sizeof( omt_move_type ), node->num_moves, in );
    node->self_ptr = self_ptr;

    if( node->num_children == 0 ) {
	node->child = NULL;
    } else {
	node->child = (omt_node_type **)malloc( node->num_children
	  * sizeof( omt_node_type * ) );

	for( i = 0; i < node->num_children; i++ ) {
	    node->child[i] = read_node( in, &node->child[i] );

	    if( node->child[i] == NULL ) {	/* ie EOF */

		if( i > 0 ) {
		    node->num_children = i;
		} else {
		    free( node->moves );
		    free( node->child );
		    free( node );
		    node = NULL;
		}

		break;
	    }
	}
    }

    return( node );
}


omt_node_type * insert_node( omt_node_type * node, num_type move_num,
  omt_move_type * move )
{
    omt_node_type * temp;

    if( node == NULL  &&  move_num > node->num_moves ) return( NULL );

/*    printf( "insert(): move_num = %d, num_moves = %d, move_buf_size = %d\n",
      move_num, node->num_moves, node->move_buf_size ); */

    if( move_num == node->num_moves ) {

	if( node->num_children == 0 ) {		/* Simple append */
	    printf( "Appending move to list...\n" );

	    if( (move_num+1) * sizeof(omt_move_type) >= node->move_buf_size ) {
		printf( "Resizing move buffer...\n" );
		node->move_buf_size += 10 * sizeof(omt_move_type);
		node->moves = (omt_move_type *)realloc( node->moves, 
		  node->move_buf_size );
	    }

	    node->moves[node->num_moves++] = *move;
	} else {				/* Add a new child */
	    printf( "Adding a new child...\n" );
	    node->child = (omt_node_type **)realloc( node->child,
	       sizeof( omt_node_type * ) * (node->num_children + 1) );
	    create_node( move, 1, 0, &node->child[node->num_children] );
	    return( node->child[node->num_children++] );
	}

    } else {				/* Split node */
	printf( "Splitting node...\n" );
#if 0
/*	printf( "Current node: " ); */
	describe_node( node );
/*	printf( "Creating temp\n" ); */
#endif
	temp = create_node( node->moves, move_num, 2, node->self_ptr );
/*	printf( "Setting child[0]\n" ); */
	temp->child[0] = node;
	node->self_ptr = &temp->child[0];
/*	printf( "Creating child[1]\n" ); */
	create_node( move, 1, 0, &temp->child[1] );
	node->num_moves -= move_num;
/*	printf( "memcpy\n" ); */
	memcpy( node->moves, &node->moves[move_num], sizeof(omt_move_type)
	  * node->num_moves );
#if 0
	printf( "Replaced node: " );
	describe_node( temp );
	printf( "Child 0: " );
	describe_node( temp->child[0] );
	printf( "Child 1: " );
	describe_node( temp->child[1] );
#endif
	return( temp->child[1] );
    }

    return( node );
}


omt_node_type * match_move( omt_node_type * node, num_type move_num,
  omt_move_type * move )
{
    num_type i;

    if( move_num < node->num_moves ) {

	if( !memcmp( move, &node->moves[move_num], sizeof(omt_move_type))) {
	    return( node );
	}

	return( NULL );
    }

    if( move_num > node->num_moves ) return( NULL );	/* ie. Error */

    for( i = 0; i < node->num_children; i++ ) {

	if( !memcmp( move, &node->child[i]->moves[0], sizeof(omt_move_type))) {
	    return( node->child[i] );
	}
    }

    return( NULL );
}


bool driver( shared_type * sh )
{
    char str[20];
    omt_move_type move;
    omt_node_type * node = NULL, * new_node;
    num_type move_num = 0;
    FILE * in, * out;

    printf( "Input file: " );
    scanf( "%s", str );
    in = fopen( str, "r" );

    if( in == NULL ) {
	printf( "Can't open '%s' for read\n\n", str );
    } else {
	printf( "Reading tree...\n\n" );
	sh->omt_root = node = read_node( in, &sh->omt_root );
	fclose( in );
    }

    for( ; ; ) {	/* For each move */
	printf( "Move %d in node (|N|Q|R|T[S]) : ", move_num );

        do {
            gets( str );		/* gets doesn't delimit by spaces */
        } while( strlen( str ) == 0 ) ;

	if( str[0] == 'Q' ) break;
	else if( str[0] == 'R' ) {	/* Reset to root */
	    printf( "node reset to root\n" );
	    node = sh->omt_root;
	    move_num = 0;
	    continue;
	} else if( str[0] == 'T' ) {	/* Traverse */
	    printf( "Traversing (sub)tree...\n" );
	    traverse_subtree( (str[1] == 'S') ? node : sh->omt_root );
	    continue;
	} else if( str[0] == 'N' ) {	/* Data on current node */
	    describe_node( node );
	    continue;
	}

	if( !omt_encode_move( &move, str ) ) {
	    printf( "Bad move.  Re-enter.\n" );
	    continue;
	}

	if( node == NULL ) {
	    printf( "Creating root...\n" );
	    node = create_node( &move, 1, 0, &sh->omt_root );
	    move_num = 1;
	    continue;
	}

	new_node = match_move( node, move_num, &move );

	if( new_node != NULL ) {
	    printf( "Move matched.\n" );
	} else {
	    printf( "Move not matched. Inserting...\n" );
	    new_node = insert_node( node, move_num, &move );

	    if( new_node == NULL ) {
		printf( "Error: Couldn't insert move.\n" );
		return( FALSE );		/* Error */
	    }
	}

	if( new_node != node ) {
	    node = new_node;
	    move_num = 1;	/* Move 0 in new node already matched */
	} else {
	    move_num++;
	}
    }

    for( ; ; ) {
	printf( "Output file: " );
	scanf( "%s", str );

	if( str[0] == 'Q' ) break;

	out = fopen( str, "w" );

	if( out == NULL ) {
	    printf( "Can't open '%s' for write\n\n", str );
	} else {
	    printf( "Writing tree...\n\n" );
	    write_node( sh->omt_root, out );
	    fclose( out );
	    break;
	}
    }

    return( TRUE );
}


#if 1
void main( void )
{
    shared_type sh;

    driver( &sh );
}
#endif
