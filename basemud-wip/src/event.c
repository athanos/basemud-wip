#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* include main header file */
#include "mud.h"

/* event_game_tick is just to show how to make global events
 * which can be used to update the game.
 */
bool event_game_tick(EVENT_DATA *event)
{
  ITERATOR Iter;
  CHAR_DATA *ch;

  /* send a tick message to everyone */
  AttachIterator(&Iter, char_list);
  while ((ch = (CHAR_DATA *) NextInList(&Iter)) != NULL)
  {
    text_to_char(ch, "Tick!\n\r");
  }
  DetachIterator(&Iter);

  /* enqueue another game tick in 10 minutes */
  event = alloc_event();
  event->fun = &event_game_tick;
  event->type = EVENT_GAME_TICK;
  add_event_game(event, 10 * 60 * PULSES_PER_SECOND);

  return FALSE;
}

bool event_char_save(EVENT_DATA *event)
{
  CHAR_DATA *ch;

  /* Check to see if there is an owner of this event.
   * If there is no owner, we return TRUE, because
   * it's the safest - and post a bug message.
   */
  if ((ch = event->owner.ch) == NULL)
  {
    bug("event_char_save: no owner.");
    return TRUE;
  }

  /* save the actual player file */
  save_player(ch);

  /* enqueue a new event to save the pfile in 2 minutes */
  event = alloc_event();
  event->fun = &event_char_save;
  event->type = EVENT_CHAR_SAVE;
  add_event_char(event, ch, 2 * 60 * PULSES_PER_SECOND);

  return FALSE;
}

bool event_socket_idle(EVENT_DATA *event)
{
  SOCKET_DATA *sock;

  /* Check to see if there is an owner of this event.
   * If there is no owner, we return TRUE, because
   * it's the safest - and post a bug message.
   */
  if ((sock = event->owner.sock) == NULL)
  {
    bug("event_socket_idle: no owner.");
    return TRUE;
  }

  /* tell the socket that it has idled out, and close it */
  text_to_socket(sock, "You have idled out...\n\n\r");
  close_socket(sock, FALSE);

  /* since we closed the socket, all events owned
   * by that socket has been dequeued, and we need
   * to return TRUE, so the caller knows this.
   */
  return TRUE;
}
