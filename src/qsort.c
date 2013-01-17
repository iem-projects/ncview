qsort_sl( r, int lo, int up )
ArrayToSort r;
{
	int 	i, j;
	char	tempr;

	while ( up>lo ) {
		i = lo;
		j = up;
		tempr = r[lo];
		/*** Split file in two ***/
		while ( i<j ) {
			for ( ; r[j].k > tempr.k; j-- )
				;
			for ( r[i]=r[j]; i<j && r[i].k<=tempr.k; i++ )
				;
			r[j] = r[i];
			}
		r[i] = tempr;
		/*** Sort recursively, the smallest first ***/
		if ( i-lo < up-i ) { 
			sort(r,lo,i-1);  
			lo = i+1; 
			}
		else    
			{ 
			sort(r,i+1,up);  
			up = i-1; 
			}
		}
}
