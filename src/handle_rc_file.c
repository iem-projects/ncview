#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ncview.includes.h"
#include "ncview.defines.h"
#include "ncview.protos.h"

#include "stringlist.h"

#define NCVIEW_STATE_FILE_VERSION	1

/* 
These routines handle the initilization (rc)/state file for ncview.

Note that ncview can already use an x windows type apps file. However,
the syntax of that is so convoluted and arcane that it's too difficult
to use in general. For ncview options (as opposed to X widows options),
I've introduced this new file just for ncview that stores its state
between invocations. By default it's $(HOME)/.ncviewrc

I've implemented the state file using Stringlists.  You can write
out a Stringlist to a file and read one in from a file, so I coerce
the state into a Stringlist and then use that functionalilty.

David W Pierce
Scripps Insititution of Oceanography
15 July 2011
*/

/*================================================================================
 * Get the persistent state that we want to save to a file
 * This generates and allocates space for a new stringlist, so when the calling routine
 * is done with the stringlist (by saving it to a file, or example), then the calling
 * routine must delete the stringlist as so:
 *
 *      state_to_save = get_persistent_state();
 *      stringlist_delete_entire_list( state_to_save );
 */
 	Stringlist *
get_persistent_state()
{
	/* Only one module saves state info at the moment */
	return( get_persistent_X_state() );
}

/*================================================================================
 * Write the state to a disk file. Returns 0 on success, != 0 on error.
 */
	int
write_state_to_file( Stringlist *state_to_save )
{
	char 	*homedir;
	char	tmp_fname[2000], final_fname[2000];
	Stringlist	*header = NULL;
	int	err, ival, outfid;
	FILE	*outf;

	/* Make our ncview header */
	ival = NCVIEW_STATE_FILE_VERSION;
	if( (err = stringlist_add_string( &header, "NCVIEW_STATE_FILE_VERSION", &ival, SLTYPE_INT )) != 0 ) {
		fprintf( stderr, "Error making header for ncview save state file\n" );
		return( -1 );
		}

	/* Alg: create a temporary file with the new state, then
	 * mv that to the actual state file.
	 */

	/* Get what temporary directory to use. NOTE: I do NOT write it to /tmp,
	 * because that could be on a different file system than the user's
	 * home directory, and posix call rename() is not guaranteed to work
	 * across file systems.  This is a reasonable solution anyway ... since
	 * we eventually will put the .rc file into the user's home dir, might
	 * as well find out right away if we can't write to it.
	 */
	homedir = getenv( "HOME" );
	if( homedir == NULL ) {
		fprintf( stderr, "Error, there is no $HOME environmental variable defined. That is where ncview puts the initialization file ... define that env var if you want ncview to save information between invocations.\n" );
		return( -78 );
		}

	/* Make template for temporary filename */
	if( strlen(homedir) > 1900 ) {
		fprintf( stderr, "Error, temporary directory name is too long: %s\n", homedir );
		return( -1 );
		}
	snprintf( final_fname, 2000, "%s/.ncviewrc",        homedir );
	final_fname[1999] = '\0';
	snprintf( tmp_fname,   2000, "%s/.ncviewrc.XXXXXX", homedir );
	tmp_fname[1999] = '\0';

	/* Make temporary file safely */
	outfid = mkstemp( tmp_fname );
	if( outfid == -1 ) {
		fprintf( stderr, "Error trying to open temporary file \"%s\" for writing!\n", tmp_fname );
		fprintf( stderr, "Ncview uses this to write out a state file, so that it can remember user-selected options between being run.\n" );
		fprintf( stderr, "If you would like to have this functionality, then fix the permissions so that ncview can write to that directory\n" );
		return( -1 );
		}

	/* Open it as a file stream */
	if( (outf = fopen( tmp_fname, "a" )) == NULL ) {
		fprintf( stderr, "Error opening temporary file \"%s\" for output!\n", tmp_fname );
		return( -1 );
		}

	close( outfid );	/* close file descriptor only */

	/* Add the header to the head of the stringlist to save */
	if( (err = stringlist_cat( &header, &state_to_save )) != 0 ) {
		fprintf( stderr, "Error trying to add header to state file to save\n" );
		return( err );
		}

	/* Write the state save information to the file */
	err = stringlist_write_to_file( header, outf );
	if( err != 0 ) {
		fprintf( stderr, "Error writing header for state information to temporary file \"%s\"\n", tmp_fname );
		return( -1 );
		}

	fclose( outf );

	/* Free storage associated with our header */
	stringlist_delete_entire_list( header );

	/* Now copy our temporary file over to its final name */
	if( rename( tmp_fname, final_fname ) != 0 ) {
		fprintf( stderr, "Error trying to copy the temporary version of the ncview options file (%s) over to the final file (%s). If you want ncview to remember options between runs, then fix permissions for this directory\n",
			tmp_fname, final_fname );
		return( -79 );
		}

	return(0);
}

/*================================================================================
 * Read the state from a disk file. Returns 0 on success, != 0 on error.
 */
	int
read_state_from_file( Stringlist **state )
{
	int		err;
	char 	*homedir;
	char	final_fname[2000];
	FILE	*fin;

	/* Handle obvious errors */
	if( state == NULL ) {
		fprintf( stderr, "read_state_from_file: passed a NULL pointer\n" );
		exit(-1);
		}
	if( *state != NULL ) {
		fprintf( stderr, "read_state_from_file: is supposed to be passed a NULL list to start with!\n" );
		exit(-1);
		}

	/* Get name of our state file */
	homedir = getenv( "HOME" );
	if( homedir == NULL ) {
		fprintf( stderr, "Error, there is no $HOME environmental variable defined. That is where ncview puts the initialization file ... define that env var if you want ncview to save information between invocations.\n" );
		return( -80 );
		}

	if( strlen(homedir) > 1900 ) {
		fprintf( stderr, "Error, environmental variable $HOME is too long (has too many characters)\n" );
		return( -81 );
		}
	snprintf( final_fname, 2000, "%s/.ncviewrc", homedir );
	final_fname[1999] = '\0';

	/* Open the file for reading */
	if( (fin = fopen(final_fname, "r")) == NULL ) {
		fprintf( stderr, "Note: could not open file %s for reading\n", final_fname );
		return( -83 );
		}

	if( (err = stringlist_read_from_file( state, fin )) != 0 ) {
		fprintf( stderr, "Error reading in ncview's state from file %s\n", final_fname );
		return( -82 );
		}

	fclose( fin );
	return(0);
}

