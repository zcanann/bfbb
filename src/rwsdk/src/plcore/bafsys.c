#include <stdio.h>
#include <rwsdk/rwcore.h>

/* Not declared by our MSL stdio.h */
extern size_t fread(void* buffer, size_t size, size_t count, FILE* stream);
extern char* fgets(char* buffer, int maxLen, FILE* stream);
extern int fputs(const char* buffer, FILE* stream);
extern int feof(FILE* stream);

RwFileFunctions* RwOsGetFileInterface(void)
{
    return &RWSRCGLOBAL(fileFuncs);
}

static RwBool rwfexist(const RwChar* name)
{
    void* fptr;
    RwBool result;

    fptr = RWSRCGLOBAL(fileFuncs.rwfopen)(name, "rb");
    result = (fptr != NULL);

    if (fptr)
    {
        RWSRCGLOBAL(fileFuncs.rwfclose)(fptr);
    }

    return result;
}

RwBool _rwFileSystemOpen(void)
{
    RWSRCGLOBAL(fileFuncs.rwfexist) = rwfexist;
    RWSRCGLOBAL(fileFuncs.rwfopen) = (rwFnFopen)fopen;
    RWSRCGLOBAL(fileFuncs.rwfclose) = (rwFnFclose)fclose;
    RWSRCGLOBAL(fileFuncs.rwfread) = (rwFnFread)fread;
    RWSRCGLOBAL(fileFuncs.rwfwrite) = (rwFnFwrite)fwrite;
    RWSRCGLOBAL(fileFuncs.rwfgets) = (rwFnFgets)fgets;
    RWSRCGLOBAL(fileFuncs.rwfputs) = (rwFnFputs)fputs;
    RWSRCGLOBAL(fileFuncs.rwfeof) = (rwFnFeof)feof;
    RWSRCGLOBAL(fileFuncs.rwfseek) = (rwFnFseek)fseek;
    RWSRCGLOBAL(fileFuncs.rwfflush) = (rwFnFflush)fflush;
    RWSRCGLOBAL(fileFuncs.rwftell) = (rwFnFtell)ftell;

    return TRUE;
}

void _rwFileSystemClose(void)
{
}
