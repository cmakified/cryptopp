#ifndef CRYPTOPP_ALGPARAM_H
#define CRYPTOPP_ALGPARAM_H

#include "cryptlib.h"
#include "smartptr.h"
#include "secblock.h"

NAMESPACE_BEGIN(CryptoPP)

//! used to pass byte array input as part of a NameValuePairs object
/*! the deepCopy option is used when the NameValuePairs object can't
	keep a copy of the data available */
class ConstByteArrayParameter
{
public:
	ConstByteArrayParameter(const char *data = NULL, bool deepCopy = false)
	{
		Assign((const byte *)data, data ? strlen(data) : 0, deepCopy);
	}
	ConstByteArrayParameter(const byte *data, unsigned int size, bool deepCopy = false)
	{
		Assign(data, size, deepCopy);
	}
	template <class T> ConstByteArrayParameter(const T &string, bool deepCopy = false)
	{
		CRYPTOPP_COMPILE_ASSERT(sizeof(string[0])==1);
		Assign((const byte *)string.data(), string.size(), deepCopy);
	}

	void Assign(const byte *data, unsigned int size, bool deepCopy)
	{
		if (deepCopy)
			m_block.Assign(data, size);
		else
		{
			m_data = data;
			m_size = size;
		}
		m_deepCopy = deepCopy;
	}

	const byte *begin() const {return m_deepCopy ? m_block.begin() : m_data;}
	const byte *end() const {return m_deepCopy ? m_block.end() : m_data + m_size;}
	unsigned int size() const {return m_deepCopy ? m_block.size() : m_size;}

private:
	bool m_deepCopy;
	const byte *m_data;
	unsigned int m_size;
	SecByteBlock m_block;
};

class ByteArrayParameter
{
public:
	ByteArrayParameter(byte *data = NULL, unsigned int size = 0)
		: m_data(data), m_size(size) {}
	ByteArrayParameter(SecByteBlock &block)
		: m_data(block.begin()), m_size(block.size()) {}

	byte *begin() const {return m_data;}
	byte *end() const {return m_data + m_size;}
	unsigned int size() const {return m_size;}

private:
	byte *m_data;
	unsigned int m_size;
};

class CRYPTOPP_DLL CombinedNameValuePairs : public NameValuePairs
{
public:
	CombinedNameValuePairs(const NameValuePairs &pairs1, const NameValuePairs &pairs2)
		: m_pairs1(pairs1), m_pairs2(pairs2) {}

	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;

private:
	const NameValuePairs &m_pairs1, &m_pairs2;
};

template <class T, class BASE>
class GetValueHelperClass
{
public:
	GetValueHelperClass(const T *pObject, const char *name, const std::type_info &valueType, void *pValue, const NameValuePairs *searchFirst)
		: m_pObject(pObject), m_name(name), m_valueType(&valueType), m_pValue(pValue), m_found(false), m_getValueNames(false)
	{
		if (strcmp(m_name, "ValueNames") == 0)
		{
			m_found = m_getValueNames = true;
			NameValuePairs::ThrowIfTypeMismatch(m_name, typeid(std::string), *m_valueType);
			if (searchFirst)
				searchFirst->GetVoidValue(m_name, valueType, pValue);
			if (typeid(T) != typeid(BASE))
				pObject->BASE::GetVoidValue(m_name, valueType, pValue);
			((*reinterpret_cast<std::string *>(m_pValue) += "ThisPointer:") += typeid(T).name()) += ';';
		}

		if (!m_found && strncmp(m_name, "ThisPointer:", 12) == 0 && strcmp(m_name+12, typeid(T).name()) == 0)
		{
			NameValuePairs::ThrowIfTypeMismatch(m_name, typeid(T *), *m_valueType);
			*reinterpret_cast<const T **>(pValue) = pObject;
			m_found = true;
			return;
		}

		if (!m_found && searchFirst)
			m_found = searchFirst->GetVoidValue(m_name, valueType, pValue);
		
		if (!m_found && typeid(T) != typeid(BASE))
			m_found = pObject->BASE::GetVoidValue(m_name, valueType, pValue);
	}

	operator bool() const {return m_found;}

	template <class R>
	GetValueHelperClass<T,BASE> & operator()(const char *name, const R & (T::*pm)() const)
	{
		if (m_getValueNames)
			(*reinterpret_cast<std::string *>(m_pValue) += name) += ";";
		if (!m_found && strcmp(name, m_name) == 0)
		{
			NameValuePairs::ThrowIfTypeMismatch(name, typeid(R), *m_valueType);
			*reinterpret_cast<R *>(m_pValue) = (m_pObject->*pm)();
			m_found = true;
		}
		return *this;
	}

	GetValueHelperClass<T,BASE> &Assignable()
	{
		if (m_getValueNames)
			((*reinterpret_cast<std::string *>(m_pValue) += "ThisObject:") += typeid(T).name()) += ';';
		if (!m_found && strncmp(m_name, "ThisObject:", 11) == 0 && strcmp(m_name+11, typeid(T).name()) == 0)
		{
			NameValuePairs::ThrowIfTypeMismatch(m_name, typeid(T), *m_valueType);
			*reinterpret_cast<T *>(m_pValue) = *m_pObject;
			m_found = true;
		}
		return *this;
	}

private:
	const T *m_pObject;
	const char *m_name;
	const std::type_info *m_valueType;
	void *m_pValue;
	bool m_found, m_getValueNames;
};

template <class BASE, class T>
GetValueHelperClass<T, BASE> GetValueHelper(const T *pObject, const char *name, const std::type_info &valueType, void *pValue, const NameValuePairs *searchFirst=NULL, BASE *dummy=NULL)
{
	return GetValueHelperClass<T, BASE>(pObject, name, valueType, pValue, searchFirst);
}

template <class T>
GetValueHelperClass<T, T> GetValueHelper(const T *pObject, const char *name, const std::type_info &valueType, void *pValue, const NameValuePairs *searchFirst=NULL)
{
	return GetValueHelperClass<T, T>(pObject, name, valueType, pValue, searchFirst);
}

// ********************************************************

template <class R>
R Hack_DefaultValueFromConstReferenceType(const R &)
{
	return R();
}

template <class R>
bool Hack_GetValueIntoConstReference(const NameValuePairs &source, const char *name, const R &value)
{
	return source.GetValue(name, const_cast<R &>(value));
}

template <class T, class BASE>
class AssignFromHelperClass
{
public:
	AssignFromHelperClass(T *pObject, const NameValuePairs &source)
		: m_pObject(pObject), m_source(source), m_done(false)
	{
		if (source.GetThisObject(*pObject))
			m_done = true;
		else if (typeid(BASE) != typeid(T))
			pObject->BASE::AssignFrom(source);
	}

	template <class R>
	AssignFromHelperClass & operator()(const char *name, void (T::*pm)(R))	// VC60 workaround: "const R &" here causes compiler error
	{
		if (!m_done)
		{
			R value = Hack_DefaultValueFromConstReferenceType(reinterpret_cast<R>(*(int *)NULL));
			if (!Hack_GetValueIntoConstReference(m_source, name, value))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name + "'");
			(m_pObject->*pm)(value);
		}
		return *this;
	}

	template <class R, class S>
	AssignFromHelperClass & operator()(const char *name1, const char *name2, void (T::*pm)(R, S))	// VC60 workaround: "const R &" here causes compiler error
	{
		if (!m_done)
		{
			R value1 = Hack_DefaultValueFromConstReferenceType(reinterpret_cast<R>(*(int *)NULL));
			if (!Hack_GetValueIntoConstReference(m_source, name1, value1))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name1 + "'");
			S value2 = Hack_DefaultValueFromConstReferenceType(reinterpret_cast<S>(*(int *)NULL));
			if (!Hack_GetValueIntoConstReference(m_source, name2, value2))
				throw InvalidArgument(std::string(typeid(T).name()) + ": Missing required parameter '" + name2 + "'");
			(m_pObject->*pm)(value1, value2);
		}
		return *this;
	}

private:
	T *m_pObject;
	const NameValuePairs &m_source;
	bool m_done;
};

template <class BASE, class T>
AssignFromHelperClass<T, BASE> AssignFromHelper(T *pObject, const NameValuePairs &source, BASE *dummy=NULL)
{
	return AssignFromHelperClass<T, BASE>(pObject, source);
}

template <class T>
AssignFromHelperClass<T, T> AssignFromHelper(T *pObject, const NameValuePairs &source)
{
	return AssignFromHelperClass<T, T>(pObject, source);
}

// ********************************************************

// This should allow the linker to discard Integer code if not needed.
CRYPTOPP_DLL extern bool (*AssignIntToInteger)(const std::type_info &valueType, void *pInteger, const void *pInt);

CRYPTOPP_DLL const std::type_info & IntegerTypeId();

class CRYPTOPP_DLL AlgorithmParametersBase : public NameValuePairs
{
public:
	class ParameterNotUsed : public Exception
	{
	public: 
		ParameterNotUsed(const char *name) : Exception(OTHER_ERROR, std::string("AlgorithmParametersBase: parameter \"") + name + "\" not used") {}
	};

	AlgorithmParametersBase(const char *name, bool throwIfNotUsed)
		: m_name(name), m_throwIfNotUsed(throwIfNotUsed), m_used(false) {}

	~AlgorithmParametersBase()
	{
#ifdef CRYPTOPP_UNCAUGHT_EXCEPTION_AVAILABLE
		if (!std::uncaught_exception())
#else
		try
#endif
		{
			if (m_throwIfNotUsed && !m_used)
				throw ParameterNotUsed(m_name);
		}
#ifndef CRYPTOPP_UNCAUGHT_EXCEPTION_AVAILABLE
		catch(...)
		{
		}
#endif
	}

	bool GetVoidValue(const char *name, const std::type_info &valueType, void *pValue) const;

protected:
	virtual void AssignValue(const char *name, const std::type_info &valueType, void *pValue) const =0;
	virtual const NameValuePairs & GetParent() const =0;

	const char *m_name;
	bool m_throwIfNotUsed;
	mutable bool m_used;
};

template <class T>
class AlgorithmParametersBase2 : public AlgorithmParametersBase
{
public:
	AlgorithmParametersBase2(const char *name, const T &value, bool throwIfNotUsed) : AlgorithmParametersBase(name, throwIfNotUsed), m_value(value) {}

	void AssignValue(const char *name, const std::type_info &valueType, void *pValue) const
	{
		// special case for retrieving an Integer parameter when an int was passed in
		if (!(AssignIntToInteger != NULL && typeid(T) == typeid(int) && AssignIntToInteger(valueType, pValue, &m_value)))
		{
			ThrowIfTypeMismatch(name, typeid(T), valueType);
			*reinterpret_cast<T *>(pValue) = m_value;
		}
	}

protected:
	T m_value;
};

template <class PARENT, class T>
class AlgorithmParameters : public AlgorithmParametersBase2<T>
{
public:
	AlgorithmParameters(const PARENT &parent, const char *name, const T &value, bool throwIfNotUsed)
		: AlgorithmParametersBase2<T>(name, value, throwIfNotUsed), m_parent(parent)
	{}

	AlgorithmParameters(const AlgorithmParameters &copy)
		: AlgorithmParametersBase2<T>(copy), m_parent(copy.m_parent)
	{
		copy.m_used = true;
	}

	template <class R>
	AlgorithmParameters<AlgorithmParameters<PARENT,T>, R> operator()(const char *name, const R &value) const
	{
		return AlgorithmParameters<AlgorithmParameters<PARENT,T>, R>(*this, name, value, m_throwIfNotUsed);
	}

	template <class R>
	AlgorithmParameters<AlgorithmParameters<PARENT,T>, R> operator()(const char *name, const R &value, bool throwIfNotUsed) const
	{
		return AlgorithmParameters<AlgorithmParameters<PARENT,T>, R>(*this, name, value, throwIfNotUsed);
	}

private:
	const NameValuePairs & GetParent() const {return m_parent;}
	PARENT m_parent;
};

//! Create an object that implements NameValuePairs for passing parameters
/*! \param throwIfNotUsed if true, the object will throw an exception if the value is not accessed
	\note throwIfNotUsed is ignored if using a compiler that does not support std::uncaught_exception(),
	such as MSVC 7.0 and earlier.
	\note A NameValuePairs object containing an arbitrary number of name value pairs may be constructed by
	repeatedly using operator() on the object returned by MakeParameters, for example:
	const NameValuePairs &parameters = MakeParameters(name1, value1)(name2, value2)(name3, value3);
*/
template <class T>
AlgorithmParameters<NullNameValuePairs,T> MakeParameters(const char *name, const T &value, bool throwIfNotUsed = true)
{
	return AlgorithmParameters<NullNameValuePairs,T>(g_nullNameValuePairs, name, value, throwIfNotUsed);
}

#define CRYPTOPP_GET_FUNCTION_ENTRY(name)		(Name::name(), &ThisClass::Get##name)
#define CRYPTOPP_SET_FUNCTION_ENTRY(name)		(Name::name(), &ThisClass::Set##name)
#define CRYPTOPP_SET_FUNCTION_ENTRY2(name1, name2)	(Name::name1(), Name::name2(), &ThisClass::Set##name1##And##name2)

NAMESPACE_END

#endif
