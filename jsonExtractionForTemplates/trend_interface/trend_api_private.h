/***************************************************************************


Description: This file holds function prototypes for the local model library
             functions as well as structure of top level of data held.

File Name: template_api_private.h

***************************************************************************/
#ifndef TRENDAPI_PRIVATE_H
#define TRENDAPI_PRIVATE_H
#include <trend_api.h>
#include <template_view_common_api.h>
ERROR_STATUS trendTemplateParse(APSHASHTBL * hashtbl, json_t * jsonTemplate, UNSIGNED16 templateKey);
ERROR_STATUS getTrendTemplateInfo(UNSIGNED16 templateId, TREND_TEMPLATE_ENTRY ** trendTemplateInfo);
ERROR_STATUS ReadTrendTemplateFile(TCHAR * pTemplateName, SIGNED8 ** buffer);
ERROR_STATUS AddTrendTemplateToHash(TCHAR * interfaceName, json_t * jsonTemplate, TREND_DATABASE * trendDb);
ERROR_STATUS loadTrendDataModelFile(TCHAR * jsonTemplatePath, SIGNED8 ** fileOutputBuffer);



#endif
