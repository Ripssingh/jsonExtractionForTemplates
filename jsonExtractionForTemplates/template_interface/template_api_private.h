/***************************************************************************

Description: This file holds function prototypes for the local model library
             functions as well as structure of top level of data held.

File Name: template_api_private.h

***************************************************************************/
#ifndef TEMPLATEINTERFACE_PRIVATE_H
#define TEMPLATEINTERFACE_PRIVATE_H


ERROR_STATUS templateParse(APSHASHTBL * hashtbl, json_t * jsonTemplate, UNSIGNED16 templateKey);
ERROR_STATUS getTemplateInfo(UNSIGNED16 templateId, TEMPLATE_ENTRY ** templateInfo);
ERROR_STATUS ReadTemplateFile(TCHAR * pTemplateName, SIGNED8 ** buffer,UNSIGNED8 tbool);
ERROR_STATUS AddTemplateToHash(TCHAR * interfaceName, json_t * jsonTemplate, TEMPLATE_DATABASE * templateDb);
ERROR_STATUS loadDataModelFile(TCHAR * jsonTemplatePath, SIGNED8 ** fileOutputBuffer);



#endif
