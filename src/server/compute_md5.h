#ifndef COMPUTE_MD5_H
#define COMPUTE_MD5_H

#include <openssl/md5.h>

/*Adapted from http://stackoverflow.com/a/10324904 and http://www.askyb.com/cpp/openssl-md5-hashing-example-in-cpp/*/

/* must call free() of char * from calling function */
char * computeMD5(const char * filename)
{
	int i, fread_size;
	//Used to store each "chunk" of the file we are calculating the md5 of.
	char buf[CHUNK_SIZE];
	
	MD5_CTX md5_var;
	//Used to store md5 of file
	unsigned char md5_sum[MD5_DIGEST_LENGTH];
	
	//Open the file we wish to calculate the md5 of
	FILE *input_file = fopen(filename, "rb");
	//Check to see if file is open
	if (input_file != NULL)
	{
		MD5_Init(&md5_var);
		
		//By reading in chunks instead of the whole file, we don't have to worry about
			//large files taking up all of our memory.
		while ((fread_size = fread(buf, 1, CHUNK_SIZE, input_file)) > 0)
		{
			MD5_Update(&md5_var, buf, fread_size);
		}
		MD5_Final(md5_sum, &md5_var);
		
		fclose(input_file);
	}
	
	char * md5String;
	md5String = (char*) malloc (33);
	for (i = 0; i < 16; i++)
	{
		sprintf(&md5String[i*2], "%02x", (unsigned int)md5_sum[i]);
	}
	
	return md5String;
}

#endif