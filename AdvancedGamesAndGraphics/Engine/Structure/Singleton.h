#pragma once

template<class T>
class Singleton
{
public:
	static T* GetInstance() const
	{
		if (m_pInstance == nullptr)
		{
			m_pInstance = new T();
		}

		return m_pInstance;
	}

protected:

private:
	static T* m_pInstance;
};