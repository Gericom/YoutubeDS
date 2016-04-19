#include <nds.h>
#include <dswifi9.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>  
#include "youtube.h"
#include <ctype.h>
#include "happyhttp/happyhttp.h"

static const char* YT_VideoInfoURL = "/get_video_info?video_id=";

static void urldecode2(char *dst, const char *src)
{
	char a, b;
	while (*src) {
		if ((*src == '%') &&
			((a = src[1]) && (b = src[2])) &&
			(isxdigit(a) && isxdigit(b))) {
				if (a >= 'a')
					a -= 'a'-'A';
				if (a >= 'A')
					a -= ('A' - 10);
				else
					a -= '0';
				if (b >= 'a')
					b -= 'a'-'A';
				if (b >= 'A')
					b -= ('A' - 10);
				else
					b -= '0';
				*dst++ = 16*a+b;
				src+=3;
		} else if (*src == '+') {
			*dst++ = ' ';
			src++;
		} else {
			*dst++ = *src++;
		}
	}
	*dst++ = '\0';
}

static uint8_t* response;
static uint8_t* pResponse;
static int count;

static volatile char* videourl = NULL;

void OnBegin( const happyhttp::Response* r, void* userdata )
{
	response = (uint8_t*)malloc(64 * 1024);
	pResponse = response;
	count = 0;
	videourl = NULL;
}

// invoked to process response body data (may be called multiple times)
void OnData( const happyhttp::Response* r, void* userdata, const unsigned char* data, int n )
{
	memcpy(pResponse, data, n);
	pResponse += n;
	count += n;
}

// invoked when response is complete
void OnComplete( const happyhttp::Response* r, void* userdata )
{
	char tmpbuf[26 + 1];
	memset(&tmpbuf[0], 0, sizeof(tmpbuf));
	pResponse = response;
	char* tagstart = (char*)pResponse;
	int taglen = 0;
	while(pResponse < response + count)
	{
		//find =
		tagstart = (char*)pResponse;
		taglen = 0;
		while(*((char*)pResponse) != '=')
		{
			taglen++;
			pResponse++;
		}
		if(taglen == 26)
		{
			memset(&tmpbuf[0], 0, sizeof(tmpbuf));
			memcpy(tmpbuf, tagstart, taglen);
			//printf(tmpbuf);
			//printf("\n");
			if(!strcmp(tmpbuf, "url_encoded_fmt_stream_map"))
			{
				//Let's find the content
				pResponse++; //skip =
				char* contentstart = (char*)pResponse;
				int contentlen = 0;
				while(pResponse < response + count && *((char*)pResponse) != '&')
				{
					pResponse++;
					contentlen++;
				}
				printf("Len: %d\n", contentlen);
				char* src = (char*)malloc(contentlen + 1);
				memset(src, 0, contentlen + 1);
				memcpy(src, contentstart, contentlen);
				free(response);
				urldecode2(src, src);
				//puts(src);
				int totallength = strlen(src);
				//
				char* pSrc = src;
				//stuff is often in different orders
				//for each entry there is fallback_host(13), itag(4), url(3) and type(4)
				//sometimes there is quality(7) at the top, and sometimes at the bottom; ignore it
				//everything, except of itag and type, can be identified by the length of the tag	
				int partmask = 0;
				int itag = 0;
				char* pURL = NULL;
				int urlLength = 0;
				while(pSrc < src + totallength)
				{
					char* tagstart2 = pSrc;
					int taglength2 = 0;
					while(pSrc < src + totallength && *((char*)pSrc) != '=')
					{
						taglength2++;
						pSrc++;
					}
					pSrc++;
					char* valuestart2 = pSrc;
					int valuelen = 0;
					while(pSrc < src + totallength && *((char*)pSrc) != '&')
					{
						valuelen++;
						pSrc++;
					}
					pSrc++;
					int mask = 0;
					if(taglength2 == 13)
						mask |= 1;
					else if(taglength2 == 4 && tagstart2[0] == 'i')
						mask |= 2;
					else if(taglength2 == 3)
						mask |= 4;
					else if(taglength2 == 4 && tagstart2[0] == 't')
						mask |= 8;

					if(partmask & mask)
					{
						printf("partmask full\n");
						if(itag == 17)
						{
							goto itag_17;
						}
						partmask = 0;
						itag = 0;
						pURL = NULL;
						urlLength = 0;
					}

					if(taglength2 == 13)
						partmask |= 1;
					else if(taglength2 == 4 && tagstart2[0] == 'i')
					{
						partmask |= 2;
						memset(&tmpbuf[0], 0, sizeof(tmpbuf));
						if(valuelen > 2) valuelen = 2;
						memcpy(&tmpbuf[0], valuestart2, valuelen);
						itag = atoi(&tmpbuf[0]);
						printf("itag: %d\n", itag);
					}
					else if(taglength2 == 3)
					{
						partmask |= 4;
						pURL = valuestart2;
						urlLength = valuelen;
					}
					else if(taglength2 == 4 && tagstart2[0] == 't')
						partmask |= 8;

					printf("taglen: %d\n", taglength2);
				}
				if(itag == 17)
				{
				itag_17:
					printf("itag == 17\n");
					char* url = (char*)malloc(urlLength + 1);
					memset(url, 0, urlLength + 1);
					memcpy(url, pURL, urlLength);
					free(src);
					urldecode2(url, url);
					printf(url);
					videourl = url;
					return;
				}
				printf("FAIL!\n");
				while(1);
			}
		}
		while(*((char*)pResponse) != '&')
		{
			pResponse++;
		}
		pResponse++;
	}
	printf("FAIL!\n");
	while(1);
}

static happyhttp::Connection* mConnection;

char* YT_GetVideoInfo(char* id)
{
	int length = strlen(id) + strlen(YT_VideoInfoURL);
	char* resultUrl = (char*)malloc(length + 1);
	memset(resultUrl, 0, length + 1);
	strcpy(resultUrl, YT_VideoInfoURL);
	strcat(resultUrl, id);
	iprintf(resultUrl);
	mConnection = new happyhttp::Connection( "www.youtube.com", 80 );
	mConnection->setcallbacks( OnBegin, OnData, OnComplete, 0 );
	mConnection->request( "GET", resultUrl);//"/happyhttp/test.php" );
	free(resultUrl);
	videourl = NULL;
	while( mConnection->outstanding() && videourl == NULL )
		mConnection->pump();
	mConnection->close();
	delete mConnection;
	return (char*)videourl;
}

//We should get itag=17 in url_encoded_fmt_stream_map