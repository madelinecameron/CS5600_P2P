/**
 * @file client_support.h
 * @authors Xiao Deng, Matthew Lindner
 *
 * @brief Header file for client_support.c
 * @details 
 *
 */
 
#ifndef __CLIENT_SUPPORT_H__
#define __CLIENT_SUPPORT_H__

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

/**
 * Store information of a file segment for seeding.
 */
struct segment_struct
{
	int start_chunk;
	int end_chunk;
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

/*-----------------------------------
            Variables
-----------------------------------*/
extern const char* test_ip_addr;				///< IP address for testing mode

extern int test_port_num;						///< Port number for testing mode

/**
 * Tracked file info structure.
 * -This structure is responsible of holding information for the tracked file,
 * it is used in createTracker().
 * -Initialize it when you want to create a new tracker file.
 * -tracker_file_parser() may populate this structure and use it as buffer.
 */
extern tracked_file_info_struct tracked_file_info;

/**
 * Live chunks vector.
 * This vector is a synchronous copy of all the chunks in a particular tracker file.
 * It allows fast access to all chunks the client is currently sharing.
 */
extern std::vector<chunks_struct> live_chunks;

/**
 * Pending chunks vector.
 * This vector stores all chunks waiting to be appended to live_chunks vector.
 * Append new chunks to share in this vector and then commit it to make them available for sharing. 
 */
extern std::vector<chunks_struct> pending_chunks;


/**
 * Segment vector.
 * 
 * This vector stores segment information for appending the 5% segment
 * periodicaly.
 * In download mode, each thread is downloading chunks in its 
 * corresponding segments. In seed mode, this vector allows segmentational
 * append of file chunks.
 */
extern std::vector<segment_struct> file_segment;


/*-----------------------------------
            Prototypes
-----------------------------------*/ 
/**
 * Find the next chunk to download from the tracker file.
 * Find the next chunk to download using a start byte & end byte combination in the tracker file, 
 * return the chunk with the largest(latest) time stamp if there are multiple available.
 * 
 * @param start The starting byte of next chunk to download, INPUT.
 * @param end The ending byte of the next chunk to download, INPUT.
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
 * @param filename File name -> tracked file; Output buffer
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
 * @param tracker_file_name File name -> tracker file to be appended to, INPUT.
 *
 * @return \b NO_ERROR
 */
int commitPendingChunks( char* tracker_file_name );


/**
 * Append a chunk to pending_chunks vector.
 *
 * @param test_chunk Test chunk structure to be appended, INPUT.
 * 
 * @return \b NO_ERROR
 */
int appendChunk( struct chunks_struct test_chunk );

/**
 * Clear \b pending_chunks vector.
 */
void clearPendingChunks();


/**
 * Clear \b live_chunks vector.
 */
void clearLiveChunks();


/**
 * Determine if a chunk is being shared.
 * Determine if a chunk is being shared ( is it in \b live_chunks vector ? )
 *
 * @return Index of the chunk in live_chunks vector if found, \b NOT_LIVE_CHUNK if not.
 */
int isLiveChunk( struct chunks_struct test_chunk );


/**
 * Init segment vector table.
 * Init segment vector table, populate all 5 segments with start and
 * end chunk index for each.
 * 
 * @param filesize Filesize of the shared file, INPUT.
 */
void initSegments( long filesize );


/**
 * Append a segment of chunks to tracker file.
 * Append a segment of chunks to tracker file and live_chunks vector.
 * 
 * @param tracker_filename File name of the tracker file to store the chunks, INPUT.
 * @param filesize File size of the shared file, INPUT.
 * @param segment_index Index for segment to append, INPUT.
 * @param port_num Port number of the seeding client for chunk construction, INPUT.
 */
void appendSegment( char* tracker_filename, long filesize, int segment_index, int port_num );


/**
 * Generate filename with path.
 * Generate filename with corresponding pathing with respect to the client index per demo requirement.
 * 
 * @param client_index Client index for constructing corresponding file path, INPUT
 * @param myfile Filename Buffer to hold the constructed filename with corresponding path string, OUTPUT
 */
void myFilePath( int client_index, char* myfile );

/**
 * Seperate of a test file into parts.
 * This function seperate a test file into the following part file format
 * 		<b><i> 'filename.ext.#' (# = 1~5) </i></b>
 * 
 * @param filename File name for the test file, INPUT.
 */
void fileSperator( char* filename );


/**
 * Concatenation of downloaded part files.
 * This function concatenate all 5 part files with the followig format
 * 		<b><i> 'filename.ext.#' (# = 1~5) </i></b>
 * into <b><i> 'filename.ext' </i></b>, the origiinal shared file.
 * 
 * @param filename File name for the shared file, INPUT.
 */
void fileCat( char* filename );

#endif
