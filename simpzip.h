typedef enum _filetype_e
{
        FILETYPE_NONE = 0
        , FILETYPE_DIR          // directory
        , FILETYPE_FILE         // file
} filetype_e;

//filename link list structure
typedef struct _filelist_t
{
        filetype_e type;
        char * name;
        struct _filelist_t * next;
} filelist_t;

extern bool ListFileInDir( char * path, filelist_t * filelist );
extern bool ReadFileData( char * path, char ** content, unsigned long * size );
extern bool ArchiveZip( char * zipPath, char * file, char * password );