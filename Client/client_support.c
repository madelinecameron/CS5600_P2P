/**
 * @file client_support.c
 * @authors Xiao Deng, Matthew Lindner
 *
 * @brief This file will eventually merge with client.c
 * @details 
 *
 * @section COMPILE
 * g++ ./client_support.c -o client_support.out -lnsl -pthread -lcrypto
 */

/*-----------------------------------
            Includes
-----------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>

#include "client_support.h"


/*-----------------------------------
            Variables
-----------------------------------*/
const char* test_ip_addr = "localhost";

int test_port_num = 12345;

tracked_file_info_struct tracked_file_info;

std::vector<chunks_struct> live_chunks;

std::vector<chunks_struct> pending_chunks;


/*-----------------------------------
            Functions
-----------------------------------*/

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
		printf( "\n\r[INFO] Parsing tracker file \"%s\" ...\n", tracker_file_name );
		
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
		printf( "\n\r[INFO] Parsing DONE!\n" );
		
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
		for( int n=0; n<(int)chunk_index.size(); n++ )
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

void initLiveChunks()
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


long appendToTracker( char* tracker_filename, long filesize, long start_byte )
{
	/** Calculate segment size of 5% of the file */
	long segment_size = filesize/20;
	
	segment_size = ( ( filesize - start_byte ) < segment_size ) ? ( filesize - start_byte ) : segment_size;
	
	/** Calculate number of chunks in the segment, rounded down */
	long num_of_chunks = segment_size/CHUNK_SIZE;
	
	num_of_chunks = ( segment_size%CHUNK_SIZE > 0 ) ? num_of_chunks+1 : num_of_chunks;
	
	/** Initilize start & end byte counter for chunk */
	long start = start_byte, end = start_byte + CHUNK_SIZE-1, end_seg;
	
	end = ( end > filesize ) ? filesize : end;
	
	chunks_struct chunk;
	long current_time = ( unsigned ) time( NULL );
	chunk.time_stamp = current_time;
	strcpy( chunk.ip_addr, "localhost" );
	chunk.port_num = test_port_num;
	
	if( TEST_MODE == 1 )
	{
		printf( "\r[DEBUG] %ld chunks in this segment\n", num_of_chunks );
	}
	
	for( long n=0; n<num_of_chunks; n++ )
	{
		chunk.start_byte = start;
		chunk.end_byte = end;
		
		appendChunk( chunk );
		end_seg = end;
		
		if( TEST_MODE == 1 )
		{
			printf( "\r[DEBUG] Chunk[ %ld - %ld ] appended\n", start, end );
		}
		start = end+1;
		end += CHUNK_SIZE;
		end = ( end > filesize ) ? filesize : end;
	}
	int rtn = commitPendingChunks( tracker_filename );
	printf( "[DEBUG] commitPendingChunk() returned %d\n", rtn );
	
	return end_seg;
}

