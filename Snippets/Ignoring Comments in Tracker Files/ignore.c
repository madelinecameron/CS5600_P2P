//g++ ./ignore.c -o ignore

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
#define DEBUG_MODE 1

/*-----------------------------------
        Types & Structures
-----------------------------------*/
struct chunk_info_struct
{
	char ip_addr[15];
	int port_num;
	long start_byte;
	long end_byte;
	long time_stamp;
};

/*-----------------------------------
        Enums & const string
-----------------------------------*/
enum rtn_val
{
	NO_NEXT_CHUNK = -1
};

/*-----------------------------------
            Variables
-----------------------------------*/
chunk_info_struct *test_struct;

//initialize chunk info vector
std::vector<chunk_info_struct> chunk_info;

/*-----------------------------------
            Functions
-----------------------------------*/
//Function prototype
int findNextChunk( long start, long end );


//int tracker_file_parser( char* tracker_file_name, char* filename, long filesize, char* description )
int main()
{
	const char tracker_file_name[] = "name.track";
	
	FILE *tracker_file;
	char *line;
	char temp_line[100];
	size_t len = 0;
	char temp;
	
	char description[100], *pch, md5[64], filename[40];
	long filesize=0;
	
	if ((tracker_file = fopen( tracker_file_name, "r")) != NULL)
	{
		if( DEBUG_MODE == 1 ) printf("[INFO] Parsing tracker file \"%s\" ...\n", tracker_file_name );
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		pch = strtok( line, " " );
		pch = strtok( NULL, "\n" );
		strcpy( filename, pch );
		
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		sscanf( line, "Filesize: %ld\n", &filesize );
		
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		pch = strtok( line, " " );
		pch = strtok( NULL, "\n" );
		strcpy( description, pch );
		
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		pch = strtok( line, " " );
		pch = strtok( NULL, "\n" );
		strcpy( md5, pch );
		
		if( DEBUG_MODE == 1)
		{
			printf( "\n\r[DEBUG]Filename 	%s\n", filename );
			printf( "[DEBUG]Filesize 	%ld\n", filesize );
			printf( "[DEBUG]Description 	%s\n", description );
			printf( "[DEBUG]MD5		%s\n", md5 );
		}
		
		int n=0;
		while ((temp = fgetc(tracker_file)) != EOF)
		{
			ungetc(temp, tracker_file);
			do
			{
				getline(&line, &len, tracker_file);
			} while (strncmp(line, "#", 1) == 0 && !EOF);
			
			if (strncmp(line, "#", 1) != 0)
			{
				//adding new chunk info to table
				chunk_info.push_back( chunk_info_struct() );
				strcpy( temp_line, line );
				
				pch = strtok( temp_line, ":" );
				strcpy( chunk_info[n].ip_addr, pch );
				
				pch = strtok( NULL, ":" );
				chunk_info[n].port_num = atoi( pch );
				
				pch = strtok( NULL, ":" );
				chunk_info[n].start_byte = atol( pch );
				
				pch = strtok( NULL, ":" );
				chunk_info[n].end_byte = atol( pch );
				
				pch = strtok( NULL, ":" );
				chunk_info[n].time_stamp = atol( pch );
				
				if( DEBUG_MODE == 1 )
				{
					printf( "\n\r[DEBUG]Chunk[%d]: 	%s", n, line );
					printf("[DEBUG] ip_addr 	%s\n", chunk_info[n].ip_addr );
					printf("[DEBUG] port 		%d\n", chunk_info[n].port_num );
					printf("[DEBUG] start_byte 	%ld\n", chunk_info[n].start_byte );
					printf("[DEBUG] end_byte 	%ld\n", chunk_info[n].end_byte );
					printf("[DEBUG] time_stamp 	%ld\n", chunk_info[n].time_stamp );
				}
				n++;
			}
		}
		
		if( DEBUG_MODE == 1 )
		{
			long test_start = 2000987;
			long test_end = 2333276;
			
			printf("\n\r[TEST] Testing findNextChunk() with start_byte = %ld, end_byte = %ld ...\n",
					test_start,
					test_end
					);
			int next_chunk_i = findNextChunk( test_start, test_end );
			if( next_chunk_i >= 0 )
			{
				printf("[TEST] Chunk[%d] is the next chunk, ip = %s, port = %d, time_stamp = %ld\n",
					next_chunk_i,
					chunk_info[ next_chunk_i ].ip_addr,
					chunk_info[ next_chunk_i ].port_num,
					chunk_info[ next_chunk_i ].time_stamp
					);
			}
			else printf("[TEST] No next chunk found\n" );
		}
		
		printf( "\n\r[INFO] END of tracker file.\n" );
		free(line);
		fclose(tracker_file);
	}
	else if( DEBUG_MODE == 1 ) printf("[ERROR] Error opening tracker file!\n");

	return 0;
}

int findNextChunk( long start, long end )
{
	int num_of_chunks = chunk_info.size();
	long latest_time = 0;
	int next_chunk_index = -1;
	
	//keep track of potential chunks
	std::vector<int> chunk_index;
	
	for( int i=0; i<num_of_chunks; i++ )
	{
		if( ( chunk_info[i].start_byte == start ) && ( chunk_info[i].end_byte == end ) )
		{
			chunk_index.push_back( i );
			//if this chunk is more recent, save its time stamp
			latest_time = ( latest_time < chunk_info[i].time_stamp ) ? chunk_info[i].time_stamp : latest_time;
		}
	}
	
	if( latest_time != 0 )
	{
		for( int n=0; n<chunk_index.size(); n++ )
		{
			next_chunk_index = ( latest_time == chunk_info[ chunk_index[n] ].time_stamp ) ? chunk_index[n] : next_chunk_index;
			if( next_chunk_index >= 0 ) break;
		}
	}
	
	if( next_chunk_index >= 0 )
	{
		return next_chunk_index;
	}
	else return NO_NEXT_CHUNK;
}
