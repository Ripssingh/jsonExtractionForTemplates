/*------------------------------------------------------------------------------

Module:   template_init

Purpose:  - InitTemplate is called once by the equipment class (not instance) 
		  - InitializeTemplate is called by each instance

Filename: template_init.c

Inputs:   NA

Outputs:  NA
------------------------------------------------------------------------------*/
#include <template_api.h>
#include "template_api_private.h"
#include <uniStr.h>
#include <unit.h>

/*------------------------------------------------------------------------------
Module:   InitTemplate method

Purpose:  Handles the initialization of the template. This is called once by
		  the equipment class (not instance)

Inputs:   Pointer to the template Database. This has to be initialized by the calling
		  function.

Outputs:  None.
------------------------------------------------------------------------------*/
ERROR_STATUS InitTemplate()
{
	APSHASHTBL *templateHash = NULL;
	APSHASHTBL * templateReferenceHash = NULL;
	TEMPLATE_DATABASE * tempDb;
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
	tempDb = (TEMPLATE_DATABASE *)OSacquire(sizeof(TEMPLATE_DATABASE));
	if(tempDb == NULL)
		return NOT_ENOUGH_MEMORY;
	
	//Hash list to store the template name and its corresponding Id.
	if((templateHash=hashtbl_create(TEMPLATE_DB_ENTRY_GROW_SIZE, HASH_TYPE_STR)) == NULL) 
		return HASH_CREATE_ERROR;

	//Hash List to store the template Id and its corresponding reference.
	if((templateReferenceHash=hashtbl_create(TEMPLATE_DB_ENTRY_GROW_SIZE, HASH_TYPE_INT)) == NULL) 
		return HASH_CREATE_ERROR;
	
	tempDb->templateCount = 0;
	tempDb->templateHash = templateHash;
	tempDb->templateStructureHash = templateReferenceHash;

	//Read the big template File
	status = ReadTemplateFile(NULL, &buffer,FALSE);
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
			getJSONObjectForKey(jsonObject, _T("Template"), &jsonTemplateArray);
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
					getJSONObjectForKey(jsonTemplateData, _T("-ID"), &jsonTempObj);

					if(jsonTempObj != NULL)
					{
						templateName = json_string_value(jsonTempObj);
						
						//Convert to Unicode
						getUnicodeFromASCII(templateName, &unicodeTemplateName);

						AddTemplateToHash((TCHAR *)unicodeTemplateName, jsonTemplateData, tempDb);
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
	

	
	classVarPtr->template_database = tempDb;

	return OK;
}
/*------------------------------------------------------------------------------
Module:   LoadTemplate method

Purpose:  This method is called whenever new VAV get connected .
Inputs:   Pointer to Template file 


------------------------------------------------------------------------------*/
ERROR_STATUS LoadTemplate()
{
	UNSIGNED16 * tempStorage = NULL;
	//UNSIGNED16 tempCount;
	TEMPLATE_DATABASE * templateDb = NULL;	
	SIGNED8 * buffer = NULL;
//	UNSIGNED16 ret;
	const SIGNED8 * templateTempName = NULL; 
	UNSIGNED16 * unicodeTemplateTempName = NULL;
json_t *jsonObject, *jsonTemplateArray, *jsonTemplateData;
	json_t * jsonTempObj;

	/*json_t *jsonObject;
	json_t *jsonTempObj;
	json_t *jsonTemplateData = NULL;*/
	TCHAR * templateName = NULL; 
	UNSIGNED16 * unicodeTemplateName = NULL;
	TCHAR * jsonFileName = NULL;
	ERROR_STATUS status = OK;
	MODEL_CLASS_VARS *classVarPtr = NULL;
	
	// get ptr to the model's class vars
  classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

	//Get the template DB
	templateDb = classVarPtr->template_database;

	if(templateDb == NULL)
		return ERROR_RESPONSE;
	
	//First check if this interface name has a corresponding id already added. If added, just return the id.
	//check templatename hash data to see if an id exists for this.
	
		//This interface name is not in the hash. Need to read from the file and store in hash

		//Get the number of bytes associated so we can allocate them

		//Commented by Ram
		//tempCount = STR_STORE(OSstrlen(interfaceName)) + 10; //to hold ".json"
		//
		////Allocate memory to hold template file name
		//jsonFileName  = (TCHAR *)OSallocate( tempCount ); 

		////Copy to jsonFileName
		//OSstrcpy(jsonFileName, interfaceName);

		////Update this to hold .json
		//OSstrcat(jsonFileName, _T(".json"));
//End Commented
		status = ReadTemplateFile(jsonFileName, &buffer,TRUE);

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
				getJSONObjectForKey(jsonObject, _T("Template"), &jsonTemplateArray);
			if(jsonTemplateArray != NULL)
			{	
					UNSIGNED32 templateCount, temp=0;
				//Get the number of properties associated.
				templateCount = json_array_size(jsonTemplateArray);	

				//Loop through the template array, get index and pass to Read
				for(temp = 0; temp < templateCount; temp++)
				{	
					//Get each template from json
					jsonTemplateData = json_array_get(jsonTemplateArray, temp);
						
					//Get the template ID from the jsonTemplateData
					getJSONObjectForKey(jsonTemplateData, _T("-ID"), &jsonTempObj);

					if(jsonTempObj != NULL)
					{
						templateTempName = json_string_value(jsonTempObj);
						
						//Convert to Unicode
						getUnicodeFromASCII(templateTempName, &unicodeTemplateTempName);

						if(hashtbl_get(templateDb->templateHash, (TCHAR *)unicodeTemplateTempName, STR_STORE(OSstrlen((TCHAR *)unicodeTemplateTempName)), (void **)&tempStorage))		
							{
								AddTemplateToHash((TCHAR *)unicodeTemplateTempName, jsonTemplateData, templateDb);
								//*templateId = templateDb->templateCount;
							}


						
					}
				}
			}

      //Clear the json Object
      json_decref(jsonObject);
		}
		
	}
	
	
	
	return status;
}

/*------------------------------------------------------------------------------
Module:   InitializeTemplate method

Purpose:  This function is called when a new template name wants to be added to hash, 
          a template id is returned. This is called by each equipment object instance  

Inputs:   Pointer to interface Name

Outputs:  Template Id is returned
------------------------------------------------------------------------------*/
ERROR_STATUS InitializeTemplate(TCHAR * interfaceName, UNSIGNED16 * templateId)
{
	UNSIGNED16 * tempStorage = NULL;
	//UNSIGNED16 tempCount;
	TEMPLATE_DATABASE * templateDb = NULL;	
	SIGNED8 * buffer = NULL;
	UNSIGNED16 ret;

json_t *jsonObject, *jsonTemplateArray, *jsonTemplateData;
	json_t * jsonTempObj;

	/*json_t *jsonObject;
	json_t *jsonTempObj;
	json_t *jsonTemplateData = NULL;*/
	const SIGNED8 * templateName = NULL; 
	UNSIGNED16 * unicodeTemplateName = NULL;
	TCHAR * jsonFileName = NULL;
	ERROR_STATUS status = OK;
	MODEL_CLASS_VARS *classVarPtr = NULL;
	
	// get ptr to the model's class vars
  classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

	//Get the template DB
	templateDb = classVarPtr->template_database;

	if(templateDb == NULL)
		return ERROR_RESPONSE;
	
	//First check if this interface name has a corresponding id already added. If added, just return the id.
	//check templatename hash data to see if an id exists for this.
	if(hashtbl_get(templateDb->templateHash, interfaceName, STR_STORE(OSstrlen(interfaceName)), (void **)&tempStorage))		
	{
		//This interface name is not in the hash. Need to read from the file and store in hash

		//Get the number of bytes associated so we can allocate them

		//Commented by Ram
		//tempCount = STR_STORE(OSstrlen(interfaceName)) + 10; //to hold ".json"
		//
		////Allocate memory to hold template file name
		//jsonFileName  = (TCHAR *)OSallocate( tempCount ); 

		////Copy to jsonFileName
		//OSstrcpy(jsonFileName, interfaceName);

		////Update this to hold .json
		//OSstrcat(jsonFileName, _T(".json"));
//End Commented
		status = ReadTemplateFile(jsonFileName, &buffer,TRUE);

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
				getJSONObjectForKey(jsonObject, _T("Template"), &jsonTemplateArray);
			if(jsonTemplateArray != NULL)
			{	
					UNSIGNED32 templateCount, temp=0;
				//Get the number of properties associated.
				templateCount = json_array_size(jsonTemplateArray);	

				//Loop through the template array, get index and pass to Read
				for(temp = 0; temp < templateCount; temp++)
				{	
					//Get each template from json
					jsonTemplateData = json_array_get(jsonTemplateArray, temp);
						
					//Get the template ID from the jsonTemplateData
					getJSONObjectForKey(jsonTemplateData, _T("-ID"), &jsonTempObj);

					if(jsonTempObj != NULL)
					{
						templateName = json_string_value(jsonTempObj);
						
						//Convert to Unicode
						getUnicodeFromASCII(templateName, &unicodeTemplateName);

						ret = OSstrcmp((TCHAR *)unicodeTemplateName,interfaceName);
							if(ret == 0)
							{
								AddTemplateToHash((TCHAR *)unicodeTemplateName, jsonTemplateData, templateDb);
								*templateId = templateDb->templateCount;
							}


						
					}
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





ERROR_STATUS CreateNewTemplate(char ** newTemplate){
	
	TEMPLATE_DATABASE * templateDb = NULL;
	MODEL_CLASS_VARS *classVarPtr = NULL;


	UNSIGNED16 * unicodeTemplateData = NULL;
	UNSIGNED16 templateData = 0;
	TCHAR* templateCharData = NULL;
	UNSIGNED16 templateCountID = 0;
	void * dataTemplateNode = NULL;
	UNSIGNED16  templateCount, temp=0;
	SIGNED8 * asciitemplateData = NULL;
	
	
	json_t *root = json_object();
	json_t *rootTemplateElement = NULL;
	json_t *json_arr = json_array();

	APSHASHTBL *templateHashNodes = NULL;
	struct hashEntry_s *templateNode;
	char* s = NULL;

	classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);
	templateDb = classVarPtr->template_database;

	//getTemplate Version and adding it to jason structure.
	unicodeTemplateData = (UNSIGNED16 *)classVarPtr->template_Version;
	if(unicodeTemplateData){
		if(!getAsciiFromUnicode(unicodeTemplateData, &asciitemplateData)){
			if(asciitemplateData){
				json_object_set_new( root, "Version", json_string(asciitemplateData ) );
				asciitemplateData = NULL;
			}
			unicodeTemplateData = NULL;
		}
	}


	json_object_set_new( root, "Template", json_arr );

	templateCount = templateDb->templateCount;
	templateHashNodes = templateDb->templateHash;
	templateNode = templateHashNodes->nodes[0];

	while(templateNode){
		TEMPLATE_PROPERTY_ATTR_INFOLIST *propertyAttributeInfoList = NULL;
		//TEMPLATE_PROPERTY_ATTR_INFO  propertyAttributeInfo;
		TEMPLATE_SUBCOMPONENT_INFO_LIST* templateSubComponentInfoList = NULL;
		//TEMPLATE_SUBCOMPONENT_INFO templateSubComponentInfo;
		
		rootTemplateElement = json_object();
		templateCountID =  *(UNSIGNED16*)(void **)templateNode->data;
	//	GetTemplatePropertyInfo(templateCountID,7011,0,&attrinfo);
		
		if(!GetTemplateType(templateCountID, &templateData)){
			if(!getAsciiFromUnicode(&templateData, &asciitemplateData)){
				if(asciitemplateData){
					json_object_set_new( rootTemplateElement, "-type", json_integer( *asciitemplateData ) );
					asciitemplateData = NULL;
				}
				templateData = 0;
			}
		}
		if(!GetTemplateSubType(templateCountID, &templateData)){
			if(!getAsciiFromUnicode(&templateData, &asciitemplateData)){
				if(asciitemplateData){
					json_object_set_new( rootTemplateElement, "-subtype", json_integer( *asciitemplateData ) );
					asciitemplateData = NULL;
				}
				templateData = 0;
			}
		}
		if(!GetTemplateParent(templateCountID, &templateCharData)){
			unicodeTemplateData = (UNSIGNED16*)templateCharData;
			if(!getAsciiFromUnicode(unicodeTemplateData, &asciitemplateData)){
				if(asciitemplateData){
					json_object_set_new( rootTemplateElement, "-extends", json_string( asciitemplateData ) );
					asciitemplateData = NULL;
				}
				unicodeTemplateData = NULL;
			}
		}
		if(!GetTemplateDescription(templateCountID, &templateCharData)){
			unicodeTemplateData = (UNSIGNED16*)templateCharData;
			if(!getAsciiFromUnicode(unicodeTemplateData, &asciitemplateData)){
				if(asciitemplateData){
					json_object_set_new( rootTemplateElement, "-description", json_string( asciitemplateData ) );
					asciitemplateData = NULL;
				}
				unicodeTemplateData = NULL;
			}
		}
		if(!GetDictionaryName(templateCountID, &templateCharData)){
			unicodeTemplateData = (UNSIGNED16*)templateCharData;
			if(!getAsciiFromUnicode(unicodeTemplateData, &asciitemplateData)){
				if(asciitemplateData){
					json_object_set_new( rootTemplateElement, "-dictionary", json_string( asciitemplateData ) );
					asciitemplateData = NULL;
				}
				unicodeTemplateData = NULL;
			}
		}
		if(!GetTemplateID(templateCountID, &templateCharData)){
			unicodeTemplateData = (UNSIGNED16*)templateCharData;
			if(!getAsciiFromUnicode(unicodeTemplateData, &asciitemplateData)){
				if(asciitemplateData){
					json_object_set_new( rootTemplateElement, "-ID", json_string( asciitemplateData ) );
					asciitemplateData = NULL;
				}
				unicodeTemplateData = NULL;
			}
		}
		if(!GetTemplateName(templateCountID, &templateCharData)){
			unicodeTemplateData = (UNSIGNED16*)templateCharData;
			if(!getAsciiFromUnicode(unicodeTemplateData, &asciitemplateData)){
				if(asciitemplateData){
					json_object_set_new( rootTemplateElement, "-name", json_string( asciitemplateData ) );
					asciitemplateData = NULL;
				}
				unicodeTemplateData = NULL;
			}
		}
		if(!GetTemplatePresentValueAttributeId(templateCountID, &templateData)){
			if(!getAsciiFromUnicode(&templateData, &asciitemplateData)){
				if(asciitemplateData){
					
					if(*asciitemplateData >= 7000)
						json_object_set_new( rootTemplateElement, "-presentValueAttributeId", json_integer( *asciitemplateData ) );
					
						asciitemplateData = NULL;
				}
				templateData = 0;
			}
		}
		//Property Array
		if(!GetTemplateKeyPropertyAttributes(templateCountID,&propertyAttributeInfoList)){
			if(propertyAttributeInfoList){
				UNSIGNED16 count=0;
				json_t *json_arr_property = json_array();
				json_t *json_property = json_object();
				json_t *json_property_array_object = NULL;
				json_t *json_property_name_object = NULL;
				json_t *json_property_description_object = NULL;
				json_t *json_property_IPunits_object = NULL;
				json_t *json_property_SIunits_object = NULL;
				json_t *json_property_measurement_object = NULL;
				json_t *json_property_IP_Range_object = NULL;
				json_t *json_property_SI_Range_object = NULL;
				TEMPLATE_PROPERTY_ATTR_INFO  propertyAttributeInfo;
				json_object_set_new( rootTemplateElement, "-PropertyList", json_property );
				json_object_set_new( json_property, "-Property", json_arr_property );
				for(count=0;count < propertyAttributeInfoList->numtemplatePropertyInfoEntries;count++)
				{
					json_property_array_object = json_object();
					json_property_name_object = json_object();
					json_property_description_object = json_object();
					json_property_IPunits_object = json_object();
					json_property_SIunits_object = json_object();
					json_property_measurement_object = json_object();
					json_property_IP_Range_object = json_object();
					json_property_SI_Range_object = json_object();
					propertyAttributeInfo = propertyAttributeInfoList->propertyInfo[count];
					if(json_property_array_object){
						if(propertyAttributeInfo.attrID >= 7000){
							json_object_set_new( json_property_array_object, "-ID", json_integer( propertyAttributeInfo.attrID ) );
						}
						json_object_set_new( json_property_array_object, "-Required", json_integer( propertyAttributeInfo.required ) );
						json_object_set_new( json_property_array_object, "-DataType", json_integer( propertyAttributeInfo.dataType ) );
						if(propertyAttributeInfo.enumSet > 0){
							json_object_set_new( json_property_array_object, "-StringsetId", json_integer( propertyAttributeInfo.enumSet ) );
						}
						if(propertyAttributeInfo.redirectedVals && (propertyAttributeInfo.redirectedEnumSetProp >= 0)){
								json_object_set_new( json_property_array_object, "-StringsetProperty", json_integer( propertyAttributeInfo.redirectedEnumSetProp ) );
						}
						json_object_set_new( json_property_array_object, "-WritableFlag", json_integer( propertyAttributeInfo.attrWritable ) );
						json_object_set_new( json_property_array_object, "-PriorityFlag", json_integer( propertyAttributeInfo.attrPriority ) );
						if(propertyAttributeInfo.maxStringLength > 0){
							json_object_set_new( json_property_array_object, "-MaxStringLength", json_integer( propertyAttributeInfo.maxStringLength ) );
						}
						if(propertyAttributeInfo.dispPrec_IP > 0){
							json_object_set_new( json_property_array_object, "-IPDisplayPrecision", json_integer( propertyAttributeInfo.dispPrec_IP ) );
						}
						if(propertyAttributeInfo.dispPrec_SI > 0){
							json_object_set_new( json_property_array_object, "-SIDisplayPrecision", json_integer( propertyAttributeInfo.dispPrec_SI ) );
						}
						if(json_property_name_object){
							json_object_set_new( json_property_array_object, "-Name", json_property_name_object );
							json_object_set_new( json_property_name_object, "-setId", json_integer( propertyAttributeInfo.attrNameset ) );
							json_object_set_new( json_property_name_object, "-value", json_integer( propertyAttributeInfo.attrName ) );
						}
						if(json_property_description_object && (0 != propertyAttributeInfo.attrDescriptionset) && (0 != propertyAttributeInfo.attrDescription)){
							json_object_set_new( json_property_array_object, "-Description", json_property_description_object );
							json_object_set_new( json_property_description_object, "-setId", json_integer( propertyAttributeInfo.attrDescriptionset ) );
							json_object_set_new( json_property_description_object, "-value", json_integer( propertyAttributeInfo.attrDescription ) );
						}
						if(NO_UNITS == propertyAttributeInfo.units_IP){
							if(json_property_IPunits_object && (0 != propertyAttributeInfo.units_set) && (0 != propertyAttributeInfo.units_IP)){
								json_object_set_new( json_property_array_object, "-IPUnits", json_property_IPunits_object );
								json_object_set_new( json_property_IPunits_object, "-setId", json_integer( propertyAttributeInfo.units_set ) );
								json_object_set_new( json_property_IPunits_object, "-value", json_integer( propertyAttributeInfo.units_IP ) );
								if(propertyAttributeInfo.redirectedVals){
									json_object_set_new( json_property_IPunits_object, "-IPUnitsProperty", json_integer( propertyAttributeInfo.redirectedUnits_IP_Prop ) );
								}
							}
						}
						if(NO_UNITS == propertyAttributeInfo.units_SI){
							if(json_property_SIunits_object && (0 != propertyAttributeInfo.units_set) && (0 != propertyAttributeInfo.units_SI)){
								json_object_set_new( json_property_array_object, "-SIUnits", json_property_SIunits_object );
								json_object_set_new( json_property_SIunits_object, "-setId", json_integer( propertyAttributeInfo.units_set ) );
								json_object_set_new( json_property_SIunits_object, "-value", json_integer( propertyAttributeInfo.units_SI ) );
								if(propertyAttributeInfo.redirectedVals){
									json_object_set_new( json_property_SIunits_object, "-SIUnitsProperty", json_integer( propertyAttributeInfo.redirectedUnits_SI_Prop ) );
								}
							}
						}
						if(json_property_measurement_object && (0 !=propertyAttributeInfo.units_set) && (0 != propertyAttributeInfo.measurementType)){
							json_object_set_new( json_property_array_object, "-MeasurementType", json_property_description_object );
							json_object_set_new( json_property_description_object, "-setId", json_integer( propertyAttributeInfo.units_set ) );
							json_object_set_new( json_property_description_object, "-value", json_integer( propertyAttributeInfo.measurementType ) );
						}
						if(json_property_IP_Range_object && (0 != propertyAttributeInfo.min_IP) && (0 != propertyAttributeInfo.max_IP)){
							json_object_set_new( json_property_array_object, "-IPRange", json_property_IP_Range_object );
							json_object_set_new( json_property_IP_Range_object, "-minvalue", json_real( (FLOAT32)propertyAttributeInfo.min_IP ) );
							json_object_set_new( json_property_IP_Range_object, "-maxvalue", json_real( (FLOAT32)propertyAttributeInfo.max_IP ) );
							if(TRUE == propertyAttributeInfo.redirectedVals){
								json_object_set_new( json_property_IP_Range_object, "-minProperty", json_integer( propertyAttributeInfo.redirectedMin_IP_Prop ) );
								json_object_set_new( json_property_IP_Range_object, "-maxProperty", json_integer( propertyAttributeInfo.redirectedMax_IP_Prop ) );
							}
						}
						if(json_property_SI_Range_object && (0 != propertyAttributeInfo.min_SI) && (0 != propertyAttributeInfo.max_SI)){
							json_object_set_new( json_property_array_object, "-SIRange", json_property_SI_Range_object );
							json_object_set_new( json_property_SI_Range_object, "-minvalue", json_real( (FLOAT32)propertyAttributeInfo.min_SI ) );
							json_object_set_new( json_property_SI_Range_object, "-maxvalue", json_real( (FLOAT32)propertyAttributeInfo.max_SI ) );
							if(TRUE == propertyAttributeInfo.redirectedVals){
								json_object_set_new( json_property_SI_Range_object, "-minProperty", json_integer( propertyAttributeInfo.redirectedMin_SI_Prop ) );
								json_object_set_new( json_property_SI_Range_object, "-maxProperty", json_integer( propertyAttributeInfo.redirectedMax_SI_Prop ) );
							}
						}
						json_array_append_new(json_arr_property, json_property_array_object );
					}
				}
			}
		}
		//subcomponent list
		if(!GetTemplateSubComponentList(templateCountID,&templateSubComponentInfoList)){
			if(templateSubComponentInfoList){
				UNSIGNED16 count=0;
				json_t *json_arr_component = json_array();
				json_t *json_component_list = json_object();
				json_t *json_component_object = NULL;
				json_t *json_component_label_object = NULL;
				TEMPLATE_SUBCOMPONENT_INFO templateSubComponentInfo;
				json_object_set_new( rootTemplateElement, "-SubComponentList", json_component_list );
				json_object_set_new( json_component_list, "-SubComponent", json_arr_component );
				for(count=0;count < templateSubComponentInfoList->numSubComponentInfoEntries;count++)
				{
					json_component_object = json_object();
					json_component_label_object = json_object();
					templateSubComponentInfo = templateSubComponentInfoList->subComponentInfo[count];
					if(json_component_object){

						if(templateSubComponentInfo.subComponentId){
							unicodeTemplateData = (UNSIGNED16*)templateSubComponentInfo.subComponentId;
							if(!getAsciiFromUnicode(unicodeTemplateData, &asciitemplateData)){
								if(asciitemplateData){
									json_object_set_new( json_component_object, "-Name", json_string( asciitemplateData ) );
									asciitemplateData = NULL;
								}
								unicodeTemplateData = NULL;
							}
						}

						json_object_set_new( json_component_object, "-Required", json_integer( templateSubComponentInfo.subComponentRequired ) );

						if(json_component_label_object && (0 != templateSubComponentInfo.subComponentSetId) && (0 != templateSubComponentInfo.subComponentLabelValue)){
							json_object_set_new( json_component_object, "-label", json_component_label_object );
							json_object_set_new( json_component_label_object, "-setId", json_integer( templateSubComponentInfo.subComponentSetId ) );
							json_object_set_new( json_component_label_object, "-value", json_integer( templateSubComponentInfo.subComponentLabelValue ) );
						}
						if(templateSubComponentInfo.templateId){
							unicodeTemplateData = (UNSIGNED16*)templateSubComponentInfo.templateId;
							if(!getAsciiFromUnicode(unicodeTemplateData, &asciitemplateData)){
								if(asciitemplateData){
									json_object_set_new( json_component_object, "-TemplateID", json_string( asciitemplateData ) );
									asciitemplateData = NULL;
								}
								unicodeTemplateData = NULL;
							}
						}

						json_array_append_new(json_arr_component, json_component_object );
					}
				}
			}
		}
		
		json_array_append_new(json_arr, rootTemplateElement );
		templateNode = templateNode->next;
	}


	s = json_dumps(root, 0);
	*newTemplate=s;
	json_decref(root);
	
	if(templateDb == NULL)
		return ERROR_RESPONSE;

	
	
	return OK;
}

