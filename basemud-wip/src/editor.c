/**************************************************************************
 *  File: editor.c                                                         *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 *                                                                         *
 *  This code was freely distributed with the The Isles 1.1 source code,   *
 *  and has been used here for OLC - OLC would not be what it is without   *
 *  all the previous coders who released their source code.                *
 *                                                                         *
 ***************************************************************************/


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mud.h"
/*
 *  Thanks to Kalgen for the new procedure (no more bug!)
 *  Original wordwrap() written by Surreality.
 */
/*****************************************************************************
 Name:		format_string
 Purpose:	Special string formating and word-wrapping.
 Called by:	string_add(string.c) (many)olc_act.c
 ****************************************************************************/
char *format_string( char *oldstring /*, bool fSpace */)
{
  char xbuf[MAX_BUFFER];
  char xbuf2[MAX_BUFFER];
  char *rdesc;
  int i=0;
  bool cap=TRUE;
  
  xbuf[0]=xbuf2[0]=0;
  
  i=0;
  
  if ( strlen(oldstring) >= (MAX_BUFFER - 4) )	/* OLC 1.1b */
  {
     bug( "String to format_string() longer than MAX_BUFFER.", 0 );
     return (oldstring);
  }

  for (rdesc = oldstring; *rdesc; rdesc++)
  {
    if (*rdesc=='\n')
    {
      if (xbuf[i-1] != ' ')
      {
        xbuf[i]=' ';
        i++;
      }
    }
    else if (*rdesc=='\r') ;
    else if (*rdesc==' ')
    {
      if (xbuf[i-1] != ' ')
      {
        xbuf[i]=' ';
        i++;
      }
    }
    else if (*rdesc==')')
    {
      if (xbuf[i-1]==' ' && xbuf[i-2]==' ' && 
          (xbuf[i-3]=='.' || xbuf[i-3]=='?' || xbuf[i-3]=='!'))
      {
        xbuf[i-2]=*rdesc;
        xbuf[i-1]=' ';
        xbuf[i]=' ';
        i++;
      }
      else
      {
        xbuf[i]=*rdesc;
        i++;
      }
    }
    else if (*rdesc=='.' || *rdesc=='?' || *rdesc=='!') {
      if (xbuf[i-1]==' ' && xbuf[i-2]==' ' && 
          (xbuf[i-3]=='.' || xbuf[i-3]=='?' || xbuf[i-3]=='!')) {
        xbuf[i-2]=*rdesc;
        if (*(rdesc+1) != '\"')
        {
          xbuf[i-1]=' ';
          xbuf[i]=' ';
          i++;
        }
        else
        {
          xbuf[i-1]='\"';
          xbuf[i]=' ';
          xbuf[i+1]=' ';
          i+=2;
          rdesc++;
        }
      }
      else
      {
        xbuf[i]=*rdesc;
        if (*(rdesc+1) != '\"')
        {
          xbuf[i+1]=' ';
          xbuf[i+2]=' ';
          i += 3;
        }
        else
        {
          xbuf[i+1]='\"';
          xbuf[i+2]=' ';
          xbuf[i+3]=' ';
          i += 4;
          rdesc++;
        }
      }
      cap = TRUE;
    }
    else
    {
      xbuf[i]=*rdesc;
      if ( cap )
        {
          cap = FALSE;
          xbuf[i] = toupper( xbuf[i] );
        }
      i++;
    }
  }
  xbuf[i]=0;
  strcpy(xbuf2,xbuf);
  
  rdesc=xbuf2;
  
  xbuf[0]=0;
  
  for ( ; ; )
  {
    for (i=0; i<77; i++)
    {
      if (!*(rdesc+i)) break;
    }
    if (i<77)
    {
      break;
    }
    for (i=(xbuf[0]?76:73) ; i ; i--)
    {
      if (*(rdesc+i)==' ') break;
    }
    if (i)
    {
      *(rdesc+i)=0;
      strcat(xbuf,rdesc);
      strcat(xbuf,"\n\r");
      rdesc += i+1;
      while (*rdesc == ' ') rdesc++;
    }
    else
    {
      bug ("No spaces", 0);
      *(rdesc+75)=0;
      strcat(xbuf,rdesc);
      strcat(xbuf,"-\n\r");
      rdesc += 76;
    }
  }
  while (*(rdesc+i) && (*(rdesc+i)==' '||
                        *(rdesc+i)=='\n'||
                        *(rdesc+i)=='\r'))
    i--;
  *(rdesc+i+1)=0;
  strcat(xbuf,rdesc);
  if (xbuf[strlen(xbuf)-2] != '\n')
    strcat(xbuf,"\n\r");

  free(oldstring);
  return(strdup(xbuf));
}

/*
 * Used above in string_add.  Because this function does not
 * modify case if fCase is FALSE and because it understands
 * parenthesis, it would probably make a nice replacement
 * for one_argument.
 */
/*****************************************************************************
 Name:		first_arg
 Purpose:	Pick off one argument from a string and return the rest.
 		Understands quates, parenthesis (barring ) ('s) and
 		percentages.
 Called by:	string_add(string.c)
 ****************************************************************************/
char *first_arg( char *argument, char *arg_first, bool fCase )
{
    char cEnd;

    while ( *argument == ' ' )
	argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"'
      || *argument == '%'  || *argument == '(' )
    {
        if ( *argument == '(' )
        {
            cEnd = ')';
            argument++;
        }
        else cEnd = *argument++;
    }

    while ( *argument != '\0' )
    {
	if ( *argument == cEnd )
	{
	    argument++;
	    break;
	}
    if ( fCase ) *arg_first = tolower(*argument);
            else *arg_first = *argument;
	arg_first++;
	argument++;
    }
    *arg_first = '\0';

    while ( *argument == ' ' )
	argument++;

    return argument;
}
/*****************************************************************************
 Name:		string_append
 Purpose:	Clears string and puts player into editing mode.
 Called by:	none
 ****************************************************************************/
void string_edit( CHAR_DATA *ch, char **pString )
{
    text_to_char(ch, "-========- Entering EDIT Mode -=========-\n\r");
    text_to_char(ch, "    Type .h on a new line for help\n\r" );
    text_to_char(ch, "  Terminate with a @ on a blank line.\n\r");
    text_to_char(ch, "-=======================================-\n\r" );

    if ( *pString == NULL )
    {
        *pString = strdup( "" );
    }
    else
    {
        **pString = '\0';
    }

    ch->socket->pString = pString;

    return;
}



/*****************************************************************************
 Name:		string_append
 Purpose:	Puts player into append mode for given string.
 Called by:	(many)olc_act.c
 ****************************************************************************/
void string_append( CHAR_DATA *ch, char **pString )
{
    text_to_char(ch, "-=======- Entering APPEND Mode -========-\n\r");
    text_to_char(ch, "    Type .h on a new line for help\n\r");
    text_to_char(ch, "  Terminate with a @ on a blank line.\n\r");
    text_to_char(ch, "-=======================================-\n\r");

    if ( *pString == NULL )
    {
        *pString = strdup( "" );
    }
    text_to_char(ch, *pString);
    
    if ( *(*pString + strlen( *pString ) - 1) != '\r' )
    text_to_char(ch, "\n\r" );

    ch->socket->pString = pString;

    return;
}



/*****************************************************************************
 Name:		string_replace
 Purpose:	Substitutes one string for another.
 Called by:	string_add(string.c) (aedit_builder)olc_act.c.
 ****************************************************************************/
char * string_replace( char * orig, char * old, char * new )
{
    char xbuf[MAX_BUFFER];
    int i;

    xbuf[0] = '\0';
    strcpy( xbuf, orig );
    if ( strstr( orig, old ) != NULL )
    {
        i = strlen( orig ) - strlen( strstr( orig, old ) );
        xbuf[i] = '\0';
        strcat( xbuf, new );
        strcat( xbuf, &orig[i+strlen( old )] );
        free( orig );
    }

    return strdup( xbuf );
}


/* OLC 1.1b */
/*****************************************************************************
 Name:		string_add
 Purpose:	Interpreter for string editing.
 Called by:	game_loop_xxxx(comm.c).
 ****************************************************************************/
void string_add( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_BUFFER];

    /*
     * Thanks to James Seng
     */
    /*smash_tilde( argument );*/

    if ( *argument == '.' )
    {
        char arg1 [MAX_BUFFER];
        char arg2 [MAX_BUFFER];
        char arg3 [MAX_BUFFER];

        argument = one_arg( argument, arg1 );
        argument = first_arg( argument, arg2, FALSE );
        argument = first_arg( argument, arg3, FALSE );

        if ( !strcasecmp( arg1, ".c" ) )
        {
            text_to_char(ch, "String cleared.\n\r");
            **ch->socket->pString = '\0';
            return;
        }

        if ( !strcasecmp( arg1, ".s" ) )
        {
            text_to_char(ch, "String so far:\n\r");
            text_to_char(ch, *ch->socket->pString);
            return;
        }

        if ( !strcasecmp( arg1, ".r" ) )
        {
            if ( arg2[0] == '\0' )
            {
                text_to_char(ch,
                    "usage:  .r \"old string\" \"new string\"\n\r");
                return;
            }

            *ch->socket->pString =
                string_replace( *ch->socket->pString, arg2, arg3 );
            sprintf( buf, "'%s' replaced with '%s'.\n\r", arg2, arg3 );
            text_to_char( ch, buf );
            return;
        }

        if ( !strcasecmp( arg1, ".f" ) )
        {
            *ch->socket->pString = format_string( *ch->socket->pString );
            text_to_char( ch, "String formatted.\n\r");
            return;
        }
        
        if ( !strcasecmp( arg1, ".h" ) )
        {
            text_to_char(ch, "Sedit help (commands on blank line):   \n\r");
            text_to_char(ch, ".r 'old' 'new'   - replace a substring \n\r");
            text_to_char(ch, "                   (requires '', \"\") \n\r");
            text_to_char(ch, ".h               - get help (this info)\n\r");
            text_to_char(ch, ".s               - show string so far  \n\r");
            text_to_char(ch, ".f               - (word wrap) string  \n\r");
            text_to_char(ch, ".c               - clear string so far \n\r");
            text_to_char(ch, "@                - end string          \n\r");
            return;
        }
            

        text_to_char(ch, "SEdit:  Invalid dot command.\n\r");
        return;
    }

    if ( *argument == '@' )
    {
        ch->socket->pString = NULL;
        return;
    }

    /*
     * Truncate strings to MAX_BUFFER.
     * --------------------------------------
     */
    if ( strlen( buf ) + strlen( argument ) >= ( MAX_BUFFER - 4 ) )
    {
        text_to_char(ch, "String too long, last line skipped.\n\r");

	/* Force character out of editing mode. */
        ch->socket->pString = NULL;
        return;
    }

    strcpy( buf, *ch->socket->pString );
    strcat( buf, argument );
    strcat( buf, "\n\r" );
    free( *ch->socket->pString );
    *ch->socket->pString = strdup( buf );
    return;
}











/*
 * Used in olc_act.c for aedit_builders.
 */
char * string_unpad( char * argument )
{
    char buf[MAX_BUFFER];
    char *s;

    s = argument;

    while ( *s == ' ' )
        s++;

    strcpy( buf, s );
    s = buf;

    if ( *s != '\0' )
    {
        while ( *s != '\0' )
            s++;
        s--;

        while( *s == ' ' )
            s--;
        s++;
        *s = '\0';
    }

    free( argument );
    return strdup( buf );
}



/*
 * Same as capitalize but changes the pointer's data.
 * Used in olc_act.c in aedit_builder.
 */
char * string_proper( char * argument )
{
    char *s;

    s = argument;

    while ( *s != '\0' )
    {
        if ( *s != ' ' )
        {
            *s = toupper(*s);
            while ( *s != ' ' && *s != '\0' )
                s++;
        }
        else
        {
            s++;
        }
    }

    return argument;
}



/*
 * Returns an all-caps string.		OLC 1.1b
 */
char* all_capitalize( const char *str )
{
    static char strcap [ MAX_BUFFER ];
           int  i;
    for ( i = 0; str[i] != '\0'; i++ )
	strcap[i] = toupper( str[i] );
    strcap[i] = '\0';
    return strcap;
}
