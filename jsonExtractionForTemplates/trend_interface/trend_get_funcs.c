/*------------------------------------------------------------------------------

Module:   trend Interface

Purpose:  Handles all the get operations on the trend template. This fetches data from the hash structure and returns the 
necessary attributes.

Filename: trend_get_funcs.c

Inputs:   Every method will atleast have templateId as a parameter. Corresponding template related data is fetched from the hash.

Outputs:  Returns the data asked by the equipment model

------------------------------------------------------------------------------*/
#include <trend_api.h>
#include "trend_api_private.h"
#include <enum.h>

/*------------------------------------------------------------------------------
Module:   gettrendInfo method

Purpose:  This is a private method and used internally. Returns the trend entry
structure by fetching from the trend database for a given template Id

Inputs:   Template Id key from which the trend template Information needs to be retrieved
from hash

Outputs:  trend Info structure of type TEMPLATE_ENTRY
------------------------------------------------------------------------------*/
ERROR_STATUS getTrendTemplateInfo(UNSIGNED16 trendId, TREND_TEMPLATE_ENTRY ** trendTemplateInfo)
{
	TREND_DATABASE * trendDb = NULL;
	
	MODEL_CLASS_VARS *classVarPtr = NULL;
	
	// get ptr to the model's class vars
  classVarPtr = cdbGetClassInstanceData(equipmentModelClassIndex);

	//Get the trend DB
	trendDb = classVarPtr->trend_database;

	if(trendDb == NULL)
		return TREND_DATABASE_NOT_FOUND;

	if(hashtbl_get(trendDb->trendStructureHash, &trendId, sizeof(trendId), (void **)trendTemplateInfo))
		return TREND_NOT_FOUND;
	
	if(*trendTemplateInfo == NULL)
		return TREND_NOT_FOUND;

	return OK;
}

/*------------------------------------------------------------------------------
Module:   GetTrendTemplateName method

Purpose:  This is a public accessible method, returns the Trend template name for a 
given template Id.

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Template Name.
------------------------------------------------------------------------------*/
ERROR_STATUS GetTrendTemplateName(UNSIGNED16 trendTemplateId, TCHAR ** trendTemplateName)
{
	TREND_TEMPLATE_ENTRY * trendTemplateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTrendTemplateInfo(trendTemplateId, &trendTemplateInfo);

	if(!errorStatus)
	{
		//Get the template name
		*trendTemplateName = trendTemplateInfo->templateName;
		return OK;
	}
	
	return errorStatus;	
}

/*------------------------------------------------------------------------------
Module:   GetTrendTemplateDescription method

Purpose:  This is a public accessible method, returns the template description for a 
given template Id.

Inputs:   Template Id key from which the template Information needs to be retrieved
from hash

Outputs:  Pointer to Template description.
------------------------------------------------------------------------------*/
ERROR_STATUS GetTrendTemplateDescription(UNSIGNED16 trendTemplateId, TCHAR ** trendTemplateDescription)
{
	TREND_TEMPLATE_ENTRY * trendTemplateInfo = NULL;
	ERROR_STATUS errorStatus;

	errorStatus = getTrendTemplateInfo(trendTemplateId, &trendTemplateInfo);

	if(!errorStatus)
	{
		//Get the template description
		*trendTemplateDescription = trendTemplateInfo->templateDescription;
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
ERROR_STATUS  GetTrendTemplateKeyPropertyAttributes (UNSIGNED16 trendTemplateId,TREND_TEMPLATE_PROPERTY_ATTR_INFOLIST** trendTemplateKeyPropertiesVal)
{
		TREND_TEMPLATE_ENTRY * trendTemplateInfo = NULL;
		TREND_TEMPLATE_PROPERTY_ATTR_INFOLIST *trendTemplateKeyProperties;
		TREND_TEMPLATE_PROPERTY_ATTR_INFO  *trendTemplateKeyProperty;
		TREND_TEMPLATE_PROPERTY_ATTR_INFO  *trendTemplateKeyProperty1;
		ERROR_STATUS errorStatus;
		struct hashEntry_s *node;
		APSHASHTBL *mainnodes;
		UNSIGNED16 index=0;
		UNSIGNED16 attIdCount=0;
		UNSIGNED16 totalAttributesCounts=0;
		void *data =NULL;
	errorStatus = getTrendTemplateInfo(trendTemplateId, &trendTemplateInfo);
		if(!errorStatus)
		{
			mainnodes =trendTemplateInfo->trendCreationAttrInfo;
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
		trendTemplateKeyProperties = (TREND_TEMPLATE_PROPERTY_ATTR_INFOLIST *)OSacquire(sizeof(TREND_TEMPLATE_PROPERTY_ATTR_INFOLIST));
			//Allocate memory for template Key Properties
			trendTemplateKeyProperty = (TREND_TEMPLATE_PROPERTY_ATTR_INFO *)OSacquire(sizeof(TREND_TEMPLATE_PROPERTY_ATTR_INFO) * totalAttributesCounts);
				mainnodes =trendTemplateInfo->trendCreationAttrInfo;
				for(index=0;index < 8 ; index++)
				{
					//Get the template present value attribute
					node = mainnodes->nodes[index];
					while(node)
					{
						data= (void **)node->data;
						trendTemplateKeyProperty1=(TREND_TEMPLATE_PROPERTY_ATTR_INFO *)node->data;
						trendTemplateKeyProperty[attIdCount]=*(TREND_TEMPLATE_PROPERTY_ATTR_INFO *)node->data;// *templateKeyProperty1;
						attIdCount++;
						node=node->next;
					}
				}
				trendTemplateKeyProperties->numtemplatePropertyInfoEntries=totalAttributesCounts;
				trendTemplateKeyProperties->propertyInfo=trendTemplateKeyProperty;
				*trendTemplateKeyPropertiesVal=trendTemplateKeyProperties;
			return OK;

		}
	return errorStatus;
	//No need for Release - 1
//	return OK;
}


