
/* author : admin
 * date : 2014.12.03
 * description : 对象自动管理，引用计数
 */
 
#include "CRefObj.h"
#include "MacroDefine.h"


namespace NCommon
{

CRefObj::CRefObj()
{
	m_ref = 0;
}

CRefObj::~CRefObj()
{
	m_ref = -1;
}

int CRefObj::addRef()
{
	return ++m_ref;
}

int CRefObj::releaseRef()
{
	int ref = --m_ref;
	if (ref <= 0)
	{
		CRefObj* pThis = this;
		DELETE(pThis);
	}
	
	return ref;
}

int CRefObj::getRef() const
{
	return m_ref;
}



CAutoMgrInstance::CAutoMgrInstance(CRefObj* refInstance) : m_refInstance(refInstance)
{
	if (m_refInstance != NULL) m_refInstance->addRef();
}

CAutoMgrInstance::CAutoMgrInstance(const CAutoMgrInstance& obj) : m_refInstance(obj.m_refInstance)
{
	if (m_refInstance != NULL) m_refInstance->addRef();
}

CAutoMgrInstance::~CAutoMgrInstance()
{
	if (m_refInstance != NULL)
	{
		m_refInstance->releaseRef();
	    m_refInstance = NULL;
	}
}

int CAutoMgrInstance::getRef() const
{
	return (m_refInstance != NULL) ? m_refInstance->getRef() : -1;
}

CRefObj* CAutoMgrInstance::getInstance()
{
	return m_refInstance;
}

}

