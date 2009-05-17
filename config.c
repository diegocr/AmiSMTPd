/* ***** BEGIN LICENSE BLOCK *****
 * Version: MIT/X11 License
 * 
 * Copyright (c) 2007 Diego Casorran
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * Contributor(s):
 *   Diego Casorran <dcasorran@gmail.com> (Original Author)
 * 
 * ***** END LICENSE BLOCK ***** */


#include "globals.h"
#include "config.h"

Config * config = NULL;

BOOL InitConfig( VOID )
{
	BOOL result = FALSE;
	
	if((config = malloc(sizeof(*config))))
	{
		char * file;
		
		if((file = FileToMem( CONFIG_FILE )))
		{
			char *ln,*ptr=NULL,*f=file;
			int line_n = 0;
			
			while((ln = _strsep( &f, "\n")))
			{
				if(!*ln || *ln == ';')
					continue;
				
				switch( line_n++ )
				{
					case 0:
						ptr = config->ServerVirtualHostname = strdup( ln );
						break;
					case 1:
						ptr = config->ServerHostname = strdup( ln );
						break;
					case 2:
						ptr = config->SysadminEmail = strdup( ln );
						break;
					case 3:
						ptr = config->SysadminMailbox = strdup( ln );
						break;
					case 4:
						ptr = config->RelayFromHosts = strdup( ln );
						break;
					case 5:
						ptr = config->RelayForUsers = strdup( ln );
						break;
					default:
						ptr = NULL;
						break;
				}
				
				if( ptr == NULL || line_n == 6 )
					break;
			}
			
			if( line_n == 6 && ptr != NULL )
				result = TRUE;
			
			fMem( file );
		}
	}
	
	return result;
}

VOID ClearConfig( VOID )
{
	if( config == NULL )
		return;
	
	fMem(config->ServerVirtualHostname);
	fMem(config->ServerHostname);
	fMem(config->SysadminEmail);
	fMem(config->SysadminMailbox);
	fMem(config->RelayFromHosts);
	fMem(config->RelayForUsers);
	fMem(config);
}
