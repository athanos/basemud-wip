#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* main header file */
#include "mud.h"

void save_pfile           ( CHAR_DATA *ch );
void save_profile         ( CHAR_DATA *ch );

void save_player(CHAR_DATA *ch)
{
  if (!ch) return;

  save_pfile(ch);      /* saves the actual player data */
  save_profile(ch);    /* saves the players profile    */
}

void save_pfile(CHAR_DATA *ch)
{
  char pName[MAX_BUFFER];
  char pfile[MAX_BUFFER];
  FILE *fp;
  int size, i;

  pName[0] = toupper(ch->name[0]);
  size = strlen(ch->name);
  for (i = 1; i < size && i < MAX_BUFFER - 1; i++)
    pName[i] = tolower(ch->name[i]);
  pName[i] = '\0';

  /* open the pfile so we can write to it */
  snprintf(pfile, MAX_BUFFER, "../players/%s.pfile", pName);
  if ((fp = fopen(pfile, "w")) == NULL)
  {
    bug("Unable to write to %s's pfile", ch->name);
    return;
  }

  /* dump the players data into the file */
  fprintf(fp, "Name            %s~\n", ch->name);
  fprintf(fp, "Level           %d\n",  ch->level);
  fprintf(fp, "Password        %s~\n", ch->password);
  fprintf(fp, "Gender          %d\n", ch->gender);
  /* terminate the file */
  fprintf(fp, "%s\n", FILE_TERMINATOR);
  fclose(fp);
}

CHAR_DATA *load_player(char *player)
{
  FILE *fp;
  CHAR_DATA *ch = NULL;
  char pfile[MAX_BUFFER];
  char pName[MAX_BUFFER];
  char *word;
  bool done = FALSE, found;
  int i, size;

  pName[0] = toupper(player[0]);
  size = strlen(player);
  for (i = 1; i < size && i < MAX_BUFFER - 1; i++)
    pName[i] = tolower(player[i]);
  pName[i] = '\0';

  /* open the pfile so we can write to it */
  snprintf(pfile, MAX_BUFFER, "../players/%s.pfile", pName);     
  if ((fp = fopen(pfile, "r")) == NULL)
    return NULL;

  /* create new char data */
  if (StackSize(char_free) <= 0)
  {
    if ((ch = malloc(sizeof(*ch))) == NULL)
    {
      bug("Load_player: Cannot allocate memory.");
      abort();
    }
  }
  else
  {
    ch = (CHAR_DATA *) PopStack(char_free);
  }
  clear_char(ch);

  /* load data */
  word = fread_word(fp);
  while (!done)
  {
    found = FALSE;
    switch (word[0])
    {
      case 'E':
        if (!strcasecmp(word, "EOF")) {done = TRUE; found = TRUE; break;}
        break;
      case 'G':
	IREAD("Gender", ch->gender);
	break;
      case 'L':
        IREAD( "Level",     ch->level     );
        break;
      case 'N':
        SREAD( "Name",      ch->name      );
        break;
      case 'P':
        SREAD( "Password",  ch->password  );
        break;
    }
    if (!found)
    {
      bug("Load_player: unexpected '%s' in %s's pfile.", word, player);
      free_char(ch);
      return NULL;
    }

    /* read one more */
    if (!done) word = fread_word(fp);
  }

  fclose(fp);
  return ch;
}

/*
 * This function loads a players profile, and stores
 * it in a char_data... DO NOT USE THIS DATA FOR
 * ANYTHING BUT CHECKING PASSWORDS OR SIMILAR.
 */
CHAR_DATA *load_profile(char *player)
{
  FILE *fp;
  CHAR_DATA *ch = NULL;
  char pfile[MAX_BUFFER];
  char pName[MAX_BUFFER];
  char *word;
  bool done = FALSE, found;
  int i, size;

  pName[0] = toupper(player[0]);
  size = strlen(player);
  for (i = 1; i < size && i < MAX_BUFFER - 1; i++)
    pName[i] = tolower(player[i]);
  pName[i] = '\0';

  /* open the pfile so we can write to it */
  snprintf(pfile, MAX_BUFFER, "../players/%s.profile", pName);
  if ((fp = fopen(pfile, "r")) == NULL)
    return NULL;

  /* create new char data */
  if (StackSize(char_free) <= 0)
  {
    if ((ch = malloc(sizeof(*ch))) == NULL)
    {
      bug("Load_profile: Cannot allocate memory.");
      abort();
    }
  }
  else
  {
    ch = (CHAR_DATA *) PopStack(char_free);
  }
  clear_char(ch);

  /* load data */
  word = fread_word(fp);
  while (!done)
  {
    found = FALSE;
    switch (word[0])
    {
      case 'E':
        if (!strcasecmp(word, "EOF")) {done = TRUE; found = TRUE; break;}
        break;
      case 'N':
        SREAD( "Name",      ch->name      );
        break;
      case 'P':
        SREAD( "Password",  ch->password  );
        break;
    }
    if (!found)
    {
      bug("Load_player: unexpected '%s' in %s's pfile.", word, player);
      free_char(ch);
      return NULL;
    }

    /* read one more */
    if (!done) word = fread_word(fp);
  }

  fclose(fp);
  return ch;
}


/*
 * This file stores only data vital to load
 * the character, and check for things like
 * password and other such data.
 */
void save_profile(CHAR_DATA *ch)
{
  char pfile[MAX_BUFFER];
  char pName[MAX_BUFFER];
  FILE *fp;
  int size, i;
  
  pName[0] = toupper(ch->name[0]);
  size = strlen(ch->name);
  for (i = 1; i < size && i < MAX_BUFFER - 1; i++)
    pName[i] = tolower(ch->name[i]);
  pName[i] = '\0';
  
  /* open the pfile so we can write to it */
  snprintf(pfile, MAX_BUFFER, "../players/%s.profile", pName);
  if ((fp = fopen(pfile, "w")) == NULL)
  {
    bug("Unable to write to %s's pfile", ch->name);
    return;
  }

  /* dump the players data into the file */
  fprintf(fp, "Name           %s~\n", ch->name);
  fprintf(fp, "Password       %s~\n", ch->password);

  /* terminate the file */
  fprintf(fp, "%s\n", FILE_TERMINATOR);
  fclose(fp);
}
