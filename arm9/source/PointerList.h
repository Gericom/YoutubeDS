#ifndef __POINTER_LIST_H__
#define __POINTER_LIST_H__

struct PointerListEntry
{
	PointerListEntry* previous;
	PointerListEntry* next;
	void* ptr;
};

void PointerList_Add(PointerListEntry** list, void* ptr);
void PointerList_RemoveEntry(PointerListEntry** list, PointerListEntry* entry);
void PointerList_Remove(PointerListEntry** list, void* ptr);
void PointerList_Clear(PointerListEntry** list);
int PointerList_Contains(PointerListEntry** list, void* ptr);
void* PointerList_GetByIndex(PointerListEntry** list, int idx);

#endif