#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define my_isblank( a ) (isspace(a) || ((a) == '\t'))

#include "stringlist.h"

static int stringlist_write_single_element_to_file( Stringlist *sl, FILE *fout );
static char *stringlist_escape_string( char *ss );
static char *stringlist_unescape_string( char *ss );
static int stringlist_delete_single_element_inner( Stringlist *sl );
static int stringlist_copy_aux( Stringlist *new_el, void *aux, int sltype );
static int stringlist_add_string_common( Stringlist **list, Stringlist **new_el, char *new_string, void *aux, int sltype );
static int stringlist_new_sl( Stringlist **el );
static int stringlist_check_args( Stringlist **list, char *new_string, void *aux, int sltype );
static int stringlist_copy_name( Stringlist *new_el, char *new_string );
static int stringlist_get_tok_indices( char *line, int *index0, int *index1, int *string0, int *string1, 
		int *aux_type0, int *aux_type1, int *aux_val0, int *aux_val1 );
static int stringlist_line_to_sl( char *line, int lineno, Stringlist **sl );
static int stringlist_read_from_file_v1( Stringlist **sl, FILE *fin, int nlines_in_header, int debug );

/**************************************************************************************
 * Given a stringlist and a string, this returns a pointer to the element on the
 * stringlist that matches the string, or NULL if there is no match.
 */
	Stringlist *
stringlist_match_string_exact( Stringlist *list, char *str )
{
	Stringlist	*cursor;

	if( str == NULL )
		return( NULL );

	cursor = list;
	while( cursor != NULL ) {
		if( strcmp( cursor->string, str ) == 0 ) 
			return( cursor );
		cursor = cursor->next;
		}

	return( NULL );
}

/**************************************************************************************
 * Adds the given string and auxilliary data to the list.
 * The first time this is called, assuming you want to make a new stringlist, pass
 * with *list == NULL. This will make a new stringlist with the passed string (and 
 * aux info) as the first element of the new string.
 * Returns 0 on success, -1 on error (usually inability to allocate memory)
 */
	int
stringlist_add_string( Stringlist **list, char *new_string, void *aux, int sltype )
{
	Stringlist 	*cursor, *new_el;
	int		i, err;

	/* Housekeeping tasks that are in common with the ordered version of this routine */
	if( (err = stringlist_add_string_common( list, &new_el, new_string, aux, sltype )) != 0 ) {
		fprintf( stderr, "stringlist_add_string: returning due to error\n" );
		return( -1 );
		}

	i = 0;
	if( *list == NULL ) {
		*list = new_el;
		new_el->prev = NULL;
		}
	else
		{
		i = 1;
		cursor = *list;
		while( cursor->next != NULL ) {
			if( cursor->magic != SL_MAGIC ) {
				if( cursor->magic == SL_BAD_MAGIC ) {
					fprintf( stderr, "stringlist: Error in stringlist_add_string: trying to use a previously deleted stringlist element (bad magic number)\n" );
					return( -25 );
					}
				fprintf( stderr, "stringlist: Error in stringlist_add_string: found a corrupt or malformed stringlist element on the stringlist (bad magic number)\n" );
				return( -26 );
				}
			cursor = cursor->next;
			i++;
			}
		if( cursor->magic != SL_MAGIC ) {
			if( cursor->magic == SL_BAD_MAGIC ) {
				fprintf( stderr, "stringlist: Error in stringlist_add_string: trying to use a previously deleted stringlist element (bad magic number)\n" );
				return( -25 );
				}
			fprintf( stderr, "stringlist: Error in stringlist_add_string: found a corrupt or malformed stringlist element on the stringlist (bad magic number)\n" );
			return( -26 );
			}
		cursor->next = new_el;
		new_el->prev = cursor;
		}
	new_el->index = i;

	return( 0 );
}

/*******************************************************************************
 * Adds the given string to the list, and returns a pointer to the 
 * new list element, with alphabetic ordering.
 * was: add_to_stringlist_ordered
 * Returns 0 on success, -1 on error (usually inability to allocate memory)
 */
	int
stringlist_add_string_ordered( Stringlist **list, char *new_string, void *aux, int sltype )
{
	Stringlist 	*cursor, *new_el, *prev_el;
	int		i, err;

	/* Housekeeping tasks that are in common with the unordered version of this routine */
	if( (err = stringlist_add_string_common( list, &new_el, new_string, aux, sltype )) != 0 ) {
		fprintf( stderr, "stringlist_add_string: returning due to error\n" );
		return( -1 );
		}

	i = 0;
	if( *list == NULL ) {
		*list = new_el;
		new_el->prev = NULL;
		}
	else
		{
		i = 1;
		cursor = *list;
		prev_el = NULL;
		while( (cursor != NULL) && (strcmp( new_string, cursor->string) > 0)) {
			if( cursor->magic != SL_MAGIC ) {
				if( cursor->magic == SL_BAD_MAGIC ) {
					fprintf( stderr, "stringlist: Error in stringlist_add_string: trying to use a previously deleted stringlist element (bad magic number)\n" );
					return( -25 );
					}
				fprintf( stderr, "stringlist: Error in stringlist_add_string: found a corrupt or malformed stringlist element on the stringlist (bad magic number)\n" );
				return( -26 );
				}
			prev_el = cursor;
			cursor  = cursor->next;
			i++;
			}
		if( cursor == NULL ) {
			prev_el->next = new_el;
			new_el->prev  = prev_el;
			}
		else if( prev_el == NULL ) {
			*list = new_el;
			new_el->next = cursor;
			cursor->prev = new_el;
			}
		else
			{
			new_el->next = cursor;
			cursor->prev = new_el;
			prev_el->next = new_el;
			new_el->prev  = prev_el;
			}
		}
	new_el->index = i;

	return( 0 );
}
		
/**************************************************************************************
 * Allocates space in the stringlist element for the auxilliary data, and copies it over
 * Returns 0 on success, -1 on error (usually inability to allocate memory)
 */
	static int
stringlist_copy_aux( Stringlist *new_el, void *aux, int sltype )
{
	int	slen;

	if( new_el == NULL ) {
		fprintf( stderr, "stringlist: Error, stringlist_copy_aux called with a null element\n" );
		return( -4 );
		}
	if( new_el->magic != SL_MAGIC ) {
		if( new_el->magic == SL_BAD_MAGIC ) {
			fprintf( stderr, "stringlist: Error in stringlist_copy_aux: trying to use a previously deleted stringlist element (bad magic number)\n" );
			return( -15 );
			}
		fprintf( stderr, "stringlist: Error in stringlist_copy_aux: passed a corrupt or malformed stringlist element (bad magic number)\n" );
		return( -5 );
		}

	new_el->sltype = sltype;

	if( sltype == SLTYPE_NULL )
		return(0);	/* Note: new elements are initialized with aux==NULL and sltype==NULL */

	if( aux == NULL ) 
		return(0);	/* Note: new elements are initialized with aux==NULL and sltype==NULL */

	switch( sltype ) {

		case SLTYPE_INT:
			new_el->aux = (void *)malloc( sizeof(int) );
			if( new_el->aux == NULL ) {
				fprintf( stderr, "stringlist_copy_aux: failed to allocate a single int\n" );
				return(-1);
				}
			*((int *)new_el->aux) = *((int *)aux);
			break;

		case SLTYPE_STRING:
			slen = strlen( (char *)aux );
			if( slen > STRINGLIST_MAX_LEN ) {
				fprintf( stderr, "stringlist_copy_aux: error, passed string is too long! Max allowed: %d\n", 
					STRINGLIST_MAX_LEN );
				return( -2 );
				}
			new_el->aux = (void *)malloc( sizeof(char)*(slen + 1) );
			if( new_el->aux == NULL ) {
				fprintf( stderr, "stringlist_copy_aux: failed to allocate a string of length %d\n", slen+1 );
				return(-1);
				}
			strcpy( (char *)(new_el->aux), (char *)aux );
			break;

		case SLTYPE_FLOAT:
			new_el->aux = (void *)malloc( sizeof(float) );
			if( new_el->aux == NULL ) {
				fprintf( stderr, "stringlist_copy_aux: failed to allocate a single float\n" );
				return(-1);
				}
			*((float *)new_el->aux) = *((float *)aux);
			break;

		case SLTYPE_BOOL:
			new_el->aux = (void *)malloc( sizeof(int) );
			if( new_el->aux == NULL ) {
				fprintf( stderr, "stringlist_copy_aux: failed to allocate a single int\n" );
				return(-1);
				}
			if( *((int *)aux) == 1 )
				*((int *)new_el->aux) = 1;
			else
				*((int *)new_el->aux) = 0;
			break;

		default:
			printf ("ERROR, unknown sltype in stringlist_copy_aux: %d\n", sltype );
		}

	return( 0 );
}

/**************************************************************************************
 * Common prologue code for stringlist_add_string and stringlist_add_string_ordered
 * Returns 0 on success, -1 on error (usually inability to allocate memory)
 */
	static int
stringlist_add_string_common( Stringlist **list, Stringlist **new_el, char *new_string, void *aux, int sltype )
{
	int	err;

	/* Check input args */
	if( (err = stringlist_check_args( list, new_string, aux, sltype )) != 0 ) {
		fprintf( stderr, "stringlist_add_string: error, bad arguments passed\n" );
		return(err);
		}

	/* Make new stringlist element */
	if( (err = stringlist_new_sl( new_el )) < 0 ) 
		return( err );

	/* Copy name over to new element */
	if( (err = stringlist_copy_name( *new_el, new_string )) != 0 ) {
		fprintf( stderr, "stringlist_add_string: error encountered when copying name to new element\n" );
		return(err);
		}

	/* Copy aux data to new element */
	if( (err = stringlist_copy_aux( *new_el, aux, sltype )) != 0 ) {
		fprintf( stderr, "stringlist_add_string: error encountered when copying aux data to new element\n" );
		return(err);
		}

	return( 0 );
}

/*******************************************************************************
 * Concatenate one stringlist onto the end of another stringlist. I.e., if
 * dest is a stringlist, and src is a stringlist, this returns (dest, src)
 * This COPIES data to a new entry on the dest list, so if you are done with 
 * the src list, it is OK to delete it (and should be deleted).
 *
 * Returns 0 on success, -1 on error (usually inability to allocate memory)
 */
	int
stringlist_cat( Stringlist **dest, Stringlist **src )
{
	Stringlist *sl;
	int	err;

	if( dest == NULL ) {
		fprintf( stderr, "stringlist_cat: Error, null reference to a destination stringlist passed!\n" );
		return( -6 );
		}
	if( src == NULL ) {
		fprintf( stderr, "stringlist_cat: Error, null reference to a source stringlist passed!\n" );
		return( -7 );
		}

	if( (*dest != NULL) &&  ((*dest)->magic != SL_MAGIC) ) {
		if( (*dest)->magic == SL_BAD_MAGIC ) {
			fprintf( stderr, "stringlist: Error in stringlist_cat: trying to use a previously deleted stringlist element (bad magic number)\n" );
			return( -16 );
			}
		fprintf( stderr, "stringlist_cat: Error, passed destination stringlist is corrupted or malformed (has wrong magic number)\n" );
		return( -47 );
		}

	if( *src == NULL ) 
		return(0);	/* Nothing to do */
	if( (*src)->magic != SL_MAGIC ) {
		if( (*src)->magic == SL_BAD_MAGIC ) {
			fprintf( stderr, "stringlist: Error in stringlist_cat: trying to use a previously deleted stringlist element (bad magic number)\n" );
			return( -17 );
			}
		fprintf( stderr, "stringlist_cat: Error, passed source stringlist is corrupted or malformed (has wrong magic number)\n" );
		return( -8 );
		}

	sl = *src;
	while( sl != NULL ) {
		if( (err = stringlist_add_string( dest, sl->string, sl->aux, sl->sltype )) < 0 ) 
			return( err );
		sl = sl->next;
		}

	return(0);
}

/*******************************************************************************
 * Allocate space for a new Stringlist element 
 * was: new_stringlist
 * Returns 0 on success, -1 on error
 */
	static int
stringlist_new_sl( Stringlist **el )
{
	(*el)         = (Stringlist *)malloc( sizeof( Stringlist ));
	if( *el == NULL ) {
		fprintf( stderr, "stringlist_new_sl: error, failed to allocate memory for a new stringlist element\n" );
		return( -1 );
		}

	(*el)->magic  = SL_MAGIC;
	(*el)->string = NULL;
	(*el)->next   = NULL;
	(*el)->prev   = NULL;
	(*el)->index  = -1;
	(*el)->aux    = NULL;
	(*el)->sltype = SLTYPE_NULL;

	return(0);
}

/******************************************************************************
 * What's in this stringlist, anyway?
 * was: dump_stringlist
 */
	void
stringlist_dump( Stringlist *s )
{
	if( s == NULL ) {
		printf( "<--- null pointer --->\n" );
		return;
		}

	while( s != NULL ) {

		if( s->magic != SL_MAGIC ) {
			if( s->magic == SL_BAD_MAGIC ) {
				fprintf( stderr, "stringlist: Error in stringlist_dump: trying to use a previously deleted stringlist (bad magic number)\n" );
				return;
				}
			fprintf( stderr, "stringlist_dump: Error, passed stringlist is corrupted or malformed (has wrong magic number)\n" );
			return;

			}

		printf( "MAGIC=%d ", s->magic );
		printf( "ADDR=%ld ", (long)s );
		printf( "PREV=%ld NEXT=%ld ", (long)s->prev, (long)s->next );
		printf( "INDEX=%d: ", s->index );
		if( s->string == NULL ) 
			printf( "(null string) " );
		else
			printf( "\"%s\" ", s->string );
		switch (s->sltype) {
			case SLTYPE_NULL:
				printf( "NULL --\n" );
				break;
			case SLTYPE_INT:
				printf( "INT %d\n", *((int *)(s->aux)) );
				break;
			case SLTYPE_STRING:
				printf( "STRING \"%s\"\n", ((char *)(s->aux)) );
				break;
			case SLTYPE_FLOAT:
				printf( "FLOAT %f\n", *((float *)(s->aux)) );
				break;
			case SLTYPE_BOOL:
				if( *((int *)(s->aux)) == 1 )
					printf( "BOOL TRUE\n" );
				else
					printf( "BOOL FALSE\n" );
				break;
			default:
				printf ("ERROR, unknown type in dump_stringlist: %d\n", s->sltype );
			}
		s = s->next;
		}
}

/******************************************************************************
 * Returns the number of entries in the Stringlist
 * was: n_strings_in_list
 */
	int
stringlist_len( Stringlist *s )
{
	Stringlist *c;
	int	   i;

	if( (s != NULL) && (s->magic != SL_MAGIC) ) {
		if( s->magic == SL_BAD_MAGIC ) {
			fprintf( stderr, "stringlist: Error in stringlist_len: trying to use a previously deleted stringlist element (bad magic number)\n" );
			return( -18 );
			}
		fprintf( stderr, "stringlist_len: Error, passed stringlist is corrupted or malformed (has wrong magic number)\n" );
		return( -8 );
		}

	i = 0;
	c = s;
	while( c != NULL ) {
		i++;
		c = c->next;
		}

	return( i );
}

/*******************************************************************************************
 * Checks args for correctness. Returns 0 on success (no error), -1 if there is a problem.
 */
	static int
stringlist_check_args( Stringlist **list, char *new_string, void *aux, int sltype )
{
	/* Can't be passed a null REFERENCE to the stringlist, what does that mean?? */
	if( list == NULL ) {
		fprintf( stderr, "stringlist: error, passed a null reference to a stringlist\n" );
		return( -5 );
		}

	/* It's OK if the passed list itself is NULL. That just means we haven't put anything
	 * on this list yet. However, if it's NOT null, then it must point to a valid
	 * stringlist element.
	 */
	if( (*list != NULL) && ((*list)->magic != SL_MAGIC) ) {
		if( (*list)->magic == SL_BAD_MAGIC ) {
			fprintf( stderr, "stringlist: Error in stringlist_check_args: trying to use a previously deleted stringlist element (bad magic number)\n" );
			return( -15 );
			}
		fprintf( stderr, "stringlist: Error, passed stringlist is corrupted or malformed (has wrong magic number)\n" );
		return( -3 );
		}

	if( new_string == NULL ) {
		fprintf( stderr, "stringlist: NULL pointer to character string passed\n" );
		return(-1);
		}

	/* Check input args */
	if( strlen(new_string) > STRINGLIST_MAX_LEN ) {
		fprintf( stderr, "stringlist: error, trying to add string that is too long to a stringlist.\n" );
		fprintf( stderr, "String trying to add:\n" );
		fprintf( stderr, "\"%s\"\n", new_string );
		return(-1);
		}

	if( (sltype != SLTYPE_NULL)   && 
	    (sltype != SLTYPE_INT)    &&
	    (sltype != SLTYPE_STRING) &&
	    (sltype != SLTYPE_FLOAT)  &&
	    (sltype != SLTYPE_BOOL)) {
	    	fprintf( stderr, "stringlist: error, an invalid aux data type was given: %d\n", sltype );
		return(-1);
		}

	if( (sltype == SLTYPE_STRING) && (strlen( (char *)aux ) > STRINGLIST_MAX_LEN) ) {
		fprintf( stderr, "stringlist_check_args: error, trying to add auxilliary string data to a stringlist element and that string is longer than allowed max of %d\n",
			STRINGLIST_MAX_LEN );
		return( -68 );
		}

	return(0);
}

/**************************************************************************************
 * Returns 0 on success, -1 on error (usually inability to allocate memory)
 */
	static int
stringlist_copy_name( Stringlist *new_el, char *new_string ) 
{
	if( new_el == NULL ) {
		fprintf( stderr, "stringlist: error, trying to copy a string to a NULL stringlist element!\n" );
		return( -1 );
		}
	if( new_el->magic != SL_MAGIC ) {
		if( new_el->magic == SL_BAD_MAGIC ) {
			fprintf( stderr, "stringlist: Error in stringlist_copy_name: trying to use a previously deleted stringlist element (bad magic number)\n" );
			return( -19 );
			}
		fprintf( stderr, "stringlist_copy_name: Error, passed stringlist element is corrupted or malformed (has wrong magic number)\n" );
		return( -9 );
		}

	if( new_string == NULL ) {
		fprintf( stderr, "stringlist: error, trying to copy a NULL to a stringlist name!\n" );
		return( -1 );
		}

	if( strlen(new_string) > STRINGLIST_MAX_LEN ) {
		fprintf( stderr, "stringlist: error, specified a string that is illegally long: %s\n", new_string );
		return( -72 );
		}

	new_el->string = (char *)malloc( strlen( new_string )+1);
	if( new_el == NULL ) {
		fprintf( stderr, "stringlist_copy_name: malloc failed\n" );
		fprintf( stderr, "string trying to add: %s\n", new_string );
		return( -1 );
		}
	strcpy( new_el->string, new_string );

	return(0);
}

/**************************************************************************************
 * Returns 0 on success, -1 on error 
 * Note this allocates all space needed for the stringlist, it must not be pre-allocated
 *
 * Structure of input file:
 *
 * integer (index #)          "string"         aux_type    aux_value
 * 
 * where aux_value will be in "quotes" if aux_type == STRING
 */
 	int
 stringlist_read_from_file( Stringlist **sl, FILE *fin ) 
 {
 	char		line[4001];
	int		err, version_number, lineno, debug;
	Stringlist 	*header_el = NULL;

	/* Check for obvious errors */
	if( sl == NULL ) {
		fprintf( stderr, "stringlist_read_from_file: error, passed a NULL pointer\n" );
		exit(-1);
		}
	if( *sl != NULL ) {
		fprintf( stderr, "stringlist_read_from_file: error, bad coding, should be passed a pointer to a NULL list\n" );
		exit(-1);
		}

	debug = 0;

	if( debug ) printf( "stringlist_read_from_file: entering\n" );

	/* Advance to our header: it should read something like this:
	 * -1 "STRINGLIST_SAVE_FILE_VERSION" INT 1
	 */
	lineno = 0;
	version_number = -1;
	while( fgets( line, 4000, fin ) != NULL ) {

		if( debug ) printf( "stringlist_read_from_file: line %d: >%s<\n", lineno+1, line );

		if( strncmp( line, "-1 \"STRINGLIST_SAVE_FILE_VERSION\" INT", 36 ) == 0 ) {

			/* found the header */
			if( debug ) printf( "stringlist_read_from_file: found header line\n" );

			/* chomp trailing LF */
			line[ strlen(line)-1 ] = '\0';

			if( (err = stringlist_line_to_sl( line, lineno, &header_el )) != 0 ) {
				fprintf( stderr, "stringlist_read_from_file: error reading header line from file!\n" );
				return( err );
				}

			if( header_el->sltype != SLTYPE_INT ) {
				fprintf( stderr, "stringlist_read_from_file: error reading save file version number from file\n" );
				return( -53 );
				}

			version_number = *((int *)(header_el->aux));
			if( debug ) printf( "stringlist_read_from_file: save file version number %d\n", version_number );
			if( version_number == 1 ) 
				return( stringlist_read_from_file_v1( sl, fin, lineno, debug ));
			else
				{
				fprintf( stderr, "This version of the code cannot read save file version number %d\n", version_number );
				return( -53 );
				}
			}
		}

	return(0);
 }

/**************************************************************************************
 * Reads a version 1 stringlist save file
 */
 	static int
 stringlist_read_from_file_v1( Stringlist **sl, FILE *fin, int nlines_in_header, int debug ) 
 {
	char		line[4001];
	int		err, lineno;
	Stringlist 	*new_el;

	if( debug ) printf( "stringlist_read_from_file_v1: entering\n" );

	lineno = 0;
	while( fgets( line, 4000, fin ) != NULL ) {

		if( debug ) printf( "stringlist_read_from_file_v1: line %d: >%s<\n", lineno+nlines_in_header, line );

		if( (strlen(line)>2) && (line[0] != '\0') && (line[0] != '#' )) {

			/* chomp trailing LF */
			line[ strlen(line)-1 ] = '\0';

			new_el = NULL; 	/* This flags that we want to make a brand new element, and alloc space for it */

			if( (err = stringlist_line_to_sl( line, lineno+nlines_in_header, &new_el )) != 0 ) 
				return( err );

			/* Now add this new information to our stringlist */
			stringlist_cat( sl, &new_el );
			
			/* Free storage associated with temp element */
			if( (err = stringlist_delete_single_element_inner( new_el )) != 0 ) {
				fprintf( stderr, "stringlist_read_from_file_v1: error trying to free tmp storage\n" );
				return( err );
				}
			}
		lineno++;
		}

	return(0);
}

/**************************************************************************************
 * Given a line of chars in savelist save file format, this creates a single stringlist
 * element holding that info
 * Returns 0 on success, < 0 on error
 */
	static int
stringlist_line_to_sl( char *line, int lineno, Stringlist **retval )
{
	int	ival, debug, err, index0, index1, string0, string1, aux_type0, aux_type1, aux_val0, aux_val1,
		nfields;
	char	t_index[4000], t_string[4000], t_aux_type[4000], t_aux_val[4000], 
		*t_string_ue, *t_aux_val_ue;
	float	fval;

	debug = 0;

	/* Get indices (into the string char array) of the 
	 * first and last positions of each of the four
	 * elements (index_num, string, aux_type, and aux_value)
	 */
	err = stringlist_get_tok_indices( line, &index0, &index1, 
			&string0,   &string1, 
			&aux_type0, &aux_type1, 
			&aux_val0,  &aux_val1 );
	if( err != 0 ) {
		fprintf( stderr, "stringlist_read_from_file: error, could not parse information on input file line %d\n", lineno+1 );
		fprintf( stderr, "here was the problematical line:\n" );
		fprintf( stderr, ">%s<\n", line );
		return( err );
		}
	strncpy( t_index,    line+index0,    index1-index0+1 );
	t_index[index1-index0+1] = '\0';

	strncpy( t_string,   line+string0,   string1-string0+1 );
	t_string[string1-string0+1] = '\0';
	if( strlen(t_string) > STRINGLIST_MAX_LEN ) {
		fprintf( stderr, "stringlist_line_to_sl: error, encountered a string that is too long. Max allowed: %d. Found: %ld. Line number: %d\n",
			STRINGLIST_MAX_LEN, strlen(t_string), lineno );
		return( -63 );
		}
	t_string_ue = stringlist_unescape_string( t_string );

	strncpy( t_aux_type, line+aux_type0, aux_type1-aux_type0+1 );
	t_aux_type[aux_type1-aux_type0+1] = '\0';

	strncpy( t_aux_val,  line+aux_val0,  aux_val1-aux_val0+1 );
	t_aux_val[aux_val1-aux_val0+1] = '\0';

	if( debug ) {
		printf( "line: >%s<\n", line );
		printf( "index:>%s< string:>%s< aux_type:>%s< aux_val:>%s<\n",
			t_index, t_string_ue, t_aux_type, t_aux_val );
		}

	/* Now put this information into a new stringlist element */
	if( strcmp( t_aux_type, "NULL" ) == 0 ) {
		if( (err = stringlist_add_string( retval, t_string_ue, NULL, SLTYPE_NULL )) != 0 ) {
			fprintf( stderr, "stringlist_read_from_file: error encountered while processing line %d\n", lineno+1 );
			return( err );
			}
		}

	else if( strcmp( t_aux_type, "BOOL" ) == 0 ) {
		if( strcmp( t_aux_val, "TRUE" ) == 0 ) {
			ival = 1;
			if( (err = stringlist_add_string( retval, t_string_ue, &ival, SLTYPE_BOOL )) != 0 ) {
				fprintf( stderr, "stringlist_read_from_file: error encountered while processing line %d\n", lineno+1 );
				return( err );
				}
			}

		else if( strcmp( t_aux_val, "FALSE" ) == 0 ) {
			ival = 0;
			if( (err = stringlist_add_string( retval, t_string_ue, &ival, SLTYPE_BOOL )) != 0 ) {
				fprintf( stderr, "stringlist_read_from_file: error encountered while processing line %d\n", lineno+1 );
				return( err );
				}
			}
		else
			{
			fprintf( stderr, "Error, got a boolean type that is neither TRUE nor FALSE, it is: %s\n",
				t_aux_val );
			fprintf( stderr, "stringlist_read_from_file: error encountered while processing line %d\n", lineno+1 );
			return( -38 );
			}
		}

	else if( strcmp( t_aux_type, "INT" ) == 0 ) {
		nfields = sscanf( t_aux_val, "%d", &ival );
		if( nfields != 1 ) {
			fprintf( stderr, "stringlist_read_from_file: error, while reading line %d could not get a single integer from token >%s<\n", 
				lineno+1, t_aux_val );
			return( -30 );
			}
		if( (err = stringlist_add_string( retval, t_string_ue, &ival, SLTYPE_INT )) != 0 ) {
			fprintf( stderr, "stringlist_read_from_file: error encountered while processing line %d\n", lineno+1 );
			return( err );
			}
		}

	else if( strcmp( t_aux_type, "FLOAT" ) == 0 ) {
		nfields = sscanf( t_aux_val, "%f", &fval );
		if( nfields != 1 ) {
			fprintf( stderr, "stringlist_read_from_file: error, while reading line %d could not get a single float from token >%s<\n", 
				lineno+1, t_aux_val );
			return( -31 );
			}
		if( (err = stringlist_add_string( retval, t_string_ue, &fval, SLTYPE_FLOAT )) != 0 ) {
			fprintf( stderr, "stringlist_read_from_file: error encountered while processing line %d\n", lineno+1 );
			return( err );
			}
		}

	else if( strcmp( t_aux_type, "STRING" ) == 0 ) {
		t_aux_val_ue = stringlist_unescape_string( t_aux_val );
		if( strlen(t_aux_val_ue) > STRINGLIST_MAX_LEN ) {
			fprintf( stderr, "stringlist_line_to_sl: error, encountered a string that is too long. Max allowed: %d. Found: %ld. Line number: %d\n",
				STRINGLIST_MAX_LEN, strlen(t_aux_val_ue), lineno );
			return( -67 );
			}
		if( (err = stringlist_add_string( retval, t_string_ue, t_aux_val_ue, SLTYPE_STRING )) != 0 ) {
			fprintf( stderr, "stringlist_read_from_file: error encountered while processing line %d\n", lineno+1 );
			return( err );
			}
		free( t_aux_val_ue );
		}

	else
		{
		fprintf( stderr, "stringlist_read_from_file: encountered unhandled type: %s\n", 
			t_aux_type );
		return( -37 );
		}

	free( t_string_ue );	/* this is the un-escaped version of what we read in */

	return(0);
}

/**************************************************************************************
 */
	static int 
stringlist_get_tok_indices( char *line, int *index0, int *index1, int *string0, int *string1, 
		int *aux_type0, int *aux_type1, int *aux_val0, int *aux_val1 )
{
	int	cursor, slen, aux_val_is_string;

	if( line == NULL )
		return( -1 );

	cursor = 0;
	slen   = strlen( line );

	/* ========= GET INDEX ======= */
	/* Advance past initial whitespace */
	while( (cursor < slen) && my_isblank( line[cursor] ))
		cursor++;
	/* cursor is now NOT white space */
	if( cursor == slen ) {
		fprintf( stderr, "stringlist_get_tok_indices: error reading from file, is input line all blank?\n" );
		return(-1);	/* line is all blank? */
		}
	*index0 = cursor;

	/* Advance to next white space */
	while( (cursor < slen) && (! my_isblank(line[cursor]))) 
		cursor++;
	/* cursor is now white space */
	if( cursor == slen ) {
		fprintf( stderr, "stringlist_get_tok_indices: error reading from file, does line end immeidiately after the index number?\n" );
		return(-1);
		}
	*index1 = cursor-1;

	/* ========= GET STRING ======= */
	/* Advance past initial whitespace */
	while( (cursor < slen) && my_isblank( line[cursor] ))
		cursor++;
	if( cursor == slen ) {
		fprintf( stderr, "stringlist_get_tok_indices: error reading from file, does line have ONLY the index number, no string?\n" );
		return(-1);	
		}
	/* cursor is now NOT white space, needs to be a quote sign (") */
	if( line[cursor] != '"' ) {
		fprintf( stderr, "stringlist_get_tok_indices: error reading from file, does string NOT start with a quote sign?\n" );
		return(-1);
		}
	cursor++;	/* advance past quote sign */
	*string0 = cursor;

	/* Advance to next quote that is NOT escaped by a backslash */
	/* an quote that is NOT escaped is this: ((line[cursor] == '"') && (line[cursor-1] != '\')) */
	/* "while line[cursor] is NOT a (quote that is NOT escaped by a backslash)" */
	while( (cursor < slen) && (! ((line[cursor] == '"') && (line[cursor-1] != '\\'))) )
		cursor++;
	if( cursor == slen ) {
		fprintf( stderr, "stringlist_get_tok_indices: error reading from file, does string not end with a quote sign?\n" );
		return(-1);	
		}
	/* cursor is now on an NOT escaped quote */
	*string1 = cursor - 1;
	cursor++;	/* advance past the NON escaped quote, should be whitespace */

	/* ========= GET AUX TYPE ======= */
	/* Advance past initial whitespace */
	while( (cursor < slen) && my_isblank( line[cursor] ))
		cursor++;
	/* cursor is now NOT white space */
	if( cursor == slen ) {
		fprintf( stderr, "stringlist_get_tok_indices: error reading from file, does line end before specifying an aux_type?\n" );
		return(-1);
		}
	*aux_type0 = cursor;

	/* Advance to next white space */
	while( (cursor < slen) && (! my_isblank(line[cursor]))) 
		cursor++;
	/* cursor is now white space */
	if( cursor == slen ) {
		fprintf( stderr, "stringlist_get_tok_indices: error reading from file, does line end immediately after the aux_type?\n" );
		return(-1);
		}
	*aux_type1 = cursor-1;

	/* ========== GET AUX VAL ========
	 * If it starts with a quote, we assume it's a string 
	 * and go to the matching unescaped quote.
	 * Otherwise, just go to first whitespace or we
	 * run out of string.
	 */
	/* Advance past initial whitespace */
	while( (cursor < slen) && my_isblank( line[cursor] ))
		cursor++;
	if( cursor == slen ) {
		fprintf( stderr, "stringlist_get_tok_indices: error reading from file, does line end before an aux_val is specified?\n" );
		return(-1);
		}
	/* cursor is now NOT white space */
	aux_val_is_string = (line[cursor] == '"');

	if( aux_val_is_string ) {
		cursor++;	/* Advance past the opening quote */
		*aux_val0 = cursor;
		/* Advance to next quote that is NOT escaped by a backslash; see above */
		while( (cursor < slen) && (! ((line[cursor] == '"') && (line[cursor-1] != '\\'))) )
			cursor++;
		if( cursor == slen ) {
			fprintf( stderr, "stringlist_get_tok_indices: error reading from file, does the string-valued aux_val NOT end with a quote?\n" );
			return(-1);	
			}
		/* cursor is now on an NOT escaped quote */
		*aux_val1 = cursor - 1;
		}
	else
		{
		*aux_val0 = cursor;
		while( (cursor < slen) && (! my_isblank(line[cursor]))) 
			cursor++;
		/* cursor is now white space */
		*aux_val1 = cursor-1;
		}

	return(0);
}

/**************************************************************************************
 * Returns 0 on success, -1 on error (usually inability to allocate memory)
 */
 	int
 stringlist_write_to_file( Stringlist *sl, FILE *fout ) 
 {
	Stringlist *cursor, *header;
	int	err, itmp;

	/* Note that if we are passed a NULL, we do not even write a header */
 	if( sl == NULL )
		return(0);

	/* Stringlists are supposed to be small amounts of data. I add this check
	 * as a security issue ... I do not want stringlists to be able to be
	 * tricked into filling up a disk or something like that.
	 */
	if( stringlist_len( sl ) > STRINGLIST_MAX_NSTRINGS_WRITE ) {
		fprintf( stderr, "stringlist_write_to_file: error, can only write a maximum of %d strings, but passed list had %d\n",
			STRINGLIST_MAX_NSTRINGS_WRITE, stringlist_len( sl ) );
		return( -61 );
		}

	/* We add, as a header, the version of this save file */
	if( (stringlist_new_sl( &header )) != 0 ) {
		fprintf( stderr, "stringlist_write_to_file: error trying to create header for file\n" );
		return( -40 );
		}
	/* Copy name over to new element */
	if( (err = stringlist_copy_name( header, "STRINGLIST_SAVE_FILE_VERSION" )) != 0 ) {
		fprintf( stderr, "stringlist_write_to_file: error encountered when copying save file version tag to header\n" );
		return(err);
		}
	/* Copy aux data to new element */
	itmp = STRINGLIST_SAVEFILE_VERSION;
	if( (err = stringlist_copy_aux( header, &itmp, SLTYPE_INT )) != 0 ) {
		fprintf( stderr, "stringlist_add_string: error encountered when copying save file version number to header\n" );
		return(err);
		}

	/* Write header to file */
	if( (err = stringlist_write_single_element_to_file( header, fout )) != 0 ) 
		return( err );

	/* Delete header. Cheat here by using the quicker locally-known call to delete 
	 * just a single element
	 */
	if( (err = stringlist_delete_single_element_inner( header )) != 0 )
		return( err );

	/* Write stringlist contents to file */
	cursor = sl;
	while( cursor != NULL ) {
		if( strlen( cursor->string ) > STRINGLIST_MAX_LEN ) {
			fprintf( stderr, "Error, trying to write an illegally long string to the file. Offending string: %s\n",
				cursor->string );
			return( -69 );
			}
		if( (cursor->sltype == SLTYPE_STRING) && (strlen( (char *)(cursor->aux) ) > STRINGLIST_MAX_LEN) ) {
			fprintf( stderr, "Error, trying to write an illegally long aux information string to the file. Offending aux info string: %s\n",
				(char *)(cursor->aux) );
			return( -70 );
			}
		if( (err = stringlist_write_single_element_to_file( cursor, fout )) != 0 ) 
			return( err );
		cursor = cursor->next;
		}

	return( 0 );
}

/**************************************************************************************
 * Returns 0 on success, -1 on error 
 */
 	static int
 stringlist_write_single_element_to_file( Stringlist *sl, FILE *fout ) 
 {
	char	*string_esc;	/* Escaped version of the string */

 	if( sl == NULL )
		return(0);

	if( sl->magic != SL_MAGIC ) {
		if( sl->magic == SL_BAD_MAGIC ) {
			fprintf( stderr, "stringlist: Error in stringlist_write_to_file: trying to use a previously deleted stringlist element (bad magic number)\n" );
			return( -15 );
			}
		fprintf( stderr, "stringlist_write_to_file: Error, passed stringlist is corrupted or malformed (has wrong magic number)\n" );
		return( -10 );
		}
	string_esc = stringlist_escape_string( sl->string );
	fprintf( fout, "%d \"%s\"", sl->index, string_esc );
	free( string_esc );

	switch( sl->sltype ) {
		case SLTYPE_NULL:
			fprintf( fout, " NULL NULL\n" );
			break;

		case SLTYPE_INT:
			fprintf( fout, " INT %d\n", *((int *)(sl->aux)) );
			break;

		case SLTYPE_FLOAT:
			fprintf( fout, " FLOAT %f\n", *((float *)(sl->aux)) );
			break;

		case SLTYPE_BOOL:
			if( *((int *)(sl->aux)) == 1 )
				fprintf( fout, " BOOL TRUE\n" );
			else
				fprintf( fout, " BOOL FALSE\n" );
			break;

		case SLTYPE_STRING:
			string_esc = stringlist_escape_string( (char *)(sl->aux) );
			fprintf( fout, " STRING \"%s\"\n", string_esc );
			free( string_esc );
			break;

		default:
			fprintf( stderr, "stringlist_write_to_file: error, encountered unknown aux data type %d\n", sl->sltype );
			return( -30 );
		}

	return(0);
}

/**************************************************************************************
 * Deletes a single element, but does NOT try to mess with pointers to take
 * it out of the list. Just a simplistic delete of storage associated with this
 * element.
 */
	static int
stringlist_delete_single_element_inner( Stringlist *sl ) 
{
	if( sl == NULL ) {
		fprintf( stderr, "stringlist_delete_single_element_inner: error, passed a NULL element\n" );
		return( -11 );
		}

	if( sl->magic != SL_MAGIC ) {
		if( sl->magic == SL_BAD_MAGIC ) {
			fprintf( stderr, "stringlist: Error in stringlist_delete_single_element: trying to delete a previously deleted stringlist element (bad magic number)\n" );
			return( -15 );
			}
		fprintf( stderr, "stringlist_delete_single_element_inner: error, corrupted or malformed stringlist element encountered\n" );
		return( -12 );
		}

	/* Mark this element as deleted */
	sl->magic = SL_BAD_MAGIC;

	if( sl->string != NULL ) 
		free( sl->string );

	if( sl->sltype == SLTYPE_STRING ) {
		if( sl->aux != NULL ) 
			free( sl->aux );
		}

	free( sl );

	return( 0 );
}

/**************************************************************************************
 * Deletes this element, and all elements both before and after it on the list
 * Returns 0 on success, -1 on error 
 */
	int
stringlist_delete_entire_list( Stringlist *sl ) 
{
	int		err;
	Stringlist	*sl_prev, *sl_next, *tt;

	if( sl == NULL )
		return( 0 );	/* that was easy */

	/* Delete all prev elements */
	sl_prev = sl->prev;
	while( sl_prev != NULL ) {
		tt = sl_prev->prev;
		if( (err=stringlist_delete_single_element_inner( sl_prev )) != 0) {
			fprintf( stderr, "stringlist_delete_entire_list: error encountered\n" );
			return( err );
			}
		sl_prev = tt;
		}

	/* Delete all next elements */
	sl_next = sl->next;
	while( sl_next != NULL ) {
		tt = sl_next->next;
		if( (err = stringlist_delete_single_element_inner( sl_next )) != 0 ) {
			fprintf( stderr, "stringlist_delete_entire_list: error encountered\n" );
			return( err );
			}
		sl_next = tt;
		}

	stringlist_delete_single_element_inner( sl );

	return( 0 );
}

/**************************************************************************************
 * Creates an "UNescaped" version of a string, that is, one with backslashes before
 * all quote or backslashes removed.
 *
 * NOTE that this allocates space for the return string, so calling code must
 * do a free( retval ) after using the return value!
 */
	static char *
stringlist_unescape_string( char *ss ) 
{
	char	*retval;
	int	ii, iout;

	if( ss == NULL ) return( NULL );
	if( strlen(ss) == 0 ) {
		retval = (char *)malloc( sizeof(char) );
		*retval = '\0';
		return( retval );
		}

	/* Unescaped version can never be longer than original size */
	retval = (char *)malloc( sizeof(char)*strlen(ss) + 1 );
	if( retval == NULL ) {
		fprintf( stderr, "Error in stringlist routine stringlist_unescape_string: could not allocate new string of length %ld\n", (strlen(ss)+1) );
		exit(-1);
		}

	iout = 0;
	for( ii=0; ii<strlen(ss); ii++ ) {
		if( ss[ii] == '\\' ) 
			ii++;
		retval[iout++] = ss[ii];
		}

	retval[iout] = '\0';
	return( retval );
}

/**************************************************************************************
 * Creates an "escaped" version of a string, that is, one with backslashes before
 * all quote or backslashes.
 *
 * NOTE that this allocates space for the return string, so calling code must
 * do a free( retval ) after using the return value!
 */
 	static char *
stringlist_escape_string( char *ss ) 
{
	char 	*retval;
	int	ii, iout;

	if( ss == NULL ) return(NULL);
	if( strlen(ss) == 0 ) {
		retval = (char *)malloc( sizeof(char) );
		*retval = '\0';
		return( retval );
		}

	/* Max length the escaped version can be is twice orig size */
	retval = (char *)malloc( sizeof(char) * 2 * (strlen(ss)+1) );
	if( retval == NULL ) {
		fprintf( stderr, "Error in stringlist routine stringlist_escape_string: could not allocate new string of length %ld\n", 2 * (strlen(ss)+1) );
		exit(-1);
		}

	iout = 0;
	for( ii=0; ii<strlen(ss); ii++ ) {
		if( (ss[ii] == '"') || (ss[ii] == '\\')) {
			retval[iout++] = '\\';
			}
		retval[iout++] = ss[ii];
		}
	retval[iout] = '\0';

	return( retval );
}

