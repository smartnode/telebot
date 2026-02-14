#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <telebot.h>
#include <libgen.h>
#include <sys/file.h>
#include <errno.h>

#define SIZE_OF_ARRAY(array) (sizeof(array)/sizeof(array[0]))
#define TOKEN_SIZE 128
#define FILENAME_TOKEN ".token"
#define FILENAME_USERID ".userid"
#define PID_FILE "/var/tmp/estgb_lock.pid"

typedef void (*sendfunc_t)(char *filename);
typedef void (*sendfuncgroup_t)(char **filenames, int count);

struct {
	char *token;
	long long user_id;
	char *imgfile;
	char *videofile;
	char *docfile;
	char *audiofile;
	char *comment;
	char *path;
	char *text;
	char *mask;
	char *proxy_addr;
	char *proxy_auth;
	int isAnimation;
	int isDaemonize;
	int isRemove;
	int isWeakConfig;
	int isWildcard;
	int isSingleton;
	int isScan;
	int needUnescape;
	int useFileConfig;
	int isPicsMediagroup;
	int timeRescan;
	int fd;
	telebot_handler_t handle;
	telebot_error_e ret;
} estgbconf;

char* concatFilename(char *path, char *filename) {
	int isSlash = 2;
	char *result = NULL;
	size_t path_len = strlen(path);

	if (path_len > 0 && path[path_len - 1] == '\\')
		isSlash = 1;

	if (isSlash == 2)
		asprintf(&result, "%s/%s", path, filename);
	else
		asprintf(&result, "%s%s", path, filename);

	return result;
}

int checkSingleton() {
	estgbconf.fd = open(PID_FILE, O_CREAT | O_RDWR, 0666);
	if (estgbconf.fd == -1)
		return 2;

	int rc = flock(estgbconf.fd, LOCK_EX | LOCK_NB);
	if (rc) {
		if (EWOULDBLOCK == errno)
			return 1;
	}
	return 0;
}

int checkMask(const struct dirent *dp) {
	return (fnmatch(estgbconf.mask, dp->d_name, 0) == 0);
}

void processWildcard_s(char *path, sendfunc_t sendfunc) {
	struct dirent **namelist;
	int n;
	char *fullname;

	n = scandir(path, &namelist, checkMask, alphasort);
	if (n < 0)
		printf("Scandir error\n");
	else {

		while (n--) {
			fullname = concatFilename(path, namelist[n]->d_name);
			sendfunc(fullname);
			free(fullname);
			free(namelist[n]);
		}
		free(namelist);
	}
}

void processWildcard_group(char *path, sendfuncgroup_t sendfunc) {
	struct dirent **namelist;
	int n, j;
	char **sendlist;

	n = scandir(path, &namelist, checkMask, alphasort);
	if (n < 0)
		printf("Scandir error\n");
	else {

		sendlist = calloc(n, sizeof(char*));

		j = n;
		while (n--) {
			sendlist[n] = concatFilename(path, namelist[n]->d_name);
		}
		sendfunc(sendlist, j);
		while (j--) {
			free(namelist[j]);
			free(sendlist[j]);
		}
		free(namelist);
		free(sendlist);
	}
}

// Code of this function grabbed from source code of 'echo' utility
// https://git.savannah.gnu.org/cgit/coreutils.git/tree/src/echo.c  
inline static int hex2bin(unsigned char c) {
	switch (c) {
	default:
		return c - '0';
	case 'a':
	case 'A':
		return 10;
	case 'b':
	case 'B':
		return 11;
	case 'c':
	case 'C':
		return 12;
	case 'd':
	case 'D':
		return 13;
	case 'e':
	case 'E':
		return 14;
	case 'f':
	case 'F':
		return 15;
	}
}

// Zero-copy unescape function
// partially based on canonical 'echo' utility source code
// https://git.savannah.gnu.org/cgit/coreutils.git/tree/src/echo.c  
char* zc_unescape(char *input) {
	unsigned char c;
	char *p, *s;

	if (input == NULL)
		return NULL;

	s = input;
	p = input;

	while ((c = *s++)) {
		if (c == '\\' && *s) {
			switch (c = *s++) {
			case 'e':
				c = '\x1B';
				break;
			case 'f':
				c = '\f';
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case 'v':
				c = '\v';
				break;
			case 'x': {
				unsigned char ch = *s;
				if (!isxdigit(ch))
					goto not_an_escape;
				s++;
				c = hex2bin(ch);
				ch = *s;
				if (isxdigit(ch)) {
					s++;
					c = c * 16 + hex2bin(ch);
				}
			}
				break;
			case '0':
				c = 0;
				if (!('0' <= *s && *s <= '7'))
					break;
				c = *s++;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				c -= '0';
				if ('0' <= *s && *s <= '7')
					c = c * 8 + (*s++ - '0');
				if ('0' <= *s && *s <= '7')
					c = c * 8 + (*s++ - '0');
				break;
			case '\\':
				break;

				not_an_escape: default:
				*p = c;
				p++;
				break;
			}
		}
		*p = c;
		p++;
	}
	*p = '\0';

	return input;
}

void printHelp(void) {
	printf(
					"-----------------------------------------------------------------------------\n"
					"| estgb :: enhanced sender telegram bot                               v1.4.0|\n"
					"-----------------------------------------------------------------------------\n"
					"\n"
					"This telegram bot sends text, pictures, video, audio and documents (files)\n"
					"according to command line paramaters\n"
					"(c) 2018-2026 Flangeneer, Saint-Petersburg, Russia\n"
					"\n"
					"Usage: estgb <command> <parameter> [options]\n"
					"\n"
					"Commands:\n"
					"--sendtext <text>       Send text\n"
					"--sendpic <filename>    Send picture\n"
					"--senddoc <filename>    Send document\n"
					"--sendvideo <filename>  Send video\n"
					"--sendaudio <filename>  Send audio\n"
					"\n"
					"Options for Telegram bot configuration (required):\n"
					"--token <token>  Bot token\n"
					"--userid <id>    User ID\n"
					"  or use:\n"
					"--fileconfigs    Read bot token and userid from files %s and %s accordingly\n"
					"--path <dir>     Working path for --fileconfigs option\n"
					"\n"
					"to use '--fileconfigs' there are should be two files:\n"
					"1) %s - text file where first string is telegram bot token\n"
					"2) %s - text file where first string is user ID\n"
					"\n"
					"Options to proceed the text and media:\n"
					"--animation       Send video as animation (GIF or H.264/MPEG-4 w/o sound mp4)\n"
					"--comment <text>  Comment for picture/audio/video\n"
					"--escape-seq      Process escape sequences (emoji!) for 'comment' and 'text' (C-style)\n"
					"--mediagroup      Send multiple pictures as media group (when 2-10 items are exist)\n"
					"\n"
					"Options for network:\n"
					"--proxy <addr>           Use libcurl proxy. Examples: socks5://addr.org:8564 or http://addr.org:8564\n"
					"--proxyauth <user:pass>  Specify username and password for proxy\n"
					"\n"
					"Options for file processing:\n"
					"--wildcard      Process <filename> as wildcard instead of single file\n"
					"--remove        Remove(!) file after use for --sendpic, --sendvideo, --sendaudio, --senddoc\n"
					"--force-remove  Force remove(!) file after use even(!) error occured while send operation\n"
					"\n"
					"Options other:\n"
					"--repeat-send <value>  Repeat send procedure N rounds. Specify 0 (zero) to infinite\n"
					"--time-sleep  <value>  Time to sleep betwen cycles in seconds. Default 1 sec\n"
					"--singleton            If another instance of bot is working, do nothing and exit\n"
					"--daemon               Daemonize process (process work in background)\n"
					"--weakconfig           [FOR DEBUG] Simplified command line parameters pre-check\n"
					"\n", FILENAME_TOKEN, FILENAME_USERID, FILENAME_TOKEN,
			FILENAME_USERID);

}

void initConfig(void) {
	memset(&estgbconf, 0, sizeof(estgbconf));
	estgbconf.timeRescan = 1;
	estgbconf.isScan = -1;
}

void freeConfig(void) {
	telebot_destroy(estgbconf.handle);
	free(estgbconf.token);
	free(estgbconf.text);
	free(estgbconf.imgfile);
	free(estgbconf.videofile);
	free(estgbconf.audiofile);
	free(estgbconf.docfile);
	free(estgbconf.path);
	free(estgbconf.comment);
	free(estgbconf.proxy_addr);
	free(estgbconf.proxy_auth);
	close(estgbconf.fd);
}

void printConfig(void) {
	printf("Current config:\n"
			"Bot token...............%s\n"
			"User ID.................%lld\n"
			"Image filename..........%s\n"
			"Video filename..........%s\n"
			"Audio filename..........%s\n"
			"Doc filename............%s\n"
			"Video as animation......%d\n"
			"Send >2 pics as group...%d\n"
			"Comment.................%s\n"
			"Parse escape sequences..%d\n"
			"Text to send............%s\n"
			"Remove file after use...%d\n"
			"Use file config.........%d\n"
			"Working path............%s\n"
			"Use wildcards...........%d\n"
			"Use weak config.........%d\n"
			"Run as singleton........%d\n"
			"Work as daemon..........%d\n"
			"Proxy...................%s\n"
			"Proxy authentication....%s\n"
			"Send rounds.............%d\n"
			"Time sleep btw rounds...%d\n"
			"\n", estgbconf.token, estgbconf.user_id, estgbconf.imgfile,
			estgbconf.videofile, estgbconf.audiofile, estgbconf.docfile,
			estgbconf.isAnimation, estgbconf.isPicsMediagroup,
			estgbconf.comment, estgbconf.needUnescape, estgbconf.text,
			estgbconf.isRemove, estgbconf.useFileConfig, estgbconf.path,
			estgbconf.isWildcard, estgbconf.isWeakConfig, estgbconf.isSingleton,
			estgbconf.isDaemonize, estgbconf.proxy_addr, estgbconf.proxy_auth,
			estgbconf.isScan, estgbconf.timeRescan);
}

void sendText(void) {
	if (estgbconf.text != NULL) {
		estgbconf.ret = telebot_send_message(estgbconf.handle,
				estgbconf.user_id, estgbconf.text, "", false, false, 0, "");
		if (estgbconf.ret != TELEBOT_ERROR_NONE) {
			printf("Failed to send text message: %d \n", estgbconf.ret);
		}
	}
}

void sendPicture(char *filename) {
	if (!access(filename, R_OK)) {
		estgbconf.ret = telebot_send_photo(
			estgbconf.handle,
			estgbconf.user_id,
			filename,
			true,
			estgbconf.comment,
			NULL,               // parse_mode
			false,              // disable_notification
			0,                  // reply_to_message_id
			NULL                // reply_markup
		);
		if (estgbconf.ret != TELEBOT_ERROR_NONE) {
			printf("Failed to send picture: %d \n", estgbconf.ret);
		} else if (estgbconf.isRemove == 1)
			remove(filename);
		if (estgbconf.isRemove == 2)
			remove(filename);
	}
}

void removeFileGroup(char **filenames, int count) {
	int j = count;
	while (j--)
		remove(filenames[j]);
}

void sendGroup(char **filenames, int count) {
	estgbconf.ret = telebot_send_media_group(
		estgbconf.handle,
		estgbconf.user_id,
		filenames,
		count,
		false,    // disable_notification
		0         // reply_to_message_id
		// ← NO reply_markup (not supported by Telegram API for media group)
	);
	if (estgbconf.ret != TELEBOT_ERROR_NONE) {
		printf("Failed to send media group: %d \n", estgbconf.ret);
	} else if (estgbconf.isRemove == 1)
		removeFileGroup(filenames, count);
	if (estgbconf.isRemove == 2)
		removeFileGroup(filenames, count);
}

void sendPicturesGroup(char **filenames, int count) {

	int i, j = 0;

	if (count == 1) {
		sendPicture(filenames[0]);
		return;
	}

	char **sendlist;
	sendlist = calloc(10, sizeof(char*));

	for (i = 0; i < count; i++) {

		if (!access(filenames[i], R_OK)) {
			sendlist[j] = strdup(filenames[i]);
			j++;
		}

		if (j == 10) {
			sendGroup(sendlist, j);
			while (j--)
				free(sendlist[j]);
			
			j = 0;
		}
	}

	if (j > 0) {
		sendGroup(sendlist, j);
		while (j--)
			free(sendlist[j]);
	}
	free(sendlist);
}

void sendVideo(char *filename) {
	if (!access(filename, R_OK)) {
		if (estgbconf.isAnimation)
			estgbconf.ret = telebot_send_animation(
				estgbconf.handle,
				estgbconf.user_id,
				filename,
				true,
				0, 0, 0,                    // duration, width, height
				NULL,                       // thumb
				estgbconf.comment,
				NULL,                       // parse_mode
				false,                      // disable_notification
				0,                          // reply_to_message_id
				NULL                        // reply_markup
			);
		else
			estgbconf.ret = telebot_send_video(
				estgbconf.handle,
				estgbconf.user_id,
				filename,
				true,
				0, 0, 0,                    // duration, width, height
				NULL,                       // thumb
				estgbconf.comment,
				NULL,                       // parse_mode
				false,                      // supports_streaming
				false,                      // disable_notification
				0,                          // reply_to_message_id
				NULL                        // reply_markup
			);
		if (estgbconf.ret != TELEBOT_ERROR_NONE) {
			printf("Failed to send video: %d \n", estgbconf.ret);

		} else if (estgbconf.isRemove == 1)
			remove(filename);
		if (estgbconf.isRemove == 2)
			remove(filename);
	}
}

void sendAudio(char *filename) {
	if (!access(filename, R_OK)) {
		estgbconf.ret = telebot_send_audio(
			estgbconf.handle,
			estgbconf.user_id,
			filename,
			true,
			estgbconf.comment,   // caption
			NULL,                // parse_mode
			0,                   // duration
			NULL,                // performer
			NULL,                // title
			NULL,                // thumb
			false,               // disable_notification
			0,                   // reply_to_message_id
			NULL                 // reply_markup
		);
		if (estgbconf.ret != TELEBOT_ERROR_NONE) {
			printf("Failed to send audio: %d \n", estgbconf.ret);

		} else if (estgbconf.isRemove == 1)
			remove(filename);
		if (estgbconf.isRemove == 2)
			remove(filename);
	}
}

void sendDocument(char *filename) {
	if (!access(filename, R_OK)) {
		estgbconf.ret = telebot_send_document(
			estgbconf.handle,
			estgbconf.user_id,
			filename,
			true,
			NULL,                // thumb
			estgbconf.comment,   // caption
			NULL,                // parse_mode
			false,               // disable_notification
			0,                   // reply_to_message_id
			NULL                 // reply_markup
		);
		if (estgbconf.ret != TELEBOT_ERROR_NONE) {
			printf("Failed to send document: %d \n", estgbconf.ret);
		} else if (estgbconf.isRemove == 1)
			remove(filename);
		if (estgbconf.isRemove == 2)
			remove(filename);
	}
}

void sendSingle(void) {
	sendText();
	if (estgbconf.imgfile)
		sendPicture(estgbconf.imgfile);
	if (estgbconf.videofile)
		sendVideo(estgbconf.videofile);
	if (estgbconf.audiofile)
		sendAudio(estgbconf.audiofile);
	if (estgbconf.docfile)
		sendDocument(estgbconf.docfile);
}

void sendMultiple(void) {

	char *imgmask = NULL, *imgdir = NULL;
	char *vidmask = NULL, *viddir = NULL;
	char *audiomask = NULL, *auddir = NULL;
	char *docmask = NULL, *docdir = NULL;

	sendText();

	if (estgbconf.imgfile) {
		imgmask = strdup(basename(estgbconf.imgfile));
		imgdir = strdup(estgbconf.imgfile);
		imgdir = dirname(imgdir);
		estgbconf.mask = imgmask;
		if (estgbconf.isPicsMediagroup)
		processWildcard_group(imgdir, sendPicturesGroup);
		else processWildcard_s(imgdir, sendPicture);
	}

	if (estgbconf.videofile) {
		vidmask = strdup(basename(estgbconf.videofile));
		viddir = strdup(estgbconf.videofile);
		viddir = dirname(viddir);
		estgbconf.mask = vidmask;
		processWildcard_s(viddir, sendVideo);
	}

	if (estgbconf.audiofile) {
		audiomask = strdup(basename(estgbconf.audiofile));
		auddir = strdup(estgbconf.audiofile);
		auddir = dirname(auddir);
		estgbconf.mask = audiomask;
		processWildcard_s(auddir, sendAudio);
	}

	if (estgbconf.docfile) {
		docmask = strdup(basename(estgbconf.docfile));
		docdir = strdup(estgbconf.docfile);
		docdir = dirname(docdir);
		estgbconf.mask = docmask;
		processWildcard_s(docdir, sendDocument);
	}

	free(imgmask);
	free(vidmask);
	free(audiomask);
	free(docmask);
	free(imgdir);
	free(viddir);
	free(auddir);
	free(docdir);
	estgbconf.mask = NULL;
}

void scan(void) {

	int infinite = (estgbconf.isScan == 0);
	if (estgbconf.isScan < 0)
		return;

	if (!estgbconf.isWildcard) {
		while (estgbconf.isScan-- || infinite) {
			sendSingle();
			sleep(estgbconf.timeRescan);
		}
		return;
	}

	while (estgbconf.isScan-- || infinite) {
		sendMultiple();
		sleep(estgbconf.timeRescan);
	}
	return;
}

int readFileConfig(void) {

	char *tokenfilename, *useridfilename;

	if (estgbconf.path == NULL) {
		tokenfilename = strdup(FILENAME_TOKEN);
		useridfilename = strdup(FILENAME_USERID);
	} else {
		tokenfilename = concatFilename(estgbconf.path, FILENAME_TOKEN);
		useridfilename = concatFilename(estgbconf.path, FILENAME_USERID);
	}

	FILE *fp = fopen(tokenfilename, "r");
	if (fp == NULL) {
		printf("Failed to open %s file\n", FILENAME_TOKEN);
		return -1;
	}

	estgbconf.token = (char*) calloc(TOKEN_SIZE, sizeof(char));
	if (fscanf(fp, "%s", estgbconf.token) == 0) {
		printf("Reading token failed\n");
		fclose(fp);
		return -1;
	}
	fclose(fp);

	fp = fopen(useridfilename, "r");
	if (fp == NULL) {
		printf("Failed to open %s file\n", FILENAME_USERID);
		return -1;
	}

	if (fscanf(fp, "%lld", &estgbconf.user_id) == 0) {
		printf("Reading user ID failed\n");
		fclose(fp);
		return -1;
	}

	free(tokenfilename);
	free(useridfilename);
	fclose(fp);
	return 0;
}

int checkConfig(void) {
	if (estgbconf.token == NULL)
		return 0;
	if (estgbconf.user_id == 0)
		return 0;
	if ((estgbconf.imgfile == NULL) && (estgbconf.text == NULL)
			&& (estgbconf.videofile == NULL) && (estgbconf.docfile == NULL)
			&& (estgbconf.audiofile == NULL))
		return 0;
	
	if (!estgbconf.isWeakConfig && (estgbconf.imgfile == NULL)
			&& (estgbconf.videofile == NULL) && (estgbconf.docfile == NULL)
			&& (estgbconf.audiofile == NULL) && (estgbconf.isRemove != 0))
		return 0;
	return 1;
}

int globalInit(void) {
	if (estgbconf.useFileConfig) {
		if (readFileConfig() != 0) {
			printf(
					"Warning! Failed to read config files. Checking for command line config...\n");
		}
	}
	 
	if (!checkConfig()) {
		printHelp();
		printConfig();
		printf("Configuration error. Nothing to do.\n\n");
		return 0;
	}

	if (telebot_create(&estgbconf.handle, estgbconf.token)
			!= TELEBOT_ERROR_NONE) {
		printf("Error. Telebot create failed (bad token?)\n\n");
		return 0;
	}

	if (estgbconf.proxy_addr != NULL) {
		estgbconf.ret = telebot_set_proxy(estgbconf.handle,
				estgbconf.proxy_addr, estgbconf.proxy_auth);
		if (estgbconf.ret != TELEBOT_ERROR_NONE) {
			printf("Warning! Failed to init proxy: %d \n", estgbconf.ret);
		}
	}

	return 1;
}

int parseCmdLine(int argc, char *argv[]) {
	int j, more;

	if (argc < 3) {
		printHelp();
		return 0;
	}

	for (j = 1; j < argc; j++) {
		more = ((j + 1) < argc);
		if (!strcmp(argv[j], "--remove")) {
			estgbconf.isRemove = 1;
		} else if (!strcmp(argv[j], "--force-remove")) {
			estgbconf.isRemove = 2;
		} else if (!strcmp(argv[j], "--escape-seq")) {
			estgbconf.needUnescape = 1;
		} else if (!strcmp(argv[j], "--singleton")) {
			estgbconf.isSingleton = 1;
		} else if (!strcmp(argv[j], "--daemon")) {
			estgbconf.isDaemonize = 1;
		} else if (!strcmp(argv[j], "--mediagroup")) {
			estgbconf.isPicsMediagroup = 1;
		} else if (!strcmp(argv[j], "--repeat-send") && more) {
			estgbconf.isScan = atoi(argv[++j]);
		} else if (!strcmp(argv[j], "--time-sleep") && more) {
			estgbconf.timeRescan = atoi(argv[++j]);
		} else if (!strcmp(argv[j], "--animation")) {
			estgbconf.isAnimation = 1;
		} else if (!strcmp(argv[j], "--wildcard")) {
			estgbconf.isWildcard = 1;
		} else if (!strcmp(argv[j], "--weakconfig")) {
			estgbconf.isWeakConfig = 1;
		} else if (!strcmp(argv[j], "--sendpic") && more) {
			estgbconf.imgfile = strdup(argv[++j]);
		} else if (!strcmp(argv[j], "--sendvideo") && more) {
			estgbconf.videofile = strdup(argv[++j]);
		} else if (!strcmp(argv[j], "--sendtext") && more) {
			estgbconf.text = strdup(argv[++j]);
		} else if (!strcmp(argv[j], "--senddoc") && more) {
			estgbconf.docfile = strdup(argv[++j]);
		} else if (!strcmp(argv[j], "--sendaudio") && more) {
			estgbconf.audiofile = strdup(argv[++j]);
		} else if (!strcmp(argv[j], "--fileconfigs")) {
			estgbconf.useFileConfig = 1;
		} else if (!strcmp(argv[j], "--token") && more) {
			estgbconf.token = strdup(argv[++j]);
		} else if (!strcmp(argv[j], "--userid") && more) {
			estgbconf.user_id = atoll(argv[++j]);
		} else if (!strcmp(argv[j], "--path") && more) {
			estgbconf.path = strdup(argv[++j]);
		} else if (!strcmp(argv[j], "--comment") && more) {
			estgbconf.comment = strdup(argv[++j]);
		} else if (!strcmp(argv[j], "--proxy") && more) {
			estgbconf.proxy_addr = strdup(argv[++j]);
		} else if (!strcmp(argv[j], "--proxyauth") && more) {
			estgbconf.proxy_auth = strdup(argv[++j]);
		}

		else if (!strcmp(argv[j], "--help")) {
			printHelp();
			return 0;
		} else {
			printHelp();
			printConfig();
			printf("Unknown or not enough arguments for option '%s'.\n\n",
					argv[j]);
			return 0;
		}
	}
	return 1;
}

int main(int argc, char *argv[]) {

	initConfig();
	if (!parseCmdLine(argc, argv))
		goto exit;

	if (estgbconf.isSingleton) {
		switch (checkSingleton()) {
		case 0:
			break; // No another instance found, continue to work
		case 1:
			goto exit;
			// Found another instance of bot, exiting
		case 2:
			printf("Error open PID file for singleton check\n");
			goto exit;
			// Something goes wrong, exiting
		}

	}

	if (!globalInit())
		goto exit;

	if (estgbconf.isDaemonize)
		daemon(0, 0);

	if (estgbconf.needUnescape) {
		estgbconf.text = zc_unescape(estgbconf.text);
		estgbconf.comment = zc_unescape(estgbconf.comment);
	}

	if (estgbconf.isScan >= 0) {
		scan();
		goto exit;
	}

	if (estgbconf.isWildcard)
		sendMultiple();
	else
		sendSingle();

	exit: freeConfig();
	return 0;
}
