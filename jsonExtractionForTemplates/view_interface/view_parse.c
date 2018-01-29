/*------------------------------------------------------------------------------

Module:   View Interface

Purpose:  This should be responsible to parse the JSON view file. Add the content of JSON into the hash.
Also has helper methods to pull the JSON content into hash

Filename: view_parse.c

Inputs:   Hash Reference to insert the JSON data
Interface Name to parse the JSON file
templateKey which acts as a key for the hash insert function. The data will be gathered from the JSON file.

Outputs:  ERROR_STATUS returned if on any issue.
------------------------------------------------------------------------------*/
#include <view_api.h>
#include <view_api_private.h>
#include <uniStr.h>
#include <unit.h>
#include <apsserv.h>


/*------------------------------------------------------------------------------
Module:   GetTemplateFromJSONView method

Purpose:  This function is responsible to return templateId that is present in JSON file

Inputs:   jsonObject - Reference to jsonObject of view JSON

Outputs:  templateId - Pointer to templateId
------------------------------------------------------------------------------*/
ERROR_STATUS GetTemplateFromJSONView(json_t *jsonObject, TCHAR ** templateId)
{
    ERROR_STATUS status = OK;
    const SIGNED8* templString = NULL;
    json_t * jsonTempObj  = NULL;
    UNSIGNED16 * pwc = NULL;

    //Get the template info
    getJSONObjectForKey(jsonObject,_T("templateId"), &jsonTempObj);
    if(jsonTempObj == NULL)
        return ERROR_RESPONSE;

    templString = json_string_value(jsonTempObj);

    if(templString != NULL)
    {
        status = getUnicodeFromASCII(templString, &pwc);
        if(!status)
        {
            *templateId = pwc;
        }
        else
        {
            //OSTrace(_T("Error in converting ASCII to Unicode.."));
        }
    }


    return status;
}

/*------------------------------------------------------------------------------
Module:   ParseViewJSON method

Purpose:  This function is responsible to parse the view buffer and create jsonObject

Inputs:   buffer - Reference to string buffer holding JSON view

Outputs:  jsonObject - Pointer to jsonObject. Return OK if parsing is successful
------------------------------------------------------------------------------*/
ERROR_STATUS ParseViewJSON(SIGNED8 * buffer, json_t ** jsonObject)
{
    ERROR_STATUS status = OK;
    json_t * jsonObj; 

    status = parseJSONString(buffer, &jsonObj);

    *jsonObject = jsonObj;

    //Buffer is read into json Object. No need to keep the buffer.
    OSrelease(buffer);
    return status;
}

/*------------------------------------------------------------------------------
Module:   View Interface

Purpose:  This should be responsible to add menu group pointer to menu Group

Method:   AddMenuGroupPointerToMenuGroup

Inputs:   menuGroup: Reference to menuGroup to which a new group to be added.
menuElementIndex - MenuElementIndex to insert this menuElement
jsonGroupObj: Pointer to json Object - Need this to fetch label enum and label set
grpHandle: Holds the current group handle Ex: 1001
itemReference: Reference to the current item Reference (Need this to pull oid from itemReference for value Elements)	  

Outputs:  newMenuGroup - Returns new Menu Group pointer for a given menu group.
ERROR_STATUS returned if on any issue.
------------------------------------------------------------------------------*/
ERROR_STATUS AddMenuGroupPointerToMenuGroup(MenuGroup * menuGroup, json_t * jsonGroupObj, UNSIGNED16 menuElementIndex, UNSIGNED16 * grpHandle, TCHAR * itemReference, MenuGroup ** newMenuGroup)
{
    ERROR_STATUS status = OK;
    MenuGroupPointer * menuGrpPointer = NULL;
    json_t *jsonlabelObject, *jsonshortLabelObject, *jsonTempObj, *jsonGrpIdObj, *jsonGrpElementArray, *jsonGrpPresenceIndicator, *jsonTypeMinor;
    UNSIGNED16 grpElementsCount, groupSize = 0;
    MenuGroup *newmenuGroupElement = NULL;


    //Step - 1 : Read label element
    getJSONObjectForKey(jsonGroupObj, _T("label"), &jsonlabelObject);
    if(jsonlabelObject == NULL)
        return ERROR_RESPONSE;

    //Get set
    getJSONObjectForKey(jsonlabelObject,_T("set"), &jsonTempObj);
    if(jsonTempObj != NULL)
        menuGroup->groupElements[menuElementIndex].Group.LabelEnumSet = (UNSIGNED16)json_integer_value(jsonTempObj);

    //Get the enum id
    jsonTempObj = NULL;
    getJSONObjectForKey(jsonlabelObject,_T("id"), &jsonTempObj);
    if(jsonTempObj != NULL)
        menuGroup->groupElements[menuElementIndex].Group.LabelEnum = (UNSIGNED16)json_integer_value(jsonTempObj);

    //Check for shortLabel if available.
    getJSONObjectForKey(jsonGroupObj, _T("shortLabel"), &jsonshortLabelObject);
    if(jsonshortLabelObject != NULL)
    {
        //Get short label set 
        jsonTempObj = NULL;
        getJSONObjectForKey(jsonshortLabelObject,_T("set"), &jsonTempObj);
        if(jsonTempObj != NULL)
            menuGroup->groupElements[menuElementIndex].Group.ShortLabelEnumSet = (UNSIGNED16)json_integer_value(jsonTempObj);

        //Get the short enum id
        jsonTempObj = NULL;
        getJSONObjectForKey(jsonshortLabelObject,_T("id"), &jsonTempObj);
        if(jsonTempObj != NULL)
            menuGroup->groupElements[menuElementIndex].Group.ShortLabelEnum = (UNSIGNED16)json_integer_value(jsonTempObj);
    }
    else
    {
        menuGroup->groupElements[menuElementIndex].Group.ShortLabelEnumSet = (UNSIGNED16)NONE_FFFF;
        menuGroup->groupElements[menuElementIndex].Group.ShortLabelEnum = (UNSIGNED16)NONE_FFFF;
    }
    

    //Step - 2: Read groupId
    getJSONObjectForKey(jsonGroupObj, _T("id"), &jsonGrpIdObj);

    //Get the groupID if it exists, 
    if(jsonGrpIdObj != NULL)
    {	
        menuGroup->groupElements[menuElementIndex].Group.GroupHandle = (UNSIGNED16)json_integer_value(jsonGrpIdObj);
    }
    else
    {
        //GroupID do not exist, add groupHandle number to menuGrpPointer
        menuGroup->groupElements[menuElementIndex].Group.GroupHandle = *grpHandle;
    }

    //Step - 3 - Check for Presense Indicator
    getJSONObjectForKey(jsonGroupObj, _T("presenceIndicator"), &jsonGrpPresenceIndicator);
    if(jsonGrpPresenceIndicator != NULL)
    {
        status = FillPresenceIndicator(jsonGrpPresenceIndicator, menuElementIndex, itemReference, menuGroup);
        if(status != OK)
            return ERROR_RESPONSE;
    }

    //Step - 4 - Check for TypeMinor
    menuGroup->groupElements[menuElementIndex].Group.TypeMinor = FALSE;
    getJSONObjectForKey(jsonGroupObj, _T("typeMinor"), &jsonTypeMinor);
    if(jsonTypeMinor != NULL)
    {
        if(json_is_true(jsonTypeMinor))
            menuGroup->groupElements[menuElementIndex].Group.TypeMinor = TRUE;
    }


    //Step - 5 Allocate new MenuGroup for this group
    //Read the json element entries to obtain no. of elements
    getJSONObjectForKey(jsonGroupObj, _T("elements"), &jsonGrpElementArray);

    if(jsonGrpElementArray == NULL) //Not able to retreive elements within a group
        return ERROR_RESPONSE;

    grpElementsCount = (UNSIGNED16)json_array_size(jsonGrpElementArray);	

    groupSize = sizeof(MenuGroup); //Has room for 1 MenuElement already
    if (grpElementsCount > 1)
        groupSize += (grpElementsCount - 1)*sizeof(MenuGroup);

    newmenuGroupElement = (MenuGroup *)OSallocate(groupSize);
    if(newmenuGroupElement == NULL)
        return NOT_ENOUGH_MEMORY;

    *newMenuGroup = newmenuGroupElement;

    return OK;
}

/*------------------------------------------------------------------------------
Purpose:  This should be responsible to add presence indicator data to MenuGroup

Method:   FillPresenceIndicator

Inputs:   jsonGrpPresenceIndicator: Reference to json Object pointing to presence Indicator
menuElementIndex - MenuElementIndex to insert this presence indicator
itemReference: Reference to the current item Reference (Need this to pull oid from itemReference for value Elements)	  
menuGroup : Reference to menuGroup to insert presence indicator data		  

Outputs:  Returns OK with everything is Ok
------------------------------------------------------------------------------*/
ERROR_STATUS FillPresenceIndicator(json_t * jsonGrpPresenceIndicator, UNSIGNED16 menuElementIndex, TCHAR * itemReference, MenuGroup * menuGroup)
{
    ERROR_STATUS status = OK;
    json_t * jsonvalueReference, *jsonTempObj, *jsonTempConstantObj;
    const SIGNED8 * objReference;
    const SIGNED8 * operatorReference;
    BAC_OID_CONVERT  bacOid = {0};

    TCHAR *unicodeOperatorReference;
    OID_TYPE oid;

    //Read the json element entries and start parsing
    getJSONObjectForKey(jsonGrpPresenceIndicator, _T("valueReference"), &jsonvalueReference);

    if(jsonvalueReference == NULL)
        return ERROR_RESPONSE;

    //Get Attribute ID
    jsonTempObj = NULL;
    getJSONObjectForKey(jsonvalueReference,_T("attributeId"), &jsonTempObj);
    if(jsonTempObj != NULL)
        menuGroup->groupElements[menuElementIndex].Group.piPoint.AttrRef = (UNSIGNED16)json_integer_value(jsonTempObj);

    //Get the enum id
    jsonTempObj = NULL;
    getJSONObjectForKey(jsonvalueReference,_T("objectReference"), &jsonTempObj);
    if(jsonTempObj != NULL)
    {
        objReference = json_string_value(jsonTempObj);			
        oid = GetOidFromFullQualifiedRefName(itemReference, objReference);

        menuGroup->groupElements[menuElementIndex].Group.piPoint.ObjectOID = oid;
    }
    else
    {
        //This could be a bacoid Reference
        getJSONObjectForKey(jsonvalueReference,_T("bacoid"), &jsonTempObj);

        bacOid.asUnsigned32 = (UNSIGNED32)json_integer_value(jsonTempObj);

        oid = getJciOid(&bacOid.asBacoid32);
        menuGroup->groupElements[menuElementIndex].Group.piPoint.ObjectOID = oid;
    }

    //Get Operator
    jsonTempObj = NULL;
    getJSONObjectForKey(jsonGrpPresenceIndicator, _T("operator"), &jsonTempObj);
    if(jsonTempObj == NULL)
        return ERROR_RESPONSE;

    operatorReference = json_string_value(jsonTempObj);			
    status = getUnicodeFromASCII(operatorReference, &unicodeOperatorReference);

    if(status != OK)
        return status;

    if(!OSstrcmp(unicodeOperatorReference, _T("equal")))
    {
        menuGroup->groupElements[menuElementIndex].Group.piPoint.PIOperator = PIOPERATOR_EQUAL;
    }
    else if(!OSstrcmp(unicodeOperatorReference, _T("not equal")))
    {
        menuGroup->groupElements[menuElementIndex].Group.piPoint.PIOperator = PIOPERATOR_NOTEQUAL;
    }
    else if(!OSstrcmp(unicodeOperatorReference, _T("greater than")))
    {
        menuGroup->groupElements[menuElementIndex].Group.piPoint.PIOperator = PIOPERATOR_GREATER;
    }
    else if(!OSstrcmp(unicodeOperatorReference, _T("less than")))
    {
        menuGroup->groupElements[menuElementIndex].Group.piPoint.PIOperator = PIOPERATOR_LESSER;
    }
    else
    {
        menuGroup->groupElements[menuElementIndex].Group.piPoint.PIOperator = PIOPERATOR_NOT_FOUND;
    }

    //Release unicodeOperatorRef
    OSrelease(unicodeOperatorReference);

    //Read Constant
    jsonTempObj = NULL;
    getJSONObjectForKey(jsonGrpPresenceIndicator, _T("constant"), &jsonTempObj);
    if(jsonTempObj != NULL && json_is_object(jsonTempObj))
    {
        getJSONObjectForKey(jsonTempObj,_T("id"), &jsonTempConstantObj);
        if(jsonTempConstantObj != NULL)
            menuGroup->groupElements[menuElementIndex].Group.piPoint.PIConstant = (UNSIGNED32)json_integer_value(jsonTempConstantObj);
        else
            menuGroup->groupElements[menuElementIndex].Group.piPoint.PIConstant = (UNSIGNED32)NONE_FFFF;

    }
    else
    {
        menuGroup->groupElements[menuElementIndex].Group.piPoint.PIConstant = (UNSIGNED32)json_integer_value(jsonTempObj);
    }
     

    return status;
}

/*------------------------------------------------------------------------------
Purpose:  This should be responsible to fetch element Type, number of elements, 
and the elementArray reference for a given json Group object

Method:   getGroupElementTypeCountAndElements

Inputs:   jsonGroupData: Reference to json Object pointing to a group		  

Outputs:  elementType - Returns if the group contains a value element or a group element
elementCount - Returns the total number of elements it contains
jElementArray : Reference a reference to json Object that holds group elements
------------------------------------------------------------------------------*/
ERROR_STATUS getGroupElementTypeCountAndElements(json_t * jsonGroupData, UNSIGNED8 * elementType, UNSIGNED16 * elementCount, json_t ** jElementArray)
{
    ERROR_STATUS status = OK;
    UNSIGNED8 eType;
    UNSIGNED16 eCount;
    const SIGNED8 * elementIsGroupOrValue = NULL;
    UNSIGNED16 * wideCharElementIsGroupOrValue = NULL;
    json_t * jsonElementArray, *jsonTempObj, *jsonViewElementType;

    //Read the json element entries and start parsing
    getJSONObjectForKey(jsonGroupData, _T("elements"), &jsonElementArray);

    if(jsonElementArray == NULL) //Not able to retreive elements within a group
        return ERROR_RESPONSE;

    //Step - 1: Get count of Elements.
    eCount = (UNSIGNED16)json_array_size(jsonElementArray);	
    if(eCount < 1)
        return ERROR_RESPONSE;

    //Step - 2: Read the first element in the group and get its element Type
    //All elements in the group will have the same elementType
    jsonTempObj = json_array_get(jsonElementArray, 0);
    if(jsonTempObj == NULL)
        return ERROR_RESPONSE;

    //Step - 3: Read the viewElementType attribute to check if it is a group/value
    getJSONObjectForKey(jsonTempObj, _T("viewElementType"), &jsonViewElementType);
    if(jsonViewElementType == NULL)
        return ERROR_RESPONSE;

    elementIsGroupOrValue = json_string_value(jsonViewElementType);
    if(elementIsGroupOrValue == NULL)
        return ERROR_RESPONSE;

    status = getUnicodeFromASCII(elementIsGroupOrValue, &wideCharElementIsGroupOrValue);
    eType = OSstrcmp(wideCharElementIsGroupOrValue, _T("group")) == 0 ? GROUP_ELEMENT_TYPE : VALUE_ELEMENT_TYPE;

    //Release wideCharElement address
    OSrelease(wideCharElementIsGroupOrValue);

    *elementType = eType;
    *elementCount = eCount;
    *jElementArray = jsonElementArray;

    return status;

}

/*------------------------------------------------------------------------------
(C) Copyright Johnson Controls, Inc. 2013
Use or copying of all or any part of the document, except as
permitted by the License Agreement is prohibited.

Module:   View Interface

Purpose:  This function is recursive since groups can contain groups under it.

Method:   AddNewMenuGroup

Inputs:   jsonGroupData pointer - Pointer to json Object - Need this to fetch info about the group elements
viewGroupHash pointer - Hash pointer to insert new groups
MenuGroup pointer - New pointer to menu Group - to insert into the hash
grpHandle - Group handle for this menu group
itemReference - Reference to template item reference (needed for fetching oid from item reference)

Outputs:  ERROR_STATUS returned if on any issue.
------------------------------------------------------------------------------*/

ERROR_STATUS AddNewMenuGroup(json_t * jsonGroupData, APSHASHTBL * viewGroupHash, MenuGroup * newmenuGroup, UNSIGNED16 * grpHandle, TCHAR * itemReference)
{
    ERROR_STATUS status = OK;
    json_t *jsonElementArray, *jsonGrpIdObj, *jsonGrpElementObj;
    UNSIGNED16 elementsCount, groupId , temp, groupSize = 0;
    UNSIGNED8 elementType;
    const SIGNED8 * elementIsGroupOrValue = NULL;
    MenuGroup * newmenuGroupElement = NULL;
    UNSIGNED16 * wideCharElementIsGroupOrValue = NULL;

    //Step -1 : Retrieve json Element Type, element Count and element array
    status = getGroupElementTypeCountAndElements(jsonGroupData, &elementType, &elementsCount, &jsonElementArray);
    if(status != OK)
        return status;

    //Step - 2: Read groupId and check if it exists, else consider groupHandle
    getJSONObjectForKey(jsonGroupData, _T("id"), &jsonGrpIdObj);

    //Get the groupID if it exists, 
    if(jsonGrpIdObj != NULL)
    {	
        groupId = (UNSIGNED16)json_integer_value(jsonGrpIdObj);
    }
    else
    {
        //GroupID do not exist, add groupHandle number to menuGrpPointer
        groupId = *grpHandle;
        //Increment groupHandle value
        (*grpHandle)++;
    }

    //Step - 3: Update newMenuGroup with all these values
    newmenuGroup->Count = elementsCount;
    newmenuGroup->ElementType = elementType;
    newmenuGroup->GroupHandle = groupId;

    //Step - 4: Add newMenuGroup to Hash
    status = hashtbl_insert(viewGroupHash, &newmenuGroup->GroupHandle, newmenuGroup, sizeof(newmenuGroup->GroupHandle));
    if (status != OK)
        return status;

    //Step - 5: Check for elementType and recurse accordingly
    if(elementType == VALUE_ELEMENT_TYPE)
    {
        FillMenuDataPoints(jsonElementArray, newmenuGroup, elementsCount, itemReference);
    }
    else
    {
        //Loop through the group Elements - 
        for(temp = 0; temp < elementsCount; temp++)
        {
            //Step - 1 - Read the element (would be a group)		
            jsonGrpElementObj = json_array_get(jsonElementArray, temp);
            if(jsonGrpElementObj == NULL)
                return ERROR_RESPONSE;

            //Step - 2 call AddMenuGroupPointerToMenuGroup 
            AddMenuGroupPointerToMenuGroup(newmenuGroup, jsonGrpElementObj, temp, grpHandle, itemReference, &newmenuGroupElement);

            //Step - 3 call same function AddNewMenuGroup
            AddNewMenuGroup(jsonGrpElementObj, viewGroupHash, newmenuGroupElement, grpHandle, itemReference);

        }
    }

    return OK;	
}

/*------------------------------------------------------------------------------
Purpose:  This should be responsible to fetch oid for a given FQRN

Method:   GetOidFromFullQualifiedRefName

Inputs:   itemReference: Reference to item Reference
          objReference: Reference to object Reference fetched from the JSON View File

Outputs:  Returns oid, returns -1 if oid not found
------------------------------------------------------------------------------*/
OID_TYPE GetOidFromFullQualifiedRefName(TCHAR * itemReference, const SIGNED8 * objReference)
{
    PARM_DATA pData;
    TCHAR * unicodeObjRef = NULL;
    TCHAR * fqrRef = NULL;
    UNSIGNED16 strLength = 0;
    OID_TYPE oidVal = -1;
    OID_TYPE * oid = NULL;
    ERROR_STATUS status;
    MODEL_CLASS_VARS *classVarPtr = NULL;
    VIEW_DATABASE * viewDb;

    // get ptr to the model's class vars and fetch oid Hash Reference
    classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

    viewDb = classVarPtr->view_database;

    //objReference is signed8. Go Past the next address to get the data
    objReference++;
    status = getUnicodeFromASCII(objReference, &unicodeObjRef);
    if(status != OK)
        return oidVal;

    strLength = OSstrlen(unicodeObjRef) + OSstrlen(itemReference);

    //Allocate memory for strLength	
    fqrRef = (TCHAR *)OSacquire(STR_STORE(strLength)); 

    //Copy to fqrRef
    OSstrcpy(fqrRef, itemReference);

    //Concatenate path with the file name
    OSstrcat(fqrRef, unicodeObjRef);


    //Done with unicodeObjRef.. Release it
    OSrelease(unicodeObjRef);

    //Check if fqrRef is in hash, if so get its Oid
    if(hashtbl_get(viewDb->oidHash, fqrRef, STR_STORE(strLength), (void **)&oid) == OK)
    {
        //Data Found, oid is filled
        oidVal = *oid;
    }
    else
    {
        //Not Found, Do apsOpenConnection and add to hash
        pData.dataType = STRING_DATA_TYPE;
        pData.parmValue.tString.strPtr = OSacquire(STR_STORE(strLength));
        pData.parmValue.tString.strLen = strLength;
        pData.parmValue.tString.releaseWhenDone = TRUE;
        OSstrncpy(pData.parmValue.tString.strPtr, fqrRef, strLength);

        oidVal = apsOpenConnectionByName(&pData);

        //Acquire memory to store this data.
        oid = (OID_TYPE *)OSacquire(sizeof(OID_TYPE));
        OSmemset(oid, 0, sizeof(UNSIGNED16));

        *oid = oidVal;

        //Insert oid reference with Oid number to hash
        hashtbl_insert(viewDb->oidHash, fqrRef, oid, STR_STORE(strLength));

        apsReleaseParm(&pData);	
    }

    OSrelease(fqrRef);

    return oidVal;
}

/*------------------------------------------------------------------------------
Purpose:  This should be responsible to fill Menu Data points

Method:   FillMenuDataPoints

Inputs:   jsonElementArray: Reference to json object pointing to value element Array
          menuGroup: Reference to menu Group 
          numElements - Number of value elements to be filled
          itemReference: Reference to itemReference

Outputs:  Returns oid, returns -1 if oid not found
------------------------------------------------------------------------------*/
ERROR_STATUS FillMenuDataPoints(json_t * jsonElementArray, MenuGroup * menuGroup, UNSIGNED16 numElements, TCHAR * itemReference)
{
    ERROR_STATUS status = OK;
    json_t *jsonlabelObject, *jsonshortLabelObject, *jsonTempObj, *jsonElementObj, *jsonValueRefObject, *jsonIgnorePresenceObject;
    UNSIGNED16 temp, bacoid = 0;
    OID_TYPE oid = 0;
    const SIGNED8 * objReference, *elementType;
    TCHAR * uniElementType;
    BAC_OID_CONVERT  bacOid = {0};

    //Step - 1: Loop Through all the data elements in a group
    for(temp = 0; temp < numElements; temp++)
    {
        //Step - 2 : Read label element
        jsonElementObj = json_array_get(jsonElementArray, temp);
        if(jsonElementObj == NULL)
            return ERROR_RESPONSE;

        getJSONObjectForKey(jsonElementObj, _T("viewElementType"), &jsonTempObj);
        if(jsonTempObj == NULL)
            return ERROR_RESPONSE;

        elementType = json_string_value(jsonTempObj);	
        status = getUnicodeFromASCII(elementType, &uniElementType);
        if(!OSstrcmp(uniElementType, _T("link")))
        {
            //This is a link.. need to ignore, just reduce the menuGroup's count
            menuGroup->Count = menuGroup->Count - 1;
        }
        else
        {
            getJSONObjectForKey(jsonElementObj, _T("label"), &jsonlabelObject);
            if(jsonlabelObject == NULL)
                return ERROR_RESPONSE;

            //Get label set 
            getJSONObjectForKey(jsonlabelObject,_T("set"), &jsonTempObj);
            if(jsonTempObj != NULL)
                menuGroup->groupElements[temp].Data.LabelEnumSet = (UNSIGNED16)json_integer_value(jsonTempObj);

            //Get the enum id
            jsonTempObj = NULL;
            getJSONObjectForKey(jsonlabelObject,_T("id"), &jsonTempObj);
            if(jsonTempObj != NULL)
                menuGroup->groupElements[temp].Data.LabelEnum = (UNSIGNED16)json_integer_value(jsonTempObj);

            //Short label 
            getJSONObjectForKey(jsonElementObj, _T("shortLabel"), &jsonshortLabelObject);
            if(jsonshortLabelObject != NULL)
            {
                //Get short label set 
                jsonTempObj = NULL;
                getJSONObjectForKey(jsonshortLabelObject,_T("set"), &jsonTempObj);
                if(jsonTempObj != NULL)
                    menuGroup->groupElements[temp].Data.ShortLabelEnumSet = (UNSIGNED16)json_integer_value(jsonTempObj);

                //Get the short enum id
                jsonTempObj = NULL;
                getJSONObjectForKey(jsonshortLabelObject,_T("id"), &jsonTempObj);
                if(jsonTempObj != NULL)
                    menuGroup->groupElements[temp].Data.ShortLabelEnum = (UNSIGNED16)json_integer_value(jsonTempObj);
            }
            else
            {
                menuGroup->groupElements[temp].Data.ShortLabelEnumSet = (UNSIGNED16)NONE_FFFF;
                menuGroup->groupElements[temp].Data.ShortLabelEnum = (UNSIGNED16)NONE_FFFF;
            }

            //Step - 3: Read Value Reference
            getJSONObjectForKey(jsonElementObj, _T("valueReference"), &jsonValueRefObject);
            if(jsonValueRefObject == NULL)
                return ERROR_RESPONSE;

            //Get Attribute ID
            jsonTempObj = NULL;
            getJSONObjectForKey(jsonValueRefObject,_T("attributeId"), &jsonTempObj);
            if(jsonTempObj != NULL)
                menuGroup->groupElements[temp].Data.AttrRef = (UNSIGNED16)json_integer_value(jsonTempObj);

            //Get the enum id
            jsonTempObj = NULL;
            getJSONObjectForKey(jsonValueRefObject,_T("objectReference"), &jsonTempObj);
            if(jsonTempObj != NULL)
            {
                objReference = json_string_value(jsonTempObj);			
                oid = GetOidFromFullQualifiedRefName(itemReference, objReference);

                menuGroup->groupElements[temp].Data.ObjectOID = oid;
            }
            else
            {	
                //This could be a bacoid Reference
                getJSONObjectForKey(jsonValueRefObject,_T("bacoid"), &jsonTempObj);
                bacOid.asUnsigned32 = (UNSIGNED32)json_integer_value(jsonTempObj);

                oid = getJciOid(&bacOid.asBacoid32);

                menuGroup->groupElements[temp].Data.ObjectOID = oid;

            }

			// Step 4 - Read Ignore Presence
			getJSONObjectForKey(jsonElementObj, _T("ignorePresence"), &jsonIgnorePresenceObject);
			if(jsonIgnorePresenceObject != NULL)
			{
				if(json_is_true(jsonIgnorePresenceObject))
					menuGroup->groupElements[temp].Data.IgnorePresence = TRUE;
			}
        }

        //Release unicode string
        OSrelease(uniElementType);

    }

    return OK;	
}


/*------------------------------------------------------------------------------
Purpose:  This should be responsible to fill viewInfo structure with equipment Type data

Method:   FillViewInfoWithEquipmentTypeData

Inputs:   jsonObject: Reference to json object to fetch equipment type data
viewInfo: Reference to view Info to fill these values	  

Outputs:  Returns Ok if successful
------------------------------------------------------------------------------*/
ERROR_STATUS FillViewInfoWithEquipmentTypeData(json_t * jsonObject, VIEW_EQUIPMENT_INFO * viewInfo)
{
    ERROR_STATUS status = OK;
    json_t *jsonEquipmentTypeObj, *jsonTempObj;
    UNSIGNED16 * pwc = NULL;


    //Step - 1 : Read equipment type
    getJSONObjectForKey(jsonObject, _T("equipmentType"), &jsonEquipmentTypeObj);

    if(jsonEquipmentTypeObj == NULL)
        return ERROR_RESPONSE;

    //Get set
    getJSONObjectForKey(jsonEquipmentTypeObj,_T("set"), &jsonTempObj);
    if(jsonTempObj != NULL)
        viewInfo->equipmentTypeSetId = (UNSIGNED16)json_integer_value(jsonTempObj);

    //Get the enum id
    jsonTempObj = NULL;
    getJSONObjectForKey(jsonEquipmentTypeObj,_T("id"), &jsonTempObj);
    if(jsonTempObj != NULL)
        viewInfo->equipmentTypeId = (UNSIGNED16)json_integer_value(jsonTempObj);

    return OK;

}