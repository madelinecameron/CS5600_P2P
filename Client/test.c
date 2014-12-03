/**
 * @file test.c
 * @authors Xiao Deng, Matthew Lindner
 *
 *
 * @section COMPILE
 * See Makefile
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client_support.h"


/*-----------------------------------
            Main for testing
-----------------------------------*/
int main()
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
		long test_filesize = 88899889;
	
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