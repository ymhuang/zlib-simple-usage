#include <Windows.h>
#include <Shlwapi.h>
#include "zlib.h"
#include "zip.h"
#include "simpzip.h"

// list files in directory
extern bool ListFileInDir( char * path, filelist_t * filelist )
{
	bool retVal = false;
	WIN32_FIND_DATA findFileData;
	HANDLE hFind = 0;
	wchar_t * parentdir = NULL;
	char * subdir = NULL;
	unsigned long size = 0;
	filelist_t * headFilenode = NULL;
	filelist_t * curFilenode = NULL;

	headFilenode = filelist;
	curFilenode = filelist;

	if ( '\\' == path[strlen(path) - 1] )
	{
		size = (int)strlen(path) + (int)strlen("*") + 1;
		parentdir = (wchar_t *) malloc( sizeof(wchar_t) * size );
		memset( parentdir, 0, sizeof(wchar_t) * size );
		swprintf( parentdir, size, L"%hs*", path );
	}
	else
	{
		size = (int)strlen(path) + (int)strlen("\\*") + 1;
		parentdir = (wchar_t *) malloc( sizeof(wchar_t) * size );
		memset( parentdir, 0, sizeof(wchar_t) * size );
		swprintf( parentdir, size, L"%hs\\*", path );
	}
	memset( &findFileData, 0, sizeof(WIN32_FIND_DATA) );
	hFind = FindFirstFile( parentdir, &findFileData );

	do
	{
		if ( (0 == wcsncmp( findFileData.cFileName, TEXT("."), wcslen(TEXT(".")) ))
			  || (0 == wcsncmp( findFileData.cFileName, TEXT(".."), wcslen(TEXT("..")) ))
			)
		{
			// ignore . and ..
			continue;
		}
		else
		{
			if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				size = (int)strlen(path) + (int)strlen("\\") + (int)wcslen(findFileData.cFileName) + 1;
				subdir = (char *) malloc( sizeof(char) * size );
				memset( subdir, 0, sizeof(char) * size );
				sprintf_s( subdir, size, "%s\\%ls", path, findFileData.cFileName );
				ListFileInDir( subdir, curFilenode );
				free(subdir);
				while ( NULL != curFilenode->next )
				{
					curFilenode = curFilenode->next;
				}
				curFilenode->type = FILETYPE_DIR;
			}
			else
			{
				curFilenode->type = FILETYPE_FILE;
			}

			size = (int)strlen(path) + (int)strlen("\\") + (int)wcslen(findFileData.cFileName) + 1;
			curFilenode->name = (char *)malloc( sizeof(char) * size );
			memset( curFilenode->name, 0, sizeof(char) * size );
			sprintf_s( curFilenode->name, size, "%s\\%ls", path, findFileData.cFileName );

			curFilenode->next = (filelist_t *)malloc( sizeof(filelist_t) );
			memset( curFilenode->next, 0, sizeof(filelist_t) );
			curFilenode = curFilenode->next;
		}
	} while (FindNextFile( hFind, &findFileData) != 0 );

	free(parentdir);

	if ( headFilenode->name != NULL )
	{
		retVal = true;
	}
	else
	{
		retVal = false;
	}

	return retVal;
}

// read file data
extern bool ReadFileData( char * path, char ** content, unsigned long * size )
{
	bool retVal = false;
	wchar_t * filepath = NULL;
	HANDLE fileHandle = 0;
	LARGE_INTEGER fileSize;
	unsigned long requiredSize = 0;
	unsigned long readSize = 0;

	// convert path in wchar_t
	requiredSize = (unsigned long)strlen(path) + 1;
	filepath = (wchar_t *)malloc( sizeof(wchar_t) * requiredSize );
	memset( filepath, 0, sizeof(wchar_t) * requiredSize );
	swprintf( filepath, requiredSize, L"%hs", path );

	// open file
	fileHandle = CreateFile( filepath, (GENERIC_READ|GENERIC_WRITE), 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	// if open success, then file will not able to access until close its handle.
	if ( INVALID_HANDLE_VALUE != fileHandle )
	{
		// check required file size
		memset( &fileSize, 0, sizeof(LARGE_INTEGER) );
		if ( 0 != GetFileSizeEx( fileHandle, &fileSize ) )
		{
			requiredSize = (unsigned long)fileSize.QuadPart;
			if( NULL == content )
			{
				*size = requiredSize;
				retVal = true;
			}
			else
			{
				if ( *size < requiredSize )
				{
					*content = (char *) realloc( *content, sizeof(char) * requiredSize );
					memset( *content, 0, sizeof(char) * requiredSize );
				}

				*size = requiredSize;
				if ( FALSE != ReadFile( fileHandle, *content, requiredSize, &readSize, NULL ) )
				{
					if ( readSize == requiredSize )
					{
						retVal = true;
					}
					else
					{
						CloseHandle(fileHandle);
						fileHandle = INVALID_HANDLE_VALUE;
						printf("ReadFileData() File size mismatch.\n");
						retVal = false;
					}
				}
				else
				{
					CloseHandle(fileHandle);
					fileHandle = INVALID_HANDLE_VALUE;
					printf("ReadFile() fail(0x%X).\n", GetLastError() );
					retVal = false;
				}
			}
		}
		else
		{
			CloseHandle(fileHandle);
			fileHandle = INVALID_HANDLE_VALUE;
			printf("GetFileSizeEx() fail(0x%X).\n", GetLastError() );
			retVal = false;
		}

		// close file
		if (INVALID_HANDLE_VALUE != fileHandle)
		{
			CloseHandle(fileHandle);
		}
	}
	else
	{
		printf( "CreateFile() fail(0x%X).\n", GetLastError() );
		retVal = false;
	}

	free(filepath);

	return retVal;
}

// convert file info to zip info
static bool ConvertZipInfo( char * file, zip_fileinfo * zipFileInfo )
{
	bool retVal = false;
	SYSTEMTIME systemTime;

	memset( &systemTime, 0, sizeof(SYSTEMTIME) );
	GetSystemTime(&systemTime);

	zipFileInfo->dosDate = 0; /* set dos_date == 0 to use tmz_date */

	// Intentionally to get system time here.
	zipFileInfo->tmz_date.tm_year = systemTime.wYear; /* years - [1980..2044] */
	zipFileInfo->tmz_date.tm_mon = systemTime.wMonth - 1; /* months since January - [0,11] */
	zipFileInfo->tmz_date.tm_mday = systemTime.wDay; /* day of the month - [1,31] */
	zipFileInfo->tmz_date.tm_hour = systemTime.wHour; /* hours since midnight - [0,23] */
	zipFileInfo->tmz_date.tm_min = systemTime.wMinute; /* minutes after the hour - [0,59] */
	zipFileInfo->tmz_date.tm_sec = systemTime.wSecond; /* seconds after the minute - [0,59] */

	zipFileInfo->internal_fa = 0; /* internal file attributes        2 bytes */
	zipFileInfo->external_fa = 0; /* external file attributes        4 bytes */

	retVal = true;

	return retVal;
}


// archive file to zip file
extern bool ArchiveZip( char * zipPath, char * file, char * password )
{
	bool retVal = false;
	zipFile zfp = NULL;
	zip_fileinfo zipFileInfo;
	unsigned long crc = 0;
	int over4gb = 0;
	char * filedata = NULL;
	unsigned long size = 0;
	char * zippedFilename = NULL;
	wchar_t * ws = NULL;

	// Check if zipfile already exist and open zip file pointer
	// convert zipPath into wchar for PathFileExists()
	size = (int)strlen(zipPath) + 1;
	ws = (wchar_t *) malloc( sizeof(wchar_t) * size );
	memset( ws, 0, sizeof(wchar_t) * size );
	swprintf( ws, size, L"%hs", zipPath );
	if ( FALSE == PathFileExists(ws) )
	{
		zfp = zipOpen64( zipPath, APPEND_STATUS_CREATE );
	}
	else
	{
		zfp = zipOpen64( zipPath, APPEND_STATUS_ADDINZIP );
	}
	free(ws);

	if (NULL != zfp )
	{
		ConvertZipInfo( file, &zipFileInfo );
		size = 0;
		if ( true == ReadFileData( file, NULL, &size ) )
		{
			filedata = (char *)malloc( sizeof(char) * size );
			if ( NULL != filedata )
			{
				memset( filedata, 0, sizeof(char) * size );
				if ( true == ReadFileData( file, &filedata, &size ) )
				{
					if ( NULL == password )
					{
						crc = 0;
					}
					else
					{
						// calculate crc of original file
						crc = crc32( crc, (const Bytef *)filedata, size );
					}

					// check if file over 4gb and need zip64 algorithm
					if ( 0xFFFFFFFF < size )
					{
						over4gb = 1;
					}
					else
					{
						over4gb = 0;
					}

					// zipped filename, ignore "%DRIVE%:\\"
					zippedFilename = strstr( file, ":\\" ) + 2;

					// save into zip file
					if ( ZIP_OK == zipOpenNewFileInZip3_64(	zfp,					/* zip file pointer */
															zippedFilename,			/* filename in zip file */
															&zipFileInfo,			/* supplemental information */
															NULL, 0,				/* extrafield_local buffer & its size to store the local header extra field data, can be NULL */
															NULL, 0,				/* extrafield_global buffer & its size to store the global header extra field data, can be NULL */
															NULL,					/* comment string */
															Z_DEFLATED,				/* compression method */
															Z_DEFAULT_COMPRESSION,	/* level of compression */
															0,						/* if set to 1, write raw file */
															-MAX_WBITS,				/* the base two logarithm of the window size (the size of the history buffer). */
															DEF_MEM_LEVEL,			/* how much memory should be allocated */
															Z_DEFAULT_STRATEGY,		/* compression algorithm, Z_DEFAULT_STRATEGY for normal data */
															password,				/* crypting password (NULL for no crypting) */
															crc,					/* crc of file to compress (needed for crypting) */
															over4gb					/* this MUST be '1' if the uncompressed size is >= 0xffffffff (4GB). */
														)
						)
					{
						if ( ZIP_OK == zipWriteInFileInZip( zfp, filedata, size ) )
						{
							retVal = true;
						}
						else
						{
							printf( "zipWriteInFileInZip() fail.\n" );
							retVal = false;
						}
						zipCloseFileInZip(zfp);
					}
					else
					{
						printf( "zipOpenNewFileInZip3_64() fail.\n" );
						retVal = false;
					}
				}
				else
				{
					printf( "ReadFileData() fail.\n" );
					retVal = false;
				}
				free(filedata);
			}
			else
			{
				printf( "ArchiveZip() Out of memory.\n" );
				retVal = false;
			}
		}
		else
		{
			printf( "ReadFileData() fail.\n" );
			retVal = false;
		}
		zipClose(zfp, NULL);
	}
	else
	{
		printf( "zipOpen64() fail.\n");
		retVal = false;
	}

	return retVal;
}
