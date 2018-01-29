/*------------------------------------------------------------------------------

Module:   Template Interface

Purpose:  Handles all the get operations on the template. This fetches data from the hash structure and returns the 
necessary attributes.

Filename: template_get_funcs.c

Inputs:   Every method will atleast have templateId as a parameter. Corresponding template related data is fetched from the hash.

Outputs:  Returns the data asked by the equipment model
------------------------------------------------------------------------------*/
#include <template_api.h>
#include "template_api_private.h"
#include <enum.h>

/*------------------------------------------------------------------------------
Module:   getTemplateInfo method

Purpose:  This is a private method and used internally. Returns the template entry
structure by fetching from the template database for a given template Id

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Template Info structure of type TEMPLATE_ENTRY
------------------------------------------------------------------------------*/
ERROR_STATUS getTemplateInfo(UNSIGNED16 templateId, TEMPLATE_ENTRY ** templateInfo)
{
	TEMPLATE_DATABASE * templateDb = NULL;
	
	MODEL_CLASS_VARS *classVarPtr = NULL;
	
	// get ptr to the model's class vars
  classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

	//Get the template DB
	templateDb = classVarPtr->template_database;


	if(templateDb == NULL)
		return TEMPLATE_DATABASE_NOT_FOUND;

	if(hashtbl_get(templateDb->templateStructureHash, &templateId, sizeof(templateId), (void **)templateInfo))
		return TEMPLATE_NOT_FOUND;
	
	if(*templateInfo == NULL)
		return TEMPLATE_NOT_FOUND;

	return OK;
}

/*------------------------------------------------------------------------------
Module:   GetTemplateType method

Purpose:  This is a public accessible method, returns the template type for a 
given template Id.

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Template type.
------------------------------------------------------------------------------*/
ERROR_STATUS GetTemplateType( UNSIGNED16 templateId,  UNSIGNED16 * templateType)
{	
	TEMPLATE_ENTRY * templateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTemplateInfo(templateId, &templateInfo);

	if(!errorStatus)
	{
		//Get the template type
		*templateType = templateInfo->type;
		if(templateType)
			return OK;
		else
			return TEMPLATE_DATABASE_NOT_FOUND;
	}
	
	return errorStatus;
}


/*------------------------------------------------------------------------------
Module:   GetTemplateSubType method

Purpose:  This is a public accessible method, returns the template sub-type for a 
given template Id.

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Template subtype.
------------------------------------------------------------------------------*/
ERROR_STATUS GetTemplateSubType( UNSIGNED16 templateId,  UNSIGNED16 * templateSubType)
{
	
	TEMPLATE_ENTRY * templateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTemplateInfo(templateId, &templateInfo);

	if(!errorStatus)
	{
		//Get the template type
		*templateSubType = templateInfo->subType;
		if(templateSubType)
			return OK;
		else
			return TEMPLATE_DATABASE_NOT_FOUND;
	}
	
	return errorStatus;	
}


/*------------------------------------------------------------------------------
Module:   GetTemplateName method

Purpose:  This is a public accessible method, returns the template name for a 
given template Id.

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Template Name.
------------------------------------------------------------------------------*/
ERROR_STATUS GetTemplateName(UNSIGNED16 templateId, TCHAR ** templateName)
{
	TEMPLATE_ENTRY * templateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTemplateInfo(templateId, &templateInfo);

	if(!errorStatus)
	{
		//Get the template name
		*templateName = templateInfo->templateName;
		if(*templateName)
			return OK;
		else
			return TEMPLATE_DATABASE_NOT_FOUND;
	}
	
	return errorStatus;	
}

/*------------------------------------------------------------------------------
Module:   GetTemplateDescription method

Purpose:  This is a public accessible method, returns the template description for a 
given template Id.

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Template description.
------------------------------------------------------------------------------*/
ERROR_STATUS GetTemplateDescription(UNSIGNED16 templateId, TCHAR ** templateDescription)
{
	TEMPLATE_ENTRY * templateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTemplateInfo(templateId, &templateInfo);

	if(!errorStatus)
	{
		//Get the template description
		*templateDescription = templateInfo->templateDescription;
		if(*templateDescription)
			return OK;
		else
			return TEMPLATE_DATABASE_NOT_FOUND;
	}
	
	return errorStatus;	
}


/*------------------------------------------------------------------------------
Module:   GetTemplateSubComponent method

Purpose:  This is a public accessible method, returns the template subcomponent for a 
given template Id and component Id. The caller must pass the component Id to be retrieved.

Inputs:   Template Id key from which the template Information needs to be retrieved
			from hash
		  Component Id that needs to be fetched from the template

Outputs:  Pointer to Template Sub Component
------------------------------------------------------------------------------*/
ERROR_STATUS  GetTemplateSubComponent(UNSIGNED16 templateId, TCHAR * componentId, TEMPLATE_SUBCOMPONENT_INFO** templateSubComponent)
{
	TEMPLATE_ENTRY * templateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTemplateInfo(templateId, &templateInfo);

	if(!errorStatus)
	{
		//Get the template subcomponent
		if(templateInfo->templateSubComponentInfo == NULL)
			return TEMPLATE_SUBCOMPONENT_NOT_FOUND;
	
		if(hashtbl_get(templateInfo->templateSubComponentInfo, componentId, STR_STORE(OSstrlen(componentId)), (void **)templateSubComponent))
			return TEMPLATE_SUBCOMPONENT_NOT_FOUND;

		return OK;
	}
	
	return errorStatus;
}

/*------------------------------------------------------------------------------
Module:   GetTemplatePropertyInfo method

Purpose:  This is a public accessible method, returns the template property info
for a given template Id and template's attribute Id.

Inputs:   Template Id key from which the template Information needs to be retrieved
			from hash
		  Property Attribute ID within the template who's property structure to be
			obtained.
      Internal OID (jci oid) of the equipment object for which the info is
      requested.  If NONE(0), redirected units/enumset/min/max values will not
      be filled in since the actual redirected value needs to be read from a
      specified/particular equipment object instance.

Outputs:  Pointer to Template Property Info Structure
------------------------------------------------------------------------------*/
ERROR_STATUS  GetTemplatePropertyInfo(UNSIGNED16 templateId, UNSIGNED16 attrId, OID_TYPE equipObjId, TEMPLATE_PROPERTY_ATTR_INFO** templatePropertyInfo)
{
	TEMPLATE_ENTRY * templateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTemplateInfo(templateId, &templateInfo);

	if(!errorStatus)
	{
		//Get the template property info
		if(templateInfo->templateAttrInfo == NULL)
			return TEMPLATE_PROPERTY_NOT_FOUND;
	
		if(hashtbl_get(templateInfo->templateAttrInfo, &attrId, sizeof(attrId), (void **)templatePropertyInfo))
			return TEMPLATE_PROPERTY_NOT_FOUND;
    else //Get any redirected values for min/max, units, or enum set
    {
      TEMPLATE_PROPERTY_ATTR_INFO * attrInfo;
      attrInfo = *templatePropertyInfo;
      if (equipObjId && attrInfo->redirectedVals)
      {
        PARM_DATA parm;
        ERROR_STATUS readStatus;
        if (attrInfo->redirectedEnumSetProp)
        {
          readStatus = stdReadInternalAttr(equipObjId, attrInfo->redirectedEnumSetProp, &parm, ENUM_DATA_TYPE);
          if (readStatus == OK)
            attrInfo->enumSet = parm.parmValue.tEnum;
        }
        if (attrInfo->redirectedUnits_IP_Prop)
        {
          readStatus = stdReadInternalAttr(equipObjId, attrInfo->redirectedUnits_IP_Prop, &parm, ENUM_DATA_TYPE);
          if (readStatus == OK)
            attrInfo->units_IP = parm.parmValue.tEnum;
        }
        if (attrInfo->redirectedUnits_SI_Prop)
        {
          readStatus = stdReadInternalAttr(equipObjId, attrInfo->redirectedUnits_SI_Prop, &parm, ENUM_DATA_TYPE);
          if (readStatus == OK)
            attrInfo->units_SI = parm.parmValue.tEnum;
        }
#ifdef USE_DOUBLE
        if (attrInfo->redirectedMin_IP_Prop)
        {
          readStatus = stdReadInternalAttr(equipObjId, attrInfo->redirectedMin_IP_Prop, &parm, DOUBLE_DATA_TYPE);
          if (readStatus == OK)
            attrInfo->min_IP = parm.parmValue.tDouble;
        }
        if (attrInfo->redirectedMax_IP_Prop)
        {
          readStatus = stdReadInternalAttr(equipObjId, attrInfo->redirectedMax_IP_Prop, &parm, DOUBLE_DATA_TYPE);
          if (readStatus == OK)
            attrInfo->max_IP = parm.parmValue.tDouble;
        }
        if (attrInfo->redirectedMin_SI_Prop)
        {
          readStatus = stdReadInternalAttr(equipObjId, attrInfo->redirectedMin_SI_Prop, &parm, DOUBLE_DATA_TYPE);
          if (readStatus == OK)
            attrInfo->min_SI = parm.parmValue.tDouble;
        }
        if (attrInfo->redirectedMax_SI_Prop)
        {
          readStatus = stdReadInternalAttr(equipObjId, attrInfo->redirectedMax_SI_Prop, &parm, DOUBLE_DATA_TYPE);
          if (readStatus == OK)
            attrInfo->max_SI = parm.parmValue.tDouble;
        }
#else
        if (attrInfo->redirectedMin_IP_Prop)
        {
          readStatus = stdReadInternalAttr(equipObjId, attrInfo->redirectedMin_IP_Prop, &parm, FLOAT_DATA_TYPE);
          if (readStatus == OK)
            attrInfo->min_IP = parm.parmValue.tFloat;
        }
        if (attrInfo->redirectedMax_IP_Prop)
        {
          readStatus = stdReadInternalAttr(equipObjId, attrInfo->redirectedMax_IP_Prop, &parm, FLOAT_DATA_TYPE);
          if (readStatus == OK)
            attrInfo->max_IP = parm.parmValue.tFloat;
        }
        if (attrInfo->redirectedMin_SI_Prop)
        {
          readStatus = stdReadInternalAttr(equipObjId, attrInfo->redirectedMin_SI_Prop, &parm, FLOAT_DATA_TYPE);
          if (readStatus == OK)
            attrInfo->min_SI = parm.parmValue.tFloat;
        }
        if (attrInfo->redirectedMax_SI_Prop)
        {
          readStatus = stdReadInternalAttr(equipObjId, attrInfo->redirectedMax_SI_Prop, &parm, FLOAT_DATA_TYPE);
          if (readStatus == OK)
            attrInfo->max_SI = parm.parmValue.tFloat;
        }
#endif
      }
    }

		return OK;
	}
	
	return errorStatus;	
}
/*------------------------------------------------------------------------------
Module:   GetTemplateKeyPropertyAttributeId method

Purpose:  This is a public accessible method, returns the template's key property
attribute Id list

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Template's key property attributes list.
------------------------------------------------------------------------------*/
ERROR_STATUS  GetTemplateKeyPropertyAttributes (UNSIGNED16 templateId,TEMPLATE_PROPERTY_ATTR_INFOLIST** templateKeyPropertiesVal)
{
		TEMPLATE_ENTRY * templateInfo = NULL;
		TEMPLATE_PROPERTY_ATTR_INFOLIST *templateKeyProperties;
		TEMPLATE_PROPERTY_ATTR_INFO  *templateKeyProperty;
		TEMPLATE_PROPERTY_ATTR_INFO  *templateKeyProperty1;
		ERROR_STATUS errorStatus;
		struct hashEntry_s *node;
		APSHASHTBL *mainnodes;
		UNSIGNED16 index=0;
		UNSIGNED16 attIdCount=0;
		UNSIGNED16 totalAttributesCounts=0;
		void *data =NULL;


	errorStatus = getTemplateInfo(templateId, &templateInfo);
		if(!errorStatus)
		{
			mainnodes =templateInfo->templateAttrInfo;
			for(index=0;index < 8 ; index++)
			{
				//Get the template present value attribute
				node = mainnodes->nodes[index];
				while(node)
				{
					//*data = node->data;
					totalAttributesCounts = totalAttributesCounts +1;
					node=node->next;
				}
			}
			//Allocate memory for template Key Properties
		templateKeyProperties = (TEMPLATE_PROPERTY_ATTR_INFOLIST *)OSacquire(sizeof(TEMPLATE_PROPERTY_ATTR_INFOLIST));
			//Allocate memory for template Key Properties
			templateKeyProperty = (TEMPLATE_PROPERTY_ATTR_INFO *)OSacquire(sizeof(TEMPLATE_PROPERTY_ATTR_INFO) * totalAttributesCounts);
				mainnodes =templateInfo->templateAttrInfo;
				for(index=0;index < 8 ; index++)
				{
					//Get the template present value attribute
					node = mainnodes->nodes[index];
					while(node)
					{
						data= (void **)node->data;
						templateKeyProperty1=(TEMPLATE_PROPERTY_ATTR_INFO *)node->data;
						templateKeyProperty[attIdCount]=*(TEMPLATE_PROPERTY_ATTR_INFO *)node->data;// *templateKeyProperty1;
						attIdCount++;
						node=node->next;
					}
				}
				templateKeyProperties->numtemplatePropertyInfoEntries=totalAttributesCounts;
				templateKeyProperties->propertyInfo=templateKeyProperty;
				*templateKeyPropertiesVal=templateKeyProperties;
			return OK;

		}
	return errorStatus;
	//No need for Release - 1
//	return OK;
}



/*------------------------------------------------------------------------------
Module:   GetTemplateKeyPropertyAttributeId method

Purpose:  This is a public accessible method, returns the template's key property
attribute Id list

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Template's key property attributes list.
------------------------------------------------------------------------------*/
ERROR_STATUS  GetTemplateKeyPropertyAttributeId (UNSIGNED16 templateId, TEMPLATE_PROPERTY_ATTR_INFOLIST** templateKeyProperties)
{
	
	//No need for Release - 1
		return OK;
}

/*------------------------------------------------------------------------------
Module:   GetTemplatePresentValueAttributeId method

Purpose:  This is a public accessible method, returns the template's present value
attribute ID for a given template Id.

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Template's present value attribute
------------------------------------------------------------------------------*/
ERROR_STATUS GetTemplatePresentValueAttributeId (UNSIGNED16 templateId, UNSIGNED16 * presentvalueAttributeId)
{

	TEMPLATE_ENTRY * templateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTemplateInfo(templateId, &templateInfo);

	if(!errorStatus)
	{
		//Get the template present value attribute
		*presentvalueAttributeId = templateInfo->presentValueAttrId;
		if(presentvalueAttributeId)
			return OK;
		else
			return TEMPLATE_DATABASE_NOT_FOUND;
	}
	
	return errorStatus;
}



/*------------------------------------------------------------------------------
Module:   GetTemplateParent method

Purpose:  This is a public accessible method, returns the template's 
		Parent ID(ID of the template this template extends) for a given template Id.

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Template's Parent ID(ID of the template this template extends)
------------------------------------------------------------------------------*/
ERROR_STATUS GetTemplateParent (UNSIGNED16 templateId, TCHAR ** templateDescription)
{

	TEMPLATE_ENTRY * templateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTemplateInfo(templateId, &templateInfo);

	if(!errorStatus)
	{
		//Get the template present value attribute
		*templateDescription = templateInfo->templateParent;
		if(*templateDescription)
			return OK;
		else
			return TEMPLATE_DATABASE_NOT_FOUND;
	}
	
	return errorStatus;
}



/*------------------------------------------------------------------------------
Module:   GetDictionaryName method

Purpose:  This is a public accessible method, returns the Dictionary Name for a given template Id.

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Dictionary Name
------------------------------------------------------------------------------*/
ERROR_STATUS GetDictionaryName (UNSIGNED16 templateId, TCHAR ** dictionaryName)
{

	TEMPLATE_ENTRY * templateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTemplateInfo(templateId, &templateInfo);

	if(!errorStatus)
	{
		//Get the template present value attribute
		*dictionaryName = templateInfo->dictionaryName;
		if(*dictionaryName)
			return OK;
		else
			return TEMPLATE_DATABASE_NOT_FOUND;
	}
	
	return errorStatus;
}


/*------------------------------------------------------------------------------
Module:   GetTemplateID method

Purpose:  This is a public accessible method, returns the Unique ID of the template .

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Unique ID of the template 
------------------------------------------------------------------------------*/
ERROR_STATUS GetTemplateID (UNSIGNED16 templateId, TCHAR ** templateID)
{

	TEMPLATE_ENTRY * templateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTemplateInfo(templateId, &templateInfo);

	if(!errorStatus)
	{
		//Get the template present value attribute
		*templateID = templateInfo->templateID;
		if(*templateID)
			return OK;
		else
			return TEMPLATE_DATABASE_NOT_FOUND;
	}
	
	return errorStatus;
}



/*------------------------------------------------------------------------------
Module:   GetTemplateSubComponent method

Purpose:  This is a public accessible method, returns the template subcomponent for a 
given template Id and component Id. The caller must pass the component Id to be retrieved.

Inputs:   Template Id key from which the template Information needs to be retrieved
			from hash
		  Component Id that needs to be fetched from the template

Outputs:  Pointer to Template Sub Component
------------------------------------------------------------------------------*/
ERROR_STATUS  GetTemplateSubComponentList(UNSIGNED16 templateId, TEMPLATE_SUBCOMPONENT_INFO_LIST** templateSubComponentInfo)
{
	TEMPLATE_ENTRY * templateInfo = NULL;
	ERROR_STATUS errorStatus;
	APSHASHTBL *mainnodes = NULL;
	struct hashEntry_s *node = NULL;
	TEMPLATE_SUBCOMPONENT_INFO_LIST* subComponentInfoList = NULL;
	TEMPLATE_SUBCOMPONENT_INFO* subComponentProperties  = NULL;
	UNSIGNED16 totalSubComponentCount = 0;
	void *data = NULL;
	errorStatus = getTemplateInfo(templateId, &templateInfo);
	

	if(!errorStatus)
	{

		//Get the template subcomponent
		if(templateInfo->templateSubComponentInfo == NULL)
			return TEMPLATE_SUBCOMPONENT_NOT_FOUND;
		mainnodes = (APSHASHTBL* )templateInfo->templateSubComponentInfo;
		node = mainnodes->nodes[0];
		while(node)
		{
			totalSubComponentCount = totalSubComponentCount + 1;
			node=node->next;
		}
		subComponentInfoList = (TEMPLATE_SUBCOMPONENT_INFO_LIST *)OSacquire(sizeof(TEMPLATE_SUBCOMPONENT_INFO_LIST));
		//mainnodes = (APSHASHTBL* )templateInfo->templateSubComponentInfo;
		subComponentProperties = (TEMPLATE_SUBCOMPONENT_INFO *)OSacquire(sizeof(TEMPLATE_SUBCOMPONENT_INFO) * totalSubComponentCount);
		node = mainnodes->nodes[0];
		if(!(subComponentInfoList && mainnodes && subComponentProperties && node))
			return TEMPLATE_PARSE_ERROR;
		totalSubComponentCount = 0;
		while(node)
		{
			data = (void **)node->data;
			subComponentProperties[totalSubComponentCount] = *(TEMPLATE_SUBCOMPONENT_INFO *)node->data;// *templateKeyProperty1;
			totalSubComponentCount = totalSubComponentCount + 1;
			node=node->next;
		}
		subComponentInfoList->numSubComponentInfoEntries = totalSubComponentCount;
		subComponentInfoList->subComponentInfo = subComponentProperties;
		*templateSubComponentInfo = subComponentInfoList;
		//else
		//	templateSubComponentInfo = templateInfo->templateSubComponentInfo

		return OK;
	}
	
	return errorStatus;
}


