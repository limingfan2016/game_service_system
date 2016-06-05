
/* author : limingfan
 * date : 2014.12.03
 * description : 对象自动管理，引用计数
 */
 
#ifndef CREFOBJ_H
#define CREFOBJ_H

namespace NCommon
{
// 对象自管理，引用计数
class CRefObj
{
public:
	int addRef();
	int releaseRef();
	int getRef() const;
	
protected:
	CRefObj();
	virtual ~CRefObj();
	
private:
    // 禁止拷贝、赋值
    CRefObj(const CRefObj&);
	CRefObj& operator =(const CRefObj&);
	
private:
    int m_ref;
};


// 自动管理对象，智能对象
// 会自动销户引用计数对象，管理对象的生命周期
class CAutoMgrInstance
{
public:
	int getRef() const;
	CRefObj* getInstance();
	
public:
	CAutoMgrInstance(CRefObj* refInstance);
	CAutoMgrInstance(const CAutoMgrInstance& obj);
	~CAutoMgrInstance();
	
private:
    // 禁止拷贝、赋值
	CAutoMgrInstance& operator =(const CAutoMgrInstance&);
	CAutoMgrInstance(CRefObj& refInstance);
	CAutoMgrInstance(CRefObj);
	
private:
    CRefObj* m_refInstance;
};

}

#endif // CREFOBJ_H
