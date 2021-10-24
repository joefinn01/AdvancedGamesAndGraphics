#include "InputManager.h"
#include "Engine/Helpers/DebugHelper.h"

Tag tag = L"InputManager";

bool InputManager::Subscribe(int iKeycode, InputObserver observer)
{
	if (iKeycode < 0 || iKeycode > 254)
	{
		LOG_ERROR(tag, L"Tried to subscribe to a key with the keycode %i but that key doesn't exist!", iKeycode);

		return false;
	}

	m_Keys[iKeycode].Observers.push_back(observer);

	return true;
}

bool InputManager::Unsubscribe(int iKeycode, InputObserver observer)
{
	if (iKeycode < 0 || iKeycode > 254)
	{
		LOG_ERROR(tag, L"Tried to unsubscribe from a key with the keycode %i but that key doesn't exist!", iKeycode);

		return false;
	}

	for (std::vector<InputObserver>::iterator it = m_Keys[iKeycode].Observers.begin(); it != m_Keys[iKeycode].Observers.end(); ++it)
	{
		if (*it == observer)
		{
			m_Keys[iKeycode].Observers.erase(it);

			return true;
		}
	}

	LOG_ERROR(tag, L"Tried to unsubscribe from a key with the keycode %i but the observer passed in doesn't match any!", iKeycode);

	return false;
}

bool InputManager::GetKeyState(int iKeycode)
{
	if (iKeycode < 0 || iKeycode > 254)
	{
		LOG_ERROR(tag, L"Tried to get the state of a key with the keycode %i but that key doesn't exist!", iKeycode);

		return false;
	}

	return m_Keys[iKeycode].Pressed;
}

void InputManager::Update()
{
	for (int i = 0; i < m_Keys.size(); ++i)
	{
		if (m_Keys[i].Pressed == true)
		{
			for (int j = 0; j < m_Keys[i].Observers.size(); ++j)
			{
				if (m_Keys[i].Observers[j].OnKeyHeld != nullptr)
				{
					m_Keys[i].Observers[j].OnKeyHeld(m_Keys[i].Observers[j].Object);
				}
			}
		}
	}
}

void InputManager::KeyDown(int iKeycode)
{
	if (m_Keys[iKeycode].Pressed == false)
	{
		m_Keys[iKeycode].Pressed = true;

		for (int i = 0; i < m_Keys[iKeycode].Observers.size(); ++i)
		{
			m_Keys[iKeycode].Observers[i].OnKeyDown(m_Keys[iKeycode].Observers[i].Object);
		}
	}
}

void InputManager::KeyUp(int iKeycode)
{
	if (m_Keys[iKeycode].Pressed == true)
	{
		m_Keys[iKeycode].Pressed = false;

		for (int i = 0; i < m_Keys[iKeycode].Observers.size(); ++i)
		{
			m_Keys[iKeycode].Observers[i].OnKeyUp(m_Keys[iKeycode].Observers[i].Object);
		}
	}
}

InputManager::InputManager()
{
	for (int i = 0; i < 255; ++i)
	{
		m_Keys[i] = KeyInfo();
	}
}
