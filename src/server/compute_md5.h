/**
 * @file compute_md5.h
 * @authors Matthew Lindner
 *
 * @brief Computes the MD5 of a file.
 * @details Computes and returns the MD5 of a file located at the file-path passed to the function.
 * 
 * Note: You should call free() of the char* variable storing the return value of this function. 
 *
 * @section References
 * This function was derived from an aswer to a question on askyb.com (http://bit.ly/15W5bID), and askovpen's post on StackOverflow (http://bit.ly/1yCh1SM).
 */

#ifndef COMPUTE_MD5_H
#define COMPUTE_MD5_H

#include <openssl/md5.h>

/**
 * Computes and returns the MD5 of a file located at the file-path passed to the function.
 * Uses the OpenSSL library.
 *
 * Note: You should call free() of the char* variable storing the return value of this function. 
 *
 * @param filename Name of the file to be hashed by MD5.
 *
 * @return a char pointer to the calculated MD5 value. Returns NULL if file does not exist / could not be opened.
 */
char * computeMD5(const char * filename)
{
	int i, fread_size;
	/* Used to store each "chunk" of the file we are calculating the md5 of. */
	char buf[CHUNK_SIZE];
	
	MD5_CTX md5_var;
	/* Used to store md5 of file */
	unsigned char md5_sum[MD5_DIGEST_LENGTH];
	
	/* Open the file we wish to calculate the md5 of. */
	FILE *input_file = fopen(filename, "rb");
	/* Check to see if file is open. */
	if (input_file != NULL)
	{
		MD5_Init(&md5_var);
		
		/* By reading in chunks instead of the whole file, we don't have to worry about large files taking up all of our memory. */
		while ((fread_size = fread(buf, 1, CHUNK_SIZE, input_file)) > 0)
		{
			/* Continuously digest the file. */ 
			MD5_Update(&md5_var, buf, fread_size);
		}
		MD5_Final(md5_sum, &md5_var);
		
		/* Close the file. */
		fclose(input_file);
		
		/* Store the file in a string. */
		char * md5String;
		md5String = (char*) malloc (33);
		for (i = 0; i < 16; i++)
		{
			sprintf(&md5String[i*2], "%02x", (unsigned int)md5_sum[i]);
		}
	}
	else
	{
		md5String = NULL;
	}
	
	return md5String;
}

#endif