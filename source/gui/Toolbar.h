#ifndef __TOOLBAR_H__
#define __TOOLBAR_H__
#include "font.h"
#include "util.h"
#include "UISlice.h"

#define TOOLBAR_BUTTON_INVALID	0
#define TOOLBAR_BUTTON_BACK		1
#define TOOLBAR_BUTTON_CLEAR	2
#define TOOLBAR_BUTTON_SEARCH	3
#define TOOLBAR_BUTTON_MENU		4

class Toolbar : public UISlice
{
public:
	typedef void (*OnButtonClickFunc)(void* context, int button);
private:
	union XBGR1555
	{
		uint16_t color;
		struct
		{
			uint16_t r : 5;
			uint16_t g : 5;
			uint16_t b : 5;
			uint16_t x : 1;
		};
	};
	XBGR1555 mBGColor;
	XBGR1555 mIconColor;
	XBGR1555 mTextColor;
	char* mTitle;
	NTFT_FONT* mFont;
	union ToolbarFlags
	{
		uint32_t flags;
		struct
		{
			uint32_t colorInvalidated : 1;
			uint32_t titleInvalidated : 1;
			uint32_t buttonsInvalidated : 1;
			uint32_t cursorInvalidated : 1;
			uint32_t : 12;
			uint32_t showBackButton : 1;
			uint32_t showClearButton : 1;
			uint32_t showSearchButton : 1;
			uint32_t showMenuButton : 1;
			uint32_t showCursor : 1;
			uint32_t : 11;
		};
	};
	ToolbarFlags mFlags;
	int mDownButton;
	int FindButtonByPoint(int x, int y);
	OnButtonClickFunc mOnButtonClickCallback;
	int mCursorX;//relative to the start of the text
public:
	Toolbar(uint16_t bgColor, uint16_t iconColor, uint16_t textColor, const char* title, NTFT_FONT* font)
		: mTitle(NULL), mFont(font), mDownButton(TOOLBAR_BUTTON_INVALID), mOnButtonClickCallback(NULL), mCursorX(0)
	{
		mFlags.flags = 0;
		SetBGColor(bgColor);
		SetIconColor(iconColor);
		SetTextColor(textColor);
		SetTitle(title);
		SetShowBackButton(FALSE);
		SetShowClearButton(FALSE);
		SetShowSearchButton(FALSE);
		SetShowMenuButton(TRUE);
		SetShowCursor(FALSE);
	}

	~Toolbar()
	{
		if(mTitle != NULL)
			free(mTitle);
	}

	void SetBGColor(uint16_t color)
	{
		mBGColor.color = color;
		mFlags.colorInvalidated = 1;
	}
	void SetIconColor(uint16_t color)
	{
		mIconColor.color = color;
		mFlags.colorInvalidated = 1;
	}
	void SetTextColor(uint16_t color)
	{
		mTextColor.color = color;
		mFlags.colorInvalidated = 1;
	}
	void SetTitle(const char* title)
	{
		if(mTitle != NULL)
			free(mTitle);
		if(title == NULL)
			mTitle = Util_CopyString("");
		else 
			mTitle = Util_CopyString(title);
		mFlags.titleInvalidated = 1;
	}
	void SetShowBackButton(int showBackButton)
	{
		mFlags.buttonsInvalidated |= mFlags.showBackButton != showBackButton;
		mFlags.showBackButton = showBackButton;
	}
	void SetShowClearButton(int showClearButton)
	{
		mFlags.buttonsInvalidated |= mFlags.showClearButton != showClearButton;
		mFlags.showClearButton = showClearButton;
	}
	void SetShowSearchButton(int showSearchButton)
	{
		mFlags.buttonsInvalidated |= mFlags.showSearchButton != showSearchButton;
		mFlags.showSearchButton = showSearchButton;
	}
	void SetShowMenuButton(int showMenuButton)
	{
		mFlags.buttonsInvalidated |= mFlags.showMenuButton != showMenuButton;
		mFlags.showMenuButton = showMenuButton;
	}
	void SetOnButtonClickCallback(OnButtonClickFunc callback)
	{
		mOnButtonClickCallback = callback;
	}
	void SetShowCursor(int showCursor)
	{
		mFlags.showCursor = showCursor;
		mFlags.cursorInvalidated = 1;
	}
	void SetCursorX(int x)
	{
		mCursorX = x;
		mFlags.cursorInvalidated = 1;
	}

	void Initialize();

	int OnPenDown(void* context, int x, int y);
	int OnPenMove(void* context, int x, int y);
	int OnPenUp(void* context, int x, int y);

	void Render(void* context);
	void CleanupVRAM();
};

#endif