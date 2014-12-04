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
	
	//test file handle
	FILE* test_file_h;

	//run tracker_file_parser()
	tracker_file_parser( 	test_tracker_filename,
							tracked_file_info.filename,
							tracked_file_info.filesize,
							tracked_file_info.description,
							tracked_file_info.md5
							);

	if( TEST_MODE == 1 )
	{	
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
	
		//test isLiveChunk()
		printf( "[TEST] Testing isLiveChunk() \n\r" );
		int test_rtn;
		test_rtn = isLiveChunk( live_chunks[1] );
		if( test_rtn != NOT_LIVE_CHUNK ) printf( "[TEST] test_chunk is live @ live_chunks[%d]!\n\r", test_rtn );
		else printf( "[TEST] test_chunk is offline!\n\r" );
		
		printf( "[TEST] Testing createNewTracker() \n\r" );
		char dog_file[] = "dog.jpg";
		char dog_track[] = "dog.jpg.track";
		long start_pos = 0;
		
		test_file_h = fopen( dog_file, "r" );
		fseek( test_file_h, 0, SEEK_END ); 				// seek to end of file
		long dog_size = ftell( test_file_h ); 			// get current file pointer
		fseek( test_file_h, 0, SEEK_SET ); 				// seek back to beginning of file
		fclose( test_file_h );
		
		printf( "[TEST] Testing file: \"%s\" with filesize = %ld\n", dog_file, dog_size );
		
		for(int n=0; n<15; n++ )
		{
			long l_test_rtn = appendToTracker( dog_track, dog_size, start_pos );
			printf( "[TEST] Ending byte of this segment: %ld\n", l_test_rtn );
			start_pos = l_test_rtn+1;
		}
		printf( "[TEST] Testing DONE!\n\n" );
	}

	return 0;
}