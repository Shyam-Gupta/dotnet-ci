//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information. 
//

/*=====================================================================
**
** Source:  test3.c
**
** Purpose: Tests the PAL implementation of the fopen function. 
**          Test to ensure that you can write to a 'w+' mode file.
**          And that you can read from a 'w+' mode file.
**
** Depends:
**      fprintf
**      fseek
**      fgets
**  

**
**===================================================================*/
                                                                         
#include <palsuite.h>

int __cdecl main(int argc, char **argv)
{
  
    FILE *fp;
    char buffer[128];
  
    if (PAL_Initialize(argc, argv))
    {
        return FAIL;
    }

  
    /* Open a file with 'w+' mode */
    if( (fp = fopen( "testfile", "w+" )) == NULL )
    {
        Fail( "ERROR: The file failed to open with 'w+' mode.\n" );
    }  
  
    /* Write some text to the file */
    if(fprintf(fp,"%s","some text") <= 0) 
    {
        Fail("ERROR: Attempted to WRITE to a file opened with 'w+' mode "
               "but fprintf failed.  Either fopen or fprintf have problems.");
    }

    if(fseek(fp, 0, SEEK_SET)) 
    {
        Fail("ERROR: fseek failed, and this test depends on it.");
    }
  
    /* Attempt to read from the 'w+' only file, should pass */
    if(fgets(buffer,10,fp) == NULL)
    {
        Fail("ERROR: Tried to READ from a file with 'w+' mode set. "
               "This should succeed, but fgets returned NULL.  Either fgets "
               "or fopen is broken.");
    }

    PAL_Terminate();
    return PASS;
}
   

