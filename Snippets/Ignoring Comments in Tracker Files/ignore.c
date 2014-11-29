#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
	FILE *tracker_file;
	char *line;
	size_t len = 0;
	char temp;
	
	char *filename, *filesize, *description, *md5, *chunk;
	
	if ((tracker_file = fopen("name.track", "r")) != NULL)
	{	
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		filename = line;
		strcpy(filename, filename);
		printf("%s", filename);
		
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		filesize = line;
		strcpy(filesize, filesize);
		printf("%s", filesize);
		
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		description = line;
		strcpy(description, description);
		printf("%s", description);
		
		do
		{
			getline(&line, &len, tracker_file);
		} while (strncmp(line, "#", 1) == 0);
		md5 = line;
		strcpy(md5, md5);
		printf("%s", md5);
		
		while ((temp = fgetc(tracker_file)) != EOF)
		{
			ungetc(temp, tracker_file);
			do
			{
				getline(&line, &len, tracker_file);
			} while (strncmp(line, "#", 1) == 0 && !EOF);
			
			if (strncmp(line, "#", 1) != 0)
			{
				chunk = line;
				strcpy(chunk, chunk);
				printf("%s", chunk);
			}
	
		}

		
		free(line);
		fclose(tracker_file); 
	}

	return 0;

}