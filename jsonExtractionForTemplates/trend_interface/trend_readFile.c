/*------------------------------------------------------------------------------

Module:   ReadTrendTemplateFile

Purpose:  This is called from 2 places
- InitTrendTemplate is called once by the equipment class (not instance) by passing
a big template file (consists of all the templates within it)
- InitializeTrendTemplate is called by each instance by passing individual templates

Filename: trend_readFile.c

Inputs:   pTemplateName - Name of the template json file to read

Outputs:  buffer - Pointer to the buffer structure the file contains

***NOTE: Caller is responsible for releasing buffer
------------------------------------------------------------------------------*/
#include <trend_api.h>
#include <oreResources.h>
#include "trend_api_private.h"
#include <fileio.h>
#include <uniStr.h>
#include <decompress_json.h>

// define to find the first curly brace "{" in the file.
// This signals the beginning of the template
#define START_OF_DATA_MODEL_TEMPLATE  123

//templateName will be null if one file needs to be read.
ERROR_STATUS ReadTrendTemplateFile(TCHAR* pTemplateName, SIGNED8** buffer)
{
  UNSIGNED16 jsonPathSize = 0;
  TCHAR* jsonFilePath = NULL;
  SIGNED8 jsonFilePathForGZ[MAX_FILE_PATH] = {0}; // expects ascii for WIN32 and Unicode for target
  SIGNED8* fileBuffer = NULL;
  UNSIGNED32 fileSize = 0;
  ERROR_STATUS status;
  PARM_DATA  TemplatePathParm, TemplateParm;
  TCHAR*       pTemplatePath;
  SIGNED8 isCompressed = 0;

  //Read the Template path
  oreGetMyResource(RID_DATA_MODEL_TEMPLATE_PATH, &TemplatePathParm);
  pTemplatePath = (TCHAR*)TemplatePathParm.parmValue.tString.strPtr;

  if (pTemplatePath == NULL)
  {
    return ERROR_RESPONSE;
  }

  if (pTemplateName == NULL)
  {
    //Read the Template Name from the resource.
    oreGetMyResource(RID_TREND_DATA_MODEL_TEMPLATE, &TemplateParm);
    pTemplateName = (TCHAR*)TemplateParm.parmValue.tString.strPtr;
  }

  if (pTemplateName == NULL)
  {
    return ERROR_RESPONSE;
  }

  //Get the number of bytes associated so we can allocate them
  jsonPathSize = STR_STORE(OSstrlen(pTemplatePath) + OSstrlen(pTemplateName)); //to hold path and template name

  //Allocate memory to hold template file name
  jsonFilePath  = (TCHAR*)OSacquire(jsonPathSize);

  if (jsonFilePath == NULL)
  {
    //Release Template Path and Template parms
    apsReleaseParm(&TemplatePathParm);
    apsReleaseParm(&TemplateParm);

    return NOT_ENOUGH_MEMORY;
  }

  OSmemset(jsonFilePath, 0, jsonPathSize);

  //Copy to jsonFilePath
  OSstrcpy(jsonFilePath, pTemplatePath);

  //Concatenate path with the file name
  OSstrcat(jsonFilePath, pTemplateName);

  //Release Template Path and Template parms
  apsReleaseParm(&TemplatePathParm);
  apsReleaseParm(&TemplateParm);

  if (OSstrstr(jsonFilePath, _T(".jz")) != NULL)
  {
    isCompressed = 1;
  }
  else
  {
    isCompressed = 0;
  }

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

  // Now open and Get the File size in bytes

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
    status = loadTrendDataModelFile(jsonFilePath, &fileBuffer);
  }

  // caller is responsible for releasing the buffer
  if (fileBuffer)
  {
    *buffer = fileBuffer;
  }

  //Release the memory allocated for storing jsonFilePath
  OSrelease(jsonFilePath);

  return status;
//lint -e{429}  pTemplateName being released by the callee
}



/*------------------------------------------------------------------------------
Module:   loadTrendDataModelFile method

Purpose:  Loads the uncompressed file into the buffer

Inputs:   jsonTemplatePath - path of the file

Outputs:  fileOutputBuffer - Uncompressed byte buffer that will be used by
                             JSON parser
------------------------------------------------------------------------------*/
ERROR_STATUS loadTrendDataModelFile(TCHAR *jsonTemplatePath, SIGNED8 **fileOutputBuffer)
{
  UNSIGNED32   fileSize = 0;
  UNSIGNED32   currentFilePosition = 0;
  UNSIGNED32   bytesRead = 0;
  UNSIGNED8    templateChar = 0;
  SIGNED8      *fileBuffer;
  ERROR_STATUS status = OK;
  void *pFile;

  pFile = OSFileOpen(jsonTemplatePath, _T("r"));

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

  //Continue looping until you find the first "{" (Char 123).
  //Typically, the first character itself will be "{"
  //But need to check just in case.
  do
  {
    OSFileRead(&templateChar, sizeof(UNSIGNED8), 1, pFile);
  }
  while (templateChar != START_OF_DATA_MODEL_TEMPLATE);
  
  
  //Get the File position of { - Subtract 1 position since we have already read.
  currentFilePosition = OSFileTell(pFile) - 1;
  fileSize = fileSize - currentFilePosition;
  OSFileSeek(pFile, currentFilePosition, SEEK_SET);

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
