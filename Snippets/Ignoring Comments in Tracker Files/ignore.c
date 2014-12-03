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
 * Store information of a file chunk.
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
 * Store information of a tracked/sharing file.
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
 * Some return value in plain English to lessen Grey-matter damage.
 */
enum rtn_val
{
	INVALID_TRACKER_INFO = -4,			///< \b tracked_file_info object error
	INVALID_PENDING_CHUNK_TABLE = -3,	///< \b pending_chunks vector error
	INVALID_CHUNK_TABLE = -2,			///< \b live_chunks vector error
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
/**
 * Tracked file info structure.
 * -This structure is responsible of holding information for the tracked file,
 * it is used in createTracker().
 * -Initialize it when you want to create a new tracker file.
 * -tracker_file_parser() may populate this structure and use it as buffer.
 * 
 */
tracked_file_info_struct tracked_file_info;


/**
 * Live chunks vector.
 * This vector is a synchronous copy of all the chunks in a particular tracker file.
 * It allows fast access to all chunks the client is currently sharing.
 * 
 */
std::vector<chunks_struct> live_chunks;

/**
 * Pending chunks vector.
 * This vector stores all chunks waiting to be appended to live_chunks vector.
 * Append new chunks to share in this vector and then commit it to make them available for sharing. 
 * 
 */
std::vector<chunks_struct> pending_chunks;

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
 * Find the next chunk to download using a start byte & end byte combination in the tracker file, 
 * return the chunk with the largest(latest) time stamp if there are multiple available.
 * 
 * @param start The starting byte of next chunk to download.
 * @param end The ending byte of the next chunk to download.
 *
 * @return If a chunk matching the start & end byte is found, return the index of that chunk in the live_chunks vector, else return \b NO_NEXT_CHUNK.
 */
int findNextChunk( long start, long end );


/**
 * Parse the tracker file to obtain file & chunk information.
 * Parse the tracker file using tracker file's name to obtain file name, file size, and description.
 * Also populates the live_chunks vector if tracker file contains lines for file chunks.
 * 
 * @param tracker_file_name File name -> tracker file; Input buffer
 * @param filename File name -> tracked file; Output buffer buffer
 * @param filesize File size in bytes -> tracked file; Output buffer
 * @param description Description -> tracked file; Output buffer
 * @param md5 MD5 -> tracked file; Output buffer
 *
 * @return \b NO_ERROR, \b NO_NEXT_CHUNK, \b INVALID_CHUNK_TABLE
 */
int tracker_file_parser( char* tracker_file_name, char* filename, long filesize, char* description, char* md5 );


/**
 * Commit chunks saved in \b pending_chunks vector.
 * Commit all chunks in the \b pending_chunks vector, chunks will be appended to the end of 
 * an existing tracker file as well as \b live_chunks vector.
 *  
 * @param tracker_file_name File name -> tracker file to be appended to.
 *
 * @return \b NO_ERROR
 */
int commitPendingChunks( char* tracker_file_name );


/**
 * Append a chunk to pending_chunks vector.
 *
 * @param test_chunk Test chunk structure to be appended.
 * 
 * @return \b NO_ERROR
 */
int appendChunk( struct chunks_struct test_chunk );


/**
 * Update tracker file with current content in \b live_chunks vector.
 * Populate \b live_chunks & \b tracked_file_info vector with chunks & file information before calling this function.
 *
 * @return \b NO_ERROR
 */
int createTracker();


/**
 * Initialize \b live_chunks vector for createTracker().
 * Commits all chunks in \b pending_chunks vector to \b live_chunks vector to get ready for
 * createTracker() creating new tracker file.
 * \b live_chunks vector will be cleared and repopulated, \b pending_chunks vector will be cleared.
 * 
 */
void initialTrackerChunks();


/**
 * Clear \b pending_chunks vector.
 * 
 */
void clearPendingChunks();


/**
 * Clear \b live_chunks vector.
 * 
 */
void clearLiveChunks();


/**
 * Determine if a chunk is being shared
 * Determine if a chunk is being shared ( is it in \b live_chunks vector ? )
 *
 * @return Index of the chunk in live_chunks vector if found, \b NOT_LIVE_CHUNK if not.
 */
int isLiveChunk( struct chunks_struct test_chunk );


/*-----------------------------------
            Main for testing
-----------------------------------*/
int main()
{
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
		
		printf( "[TEST] Testing createTracker() with 	filename = %s, filesize = %ld, \n					description = %s, md5 = %s\n\r",
				test_filename,
				test_filesize,
				test_description,
				test_md5
				);
		
		//test createTracker()
		createTracker();
		
		//test isLiveChunk()
		printf( "[TEST] Testing isLiveChunk() \n\r" );
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
		
		
		/** Reset live_chunks vector, initialize chunk counter  */
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
			 * Append new chunk to \b live_chunks vector and store parsed values.
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
	/** Get live_chunks vector size  */
	int num_of_live_chunks = live_chunks.size();
	
	/** Return \b INVALID_CHUNK_TABLE if table size is invalid  */
	if( num_of_live_chunks <= 0 ) return INVALID_CHUNK_TABLE;
	
	long latest_time = 0;			///< Buffer for time stamp
	int next_chunk_index = -1;		///< Buffer for next chunk index, default to -1
	
	std::vector<int> chunk_index;	///< Keep track of potential chunks in vector
	
	/** Loop through all chunks in live_chunks vector  */
	for( int i=0; i<num_of_live_chunks; i++ )
	{
		/** Append matching chunks to vector, update time stamp if it's more current  */
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
	/** Get pending_chunks vector size */
	int num_of_pending_chunks = pending_chunks.size();
	/** If size is invalid, return \b INVALID_PENDING_CHUNK_TABLE */
	if( num_of_pending_chunks < 0 ) return INVALID_PENDING_CHUNK_TABLE;
	
	
	/** Append new chunk to pending_chunks vector */
	pending_chunks.push_back( chunks_struct() );
	
	/** Fill in all chunk information */
	strcpy( pending_chunks[ num_of_pending_chunks ].ip_addr, test_chunk.ip_addr );
	pending_chunks[ num_of_pending_chunks ].port_num = test_chunk.port_num;
	pending_chunks[ num_of_pending_chunks ].start_byte = test_chunk.start_byte;
	pending_chunks[ num_of_pending_chunks ].end_byte = test_chunk.end_byte;
	pending_chunks[ num_of_pending_chunks ].time_stamp = test_chunk.time_stamp;
	
	/** Return normal */
	return NO_ERROR;
}


int commitPendingChunks( char* tracker_file_name )
{
	FILE* tracker_file_h;		///< File handle for tracker file
	
	/** Get live_chunks vector size */
	int num_of_live_chunks = live_chunks.size();
	/** If size is invalid, return INVALID_PENDING_CHUNK_TABLE */
	if( num_of_live_chunks < 0 ) return INVALID_CHUNK_TABLE;
	
	/** Get pending_chunks vector size */
	int num_of_pending_chunks = pending_chunks.size();
	/** If size is invalid, return INVALID_PENDING_CHUNK_TABLE */
	if( num_of_pending_chunks <= 0 ) return INVALID_PENDING_CHUNK_TABLE;
	
	/** Open tracker file */
	tracker_file_h = fopen( tracker_file_name, "a+" );
	
	/** Add chunk info for all pending chunks to live_chunks vector */
	for( int n=0; n<num_of_pending_chunks; n++ )
	{
		live_chunks.push_back( chunks_struct() );
		strcpy( live_chunks[ num_of_live_chunks ].ip_addr, pending_chunks[n].ip_addr );
		live_chunks[ num_of_live_chunks ].port_num = pending_chunks[n].port_num;
		live_chunks[ num_of_live_chunks ].start_byte = pending_chunks[n].start_byte;
		live_chunks[ num_of_live_chunks ].end_byte = pending_chunks[n].end_byte;
		live_chunks[ num_of_live_chunks ].time_stamp = pending_chunks[n].time_stamp;
		
		/** Add new chunk to tracker file */
		fprintf( tracker_file_h, "\r\n%s:%d:%ld:%ld:%ld",
				pending_chunks[n].ip_addr,
				pending_chunks[n].port_num,
				pending_chunks[n].start_byte,
				pending_chunks[n].end_byte,
				pending_chunks[n].time_stamp
				);
				
		/** Increment live_chunks counter */
		num_of_live_chunks++;
	}
	/** CLear pending_chunks vector */
	clearPendingChunks();
	
	/** CLose tracker file handle */
	fclose( tracker_file_h );
	/** return normal */
	return NO_ERROR;
}


int createTracker()
{
	char tracker_file_name[ FILENAME_SIZE ];		///< Tracker file name buffer
	
	/** If tracked file info struct is un-initialized, return \b INVALID_TRACKER_INFO */
	if( tracked_file_info.filename[0] == '\0' ) return INVALID_TRACKER_INFO;
	/** Else generate tracker file name using tracked file name */
	else
	{
		char tracker_file_ext[] = ".track";
		strcpy( tracker_file_name, tracked_file_info.filename );
		strcat( tracker_file_name, tracker_file_ext );
	}
	
	/** Get live_chunks vector size */
	int num_of_live_chunks = live_chunks.size();
	/** If size is invalid, return INVALID_PENDING_CHUNK_TABLE */
	if( num_of_live_chunks < 0 ) return INVALID_CHUNK_TABLE;
	
	FILE* tracker_file_h;		///< File handle for tracker file
	/** Create tracker file */
	tracker_file_h = fopen( tracker_file_name, "w" );
	
	/** Write tracker file header info */
	fprintf( tracker_file_h, "Filename: %s\nFilesize: %ld\nDescription: %s\nMD5: %s\n#test_comments",
				tracked_file_info.filename,
				tracked_file_info.filesize,
				tracked_file_info.description,
				tracked_file_info.md5
				);
	
	/** Write chunks info for all chunks in live_chunks vector */
	for( int n=0; n< num_of_live_chunks; n++ )
	{
		fprintf( tracker_file_h, "\r\n%s:%d:%ld:%ld:%ld",
				live_chunks[n].ip_addr,
				live_chunks[n].port_num,
				live_chunks[n].start_byte,
				live_chunks[n].end_byte,
				live_chunks[n].time_stamp
				);
	}
	
	/** Close tracker file handle */
	fclose( tracker_file_h );
	/** Return normal */
	return NO_ERROR;
}


void initialTrackerChunks()
{
	/** Get pending_chunks vector size */
	int num_of_pending_chunks = pending_chunks.size();
	/** If pending_chunks vector is not empty */
	if( num_of_pending_chunks > 0 )
	{
		/** Clear the live_chunks vector */
		clearLiveChunks();
		/** Add chunk info for all pending chunks to chunk table */
		for( int n=0; n<num_of_pending_chunks; n++ )
		{
			live_chunks.push_back( chunks_struct() );
			strcpy( live_chunks[ n ].ip_addr, pending_chunks[n].ip_addr );
			live_chunks[ n ].port_num = pending_chunks[n].port_num;
			live_chunks[ n ].start_byte = pending_chunks[n].start_byte;
			live_chunks[ n ].end_byte = pending_chunks[n].end_byte;
			live_chunks[ n ].time_stamp = pending_chunks[n].time_stamp;
		}
		/** CLear pending_chunks vector */
		clearPendingChunks();
	}
}


int isLiveChunk( struct chunks_struct test_chunk )
{
	int found=0;		///< Found flag
	int n=0;			///< Iterator for live_chunks vector
	/** Get live_chunks vector size */
	int num_of_live_chunks = live_chunks.size();
	
	/** Loop through live_chunks vector to find a matching chunk with test_chunk */
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
	
	/** If a matching chunk is found, return its index in live_chunks vector */
	if( found == 1 ) return n-1;
	/** Else return \b NOT_LIVE_CHUNK */
	else return NOT_LIVE_CHUNK;
}


void clearPendingChunks()
{
	/** Invoke vector clear for pending_chunks */
	pending_chunks.clear();
}


void clearLiveChunks()
{
	/** Invoke vector clear for live_chunks */
	live_chunks.clear();
}
