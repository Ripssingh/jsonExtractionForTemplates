/*------------------------------------------------------------------------------

Module:   View Interface

Purpose:  Handles all the get operations on the view. This fetches data from the hash structure and returns the 
          necessary attributes.

Filename: view_get_funcs.c

Inputs:   Every method will atleast have viewID as a parameter. Corresponding view related data is fetched from the hash.

Outputs:  Returns the data asked by the local UI
------------------------------------------------------------------------------*/
#include <view_api.h>
#include "view_api_private.h"

/*-------------------------------------------------------------------------------
Module:   GetGroupByHandle method

Purpose:  This is a public method and can be called from any component. 
          Returns the menu group structure for a given viewId and GroupHandle

Inputs:   view Id key from which the view Information needs to be retrieved from hash
          groupHandle - Returns pointer to menu Group based on the group handle passed

Outputs:  Menu Group structure of type MenuGroup
------------------------------------------------------------------------------*/
ERROR_STATUS GetGroupByHandle(UNSIGNED16 viewId, UNSIGNED16 groupHandle,  MenuGroup ** menuGroup)
{
    VIEW_DATABASE * viewDb;
    VIEW_EQUIPMENT_INFO * viewInfo;
    MODEL_CLASS_VARS *classVarPtr;

    // get ptr to the model's class vars
    classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

    //Get the view DB
    viewDb = classVarPtr->view_database;


    if(viewDb == NULL)
        return VIEW_DATABASE_NOT_FOUND;

    if(hashtbl_get(viewDb->viewHash, &viewId, sizeof(viewId), (void **)&viewInfo))
        return JSONVIEW_NOT_FOUND;

    //Pick menuGroup based on the passed in group handle
    if(viewInfo->viewGrpHash == NULL)
        return JSONVIEW_GROUPHASH_NOT_FOUND;

    if(hashtbl_get(viewInfo->viewGrpHash, &groupHandle, sizeof(groupHandle), (void **)menuGroup))
        return JSONVIEW_MENUGROUP_NOT_FOUND;

    if(*menuGroup == NULL)
        return JSONVIEW_MENUGROUP_NOT_FOUND;

    return OK;
}

/*------------------------------------------------------------------------------
Module:   GetViewGroup method

Purpose:  This is a public accessible method, returns the top level menu Group
          for a given view ID

Inputs:   view Id key from which the view Information needs to be retrieved
          from hash

Outputs:  Pointer to MenuGroup
------------------------------------------------------------------------------*/
ERROR_STATUS GetViewGroup(UNSIGNED16 viewId, MenuGroup ** menuGroup)
{	
    VIEW_DATABASE * viewDb;
    VIEW_EQUIPMENT_INFO * viewInfo;
    MODEL_CLASS_VARS *classVarPtr;

    //The top level group will always have a groupHandle of 1000
    UNSIGNED16 groupHandle = 1000;

    // get ptr to the model's class vars
    classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

    //Get the view DB
    viewDb = classVarPtr->view_database;


    if(viewDb == NULL)
        return VIEW_DATABASE_NOT_FOUND;

    if(hashtbl_get(viewDb->viewHash, &viewId, sizeof(viewId), (void **)&viewInfo))
        return JSONVIEW_NOT_FOUND;

    //Pick menuGroup based on the passed in group handle
    if(viewInfo->viewGrpHash == NULL)
        return JSONVIEW_GROUPHASH_NOT_FOUND;

    if(hashtbl_get(viewInfo->viewGrpHash, &groupHandle, sizeof(groupHandle), (void **)menuGroup))
        return JSONVIEW_MENUGROUP_NOT_FOUND;

    if(*menuGroup == NULL)
        return JSONVIEW_MENUGROUP_NOT_FOUND;

    return OK;
}

/*******************************************************************************
  Method:  GetTopLevelViews

  Purpose: Finds the top level view IDs in the system and returns them to
           the client in a JCI list of lists.  The data is organized in
           the following manner:

           <list>
             <list>
               <unsigned short>View Id</unsigned short>
               <byte>Internal</byte>
             </list>
             <list>
               ...
             </list>
           </list>

  Inputs:  pOutputList -- [OUT] A JCI list variable that has already been initialized
                                prior to calling this function

  Return:  OK if there is a view hash to parse,
           VIEW_DATABASE_NOT_FOUND if there is no view hash available.
           Note that OK is returned even if there are no elements returned.
*******************************************************************************/
ERROR_STATUS GetTopLevelViews(PARM_DATA* pOutputList)
{
  VIEW_DATABASE * viewDb;
  VIEW_EQUIPMENT_INFO * viewInfo;
  struct hashEntry_s *node;
  PARM_DATA* pParm;
  PARM_DATA* pView;
  ERROR_STATUS status;
  MODEL_CLASS_VARS *classVarPtr;

  // get ptr to the model's class vars
  classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

  //Get the view DB
  viewDb = classVarPtr->view_database;

  if (viewDb == NULL)
  {
    status = VIEW_DATABASE_NOT_FOUND;
  }
  else
  {
    for (node = viewDb->viewHash->nodes[0]; node != NULL; node = node->next)
    {
      viewInfo = (VIEW_EQUIPMENT_INFO*)node->data;
      pParm = apsNextListElement(pOutputList);
      apsNewList(pParm, 2);
      pView = apsNextListElement(pParm);
      pView->dataType = USHORT_DATA_TYPE;
      pView->parmValue.tUnint = viewInfo->viewId;
      pView = apsNextListElement(pParm);
      pView->dataType = BYTE_DATA_TYPE;
      pView->parmValue.tByte = viewInfo->internal;
    }

    status = OK;
  }

  return status;
}
