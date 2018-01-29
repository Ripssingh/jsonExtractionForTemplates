/*------------------------------------------------------------------------------

Module:   Template Interface

Purpose:  This should be responsible to parse the JSON template. Add the content of JSON into the hash.

Filename: template_parse.c

Inputs:   Hash Reference to insert the JSON data
Interface Name to parse the JSON file
templateKey which acts as a key for the hash insert function. The data will be gathered from the JSON file.

Outputs:  ERROR_STATUS returned if on any issue.
------------------------------------------------------------------------------*/
#include <template_api.h>
#include "template_api_private.h"
#include <uniStr.h>
#include <unit.h>

ERROR_STATUS getUnicodeFromASCII(const SIGNED8 * source, UNSIGNED16 ** destination)
{
    ERROR_STATUS status = OK;
    UNSIGNED16 unicodeData[200] = {0};
    UNSIGNED32 temp = 0;
    UNSIGNED16 * pwc = NULL;

    //Convert stringkey to unicode
    if(!uniAsciiToUnicode(source, unicodeData))
    {
        //Get the number of bytes associated so we can allocate them
        temp = STR_STORE(OSstrlen(unicodeData));

        //Allocate memory to hold data
        pwc  = (UNSIGNED16 *)OSacquire( sizeof( UNSIGNED16 ) * temp);

        if(pwc)
        {

            uniStrCpy(pwc, unicodeData);

            if(pwc != NULL)
            {
                *destination = pwc;
            }
        }
        else
        {
            status = NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        status = ERROR_RESPONSE;
    }

    return status;
}


ERROR_STATUS getAsciiFromUnicode(UNSIGNED16 * source, SIGNED8 ** destination)
{
    ERROR_STATUS status = OK;
    SIGNED8 AsciiData[200] = {0};
    UNSIGNED16 temp = 0;
    SIGNED8 * pwc = NULL;

    //Convert stringkey to unicode
    if(!uniToAscii(source, AsciiData))
    {
        //Get the number of bytes associated so we can allocate them
       temp = asciiStrlen(AsciiData);

        //Allocate memory to hold data
       pwc  = (SIGNED8 *)OSacquire( sizeof( SIGNED8 ) * (temp));
		
        if(pwc)
        {

            //uniStrCpy(pwc, AsciiData);
			OSmemset(pwc, 0, temp);
			OSmemcpy(pwc, AsciiData, temp);
            if(pwc != NULL)
            {
                *destination = pwc;
            }
        }
        else
        {
            status = NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        status = ERROR_RESPONSE;
    }
    return status;
}

UNSIGNED16 asciiStrlen (const SIGNED8 * asciiStr)
{
        const SIGNED8 *tempStr = asciiStr;
		UNSIGNED16 i = 0;
		while( *tempStr++ ){
			i++;	
		}

    return( i + 1);
}

void getJSONObjectForKey(json_t * sourceObject, UNSIGNED16 * key, json_t ** destjsonObject)
{
    json_t * jsonObj;
    SIGNED8 buffer[200] = {0};

    //Convert Unicode string to ASCII
    uniToAscii(key, buffer);

    if(buffer)
    {
        jsonObj = json_object_get(sourceObject, buffer);
        *destjsonObject = jsonObj;
    }
    else
    {
        *destjsonObject = NULL;
    }
}

ERROR_STATUS parseJSONString(SIGNED8 * buffer, json_t ** jsonObject)
{
    json_t *json;
    json_error_t error;
    ERROR_STATUS status = OK;

    json = json_loads(buffer, 0, &error);	 

    if(json != NULL)
    {
        *jsonObject = json;
        status = OK;
    }
    else
    {
        status = ERROR_RESPONSE;
    }
    return status;
}

ERROR_STATUS AddTemplateToHash(UNSIGNED16 * interfaceName, json_t * jsonTemplate, TEMPLATE_DATABASE * templateDb)
{
    ERROR_STATUS status = OK;
    UNSIGNED16 * templateNumber = NULL;


    if(templateDb != NULL)
    {	

        //Template is not added into the hash. Perform the below steps.
        //1. Increment the counter Id. 
        //2. Add to template Hash - Template Name as the key, Counter Id as the value. This value will be used as a
        //reference for all further get operations by the equipment model.
        //3. Parse the template and add to template structure hash. 

        templateDb->templateCount++;

        //Acquire memory to store this number in hash
        templateNumber = (UNSIGNED16 *)OSacquire(sizeof(UNSIGNED16));
        OSmemset(templateNumber, 0, sizeof(UNSIGNED16));

        *templateNumber = templateDb->templateCount;

        status = hashtbl_insert(templateDb->templateHash, interfaceName, templateNumber, STR_STORE(OSstrlen(interfaceName)));

        if(!status)
        {
            if(templateParse(templateDb->templateStructureHash, jsonTemplate, templateDb->templateCount))
            {
                //Template Parse resulted in error status.
                //Remove the previously added hash entry.
                hashtbl_remove(templateDb->templateHash, interfaceName, STR_STORE(OSstrlen(interfaceName)));
                return TEMPLATE_PARSE_ERROR;	
            }
        }
        else
        {
            return status;
        }
    }

    return status;

}




ERROR_STATUS templateParse(APSHASHTBL * hashTable, json_t * jsonTemplate, UNSIGNED16 templateKey)
{
    //Declare fields 
    TEMPLATE_ENTRY * templateEntry = NULL;
    APSHASHTBL * attributeHashInfo = NULL;
    APSHASHTBL * subcomponentHashInfo = NULL;
    TEMPLATE_PROPERTY_ATTR_INFO * propertyAttributeInfo = NULL;
    TEMPLATE_SUBCOMPONENT_INFO * subcomponentInfo = NULL;
    ERROR_STATUS status = OK;

    //json Fields
    json_t *jsonPropertyArray;
    json_t *jsonSubComponentArray;
    json_t *jsonPropertyData;
    json_t * jsonTempObj;

    const SIGNED8 * propertyKey;
    json_t *propertyValue;

    const SIGNED8 * templKey;
    UNSIGNED16 unicodetemplKey[200] = {0};
    UNSIGNED16 * pwc = NULL;

    json_t *templValue;

    void *iter;
    void *propertyiter;
    void *subcomponentiter;

    const SIGNED8 * stringKey;

    UNSIGNED32 propertyCount;
    UNSIGNED32 temp;

    if(jsonTemplate != NULL)
    {
        //Create attribute and subcomponent hash
        if(!(attributeHashInfo=hashtbl_create(TEMPLATE_PROPERTY_DB_ENTRY_GROW_SIZE, HASH_TYPE_INT))) {
            return HASH_CREATE_ERROR;
        }

        //Hash List to store the template Id and its corresponding reference.
        if(!(subcomponentHashInfo=hashtbl_create(TEMPLATE_COMPONENT_DB_ENTRY_GROW_SIZE, HASH_TYPE_STR))) {
            return HASH_CREATE_ERROR;			
        }

        //Allocate memory for template entry
        templateEntry = (TEMPLATE_ENTRY *)OSacquire(sizeof(TEMPLATE_ENTRY));

        //Iterate through template properties
        iter = json_object_iter(jsonTemplate);

        while(iter)
        {
            templKey = json_object_iter_key(iter);
            OSmemset(unicodetemplKey, 0, sizeof(unicodetemplKey));

            uniAsciiToUnicode(templKey, unicodetemplKey);

            templValue = json_object_iter_value(iter);

            if(!OSstrcmp(unicodetemplKey, _T("-type")))
            {
                templateEntry->type = (UNSIGNED16)json_integer_value(templValue);
            }
            else if(!OSstrcmp(unicodetemplKey, _T("-subtype")))
            {
                templateEntry->subType = (UNSIGNED16)json_integer_value(templValue);
            }
            else if(!OSstrcmp(unicodetemplKey, _T("-presentValueAttributeId")))
            {
                templateEntry->presentValueAttrId = (UNSIGNED16)json_integer_value(templValue);
            }
            else if(!OSstrcmp(unicodetemplKey, _T("-extends")))
            {
                stringKey = json_string_value(templValue);

                if(stringKey != NULL)
                {
                    status = getUnicodeFromASCII(stringKey, &pwc);
                    if(!status)
                    {
                        templateEntry->templateParent = pwc;
                    }
                    else
                    {
                        //OSTrace(_T("Error in converting ASCII to Unicode.."));
                    }
                }
            }
            else if(!OSstrcmp(unicodetemplKey, _T("-dictionary")))
            {
                stringKey = json_string_value(templValue);

                if(stringKey != NULL)
                {
                    status = getUnicodeFromASCII(stringKey, &pwc);
                    if(!status)
                    {
                        templateEntry->dictionaryName = pwc;
                    }
                    else
                    {
                        //OSTrace(_T("Error in converting ASCII to Unicode.."));
                    }
                }

            }
            else if(!OSstrcmp(unicodetemplKey, _T("-name")))
            {
                stringKey = json_string_value(templValue);

                if(stringKey != NULL)
                {
                    status = getUnicodeFromASCII(stringKey, &pwc);
                    if(!status)
                    {
                        templateEntry->templateName = pwc;
                    }
                    else
                    {
                        //OSTrace(_T("Error in converting ASCII to Unicode.."));
                    }
                }
            }
            else if(!OSstrcmp(unicodetemplKey, _T("-description")))
            {
                stringKey = json_string_value(templValue);

                if(stringKey != NULL)
                {
                    status = getUnicodeFromASCII(stringKey, &pwc);
                    if(!status)
                    {
                        templateEntry->templateDescription = pwc;
                    }
                    else
                    {
                        //OSTrace(_T("Error in converting ASCII to Unicode.."));
                    }
                }

            }
            else if(!OSstrcmp(unicodetemplKey, _T("-ID")))
            {
                stringKey = json_string_value(templValue);

                if(stringKey != NULL)
                {
                    status = getUnicodeFromASCII(stringKey, &pwc);
                    if(!status)
                    {
                        templateEntry->templateID = pwc;
                    }
                    else
                    {
                        //OSTrace(_T("Error in converting ASCII to Unicode.."));
                    }
                }

            }
            else if(!OSstrcmp(unicodetemplKey, _T("-PropertyList")))
            {
                getJSONObjectForKey(templValue, _T("-Property"), &jsonPropertyArray);

                if(jsonPropertyArray != NULL && json_is_array(jsonPropertyArray) == JSON_ARRAY)
                {
                    //Get the number of properties associated.
                    propertyCount = json_array_size(jsonPropertyArray);	

                    //Loop through the array
                    for(temp = 0; temp < propertyCount; temp++)
                    {
                        //Allocate space for property structure
                        propertyAttributeInfo = (TEMPLATE_PROPERTY_ATTR_INFO *)OSacquire(sizeof(TEMPLATE_PROPERTY_ATTR_INFO));

                        //initialize the enumSet to be FALSETRUE_ENUM_SET as default - to be used for bool and enum types 
                        propertyAttributeInfo->enumSet = FALSETRUE_ENUM_SET;

                        //Get each index object
                        jsonPropertyData = json_array_get(jsonPropertyArray, temp);
                        
                        //Iterate through each of the properties
                        propertyiter = json_object_iter(jsonPropertyData);
                        while(propertyiter)
                        {
                            propertyKey = json_object_iter_key(propertyiter);

                            OSmemset(unicodetemplKey, 0, sizeof(unicodetemplKey));

                            uniAsciiToUnicode(propertyKey, unicodetemplKey);

                            propertyValue = json_object_iter_value(propertyiter);

                            //Add to property Entry based on the data..
                            if(!OSstrcmp(unicodetemplKey, _T("-ID")))
                            {
                                propertyAttributeInfo->attrID = (UNSIGNED16)json_integer_value(propertyValue);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-Required")))
                            {
                                propertyAttributeInfo->required = (UNSIGNED8)json_integer_value(propertyValue);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-DataType")))
                            {
                                propertyAttributeInfo->dataType = (UNSIGNED8)json_integer_value(propertyValue);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-StringsetId")))
                            {
                                propertyAttributeInfo->enumSet = (UNSIGNED16)json_integer_value(propertyValue);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-StringsetProperty")))
                            {
                                propertyAttributeInfo->redirectedEnumSetProp = (UNSIGNED16)json_integer_value(propertyValue);
                                propertyAttributeInfo->redirectedVals = TRUE;
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-WritableFlag")))
                            {
                                propertyAttributeInfo->attrWritable = (UNSIGNED8)json_integer_value(propertyValue);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-PriorityFlag")))
                            {
                                propertyAttributeInfo->attrPriority = (UNSIGNED8)json_integer_value(propertyValue);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-MaxStringLength")))
                            {
                                propertyAttributeInfo->maxStringLength = (UNSIGNED8)json_integer_value(propertyValue);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-IPDisplayPrecision")))
                            {
                                propertyAttributeInfo->dispPrec_IP = (UNSIGNED16)json_integer_value(propertyValue);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-SIDisplayPrecision")))
                            {
                                propertyAttributeInfo->dispPrec_SI = (UNSIGNED16)json_integer_value(propertyValue);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-Name")))
                            {
                                //Get setId for Name
                                getJSONObjectForKey(propertyValue,_T("-setId"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->attrNameset = (UNSIGNED16)json_integer_value(jsonTempObj);								

                                //Get Value for Name
                                getJSONObjectForKey(propertyValue,_T("-value"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->attrName = (UNSIGNED16)json_integer_value(jsonTempObj);

                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-Description")))
                            {
                                getJSONObjectForKey(propertyValue,_T("-setId"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->attrDescriptionset = (UNSIGNED16)json_integer_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-value"), &jsonTempObj);
                                if(jsonTempObj != NULL)	
                                    propertyAttributeInfo->attrDescription = (UNSIGNED16)json_integer_value(jsonTempObj);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-IPUnits")))
                            {
                                //Default units set to NO_UNITS
                                propertyAttributeInfo->units_IP = NO_UNITS;
                                getJSONObjectForKey(propertyValue,_T("-setId"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->units_set = (UNSIGNED16)json_integer_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-value"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->units_IP = (UNSIGNED16)json_integer_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-IPUnitsProperty"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                {
                                    propertyAttributeInfo->redirectedUnits_IP_Prop = (UNSIGNED16)json_integer_value(jsonTempObj);
                                    propertyAttributeInfo->redirectedVals = TRUE;
                                }

                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-SIUnits")))
                            {
                                //Default units set to NO_UNITS
                                propertyAttributeInfo->units_SI = NO_UNITS;

                                getJSONObjectForKey(propertyValue,_T("-setId"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->units_set = (UNSIGNED16)json_integer_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-value"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->units_SI = (UNSIGNED16)json_integer_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-SIUnitsProperty"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                {
                                    propertyAttributeInfo->redirectedUnits_SI_Prop = (UNSIGNED16)json_integer_value(jsonTempObj);
                                    propertyAttributeInfo->redirectedVals = TRUE;
                                }
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-MeasurementType")))
                            {
                                getJSONObjectForKey(propertyValue,_T("-setId"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->units_set = (UNSIGNED16)json_integer_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-value"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->measurementType = (UNSIGNED16)json_integer_value(jsonTempObj);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-IPRange")))
                            {
#ifdef USE_DOUBLE
                                getJSONObjectForKey(propertyValue,_T("-minvalue"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->min_IP = (FLOAT64)json_number_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-maxvalue"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->max_IP = (FLOAT64)json_number_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-minProperty"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                {
                                    propertyAttributeInfo->redirectedMin_IP_Prop = (UNSIGNED16)json_integer_value(jsonTempObj);
                                    propertyAttributeInfo->redirectedVals = TRUE;
                                }

                                getJSONObjectForKey(propertyValue,_T("-maxProperty"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                {
                                    propertyAttributeInfo->redirectedMax_IP_Prop = (UNSIGNED16)json_integer_value(jsonTempObj);
                                    propertyAttributeInfo->redirectedVals = TRUE;
                                }
#else
                                getJSONObjectForKey(propertyValue,_T("-minvalue"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->min_IP = (FLOAT32)json_number_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-maxvalue"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->max_IP = (FLOAT32)json_number_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-minProperty"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                {
                                    propertyAttributeInfo->redirectedMin_IP_Prop = (UNSIGNED16)json_integer_value(jsonTempObj);
                                    propertyAttributeInfo->redirectedVals = TRUE;
                                }

                                getJSONObjectForKey(propertyValue,_T("-maxProperty"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                {
                                    propertyAttributeInfo->redirectedMax_IP_Prop = (UNSIGNED16)json_integer_value(jsonTempObj);
                                    propertyAttributeInfo->redirectedVals = TRUE;
                                }
#endif
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-SIRange")))
                            {
#ifdef USE_DOUBLE
                                getJSONObjectForKey(propertyValue,_T("-minvalue"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->min_SI = (FLOAT64)json_number_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-maxvalue"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->max_SI = (FLOAT64)json_number_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-minProperty"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                {
                                    propertyAttributeInfo->redirectedMin_SI_Prop = (UNSIGNED16)json_integer_value(jsonTempObj);
                                    propertyAttributeInfo->redirectedVals = TRUE;
                                }

                                getJSONObjectForKey(propertyValue,_T("-maxProperty"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                {
                                    propertyAttributeInfo->redirectedMax_SI_Prop = (UNSIGNED16)json_integer_value(jsonTempObj);
                                    propertyAttributeInfo->redirectedVals = TRUE;
                                }
#else
                                getJSONObjectForKey(propertyValue,_T("-minvalue"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->min_SI = (FLOAT32)json_number_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-maxvalue"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                    propertyAttributeInfo->max_SI = (FLOAT32)json_number_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-minProperty"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                {
                                    propertyAttributeInfo->redirectedMin_SI_Prop = (UNSIGNED16)json_integer_value(jsonTempObj);
                                    propertyAttributeInfo->redirectedVals = TRUE;
                                }

                                getJSONObjectForKey(propertyValue,_T("-maxProperty"), &jsonTempObj);
                                if(jsonTempObj != NULL)
                                {
                                    propertyAttributeInfo->redirectedMax_SI_Prop = (UNSIGNED16)json_integer_value(jsonTempObj);
                                    propertyAttributeInfo->redirectedVals = TRUE;
                                }
#endif
                            }


                            /* use key and value ... */
                            propertyiter = json_object_iter_next(jsonPropertyData, propertyiter);
                        }

                        //Add to property hash
                        status = hashtbl_insert(attributeHashInfo, &propertyAttributeInfo->attrID, propertyAttributeInfo, sizeof(propertyAttributeInfo->attrID));
                        if (status != OK)
                            return status;

                    }					
                }
            }
            else if(!OSstrcmp(unicodetemplKey, _T("-SubComponentList")))
            {
                getJSONObjectForKey(templValue,_T("-SubComponent"), &jsonSubComponentArray);
                if(jsonSubComponentArray != NULL && json_is_array(jsonSubComponentArray) == JSON_ARRAY)
                {	
                    //Get the number of properties associated.
                    propertyCount = json_array_size(jsonSubComponentArray);	


                    //Loop through the array
                    for(temp = 0; temp < propertyCount; temp++)
                    {
                        //Allocate space for property structure
                        subcomponentInfo = (TEMPLATE_SUBCOMPONENT_INFO *)OSacquire(sizeof(TEMPLATE_SUBCOMPONENT_INFO));

                        //Get each index object
                        jsonPropertyData = json_array_get(jsonSubComponentArray, temp);

                        //Iterate through each of the properties
                        subcomponentiter = json_object_iter(jsonPropertyData);
                        while(subcomponentiter)
                        {
                            propertyKey = json_object_iter_key(subcomponentiter);

                            OSmemset(unicodetemplKey, 0, sizeof(unicodetemplKey));

                            uniAsciiToUnicode(propertyKey, unicodetemplKey);

                            propertyValue = json_object_iter_value(subcomponentiter);

                            //Add to property Entry based on the data..
                            if(!OSstrcmp(unicodetemplKey, _T("-Name")))
                            {
                                stringKey = json_string_value(propertyValue);
                                if(stringKey != NULL)
                                {
                                    status = getUnicodeFromASCII(stringKey, &pwc);
                                    if(!status)
                                    {
                                        subcomponentInfo->subComponentId = pwc;
                                    }
                                    else
                                    {
                                        //OSTrace(_T("Error in converting ASCII to Unicode.."));
                                    }
                                }
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-Required")))
                            {
                                subcomponentInfo->subComponentRequired = (UNSIGNED8)json_integer_value(propertyValue);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-label")))
                            {	
                                getJSONObjectForKey(propertyValue,_T("-setId"), &jsonTempObj);
                                if(jsonTempObj != NULL)									
                                    subcomponentInfo->subComponentSetId = (UNSIGNED16)json_integer_value(jsonTempObj);

                                getJSONObjectForKey(propertyValue,_T("-value"), &jsonTempObj);
                                if(jsonTempObj != NULL)									
                                    subcomponentInfo->subComponentLabelValue = (UNSIGNED16)json_integer_value(jsonTempObj);
                            }
                            else if(!OSstrcmp(unicodetemplKey, _T("-TemplateID")))
                            {
                                stringKey = json_string_value(propertyValue);
                                if(stringKey != NULL)
                                {
                                    status = getUnicodeFromASCII(stringKey, &pwc);
                                    if(!status)
                                    {
                                        subcomponentInfo->templateId = pwc;
                                    }
                                    else
                                    {
                                        //OSTrace(_T("Error in converting ASCII to Unicode.."));
                                    }
                                }
                            }

                            /* use key and value ... */
                            subcomponentiter = json_object_iter_next(jsonPropertyData, subcomponentiter);

                        }

                        //Add to subcomponent info hash
                        if(subcomponentInfo->subComponentId != NULL)
                            status = hashtbl_insert(subcomponentHashInfo, subcomponentInfo->subComponentId, subcomponentInfo, STR_STORE(OSstrlen(subcomponentInfo->subComponentId)));
                        else
                            status = TEMPLATE_PARSE_ERROR;
                        
                        if(status != OK)
                            return status;

                    }
                }


            }


            /* use key and value to iterate the next entry*/
            iter = json_object_iter_next(jsonTemplate, iter);

        }

        //Add property info to template structure
        templateEntry->templateAttrInfo = attributeHashInfo;

        //Add component info to template structure. 
        templateEntry->templateSubComponentInfo = subcomponentHashInfo;

        status = hashtbl_insert(hashTable, &templateKey, templateEntry, sizeof(templateKey));
        if(status != OK)
            return status;

    }

    return OK;
}


