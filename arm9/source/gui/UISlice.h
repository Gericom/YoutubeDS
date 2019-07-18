#ifndef __UISLICE_H__
#define __UISLICE_H__

class UIManager;

class UISlice
{
	friend class UIManager;
protected:
	int mX;
	int mY;
public:
	UISlice() 
		: mX(0), mY(0) 
	{ 
	
	}

	virtual ~UISlice()
	{

	}
	//Sets the position of the slice. Not all slices use this as their absolute position,
	//because sometimes this is only used for an animation
	void SetPosition(int x, int y)
	{
		mX = x;
		mY = y;
	}
	void GetPosition(int &x, int &y) { x = mX; y = mY; }

	virtual int OnPenDown(void* context, int x, int y) = 0;
	virtual int OnPenMove(void* context, int x, int y) = 0;
	virtual int OnPenUp(void* context, int x, int y) = 0;

	virtual void Render(void* context) = 0;

	virtual void CleanupVRAM() = 0;
};

#endif