/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _minivec_h
#define _minivec_h

/////////////////////////////////////////////////////////////////////////////
// Class: MiniVec
// --------------
// A very simple & generic vector implementation. Used to cut down
// on code size, so that the project is small enough for the
// CodeWarrior limited linker.

template<class T>
class MiniVec
{
public:
	MiniVec() : m_pVecData(0), m_nItems(0), m_nSize(0) {}
	~MiniVec() { if (m_pVecData) delete [] m_pVecData; }

	int32 CountItems() const { return m_nItems; }
	T& operator[](int32 index) const { return ItemAt(index); }
	T& ItemAt(int32 index) const { return m_pVecData[index]; }

	void Push(const T& item)
	{
		GrowIfNeeded(1);
		m_pVecData[m_nItems++] = item;
	}
	
	const T& Pop()
	{
		return m_pVecData[--m_nItems];
	}
	
private:
	void GrowIfNeeded(int32 numItems)
	{
		if ((! m_pVecData) || (m_nItems + numItems > m_nSize))
			Resize(m_nSize + s_nAllocBlock);
	}
	
	void Resize(int32 newSize)
	{
		T* newData = new T[newSize];
		
		if (m_nItems > newSize)
			m_nItems = newSize;
		
		m_nSize = newSize;
		for (int32 i=0; i<m_nItems; i++)
			newData[i] = m_pVecData[i];
		
		if (m_pVecData)
			delete [] m_pVecData;
		
		m_pVecData = newData;		
	}
	
private:
	T* m_pVecData;
	int32 m_nItems;
	int32 m_nSize;
	static const int32 s_nAllocBlock;
};

template<class T>
const int32 MiniVec<T>::s_nAllocBlock = 50;

#endif /* _minivec_h */
