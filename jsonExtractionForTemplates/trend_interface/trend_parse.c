/*------------------------------------------------------------------------------

Module:   Trend Template Interface

Purpose:  This should be responsible to parse the JSON trend template. Add the content of JSON into the hash.

Filename: trend_parse.c

Inputs:   Hash Reference to insert the JSON data
Interface Name to parse the JSON file
templateKey which acts as a key for the hash insert function. The data will be gathered from the JSON file.

Outputs:  ERROR_STATUS returned if on any issue.

------------------------------------------------------------------------------*/
#include "trend_api_private.h"
#include <uniStr.h>
#include <unit.h>

ERROR_STATUS AddTrendTemplateToHash(UNSIGNED16 * interfaceName, json_t * jsonTemplate, TREND_DATABASE * templateDb)
{
    ERROR_STATUS status = OK;
    UNSIGNED16 * templateNumber = NULL;


    if(templateDb != NULL)
    {	

        templateDb->trendCount++;

        //Acquire memory to store this number in hash
        templateNumber = (UNSIGNED16 *)OSacquire(sizeof(UNSIGNED16));
        OSmemset(templateNumber, 0, sizeof(UNSIGNED16));

        *templateNumber = templateDb->trendCount;

        status = hashtbl_insert(templateDb->trendHash, interfaceName, templateNumber, STR_STORE(OSstrlen(interfaceName)));

        if(!status)
        {
            if(trendTemplateParse(templateDb->trendStructureHash, jsonTemplate, templateDb->trendCount))
            {
                //Template Parse resulted in error status.
                //Remove the previously added hash entry.
                hashtbl_remove(templateDb->trendHash, interfaceName, STR_STORE(OSstrlen(interfaceName)));
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




ERROR_STATUS trendTemplateParse(APSHASHTBL * hashTable, json_t * jsonTemplate, UNSIGNED16 templateKey)
{
    //Declare fields 
    TREND_TEMPLATE_ENTRY * templateEntry = NULL;
    APSHASHTBL * attributeHashInfo = NULL;
 //   APSHASHTBL * subcomponentHashInfo = NULL;
    TREND_TEMPLATE_PROPERTY_ATTR_INFO * propertyAttributeInfo = NULL;
//    TREND_TEMPLATE_SUBCOMPONENT_INFO * subcomponentInfo = NULL;
    ERROR_STATUS status = OK;

    //json Fields
    json_t *jsonPropertyArray;
  //  json_t *jsonSubComponentArray;
    json_t *jsonPropertyData;
  //  json_t * jsonTempObj;

    const SIGNED8 * propertyKey;
    json_t *propertyValue;

    const SIGNED8 * templKey;
    UNSIGNED16 unicodetemplKey[200] = {0};
    UNSIGNED16 * pwc = NULL;

    json_t *templValue;

    void *iter;
    void *propertyiter;
//    void *subcomponentiter;

    const SIGNED8 * stringKey;

    UNSIGNED32 propertyCount;
    UNSIGNED32 temp;

    if(jsonTemplate != NULL)
    {
        //Create attribute and subcomponent hash
        if(!(attributeHashInfo=hashtbl_create(TREND_PROPERTY_DB_ENTRY_GROW_SIZE, HASH_TYPE_INT))) {
            return HASH_CREATE_ERROR;
        }

     
        //Allocate memory for template entry
        templateEntry = (TREND_TEMPLATE_ENTRY *)OSacquire(sizeof(TREND_TEMPLATE_ENTRY));

        //Iterate through template properties
        iter = json_object_iter(jsonTemplate);

        while(iter)
        {
            templKey = json_object_iter_key(iter);
            OSmemset(unicodetemplKey, 0, sizeof(unicodetemplKey));

            uniAsciiToUnicode(templKey, unicodetemplKey);

            templValue = json_object_iter_value(iter);
           if(!OSstrcmp(unicodetemplKey, _T("-extends")))
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
                        templateEntry->trendExtensionID = pwc;
                    }
                    else
                    {
                        //OSTrace(_T("Error in converting ASCII to Unicode.."));   trendExtensionID
                    }
                }

            }
			else if(!OSstrcmp(unicodetemplKey, _T("-TemplateID")))
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
                        //OSTrace(_T("Error in converting ASCII to Unicode.."));   trendExtensionID
                    }
                }

            }
            else if(!OSstrcmp(unicodetemplKey, _T("-TrendCreationPropertyList")))
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
                        propertyAttributeInfo = (TREND_TEMPLATE_PROPERTY_ATTR_INFO *)OSacquire(sizeof(TREND_TEMPLATE_PROPERTY_ATTR_INFO));

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
        

            /* use key and value to iterate the next entry*/
            iter = json_object_iter_next(jsonTemplate, iter);

        }

        //Add property info to template structure
        templateEntry->trendCreationAttrInfo = attributeHashInfo;

        //Add component info to template structure. 
      //  templateEntry->trendTemplateSubComponentInfo = subcomponentHashInfo;

        status = hashtbl_insert(hashTable, &templateKey, templateEntry, sizeof(templateKey));
        if(status != OK)
            return status;

    }

    return OK;
}


