
#include "mail_handler.h"


int main(int argc, char *argv[])						  
{
	Mail_Data mail;
	Mail_Attach attach;
	Mail_Buffer buff;

	memset(&mail, 0, sizeof(mail));
	memset(&attach, 0, sizeof(attach));
	memset(&buff, 0, sizeof(buff));

	attach.yes = FALSE,
	attach.zip = FALSE;

	if (argc < 4 || argc > 6) 
	{
		printf( HELP );	
		return 0;
	}
	else if ( argc == 4 )
	{
		setBaseArgs(&mail, argv);
	}	
	else if (argc == 5)
	{
		setBaseArgs(&mail, argv);			
		setAttachArgs(&attach, argv[4]);

		readB64File(&attach, argv[4]);
	}
	else if (argc == 6) 
	{
		if (!strcmp( argv[5], "-z" ))
		{
			setBaseArgs(&mail, argv);
			setAttachArgs(&attach, argv[4]);			
			setZipArgs(&attach);

			readB64File(&attach, argv[4]);
		}
		else
		{
			perror("Unknown key! Maybe you meant '-z'?");
			return 0;
		}		
	}

	if (!sendMail(&mail, &attach, &buff))
		printf("Mail not sended");

	return 0;
}