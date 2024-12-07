/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Menu System
 * Written by komashchenko & Wend4r (Borys Komashchenko & Vladimir Ezhikov).
 * ======================================================

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_HPP_

#	pragma once

#	include <tier0/dbg.h>
#	include <tier0/platform.h>
#	include <tier0/utlstring.h>
#	include <tier1/utlsymbollarge.h>
#	include <tier1/utlvector.h>
#	include <tier1/utlmap.h>
#	include <entity2/entityinstance.h>

#	include <gamedata.hpp>

#	define INVALID_SCHEMA_FIELD_OFFSET -1
#	define INVALID_SCHEMA_FIELD_ARRAY_SIZE -1

#	ifdef DEBUG
#		define SCHEMA_FORCEINLINE
#	else
#		define SCHEMA_FORCEINLINE FORCEINLINE
#	endif

#	define SCHEMA_METHOD_ACCESSOR(methodName, classType, fieldAccessor, fieldType, fieldOffsetVar) \
	SCHEMA_FORCEINLINE fieldAccessor<fieldType> methodName(classType *pInstance) \
	{ \
		Assert(fieldOffsetVar != INVALID_SCHEMA_FIELD_OFFSET); \
	\
		return {pInstance, static_cast<uintp>(fieldOffsetVar)}; \
	}

#	define SCHEMA_METHOD_ACCESSOR2(methodName, classType, fieldAccessor, fieldType, fieldOffsetVar) \
	SCHEMA_FORCEINLINE fieldAccessor<classType, fieldType> methodName(classType *pComponent) \
	{ \
		Assert(fieldOffsetVar != INVALID_SCHEMA_FIELD_OFFSET); \
	\
		return {pComponent, static_cast<uintp>(fieldOffsetVar)}; \
	}

#	define SCHEMA_METHOD_ARRAY_ACCESSOR(methodName, classType, fieldAccessor, fieldType, fieldOffsetVar, fieldSizeVar) \
	SCHEMA_FORCEINLINE fieldAccessor<fieldType> methodName(classType *pInstance) \
	{ \
		Assert(fieldOffsetVar != INVALID_SCHEMA_FIELD_OFFSET); \
	\
		return {{pInstance, static_cast<uintp>(fieldOffsetVar)}, static_cast<uintp>(fieldSizeVar)}; \
	}

#	define SCHEMA_METHOD_ARRAY_ACCESSOR2(methodName, classType, fieldAccessor, fieldType, fieldOffsetVar, fieldSizeVar) \
	SCHEMA_FORCEINLINE fieldAccessor<classType, fieldType> methodName(classType *pComponent) \
	{ \
		Assert(fieldOffsetVar != INVALID_SCHEMA_FIELD_OFFSET); \
	\
		return {{pComponent, static_cast<uintp>(fieldOffsetVar)}, static_cast<uintp>(fieldSizeVar)}; \
	}

#	define SCHEMA_COMPONENT_ACCESSOR_METHOD(methodName, classType, fieldType, fieldOffsetVar) SCHEMA_METHOD_ACCESSOR2(methodName, classType, Accessor::CField, fieldType, fieldOffsetVar)
#	define SCHEMA_COMPONENT_ARRAY_ACCESSOR_METHOD(methodName, classType, fieldType, fieldOffsetVar, fieldSizeVar) SCHEMA_METHOD_ARRAY_ACCESSOR2(methodName, classType, Accessor::CArrayField, fieldType, fieldOffsetVar, fieldSizeVar)

#	define SCHEMA_INSTANCE_ACCESSOR_METHOD(methodName, classType, fieldType, fieldOffsetVar) SCHEMA_METHOD_ACCESSOR(methodName, classType, Accessor::CInstanceField, fieldType, fieldOffsetVar)
#	define SCHEMA_INSTANCE_ARRAY_ACCESSOR_METHOD(methodName, classType, fieldType, fieldOffsetVar, fieldSizeVar) SCHEMA_METHOD_ARRAY_ACCESSOR(methodName, classType, Accessor::CInstanceArrayField, fieldType, fieldOffsetVar, fieldSizeVar)

class ISchemaSystem;
class CSchemaClassInfo;
class CSchemaSystemTypeScope;
class CSchemaType_DeclaredClass;
struct SchemaClassFieldData_t;

namespace MenuSystem
{
	namespace Schema
	{
		class CSystem
		{
		public:
			CSystem();

		public:
			bool Init(ISchemaSystem *pSchemaSystem, const CUtlVector<const char *> &vecLoadedLibraries, GameData::CBufferStringVector *pMessages = nullptr);
			void Destroy();

		public:
			class CClass
			{
				friend class CSystem;

			public:
				using Fields = GameData::Config::Storage<CUtlSymbolLarge, SchemaClassFieldData_t *>;

				Fields &GetFields();

				SchemaClassFieldData_t *GetField(const CUtlSymbolLarge &sName) const;
				void SetField(const CUtlSymbolLarge &sName, SchemaClassFieldData_t *pData);

				SchemaClassFieldData_t *FindField(const char *pszName) const;
				int FindFieldOffset(const char *pszName) const; // Returns -1 if not found.

			protected:
				void ParseFields(CSchemaClassInfo *pInfo);

			public: // Fields symbols.
				CUtlSymbolLarge GetFieldSymbol(const char *pszName);
				CUtlSymbolLarge FindFieldSymbol(const char *pszName) const;

			private:
				Fields m_aFieldStorage;
				CUtlSymbolTableLarge m_tableFileds;
			};

			CClass *GetClass(const char *pszName);
			CClass *FindClass(const char *pszName);
			int FindClassFieldOffset(const char *pszClassName, const char *pszFiledName); // Returns -1 if not found.

		protected:
			void ParseClasses(CSchemaSystemTypeScope *pType);
			void ClearClasses();

		public: // Class symbols.
			CUtlSymbolLarge GetClassSymbol(const char *pszName);
			CUtlSymbolLarge FindClassSymbol(const char *pszName) const;

		private:
			CUtlVector<CSchemaSystemTypeScope *> m_vecTypeScopes;

			CUtlSymbolTableLarge m_tableClasses;
			CUtlMap<CUtlSymbolLarge, CClass> m_mapClasses;
		}; // CClass

		namespace Accessor
		{
			template<class T, typename F>
			class CField
			{
			public:
				FORCEINLINE CField(T *pInitTarget, uintp nInitOffset)
				:  m_pTarget(pInitTarget), 
				   m_nOffset(nInitOffset)
				{
				}

			public:
				FORCEINLINE operator F()
				{
					return GetRef();
				}

				FORCEINLINE F *operator->()
				{
					return &GetRef();
				}

			protected:
				FORCEINLINE T *GetTarget()
				{
					return m_pTarget;
				}

				FORCEINLINE uintp GetOffset()
				{
					return m_nOffset;
				}

				FORCEINLINE F &GetRef(uintp nExtraOffset = 0)
				{
					return *reinterpret_cast<F *>(reinterpret_cast<uintp>(GetTarget()) + GetOffset() + nExtraOffset);
				}

				FORCEINLINE F &operator=(const F &aData)
				{
					return GetRef() = aData;
				}

			private:
				T *m_pTarget;
				uintp m_nOffset;
			}; // MenuSystem::Schema::Accessor::CField<T, F>

			template<class T, typename F>
			class CArrayField : virtual public CField<T, F>
			{
			public:
				using Base = CField<T, F>;

				FORCEINLINE CArrayField(const Base &aInitField, uintp nInitSize)
				 :  Base(aInitField), 
				    m_nSize(nInitSize)
				{
				}

			public:
				FORCEINLINE F &operator[](uintp nCell)
				{
					Assert(nCell < m_nSize);

					return Base::GetRef(nCell * sizeof(F *));
				}

			private:
				uintp m_nSize;
			}; // MenuSystem::Schema::Accessor::CArrayField<T, F>

			template<typename F>
			using CInstanceFieldBase = CField<CEntityInstance, F>;

			template<typename F>
			class CInstanceField : virtual public CInstanceFieldBase<F>
			{
			public:
				using Base = CInstanceFieldBase<F>;
				using Base::Base;
				using Base::operator=;
				using Base::operator->;

			public:
				FORCEINLINE void MarkNetworkChanged()
				{
					Base::GetTarget()->NetworkStateChanged(Base::GetOffset());
				}
			}; // MenuSystem::Schema::Accessor::CInstanceField<F>

			template<typename F>
			using CInstanceArrayFieldBase = CArrayField<CEntityInstance, F>;

			template<typename F>
			class CInstanceArrayField : public CInstanceArrayFieldBase<F>, virtual public CInstanceField<F>
			{
			public:
				using Base = CInstanceArrayFieldBase<F>;
				using Base::Base;
				using Base::operator=;
				using Base::operator->;
				using Base::operator[];
			}; // MenuSystem::Schema::Accessor::CInstanceArrayField<F>
		}; // MenuSystem::Schema::Accessor
	}; // MenuSystem::Schema
}; // MenuSystem

#endif // _INCLUDE_METAMOD_SOURCE_MENU_SYSTEM_SCHEMA_HPP_
