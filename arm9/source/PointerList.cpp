#include <nds.h>
#include "PointerList.h"

void PointerList_Add(PointerListEntry** list, void* ptr)
{
	PointerListEntry* listEntry = (PointerListEntry*)malloc(sizeof(PointerListEntry));
	listEntry->ptr = ptr;
	listEntry->previous = NULL;
	listEntry->next = *list;
	if(*list) (*list)->previous = listEntry;
	*list = listEntry;
}

void PointerList_RemoveEntry(PointerListEntry** list, PointerListEntry* entry)
{
	if(!entry) return;
	if(entry->previous) entry->previous->next = entry->next;
	if(entry->next) entry->next->previous = entry->previous;
	if(*list == entry) *list = entry->next;
	free(entry);
}

void PointerList_Remove(PointerListEntry** list, void* ptr)
{
	//Find the animation in the list
	PointerListEntry* entry = NULL;
	PointerListEntry* cur = *list;
	while(cur)
	{
		if(cur->ptr == ptr)
		{
			entry = cur;
			break;
		}
		cur = cur->next;
	}
	if(entry)
		PointerList_RemoveEntry(list, entry);
}

void PointerList_Clear(PointerListEntry** list)
{
	if(*list == NULL) return;
	PointerListEntry* cur = *list;
	while(cur)
	{
		PointerListEntry* cur2 = cur;
		cur = cur->next;
		free(cur2);
	}
	*list = NULL;
}

int PointerList_Contains(PointerListEntry** list, void* ptr)
{
	PointerListEntry* cur = *list;
	while(cur)
	{
		if(cur->ptr == ptr) return TRUE;
		cur = cur->next;
	}
	return FALSE;
}

void* PointerList_GetByIndex(PointerListEntry** list, int idx)
{
	int i = 0;
	PointerListEntry* cur = *list;
	while(cur)
	{
		if(i == idx) return cur->ptr;
		i++;
		cur = cur->next;
	}
	return NULL;
}