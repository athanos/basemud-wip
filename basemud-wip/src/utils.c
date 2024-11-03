/*
 * This file contains all sorts of utility functions used
 * all sorts of places in the code.
 */
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

/* include main header file */
#include "mud.h"

/*
 * Check to see if a given name is
 * legal, returning FALSE if it
 * fails our high standards...
 */
bool IsPlayer(CHAR_DATA *ch) {
  if(!ch)
    return FALSE;
  return TRUE;
}

int rand_number(int min, int max) {
  if(min > max) {
    log_string("ERROR: rand_number passed a min higher than its max.");
    return 0;
  }

  return min + rand() % (max-min + 1);
}

bool check_name(const char *name)
{
  int size, i;

  if ((size = strlen(name)) < 3 || size > 12)
    return FALSE;

  for (i = 0 ;i < size; i++)
    if (!isalpha(name[i])) return FALSE;

  return TRUE;
}

void clear_char(CHAR_DATA *ch)
{
  memset(ch, 0, sizeof(*ch));
  int i = 0;

  ch->name         =  NULL;
  ch->password     =  NULL;
  ch->level        =  LEVEL_PLAYER;
  ch->events       =  AllocList();
  ch->gender	   = GENDER_NB;
}

void free_char(CHAR_DATA *ch)
{
  EVENT_DATA *pEvent;
  ITERATOR Iter;


  DetachFromList(ch, char_list);


  if (ch->socket) ch->socket->player = NULL;

  AttachIterator(&Iter, ch->events);
  while ((pEvent = (EVENT_DATA *) NextInList(&Iter)) != NULL)
    dequeue_event(pEvent);
  DetachIterator(&Iter);
  FreeList(ch->events);

  /* free allocated memory */
  free(ch->name);
  free(ch->password);
  PushStack(ch, char_free);
}

void communicate(CHAR_DATA *ch, char *txt, int range)
{
  CHAR_DATA *player;
  ITERATOR Iter;
  char buf[MAX_BUFFER];
  char message[MAX_BUFFER];

  switch(range)
  {
    default:
      bug("Communicate: Bad Range %d.", range);
      return;
    case COMM_LOCAL:  /* everyone is in the same room for now... */
      snprintf(message, MAX_BUFFER, "%s says '%s'.\n\r", ch->name, txt);
      snprintf(buf, MAX_BUFFER, "You say '%s'.\n\r", txt);
      text_to_char(ch, buf);
      AttachIterator(&Iter, char_list);
      while ((player = (CHAR_DATA *) NextInList(&Iter)) != NULL)
      {
        if (player == ch) continue;
        text_to_char(player, message);
      }
      DetachIterator(&Iter);
      break;
    case COMM_LOG:
      snprintf(message, MAX_BUFFER, "[LOG: %s]\n\r", txt);
      AttachIterator(&Iter, char_list);
      while ((player = (CHAR_DATA *) NextInList(&Iter)) != NULL)
      {
        if (!IS_ADMIN(player)) continue;
        text_to_char(player, message);
      }
      DetachIterator(&Iter);
      break;
  }
}

/*
 * Loading of help files, areas, etc, at boot time.
 */
void load_muddata(bool fCopyOver)
{  
  load_helps();

  /* copyover */
  if (fCopyOver)
    copyover_recover();
}

char *get_time()
{
  static char buf[16];
  char *strtime;
  int i;

  strtime = ctime(&current_time);
  for (i = 0; i < 15; i++)   
    buf[i] = strtime[i + 4];
  buf[15] = '\0';

  return buf;
}

/* Recover from a copyover - load players */
void copyover_recover()
{     
  CHAR_DATA *ch;
  SOCKET_DATA *sock;
  FILE *fp;
  char name [100];
  char host[MAX_BUFFER];
  int desc;
      
  log_string("Copyover recovery initiated");
   
  if ((fp = fopen(COPYOVER_FILE, "r")) == NULL)
  {  
    log_string("Copyover file not found. Exitting.");
    exit (1);
  }
      
  /* In case something crashes - doesn't prevent reading */
  unlink(COPYOVER_FILE);
    
  for (;;)
  {  
    fscanf(fp, "%d %s %s\n", &desc, name, host);
    if (desc == -1)
      break;

    sock = malloc(sizeof(*sock));
    clear_socket(sock, desc);
  
    sock->hostname     =  strdup(host);
    AttachToList(sock, sock_list);
 
    /* load player data */
    if ((ch = load_player(name)) != NULL)
    {
      /* attach to socket */
      ch->socket     =  sock;
      sock->player    =  ch;
  
      /* attach to char list */
      AttachToList(ch, char_list);

      /* initialize events on the player */
      init_events_player(ch);
    }
    else /* ah bugger */
    {
      close_socket(sock, FALSE);
      continue;
    }
   
    /* Write something, and check if it goes error-free */
    if (!text_to_socket(sock, "\n\r <*>  And before you know it, everything has changed  <*>\n\r"))
    { 
      close_socket(sock, FALSE);
      continue;
    }
  
    /* make sure the socket can be used */
    sock->bust_prompt    =  TRUE;
    sock->lookup_status  =  TSTATE_DONE;
    sock->state          =  STATE_PLAYING;

    /* negotiate compression */
    text_to_buffer(sock, (char *) compress_will2);
    text_to_buffer(sock, (char *) compress_will);
  }
  fclose(fp);
}     

CHAR_DATA *check_reconnect(char *player)
{
  CHAR_DATA *ch;
  ITERATOR Iter;

  AttachIterator(&Iter, char_list);
  while ((ch = (CHAR_DATA *) NextInList(&Iter)) != NULL)
  {
    if (!strcasecmp(ch->name, player))
    {
      if (ch->socket)
        close_socket(ch->socket, TRUE);

      break;
    }
  }
  DetachIterator(&Iter);

  return ch;
}

