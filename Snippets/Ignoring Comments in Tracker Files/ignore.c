/**
 * @file client.c
 * @authors Xiao Deng, Matthew Lindner
 *
 * @brief This file will eventually merge with client.c
 * @details 
 *
 * @section COMPILE
 * g++ ./ignore.c -o client.out -lnsl -pthread -lcrypto
 */

/*-----------------------------------
            Includes
-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

/*-----------------------------------
            Defines
-----------------------------------*/
#define DEBUG_MODE 1			///< 1 = ON, 0 = OFF, printout program debug info
#define TEST_MODE 1				///< 1 = ON, 0 = OFF, printout function testing info

#define CHUNK_SIZE 1024			///< Chunk size in Byte, MAX = 1024

#define IP_ADDR_SIZE 15			///< IP address buffer string size
#define FILENAME_SIZE 40		///< Filename buffer string size for tracked file
#define DESCRIPTION_SIZE 100	///< Description buffer string size for tracked file
#define MD5_SIZE 64				///< MD5 buffer string size for tracked file

/*-----------------------------------
        Types & Structures
-----------------------------------*/
/**
 * Storing information of a file chunk.
 */
struct chunks_struct
{
	char	ip_addr[ IP_ADDR_SIZE ];	///< IP address string buffer
	int		port_num;					///< Port number buffer
	long	start_byte;					///< Starting byte buffer
	long	end_byte;					///< Ending byte buffer
	long	time_stamp;					///< Time stamp buffer
};

/**
 * Storing information of a tracked/sharing file.
 */
struct tracked_file_info_struct
{
	char	filename[ FILENAME_SIZE ];			///< Filename string buffer
	long	filesize;							///< Filesize buffer
	char	description[ DESCRIPTION_SIZE ];	///< Description buffer
	char	md5[ MD5_SIZE ];					///< MD5 string buffer
};

/*-----------------------------------
        Enums & const string
-----------------------------------*/
/**
 * Some return value in plain English for less grey-matter damage.
 */
enum rtn_val
{
	INVALID_TRACKER_INFO = -4,			///< \b tracked_file_info object error
	INVALID_PENDING_CHUNK_TABLE = -3,	///< \b pending_chunks vector table error
	INVALID_CHUNK_TABLE = -2,			///< \b live_chunks vector table error
	INVALID_TRACKER_FILE = -1,			///< Error accessing tracker file
	NO_ERROR = 0,						///< Normal return value
	NO_NEXT_CHUNK = 1,					///< No matched next chunk found - findNextChunk()
	NOT_LIVE_CHUNK = 2					///< Test chunk is not live - isLiveChunk()
};

const char* test_ip_addr = "localhost";	///< IP address for testing mode

int test_port_num = 12345;				///< Port number for testing mode

/*-----------------------------------
            Variables
-----------------------------------*/

tracked_file_info_struct tracked_file_info;	///< Initialize tracked file object.

std::vector<chunks_struct> live_chunks;		///< Initialize chunk info vector table.

std::vector<chunks_struct> pending_chunks;	///< Initialize pending chunk info vector table.

/*-----------------------------------
            Prototypes
-----------------------------------*/

/**
 * Test this file.
 * 
 */
int testMain();


/**
 * Find the next chunk to down load from the tracker file.
 * Find the next chunk to download using a starting byte & ending byte combination in the tracker file, return the chunk with the largest(latest) time stamp if there are multiple available.
 * 
 * @param start The starting byte of next chunk to download.
 * @param end The ending byte of the next chunk to download.
 *
 * @return If the a chunk matches the starting & ending byte is found, return the index of that chunk in the chunk table, else return \b NO_NEXT_CHUNK.
 */
int findNextChunk( long start, long end );


/**
 * Parse the tracker file to obtain file & chunk information.
 * Parse the tracker file using tracker file's name to obtain file name, file size, and description. Also populates the live_chunks vector table if tracker file contains lines for file chunks.
 * 
 * @param tracker_file_name File name -> tracker file.
 * @param filename File name -> tracked file.
 * @param filesize File size in bytes -> tracked file.
 * @param description Description -> tracked file.
 * @param md5 MD5 -> tracked file.
 *
 * @return \b NO_ERROR, \b NO_NEXT_CHUNK, \b INVALID_CHUNK_TABLE
 */
int tracker_file_parser( char* tracker_file_name, char* filename, long filesize, char* description, char* md5 );


/**
 * Commit chunks saved in \b pending_chunks vector table.
 * Commit all chunks in the \b pending_chunks vector table, chunks will be appended to the end of an existing tracker file as well as \b live_chunks vector table.
 *  
 * @param tracker_file_name File name -> tracker file.
 *
 * @return \b NO_ERROR
 */
int commitPendingChunks( char* tracker_file_name );


/**
 * Append a chunk to pending_chunks vector table.
 *
 * @param test_chunk Test chunk struct to be appended.
 * 
 * @return \b NO_ERROR
 */
int appendChunk( struct chunks_struct test_chunk );


/**
 * Update tracker file with current content in \b live_chunks vector table.
 * Populate \b live_chunks & \b tracked_file_info vector table with &chunks & file information before calling this function.
 *
 * @return \b NO_ERROR
 */
int createTracker();


/**
 * Initialize \b live_chunks vector table for createTracker().
 * Commits all chunks in \b pending_chunks vector table to \b live_chunks vector table to get ready for createTracker() creating new tracker file. \b live_chunks vector table will be cleared and repopulated, \b pending_chunks vector table will be cleared after execution.
 * 
 */
void initialTrackerChunks();


/**
 * Clear \b pending_chunks vector table.
 * 
 */
void clearPendingChunks();


/**
 * Clear \b live_chunks vector table.
 * 
 */
void clearLiveChunks();


/**
 * Determine if a chunk is being shared
 * Determine if a chunk is being shared ( is it in \b live_chunks vector table ? )
 *
 * @return chunk index in live_chunks if found, \b NOT_LIVE_CHUNK if not.
 */
int isLiveChunk( struct chunks_struct test_chunk );


/*-----------------------------------
            Main for testing
-----------------------------------*/
int main()
{
	/**
	 * 1. Get IP & port either from user input or config
	 * 2. 	a. If user wants to share file, make sure file exist, then get file properties into tracked_file_info vector table.
	 *		b. Calculate sharing info for all sharing chunks and populate pending_chunks vector table.
	 *		c. 
	 *    	d. When sharing chunks change dramatically, call createTracker() and push out new tracker file to server periodically.
	 */
	testMain();
	return 0;
}

int testMain()
{
	//tracker file name for testing
	char test_tracker_filename[] = "name.track";
	
	//run tracker_file_parser()
	tracker_file_parser( 	test_tracker_filename,
							tracked_file_info.filename,
							tracked_file_info.filesize,
							tracked_file_info.description,
							tracked_file_info.md5
							);
	
	if( TEST_MODE == 1 )
	{
		char test_filename[ FILENAME_SIZE ] = "test_tracker.track";
		char test_description[ DESCRIPTION_SIZE ] = "This_description_is_sick";
		char test_md5[ MD5_SIZE ] = "FFA3B4F56AC712387CCBFE";
		long test_filesize=88899889;
		
		long test_start = 1234567;
		long test_end = 2345678;
		long test_time_stamp = 1999999999;
		
		printf( "\n\r[TEST] Testing commitPendingChunks() & findNextChunk() with start_byte = %ld, end_byte = %ld ...\n",
				test_start,
				test_end
				);
		
		//construct test_chunk struct, could use appendChunk() instead
		for( int n=0; n<2; n++ )
		{
			pending_chunks.push_back( chunks_struct() );
			strcpy( pending_chunks[n].ip_addr, test_ip_addr );
			pending_chunks[n].port_num = test_port_num;
			pending_chunks[n].start_byte = test_start;
			pending_chunks[n].end_byte = test_end;
			pending_chunks[n].time_stamp = test_time_stamp;
			test_port_num++;
			test_time_stamp++;
		}
		
		//test commitPendingChunks()
		commitPendingChunks( test_tracker_filename );
		
		int next_chunk_i = findNextChunk( test_start, test_end );
		if( next_chunk_i >= 0 )
		{
			printf( "[TEST] Chunk[%d] is the next chunk, ip = %s, port = %d, time_stamp = %ld\n",
				next_chunk_i,
				live_chunks[ next_chunk_i ].ip_addr,
				live_chunks[ next_chunk_i ].port_num,
				live_chunks[ next_chunk_i ].time_stamp
				);
		}
		else printf( "[TEST] No next chunk found\n" );
		
		printf( "[TEST] Testing createTracker() with new tracker filename = %s, filesize = %ld, description = %s, md5 = %s\n\r",
				test_filename,
				test_filesize,
				test_description,
				test_md5
				);
		
		//test createTracker()
		createTracker();
		
		//test isLiveChunk()
		printf( "[TEST] Testing isLiveChunk() with live_chunks[1] \n\r" );
		int test_rtn;
		test_rtn = isLiveChunk( live_chunks[1] );
		if( test_rtn != NOT_LIVE_CHUNK ) printf( "[TEST] test_chunk is live @ live_chunks[%d]!\n\r", test_rtn );
		else printf( "[TEST] test_chunk is offline!\n\r" );
		
		printf( "[TEST] Testing DONE!\n\n" );
	}
	
	return 0;
}

int tracker_file_parser( char* tracker_file_name, char* filename, long filesize, char* description, char* md5 )
{
	FILE *tracker_file;							///< File handle for tracker file
	char *line;									///< Line buffer
	char *pch;									///< \c strtok buffer
	char temp_line[ DESCRIPTION_SIZE ];			///< Temp line buffer to be strtok-ed
	char temp;									///< Temp char buffer 
	size_t len = 0;								///< Line length buffer
	
	/** Open tracker file using tracker_file_name, read-only mode */
	if ((tracker_file = fopen( tracker_file_name, "r")) != NULL)
	{
		//print out debug info
		if( DEBUG_MODE == 1 ) printf( "[INFO] Parsing tracker file \"%s\" ...\n", tracker_file_name );
		
		/** Get first line, if it's comment, get next line; if not, get filename  */
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		pch = strtok( line, " " );
		pch = strtok( NULL, "\n" );
		strcpy( filename, pch );
		
		/** Get next line, if it's comment, get next line; if not, get filesize  */
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		sscanf( line, "Filesize: %ld\n", &filesize );
		
		/** Get next line, if it's comment, get next line; if not, get description  */
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		pch = strtok( line, " " );
		pch = strtok( NULL, "\n" );
		strcpy( description, pch );
		
		/** Get next line, if it's comment, get next line; if not, get md5  */
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		pch = strtok( line, " " );
		pch = strtok( NULL, "\n" );
		strcpy( md5, pch );
		
		//print out debug info of for filename, filesize, description, and md5
		if( DEBUG_MODE == 1)
		{
			printf( "\n\r[DEBUG]Filename 	%s\n", filename );
			printf( "[DEBUG]Filesize 	%ld\n", filesize );
			printf( "[DEBUG]Description 	%s\n", description );
			printf( "[DEBUG]MD5		%s\n", md5 );
		}
		
		
		/** Reset live_chunks vector table, initilize chunk counter  */
		int n=0;
		clearLiveChunks();
		
		/** 
		 * Read in chunk info to populate chunk table until end of file, ignore comment lines.
		 */
		while ((temp = fgetc(tracker_file)) != EOF)
		{
			ungetc(temp, tracker_file);
			do
			{
				getline(&line, &len, tracker_file);
			} while (strncmp(line, "#", 1) == 0 && !EOF);
			
			/**
			 * For not-comment lines, make a copy of the line into \b temp_line.
			 * Parse out IP address, Port number, Start byte, End byte, and Time stamp.
			 * Append new chunk to \b live_chunks vector table and store parsed values.
			 * 
			 */
			if (strncmp(line, "#", 1) != 0)
			{
				//adding new chunk info to table
				live_chunks.push_back( chunks_struct() );
				strcpy( temp_line, line );
				
				pch = strtok( temp_line, ":" );
				strcpy( live_chunks[n].ip_addr, pch );
				
				pch = strtok( NULL, ":" );
				live_chunks[n].port_num = atoi( pch );
				
				pch = strtok( NULL, ":" );
				live_chunks[n].start_byte = atol( pch );
				
				pch = strtok( NULL, ":" );
				live_chunks[n].end_byte = atol( pch );
				
				pch = strtok( NULL, ":" );
				live_chunks[n].time_stamp = atol( pch );
				
				//print out result for debugging
				if( DEBUG_MODE == 1 )
				{
					printf( "\n\r[DEBUG]Chunk[%d]: 	%s", n, line );
					printf( "[DEBUG] ip_addr 	%s\n", live_chunks[n].ip_addr );
					printf( "[DEBUG] port 		%d\n", live_chunks[n].port_num );
					printf( "[DEBUG] start_byte 	%ld\n", live_chunks[n].start_byte );
					printf( "[DEBUG] end_byte 	%ld\n", live_chunks[n].end_byte );
					printf( "[DEBUG] time_stamp 	%ld\n", live_chunks[n].time_stamp );
				}
				
				/** Increment chunk counter  */
				n++;
			}
		}
		
		/** Print out info when parsing is done  */
		printf( "\n\r[INFO] END of tracker file.\n" );
		
		/** Free mem for line buffer  */
		free(line);
		
		/** Close tracker file handle  */
		fclose(tracker_file);
	}
	else ///< Print out debug info and return \b INVALID_TRACKER_FILE if tracker file can't be accessed
	{
		if( DEBUG_MODE == 1 ) printf( "[ERROR] Error opening tracker file!\n");
		return INVALID_TRACKER_FILE;
	}
	
	/** Return normal if no error  */
	return NO_ERROR;
}

int findNextChunk( long start, long end )
{
	/** Get live_chunks vector table size  */
	int num_of_live_chunks = live_chunks.size();
	
	/** Return \b INVALID_CHUNK_TABLE if table size is invalid  */
	if( num_of_live_chunks <= 0 ) return INVALID_CHUNK_TABLE;
	
	long latest_time = 0;			///< Buffer for time stamp
	int next_chunk_index = -1;		///< Buffer for next chunk index, default to -1
	
	std::vector<int> chunk_index;	///< Keep track of potential chunks in vector table
	
	/** Loop through all chunks in live_chunks vector table  */
	for( int i=0; i<num_of_live_chunks; i++ )
	{
		/** Append matching chunks to vector table, update time stamp if it's more current  */
		if( ( live_chunks[i].start_byte == start ) && ( live_chunks[i].end_byte == end ) )
		{
			chunk_index.push_back( i );
			latest_time = ( latest_time < live_chunks[i].time_stamp ) ? live_chunks[i].time_stamp : latest_time;
		}
	}
	
	/** Find the index for the most current matching chunk */
	if( latest_time != 0 )
	{
		for( int n=0; n<chunk_index.size(); n++ )
		{
			next_chunk_index = ( latest_time == live_chunks[ chunk_index[n] ].time_stamp ) ? chunk_index[n] : next_chunk_index;
			if( next_chunk_index >= 0 ) break;
		}
	}
	
	/** Return the chunk index if it's valid, else return \b NO_NEXT_CHUNK */
	if( next_chunk_index >= 0 )
	{
		return next_chunk_index;
	}
	else return NO_NEXT_CHUNK;
}


int appendChunk( struct chunks_struct test_chunk )
{
	int num_of_pending_chunks = pending_chunks.size();
	if( num_of_pending_chunks < 0 ) return INVALID_PENDING_CHUNK_TABLE;
	
	pending_chunks.push_back( chunks_struct() );
	strcpy( pending_chunks[ num_of_pending_chunks ].ip_addr, test_chunk.ip_addr );
	pending_chunks[ num_of_pending_chunks ].port_num = test_chunk.port_num;
	pending_chunks[ num_of_pending_chunks ].start_byte = test_chunk.start_byte;
	pending_chunks[ num_of_pending_chunks ].end_byte = test_chunk.end_byte;
	pending_chunks[ num_of_pending_chunks ].time_stamp = test_chunk.time_stamp;
	
	return NO_ERROR;
}


int commitPendingChunks( char* tracker_file_name )
{
	FILE* tracker_file_h;
	int num_of_live_chunks = live_chunks.size();
	if( num_of_live_chunks < 0 ) return INVALID_CHUNK_TABLE;
	
	int num_of_pending_chunks = pending_chunks.size();
	if( num_of_pending_chunks <= 0 ) return INVALID_PENDING_CHUNK_TABLE;
	
	tracker_file_h = fopen( tracker_file_name, "a+" );
	
	//add chunk info for all pending chunks to chunk table
	for( int n=0; n<num_of_pending_chunks; n++ )
	{
		live_chunks.push_back( chunks_struct() );
		strcpy( live_chunks[ num_of_live_chunks ].ip_addr, pending_chunks[n].ip_addr );
		live_chunks[ num_of_live_chunks ].port_num = pending_chunks[n].port_num;
		live_chunks[ num_of_live_chunks ].start_byte = pending_chunks[n].start_byte;
		live_chunks[ num_of_live_chunks ].end_byte = pending_chunks[n].end_byte;
		live_chunks[ num_of_live_chunks ].time_stamp = pending_chunks[n].time_stamp;
		
		//add new chunk to tracker file
		fprintf( tracker_file_h, "\r\n%s:%d:%ld:%ld:%ld",
				pending_chunks[n].ip_addr,
				pending_chunks[n].port_num,
				pending_chunks[n].start_byte,
				pending_chunks[n].end_byte,
				pending_chunks[n].time_stamp
				);
				
		num_of_live_chunks++;
	}
	clearPendingChunks();
	
	fclose( tracker_file_h );
	return NO_ERROR;
}


int createTracker()
{
	char tracker_file_name[ FILENAME_SIZE ];
	if( tracked_file_info.filename[0] == '\0' ) return INVALID_TRACKER_INFO;
	else
	{
		char tracker_file_ext[] = ".track";
		strcpy( tracker_file_name, tracked_file_info.filename );
		strcat( tracker_file_name, tracker_file_ext );
	}
	
	int num_of_live_chunks = live_chunks.size();
	if( num_of_live_chunks < 0 ) return INVALID_CHUNK_TABLE;
	
	FILE* tracker_file_h;
	tracker_file_h = fopen( tracker_file_name, "w" );
	
	//construct tracker file header info
	fprintf( tracker_file_h, "Filename: %s\nFilesize: %ld\nDescription: %s\nMD5: %s\n#test_comments",
				tracked_file_info.filename,
				tracked_file_info.filesize,
				tracked_file_info.description,
				tracked_file_info.md5
				);
	
	for( int n=0; n< num_of_live_chunks; n++ )
	{
		//add chunk info to tracker file
		fprintf( tracker_file_h, "\r\n%s:%d:%ld:%ld:%ld",
				live_chunks[n].ip_addr,
				live_chunks[n].port_num,
				live_chunks[n].start_byte,
				live_chunks[n].end_byte,
				live_chunks[n].time_stamp
				);
	}
	
	fclose( tracker_file_h );
	return NO_ERROR;
}


void initialTrackerChunks()
{
	int num_of_pending_chunks = pending_chunks.size();
	if( num_of_pending_chunks > 0 )
	{
		clearLiveChunks();
		//add chunk info for all pending chunks to chunk table
		for( int n=0; n<num_of_pending_chunks; n++ )
		{
			live_chunks.push_back( chunks_struct() );
			strcpy( live_chunks[ n ].ip_addr, pending_chunks[n].ip_addr );
			live_chunks[ n ].port_num = pending_chunks[n].port_num;
			live_chunks[ n ].start_byte = pending_chunks[n].start_byte;
			live_chunks[ n ].end_byte = pending_chunks[n].end_byte;
			live_chunks[ n ].time_stamp = pending_chunks[n].time_stamp;
		}
		clearPendingChunks();
	}
}


int isLiveChunk( struct chunks_struct test_chunk )
{
	int found=0;
	int num_of_live_chunks = live_chunks.size();
	int n=0;
	
	while( ( found == 0 ) && ( n<num_of_live_chunks ) )
	{
		if( ( strncmp( live_chunks[n].ip_addr, test_chunk.ip_addr, IP_ADDR_SIZE ) == 0 ) &&
			( live_chunks[n].port_num == test_chunk.port_num ) &&
			( live_chunks[n].start_byte == test_chunk.start_byte ) &&
			( live_chunks[n].end_byte == test_chunk.end_byte )
			)
		{
			found = 1;
		}
		n++;
	}
	
	if( found == 1 ) return n-1;
	else return NOT_LIVE_CHUNK;
}


void clearPendingChunks()
{
	pending_chunks.clear();
}


void clearLiveChunks()
{
	live_chunks.clear();
}
