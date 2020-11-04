
#pragma comment(lib,"libssl.lib")
#pragma comment(lib,"libcrypto.lib")
#pragma comment(lib, "user32.lib") 

#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "third_party/base64.h"
#include "third_party/base64_file.h"
#include "third_party/zip.h"
#include "net_handler.h"


//#define fopen(fp, fmt, mode) fopen_s((fp), (fmt), (mode))          
#define FILE_B64 "file.b64"
#define FILE_ZIP "zfile.zip"
#define CONFIG "config"
#define LINE_SIZE 128
#define BUFF 1024
#define NUM_LINES 4

#define HELP "Use like this:\n \
\t[mail_to] [subject] [text] [attach] -z\n \
\t-z zipping attach file\n \
Config file:\n \
\tmust be have name is 'config'\n \
\tmust be have 4 lines of SMTP data:\n \
\t1) host\n\t2) port\n \
\t3) login\n\t4) password\n"	

#define FORMAT_MAIL_FROM "MAIL FROM:<%s>\n"
#define FORMAT_RCPT_TO "RCPT TO:<%s>\n"
#define FORMAT_TO_FROM_SUBJECT "To: %s\nFrom: <%s>\nSubject: %s\n\n%s\n.\n"

#define MIME_TYPE "application/octet-stream"

#define MIME_FORMAT_HEADS "To: %s\nFrom: <%s>\nSubject: %s\n" \
"MIME-Version: 1.0\nContent-Type: multipart/mixed;\n" \
" boundary=\"------------%s\"\n\n" \
"This is a multi-part message in MIME format.\n" \
"--------------%s\n" \
"Content-Type: text/plain; charset=UTF-8; format=flowed\n" \
"Content-Transfer-Encoding: 7bit\n\n" \
"%s\n\n\n%s\n%s\n\n" \
"--------------%s--\n.\n" 

#define MIME_FORMAT_CONTENT "--------------%s\n" \
"Content-Type: %s;\n" \
" name=\"%s\"\n" \
"Content-Transfer-Encoding: base64\n" \
"Content-Disposition: attachment;\n" \
" filename=\"%s\"\n"

typedef struct
{
	char* host,
		* login,
		* pass,
		* body,
		* buff,
		* port,
		login_b64[LINE_SIZE],
		pass_b64[LINE_SIZE],
		subj[LINE_SIZE],
		from[LINE_SIZE],
		addr_to[LINE_SIZE],
		rcpt_to[LINE_SIZE],
		conf_data[NUM_LINES][LINE_SIZE];

} Mail_Data;

typedef struct
{
	char recive[BUFF], // message from server
		*send[LINE_SIZE]; // message to server

} Mail_Buffer;

typedef struct
{
	char* name,
		* path,
		* data_b64;

	int	size;

	BOOL yes,
		zip;

} Mail_Attach;

int sendMail(Mail_Data*, Mail_Attach*, Mail_Buffer*);
void mailBase(Mail_Data*);
void setBuffer(Mail_Data*, Mail_Attach*);
void setCmds(Mail_Data*, Mail_Buffer*);

void setBaseArgs(Mail_Data*, char**);
void setAttachArgs(Mail_Attach*, char*);
void setZipArgs(Mail_Attach*);

size_t readB64File(Mail_Attach*, char*);
BOOL readConfig(Mail_Data*); 	
void deleteFiles(BOOL);

unsigned long randomBound(void);

