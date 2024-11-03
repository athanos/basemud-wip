/*
 * This file handles string copy/search/comparison/etc.
 */
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

/* include main header file */
#include "mud.h"

/*
 * Compares two strings, and returns TRUE
 * if they match 100% (not case sensetive).
 */
bool compares(const char *aStr, const char *bStr)
{
  int i = 0;

  /* NULL strings never compares */
  if (aStr == NULL || bStr == NULL) return FALSE;

  while (aStr[i] != '\0' && bStr[i] != '\0' && toupper(aStr[i]) == toupper(bStr[i]))
    i++;

  /* if we terminated for any reason except the end of both strings return FALSE */
  if (aStr[i] != '\0' || bStr[i] != '\0')
    return FALSE;

  /* success */
  return TRUE;
}

/*
 * Checks if aStr is a prefix of bStr.
 */
bool is_prefix(const char *aStr, const char *bStr)
{
  /* NULL strings never compares */
  if (aStr == NULL || bStr == NULL) return FALSE;

  /* empty strings never compares */
  if (aStr[0] == '\0' || bStr[0] == '\0') return FALSE;

  /* check if aStr is a prefix of bStr */
  while (*aStr)
  {
    if (tolower(*aStr++) != tolower(*bStr++))
      return FALSE;
  }

  /* success */
  return TRUE;
}

char *one_arg(char *fStr, char *bStr)
{
  /* skip leading spaces */
  while (isspace(*fStr))
    fStr++; 

  /* copy the beginning of the string */
  while (*fStr != '\0')
  {
    /* have we reached the end of the first word ? */
    if (*fStr == ' ')
    {
      fStr++;
      break;
    }

    /* copy one char */
    *bStr++ = *fStr++;
  }

  /* terminate string */
  *bStr = '\0';

  /* skip past any leftover spaces */
  while (isspace(*fStr))
    fStr++;

  /* return the leftovers */
  return fStr;
}

char *capitalize(char *txt)
{
  static char buf[MAX_BUFFER];
  int size, i;

  buf[0] = '\0';

  if (txt == NULL || txt[0] == '\0')
    return buf;

  size = strlen(txt);

  for (i = 0; i < size; i++)
    buf[i] = toupper(txt[i]);
  buf[size] = '\0';

  return buf;
}

/*  
 * Create a new buffer.
 */
BUFFER *__buffer_new(int size)
{
  BUFFER *buffer;
    
  buffer = malloc(sizeof(BUFFER));
  buffer->size = size;
  buffer->data = malloc(size);
  buffer->len = 0;
  return buffer;
}

/*
 * Add a string to a buffer. Expand if necessary
 */
void __buffer_strcat(BUFFER *buffer, const char *text)  
{
  int new_size;
  int text_len;
  char *new_data;
 
  /* Adding NULL string ? */
  if (!text)
    return;

  text_len = strlen(text);
    
  /* Adding empty string ? */ 
  if (text_len == 0)
    return;

  /* Will the combined len of the added text and the current text exceed our buffer? */
  if ((text_len + buffer->len + 1) > buffer->size)
  { 
    new_size = buffer->size + text_len + 1;
   
    /* Allocate the new buffer */
    new_data = malloc(new_size);
  
    /* Copy the current buffer to the new buffer */
    memcpy(new_data, buffer->data, buffer->len);
    free(buffer->data);
    buffer->data = new_data;  
    buffer->size = new_size;
  }
  memcpy(buffer->data + buffer->len, text, text_len);
  buffer->len += text_len;
  buffer->data[buffer->len] = '\0';
}

/* free a buffer */
void buffer_free(BUFFER *buffer)
{
  /* Free data */
  free(buffer->data);
 
  /* Free buffer */
  free(buffer);
}

/* Clear a buffer's contents, but do not deallocate anything */
void buffer_clear(BUFFER *buffer)
{
  buffer->len = 0;
  buffer->data[0] = '\0';
}

/* print stuff, append to buffer. safe. */
int bprintf(BUFFER *buffer, char *fmt, ...)
{  
  char buf[MAX_BUFFER];
  va_list va;
  int res;
    
  va_start(va, fmt);
  res = vsnprintf(buf, MAX_BUFFER, fmt, va);
  va_end(va);
    
  if (res >= MAX_BUFFER - 1)  
  {
    buf[0] = '\0';
    bug("Overflow when printing string %s", fmt);
  }
  else
    buffer_strcat(buffer, buf);
   
  return res;
}

char *strdup(const char *s)
{
  char *pstr;
  int len;

  len = strlen(s) + 1;
  pstr = (char *) calloc(1, len);
  strcpy(pstr, s);

  return pstr;
}

int strcasecmp(const char *s1, const char *s2)
{
  int i = 0;

  while (s1[i] != '\0' && s2[i] != '\0' && toupper(s1[i]) == toupper(s2[i]))
    i++;

  /* if they matched, return 0 */
  if (s1[i] == '\0' && s2[i] == '\0')
    return 0;

  /* is s1 a prefix of s2? */
  if (s1[i] == '\0')
    return -110;

  /* is s2 a prefix of s1? */
  if (s2[i] == '\0')
    return 110;

  /* is s1 less than s2? */
  if (toupper(s1[i]) < toupper(s2[i]))
    return -1;

  /* s2 is less than s1 */
  return 1;
}

const char *GetSpaces( int length )
{
        static char buf[MAX_BUFFER];

        buf[0] = 0;

        for ( int i = 0; i < length; i++ )
                strcat( buf, " " );

        strcat( buf, "\0" );

        return buf;
}

char *Ordinal( int num )
{
        switch ( num % 100 )
        {
                case 11:
                case 12:
                case 13:
                        return "th";
        }

        switch ( num % 10 )
        {
                default: return "th";

                case 1: return "st";
                case 2: return "nd";
                case 3: return "rd";
        }

        return "th";
}

char *Proper( const char *text )
{
        if ( text[0] == 0 )
                return "";

        static char proper_buf[MAX_BUFFER];

        snprintf( proper_buf, MAX_BUFFER, "%s", text );
        proper_buf[0] = toupper( proper_buf[0] );

        return proper_buf;
}

char *CommaStyle( long long number )
{
        static char     num[MAX_BUFFER], buf[MAX_BUFFER];
        int                     str_size = 0, o = 0;

        snprintf( num, MAX_BUFFER, "%lld", number );
        buf[0] = 0;
        str_size = strlen( num );

        for ( int i = 0; i < str_size; i++ )
        {
                if ( ( str_size - i ) % 3 == 0 && i != 0 )
                        buf[o++] = ',';

                buf[o++] = num[i];
        }

        buf[o] = 0;

        return buf;
}

char *NewString( char *old_string )
{
        if ( !old_string )
                return NULL;

        return strdup( old_string );
}

char *FormattedWordWrap( SOCKET_DATA *dsock, const char *string, int start, int length )
{
        if ( !dsock )
                return NULL;

        static char output[MAX_OUTPUT];
        char word[MAX_OUTPUT];
        output[0] = 0;
        word[0] = 0;
        int iPtr = 0, i = 0, line_len = 0, word_cnt = 0;
        int string_len = strlen( string );

        for ( i = 0; i < string_len; i++ )
        {
                if ( i > 0 && string[i-1] == '\r' && string[i] == '\n' )
                {
                        output[iPtr++] = '\r'; output[iPtr++] = '\n';
                        continue;
                }

                for ( int j = i; j < ( i + 62 ) && string[j] != 0 && string[j] != ' ' && string[j] != '\r' && string[j] != '\n'; j++ )
                {
                        switch ( string[j] )
                        {
                                case '^':
                                {
                                        if ( string[j+1] != '^' )
                                                line_len -= 2;
                                        else line_len -= 1;
                                }
                                break;
                        }

                        word[word_cnt++] = string[j];
                }

                word[word_cnt] = 0;

                if ( line_len + word_cnt > 79 )
                {
                        output[iPtr++] = '\r'; output[iPtr++] = '\n';
                        line_len = word_cnt + 18;
                        for ( int k = 0; k < 17; k++ )
                                output[iPtr++] = ' ';
                }
                else
                        line_len += word_cnt + 1;

                for ( int j = 0; word[j] != 0; j++ )
                        output[iPtr++] = word[j];

                i += word_cnt;

                output[iPtr++] = ' ';

                word[0] = 0;
                word_cnt = 0;
        }

        output[iPtr] = 0;

        return output;
}

char *NumberedWordWrap( SOCKET_DATA *dsock, const char *string )
{
        if ( !dsock || !string )
                return NULL;

        static char     output[MAX_OUTPUT];
        char            word[MAX_OUTPUT];
        char            line_cnt[20];
        bool            bBeginBracket = FALSE;

        output[0] = 0;
        word[0] = 0;

        memset( word, 0, sizeof( word ) );
        memset( output, 0, sizeof( output ) );

        int iPtr = 0, i = 0, line_len = 0, word_cnt = 0;
        int string_len = strlen( string );
        int line = 0;

        for ( i = 0; i < string_len; i++ )
        {
                if ( i > 0 && ( ( string[i-1] == '/' && string[i] == 'n' ) ) )
                {
                        output[iPtr++] = '\r';
                        output[iPtr++] = '\n';
                        line_len = 0;
                        continue;
                }

                if ( line_len == 0 )
                {
                        snprintf( line_cnt, 20, "[%3d] ", ++line );
                        strcat( output, line_cnt );
                        iPtr += 6;
                }

                for ( int j = i; j < ( i + dsock->word_wrap ) && string[j] != 0 && string[j] != ' '; j++ )
                {
                        switch ( string[j] )
                        {
                                case '^':
                                {
                                        if ( string[j+1] != '^' )
                                                line_len -= 2;
                                        else line_len -= 1;
                                }
                                break;

                                case '[':
                                {
                                        if ( string[j+1] != '[' )
                                        {
                                                line_len -= 1;
                                                bBeginBracket = TRUE;
                                        }
                                }
                                break;

                                case ']':
                                {
                                        if ( bBeginBracket )
                                        {
                                                bBeginBracket = FALSE;
                                                line_len--;
                                        }
                                }
                                break;
                        }

                        word[word_cnt++] = string[j];
                }

                word[word_cnt] = 0;

                if ( line_len + word_cnt > dsock->word_wrap )
                {
                        output[iPtr++] = '\r';
                        output[iPtr++] = '\n';
                        line_len = word_cnt + 1;

                        snprintf( line_cnt, 20, "[%3d] ", ++line );
                        strcat( output, line_cnt );
                        iPtr += 6;
                }
                else
                        line_len += word_cnt + 1;

                for ( int j = 0; word[j] != 0; j++ )
                        output[iPtr++] = word[j];

                i += word_cnt;

                output[iPtr++] = ' ';

                word[0] = 0;
                word_cnt = 0;
        }

        output[iPtr] = 0;

        return output;
}

char *WordWrap( SOCKET_DATA *dsock, const char *string )
{
        if ( !dsock )
                return NULL;

        static char output[MAX_OUTPUT];
        char            word[MAX_BUFFER];
        int                     cnt = 0;
        int                     line = 0;
        bool            bBracket = FALSE;

        output[0]       = 0;
        word[cnt]       = 0;

        while ( *string != 0 )
        {
                if ( *string == '\n' )
                {
                        if ( word[0] != 0 )
                        {
                                strcat( output, word );
                                strcat( output, " " );
                                line = 0;
                                cnt = 0;
                                word[cnt] = 0;
                        }

                        strcat( output, "\r\n" );

                        string += 1;

                        continue;
                }

                if ( *string == '^' || *string == '\\' )
                        line -= 2;
                else if ( *string == '{' || *string == '[' )
                {
                        line--;
                        bBracket = TRUE;
                }
                else if ( *string == '}' || *string == ']' )
                {
                        line--;
                        bBracket = FALSE;
                }

                word[cnt++] = *string;
                word[cnt] = 0;

                if ( *string++ == ' ' && !bBracket )
                {
                        if ( line + cnt > dsock->word_wrap )
                        {
                                int len = strlen( output );

                                line = 0;

                                output[len-1] = 0;
                                strcat( output, "\r\n" );
                        }

                        strcat( output, word );

                        line += cnt;

                        cnt = 0;
                        word[cnt] = 0;
                }
        }

        if ( word[0] != 0 )
        {
                if ( line + cnt > dsock->word_wrap )
                {
                        int len = strlen( output );

                        line = 0;

                        output[len-1] = 0;
                        strcat( output, "\r\n" );
                }

                strcat( output, word );
        }

        int len = strlen( output );

        if ( output[len] == ' ' )
                output[len-1] = 0;

        return output;
}

bool StringEquals( const char *first_string, const char *second_string )
{
        if ( !first_string || !second_string )
                return FALSE;

        while ( *first_string && tolower( *first_string ) == tolower( *second_string ) )
                ++first_string, ++second_string;

        return ( !*first_string && !*second_string );
}

bool StringPrefix( const char *apPart, const char *apWhole )
{
        if ( !apPart || !apWhole )
                return FALSE;

   while ( *apPart && tolower( *apPart ) == tolower( *apWhole ) )
      ++apPart, ++apWhole;

   return ( !*apPart );
}


bool StringSplitEquals( char *aStr, char *bStr )
{
        if ( !aStr || !bStr )
                return FALSE;

        if ( aStr[0] == 0 || bStr[0] == 0 )
                return FALSE;

        char name[MAX_BUFFER], part[MAX_BUFFER];
        char *list, *string;

        name[0] = 0;
        part[0] = 0;

        string = aStr;

        for ( ; ; )
        {
                aStr = one_arg( aStr, part );

                if ( part[0] == 0 )
                        return TRUE;

                list = bStr;

                for ( ; ; )
                {
                        list = one_arg( list, name );

                        if ( name[0] == 0 )
                                return FALSE;

                        if ( StringPrefix( string, name ) )
                                return TRUE;

                        if ( StringPrefix( part, name ) )
                                break;
                }
        }

        return FALSE;
}

const char *OneArgDot( const char *fStr, char *bStr )
{
        while ( isspace( *fStr ) )
                fStr++;

        char argEnd = '.';

        if( *fStr == '\'')
        {
                argEnd = *fStr;
                fStr++;
        }

        while ( *fStr != 0 )
        {
                if ( *fStr == argEnd )
                {
                        fStr++;
                        break;
                }

                *bStr++ = *fStr++;
        }

        *bStr = 0;

        while ( isspace( *fStr ) )
                fStr++;

        return fStr;
}

char *TwoArgs( char *from, char *arg1, char *arg2 ) { return one_arg( one_arg( from, arg1 ), arg2 ); }
char *ThreeArgs( char *from, char *arg1, char *arg2, char *arg3 ) { return one_arg( one_arg( one_arg( from, arg1 ), arg2 ), arg3 ); }

char *OneArgChar( char *fStr, char *bStr, char ch )
{
        while ( isspace( *fStr ) )
                fStr++;

        while ( *fStr != 0 )
        {
                if ( *fStr == ch )
                {
                        fStr++;
                        break;
                }

                *bStr++ = *fStr++;
        }

        *bStr = 0;

        while ( isspace( *fStr ) )
                fStr++;

        return fStr;
}

char *VerboseNumber( int number )
{
        static char buf[MAX_BUFFER];
        static char *num[11] = { "zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten" };
        static char *teen[9] = { "eleven", "twelve", "thirteen", "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen" };
        static char *higher[8] = { "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety" };

        buf[0] = 0;

        if ( number < 0 ) return "error";
        else if ( number < 11 ) strcat( buf, num[number] );
        else if ( number < 20 ) strcat( buf, teen[number-11] );
        else if ( number == 100 ) strcat( buf, "one hundred" );
        else
        {
                int tens = ( number / 10 ) - 2;
                int mod = number % 10;

                if ( mod == 0 ) sprintf( buf, "%s", higher[tens] );
                else sprintf( buf, "%s-%s", higher[tens], num[mod] );
        }

        return buf;
}

