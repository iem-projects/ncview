
#ifndef SEEN_STRINGLIST_H
#define SEEN_STRINGLIST_H

/*-------------------------------------------
 * How long the string in a stringlist can be
 *-------------------------------------------*/
#define STRINGLIST_MAX_LEN	1000

/*-------------------------------------------------------------------
 * When writing to a file, we will never write more strings than this
 *-------------------------------------------------------------------*/
#define STRINGLIST_MAX_NSTRINGS_WRITE 	10000

/*-----------------------------------------------
 * Types that the aux data in a stringlist can be
 *-----------------------------------------------*/
#define SLTYPE_NULL	0	
#define SLTYPE_INT	1
#define SLTYPE_STRING	2
#define SLTYPE_FLOAT	3
#define SLTYPE_BOOL	4

/* A signature to check for bad values or corruption 
 * The 'bad' one is what we set deleted elements to,
 * so maybe we can detect trying to use a deleted
 * stringlist.
 */
#define SL_MAGIC	93823
#define SL_BAD_MAGIC	13423

/* Version number for the save file */
#define STRINGLIST_SAVEFILE_VERSION	1

/*****************************************************************************/
/* A general purpose list of character strings */
typedef struct {
	int	magic;		/* Will be SL_MAGIC for valid elements */
	char	*string;
	void	*next, *prev;
	int	index;		/* initialized to position in list */
	void	*aux;		/* auxilliary data */
	int	sltype;		/* one of the defined SLTYPEs, indicating type of aux */
} Stringlist;


/* Return 0 on success, -1 on error (usually inability to allocate memory */
int 	stringlist_add_string( Stringlist **list, char *new_string, void *aux, int sltype );

/* Return 0 on success, -1 on error (usually inability to allocate memory */
int 	stringlist_add_string_ordered( Stringlist **list, char *new_string, void *aux, int sltype );

/* Return 0 on success, -1 on error (usually inability to allocate memory */
int 	stringlist_cat( Stringlist **dest, Stringlist **src );

/* Return 0 on success, -1 on error (usually inability to allocate memory */
int 	stringlist_write_to_file( Stringlist *sl, FILE *fout );

/* Return 0 on success, -1 on error (usually inability to allocate memory */
int 	stringlist_read_from_file( Stringlist **sl, FILE *fin );

/* Return NULL if the string is not found, and pointer to matching element if it is found */
Stringlist *stringlist_match_string_exact( Stringlist *list, char *str );

/* Return 0 on success, -1 on error */
int 	stringlist_delete_entire_list( Stringlist *sl );


void 	stringlist_dump( Stringlist *s );
int 	stringlist_len( Stringlist *s );

#endif

