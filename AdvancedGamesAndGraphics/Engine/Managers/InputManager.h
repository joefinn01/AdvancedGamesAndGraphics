#pragma once

// I decided to keep this input manager simple as it isn't vital to the actual application.
// I could improve it by adding an extra level of abstraction between the keycodes and the actual keys allowing for rebinding of keys.


#include "Engine/Structure/Singleton.h"
#include "Engine/Commons/Timer.h"

#include <unordered_map>
#include <vector>

typedef void (*InputCallback)(void*, int);
typedef void (*UpdateInputCallback)(void*, int, const Timer&);

struct InputObserver
{
	void* Object;
	InputCallback OnKeyDown;
	UpdateInputCallback OnKeyHeld;
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
	bool Subscribe(std::vector<int> keycodes, InputObserver observer);
	bool Unsubscribe(int iKeycode, InputObserver observer);

	bool GetKeyState(int iKeycode);

	void Update(const Timer& kTimer);

	void KeyDown(int iKeycode);
	void KeyUp(int iKeycode);

protected:

private:
	std::unordered_map<int, KeyInfo> m_Keys;
};

