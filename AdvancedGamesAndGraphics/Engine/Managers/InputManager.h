#pragma once

#include "Engine/Structure/Singleton.h"

#include <unordered_map>
#include <vector>

typedef void (*InputCallback)(void*);

struct InputObserver
{
	void* Object;
	InputCallback OnKeyDown;
	InputCallback OnKeyHeld;
	InputCallback OnKeyUp;

	bool operator== (InputObserver rhs)
	{
		return memcmp(this, &rhs, sizeof(InputObserver)) == 0;
	}
};

struct KeyInfo
{
	KeyInfo()
	{
		Pressed = false;
		Observers = std::vector<InputObserver>();
	}

	bool Pressed;
	std::vector<InputObserver> Observers;
};

class InputManager : public Singleton<InputManager>
{
public:
	InputManager();

	bool Subscribe(int iKeycode, InputObserver observer);
	bool Unsubscribe(int iKeycode, InputObserver observer);

	bool GetKeyState(int iKeycode);

	void Update();

	void KeyDown(int iKeycode);
	void KeyUp(int iKeycode);

protected:

private:
	std::unordered_map<int, KeyInfo> m_Keys;
};

