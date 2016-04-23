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
#include "youtube_apikey.h"
#include "rapidjson/document.h"

using namespace rapidjson;

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
					puts(url);
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

char* YT_GetVideoInfo(char* id)
{
	int length = strlen(id) + strlen(YT_VideoInfoURL);
	char* resultUrl = (char*)malloc(length + 1);
	memset(resultUrl, 0, length + 1);
	strcpy(resultUrl, YT_VideoInfoURL);
	strcat(resultUrl, id);
	iprintf(resultUrl);
	happyhttp::Connection* mConnection = new happyhttp::Connection( "www.youtube.com", 80 );
	response = (uint8_t*)malloc(64 * 1024);
	pResponse = response;
	count = 0;
	videourl = NULL;
	mConnection->setcallbacks( NULL, OnData, OnComplete, 0 );
	mConnection->request( "GET", resultUrl);
	free(resultUrl);
	videourl = NULL;
	while( mConnection->outstanding() && videourl == NULL )
		mConnection->pump();
	mConnection->close();
	delete mConnection;
	return (char*)videourl;
}

//We should get itag=17 in url_encoded_fmt_stream_map

static char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

static char *url_encode(char *str) {
  char *pstr = str, *buf = (char*)malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
      *pbuf++ = *pstr;
    else if (*pstr == ' ') 
      *pbuf++ = '+';
    else 
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

static char* copyString(const char* string)
{
	int len = strlen(string);
	char* result = (char*)malloc(len + 1);
	memcpy(result, string, len + 1);
	return result;
}

//Search and parsing of results
YT_SearchListResponse* YT_Search(char* query, char* pageToken)
{
	happyhttp::Connection* mConnection = new happyhttp::Connection("florian.nouwt.com", 80);
	int qlen = strlen(query);
	char* query2 = (char*)malloc(qlen + 1);
	for(int i = 0; i < qlen; i++)
	{
		if(query[i] != ' ')
			query2[i] = query[i];
		else query2[i] = '+';
	}
	query2[qlen] = 0;
	char* query3 = url_encode(query2);
	free(query2);
	char* url = NULL;
	if(pageToken == NULL)
		asprintf(&url, "https://www.googleapis.com/youtube/v3/search?part=snippet&q=%s&type=video&key=%s", query3, youtube_apikey);
	else 
		asprintf(&url, "https://www.googleapis.com/youtube/v3/search?part=snippet&q=%s&type=video&pageToken=%s&key=%s", query3, pageToken, youtube_apikey);
	char* url2 = url_encode(url);
	free(url);
	char* url3 = NULL;
	asprintf(&url3, "/ythttps.php?u=%s", url2);
	free(url2);
	response = (uint8_t*)malloc(64 * 1024);
	pResponse = response;
	count = 0;
	mConnection->setcallbacks( NULL, OnData, NULL, 0 );
	mConnection->request( "GET", url3);
	free(url3);
	while(mConnection->outstanding())
		mConnection->pump();
	mConnection->close();
	delete mConnection;
	pResponse[0] = 0;
	Document document;
	document.ParseInsitu((char*)response);
	YT_SearchListResponse* result = (YT_SearchListResponse*)malloc(sizeof(YT_SearchListResponse));
	memset(result, 0, sizeof(YT_SearchListResponse));
	if(document.HasMember("prevPageToken"))
		result->prevPageToken = copyString(document["prevPageToken"].GetString());
	if(document.HasMember("nextPageToken"))
		result->prevPageToken = copyString(document["nextPageToken"].GetString());
	result->totalNrResults = document["pageInfo"]["totalResults"].GetInt();
	result->nrResultsPerPage = document["pageInfo"]["resultsPerPage"].GetInt();
	result->searchResults = (YT_SearchResult*)malloc(result->nrResultsPerPage * sizeof(YT_SearchResult));
	memset(result->searchResults, 0, result->nrResultsPerPage * sizeof(YT_SearchResult));
	for(int i = 0; i < result->nrResultsPerPage; i++)
	{
		YT_SearchResult* cur = &result->searchResults[i];
		cur->videoId = copyString(document["items"][i]["id"]["videoId"].GetString());
		cur->title = copyString(document["items"][i]["snippet"]["title"].GetString());
		cur->description = copyString(document["items"][i]["snippet"]["description"].GetString());
		cur->thumbnail = copyString(document["items"][i]["snippet"]["thumbnails"]["default"]["url"].GetString());
		cur->channelId = copyString(document["items"][i]["snippet"]["channelId"].GetString());
		cur->channelTitle = copyString(document["items"][i]["snippet"]["channelTitle"].GetString());
	}
	free(response);
	return result;
}

void YT_FreeSearchListResponse(YT_SearchListResponse* response)
{
	if(response->prevPageToken != NULL)
		free(response->prevPageToken);
	if(response->nextPageToken != NULL)
		free(response->nextPageToken);
	for(int i = 0; i < response->nrResultsPerPage; i++)
	{
		YT_SearchResult* cur = &response->searchResults[i];
		free(cur->videoId);
		free(cur->title);
		free(cur->description);
		free(cur->thumbnail);
		free(cur->channelId);
		free(cur->channelTitle);
	}
	free(response->searchResults);
	free(response);
}