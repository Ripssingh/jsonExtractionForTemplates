/*------------------------------------------------------------------------------

Module:   view_init

Purpose:  - InitializeView is called  by eq. obj when device is idle
          Called on modelClassFeatureStart method of the equipment object

Filename: view_init.c

Inputs:   NA

Outputs:  NA

------------------------------------------------------------------------------*/
#include <view_api.h>
#include "view_api_private.h"

CLASS_INDEX equipmentModelClassIndex = 0;

/*------------------------------------------------------------------------------
Module:   InitView method

Purpose:  This function is called once during equipment class start. It doesn't parse
          the json view File, but does the initialization of hash structures.

Inputs:   NA

Outputs:  NA
------------------------------------------------------------------------------*/
ERROR_STATUS InitView()
{
    APSHASHTBL *viewHash;
    APSHASHTBL *oidHash;
    VIEW_DATABASE * viewDb;
    ERROR_STATUS status = OK;
    MODEL_CLASS_VARS* classVarPtr = NULL;

    if(equipmentModelClassIndex == 0)
    {
      equipmentModelClassIndex = getEquipmentModelClassIndexHelper();
    }
    
    classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

    //Allocate memory for Template
    viewDb = (VIEW_DATABASE *)OSacquire(sizeof(VIEW_DATABASE));
    if(viewDb == NULL)
        return NOT_ENOUGH_MEMORY;

    //Hash list to store the viewId and its corresponding reference to menu Grup Pointer
    if((viewHash=hashtbl_create(VIEW_DB_ENTRY_GROW_SIZE, HASH_TYPE_INT)) == NULL) 
        return HASH_CREATE_ERROR;

    //Hash list to store the apsOpenConnectionString and its corresponding OID reference
    if((oidHash=hashtbl_create(VIEW_OID_CONV_GROW_SIZE, HASH_TYPE_STR)) == NULL) 
        return HASH_CREATE_ERROR;


    viewDb->viewCount = 0;
    viewDb->viewHash = viewHash;
    viewDb->oidHash = oidHash;

    classVarPtr->view_database = viewDb;

    return status;

}

/*------------------------------------------------------------------------------
Module:   InitializeView method

Purpose:  This function is called when view needs to be initialized, 
          Called by the equipment object when device is idle.
          Parses the JSON file and pushes data into hash

Inputs:   itemReference: Pointer to top level object item Reference
          jsonObject: Pointer to json Object that holds the JSON view

Outputs:  There could be a possibility that this is called multiple times. First check if viewDatabase 
          already has views in it. If exists just return.
          Else fill MenuGroup structures from jsonObject, Returns OK
------------------------------------------------------------------------------*/
ERROR_STATUS InitializeView(TCHAR*  itemReference, json_t * jsonObject)
{	
    APSHASHTBL *viewGroupHash = NULL; //Reference to store view Groups - Group Handle & MenuGroup Pointer
    VIEW_DATABASE * viewDb;
    ERROR_STATUS status = OK;
    json_t *jsonViewArray, *jsonViewGroupArray, *jsonViewId, *jsonTempObj;
    UNSIGNED16 viewCount, temp, groupTemp, viewId = 0, groupSize, elementsCount  = 0;
    UNSIGNED16 * groupHandle;
    MenuGroup *menuGroup, *newmenuGroup = NULL; //Reference to menu Group structure
    UNSIGNED8 elementType;
    MODEL_CLASS_VARS* classVarPtr = NULL;

    json_t *jsonViewData, *jsonGroupData, *jsonVersionObject = NULL;
    VIEW_EQUIPMENT_INFO * viewInfo = NULL;
    const SIGNED8 * viewVersion;

    UNSIGNED16 * unicodeviewVersion = NULL;

    classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);
    
    viewDb = classVarPtr->view_database;
    if(viewDb == NULL)
        return ERROR_RESPONSE;

    if(viewDb->viewCount > 0) //View Count greater than 0, data is already parsed just return.
        return OK;

    //Allocate groupHandle
    groupHandle = (UNSIGNED16 *)OSacquire(sizeof(UNSIGNED16));
    if(groupHandle == NULL)
        return NOT_ENOUGH_MEMORY;	

    getJSONObjectForKey(jsonObject, _T("Version"), &jsonVersionObject);

    //Get the version number and add to MODEL CLASS VARS
    if(jsonVersionObject != NULL)
    {
        //Get the version number
        viewVersion = (SIGNED8 const*)json_string_value(jsonVersionObject);

        //Convert to Unicode -- No Need to release unicodeviewVersion - Will be in memory forever.
        getUnicodeFromASCII(viewVersion, &unicodeviewVersion);

        //Add this to Class Vars
        classVarPtr->view_Version = (TCHAR *)unicodeviewVersion;

    }


    //Read the json array entries and parse each view
    getJSONObjectForKey(jsonObject, _T("views"), &jsonViewArray);
    if(jsonViewArray != NULL)
    {	
        //Get the number of views associated.
        viewCount = (UNSIGNED16)json_array_size(jsonViewArray);	

        //Loop through the view array, get index and pass to Read
        for(temp = 0; temp < viewCount; temp++)
        {	
            //Step - 1: Initialize GroupHash
            //Hash list to store the groupHandle and its corresponding reference to menu Group Pointer
            if((viewGroupHash=hashtbl_create(VIEW_DB_GROUP_ENTRY_GROW_SIZE, HASH_TYPE_INT)) == NULL) 
                return HASH_CREATE_ERROR;

            *groupHandle = 1000; //Initial value of group Handle - Refer design spec for this number.

            //Allocate ViewEquipmentInfo structure
            viewInfo = (VIEW_EQUIPMENT_INFO *)OSallocate(sizeof(VIEW_EQUIPMENT_INFO));
            if(viewInfo == NULL)
                return NOT_ENOUGH_MEMORY;

            FillViewInfoWithEquipmentTypeData(jsonObject, viewInfo);	

            //Step - 2: Read JSON Object for the view
            jsonViewData = json_array_get(jsonViewArray, temp);
            if(jsonViewData == NULL)
                return ERROR_RESPONSE;

            getJSONObjectForKey(jsonViewData, _T("viewId"), &jsonViewId);
            if(jsonViewId == NULL)
                return ERROR_RESPONSE;

              //Get the enum view id, ignore set
            jsonTempObj = NULL;
            getJSONObjectForKey(jsonViewId,_T("id"), &jsonTempObj);
            if(jsonTempObj != NULL)
                viewId = (UNSIGNED16)json_integer_value(jsonTempObj);

            //Step -3 : Retreive element type, element count and element Array
            status = getGroupElementTypeCountAndElements(jsonViewData, &elementType, &elementsCount, &jsonViewGroupArray);
            if(status != OK)
                return status;

            //Step - 4: Allocate MenuGroup pointer
            groupSize = sizeof(MenuGroup); //Has room for 1 MenuElement already
            if (elementsCount > 1)
                groupSize += (UNSIGNED16)((elementsCount - 1) * sizeof(MenuGroup));
            menuGroup = (MenuGroup *)OSallocate(groupSize);

            if(menuGroup == NULL)
                return NOT_ENOUGH_MEMORY;


            //Update menuGroup structure except Group Elements - Top Level will always "act" as a group
            //Group Handle will always be 1000
            menuGroup->ElementType = elementType;
            menuGroup->GroupHandle = *groupHandle;
            menuGroup->Count = elementsCount;

            //Step - 5: Add to Hash - Top level menuGroup for this view
            status = hashtbl_insert(viewGroupHash, &menuGroup->GroupHandle, menuGroup, sizeof(menuGroup->GroupHandle));
            if (status != OK)
                return status;

            //Step - 6: Increment groupHandle value
            (*groupHandle)++;


            if(elementType == VALUE_ELEMENT_TYPE)
            {
                //Step - 7: Value Elements.. Fill with Menu Data points
                FillMenuDataPoints(jsonViewGroupArray, menuGroup, elementsCount, itemReference);
            }
            else
            {

                //Step - 7: Loop through Group Element array (This can contain just value elements also...)
                for(groupTemp = 0; groupTemp < elementsCount; groupTemp++)
                {
                    jsonGroupData = json_array_get(jsonViewGroupArray, groupTemp);

                    if(jsonGroupData == NULL)
                        return ERROR_RESPONSE;

                    //Step - 8: Add this Group Information to menuGroup (Its label set, label enum, groupId)	
                    //groupTemp is passed to insert menuElement at that index. i.e MenuGroup's Element(groupTemp) will be filled
                    //groupHandle is passed to update MenuGroupPointer with the groupHandle if groupId do not exist
                    AddMenuGroupPointerToMenuGroup(menuGroup, jsonGroupData, groupTemp, groupHandle, itemReference, &newmenuGroup);					

                    //Step - 9: Add this to Menu Group (Recursive Function)
                    AddNewMenuGroup(jsonGroupData, viewGroupHash, newmenuGroup, groupHandle, itemReference);
                }

            }	

            //Step - 10: Fill ViewEquipmentInfo Structure
            viewInfo->toplevelGroup = menuGroup;
            viewInfo->viewGrpHash = viewGroupHash;
            viewInfo->viewId = viewId;

            //Step - 11: Record internal flag to indicate if this view should be exposed to the outside world
            getJSONObjectForKey(jsonViewData, _T("internalView"), &jsonTempObj);
            if(jsonTempObj == NULL)
                return ERROR_RESPONSE;
            viewInfo->internal = jsonTempObj->type == JSON_TRUE ? 1 : 0;

            //Insert view Equipment Info to view Hash
            status = hashtbl_insert(viewDb->viewHash, &viewId, viewInfo, sizeof(viewId));

            if (status != OK)
                return status;

        }//End of View For

        //Update View Database with total number of views
        viewDb->viewCount = viewCount;			
    }


    //Release group handle
    OSrelease(groupHandle);

    //Deallocate json objects 
    json_decref(jsonObject);

    
    return OK;
}

CLASS_INDEX getEquipmentModelClassIndexHelper(void)
{
  CLASS_INDEX classIndex = 0;

  classIndex = cdbGetClassIndex(EQUIPMENT_CLASS);
  if (classIndex == NO_CLASS_INDEX)
  {
    // try the matlab app class
    classIndex = cdbGetClassIndex(MATLAB_APP_CLASS);
  }

  return classIndex;
}