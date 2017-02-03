/* Jonah Groendal
 * CS5950
 * Program 2
 * 
 * This is a secure server program that follows a specific, predefined protocol and 
 * validates each message it receives.
 */

#include "message.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <regex.h>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>

#define USERNAME_MAX_LEN	32
#define FILEPATH_REGEX	"^/(([0-9]|[a-z]|[A-Z]|-|_|\\.| )+?/?)+([0-9]|[a-z]|[A-Z]|-|_|\\.| |/)$"

MessageType0 * receive_type0(int sock);
MessageType3 * receive_type3(int sock);
MessageType6 * receive_type6(int sock);
void send_type1(int sock, char sessionId[], int sidLength);
void send_type4(int sock, char contentBuffer[MAX_CONTENT_LENGTH+1], 
                int contentLength, char sessionId[SID_LENGTH+1], int sidLength);
void send_type5(int sock, char *sessionId, int sidLength);
int validate_msg(void *msg, int len, int type);
int validate_pathName(char *pathName);
Header generate_header(unsigned char messageType);
unsigned int generate_id(char sessionId[SID_LENGTH+1]);
int check_acl(char *username, char *filename);

int main()
{
    // create socket
    int sock = nn_socket(AF_SP, NN_PAIR);
    // connect to the socket
    nn_bind(sock, IPC_ADDR);

    // seed rand()
    srand(time(NULL));

    for (;;)
    {
        // receive type 0 message from socket
        MessageType0 *msg0;
        msg0 = receive_type0(sock);
        assert(msg0 != NULL);

        // generate session id
        char sessionId[SID_LENGTH+1];
        unsigned int sidLength = generate_id(sessionId);
        // send id as type 1 message
        send_type1(sock, sessionId, sidLength);

        // receive message
        MessageType3 *msg3;
        msg3 = receive_type3(sock);
        assert(msg3 != NULL);

        // check if user has access to file
        if (check_acl(msg0->distinguishedName, msg3->pathName))
        {
            printf("Username '%s' not in .acl file!\n", msg0->distinguishedName);
        }
        else
        {
            // send file as type 4 message --------------------------------------
            FILE *fp = fopen(msg3->pathName, "r");
            assert(fp != NULL);
            // while not EOF, send up to MAX_CONTENT_LENGTH bytes from file
            char inbuff[MAX_CONTENT_LENGTH+1] = {0};
            int i = 0;
            do
            {
                while (i < MAX_CONTENT_LENGTH && inbuff[i-1] != EOF)
                { 
                    inbuff[i] = fgetc(fp);
                    i++;
                }
                inbuff[i] = '\0';

                //send file as type 4 message
                send_type4(sock, inbuff, i, sessionId, sidLength);

                // receive acknowledgement as type 6 message
                MessageType6 *msg6;
                msg6 = receive_type6(sock);
                assert(msg6 != NULL);

            } while (inbuff[i-1] != EOF);
            // ---------------------------------------------------------------------
        }

        // send type 5 message to terminate session
        send_type5(sock, sessionId, sidLength);

        // receive acknowledgement as type 6 message
        MessageType6 *msg6;
        msg6 = receive_type6(sock);
        assert(msg6 != NULL);
    }
}

/* Ensures content of message adheres to the specifications of its type. 
 *
 * RETURN VALUE:
 *  - 0: message is the expected type and valid
 *  - INVALID_MESSAGE_LENGTH
 *  - INVALID_MESSAGE_TYPE
 */
int validate_msg(void *msg, int len, int type)
{
    int retval = 0;
    MessageType0 *msg0;
    MessageType3 *msg3;
    MessageType6 *msg6;
    switch (type)
    {
        case TYPE0:
            msg0 = msg;
            // check length of message
            if (len != sizeof(MessageType0)) retval = INVALID_MESSAGE_LENGTH;
            // check type in header
            if (msg0->header.messageType != TYPE0) retval = INVALID_MESSAGE_TYPE;
            // check lenghth of distinguishedName
            if (msg0->dnLength < 1 || msg0->dnLength > DN_LENGTH) retval = INVALID_MESSAGE_RECVD;
            // ensure dnLength is accurate
            if (strnlen(msg0->distinguishedName, DN_LENGTH+1) != msg0->dnLength) retval = INVALID_MESSAGE_RECVD;
            break;
        case TYPE3:
            msg3 = msg;
            // check length of message
            if (len != sizeof(MessageType3)) retval = INVALID_MESSAGE_LENGTH;
            // check type in header
            if (msg3->header.messageType != TYPE3) retval = INVALID_MESSAGE_TYPE;
            // check length of sessionId
            if (msg3->sidLength != SID_LENGTH) retval = INVALID_MESSAGE_RECVD;
            // check length of pathName
            if (msg3->pathLength < 2 || msg3->pathLength > PATH_MAX) retval = INVALID_MESSAGE_RECVD;
            // ensure sidLength is accurate
            if (strnlen(msg3->sessionId, SID_LENGTH+1) !=  msg3->sidLength) retval = INVALID_MESSAGE_RECVD;
            // ensure pathLength is accurate
            if (strnlen(msg3->pathName, PATH_MAX+1) !=  msg3->pathLength) retval = INVALID_MESSAGE_RECVD;
            // ensure pathName is a valid and full filepath
            if (validate_pathName(msg3->pathName)) retval = INVALID_MESSAGE_RECVD;
            break;
        case TYPE6:
            msg6 = msg;
            // check length of message
            if (len != sizeof(MessageType6)) retval = INVALID_MESSAGE_LENGTH;
            // check type in header
            if (msg6->header.messageType != TYPE6) retval = INVALID_MESSAGE_TYPE;
            // check length of sessionId
            if (msg6->sidLength != SID_LENGTH) retval = INVALID_MESSAGE_RECVD;
            // ensure sidLength is accurate
            if (strnlen(msg6->sessionId, SID_LENGTH+1) !=  msg6->sidLength) retval = INVALID_MESSAGE_RECVD;
            break;
    }
    
    // if message is invalid, print why
    switch (retval)
    {
        case INVALID_MESSAGE_LENGTH:
            printf("Invalid length for type %d message!\n", type);
            break;
        case INVALID_MESSAGE_TYPE:
            printf("Invalid type value for type %d message!\n", type);
            break;
        case INVALID_MESSAGE_RECVD:
            printf("Invalid type %d message!\n", type);
            break;
    }

    return retval;
}
/*
 * Ensures pathName is a full filepath and syntactically correct.
 * For use in validate_msg().
 *
 * RETURN VALUE:
 *   0: pathName is correct
 *  -1: pathname is not correct
 */
int validate_pathName(char *pathName)
{
    // compile regular expression and save it in 'regex'
	regex_t regex;
	if ((regcomp(&regex, FILEPATH_REGEX, REG_EXTENDED)) != 0)
	{
    	fprintf(stdout, "Could not compile regex\n");
    	exit(1);
	}

    int retval = 0;
    // compare pathName to regular expression value
	if ((regexec(&regex, pathName, 0, NULL, 0)) != 0)
	{
		printf("Path name is not syntactically correct!\n");
    	retval = 1;
	}
	// free memory allocated by regcomp()
	regfree(&regex);

    return retval;

}

/*
 * Ensures .acl file is not malformed and that username is listed
 * within.
 *
 * RETURN VALUE:
 *   0: not malformed and username is listed
 *  -1: malformed or usernmame not listed
 */
int check_acl(char *username, char *filename)
{
    // create copy of 'filename' with '.acl' appended
	char *aclFilename = calloc(strlen(filename)+5, sizeof(char));
	strncpy(aclFilename, filename, strlen(filename)+5);
	if (*(aclFilename+strlen(filename)-1) == '/')
		*(aclFilename+strlen(filename)-1) = '\0';
	strcat(aclFilename, ".acl");

    // open <source>.acl
	FILE *fpacl;
	if ((fpacl = fopen(aclFilename, "r")) == NULL)
	{
		printf("Cannot open file <%s>\n", aclFilename);
		exit(0);
	}
    // buffer for each username entry
    char *buff = calloc(USERNAME_MAX_LEN+1, sizeof(char));
	// check if <source>.acl is malformed
	// make sure each line is a valid username
	int malformed = 0;
	while (fgets(buff, USERNAME_MAX_LEN+1, fpacl) != NULL && !malformed)
	{
		if (*(buff) == '-')
			malformed = 1;	// first character of username is a hyphen
		int i = 0;
		while (*(buff+i) != '\n' && !malformed && i < strlen(buff))
		{
			if (!((*(buff+i) >= 'A' && *(buff+i) <= 'Z') ||
				  (*(buff+i) >= 'a' && *(buff+i) <= 'z') ||
				  (*(buff+i) >= '0' && *(buff+i) <= '9') ||
				   *(buff+i) == '.' || *(buff+i) <= '-'  ||
				   *(buff+i) == '_'))
				malformed = 1;	// username contains invalid characters
			
			if (i > USERNAME_MAX_LEN-1)
				malformed = 1;	// username is > 32 characters
			i++;
		}
	}
	if (malformed)
	{
		return -1;	// <source>.acl is malformed
	}
	// move back to beginning of file
	rewind(fpacl);

    // Check if username is in <source>.acl
	int found = 0;
	while (fgets(buff, USERNAME_MAX_LEN+1, fpacl) != NULL && !found)
	{
		// Remove \n from end of string
		if (*(buff+strlen(buff)-1) == '\n')
			*(buff+strlen(buff)-1) = '\0';

		if (strcmp(buff, username) == 0)
		{
			found = 1;	// <source>.acl contains USER's username
		}
	}

	if (found) return 0;
    else return -1;
}

/*
 * Generates and returns Header for given messageType.
 */
 Header generate_header(unsigned char messageType)
 {
     Header header;
     header.messageType = messageType;
     switch (messageType)
     {
         case TYPE1:
            header.messageLength = sizeof(MessageType1);
            break;
        case TYPE2:
            header.messageLength = sizeof(MessageType2);
            break;
        case TYPE4:
            header.messageLength = sizeof(MessageType4);
            break;
        case TYPE5:
            header.messageLength = sizeof(MessageType5);
            break;
     }

     return header;
 }

/* 
 * Generates a session id and returns its length
 */
unsigned int generate_id(char sessionId[SID_LENGTH+1])
{
    // fill array with random characters
    int i;
    int chartype;
    for (i=0; i<SID_LENGTH; i++)
    {
        chartype = rand() % 5;
        switch (chartype)
        {
            // generate random char [0-9]
            case 0:
                sessionId[i] = rand() % 10 + 48;
                break;
            // or generate random char [A-Z]
            case 1: case 2:
                sessionId[i] = rand() % 26 + 65;
                break;
            // or generate random char [a-z]
            case 3: case 4:
                sessionId[i] = rand() % 26 + 97;
                break;
        }
    }
    sessionId[SID_LENGTH] = '\0';

    // return length of session id
    return SID_LENGTH;
}

/*
 * receives a message and validates that it's type 3.
 *
 * RETURN VALUE:
 *     pointer to _type3 struct containing message on success
 *     NULL if error occured
 */
 MessageType0 * receive_type0(int sock)
 {
    // receive message
    MessageType0 *msg = NULL;
    int len = nn_recv(sock, &msg, NN_MSG, 0);

    // ensure it is of type 0 and valid
    if (validate_msg(msg, len, TYPE0)) return NULL;

    return msg;
 }
 /*
  * Sends sessionId to establish connection
  */
void send_type1(int sock, char sessionId[SID_LENGTH+1], int sidLength)
{
    // create _type1 message, generate header, and assign session id
    MessageType1 msg;
    msg.header = generate_header(TYPE1);
    msg.sidLength = sidLength;
    strcpy(msg.sessionId, sessionId);

    // send message
    nn_send(sock, &msg, sizeof(msg), 0);
}
/*
 * Sends error message.
 *
 * RETURN VALUE:
 *  0 - message sent
 *  1 - message not sent, > 256 bytes
 */
void send_type2(int sock, char *errMsg)
{

}
/*
 * receives a message and validates that it's type 3.
 * If an error occurs, errno is set accordingly.
 *
 * RETURN VALUE:
 *     pointer to _type3 struct containing message on success
 *     NULL if error occured
 */
MessageType3 * receive_type3(int sock)
{
    // receive message
    MessageType3 *msg = NULL;
    int len = nn_recv(sock, &msg, NN_MSG, 0);

    // ensure it is of type 3 and valid
    if (validate_msg(msg, len, TYPE3)) return NULL;

    return msg;
}

/*
 * Sends the contents of a file as a type 4 message.
 *
 * RETURN VALUE:
 *  0 - successfully sent
 *  1 - not sent, fileContents > 4096 bytes
 */
 void send_type4(int sock, char contentBuffer[MAX_CONTENT_LENGTH+1], 
                 int contentLength, char sessionId[SID_LENGTH+1], int sidLength)
{
    // create _type4 message and generate header
    MessageType4 msg;
    msg.header = generate_header(TYPE4);

    // set attributes
    msg.sidLength = sidLength;
    msg.contentLength = contentLength;
    strcpy(msg.sessionId, sessionId);
    strcpy(msg.contentBuffer, contentBuffer);

    // send message 
    nn_send(sock, &msg, sizeof(msg), 0);

}
/*
 * Sends type 5 message to terminate a session
 */
void send_type5(int sock, char *sessionId, int sidLength)
{
    // create _type4 message and generate header
    MessageType5 msg;
    msg.header = generate_header(TYPE5);

    // set attributes
    msg.sidLength = sidLength;
    strcpy(msg.sessionId, sessionId);

    // send message 
    nn_send(sock, &msg, sizeof(msg), 0);
}
/*
 * receives a message and validates that it's type 6.
 * 
 * RETURN VALUE:
 *    pointer to _type6 struct on success
 *    NULL if error ocurred
 */
MessageType6 * receive_type6(int sock)
{
    // receive message
    MessageType6 *msg = NULL;
    int len = nn_recv(sock, &msg, NN_MSG, 0);

    // ensure it is of type 6 and valid
    if (validate_msg(msg, len, TYPE6)) return NULL;

    return msg;
}
