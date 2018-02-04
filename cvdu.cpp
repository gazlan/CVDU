/* ******************************************************************** **
** @@ CVDU
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Dscr   :
** ******************************************************************** */

/* ******************************************************************** **
**                uses pre-compiled headers
** ******************************************************************** */

#include <stdafx.h>

#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "pack_zlib.h"

/* ******************************************************************** **
** @@                   internal defines
** ******************************************************************** */

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef NDEBUG
#pragma optimize("gsy",on)
#pragma comment(linker,"/FILEALIGN:512 /MERGE:.rdata=.text /MERGE:.data=.text /SECTION:.text,EWR /IGNORE:4078")
#endif 

#define TAR_BLOCKSIZE      (512)

/* ******************************************************************** **
** @@                   internal prototypes
** ******************************************************************** */

/* ******************************************************************** **
** @@                   external global variables
** ******************************************************************** */

/* ******************************************************************** **
** @@                   static global variables
** ******************************************************************** */
  
/* ******************************************************************** **
** @@                   real code
** ******************************************************************** */

CWinApp theApp;

/* ******************************************************************** **
** @@ ()
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

static int cli_untgz(int fd,const char* const destdir)
{
   char *path,osize[13],name[101],type;
   
   char block[TAR_BLOCKSIZE];
   
   int nbytes,nread,nwritten,in_block = 0,fdd;
   
   unsigned int size = 0, pathlen = strlen(destdir) + 100 + 5;
   
   FILE*    outfile = NULL;
   
   struct _stat   foo;
   
   gzFile*     infile;

   TRACE("in cli_untgz()\n");

   if ((fdd = _dup(fd)) == -1) 
   {
      TRACE("cli_untgz: Can't duplicate descriptor %d\n",fd);
      return -1;
   }

   if ((infile = (gzFile*)gzdopen(fdd,"rb")) == NULL) 
   {
      TRACE("cli_untgz: Can't gzdopen() descriptor %d,errno = %d\n",fdd,errno);

      if (_fstat(fdd,&foo) == 0)
      {
         _close(fdd);
      }

      return -1;
   }

   path = (char*)calloc(sizeof(char),pathlen);

   if (!path) 
   {
      TRACE("cli_untgz: Can't allocate memory for path\n");
      gzclose(infile);
      return -1;
   }

   while (true) 
   {
      nread = gzread(infile,block,TAR_BLOCKSIZE);

      if (!in_block && !nread)
      {
         break;
      }

      if (nread != TAR_BLOCKSIZE) 
      {
         TRACE("cli_untgz: Incomplete block read\n");
         free(path);
         gzclose(infile);
         return -1;
      }

      if (!in_block) 
      {
         if (block[0] == '\0')  // We're done 
         {
            break;
         }

         strncpy(name,block,100);
         name[100] = '\0';

         if (strchr(name,'/')) 
         {
            TRACE("cli_untgz: Slash separators are not allowed in CVD\n");
            free(path);
            gzclose(infile);
            return -1;
         }

         _snprintf(path,pathlen,"%s/%s",destdir,name);
         TRACE("cli_untgz: Unpacking %s\n",path);

         type = block[156];

         switch (type) 
         {
            case '0':
            case '\0':
            {
               break;
            }
            case '5':
            {
               TRACE("cli_untgz: Directories are not supported in CVD\n");
               free(path);
               gzclose(infile);
               return -1;
            }
            default:
            {
               TRACE("cli_untgz: Unknown type flag '%c'\n",type);
               free(path);
               gzclose(infile);
               return -1;
            }
         }

         in_block = 1;

         if (outfile) 
         {
            if (fclose(outfile)) 
            {
               TRACE("cli_untgz: Cannot _close file %s\n",path);
               free(path);
               gzclose(infile);
               return -1;
            }

            outfile = NULL;
         }

         outfile = fopen(path,"wb");

         if (!outfile) 
         {
            TRACE("cli_untgz: Cannot create file %s\n",path);
            free(path);
            gzclose(infile);
            return -1;
         }

         strncpy(osize,block + 124,12);
         osize[12] = '\0';

         if ((sscanf(osize,"%o",&size)) == 0) 
         {
            TRACE("cli_untgz: Invalid size in header\n");
            free(path);
            gzclose(infile);
            fclose(outfile);
            return -1;
         }
      } 
      else // write or continue writing file contents
      { 
         nbytes   = size > TAR_BLOCKSIZE ? TAR_BLOCKSIZE : size;
         nwritten = fwrite(block,1,nbytes,outfile);

         if (nwritten != nbytes) 
         {
            TRACE("cli_untgz: Wrote %d instead of %d (%s)\n",nwritten,nbytes,path);
            free(path);
            gzclose(infile);
            return -1;
         }

         size -= nbytes;
   
         if (size == 0)
         {
            in_block = 0;
         }
      }
   }

   if (outfile)
   {
      fclose(outfile);
   }

   gzclose(infile);
   free(path);
   
   return 0;
}

/* ******************************************************************** **
** @@ ShowHelp()
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

void ShowHelp()
{
   const char  pszCopyright[] = "-*-   CVDU   *   Copyright (c) Gazlan, 2011   -*-";
   const char  pszDescript [] = "CVD ClamAV database unpacker";
   const char  pszE_Mail   [] = "complains_n_suggestions direct to gazlan@yandex.ru";

   printf("%s\n\n",pszCopyright);
   printf("%s\n\n",pszDescript);
   printf("Usage: cvdu.com file.cvd\n\n");
   printf("%s\n",pszE_Mail);
}

/* ******************************************************************** **
** @@ main()
** @  Copyrt :
** @  Author :
** @  Modify :
** @  Update :
** @  Notes  :
** ******************************************************************** */

int main(int argc,char** argv)
{
   // initialize MFC and print and error on failure
   if (!AfxWinInit(::GetModuleHandle(NULL),NULL,::GetCommandLine(),0))
   {
      return -1;
   }

   if (argc != 2)
   {
      ShowHelp();
      return 0;
   }

   if ((!strcmp(argv[1],"?")) || (!strcmp(argv[1],"/?")) || (!strcmp(argv[1],"-?")) || (!stricmp(argv[1],"/h")) || (!stricmp(argv[1],"-h")))
   {
      ShowHelp();
      return 0;
   }

   char     pszFile[MAX_PATH + 1];
   char     pszPath[MAX_PATH + 1];

   strcpy(pszPath,".");

   strncpy(pszFile,argv[1],MAX_PATH);
   pszFile[MAX_PATH] = 0;  // ASCIIZ

   int fd = _open(pszFile,O_RDONLY | O_BINARY);

   if (fd == -1)
   {
      return -1;
   }

   if (_lseek(fd,512,SEEK_SET) < 0) 
   {
      _close(fd);
      return -1;
   }

   int  ret = cli_untgz(fd,pszPath);

   _close(fd);
   
   return ret;
}

/* ******************************************************************** **
** @@                   End of File
** ******************************************************************** */
