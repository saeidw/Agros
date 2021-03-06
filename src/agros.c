//
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
#include <unistd.h>
#include <assert.h>
#include <syslog.h>
#include <glib.h>
#include <pwd.h>
#include <sys/types.h>
#include "agros.h"

#include <readline/readline.h>
#include <readline/history.h>

#ifndef CONFIG_FILE
#define CONFIG_FILE "agros.conf"
#endif

/*
 * A global array holding the associations between each built-in command
 * and their command_code. More explanations can be found in the declaration
 * of the "built_in_commands" structure.
 *
 */

built_in_commands my_commands[CMD_NBR] = {
    {"exit" , EXIT_CMD  },
    {""     , EMPTY_CMD },
    {"cd"   , CD_CMD    },
    {"env"  , ENV_CMD   },
    {"help" , HELP_CMD  },
    {"?"    , HELP_CMD  }
};

/*
 * A reference to the list and number of allowed commands.
 * This list is used when autocompleting commands.
 */
char **allowed_list = (char **)NULL;
int allowed_nbr = 0;

/* This variable contains the environment. I use it in my "env" built-in
   function */
extern char** environ;

/*
 * This function parses a string and fills a command_t struct.
 * It uses the strtok() to split the string into tokens. Then it fills the argv
 * array with the tokens.
 *
 * After filling the array, it copies argv[0] as the cmd->name and cmd->argc
 * as the length of the array.
 *
 */

void parse_command (char *cmdline, command_t *cmd){
    int count = 0;
    char* word;

    word = strtok (cmdline, WHITESPACE);

    if (word == NULL) { word = ""; } // Fixes blank line bug

    if (cmd->name != NULL){
        free (cmd->name);
    }

    while (word) {
        cmd->argv[count] = word;
        word = strtok (NULL, WHITESPACE);
        count++;
    }
    cmd->argv[count] = NULL;

    cmd->argc = count;
    cmd->name = (char *) malloc (strlen (cmd->argv[0])+1);
    strcpy (cmd->name, cmd->argv[0]);
}

/*
 * Reads the input using GNU Readline.
 * Saves each input in a history list.
 * - prompt: The prompt to display when asking for input.
 * The result is dynamically allocated and should 
 * be cleaned up with free().
 */

char *read_input (char *prompt)
{
    char *result;
    result  = readline(prompt);

    /* Add the line to the history if it's valid and non-empty */
    if (result  && *result) {
	add_history(result);
    } 

    return result;
}

/*
 * Modifiy this function to modify the prompt. Ultimately, I want to define the prompt
 * inside the conf file à la bash.
 *
 * Some systems use USERNAME, others use USER. Hence the ugly if.
 *
 */

void print_prompt (char* username){
    fprintf (stdout, "[AGROS]%s:%s$ ", username, getenv ("PWD"));
}

/*
 * Prints the prompt to the given string.
 * - prompt: The string that will contain the prompt
 * - length: The maximum length of the prompt
 * - username: The username to display
 */
void get_prompt (char *prompt, int length, char *username)
{
    snprintf(prompt, length, "[AGROS]%s:%s$ ", username, getenv("PWD"));
}

/*
 * Prints the help message.
 * TODO: Store my string messages (help + error messages) in a separate file.
 */

void print_help(config_t* config){
    int i =0;

    fprintf (stdout, "\n");
    fprintf (stdout, "\n");
    for (i=0; i<70; i++)
        fprintf (stdout, "*");
    fprintf (stdout, "\nWelcome to AGROS, the newer limited shell.\n");
    fprintf (stdout, "Note: At any time, you can type 'exit' to close the shell.\n\n");
    print_allowed (config->allowed_list);
    print_forbidden (config->forbidden_list);
    for (i=0; i<70; i++)
        fprintf (stdout, "*");
    fprintf (stdout, "\n");
    fprintf (stdout, "\n");
}

/*
 * Uses chdir() to change current working directory. Then updates the environement,
 * with the new value of PWD.
 *
 * If path is NULL, the function sends the user to his home directory. It builds the path
 * in the temp string "home" then copies it into "path" before freeing "home".
 *
 */

void change_directory (char* path, int loglevel){

    /* If no arguments are given, go to $HOME directory */
    if (path == NULL)
        set_homedir (&path);

    if (chdir (path) == 0){
        if (loglevel >= 3) syslog (LOG_NOTICE, "Changing to directory: %s.", path);
        getcwd (path, MAX_LINE_LEN);
        setenv ("PWD", path, 1);
    } else {
        fprintf (stderr, "%s: Could not change to such directory\n", path);
        if (loglevel >= 2) syslog (LOG_NOTICE, "Could not change to directory: %s.", path);
    }

}

/*
 * This function access the global array variable my_commands
 * and returns the command_code eauivalent to each command.
 *
 */

int get_cmd_code (char* cmd_name){
    int i = 0;
    for (i=0; i<CMD_NBR; i++){
	    if (!strcmp (my_commands[i].command_name, cmd_name))
	        return my_commands[i].command_code;
    }
    return OTHER_CMD;
}

/*
 * This function checks for the validity of user input.
 * allowed_list is an array of strings, the only commands
 * allowed to use.
 *
 *
 */

int check_validity (command_t cmd, config_t config){
    int valid = 1;
    int i = 0, j = 0;

    /* Checks if the command name is part of the allowed list */
    while (config.allowed_list[i]){
	    if (!strcmp (config.allowed_list[i], cmd.name) || !strcmp (config.allowed_list[i], "*"))
	        valid = AG_FALSE;
	    i++;
    }

    /* Checks that the command line does not include any forbidden character */
    i = 0;

    while (config.forbidden_list[i]){
        for (j=0; j<cmd.argc; j++){
            if (strstr (cmd.argv[j], config.forbidden_list[i]) != NULL)
                valid = AG_TRUE;
        }

        i++;
    }

    return valid;
}

/* 
 * Built-in function that displays the environment. As simple as that. 
 * 
 */

void print_env (char* env_variable){
    char* env_value = NULL;
    char** var = NULL;

    if (env_variable != NULL){
	    env_value = getenv(env_variable);
	    if (env_value)
	        fprintf (stdout, "%s:\t%s\n", env_variable, getenv(env_variable));
	    else
	        fprintf (stdout, "Environment variable %s does not exist.\n", env_variable);
    }else {
	    for (var = environ; *var != NULL; ++var)
	        fprintf (stdout, "%s\n", *var);
    }
}

/*
 * Prints the list of forbidden commands. Used in the print_help() function
 *
 */

void print_forbidden (char** forbidden){
    int i=0;

    fprintf (stdout, "List of forbidden characters:\n\n");
    if (forbidden[0] != NULL){
        while (forbidden[i]){
            fprintf (stdout, " * %s\n", forbidden[i]);
            i++;
        }
    } else{
        fprintf (stdout, " * (none)\n");
    }

    fprintf (stdout, "\n");
}


/*
 * Prints the list of allowed commands. Used in the print_help() function
 *
 */

void print_allowed (char** allowed){
    int i=0;

    fprintf (stdout, "List of allowed actions:\n\n");
    if (strcmp (allowed[0],"*")){
        while (allowed[i]){
	        fprintf (stdout, " * %s\n", allowed[i]);
	        i++;
        }
        fprintf (stdout, "\n");
    } else
        fprintf (stdout, " * (all)\n\n");
}

/*
 * EFFECTS: parses CONFIG_FILE.
 * MODIFIES: allowed_list, allowed_nbr, welcome_message, loglevel
 */

void parse_config (config_t* config, char* username){
    GKeyFile* gkf;
    gsize gallowed_nbr;
    gsize gforbidden_nbr;
    char* glib_group = NULL;

    gkf = g_key_file_new ();

    /* Loads the file into gkf */
    if (!g_key_file_load_from_file (gkf, CONFIG_FILE, G_KEY_FILE_NONE, NULL)){
	    fprintf (stderr, "Could not read config file %s\nTry using another shell or contact an administrator.\n", CONFIG_FILE);
        syslog (LOG_ERR, "Could not read config file: %s.", CONFIG_FILE);
        closelog ();
	    exit (EXIT_FAILURE);
    }

    /* If the file exists and is loaded, we proceed to parsing it */

    /* LOGLEVEL */
    if (g_key_file_has_group (gkf, username) && g_key_file_has_key(gkf, username, "loglevel", NULL)){
        glib_group =  username;
    }else
        glib_group = "General";
    if (g_key_file_has_key (gkf, glib_group, "loglevel", NULL)){
	    config->loglevel = g_key_file_get_integer (gkf, glib_group, "loglevel", NULL);
        syslog (LOG_NOTICE, "Setting log level to: %d.", config->loglevel);
    }
    else
	    config->loglevel = 0;

    /* WELCOME MESSAGE */
    if (g_key_file_has_group (gkf, username) && g_key_file_has_key(gkf, username, "welcome", NULL)){
        glib_group =  username;
    }else
        glib_group = "General";
    if (g_key_file_has_key (gkf, glib_group, "welcome", NULL)){
	    config->welcome_message = g_key_file_get_string (gkf, glib_group, "welcome", NULL);
        if (config->loglevel >=3) syslog (LOG_NOTICE, "Setting welcome message to: %s.", config->welcome_message);
    } else
	    config->welcome_message = NULL;

    /* ALLOWED COMMANDS */
    if (g_key_file_has_group (gkf, username) && g_key_file_has_key(gkf, username, "allowed", NULL)){
        glib_group =  username;
    }else
        glib_group = "General";
    if (g_key_file_has_key (gkf, glib_group, "allowed", NULL)){
	    config->allowed_list = g_key_file_get_string_list (gkf, glib_group, "allowed", &gallowed_nbr, NULL);
	    config->allowed_nbr = gallowed_nbr;
    }else {
        fprintf (stderr, "Cannot launch AGROS; missing allowed list from conf file.\n");
        if (config->loglevel >=1) syslog (LOG_NOTICE, "Error in conf file, missing allowed list!");
        exit (EXIT_SUCCESS);
    }

    /* FORBIDDEN CHARACTERS */
    if (g_key_file_has_group (gkf, username) && g_key_file_has_key(gkf, username, "forbidden", NULL)){
        glib_group =  username;
    }else
        glib_group = "General";
    if (g_key_file_has_key (gkf, glib_group, "forbidden", NULL)){
	    config->forbidden_list = g_key_file_get_string_list (gkf, glib_group, "forbidden", &gforbidden_nbr, NULL);
	    config->forbidden_nbr = gforbidden_nbr;
    }else {
        fprintf (stderr, "Cannot launch AGROS; missing parameter from conf file.\n");
        if (config->loglevel >=1) syslog (LOG_NOTICE, "Error in conf file, missing forbidden list!");
        exit (EXIT_SUCCESS);
    }

    /* WARNING_NBR */
    if (g_key_file_has_group (gkf, username) && g_key_file_has_key(gkf, username, "warnings", NULL)){
        glib_group =  username;
    }else
        glib_group = "General";
    if (g_key_file_has_key (gkf, glib_group, "warnings", NULL)){
	    config->warnings = g_key_file_get_integer (gkf, glib_group, "warnings", NULL);
        syslog (LOG_NOTICE, "Setting initial warning number to: %d.", config->warnings);
    }
    else
	    config->warnings = -1;

     g_key_file_free (gkf);
}

/*
 * Setting variables using getuid() and getpwuid()
 * More info on these functions can easily be found in man pages.
 *
 */
void set_username (char** pusername){
    struct passwd *pwd = NULL;
    pwd = getpwuid (getuid());
    *pusername = pwd->pw_name;
}

void set_homedir (char** phomedir){
    struct passwd *pwd = NULL;
    pwd = getpwuid (getuid());
    *phomedir = pwd->pw_dir;
}

void decrease_warnings (config_t* ag_config){
    if (ag_config->warnings > 0){
        ag_config->warnings--;
        if (ag_config->warnings >= 0)  fprintf (stdout, "Warnings remaining: %d\n", ag_config->warnings);
    }else {
        fprintf (stderr, "Exiting AGROS. The incident will be reported. \n");
        if (ag_config->loglevel >= 1)    syslog (LOG_NOTICE, "User reached Max warnings. \n");
        kill (getppid(), SIGTERM);
        _exit (EXIT_FAILURE);
    }
}

/*
 * This function looks for the '&' character.
 *
 */
int runs_in_background (command_t* cmd){
    int i=0;
    char* position=NULL;

    for (i=0; i<cmd->argc; i++){
        position = strchr (cmd->argv[i], '&');
        if (position){
            *position = '\0';
            return AG_TRUE;
        }
    }

    return AG_FALSE;
}

/***************************************************************************************************************************

void set_glib_group (char** glib_group, GKeyFile* gkf, char* username, char* key){
    if (g_key_file_has_group (gkf, username) && g_key_file_has_key(gkf, username, key, NULL)){
        *glib_group =  username;
    }else
        *glib_group = "General";
}

***************************************************************************************************************************/

/*
 * Readline functionality
 */

/*
 * Set up autocompletion and history using GNU Readline
 * - config: The AGROS configuration to use when autocompleting.
 */
void initialize_readline(config_t *config)
{
    /* Can be used for customization in the future */
    rl_readline_name = "AGROS";

    /* The function to call before default autocompletion kicks in */
    rl_attempted_completion_function = cmd_completion;

    /* Get a handle to the list of allowed commands and its length
     * This allows us to autocomplete these as well.
     */
    allowed_list = config->allowed_list;
    allowed_nbr = config->allowed_nbr;
}

/*
 * This function is automatically called by GNU Readline
 * when autocompleting.
 * - text: The text the user has entered.
 * - start: An index to the start of the text in the input buffer.
 * - end: An index to the end of the text in the input buffer.
 */
char **cmd_completion(const char *text, int start, int end)
{
    char **matches = (char **)NULL;
    
    /* Making sure end is used, not really useful for anything */
    if (end == 0) {
	;
    }

    /* 
     * We're at the beginning of the buffer, so we should match
     * built-in functions and commands. Otherwise, readline will
     * automatically match file names
     */
    if (start == 0)
	matches = rl_completion_matches(text, cmd_generator);

    return matches;
}

/*
 * Dynamically allocates a string and returns a pointer to it.
 * - string: The string to copy into the new location
 */
char *make_completion(char *string)
{
    char *result = (char *)NULL;
    int length= strlen(string) + 1;
    result = malloc(length);
    if (result)
	strncpy(result, string, length);

    return result;
}

/*
 * Searches built-in functions and allowed commands 
 * for matches to currently entered text.
 * - text: The text to match.
 * - state: Indicates the status of the search.
 *   When state is 0, this function is being called 
 *   in a new autocompletion, and it should start a 
 *   new search.
 */
char *cmd_generator(const char *text, int state)
{
    static int cmd_index;
    static int allowed_index;
    static int length;

    char *value = (char *)NULL;

    /* Prepare a new search for matches */
    if (state == 0) {
	cmd_index = 0;
	allowed_index = 0;
	length = strlen(text);
    }

    /* Check the list of built-in functions for a match */
    while (cmd_index < CMD_NBR) {
	value = my_commands[cmd_index].command_name;
	cmd_index++;
	if (strncmp(value, text, length) == 0) {
	    return make_completion(value);
	}
    }

    /* Check the list of allowed commands for a match */
    while (allowed_index < allowed_nbr) {
	value = allowed_list[allowed_index];
	allowed_index++;
	if (strncmp(value, text, length) == 0) {
	    return make_completion(value);
	}
    }

    /* No matches were found. */
    return (char *)NULL;
}

