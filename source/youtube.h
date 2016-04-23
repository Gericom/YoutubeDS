#ifndef __YOUTUBE_H__
#define __YOUTUBE_H__

char* YT_GetVideoInfo(char* id);

typedef struct
{
	char* videoId;
	char* title;
	char* description;
	char* thumbnail;
	char* channelId;
	char* channelTitle;
} YT_SearchResult;

typedef struct
{
	char* prevPageToken;
	char* nextPageToken;
	int totalNrResults;
	int nrResultsPerPage;
	YT_SearchResult* searchResults;
} YT_SearchListResponse;

YT_SearchListResponse* YT_Search(char* query, char* pageToken);
void YT_FreeSearchListResponse(YT_SearchListResponse* response);

#endif