/************************************************************
*
*  smtp-sender
*  Very simple command line SMTP client with file attachment and ZIP compression
*  OpenSSL library used
* 
*  Serge Bash 2013 
*
************************************************************/

#include "mail_handler.h"


size_t readB64File(Mail_Attach* attach, char *fpath)
{
	size_t result = 0;
	size_t elem_size = 1;
	long fsize;
	FILE* fp = NULL;

	// File encoding	
	b64('e', (attach->zip ? FILE_ZIP : fpath), FILE_B64, 76);

	fp = fopen(FILE_B64, "r");
	if (fp == NULL)
	{
		perror(FILE_B64);
		return FALSE;
	}
	// File size
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// Set data 
	attach->data_b64 = (char*)malloc(sizeof(char) * fsize);
	if (attach->data_b64 != NULL)
		result = fread(attach->data_b64, elem_size, (size_t)fsize, fp);

	fclose(fp);
	return result;
}


BOOL readConfig(Mail_Data* mail)
{
	int i = 0;
	int count = 0;
	FILE* fp = NULL;

	//Read config 
	fp = fopen(CONFIG, "r");
	if (fp == NULL)
	{
		perror(CONFIG);
		return FALSE;
	}
	memset(mail->conf_data, 0, sizeof(mail->conf_data));
	while (fgets(mail->conf_data[count], LINE_SIZE, fp))
		count++;
	fclose(fp);

	while (i < count)
	{
		if (mail->conf_data[i][strlen(mail->conf_data[i]) - 1] == '\n')
			mail->conf_data[i][strlen(mail->conf_data[i]) - 1] = '\0';
		i++;
	}
	// And set data 
	mail->host = (char*)malloc(strlen(mail->conf_data[0]));
	mail->port = (char*)malloc(strlen(mail->conf_data[1]));
	mail->login = (char*)malloc(strlen(mail->conf_data[2]));
	mail->pass = (char*)malloc(strlen(mail->conf_data[3]));
	snprintf(mail->host, strlen(mail->conf_data[0]) + 1, "%s", mail->conf_data[0]);
	snprintf(mail->port, strlen(mail->conf_data[1]) + 1, "%s", mail->conf_data[1]);
	snprintf(mail->login, strlen(mail->conf_data[2]) + 1, "%s", mail->conf_data[2]);
	snprintf(mail->pass, strlen(mail->conf_data[3]) + 1, "%s", mail->conf_data[3]);

	return TRUE;
}


void setBaseArgs(Mail_Data* mail, char** argv)
{
	snprintf(mail->addr_to, strlen(argv[1])+1, "%s", argv[1]);
	snprintf(mail->subj, strlen(argv[2])+1, "%s", argv[2]);
	mail->body = (char*)malloc(strlen(argv[3]));
	mail->body = argv[3];
}


void setAttachArgs(Mail_Attach* attach, char* fpath)
{
	attach->path = (char*)malloc(strlen(fpath));
	attach->path = fpath;
	attach->name = (char*)malloc(strlen(fpath));
	attach->name = strrchr(fpath, '\\');
	attach->name++;
	attach->yes = TRUE;
}


void mailBase(Mail_Data* mail)
{
	base64_encode(mail->login, (unsigned int)strlen(mail->login), mail->login_b64);
	snprintf(mail->login_b64, strlen(mail->login_b64)+2, "%s\n", mail->login_b64);

	base64_encode(mail->pass, (unsigned int)strlen(mail->pass), mail->pass_b64);
	snprintf(mail->pass_b64, strlen(mail->pass_b64)+2, "%s\n", mail->pass_b64);

	snprintf(mail->from, strlen(mail->login) + strlen(FORMAT_MAIL_FROM) + 1, FORMAT_MAIL_FROM, mail->login);
	snprintf(mail->rcpt_to, strlen(mail->addr_to) + strlen(FORMAT_RCPT_TO) + 1, FORMAT_RCPT_TO, mail->addr_to);
}


void setBuffer(Mail_Data* mail, Mail_Attach* attach)
{
	size_t buff_size;

	if (attach == NULL)
	{
		buff_size = strlen(FORMAT_TO_FROM_SUBJECT) + 
			strlen(mail->login) + 
			strlen(mail->addr_to) + 
			strlen(mail->subj) + 
			strlen(mail->body);
		mail->buff = (char*)malloc(sizeof(char) * buff_size);
		memset(mail->buff, 0, buff_size);
		snprintf(mail->buff, buff_size,
			FORMAT_TO_FROM_SUBJECT,
			mail->addr_to, mail->login, mail->subj, mail->body);
	}
	else
	{
		int bound;
		char str_bound[32];		
		char mime_info[BUFF];
		size_t mime_info_size;

		bound = randomBound();
		snprintf(str_bound, 32, "%d", bound);
		mime_info_size = strlen(MIME_FORMAT_CONTENT) +
			strlen(str_bound) + 
			strlen(MIME_TYPE) +
			strlen((attach->zip ? FILE_ZIP : attach->name)) +
			strlen((attach->zip ? FILE_ZIP : attach->name));
		
		snprintf(mime_info, mime_info_size,
			MIME_FORMAT_CONTENT,
			str_bound,
			MIME_TYPE,
			(attach->zip ? FILE_ZIP : attach->name),
			(attach->zip ? FILE_ZIP : attach->name));
		
		buff_size = strlen(MIME_FORMAT_HEADS) +
			strlen(mail->addr_to) +
			strlen(mail->login) + 
			strlen(mail->subj) +
			strlen(mail->body) + 
			strlen(mime_info) +
			strlen(attach->data_b64) +
			strlen(str_bound)*3;

		mail->buff = (char*)malloc(sizeof(char) * buff_size);
		if (mail->buff != NULL)
		{
			memset(mail->buff, 0, buff_size);
			snprintf(mail->buff, buff_size,
				MIME_FORMAT_HEADS,
				mail->addr_to, mail->login, mail->subj,
				str_bound,
				str_bound,
				mail->body, mime_info, attach->data_b64,
				str_bound);
		}
	}
}


void setCmds(Mail_Data* mail, Mail_Buffer* buff)
{
	buff->send[0] = "EHLO there\n";
	buff->send[1] = "AUTH LOGIN\n";
	buff->send[2] = mail->login_b64;
	buff->send[3] = mail->pass_b64;
	buff->send[4] = mail->from;
	buff->send[5] = mail->rcpt_to;
	buff->send[6] = "DATA\n";
	buff->send[7] = mail->buff;
	buff->send[8] = "QUIT\n";
}


void setZipArgs(Mail_Attach* attach)
{
	struct zip_t* zip = zip_open(FILE_ZIP, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
	zip_entry_open(zip, attach->name);
	zip_entry_fwrite(zip, attach->path);
	zip_entry_close(zip);
	zip_close(zip);
	attach->zip = TRUE;
}


int sendMail(Mail_Data* mail,
	Mail_Attach* attach,
	Mail_Buffer* buff)
{
	int i = 0;
	SOCKET sock = INVALID_SOCKET;
	SSL_CTX* ctx = NULL;
	SSL* ssl = NULL;

	memset(buff->send, 0, sizeof(buff->send));
	memset(buff->recive, 0, sizeof(buff->recive));

	readConfig(mail);
	mailBase(mail);
	setBuffer(mail, (attach->yes ? attach : NULL));
	setCmds(mail, buff);

	sock = openConnect(mail->host, mail->port);
	if (sock == INVALID_SOCKET)
	{
		fprintf(stderr, "SOCKET not created");
		return FALSE;
	}

	ssl = openSSLConnect(sock, ctx);
	if (ssl == NULL)
	{
		fprintf(stderr, "SSL not created");
		return FALSE;
	}

	while (getSSLData(ssl, buff->recive))
	{
		if (buff->send[i] != 0)
			sendSSLData(ssl, buff->send[i]);
		i++;
	}

	memset(buff->recive, 0, sizeof(buff->recive));
	memset(buff->send, 0, sizeof(buff->send));

	killSocket(sock);
	killSSLSocket(ssl, ctx);
	deleteFiles(attach->yes);

	return TRUE;
}


unsigned long randomBound(void)
{
	unsigned long x, y, rnd;
	srand((unsigned int)time(NULL));
	x = rand();
	y = rand();
	rnd = x * y;
	return rnd;
}


void deleteFiles(BOOL file)
{
	if (file)
	{
		remove(FILE_B64);
		remove(FILE_ZIP);
	}
}