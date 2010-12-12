#include <stdio.h>
#include <string.h>

/* 
 * utf8_strlen - returns the number of CHARACTERS in the string
 */
int utf8_strlen( const char * str )
{
	int nb = 0;

	while( *str ){
		/* This is because each NEW character has the first two bits set to '00' or '11' */
		if( (*str & 0xc0 ) != 0x80 ) nb++;
		str ++;
	}

	return nb;
}

/*
 * utf8_arrlen - returns the number of bytes in the array 
 */
int utf8_bytelen( const char * const str ){
	return strlen(str);
}
