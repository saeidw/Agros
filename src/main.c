/*
 *    AGROS - The new Limited Shell
 *
 *    Author: Joe "rahmu" Hakim Rahme <joe.hakim.rahme@gmail.com>
 *
 *
 *    This file is part of AGROS.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include "agros.h"



int main(int argc, char** argv, char** envp){
    int pid = 0;
    command_t cmd;
    char commandline[MAX_LINE_LEN];
    int should_exit = 0;


    /* Opens a log connection. AGROS relies on underlying Syslog to deal with logging issues.
       Log file manipulations such as compressions, purging or backup are left for the user
       to deal with. Until I find a better way to do it, that is :-) */
    openlog("[AGROS]", LOG_CONS, LOG_USER);

    /*
     *   Main loop:
     *   - print prompt
     *   - read input and parse it
     *   - either a built-in command ("cd", "?" or "exit)
     *   - or a system command, in which case the program forks and executes it with execvp()
     */

    while (!should_exit){
        print_prompt();
        read_input(commandline, MAX_LINE_LEN);
        parse_command(commandline, &cmd);

        switch (get_cmd_code(cmd.name)){
            case EMPTY_CMD:
                break;

            case CD_CMD:
                change_directory(concat_spaces(cmd.argv));
                break;

            case HELP_CMD:
                print_help();
                break;

            case EXIT_CMD:
                return 0;

            case OTHER_CMD:
                pid = fork();
                if (pid == 0){
                    execvp(cmd.name, cmd.argv);
                    fprintf(stderr, "%s: Could not execute command!\nType '?' for help.\n", cmd.name);
                    syslog(LOG_USER, "<%s> %s: Could not execute command. \n", getenv("USER"), cmd.name);
                    should_exit = 1;
                    break;
                }else if (pid < 0){
                    fprintf(stderr, "Error! ... Negative PID. God knows what that means ...\n");
                }else {
                    wait(0);
                }
                break;
        }
    }

    /* Temporary blah solution. This integer should be gotten rid of when dealing with signals. It sucks... */
    should_exit = 0;

    /* Close your log when you're done. Always. */
    closelog();

    return 0;
}
