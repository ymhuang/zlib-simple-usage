#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include "simpzip.h"

int _tmain(int argc, _TCHAR * argv[])
{
	char * folder = "F:\\tmp";
	filelist_t * filelist = NULL;
	filelist_t * curfilenode = NULL;
	filelist_t * tmpfilenode = NULL;

	// list all folders/files inside folder
	filelist = (filelist_t *) malloc( sizeof(filelist_t) );
	memset( filelist, 0, sizeof(filelist_t) );
	if ( true == ListFileInDir( folder, filelist ) )
	{
		curfilenode = filelist;
		while( curfilenode->name != NULL )
		{
			if ( curfilenode->type != FILETYPE_DIR )
			{
				// zip file now
				ArchiveZip( "F:\\testzip.zip", curfilenode->name, "password" );
			}
			tmpfilenode = curfilenode;
			curfilenode = curfilenode->next;
			free(tmpfilenode);
		}
	}
	else
	{
		// no file inside folder
		free(filelist);
	}

	return 0;
}
