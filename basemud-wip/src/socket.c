/*
 * This file contains the socket code, used for accepting
 * new connections as well as reading and writing to
 * sockets, and closing down unused sockets.
 */

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>

/* including main header file */
#include "mud.h"

/* global variables */
fd_set     fSet;                  /* the socket list for polling       */
STACK    * sock_free = NULL;     /* the socket free list              */
LIST     * sock_list = NULL;     /* the linked list of active sockets */
STACK    * char_free = NULL;   /* the char free list              */
LIST     * char_list = NULL;   /* the char list of active chars */


/* mccp support */
const unsigned char compress_will   [] = { IAC, WILL, TELOPT_COMPRESS,  '\0' };
const unsigned char compress_will2  [] = { IAC, WILL, TELOPT_COMPRESS2, '\0' };
const unsigned char do_echo         [] = { IAC, WONT, TELOPT_ECHO,      '\0' };
const unsigned char dont_echo       [] = { IAC, WILL, TELOPT_ECHO,      '\0' };

/* local procedures */
void GameLoop         ( int control );
int get_next_index();


/* intialize shutdown state */
bool shut_down = FALSE;
int  control;
int  log_index = 0;

int get_next_index() {
    FILE *file;
    int index;

    // Open the log_index file for reading
    file = fopen("../log/log_index", "r+");
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open log_index file for reading.\n");
        exit(1);
    }

    // Read the index from the file
    if (fscanf(file, "%d", &index) != 1) {
        fprintf(stderr, "Error: Unable to read index from log_index file.\n");
        fclose(file);
        exit(1);
    }

    // Increment the index for the next log file
    index++;

    // Reset the file pointer to the beginning and overwrite the index in the file
    fseek(file, 0, SEEK_SET);
    fprintf(file, "%d", index);
    fclose(file);

    return index;
}


/*
 * This is where it all starts, nothing special.
 */
int main(int argc, char **argv)
{
  bool fCopyOver;

  log_index = get_next_index();

  /* get the current time */
  current_time = time(NULL);

  /* allocate memory for socket and char lists'n'stacks */
  sock_free = AllocStack();
  sock_list = AllocList();
  char_free = AllocStack();
  char_list = AllocList();

  /* note that we are booting up */
  log_string("Program starting.");

  /* initialize the event queue - part 1 */
  init_event_queue(1);

  if (argc > 2 && !strcmp(argv[argc-1], "copyover") && atoi(argv[argc-2]) > 0)
  {
    fCopyOver = TRUE;
    control = atoi(argv[argc-2]);
  }
  else fCopyOver = FALSE;

  /* initialize the socket */
  if (!fCopyOver)
    control = init_socket();

  /* load all external data */
  load_muddata(fCopyOver);

  /* initialize the event queue - part 2*/
  init_event_queue(2);

  /* main game loop */
  GameLoop(control);

  /* close down the socket */
  close(control);

  /* terminated without errors */
  log_string("Program terminated without errors.");

  /* and we are done */
  return 0;
}

void GameLoop(int control)   
{
  SOCKET_DATA *dsock;
  ITERATOR Iter;
  static struct timeval tv;
  struct timeval last_time, new_time;
  extern fd_set fSet;
  fd_set rFd;
  long secs, usecs;

  /* set this for the first loop */
  gettimeofday(&last_time, NULL);

  /* clear out the file socket set */
  FD_ZERO(&fSet);

  /* add control to the set */
  FD_SET(control, &fSet);

  /* copyover recovery */
  AttachIterator(&Iter, sock_list);
  while ((dsock = (SOCKET_DATA *) NextInList(&Iter)) != NULL)
    FD_SET(dsock->control, &fSet);
  DetachIterator(&Iter);

  /* do this untill the program is shutdown */
  while (!shut_down)
  {
    /* set current_time */
    current_time = time(NULL);

    /* copy the socket set */
    memcpy(&rFd, &fSet, sizeof(fd_set));

    /* wait for something to happen */
    if (select(FD_SETSIZE, &rFd, NULL, NULL, &tv) < 0)
      continue;

    /* check for new connections */
    if (FD_ISSET(control, &rFd))
    {
      struct sockaddr_in sock;
      unsigned int socksize;
      int newConnection;

      socksize = sizeof(sock);
      if ((newConnection = accept(control, (struct sockaddr*) &sock, &socksize)) >=0)
        new_socket(newConnection);
    }

    /* poll sockets in the socket list */
    AttachIterator(&Iter ,sock_list);
    while ((dsock = (SOCKET_DATA *) NextInList(&Iter)) != NULL)
    {
      /*
       * Close sockects we are unable to read from.
       */
      if (FD_ISSET(dsock->control, &rFd) && !read_from_socket(dsock))
      {
        close_socket(dsock, FALSE);
        continue;
      }

      /* Ok, check for a new command */
      next_cmd_from_buffer(dsock);

      /* Is there a new command pending ? */
      if (dsock->next_command[0] != '\0')
      {
        /* figure out how to deal with the incoming command */
        switch(dsock->state)
        {
          default:
            bug("Descriptor in bad state.");
            break;
          case STATE_NEW_NAME:
          case STATE_NEW_PASSWORD:
          case STATE_VERIFY_PASSWORD:
          case STATE_ASK_PASSWORD:
	  case STATE_ASK_GENDER:
            handle_new_connections(dsock, dsock->next_command);
            break;
          case STATE_PLAYING:
            handle_cmd_input(dsock, dsock->next_command);
            break;
        }

        dsock->next_command[0] = '\0';
      }

      /* if the player quits or get's disconnected */
      if (dsock->state == STATE_CLOSED) continue;

      /* Send all new data to the socket and close it if any errors occour */
      if (!flush_output(dsock))
        close_socket(dsock, FALSE);
    }
    DetachIterator(&Iter);

    /* call the event queue */
    heartbeat();

    /*
     * Here we sleep out the rest of the pulse, thus forcing
     * SocketMud(tm) to run at PULSES_PER_SECOND pulses each second.
     */
    gettimeofday(&new_time, NULL);

    /* get the time right now, and calculate how long we should sleep */
    usecs = (int) (last_time.tv_usec -  new_time.tv_usec) + 1000000 / PULSES_PER_SECOND;
    secs  = (int) (last_time.tv_sec  -  new_time.tv_sec);

    /*
     * Now we make sure that 0 <= usecs < 1.000.000
     */
    while (usecs < 0)
    {
      usecs += 1000000;
      secs  -= 1;
    }
    while (usecs >= 1000000)
    {
      usecs -= 1000000;
      secs  += 1;
    }

    /* if secs < 0 we don't sleep, since we have encountered a laghole */
    if (secs > 0 || (secs == 0 && usecs > 0))
    {
      struct timeval sleep_time;

      sleep_time.tv_usec = usecs;
      sleep_time.tv_sec  = secs;

      if (select(0, NULL, NULL, NULL, &sleep_time) < 0)
        continue;
    }

    /* reset the last time we where sleeping */
    gettimeofday(&last_time, NULL);

    /* recycle sockets */
    recycle_sockets();
  }
}

/*
 * Init_socket()
 *
 * Used at bootup to get a free
 * socket to run the server from.
 */
int init_socket()
{
  struct sockaddr_in my_addr;
  int sockfd, reuse = 1;

  /* let's grab a socket */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  /* setting the correct values */
  my_addr.sin_family = AF_INET;
  my_addr.sin_addr.s_addr = INADDR_ANY;
  my_addr.sin_port = htons(MUDPORT);

  /* this actually fixes any problems with threads */
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1)
  {
    perror("Error in setsockopt()");
    exit(1);
  } 

  /* bind the port */
  bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));

  /* start listening already :) */
  listen(sockfd, 3);

  /* return the socket */
  return sockfd;
}

/* 
 * New_socket()
 *
 * Initializes a new socket, get's the hostname
 * and puts it in the active socket_list.
 */
bool new_socket(int sock)
{
  struct sockaddr_in   sock_addr;
  pthread_attr_t       attr;
  pthread_t            thread_lookup;
  LOOKUP_DATA        * lData;
  SOCKET_DATA        * sock_new;
  int                  argp = 1;
  socklen_t            size;

  /* initialize threads */
  pthread_attr_init(&attr);   
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  /*
   * allocate some memory for a new socket if
   * there is no free socket in the free_list
   */
  if (StackSize(sock_free) <= 0)
  {
    if ((sock_new = malloc(sizeof(*sock_new))) == NULL)
    {
      bug("New_socket: Cannot allocate memory for socket.");
      abort();
    }
  }
  else
  {
    sock_new = (SOCKET_DATA *) PopStack(sock_free);
  }

  /* attach the new connection to the socket list */
  FD_SET(sock, &fSet);

  /* clear out the socket */
  clear_socket(sock_new, sock);

  /* set the socket as non-blocking */
  ioctl(sock, FIONBIO, &argp);

  /* update the linked list of sockets */
  AttachToList(sock_new, sock_list);

  /* do a host lookup */
  size = sizeof(sock_addr);
  if (getpeername(sock, (struct sockaddr *) &sock_addr, &size) < 0)
  {
    perror("New_socket: getpeername");
    sock_new->hostname = strdup("unknown");
  }
  else
  {
    /* set the IP number as the temporary hostname */
    sock_new->hostname = strdup(inet_ntoa(sock_addr.sin_addr));

    if (strcasecmp(sock_new->hostname, "127.0.0.1"))
    {
      /* allocate some memory for the lookup data */
      if ((lData = malloc(sizeof(*lData))) == NULL)
      {
        bug("New_socket: Cannot allocate memory for lookup data.");
        abort();
      }

      /* Set the lookup_data for use in lookup_address() */
      lData->buf    =  strdup((char *) &sock_addr.sin_addr);
      lData->sock  =  sock_new;

      /* dispatch the lookup thread */
      pthread_create(&thread_lookup, &attr, &lookup_address, (void*) lData);
    }
    else sock_new->lookup_status++;
  }

  /* negotiate compression */
  text_to_buffer(sock_new, (char *) compress_will2);
  text_to_buffer(sock_new, (char *) compress_will);

  /* send the greeting */
  text_to_buffer(sock_new,"\033c");
  text_to_buffer(sock_new, greeting);
  text_to_buffer(sock_new, "What is your name? ");

  /* initialize socket events */
  init_events_socket(sock_new);

  /* everything went as it was supposed to */
  return TRUE;
}

/*
 * Close_socket()
 *
 * Will close one socket directly, freeing all
 * resources and making the socket availably on
 * the socket free_list.
 */
void close_socket(SOCKET_DATA *dsock, bool reconnect)
{
  EVENT_DATA *pEvent;
  ITERATOR Iter;

  if (dsock->lookup_status > TSTATE_DONE) return;
  dsock->lookup_status += 2;

  /* remove the socket from the polling list */
  FD_CLR(dsock->control, &fSet);

  if (dsock->state == STATE_PLAYING)
  {
    if (reconnect)
      text_to_socket(dsock, "This connection has been taken over.\n\r");
    else if (dsock->player)
    {
      dsock->player->socket = NULL;
      log_string("Closing link to %s", dsock->player->name);
    }
  }
  else if (dsock->player)
    free_char(dsock->player);

  /* dequeue all events for this socket */
  AttachIterator(&Iter, dsock->events);
  while ((pEvent = (EVENT_DATA *) NextInList(&Iter)) != NULL)
    dequeue_event(pEvent);
  DetachIterator(&Iter);

  /* set the closed state */
  dsock->state = STATE_CLOSED;
}

/* 
 * Read_from_socket()
 *
 * Reads one line from the socket, storing it
 * in a buffer for later use. Will also close
 * the socket if it tries a buffer overflow.
 */
bool read_from_socket(SOCKET_DATA *dsock)
{
  int size;
  extern int errno;

  /* check for buffer overflows, and drop connection in that case */
  size = strlen(dsock->inbuf);
  if (size >= sizeof(dsock->inbuf) - 2)
  {
    text_to_socket(dsock, "\n\r!!!! Input Overflow !!!!\n\r");
    return FALSE;
  }

  /* start reading from the socket */
  for (;;)
  {
    int sInput;
    int wanted = sizeof(dsock->inbuf) - 2 - size;

    sInput = read(dsock->control, dsock->inbuf + size, wanted);

    if (sInput > 0)
    {
      size += sInput;

      if (dsock->inbuf[size-1] == '\n' || dsock->inbuf[size-1] == '\r')
        break;
    }
    else if (sInput == 0)
    {
      log_string("Read_from_socket: EOF");
      return FALSE;
    }
    else if (errno == EAGAIN || sInput == wanted)
      break;
    else
    {
      perror("Read_from_socket");
      return FALSE;
    }     
  }
  dsock->inbuf[size] = '\0';
  return TRUE;
}

/*
 * Text_to_socket()
 *
 * Sends text directly to the socket,
 * will compress the data if needed.
 */
bool text_to_socket(SOCKET_DATA *dsock, const char *txt)
{
  int iBlck, iPtr, iWrt = 0, length, control = dsock->control;

  length = strlen(txt);

  /* write compressed */
  if (dsock && dsock->out_compress)
  {
    dsock->out_compress->next_in  = (unsigned char *) txt;
    dsock->out_compress->avail_in = length;

    while (dsock->out_compress->avail_in)
    {
      dsock->out_compress->avail_out = COMPRESS_BUF_SIZE - (dsock->out_compress->next_out - dsock->out_compress_buf);

      if (dsock->out_compress->avail_out)
      {
        int status = deflate(dsock->out_compress, Z_SYNC_FLUSH);

        if (status != Z_OK)
        return FALSE;
      }

      length = dsock->out_compress->next_out - dsock->out_compress_buf;
      if (length > 0)
      {
        for (iPtr = 0; iPtr < length; iPtr += iWrt)
        {
          iBlck = UMIN(length - iPtr, 4096);
          if ((iWrt = write(control, dsock->out_compress_buf + iPtr, iBlck)) < 0)
          {
            perror("Text_to_socket (compressed):");
            return FALSE;
          }
        }
        if (iWrt <= 0) break;
        if (iPtr > 0)
        {
          if (iPtr < length)
            memmove(dsock->out_compress_buf, dsock->out_compress_buf + iPtr, length - iPtr);

          dsock->out_compress->next_out = dsock->out_compress_buf + length - iPtr;
        }
      }
    }
    return TRUE;
  }

  /* write uncompressed */
  for (iPtr = 0; iPtr < length; iPtr += iWrt)
  {
    iBlck = UMIN(length - iPtr, 4096);
    if ((iWrt = write(control, txt + iPtr, iBlck)) < 0)
    {
      perror("Text_to_socket:");
      return FALSE;
    }
  }

  return TRUE;
}

/*
 * Text_to_buffer()
 *
 * Stores outbound text in a buffer, where it will
 * stay untill it is flushed in the gameloop.
 *
 * Will also parse ANSI colors and other tags.
 */
void text_to_buffer(SOCKET_DATA *dsock, const char *text, ...)
{
  char buf[MAX_OUTPUT];
  va_list args;

  va_start (args, text);
  vsnprintf(buf,MAX_OUTPUT, text, args);
  va_end(args);

  int color = 256;

//  if(dsock->top_output == 0) {
//    strcpy(dsock->outbuf,"\n\r");
//    dsock->top_output += 2;
//  }

//  dsock->top_output += substitute_color((char *) txt, dsock->outbuf + dsock->top_output, color);
  dsock->top_output += substitute_color((char *) buf, dsock->outbuf + dsock->top_output, color);
  dsock->top_output += sprintf(dsock->outbuf + dsock->top_output, "\033[0m");

  return;
}

/*
 * Text_to_char()
 *
 * If the char has a socket, then the data will
 * be send to text_to_buffer().
 */
void text_to_char(CHAR_DATA *ch, const char *txt, ...)
{
  char buf[MAX_OUTPUT];
  va_list args;

  va_start (args, txt);
  vsnprintf(buf,MAX_OUTPUT, txt, args);
  va_end(args);

  if (ch->socket)
  {
    text_to_buffer(ch->socket, buf);
    ch->socket->bust_prompt = TRUE;
  }
}

void next_cmd_from_buffer(SOCKET_DATA *dsock)
{
  int size = 0, i = 0, j = 0, telopt = 0;

  /* if theres already a command ready, we return */
  if (dsock->next_command[0] != '\0')
    return;

  /* if there is nothing pending, then return */
  if (dsock->inbuf[0] == '\0')
    return;

  /* check how long the next command is */
  while (dsock->inbuf[size] != '\0' && dsock->inbuf[size] != '\n' && dsock->inbuf[size] != '\r')
    size++;

  /* we only deal with real commands */
  if (dsock->inbuf[size] == '\0')
    return;

  /* copy the next command into next_command */
  for ( ; i < size; i++)
  {
    if (dsock->inbuf[i] == (signed char) IAC)
    {
      telopt = 1;
    }
    else if (telopt == 1 && (dsock->inbuf[i] == (signed char) DO || dsock->inbuf[i] == (signed char) DONT))
    {
      telopt = 2;
    }
    else if (telopt == 2)
    {
      telopt = 0;

      if (dsock->inbuf[i] == (signed char) TELOPT_COMPRESS)         /* check for version 1 */
      {
        if (dsock->inbuf[i-1] == (signed char) DO)                  /* start compressing   */
          compressStart(dsock, TELOPT_COMPRESS);
        else if (dsock->inbuf[i-1] == (signed char) DONT)           /* stop compressing    */
          compressEnd(dsock, TELOPT_COMPRESS, FALSE);
      }
      else if (dsock->inbuf[i] == (signed char) TELOPT_COMPRESS2)   /* check for version 2 */
      {
        if (dsock->inbuf[i-1] == (signed char) DO)                  /* start compressing   */
          compressStart(dsock, TELOPT_COMPRESS2);
        else if (dsock->inbuf[i-1] == (signed char) DONT)           /* stop compressing    */
          compressEnd(dsock, TELOPT_COMPRESS2, FALSE);
      }
    }
    else if (isprint(dsock->inbuf[i]) && isascii(dsock->inbuf[i]))
    {
      dsock->next_command[j++] = dsock->inbuf[i];
    }
  }
  dsock->next_command[j] = '\0';

  /* skip forward to the next line */
  while (dsock->inbuf[size] == '\n' || dsock->inbuf[size] == '\r')
  {
    dsock->bust_prompt = TRUE;   /* seems like a good place to check */
    size++;
  }

  /* use i as a static pointer */
  i = size;

  /* move the context of inbuf down */
  while (dsock->inbuf[size] != '\0')
  {
    dsock->inbuf[size - i] = dsock->inbuf[size];
    size++;
  }
  dsock->inbuf[size - i] = '\0';
}

bool flush_output(SOCKET_DATA *dsock)
{
  BUFFER *buf = buffer_new(MAX_BUFFER);
  /* nothing to send */
  if (dsock->top_output <= 0 && !(dsock->bust_prompt && dsock->state == STATE_PLAYING))
    return TRUE;

  /* bust a prompt */
  if (dsock->state == STATE_PLAYING && dsock->bust_prompt && dsock->pString)
  {
    text_to_buffer(dsock, ">");
    dsock->bust_prompt = FALSE;
  }
  else if (dsock->state == STATE_PLAYING && dsock->bust_prompt)
  {
    bprintf(buf,"\n\r%s:> ", dsock->player->name);
    text_to_buffer(dsock, buf->data);
    buffer_free(buf);
    dsock->bust_prompt = FALSE;
  }

  /* reset the top pointer */
  dsock->top_output = 0;

  /*
   * Send the buffer, and return FALSE
   * if the write fails.
   */
  if (!text_to_socket(dsock, dsock->outbuf))
    return FALSE;

  /* Success */
  return TRUE;
}

void display_gender(SOCKET_DATA *dsock)
{
  int i = 10;

  text_to_buffer(dsock, "\033c");
  text_to_buffer(dsock, "^AA^W) ^jMale %d\n\r", i);
  text_to_buffer(dsock, "^AB^W) ^jFemale\n\r");
  text_to_buffer(dsock, "^AC^W) ^jNon-Binary\n\r");
  text_to_buffer(dsock, "\n\r\n\r^wPlease enter your choice: ");
}


void handle_new_connections(SOCKET_DATA *dsock, char *arg)
{
  CHAR_DATA *p_new;
  int i;
  BUFFER *buf = buffer_new(MAX_BUFFER);

  switch(dsock->state)
  {
    default:
      bug("Handle_new_connections: Bad state.");
      break;
    case STATE_NEW_NAME:
      if (dsock->lookup_status != TSTATE_DONE)
      {
        text_to_buffer(dsock, "Making a dns lookup, please have patience.\n\rWhat is your name? ");
        return;
      }
      if (!check_name(arg)) /* check for a legal name */
      {
        text_to_buffer(dsock, "Sorry, that's not a legal name, please pick another.\n\rWhat is your name? ");
        break;
      }
      arg[0] = toupper(arg[0]);
      log_string("%s is trying to connect.", arg);

      /* Check for a new Player */
      if ((p_new = load_profile(arg)) == NULL)
      {
        if (StackSize(char_free) <= 0)
        {
          if ((p_new = malloc(sizeof(*p_new))) == NULL)
          {
            bug("Handle_new_connection: Cannot allocate memory.");
            abort();
          }
        }
        else
        {
          p_new = (CHAR_DATA *) PopStack(char_free);
        }
        clear_char(p_new);

        /* give the player it's name */
        p_new->name = strdup(arg);

        /* prepare for next step */
        text_to_buffer(dsock, "Please enter a new password: ");
        dsock->state = STATE_NEW_PASSWORD;
      }
      else /* old player */
      {
        /* prepare for next step */
        text_to_buffer(dsock, "What is your password? ");
        dsock->state = STATE_ASK_PASSWORD;
      }
      text_to_buffer(dsock, (char *) dont_echo);

      /* socket <-> player */
      p_new->socket = dsock;
      dsock->player = p_new;
      break;
    case STATE_NEW_PASSWORD:
      if (strlen(arg) < 5 || strlen(arg) > 12)
      {
        text_to_buffer(dsock, "Between 5 and 12 chars please!\n\rPlease enter a new password: ");
        return;
      }

      free(dsock->player->password);
      dsock->player->password = strdup(crypt(arg, dsock->player->name));

      for (i = 0; dsock->player->password[i] != '\0'; i++)
      {
	if (dsock->player->password[i] == '~')
	{
	  text_to_buffer(dsock, "Illegal password!\n\rPlease enter a new password: ");
	  return;
	}
      }

      text_to_buffer(dsock, "\n\rPlease verify the password: ");
      dsock->state = STATE_VERIFY_PASSWORD;
      break;
    case STATE_VERIFY_PASSWORD:
      if (!strcmp(crypt(arg, dsock->player->name), dsock->player->password))
      {
          dsock->state = STATE_ASK_GENDER;
          display_gender(dsock);
      }
      else
      {
        free(dsock->player->password);
        dsock->player->password = NULL;
        text_to_buffer(dsock, "Password mismatch!\n\rPlease enter a new password: ");
        dsock->state = STATE_NEW_PASSWORD;
      }
      break;
    case STATE_ASK_GENDER:
      arg[0] = toupper(arg[0]);
      switch(arg[0]){
        case 'A':
          dsock->player->gender = GENDER_MALE;
          dsock->state = STATE_INTO_GAME;
          break;
        case 'B':
          dsock->player->gender = GENDER_FEMALE;
          dsock->state = STATE_INTO_GAME;
          break;
        case 'C':
          dsock->player->gender = GENDER_NB;
          dsock->state = STATE_INTO_GAME;
          break;
        default:
          display_gender(dsock);
          break;
      }
	dsock->state = STATE_INTO_GAME;
	        if (!strcasecmp(dsock->player->name, "Athanos")) {
          dsock->player->level = LEVEL_GOD;
        }

        text_to_buffer(dsock, (char *) do_echo);

        /* put him in the list */
        AttachToList(dsock->player, char_list);

        log_string("New player: %s has entered the game.", dsock->player->name);
	save_player(dsock->player);
        /* and into the game */
        dsock->state = STATE_PLAYING;
        text_to_buffer(dsock, "\033c");
        text_to_buffer(dsock, motd);
        /* initialize events on the player */
        init_events_player(dsock->player);

        /* strip the idle event from this socket */
        strip_event_socket(dsock, EVENT_SOCKET_IDLE);
      break;
    case STATE_ASK_PASSWORD:
      text_to_buffer(dsock, (char *) do_echo);
      if (!strcmp(crypt(arg, dsock->player->name), dsock->player->password))
      {
        if ((p_new = check_reconnect(dsock->player->name)) != NULL)
        {
          /* attach the new player */
          free_char(dsock->player);
          dsock->player = p_new;
          p_new->socket = dsock;
          log_string("%s has reconnected.", dsock->player->name);

          /* and let him enter the game */
          dsock->state = STATE_PLAYING;
          text_to_buffer(dsock, "You take over a body already in use.\n\r");

          /* strip the idle event from this socket */
          strip_event_socket(dsock, EVENT_SOCKET_IDLE);
        }
        else if ((p_new = load_player(dsock->player->name)) == NULL)
        {
          text_to_socket(dsock, "ERROR: Your pfile is missing!\n\r");
          free_char(dsock->player);
          dsock->player = NULL;
          close_socket(dsock, FALSE);
          return;
        }
        else
        {
          /* attach the new player */
          free_char(dsock->player);
          dsock->player = p_new;
          p_new->socket = dsock;

          /* put him in the active list */
          AttachToList(p_new, char_list);
          log_string("%s has entered the game.", dsock->player->name);

          /* and let him enter the game */
          dsock->state = STATE_PLAYING;
	  text_to_buffer(dsock, "\033c");
          text_to_buffer(dsock, motd);

	  /* initialize events on the player */
	  init_events_player(dsock->player);

	  /* strip the idle event from this socket */
	  strip_event_socket(dsock, EVENT_SOCKET_IDLE);
        }
      }
      else
      {
        text_to_socket(dsock, "Bad password!\n\r");
        free_char(dsock->player);
        dsock->player = NULL;
        close_socket(dsock, FALSE);
      }
      break;
  }
}

void clear_socket(SOCKET_DATA *sock_new, int sock)
{
  memset(sock_new, 0, sizeof(*sock_new));

  sock_new->control        =  sock;
  sock_new->state          =  STATE_NEW_NAME;
  sock_new->lookup_status  =  TSTATE_LOOKUP;
  sock_new->player         =  NULL;
  sock_new->top_output     =  0;
  sock_new->events         =  AllocList();
  sock_new->pEdit	   =  NULL; //Editor
  sock_new->pString	   =  NULL; //Editor
  sock_new->editor	   =  0;
}

/* does the lookup, changes the hostname, and dies */
void *lookup_address(void *arg)
{
  LOOKUP_DATA *lData = (LOOKUP_DATA *) arg;
  struct hostent *from = 0;
  struct hostent ent;
  char buf[16384];
  int err;

  /* do the lookup and store the result at &from */
//  gethostbyaddr_r(lData->buf, sizeof(lData->buf), AF_INET, &ent, buf, 16384, &from, &err);

  /* did we get anything ? */
  if (from && from->h_name)
  {
    free(lData->sock->hostname);
    lData->sock->hostname = strdup(from->h_name);
  }

  /* set it ready to be closed or used */
  lData->sock->lookup_status++;

  /* free the lookup data */
  free(lData->buf);
  free(lData);

  /* and kill the thread */
  pthread_exit(0);
}

void recycle_sockets()
{
  SOCKET_DATA *dsock;
  ITERATOR Iter;

  AttachIterator(&Iter, sock_list);
  while ((dsock = (SOCKET_DATA *) NextInList(&Iter)) != NULL)
  {
    if (dsock->lookup_status != TSTATE_CLOSED) continue;

    /* remove the socket from the socket list */
    DetachFromList(dsock, sock_list);

    /* close the socket */
    close(dsock->control);

    /* free the memory */
    free(dsock->hostname);

    /* free the list of events */
    FreeList(dsock->events);

    /* stop compression */
    compressEnd(dsock, dsock->compressing, TRUE);

    /* put the socket in the free stack */
    PushStack(dsock, sock_free);
  }
  DetachIterator(&Iter);
}
