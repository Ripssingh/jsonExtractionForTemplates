/*------------------------------------------------------------------------------

Module:   template_init

Purpose:  - InitTrendTemplate is called once by the equipment class (not instance) 
		  - InitializeTrendTemplate is called by each instance

Filename: trend_init.c

Inputs:   NA

Outputs:  NA

------------------------------------------------------------------------------*/
#include <trend_api.h>
#include "trend_api_private.h"
#include <uniStr.h>


/*------------------------------------------------------------------------------
Module:   InitTrendTemplate method

Purpose:  Handles the initialization of the trend template. This is called once by
		  the equipment class (not instance)

Inputs:   Pointer to the template Database. This has to be initialized by the calling
		  function.

Outputs:  None.
------------------------------------------------------------------------------*/
ERROR_STATUS InitTrendTemplate()
{
	APSHASHTBL *templateHash = NULL;
	APSHASHTBL * templateReferenceHash = NULL;
	TREND_DATABASE * tempDb;
	SIGNED8 * buffer = NULL;
	ERROR_STATUS status = OK;
	json_t *jsonObject, *jsonTemplateArray, *jsonTemplateData, *jsonVersionObject;
	json_t * jsonTempObj;
	UNSIGNED32 templateCount, temp=0;
	const SIGNED8 * templateName = NULL; 
	const SIGNED8 * templateVersion = NULL;
	UNSIGNED16 * unicodeTemplateName = NULL;
	UNSIGNED16 * unicodeTemplateVersion = NULL;
	MODEL_CLASS_VARS *classVarPtr = NULL;
	
	// get ptr to the model's class vars
  if(equipmentModelClassIndex == 0)
  {
    equipmentModelClassIndex = getEquipmentModelClassIndexHelper();
  }
  
  classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

	//Allocate memory for Template
	tempDb = (TREND_DATABASE *)OSacquire(sizeof(TREND_DATABASE));
	if(tempDb == NULL)
		return NOT_ENOUGH_MEMORY;
	
	//Hash list to store the template name and its corresponding Id.
	if((templateHash=hashtbl_create(TREND_DB_ENTRY_GROW_SIZE, HASH_TYPE_STR)) == NULL) 
		return HASH_CREATE_ERROR;

	//Hash List to store the template Id and its corresponding reference.
	if((templateReferenceHash=hashtbl_create(TREND_DB_ENTRY_GROW_SIZE, HASH_TYPE_INT)) == NULL) 
		return HASH_CREATE_ERROR;
	
	tempDb->trendCount = 0;
	tempDb->trendHash = templateHash;
	tempDb->trendStructureHash = templateReferenceHash;

	//Read the big template File
	status = ReadTrendTemplateFile(NULL, &buffer);
	if(!status)
	{
		//Status is Ok..We should be able to pass this data to json interface.
		status = parseJSONString(buffer, &jsonObject);

		//Buffer is read into json Object. No need to keep the buffer.
		OSrelease(buffer);

		if(!status)
		{
			//JSON was parsed successfully..
			getJSONObjectForKey(jsonObject, _T("Version"), &jsonVersionObject);

			//Get the version number and add to MODEL CLASS VARS
			if(jsonVersionObject != NULL)
			{
				//Get the version number
				templateVersion = json_string_value(jsonVersionObject);

				//Convert to Unicode
				getUnicodeFromASCII(templateVersion, &unicodeTemplateVersion);

				//Add this to Class Vars
				classVarPtr->template_Version = (TCHAR *)unicodeTemplateVersion;

			}

			//Read the json array entries and parse each template
			getJSONObjectForKey(jsonObject, _T("TemplateExtensions"), &jsonTemplateArray);
			if(jsonTemplateArray != NULL)
			{	
				//Get the number of properties associated.
				templateCount = json_array_size(jsonTemplateArray);	

				//Loop through the template array, get index and pass to Read
				for(temp = 0; temp < templateCount; temp++)
				{	
					//Get each template from json
					jsonTemplateData = json_array_get(jsonTemplateArray, temp);

					//Get the template ID from the jsonTemplateData
					getJSONObjectForKey(jsonTemplateData, _T("-TemplateID"), &jsonTempObj);

					if(jsonTempObj != NULL)
					{
						templateName = json_string_value(jsonTempObj);

						//Convert to Unicode
						getUnicodeFromASCII(templateName, &unicodeTemplateName);

						AddTrendTemplateToHash((TCHAR *)unicodeTemplateName, jsonTemplateData, tempDb);
					}
				}
			}

      //Clear the json Object
      json_decref(jsonObject);
		}
		
	}
	else if(status == NOT_ENOUGH_MEMORY)
	{
		//OSTrace(_T("Not enough memory to perform this operation.."));
		return status;
	}
	else
	{
		//OSTrace(_T("Resource did not exist, we might get single template loaded"));
	}
	

	
	classVarPtr->trend_database = tempDb;

	return OK;
}



/*------------------------------------------------------------------------------
Module:   InitializeTrendTemplate method

Purpose:  This function is called when a new template name wants to be added to hash, 
          a template id is returned. This is called by each equipment object instance  

Inputs:   Pointer to interface Name

Outputs:  Template Id is returned
------------------------------------------------------------------------------*/
ERROR_STATUS InitializeTrendTemplate(TCHAR * interfaceName, UNSIGNED16 * templateId)
{
	UNSIGNED16 * tempStorage = NULL;
	UNSIGNED16 tempCount;
	TREND_DATABASE * templateDb = NULL;	
	SIGNED8 * buffer = NULL;
	json_t *jsonObject;
	json_t *jsonTempObj;
	json_t *jsonTemplateData = NULL;
	const SIGNED8 * templateName = NULL; 
	UNSIGNED16 * unicodeTemplateName = NULL;
	TCHAR * jsonFileName = NULL;
	ERROR_STATUS status = OK;
	MODEL_CLASS_VARS *classVarPtr = NULL;
	
	// get ptr to the model's class vars
  classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

	//Get the template DB
	templateDb = classVarPtr->trend_database;

	if(templateDb == NULL)
		return ERROR_RESPONSE;
	
	//First check if this interface name has a corresponding id already added. If added, just return the id.
	//check templatename hash data to see if an id exists for this.
	if(hashtbl_get(templateDb->trendHash, interfaceName, STR_STORE(OSstrlen(interfaceName)), (void **)&tempStorage))		
	{
		//This interface name is not in the hash. Need to read from the file and store in hash

		//Get the number of bytes associated so we can allocate them
		tempCount = STR_STORE(OSstrlen(interfaceName)) + 10; //to hold ".json"
		
		//Allocate memory to hold template file name
		jsonFileName  = (TCHAR *)OSallocate( tempCount ); 

		//Copy to jsonFileName
		OSstrcpy(jsonFileName, interfaceName);

		//Update this to hold .json
		OSstrcat(jsonFileName, _T(".json"));

		status = ReadTrendTemplateFile(jsonFileName, &buffer);

		//Release jsonFileName, not required anymore.
		OSrelease(jsonFileName);

		if(status == OK)
		{
			//Status is Ok..We should be able to pass this data to json interface.
			status = parseJSONString(buffer, &jsonObject);

			//Buffer is read into json Object. No need to keep the buffer.
			OSrelease(buffer);

			if(status == OK)
			{
				//Read the json template key
				getJSONObjectForKey(jsonObject, _T("TemplateExtensions"), &jsonTemplateData);
				if(jsonTemplateData != NULL)
				{
					//Get the template ID from the jsonTemplateData
					getJSONObjectForKey(jsonTemplateData, _T("-TemplateID"), &jsonTempObj);

					if(jsonTempObj != NULL)
					{
						templateName = json_string_value(jsonTempObj);

						//Convert to Unicode
						getUnicodeFromASCII(templateName, &unicodeTemplateName);

						AddTrendTemplateToHash((TCHAR *)unicodeTemplateName, jsonTemplateData, templateDb);

						//Latest Template Count will be the template Id
						*templateId = templateDb->trendCount;
					}
				}

        //Clear the json Object
        json_decref(jsonObject);
			}      
		}
	}
	else
	{
		*templateId = *tempStorage;
	}
	
	return status;
}


