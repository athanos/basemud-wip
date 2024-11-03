/*
 * This file handles command interpreting
 */
#include <sys/types.h>
#include <stdio.h>

/* include main header file */
#include "mud.h"

void handle_cmd_input(SOCKET_DATA *sock, char *arg)
{
  CHAR_DATA *ch;
  char command[MAX_BUFFER];
  bool found_cmd = FALSE;
  int i;

  if ((ch = sock->player) == NULL)
    return;

  arg = one_arg(arg, command);

  for (i = 0; tabCmd[i].cmd_name[0] != '\0' && !found_cmd; i++)
  {
    if (tabCmd[i].level > ch->level) continue;

    if (is_prefix(command, tabCmd[i].cmd_name))
    {
      found_cmd = TRUE;
      (*tabCmd[i].cmd_funct)(ch, arg);
    }
  }

  if (!found_cmd)
    text_to_char(ch, "No such command.\n\r");
}

/*
 * The command table, very simple, but easy to extend.
 */
const struct typCmd tabCmd [] =
{

 /* command          function        Req. Level   */
 /* --------------------------------------------- */

  { "commands",      cmd_commands,   LEVEL_GUEST  },
  { "compress",      cmd_compress,   LEVEL_GUEST  },
  { "copyover",      cmd_copyover,   LEVEL_GOD    },
  { "help",          cmd_help,       LEVEL_GUEST  },
  { "linkdead",      cmd_linkdead,   LEVEL_ADMIN  },
  { "say",           cmd_say,        LEVEL_GUEST  },
  { "save",          cmd_save,       LEVEL_GUEST  },
  { "shutdown",      cmd_shutdown,   LEVEL_GOD    },
  { "quit",          cmd_quit,       LEVEL_GUEST  },
  { "who",           cmd_who,        LEVEL_GUEST  },

  /* end of table */
  { "", 0 }
};
