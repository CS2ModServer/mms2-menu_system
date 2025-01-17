/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Menu System
 * Written by Wend4r & komashchenko (Vladimir Ezhikov & Borys Komashchenko).
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

#ifndef _INCLUDE_METAMOD_SOURCE_MENU_SCHEMA_HPP_
#	define _INCLUDE_METAMOD_SOURCE_MENU_SCHEMA_HPP_

#	pragma once

#	include <array>
#	include <cstddef>
#	include <type_traits>

#	include <tier0/dbg.h>
#	include <tier0/platform.h>
#	include <tier0/utlstring.h>
#	include <tier1/utlsymbollarge.h>
#	include <tier1/utlvector.h>
#	include <tier1/utlmap.h>
#	include <entity2/entityinstance.h>

#	include <concat.hpp>
#	include <gamedata.hpp>

#	define INVALID_SCHEMA_FIELD_OFFSET -1
#	define INVALID_SCHEMA_FIELD_ARRAY_SIZE -1

#	define MAX_SCHEMA_TYPE_NAME_SIZE 256

#	ifdef _DEBUG
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
		Assert(fieldSizeVar != INVALID_SCHEMA_FIELD_ARRAY_SIZE); \
	\
		return {{pInstance, static_cast<uintp>(fieldOffsetVar)}, static_cast<uintp>(fieldSizeVar)}; \
	}

#	define SCHEMA_METHOD_ARRAY_ACCESSOR2(methodName, classType, fieldAccessor, fieldType, fieldOffsetVar, fieldSizeVar) \
	SCHEMA_FORCEINLINE fieldAccessor<classType, fieldType> methodName(classType *pComponent) \
	{ \
		Assert(fieldOffsetVar != INVALID_SCHEMA_FIELD_OFFSET); \
		Assert(fieldSizeVar != INVALID_SCHEMA_FIELD_ARRAY_SIZE); \
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

class CConcatLineString;

template<class T, class CLASS>
T entity_upper_cast(CLASS aEntity)
{
	if constexpr (std::is_pointer_v<T>)
	{
		return reinterpret_cast<T>(aEntity);
	}

	return static_cast<T>(aEntity);
}

namespace Menu
{
	namespace Schema
	{
		class CSystem
		{
		public:
			CSystem();

			using CBufferStringVector = GameData::CBufferStringVector;

			struct DetailsBase_t
			{
				DetailsBase_t() = delete;

				CBufferStringVector *m_pMessages;
			}; // Menu::Schema::CSystem::DetailsBase_t

			template<std::size_t N>
			struct DetailsConcatBase_t : DetailsBase_t
			{
				using Base_t = DetailsBase_t;

				DetailsConcatBase_t() = delete;

				static_assert(N != 0, "Template parameter N (number of nests) must not be 0. Use \"DetailsBase_t\" instead");
				static_assert(N <= g_arrEmbedsConcat.size(), "Template parameter N (number of nests) over the limit");
				static constexpr std::size_t sm_nEmbeds = N;

				std::array<const CConcatLineString *, N> m_pConcats; // From more nested to less.
			}; // Menu::Schema::CSystem::DetailsConcatBase_t<N>

			template<std::size_t N>
			using Details_t = DetailsConcatBase_t<N>;

			static constexpr std::size_t sm_nMaxDetailsNesting = 8;
			static_assert(sm_nMaxDetailsNesting <= g_arrEmbedsConcat.size(), "Number of max details nestings over the limit");

			template<std::size_t N>
			using NestingDetails_t = DetailsConcatBase_t<sm_nMaxDetailsNesting - N>;

			using FullDetails_t = NestingDetails_t<1>;
			using SingleDetails_t = Details_t<1>;

			using TypeScopeDetails_t =      NestingDetails_t<1>;
			using ClassDetails_t =          NestingDetails_t<3>;
			using ClassTypeDetails_t =      NestingDetails_t<4>;
			using BaseClassTypeDetails_t =  NestingDetails_t<4>;
			using FieldDetails_t =          NestingDetails_t<5>;
			using FieldTypeDetails_t =      NestingDetails_t<6>;
			using MetadataDetails_t =       NestingDetails_t<6>;
			using MetadataEntryDetails_t =  NestingDetails_t<7>;

		protected:
			using CDetailsConcatImpl = SingleDetails_t;

			abstract_class IDetailsConcat
			{
			public:
				virtual void AppendHeader() = 0;
				virtual void AppendMembers() = 0;
				virtual void AppendEmpty() = 0;
			}; // Menu::Schema::CSystem::IDetailsConcat

			class CDetailsConcatBase : private CDetailsConcatImpl, public IDetailsConcat
			{
			public:
				using Impl = CDetailsConcatImpl;

				template<class T, uintp N = T::sm_nEmbeds, typename std::enable_if_t<std::is_base_of_v<DetailsConcatBase_t<N>, T>, int> = 0>
				CDetailsConcatBase(const T *pDetails)
				 :  Impl({{pDetails->m_pMessages}, {pDetails->m_pConcats[N - 1]}}) // Go down to a single concat.
				{
					Assert(pDetails);
				}

			protected:
				CBufferStringVector *GetMessages();
				const CConcatLineString *GetConcatLine() const;

			public:
				void AppendHeader() override;
				void AppendMembers() override;
				void AppendEmpty() override;
			}; // Menu::Schema::CSystem::CDetailsConcatBase

			class CDetailsConcatTypeScope : public CDetailsConcatBase
			{
			public:
				using Base = CDetailsConcatBase;
				using Base::Base;

				CDetailsConcatTypeScope(const Base &aBase, const CSchemaSystemTypeScope *pData)
				 :  Base(aBase), 
				    m_pData(pData)
				{
				}

			public:
				void AppendHeader() override;
				void AppendMembers() override;
				virtual void AppendClasses();

			private:
				const CSchemaSystemTypeScope *m_pData;
			}; // Menu::Schema::CSystem::CDetailsConcatTypeScope

			class CDetailsConcatType : public CDetailsConcatBase
			{
			public:
				using Base = CDetailsConcatBase;

				CDetailsConcatType(const Base &aBase, const CSchemaType *pData)
				 :  Base(aBase), 
				    m_pData(pData)
				{
				}

			public:
				void AppendHeader() override;
				void AppendMembers() override;

			private:
				const CSchemaType *m_pData;
			}; // Menu::Schema::CSystem::CDetailsConcatType

			class CDetailsConcatClass : public CDetailsConcatBase
			{
			public:
				using Base = CDetailsConcatBase;

				CDetailsConcatClass(const Base &aBase, const SchemaClassInfoData_t *pData)
				 :  Base(aBase), 
				    m_pData(pData)
				{
				}

			public:
				void AppendHeader() override;
				void AppendMembers() override;
				virtual void AppendBaseClasses();
				virtual void AppendFields();

			private:
				const SchemaClassInfoData_t *m_pData;
			}; // Menu::Schema::CSystem::CDetailsConcatClass

			class CDetailsConcatField : public CDetailsConcatBase
			{
			public:
				using Base = CDetailsConcatBase;

				CDetailsConcatField(const Base &aBase, const SchemaClassFieldData_t *pData)
				 :  Base(aBase), 
				    m_pData(pData)
				{
				}

			public:
				void AppendHeader() override;
				void AppendMembers() override;
				virtual void AppendMetadataMember();

			private:
				const SchemaClassFieldData_t *m_pData;
			}; // Menu::Schema::CSystem::CDetailsConcatField

			class CDetailsConcatMetadataEntry : public CDetailsConcatBase
			{
			public:
				using Base = CDetailsConcatBase;

				CDetailsConcatMetadataEntry(const Base &aBase, const SchemaMetadataEntryData_t *pData)
				 :  Base(aBase), 
				    m_pData(pData)
				{
				}

			public:
				void AppendHeader() override;
				void AppendMembers() override;

			private:
				const SchemaMetadataEntryData_t *m_pData;
			}; // Menu::Schema::CSystem::CDetailsConcatMetadataEntry

		public:
			bool Init(ISchemaSystem *pSchemaSystem, const CUtlVector<const char *> &vecLoadedLibraries, CBufferStringVector *pMessages = nullptr);
			bool Load(FullDetails_t *pDetails = nullptr); // Calls the classes -> fields callbacks.
			void Clear();

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
				void LoadFields(CSchemaClassInfo *pInfo, FieldDetails_t *pDetails = nullptr);

			public: // Fields symbols.
				CUtlSymbolLarge GetFieldSymbol(const char *pszName);
				CUtlSymbolLarge FindFieldSymbol(const char *pszName) const;

			private:
				Fields m_aFieldStorage;
				CUtlSymbolTableLarge m_tableFileds;
			}; // Menu::Schema::CSystem::CClass

			CClass *GetClass(const char *pszName);
			CClass *FindClass(const char *pszName);
			int FindClassFieldOffset(const char *pszClassName, const char *pszFiledName); // Returns -1 if not found.

		protected:
			void LoadClasses(CSchemaSystemTypeScope *pScope, ClassDetails_t *pDetails = nullptr);
			void ClearClasses();

		public: // Class symbols.
			CUtlSymbolLarge GetClassSymbol(const char *pszName);
			CUtlSymbolLarge FindClassSymbol(const char *pszName) const;

		private:
			CUtlVector<CSchemaSystemTypeScope *> m_vecTypeScopes;

			CUtlSymbolTableLarge m_tableClasses;
			CUtlMap<CUtlSymbolLarge, CClass> m_mapClasses;
		}; // Menu::Schema::CSystem

		namespace Accessor
		{
			template<class T, typename F>
			class CField
			{
			public:
				CField() = delete;

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

				FORCEINLINE F &operator=(const F &aData)
				{
					return GetRef() = aData;
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

			private:
				T *m_pTarget;
				uintp m_nOffset;
			}; // Menu::Schema::Accessor::CField<T, F>

			template<class T, typename F>
			class CArrayField : virtual public CField<T, F>
			{
			public:
				using Base = CField<T, F>;

				CArrayField() = delete;

				FORCEINLINE CArrayField(const Base &aInitField, uintp nInitSize)
				 :  Base(aInitField), 
				    m_nSize(nInitSize)
				{
				}

			public:
				FORCEINLINE F &operator[](uintp nCell)
				{
					Assert(nCell < m_nSize);

					return Base::GetRef(nCell * sizeof(F));
				}

			private:
				uintp m_nSize;
			}; // Menu::Schema::Accessor::CArrayField<T, F>

			template<typename F>
			using CInstanceFieldBase = CField<CEntityInstance, F>;

			template<typename F>
			class CInstanceField : virtual public CInstanceFieldBase<F>
			{
			public:
				using Base = CInstanceFieldBase<F>;
				using Base::Base;
				using Base::operator F;
				using Base::operator=;
				using Base::operator->;

			public:
				FORCEINLINE void MarkNetworkChanged()
				{
					Base::GetTarget()->NetworkStateChanged(Base::GetOffset());
				}
			}; // Menu::Schema::Accessor::CInstanceField<F>

			template<typename F>
			using CInstanceArrayFieldBase = CArrayField<CEntityInstance, F>;

			template<typename F>
			class CInstanceArrayField : public CInstanceArrayFieldBase<F>, virtual public CInstanceField<F>
			{
			public:
				using Base = CInstanceArrayFieldBase<F>;
				using Base::Base;
				using Base::operator F;
				using Base::operator=;
				using Base::operator->;
				using Base::operator[];
			}; // Menu::Schema::Accessor::CInstanceArrayField<F>
		}; // Menu::Schema::Accessor
	}; // Menu::Schema
}; // Menu

#endif // _INCLUDE_METAMOD_SOURCE_MENU_SCHEMA_HPP_
