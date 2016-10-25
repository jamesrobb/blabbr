/* getpasswd.c
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

/* To read a password without echoing it to the console.
 *
 * We assume that stdin is not redirected to a pipe and we won't
 * access tty directly. It does not make much sense for this program
 * to redirect input and output.
 *
 * This function is not safe to terminate. If the program crashes
 * during getpasswd or gets terminated, then echoing may remain
 * disabled for the shell (that depends on shell, operating system and
 * C library). To restore echoing, type 'reset' into the sell and
 * press enter.
 */
void getpasswd(const char *prompt, char *passwd, size_t size) {
	struct termios old_flags, new_flags;

    /* Clear out the buffer content. */
    memset(passwd, 0, size);
        
	/* Disable echo. */
	tcgetattr(fileno(stdin), &old_flags);
	memcpy(&new_flags, &old_flags, sizeof(old_flags));
	new_flags.c_lflag &= ~ECHO;
	new_flags.c_lflag |= ECHONL;
	if (tcsetattr(fileno(stdin), TCSANOW, &new_flags) != 0) {
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}

    /* Write the prompt. */
	write(STDOUT_FILENO, prompt, strlen(prompt));
        fsync(STDOUT_FILENO);
	fgets(passwd, size, stdin);

	/* The result in passwd is '\0' terminated and may contain a final
	 * '\n'. If it exists, we remove it.
	 */
	if (passwd[strlen(passwd) - 1] == '\n') {
		passwd[strlen(passwd) - 1] = '\0';
	}

	/* Restore the terminal */
	if (tcsetattr(fileno(stdin), TCSANOW, &old_flags) != 0) {
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}
}
