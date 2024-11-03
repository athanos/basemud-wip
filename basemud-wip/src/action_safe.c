/*
 * This file handles non-fighting player actions.
 */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* include main header file */
#include "mud.h"

void cmd_say(CHAR_DATA *ch, char *arg)
{
  if (arg[0] == '\0')
  {
    text_to_char(ch, "Say what?\n\r");
    return;
  }
  communicate(ch, arg, COMM_LOCAL);
}

void cmd_quit(CHAR_DATA *ch, char *arg)
{
  char buf[MAX_BUFFER];

  /* log the attempt */
  snprintf(buf, MAX_BUFFER, "%s has left the game.", ch->name);
  log_string(buf);

  save_player(ch);

  ch->socket->player = NULL;
  free_char(ch);
  close_socket(ch->socket, FALSE);
}

void cmd_shutdown(CHAR_DATA *ch, char *arg)
{
  shut_down = TRUE;
}

void cmd_commands(CHAR_DATA *ch, char *arg)
{
  BUFFER *buf = buffer_new(MAX_BUFFER);
  int i, col = 0;

  bprintf(buf, "    - - - - ----==== The full command list ====---- - - - -\n\n\r");
  for (i = 0; tabCmd[i].cmd_name[0] != '\0'; i++)
  {
    if (ch->level < tabCmd[i].level) continue;

    bprintf(buf, " %-16.16s", tabCmd[i].cmd_name);
    if (!(++col % 4)) bprintf(buf, "\n\r");
  }
  if (col % 4) bprintf(buf, "\n\r");
  text_to_char(ch, buf->data);
  buffer_free(buf);
}

void cmd_who(CHAR_DATA *ch, char *arg)
{
  CHAR_DATA *player;
  SOCKET_DATA *sock;
  ITERATOR Iter;
  BUFFER *buf = buffer_new(MAX_BUFFER);

  bprintf(buf, "^A - - - - ----==== Who's Online ====---- - - - -^j\n\r");

  AttachIterator(&Iter, sock_list);
  while ((sock = (SOCKET_DATA *) NextInList(&Iter)) != NULL)
  {
    if (sock->state != STATE_PLAYING) continue;
    if ((player = sock->player) == NULL) continue;

    bprintf(buf, " %-12s   %s\n\r", player->name, sock->hostname);
  }
  DetachIterator(&Iter);

  bprintf(buf, "^A - - - - ----======================---- - - - -^w\n\r");
  text_to_char(ch, buf->data);

  buffer_free(buf);
}

void cmd_help(CHAR_DATA *ch, char *arg)
{
  if (arg[0] == '\0')
  {
    HELP_DATA *pHelp;
    ITERATOR Iter;
    BUFFER *buf = buffer_new(MAX_BUFFER);
    int col = 0;

    bprintf(buf, "      - - - - - ----====//// HELP FILES  \\\\\\\\====---- - - - - -\n\n\r");

    AttachIterator(&Iter, help_list);
    while ((pHelp = (HELP_DATA *) NextInList(&Iter)) != NULL)
    {
      bprintf(buf, " %-19.18s", pHelp->keyword);
      if (!(++col % 4)) bprintf(buf, "\n\r");
    }
    DetachIterator(&Iter);

    if (col % 4) bprintf(buf, "\n\r");
    bprintf(buf, "\n\r Syntax: help <topic>\n\r");
    text_to_char(ch, buf->data);
    buffer_free(buf);

    return;
  }

  if (!check_help(ch, arg))
    text_to_char(ch, "Sorry, no such helpfile.\n\r");
}

void cmd_compress(CHAR_DATA *ch, char *arg)
{
  /* no socket, no compression */
  if (!ch->socket)
    return;

  /* enable compression */
  if (!ch->socket->out_compress)
  {
    text_to_char(ch, "Trying compression.\n\r");
    text_to_buffer(ch->socket, (char *) compress_will2);
    text_to_buffer(ch->socket, (char *) compress_will);
  }
  else /* disable compression */
  {
    if (!compressEnd(ch->socket, ch->socket->compressing, FALSE))
    {
      text_to_char(ch, "Failed.\n\r");
      return;
    }
    text_to_char(ch, "Compression disabled.\n\r");
  }
}

void cmd_save(CHAR_DATA *ch, char *arg)
{
  save_player(ch);
  text_to_char(ch, "Saved.\n\r");
}

void cmd_copyover(CHAR_DATA *ch, char *arg)
{ 
  FILE *fp;
  ITERATOR Iter;
  SOCKET_DATA *sock;
  char buf[MAX_BUFFER];
  
  if ((fp = fopen(COPYOVER_FILE, "w")) == NULL)
  {
    text_to_char(ch, "Copyover file not writeable, aborted.\n\r");
    return;
  }

  strncpy(buf, "\n\r <*>            The world starts spinning             <*>\n\r", MAX_BUFFER);

  /* For each playing descriptor, save its state */
  AttachIterator(&Iter, sock_list);
  while ((sock = (SOCKET_DATA *) NextInList(&Iter)) != NULL)
  {
    compressEnd(sock, sock->compressing, FALSE);

    if (sock->state != STATE_PLAYING)
    {
      text_to_socket(sock, "\n\rSorry, we are rebooting. Come back in a few minutes.\n\r");
      close_socket(sock, FALSE);
    }
    else
    {
      fprintf(fp, "%d %s %s\n",
        sock->control, sock->player->name, sock->hostname);

      /* save the player */
      save_player(sock->player);

      text_to_socket(sock, buf);
    }
  }
  DetachIterator(&Iter);

  fprintf (fp, "-1\n");
  fclose (fp);

  /* close any pending sockets */
  recycle_sockets();
  
  /*
   * feel free to add any additional arguments between the 2nd and 3rd,
   * that is "SocketMud" and buf, but leave the last three in that order,
   * to ensure that the main() function can parse the input correctly.
   */
  snprintf(buf, MAX_BUFFER, "%d", control);
  execl(EXE_FILE, "basemud", buf, "copyover", (char *) NULL);

  /* Failed - sucessful exec will not return */
  text_to_char(ch, "Copyover FAILED!\n\r");
}

void cmd_linkdead(CHAR_DATA *ch, char *arg)
{
  CHAR_DATA *player;
  ITERATOR Iter;
  char buf[MAX_BUFFER];
  bool found = FALSE;

  AttachIterator(&Iter, char_list);
  while ((player = (CHAR_DATA *) NextInList(&Iter)) != NULL)
  {
    if (!player->socket)
    {
      snprintf(buf, MAX_BUFFER, "%s is linkdead.\n\r", player->name);
      text_to_char(ch, buf);
      found = TRUE;
    }
  }
  DetachIterator(&Iter);

  if (!found)
    text_to_char(ch, "Noone is currently linkdead.\n\r");
}

