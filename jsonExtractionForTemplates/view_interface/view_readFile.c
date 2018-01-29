/*------------------------------------------------------------------------------

Module:   View Interface - Read View File

Purpose:  This is responsible to read view file. Fetches the view Name from
          ore Resource RID_DATA_MODEL_VIEW_DEF

Filename: view_readFile.c

Inputs:   NA

Outputs:  buffer - Pointer to the buffer structure the file contains

***NOTE: Caller is responsible for releasing buffer

------------------------------------------------------------------------------*/
#include <view_api.h>
#include <oreResources.h>
#include "view_api_private.h"
#include <uniStr.h>
#include <decompress_json.h>


ERROR_STATUS ReadViewFile(SIGNED8** buffer)
{
  UNSIGNED16 jsonPathSize = 0;
  TCHAR* jsonFilePath = NULL;
  SIGNED8 jsonFilePathForGZ[MAX_FILE_PATH] = {0}; // expects ascii for WIN32 and Unicode for target
  SIGNED8* fileBuffer = NULL;
  UNSIGNED32 fileSize = 0;
  ERROR_STATUS status;
  PARM_DATA  TemplatePathParm, viewParm;
  TCHAR*       pTemplatePath;
  TCHAR*    pViewFilename;
  MODEL_CLASS_VARS* classVarPtr = NULL;
  VIEW_DATABASE* viewDb;
  SIGNED8 isCompressed = 0;

  //Check if the file is already read - If read, view Count will have Data in viewDatabase

  // get ptr to the model's class vars
  classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

  viewDb = classVarPtr->view_database;

  if (viewDb == NULL)
  {
    return ERROR_RESPONSE;
  }

  if (viewDb->viewCount > 0) //View Count greater than 0, data is already parsed just return.
  {
    return VIEWFILE_ALREADY_PARSED;
  }


  //Read the View path - This is same as the template path
  oreGetMyResource(RID_DATA_MODEL_TEMPLATE_PATH, &TemplatePathParm);
  pTemplatePath = (TCHAR*)TemplatePathParm.parmValue.tString.strPtr;

  if (pTemplatePath == NULL)
  {
    return ERROR_RESPONSE;
  }

  //Read the View Name from the resource.
  oreGetMyResource(RID_DATA_MODEL_VIEW_DEF, &viewParm);
  pViewFilename = (TCHAR*)viewParm.parmValue.tString.strPtr;

  if (pViewFilename == NULL)
  {
    return ERROR_RESPONSE;
  }

  //Get the number of bytes associated so we can allocate them
  jsonPathSize = STR_STORE(OSstrlen(pTemplatePath) + OSstrlen(pViewFilename)); //to hold path and template name

  //Allocate memory to hold template file name
  jsonFilePath  = (TCHAR*)OSacquire(jsonPathSize);

  if (jsonFilePath == NULL)
  {
    //Release Archive Parm and view Parm
    apsReleaseParm(&TemplatePathParm);
    apsReleaseParm(&viewParm);

    return NOT_ENOUGH_MEMORY;
  }

  OSmemset(jsonFilePath, 0, jsonPathSize);

  //Copy to jsonFilePath
  OSstrcpy(jsonFilePath, pTemplatePath);

  //Concatenate path with the file name
  OSstrcat(jsonFilePath, pViewFilename);

  //Release Archive Parm and view Parm
  apsReleaseParm(&TemplatePathParm);
  apsReleaseParm(&viewParm);

  if (OSstrstr(jsonFilePath, _T(".jz")) != NULL)
  {
    isCompressed = 1;
  }
  else
  {
    isCompressed = 0;
  }


  //Open and Get the File size in bytes
  if (OSValidateFile(jsonFilePath) == OK) // Validate Size and CRC for given file.
  {
#if defined(WIN32)
    uniToAscii(jsonFilePath, jsonFilePathForGZ);  // store file path in ascii
#else
    OSstrcpy((TCHAR*)jsonFilePathForGZ, jsonFilePath);  // store file path in UNICODE
#endif
  }
  else
  {
    //Release the memory allocated for storing jsonFilePath
    OSrelease(jsonFilePath);
    return FILE_TRANSFER_ERROR;
  }

  if (isCompressed)
  {
    status = GetUncompressedFileSize(jsonFilePath, &fileSize);

    if(status == OK)
    {
      status = DecompressJZFile(jsonFilePathForGZ, fileSize, &fileBuffer);
    }
  }
  else
  {
    status = loadViewFile(jsonFilePath, &fileBuffer);
  }

  // caller is responsible for releasing the buffer
  if (fileBuffer)
  {
    *buffer = fileBuffer;
  }

  //Release the memory allocated for storing jsonFilePath
  OSrelease(jsonFilePath);

  return status;

}


/*------------------------------------------------------------------------------
Module:   loadViewFile method

Purpose:  Loads the uncompressed file into the buffer

Inputs:   jsonTemplatePath - path of the file

Outputs:  fileOutputBuffer - Uncompressed byte buffer that will be used by
                             JSON parser
------------------------------------------------------------------------------*/
ERROR_STATUS loadViewFile(TCHAR *jsonTemplatePath, SIGNED8 **fileOutputBuffer)
{
  UNSIGNED32   fileSize = 0;
  UNSIGNED32   bytesRead = 0;
  SIGNED8      *fileBuffer;
  ERROR_STATUS status = OK;
  void *pFile;

  TCHAR rb_filemode[] = {(TCHAR)'r', (TCHAR)'b', (TCHAR)'\0'};
  
  pFile = (void*)OSFileOpen(jsonTemplatePath, rb_filemode);

  if (pFile == NULL)
  {
    return FILE_NOT_FOUND;
  }

  OSFileSeek(pFile, 0, SEEK_END);
  fileSize = OSFileTell(pFile);

  if (fileSize <= 1)
  {
    // close the file
    OSFileClose(pFile);
    return ERROR_RESPONSE;
  }

  //Set back to the beginning.
  OSFileSeek(pFile, 0, SEEK_SET);

  fileBuffer = (SIGNED8*)OSacquire(sizeof(SIGNED8) * fileSize + 1);  //Add 1 more byte for '\0'

  if (fileBuffer)
  {
    OSmemset(fileBuffer, 0, sizeof(UNSIGNED8) * fileSize + 1);

    bytesRead = OSFileRead(fileBuffer, sizeof(UNSIGNED8), fileSize, pFile);

    //Mark the last item Read as end of File.
    fileBuffer[bytesRead] = '\0';

    // caller is responsible for releasing the buffer
    *fileOutputBuffer = fileBuffer;

    status = OK;
  }
  else
  {
    status = NOT_ENOUGH_MEMORY;
  }

  // close the file
  OSFileClose(pFile);

  return status;
  //lint -e{429}  jsonTemplatePath being released by the callee
}
