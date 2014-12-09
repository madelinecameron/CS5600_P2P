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

std::vector<segment_struct> file_segment;


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


/*
	dog.jpgexample filesize = 43262
	43262 = 1024*42 + 254, total of 43 chunks
	43 = 2*20 + 3, first 3 segment will have 3 chunks, rest 2 chunks:
		0-2, 3-5, 6-8, 9-10, 11-12, ...
	client - segment list:
		client[0]  - segment[0-3]
		client[1]  - segment[4-7]
		client[2]  - segment[8-11]
		client[3]  - segment[12-15]
		client[4]  - segment[16-19]
*/
void initSegments( long filesize )
{
	/** Calculate total number of chunks in this segment */
	long total_num_of_chunks = filesize/CHUNK_SIZE;
	total_num_of_chunks = ( filesize%CHUNK_SIZE > 0 ) ? total_num_of_chunks+1 : total_num_of_chunks;
	
	int chunk_per_seg = total_num_of_chunks/20;
	int chunk_remainder = total_num_of_chunks%20;
	int chunk_start = 0;
	
	file_segment.clear();
	
	for( int n=0; n<20; n++ )
	{
		file_segment.push_back( segment_struct() );
		file_segment[n].start_chunk = chunk_start;
		file_segment[n].end_chunk = ( chunk_remainder > 0 ) ? chunk_start + chunk_per_seg : chunk_start + chunk_per_seg - 1;
		
		/** update starting chunk and chunk remainder */
		chunk_start = file_segment[n].end_chunk + 1;
		chunk_remainder--;
		
		if( TEST_MODE == 1 ) printf( "[DEBUG] segment[%d] - chunk[%d ~ %d]\n",
									n,
									file_segment[n].start_chunk,
									file_segment[n].end_chunk
									);
	}
}


void appendSegment( char* tracker_filename, long filesize, int segment_index )
{
	int start_chunk = file_segment[ segment_index ].start_chunk;
	int end_chunk = file_segment[ segment_index ].end_chunk;
	
	int num_of_chunks = end_chunk - start_chunk + 1;
	
	/** new chunks struct as template */
	chunks_struct chunk;
	long current_time = ( unsigned ) time( NULL );
	chunk.time_stamp = current_time;
	strcpy( chunk.ip_addr, "localhost" );
	chunk.port_num = test_port_num;
	
	if( TEST_MODE == 1 )
	{
		printf( "\r\n[DEBUG] Appending %d chunks in segment[%d], start_byte = %d\n",
			 	num_of_chunks,
			 	segment_index,
			  	start_chunk*CHUNK_SIZE
			   	);
	}
	
	long start_byte = start_chunk*CHUNK_SIZE;
	long end_byte = start_byte + CHUNK_SIZE - 1;
	
	for( int n=0; n<num_of_chunks; n++ )
	{
		if( start_byte > filesize ) break;
		chunk.start_byte = start_byte;
		chunk.end_byte = end_byte;
		
		appendChunk( chunk );
		
		if( TEST_MODE == 1 )
		{
			printf( "\r[DEBUG] Chunk[ %ld - %ld ] appended\n", start_byte, end_byte );
		}
		start_byte = end_byte+1;
		end_byte += CHUNK_SIZE;
		end_byte = ( end_byte > filesize ) ? filesize :end_byte;
	}
	int rtn = commitPendingChunks( tracker_filename );
	printf( "[DEBUG] commitPendingChunk() returned %d\n", rtn );
}


void myFilePath( int client_index, char* myfile )
{
	sprintf( myfile, "./test_clients/client_%d/picture-wallpaper.jpg", client_index );
}

void fileSperator( char* filename )
{
	FILE *file_h;
	char buf[128];
	long total_read=0, total_wrote=0;
	
	if( ( file_h = fopen( filename, "rb" ) ) == NULL ) printf("[ERROR] Error open file\n");
	
	fseek( file_h, 0, SEEK_END );
	int filesize = ftell( file_h );
	fseek( file_h, 0, SEEK_SET );
	
	for( int i=1; i<=5; i++ )
	{
		char temp[4];
		char part_file_name[64];
		FILE* part_file_h;
		int read, wrote;
		long total_r=0;
		
		strcpy( part_file_name, filename );
		sprintf( temp, ".%d", i );
		strcat( part_file_name, temp );
		
		if( ( part_file_h = fopen( part_file_name, "wb" ) ) == NULL )  printf("[ERROR] Error creating partfile\n");
		printf( "\r\n[DEBUG] New file: %s ... \n", part_file_name );
		
		while( total_r < ( filesize/5 + 128 ) )
		{
			read = fread( buf, 1, 128, file_h );
			wrote = fwrite( buf, 1, read, part_file_h );
			printf( "\r[DEBUG] total_read = %ld, total wrote = %ld", total_read, total_wrote );
			total_read += read;
			total_r += read;
			total_wrote += wrote;
			
			if( total_read == filesize) break;
		}
		fclose( part_file_h );
	}
	
	fclose( file_h );
	
}

void fileBabyMaking( char* filename )
{
	FILE *file_h;
	char buf[128];
	long total_read=0, total_wrote=0;
	char new_filename[64];
	
	strcpy( new_filename, filename );
	if( ( file_h = fopen( strcat( new_filename, ".new" ), "wb" ) ) == NULL ) printf("[ERROR] Error open file\n");
	
	for( int i=1; i<=5; i++ )
	{
		char temp[4];
		char part_file_name[64];
		FILE* part_file_h;
		int read, wrote;
		long total_w=0;
		
		strcpy( part_file_name, filename );
		sprintf( temp, ".%d", i );
		strcat( part_file_name, temp );
		
		if( ( part_file_h = fopen( part_file_name, "rb" ) ) == NULL )  printf("[ERROR] Error opening partfile\n");
		printf( "\r\n[DEBUG] Processing file: %s ... \n", part_file_name );
		
		fseek( part_file_h, 0, SEEK_END );
		int filesize = ftell( part_file_h );
		fseek( part_file_h, 0, SEEK_SET );
		
		while(1)
		{
			read = fread( buf, 1, 128, part_file_h );
			wrote = fwrite( buf, 1, read, file_h );
			printf( "\r[DEBUG] total_read = %ld, total wrote = %ld", total_read, total_wrote );
			total_read += read;
			total_w += read;
			total_wrote += wrote;
			
			if( total_w == filesize) break;
		}
		fclose( part_file_h );
	}
	
	fclose( file_h );
}

