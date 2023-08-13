/*
 * $Id$ --
 *
 *   Generic routines for internationalization operations
 *
 * Copyright (c) 2001-2010 InHand Networks, Inc.
 *
 * PROPRIETARY RIGHTS of InHand Networks are involved in the
 * subject matter of this material.  All manufacturing, reproduction,
 * use, and sales rights pertaining to this subject matter are governed
 * by the license agreement.  The recipient of this software implicitly
 * accepts the terms of the license.
 *
 * Creation Date: 06/12/2010
 * Author: Jianliang Zhang
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "shared.h"

/*
#define DEFAULT_LANGUAGE	"English"
#define DEFAULT_OEM_NAME	"inhand"
#define DEFAULT_MODEL_NAME	"ISD2008"

char g_lang[MAX_LANG_NAME] = DEFAULT_LANGUAGE;
char g_model_name[MAX_MODEL_NAME] = DEFAULT_MODEL_NAME;
char g_oem_name[MAX_OEM_NAME] = DEFAULT_OEM_NAME;
*/

/**
 * get the current language
 * @param	none
 * @return	the language name
 */
char *
get_language (void)
{
	char *lang = g_sysinfo.lang;
	
	if(strcmp(lang, "auto")==0) 
		lang = g_auto_lang; //FIXME: get browser setting

	return *lang ? lang : DEFAULT_LANG_NAME;
}

/**
 * get the current language resource file
 * @param	none
 * @return	the language resource file path
 */
char *
get_lang_resource (void)
{
	char *lang = get_language();
	static char buf[MAX_LANG_NAME_LEN+16];
	
	snprintf (buf, sizeof(buf), LANG_PACK_PATH "%s.res", lang);
	
	return buf;
}

/**
 * get the current product path for oem / model
 * @param	none
 * @return	the language resource file path
 */
char *
get_product_path (void)
{
	char *p = g_sysinfo.oem_name;
	static char buf[MAX_LANG_NAME_LEN+16];
	
	if(!p || !*p) p = DEFAULT_OEM_NAME;

	snprintf (buf, sizeof(buf), PRODUCTS_PATH "%s", p);
	
	return buf;
}


/**
 * get the current language resource file for oem / model
 * @param	none
 * @return	the language resource file path
 */
char *
get_product_resource (void)
{
	char *p;
	char *lang;
	static char buf[MAX_LANG_NAME_LEN+16];
	
	p = get_product_path();
	lang = get_language();

	snprintf (buf, sizeof(buf), "%s/%s.res", p, lang);
	
	return buf;
}

/**
 * translate text according to current language setting
 * if the text is not found in the current language, it will be searched in the English resource file,
 * and if it's still not found, the same text as the input is returned to avoid trouble
 * @param	tran	the text to be translated
 * @return	the translated text or the original input
 */
char *
get_product_text (char* tran)
{
	char *lang = get_product_resource ();
	char *p;

	p = find_text (tran, lang);

	return p ? p : tran;
}

/**
 * translate text according to current language setting
 * if the text is not found in the current language, it will be searched in the English resource file,
 * and if it's still not found, the same text as the input is returned to avoid trouble
 * @param	tran	the text to be translated
 * @return	the translated text or the original input
 */
char *
get_text (char* tran)
{
	char *lang = get_lang_resource ();
	char *p;

	p = find_text (tran, lang);

	return p ? p : tran;
}

/**
 * find text in the the resource file
 * @param	tran	the text to be translated
 * @return	the translated text or null if not found
 */
char *
find_text (char *tran, char* resource)
{
	FILE *fp;
	static char temp[MAX_LANG_LINE];
	char temp1[MAX_LANG_LINE];
	char *temp2;
	char* p;
	int len;
		
	strcpy (temp1, tran);
	strcat (temp1, "=\"");
	
	len = strlen (temp1);
	
	fp = fopen (resource, "r");
	if (fp == NULL) return NULL;
	
	while (fgets (temp, MAX_LANG_LINE, fp) != NULL)
    {
		if ((memcmp (temp, temp1, len)) == 0)
		{
			fclose (fp);
			
			p = temp + len;
			
			temp2 = strrchr(p, '\"');
			if(!temp2) temp2 = strrchr(p, '\'');
			if(!temp2) temp2 = strrchr(p, ';');
			if(temp2) *temp2 = '\0';
			
			return p;
		}
    }
	fclose (fp);
	
	return NULL;		//not found
}

/**
 * get the current help resource file
 * @param	none
 * @return	the help resource file path
 */
char *
get_help_resource (void)
{
	char *lang = get_language();
	static char buf[MAX_LANG_NAME_LEN+16];
	
	sprintf (buf, LANG_PACK_PATH "%s_help.js", lang);
	
	return buf;
}

/**
 * get help definition according to current language setting
 * if the text is not found in the current language, it will be searched in the English help resource file,
 * and if it's still not found, the same text as the input is returned to avoid trouble
 * @param	term	the text to search
 * @return	the help text or the original input
 */
char *
get_help(char* term)
{
	char *help = get_help_resource ();
	char *p;

	p = find_text (term, help);
	if (!p) {
		help = get_lang_resource();
		p = find_text (term, help);
	}
	
	return p ? p : term;
}

/**
 * get the current short help resource file
 * @param	none
 * @return	the help resource file path
 */
char *
get_short_help_resource (void)
{
	char *lang = get_language();
	static char buf[MAX_LANG_NAME_LEN+16];
	
	sprintf (buf, LANG_PACK_PATH "%s_short.js", lang);
	
	return buf;
}

/**
 * get short help definition according to current language setting
 * if the text is not found in the current language, it will be searched in the English help resource file,
 * and if it's still not found, the same text as the input is returned to avoid trouble
 * @param	term	the text to search
 * @return	the help text or the original input
 */
char *
get_short_help(char* term)
{
	char *help = get_short_help_resource ();
	char *p;

	p = find_text (term, help);
	if (!p) {
		help = get_lang_resource();
		p = find_text (term, help);
	}
	
	return p ? p : term;
}

char * get_default_syslocation(void)
{
        return get_product_text("product.snmpSysLocation"); 
}

char * get_default_syscontact(void)
{
        return get_product_text("product.snmpSysContact"); 
}

char * get_default_sysobjid(void)
{
        return get_product_text("product.snmpSysObjectID"); 
}

char * get_default_product_name(void)
{
        return get_product_text("product.name"); 
}

char * get_product_welcome(void)
{
        return get_product_text("product.welcome");
}

