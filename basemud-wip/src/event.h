/* event.h
 *
 * This file contains the event data struture, global variables
 * and specially defined values like MAX_EVENT_HASH.
 */

/* the size of the event queue */
#define MAX_EVENT_HASH        128

/* the different types of owners */
#define EVENT_UNOWNED           0
#define EVENT_OWNER_NONE        1
#define EVENT_OWNER_SOCKET     2
#define EVENT_OWNER_CHAR        3
#define EVENT_OWNER_GAME        4

/* the NULL event type */
#define EVENT_NONE              0

/* Char events are given a type value here.
 * Each value should be unique and explicit,
 * besides that, there are no restrictions.
 */
#define EVENT_CHAR_SAVE       1

/* Socket events are given a type value here.
 * Each value should be unique and explicit,
 * besides that, there are no restrictions.
 */
#define EVENT_SOCKET_IDLE       1

/* Game events are given a type value here.
 * Each value should be unique and explicit,
 * besides that, there are no restrictions
 */
#define EVENT_GAME_TICK         1

/* the event prototype */
typedef bool EVENT_FUN ( EVENT_DATA *event );

/* the event structure */
struct event_data
{
  EVENT_FUN        * fun;              /* the function being called           */
  char             * argument;         /* the text argument given (if any)    */
  sh_int             passes;           /* how long before this event executes */
  sh_int             type;             /* event type EVENT_XXX_YYY            */
  sh_int             ownertype;        /* type of owner (unlinking req)       */
  sh_int             bucket;           /* which bucket is this event in       */

  union 
  {                                    /* this is the owner of the event, we  */
    CHAR_DATA       * ch;             /* use a union to make sure any of the */
    SOCKET_DATA     * sock;            /* types can be used for an event.     */
  } owner;
};

/* functions which can be accessed outside event-handler.c */
EVENT_DATA *alloc_event          ( void );
EVENT_DATA *event_isset_socket   ( SOCKET_DATA *sock, int type );
EVENT_DATA *event_isset_char   ( CHAR_DATA *ch, int type );
void dequeue_event               ( EVENT_DATA *event );
void init_event_queue            ( int section );
void init_events_player          ( CHAR_DATA *ch );
void init_events_socket          ( SOCKET_DATA *sock );
void heartbeat                   ( void );
void add_event_char            ( EVENT_DATA *event, CHAR_DATA *ch, int delay );
void add_event_socket            ( EVENT_DATA *event, SOCKET_DATA *sock, int delay );
void add_event_game              ( EVENT_DATA *event, int delay );
void strip_event_socket          ( SOCKET_DATA *sock, int type );
void strip_event_char          ( CHAR_DATA *ch, int type );

/* all events should be defined here */
bool event_char_save           ( EVENT_DATA *event );
bool event_socket_idle           ( EVENT_DATA *event );
bool event_game_tick             ( EVENT_DATA *event );
