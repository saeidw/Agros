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


#define MAX_LINE_LEN 256
#define MAX_ARGS (MAX_LINE_LEN/2)
#define WHITESPACE " \t\n"

#define OTHER_CMD   0
#define EMPTY_CMD   1
#define CD_CMD      2
#define HELP_CMD    3
#define ENV_CMD     4
#define EXIT_CMD    5

#define CMD_NBR     6

#define AG_FALSE 0
#define AG_TRUE  1


/*
 * This structure allows me to handle built-in commands with a switch statement
 * instead of multiple ifs. Since I cannot switch on a string, I associate a
 * command_code to each built-in command. The codes are define as preprocessor
 * instructions.
 */

typedef struct built_in_commands built_in_commands;
struct built_in_commands{
    char command_name[MAX_LINE_LEN];
    int command_code;
};

/*
 * This structure holds user input. 3 fields:
 *   - argv: an array of strings. Each word of the input is a case of the array.
 *   - name: the name of the executable called. By default it's argv[0]
 *   - argc: the number of words given in input. It's equivalent to the length of argv.
 *
 * Note: Some filenames have a space character (" ") in their names. I will have
 * to deal with that properly ... someday.
 */

typedef struct command_t command_t;
struct command_t{		
    char* name;
    int argc;
    char* argv[MAX_ARGS+1];
};


/*
 * A structure that holds the AGROS conf.
 */

typedef struct config_t config_t;
struct config_t{
    char** allowed_list;
    char** forbidden_list;
    int allowed_nbr;
    int forbidden_nbr;
    char* welcome_message;
    int loglevel;
    int warnings;
};

/*
 * These are the functions called by AGROS. These declarations are pretty explicit.
 * More detailed comments can be found in source files.
 *
 */

void    parse_command       (char *cmdline, command_t *cmd);
void	get_prompt	    (char *prompt, int length, char *username);
char*	read_input	    (char *prompt);
void    print_prompt        (char* username);
void    print_help          (config_t* config);
void    change_directory    (char* path, int loglevel);
int     get_cmd_code        (char* cmd_name);
int     check_validity      (command_t cmd, config_t config);
void    print_env           (char* env_variable);
void    print_allowed       (char** allowed);
void    print_forbidden     (char** forbidden);
void    parse_config        (config_t* config, char* username);
void    set_username        (char** username);
void    set_homedir         (char** homedir);
void    decrease_warnings   (config_t* ag_config);
int     runs_in_background  (command_t* cmd);
/*void    set_glib_group      (char** glib_group, GKeyFile* gkf, char* username, char* key);*/
void	initialize_readline (config_t *config);
char*	make_completion	    (char *string);
char**	cmd_completion	    (const char *text, int start, int end);
char*	cmd_generator	    (const char *text, int state);

