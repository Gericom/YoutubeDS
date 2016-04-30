#ifndef __SCREENKEYBOARD_H__
#define __SCREENKEYBOARD_H__
#include "UISlice.h"

#define SCREENKEYBOARD_MODE_QWERTY_LOWER	0
#define SCREENKEYBOARD_MODE_QWERTY_UPPER	1
#define SCREENKEYBOARD_MODE_NUMSYM			2

#define SCREENKEYBOARD_BUTTON_SPECIAL_MODESWITCH	((1 << 16) | 0)
#define SCREENKEYBOARD_BUTTON_SPECIAL_CAPS			((1 << 16) | 1)
#define SCREENKEYBOARD_BUTTON_SPECIAL_SEARCH		((1 << 16) | 2)
#define SCREENKEYBOARD_BUTTON_SPECIAL_BACKSPACE		((1 << 16) | 3)

class ScreenKeyboard : public UISlice
{
public:
	typedef void (*OnButtonClickFunc)(void* context, int button);
private:
	int mKeyboardMode;
	int mDownButton;
	int FindButtonByPoint(int x, int y);
	void GetButtonCenter(int &x, int &y);
	OnButtonClickFunc mOnButtonClickCallback;
	int mShowRipple;
	int mRippleCenterX;
	int mRippleCenterY;
	int mRippleFrame;
	int mShowing;
	int mHiding;
	int mShowHideFrame;
	int mHidden;
	int mShowSpaceHighlight;
public:
	ScreenKeyboard()
		: mKeyboardMode(SCREENKEYBOARD_MODE_QWERTY_LOWER), mDownButton(0), mOnButtonClickCallback(NULL), mShowRipple(FALSE),
			mShowing(FALSE), mHiding(FALSE), mShowHideFrame(0), mHidden(TRUE), mShowSpaceHighlight(FALSE)
	{ }

	void Initialize();

	int OnPenDown(void* context, int x, int y);
	int OnPenMove(void* context, int x, int y);
	int OnPenUp(void* context, int x, int y);

	void Render(void* context);
	void CleanupVRAM();

	void SetKeyboardMode(int mode)
	{
		mKeyboardMode = mode;
	}
	void SetOnButtonClickCallback(OnButtonClickFunc callback)
	{
		mOnButtonClickCallback = callback;
	}
	void Show()
	{
		if(mShowing || !mHidden) return;
		SetKeyboardMode(SCREENKEYBOARD_MODE_QWERTY_LOWER);
		mShowing = TRUE;
		if(mHiding)
		{
			mHiding = FALSE;
			mShowHideFrame = 9 - mShowHideFrame;
		}
		else
			mShowHideFrame = 0;
	}
	void Hide()
	{
		if(mHiding || mHidden) return;
		mHiding = TRUE;
		if(mShowing)
		{
			mShowing = FALSE;
			mShowHideFrame = 9 - mShowHideFrame;
		}
		else
			mShowHideFrame = 0;
	}
	int IsHidden()
	{
		return mHidden;
	}
};

#endif